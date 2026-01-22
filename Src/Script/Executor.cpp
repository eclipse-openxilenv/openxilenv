/*
 * Copyright 2023 ZF Friedrichshafen AG
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern "C" {
#include "MyMemory.h"
#include "PrintFormatToString.h"
#include "ConfigurablePrefix.h"
#include "Files.h"
#include "Scheduler.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "MainValues.h"
#include "ScriptHtmlFunctions.h"
#include "ScriptStartExe.h"
}
#include "Executor.h"


cExecutor::cExecutor (void)
{
    CmdTable = nullptr;
    InsideProc = nullptr;
    ProcTable = nullptr;

    CurrentIp = 0;
    NextIp = 0;

    CurrentCmdIdx = 0;

    IfFlag = 0; 
    WhileFlag = 0; 
    DefProcFlag = 0;

    DataForCmd = 0;

    MEMSET (DataArrayForCmd, 0, sizeof (DataArrayForCmd));

    NextCmdInSameCycleFlag = 0;

    AtomicCounter = 0;

    TimeoutActiveFlag = 0;
    StartTimeoutCycle = 0; 
    CurrentCycle = 0; 
    AtWhichCycleTimeoutReached = 0;


    HtmlReportFile = nullptr;
    HtmlReportFilename[0] = 0;

    SingleStep = 0;
    StopAtIp = -1;

    char Name[BBVARI_NAME_SIZE];
    ExitCodeVid = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SHORT_BLACKBOARD, "ExitCode", Name, sizeof(Name)), BB_DWORD, "intern");
    StartExeProcessHandle = INVALID_HANDLE_VALUE;

    CanRecorder = nullptr;
}

cExecutor::~cExecutor (void)
{
    remove_bbvari (ExitCodeVid);
}

int cExecutor::ExecuteNextCmd (cParser *par_Parser)
{
    int FileNr; 
    int LineNr;
    uint32_t FileOffset;
    cFileCache *FileCache;

    // Some commands can be executed as a pair
    for (;;) {
        NextCmdInSameCycleFlag = 0;
        CurrentIp = NextIp;

        // Breakpoint start
        if (SingleStep) {
            if (SingleStep == 1) {
                if (StopAtIp == -1) {   
                    SingleStep = 0;
                    return 0xB;   // Step in
                } else {
                    if (StopAtIp == CurrentIp) {
                        SingleStep = 0;
                        StopAtIp = -1;
                        return 0xB;    // Step over
                    }
                }
            } else if (SingleStep == 2) {
                SingleStep = 1;
            } else {   // == -1  ignore breakpoint
                SingleStep = 0;
            }
        } else {
            int BreakPointIdx;
            if ((BreakPointIdx = CmdTable->GetBreakPointAtCmd (static_cast<uint32_t>(CurrentIp))) >= 0) {
                if (BreakPoints.CheckIfBreakPointHit (BreakPointIdx)) {
                    return 0xB;   // Hit breakpoint
                }
            }
        }
        // Breakpoint end

        NextIp++;
        CurrentCmdIdx = CmdTable->GetCmdInfos (static_cast<uint32_t>(CurrentIp), &FileNr, &LineNr, &FileOffset);

        if (CurrentCmdIdx >= 0) {
            FileCache = cFileCache::IsFileCached (FileNr);
            if (FileCache == nullptr) {
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error %s %i", __FILE__, __LINE__);
                return -1;  //  -1 Error or 0xE0F for end of the main script
            }
            FileCache->Fseek (static_cast<int32_t>(FileOffset), SEEK_SET, LineNr);
            par_Parser->SetCurrentScript (FileCache, FileNr);
            if (CurrentCmdIdx != par_Parser->ParseNextCommand ()) {
                return -1;  //  -1 Error or 0xE0F for end of the main script
            }
            if (par_Parser->GenDebugFile ()) {
                par_Parser->PrintCurrentCommandToDebugFile (CurrentIp, AtomicCounter);
            }
            int Ret = par_Parser->CallActualCommandExecuteFunc (this, CurrentCmdIdx);
            if (par_Parser->GenDebugFile ()) {
                par_Parser->PrintDebugFile ("\n");
            }
            if (Ret) {
                return Ret;  //  -1 Error or 0xE0F for end of the main script
            }
        } else {
            return -1; // -1 Error or 0xE0F for end of the main script
        }
        if (AtomicCounter || NextCmdInSameCycleFlag) {
            do {
                if (AtomicCounter) {
                    if (int Ret = par_Parser->CheckIfCommandInsideAtomicAllowed (CurrentCmdIdx)) {
                        return Ret;
                    }
                }
            } while (WaitForResponseOfLastCmd (par_Parser, 0));
        } else {
            break;
        }
    }
    return 0;
}


int cExecutor::WaitForResponseOfLastCmd (cParser *par_Parser,uint32_t par_Cycles)
{
    if (CanRecorder != nullptr) {
        if (CanRecorderReadAndProcessMessages(CanRecorder)) {
            FreeCanRecorder(CanRecorder);
        }
    }
    if (!TimeoutActiveFlag) {
        return 0;
    }
    CurrentCycle = GetCycleCounter ();
    int Ret = par_Parser->CallActualCommandWaitFunc (this, CmdTable->GetCmdIdx (static_cast<uint32_t>(CurrentIp)), par_Cycles);
    if (Ret) {
        // Check if timeout
        if (CurrentCycle >= AtWhichCycleTimeoutReached) {
            TimeoutActiveFlag = 0;
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Timeout reached at current command"); 
            return 0;
        }
    } else {
        TimeoutActiveFlag = 0;
    }
    return Ret; 
}

void cExecutor::StartTimeoutControl (int par_Timeout)
{
    TimeoutActiveFlag = 1;
    StartTimeoutCycle = CurrentCycle = GetCycleCounter (); 
    if (par_Timeout < 0) {
        AtWhichCycleTimeoutReached = 0xFFFFFFFF;
    } else {
        AtWhichCycleTimeoutReached = CurrentCycle + static_cast<uint32_t>(par_Timeout);
    }
}


int cExecutor::GetWaitCyclesNeeded (void)
{
    return static_cast<int>(CurrentCycle - StartTimeoutCycle);
}

int cExecutor::ReopenHtmlReportFile (void)
{
    HtmlReportFile = open_file (HtmlReportFilename, "a");
    return (HtmlReportFile == nullptr) ? -1 : 0;
}

int cExecutor::ReopenHtmlReportFile (char *par_HtmlReportFilename)
{
    CloseHtmlReportFile ();
    STRING_COPY_TO_ARRAY (HtmlReportFilename, par_HtmlReportFilename);
    HtmlReportFile = open_file (HtmlReportFilename, "a");
    return (HtmlReportFile == nullptr) ? -1 : 0;
}

int cExecutor::OpenNewHtmlReportFile (char *par_HtmlReportFilename)
{
    CloseHtmlReportFile ();
    STRING_COPY_TO_ARRAY (HtmlReportFilename, par_HtmlReportFilename);
    HtmlReportFile = open_file (HtmlReportFilename, "w");
    return (HtmlReportFile == nullptr) ? -1 : 0;
}


void cExecutor::CloseHtmlReportFile (void)
{
    if (HtmlReportFile != nullptr) {
        close_file (HtmlReportFile);
    }
    HtmlReportFile = nullptr;
}


void cExecutor::Init (cCmdTable *par_CmdTable, cProcTable *par_ProcTable, cProc *par_MainProc)
{
    Stack.Reset ();
    AtomicCounter = 0;

    CmdTable = par_CmdTable;
    ProcTable = par_ProcTable;
    InsideProc = par_MainProc;

    CurrentIp = 0;
    NextIp = 0;
    NextCmdInSameCycleFlag = 0;

    IfFlag = 0;
    WhileFlag = 0;
    DefProcFlag = 0;
    DataForCmd = 0;
    MEMSET (DataArrayForCmd, 0, sizeof (DataArrayForCmd));
    TimeoutActiveFlag = 0;
    AtomicCounter = 0;

    StartTimeoutCycle = 0;
    CurrentCycle = 0;
    AtWhichCycleTimeoutReached = 0;

    SingleStep = 0;
    StopAtIp = -1;

    // generate report file
    PrintFormatToString (HtmlReportFilename, sizeof(HtmlReportFilename), "%sreport.html", s_main_ini_val.ScriptOutputFilenamesPrefix);
    HtmlReportFile = GenerateHTMLReportFile (HtmlReportFilename);
    init_html_strings ();
}

void cExecutor::EndOfExecution (void)
{
    CloseHTMLReportFile (HtmlReportFile);
}

void cExecutor::SetStopScriptAtStartDebuggingFlag(int Flag)
{
    if (CmdTable != nullptr) {
        int FileNr = CmdTable->GetFileNr(0);
        int LineNr = CmdTable->GetLineNr(0);
        const char *Filename = cFileCache::GetFilenameByNr (FileNr);
        if (Flag) {
            AddBreakPoint (1, FileNr, Filename, LineNr, 0, 0, nullptr);
        } else {
            DelBreakPoint (Filename, LineNr);
        }
    }
}

void cExecutor::DetachAllBbvaris()
{
    int Pid = get_pid_by_name("Script");
    if (Pid > 0) {
        remove_all_bbvari(Pid);
        // now we will attach this 2 variables again
        attach_bbvari_by_name("Script", Pid);
        attach_bbvari_by_name("ExitCode", Pid);
    }
}

