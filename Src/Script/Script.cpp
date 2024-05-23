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
#include "Config.h"
#include "MyMemory.h"
#include "ConfigurablePrefix.h"
#include "Files.h"
#include "Scheduler.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "EquationParser.h"
#include "Message.h"
#include "FileExtensions.h"
#include "ScriptDebugFile.h"
#include "ScriptMessageFile.h"
#include "ScriptErrorFile.h"
#include "ScriptList.h"
#include "ScriptChangeSettings.h"
#include "MainValues.h"
}
#include "InterfaceToScript.h"
#include "Script.h"
#include "ScriptDebuggingDialog.h"
#include "MainWinowSyncWithOtherThreads.h"

#define UNUSED(x) (void)(x)

cScript::cScript (void)
{
    InitializeCriticalSection(&CriticalSection);
    StatusFlagVid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SCRIPT), BB_UBYTE, "intern");
    set_bbvari_conversion (StatusFlagVid, 2, "0 0 \"Stop\"; 1 1 \"Start\"; 2 2 \"Sleep\"; 3 3 \"Running\"; 4 4 \"Ready\"; 5 5 \"Finished\"; 6 6 \"Error\"; 7 7 \"Debug hold\";");
}

cScript::~cScript (void)
{
    remove_bbvari (StatusFlagVid);
}

int cScript::Start (char *par_Filename)
{
    int Ret;

    cProc *MainProc = Parser.Init (&CmdTable, &ProcTable);
    Executor.Init (&CmdTable, &ProcTable, MainProc);
    CycleCounter = 0;

    ResetChangeSettingsStack();

    char Filename[MAX_PATH];

    // internal only use absolute pathes
    if (expand_filename (par_Filename, Filename, sizeof(Filename)) != 0) {
        return -1;
    }

    // First make a syntax check
    Ret = Parser.SyntaxCheck (par_Filename);
    if (Ret) {
        CloseScriptErrorFile ();
        State = SCRIPT_STATE_ERROR_SCT;
        ExitIfSyntaxErrorInScript();
        return Ret;
    }
    CloseScriptErrorFile ();
    State = SCRIPT_STATE_RUNNING;
    Parser.SetRunningFlag (); // now the script is running

    GenerateNewDebugFile ();

    CycleCounter = 0;

    if (IsScriptDebugWindowOpen ()) {
         Executor.SaveBreakPointsToIni ();
         Executor.InitBreakPointsFromIni ();
         // Set a breakpoint on the first command if "Stop at start" is active
         Executor.SetStopScriptAtStartDebuggingFlag (GetStopScriptAtStartDebuggingFlag ());
    } else {
        // If script debugging dialog is not open delete the breakpoint
        Executor.SetStopScriptAtStartDebuggingFlag (0);
    }

    return 0;
}

void cScript::SendResponseToDebugWindow (void)
{
}


double ReadVariableFromBlackboard (const char *name) {
    long vid;
    double value;

    if ((vid = add_bbvari (name, BB_UNKNOWN, NULL)) < 0) {
        return 0.0;
    }
    value = read_bbvari_convert_double (vid);
    remove_bbvari (vid);
    return value;
}

void cScript::CleanUpEndOfScriptReached (void)
{
    CloseScriptDebugFile ();
    CloseScriptMessageFile (0);  // close the file really (no reopen)
    Executor.EndOfExecution ();
    Executor.Stack.Reset ();

    /* Terminate recorder  */
    if (s_main_ini_val.StopRecorderIfScriptStopped && (ReadVariableFromBlackboard (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_TRACE_RECORDER)) != 0.0)) {
        Parser.SendMessageToProcess (nullptr, 0, PN_TRACE_RECORDER, STOP_REC_MESSAGE);
    }
    /* Terminate player  */
    if (s_main_ini_val.StopPlayerIfScriptStopped && (ReadVariableFromBlackboard (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_STIMULUS_PLAYER)) != 0.0)) {
        Parser.SendMessageToProcess (nullptr, 0, PN_STIMULI_PLAYER, HDPLAY_STOP_MESSAGE);
    }
    /* Terminate ramps */
    if (s_main_ini_val.StopGeneratorIfScriptStopped && (ReadVariableFromBlackboard (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_GENERATOR)) != 0.0)) {
        Parser.SendMessageToProcess (nullptr, 0, PN_SIGNAL_GENERATOR_COMPILER, RAMPEN_STOP);
    }
    /* Terminate equations */
    if (s_main_ini_val.StopEquationIfScriptStopped && (ReadVariableFromBlackboard (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_EQUATION_CALCULATOR)) != 0.0)) {
        Parser.SendMessageToProcess (nullptr, 0, PN_EQUATION_COMPILER, TRIGGER_STOP_MESSAGE);
    }

    ScriptDestroyDialogFromOtherThread();

    UnRegisterSchedFunc ();     // Give free the default scheduler again
    ScriptDeleteAllReferenceVariLists ();
}

int cScript::Cyclic (int par_script_status_flag)
{
    int RestDelay;

    EnterCriticalSection(&CriticalSection);
    if (State != par_script_status_flag) {  // Ask if the user have changed the script status variable
        switch (par_script_status_flag) {
        case SCRIPT_STATE_STOP:
            if (State == SCRIPT_STATE_RUNNING) {
                CleanUpEndOfScriptReached ();
                State = SCRIPT_STATE_READY; 
            }
            State = SCRIPT_STATE_STOP;
            break;
        case SCRIPT_STATE_START:
            State = SCRIPT_STATE_START;
            break;
        default:
            break; 
        }
    }        
    switch (State) {
    case SCRIPT_STATE_STOP:
        break;
    case SCRIPT_STATE_START:
        Start (script_filename);
        CycleCounter = 0;
        break;
    case SCRIPT_STATE_RUNNING:
        CycleCounter++;
        RestDelay = Executor.WaitForResponseOfLastCmd (&Parser, CycleCounter);
        if (!RestDelay) {
            switch (Executor.ExecuteNextCmd (&Parser)) {
            case 0xE0F:   // End of the main script file
                CleanUpEndOfScriptReached ();
                State = SCRIPT_STATE_READY; 
                break;
            case -1:   // Error!!
                CleanUpEndOfScriptReached ();
                State = SCRIPT_STATE_ERROR_SCT; 
                ExitIfErrorInScript ();
                break;
            case 0xB:  // Breakpoint
                 DebugStopScriptExecution ();
                break;
            }
        } else if (RestDelay == -1) {
            // Error during WAIT_UNTIL
            CleanUpEndOfScriptReached ();
            State = SCRIPT_STATE_ERROR_SCT; 
            ExitIfErrorInScript ();
        }
        break;
    case SCRIPT_STATE_DEBUG_HOLD:
    case SCRIPT_STATE_READY:
    case SCRIPT_STATE_FINISHED:
    case SCRIPT_STATE_ERROR_SCT:
        break;
    }
    write_bbvari_ubyte (StatusFlagVid, static_cast<uint8_t>(State));
    LeaveCriticalSection(&CriticalSection);
    return State;
}

int cScript::DebugStopScriptExecution (void)
{
    EnterCriticalSection(&CriticalSection);
    if (State == SCRIPT_STATE_RUNNING) {
        State = SCRIPT_STATE_DEBUG_HOLD;
        disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_USER, nullptr, nullptr);
    }
    LeaveCriticalSection(&CriticalSection);
    return 0;
}

int cScript::DebugContScriptExecution (void)
{
    EnterCriticalSection(&CriticalSection);
    if (State == SCRIPT_STATE_DEBUG_HOLD) {
        State = SCRIPT_STATE_RUNNING;
        Executor.IgnoreBreakpointAtNextStep ();
        enable_scheduler(SCHEDULER_CONTROLED_BY_USER);
    }
    LeaveCriticalSection(&CriticalSection);
    return 0;
}

int cScript::DebugStepOverScriptExecution (void)
{
    EnterCriticalSection(&CriticalSection);
    if (State == SCRIPT_STATE_DEBUG_HOLD) {
        uint32_t NextIp = static_cast<uint32_t>(Executor.GetNextIp ());
        int CmdIdx = CmdTable.GetCmdIdx (NextIp);
        const char *CmdName = cTokenizer::GetCmdToTokenName (CmdIdx);
        if (!strcmp (CmdName, "RUN") || !strcmp (CmdName, "CALL") || !strcmp (CmdName, "GOSUB") || !strcmp (CmdName, "CALL_PROC")) {
            Executor.SetSingleStep (static_cast<int>(NextIp) + 1);
        } else {
            Executor.SetSingleStep (-1);
        }
        State = SCRIPT_STATE_RUNNING;
        enable_scheduler(SCHEDULER_CONTROLED_BY_USER);
    }
    LeaveCriticalSection(&CriticalSection);
    return 0;
}

int cScript::DebugRunToScriptExecution (int par_FileNr, char *par_Filename, 
                                        int par_LineNr, int par_Ip)
{
    UNUSED(par_FileNr);
    UNUSED(par_Filename);
    UNUSED(par_LineNr);
    EnterCriticalSection(&CriticalSection);
    if (State == SCRIPT_STATE_DEBUG_HOLD) {
        Executor.SetSingleStep (par_Ip);
        State = SCRIPT_STATE_RUNNING;
        enable_scheduler(SCHEDULER_CONTROLED_BY_USER);
    }
    LeaveCriticalSection(&CriticalSection);
    return 0;
}

int cScript::DebugStepIntoScriptExecution (void)
{
    EnterCriticalSection(&CriticalSection);
    if (State == SCRIPT_STATE_DEBUG_HOLD) {
        Executor.SetSingleStep (-1);
        State = SCRIPT_STATE_RUNNING;
        enable_scheduler(SCHEDULER_CONTROLED_BY_USER);
    }
    LeaveCriticalSection(&CriticalSection);
    return 0;
}

int cScript::DebugStepOutScriptExecution (void)
{
    EnterCriticalSection(&CriticalSection);
    if (State == SCRIPT_STATE_DEBUG_HOLD) {
        int Ip = Executor.Stack.GetFirstReturnIp ();
        if (Ip >= 0) {
            Executor.SetSingleStep (Ip);
        } else {
            DebugStepOverScriptExecution ();
            return 0;
        }
        State = SCRIPT_STATE_RUNNING;
        enable_scheduler(SCHEDULER_CONTROLED_BY_USER);
    }
    LeaveCriticalSection(&CriticalSection);
    return 0;
}

int cScript::UpdateBreakPointsHighlight (int par_Flag)
{
    UNUSED(par_Flag);
    return 0;
}

int cScript::OpenScriptDebugWindow (void)
{
    Executor.InitBreakPointsFromIni ();
    return 0;
}

int cScript::CloseScriptDebugWindow (void)
{
    Executor.SaveBreakPointsToIni ();
    return 0;
}

int cScript::LoadFileContent (int par_Flag)
{
    UNUSED(par_Flag);
    return 0;
}

void cScript::ScriptEnterCriticalSection()
{
    EnterCriticalSection(&CriticalSection);
}

bool cScript::ScriptTryEnterCriticalSection()
{
    return (TryEnterCriticalSection(&CriticalSection) != 0);
}

void cScript::ScriptLeaveCriticalSection()
{
    LeaveCriticalSection(&CriticalSection);
}

int cScript::ScriptDebugGetCurrentState(int *ret_StartSelOffset, int *ret_EndSelOffset, int *ret_FileNr, int *ret_LineNr, int *ret_DelayCounter, cExecutor **ret_Executor, cParser **ret_Parser)
{
    if ((State == SCRIPT_STATE_RUNNING) ||
        (State == SCRIPT_STATE_DEBUG_HOLD)) {
        int Ip = Executor.GetCurrentIp ();
        *ret_StartSelOffset = CmdTable.GetFileOffset (Ip);
        *ret_EndSelOffset = CmdTable.GetBeginOfParamOffset (Ip);
        *ret_LineNr = CmdTable.GetLineNr (Ip);
        *ret_FileNr = CmdTable.GetFileNr (Ip);
        *ret_DelayCounter = 0;  // ToDo!
        *ret_Executor = &Executor;
        *ret_Parser = &Parser;
    } else {
        *ret_StartSelOffset = 0;
        *ret_EndSelOffset = 0;
        *ret_LineNr = -1;
        *ret_FileNr = -1;
        *ret_DelayCounter = 0;
        *ret_Executor = &Executor;
        *ret_Parser = &Parser;
    }
    return 0;
}

static cScript *Script;

int init_script (void)
{
    Script = new cScript;
    return Script->Init ();
}

void cyclic_script (void)
{
    if (Script != nullptr) {
        script_status_flag = static_cast<unsigned char>(Script->Cyclic (script_status_flag));
    }
}

void terminate_script (void)
{
    ScriptDestroyDialogFromOtherThread();
    Script->Terminate ();
    delete Script;
    Script = nullptr;
}


int GetLocalVariableValue (char *Name, double *pRet, int StackPos)
{
    return Script->GetLocalVariableValue (Name, pRet, StackPos);
}

int GetValueOfRefToLocalVariable (char *RefName, double *pRet, int StackPos, int Cmd)
{
    return Script->GetValueOfRefToLocalVariable (RefName, pRet, StackPos, Cmd);
}

int SetLocalVariableValue (char *Name, double Value)
{
    return Script->SetLocalVariableValue (Name, Value);
}

int SetValueOfLocalVariableRefTo (char *RefName, double Value, int Cmd)
{
    return Script->SetValueOfLocalVariableRefTo (RefName, Value, Cmd);
}

void TimerUpdateScriptDebugWindow (void)
{
    if (Script != nullptr) Script->SendResponseToDebugWindow ();
}

const char *GetCurrentScriptPath (void)
{
    if (Script == nullptr) return "";
    return Script->GetCurrentScriptPath ();
}

int GetCurrentLineNr (void)
{
    if (Script == nullptr) return -1;
    return Script->GetCurrentLineNr ();
}

void FileOfDebugWindowHasChangedByUser (void)
{
    if (Script != nullptr) Script->FileOfDebugWindowHasChangedByUser ();
}

void ReceiveScriptDebugWindowUpdateRequest (unsigned long par_Flags)
{
    if (Script != nullptr) Script->ReceiveScriptDebugWindowUpdateRequest (par_Flags);
}

int CheckIfBreakPointIsValid (const char *par_Filename, int par_LineNr, int *ret_FileNr, int *ret_Ip)
{
    if (Script != nullptr) {
        return Script->CheckIfBreakPointIsValid (par_Filename, par_LineNr, ret_FileNr, ret_Ip);
    } else {
        return 0;
    }
}

int AddBreakPoint (int par_Active, int par_FileNr, const char *par_Filename,
                   int par_LineNr, int par_Ip, int par_HitCount, const char *par_Condition)
{
    if (Script != nullptr) {
        return Script->AddBreakPoint (par_Active, par_FileNr, par_Filename, 
                                      par_LineNr, par_Ip, par_HitCount, par_Condition);
    } else {
        return -1;
    }
}

int DelBreakPoint (const char *par_Filename, int par_LineNr)
{
    if (Script != nullptr) {
        return Script->DelBreakPoint (par_Filename, par_LineNr);
    } else {
        return -1;
    }
}

int IsThereABreakPoint (const char *par_Filename, int par_LineNr)
{
    if (Script != nullptr) {
        return Script->IsThereABreakPoint (par_Filename, par_LineNr);
    } else {
        return -1;
    }
}

int DebugRunToScriptExecution (int par_FileNr, char *par_Filename, 
                               int par_LineNr, int par_Ip)
{
    if (Script != nullptr) {
        return Script->DebugRunToScriptExecution (par_FileNr, par_Filename, 
                                                  par_LineNr, par_Ip);
    } else {
        return -1;
    }
}

int DebugStopScriptExecution (void)
{
    if (Script != nullptr) {
        return Script->DebugStopScriptExecution ();
    }
    return -1;
}

int DebugContScriptExecution (void)
{
    if (Script != nullptr) {
        return Script->DebugContScriptExecution ();
    }
    return -1;
}

int DebugStepOverScriptExecution (void)
{
    if (Script != nullptr) {
        return Script->DebugStepOverScriptExecution ();
    }
    return -1;
}

int DebugStepIntoScriptExecution (void)
{
    if (Script != nullptr) {
        return Script->DebugStepIntoScriptExecution ();
    }
    return -1;
}

int DebugStepOutScriptExecution (void)
{
    if (Script != nullptr) {
        return Script->DebugStepOutScriptExecution ();
    }
    return -1;
}

int UpdateBreakPointsHighlight (int par_Flag)
{
    if (Script != nullptr) {
        return Script->UpdateBreakPointsHighlight (par_Flag);
    }
    return -1;
}

int OpenScriptDebugWindow_GUIToSchedThread (void)
{
    if (Script != nullptr) {
        return Script->OpenScriptDebugWindow ();
    }
    return -1;
}

int CloseScriptDebugWindow_GUIToSchedThread (void)
{
    if (Script != nullptr) {
        return Script->CloseScriptDebugWindow ();
    }
    return -1;
}

int LoadFileContent (int par_FileNr)
{
    if (Script != nullptr) {
        return Script->LoadFileContent (par_FileNr);
    }
    return -1;
}

int ScriptEnterCriticalSection(void)
{
    if (Script != nullptr) {
        if (Script->ScriptTryEnterCriticalSection ()) {
            return 1;
        } else {
            return 0;
        }
    }
    return 0;
}

void ScriptLeaveCriticalSection(void)
{
    if (Script != nullptr) {
        Script->ScriptLeaveCriticalSection ();
    }
}

int ScriptDebugGetCurrentState (int *ret_StartSelOffset, int *ret_EndSelOffset, int *ret_FileNr, int *ret_LineNr, int *ret_DelayCounter, cExecutor **ret_Executor, cParser **ret_Parser)
{
    if (Script != nullptr) {
        return Script->ScriptDebugGetCurrentState (ret_StartSelOffset, ret_EndSelOffset, ret_FileNr, ret_LineNr, ret_DelayCounter, ret_Executor, ret_Parser);
    }
    return -1;

}
