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


#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <stdint.h>
#include "Tokenizer.h"
#include "Proc.h"
#include "ProcTable.h"
#include "Parser.h"
#include "Stack.h"
#include "Breakpoints.h"
extern "C" {
#include "CanRecorder.h"
}

class cExecutor {
private:
    cCmdTable *CmdTable;
    cProc *InsideProc;      // gerade beim Ausfuehren aktive Procedure
    cProcTable *ProcTable;

    int CurrentIp;
    int NextIp;

    int CurrentCmdIdx;

    int IfFlag; 
    int WhileFlag; 
    int DefProcFlag; 

    // Kann von Befehlen Mittels SetData und GetData verwendet werden
    int DataForCmd;

#define DATA_ARRAY_SIZE 64
    uint64_t DataArrayForCmd[DATA_ARRAY_SIZE];

    // aus Historischen Gruenden werden manche Befehle immer zu zweit in einem Zyklus ausgefuehrt!
    int NextCmdInSameCycleFlag;

    int AtomicCounter;  // wenn 0 dann in keinem ATOMIC ENDATOMIC Block

    int TimeoutActiveFlag;
   uint32_t StartTimeoutCycle;
   uint32_t CurrentCycle;
   uint32_t AtWhichCycleTimeoutReached;


    FILE *HtmlReportFile;
    char HtmlReportFilename[MAX_PATH + 16];

    // Exit-Code des mittels START_EXE aufgerufenen Programms
    int ExitCodeVid;
    HANDLE StartExeProcessHandle;

    //int BreakPointsCount;
    cBreakPoints BreakPoints;
    int SingleStep;
    int StopAtIp;

    struct CAN_RECORDER *CanRecorder;

public:
    cExecutor (void);
    ~cExecutor (void);

    cStack Stack;

//    int flag_add_bbvari_automatic;

    void Init (cCmdTable *par_CmdTable, cProcTable *par_ProcTable, cProc *par_MainProc);
    
    void NextCmdInSameCycle (void)
    {
        NextCmdInSameCycleFlag = 1;
    }

    int GetIfFlag (void)
    {
        return IfFlag;
    }
    void SetIfFlag (int par_IfFlag)
    {
        IfFlag = par_IfFlag;
    }
    int GetWhileFlag (void)
    {
        return WhileFlag;
    }
    void SetWhileFlag (int par_WhileFlag)
    {
        WhileFlag = par_WhileFlag;
    }
    int GetDefProcFlag (void)
    {
        return DefProcFlag;
    }
    void SetDefProcFlag (int par_DefProcFlag)
    {
        DefProcFlag = par_DefProcFlag;
    }

    int GetData (void)
    {
        return DataForCmd;
    }
    void SetData (int par_Data)
    {
        DataForCmd = par_Data;
    }

    uint64_t GetData (int par_Idx)
    {
        if (par_Idx >= DATA_ARRAY_SIZE) par_Idx = DATA_ARRAY_SIZE-1;
        return DataArrayForCmd[par_Idx];
    }

    void SetData (int par_Idx, uint64_t par_Data)
    {
        if (par_Idx >= DATA_ARRAY_SIZE) par_Idx = DATA_ARRAY_SIZE-1;
        DataArrayForCmd[par_Idx] = par_Data;
    }

    void *GetDataPtr (void)
    {
        return (void*)DataArrayForCmd;
    }

    int ExecuteNextCmd (cParser *par_Parser);
    
    int WaitForResponseOfLastCmd (cParser *par_Parser, uint32_t par_Cycles);

    void SetNextIp (int par_Ip)
    {
        NextIp = par_Ip;
    }

    int GetCurrentIp (void)
    {
        return CurrentIp;
    }

    int GetNextIp (void)
    {
        return NextIp;
    }

    uint32_t GetOptParameterCurrentCmd (void)
    {
        return CmdTable->GetOptParameterCmd (static_cast<uint32_t>(CurrentIp));
    }

    uint32_t GetReservedCurrentCmd (void)
    {
        return CmdTable->GetReservedCmd (static_cast<uint32_t>(CurrentIp));
    }

    int GotoFromToDefLocalsPos (int par_FromPos, int par_ToPos, int par_DiffAtomicDepth, cParser *par_Parser, cExecutor *par_Executor)
    {
        AtomicCounter += par_DiffAtomicDepth;
        return InsideProc->GotoFromToDefLocalsPos (par_FromPos, par_ToPos, par_Parser, par_Executor);
    }

    int GetGotoFromToDefLocalsPos (int par_Idx, int *ret_FromPos, int *ret_ToPos, int *ret_Ip, int *ret_DiffAtomicDepth,
                                   char *par_Name, int par_FileNr)
    {
        return InsideProc->GetGotoFromToDefLocalsPos (par_Idx, ret_FromPos, ret_ToPos, ret_Ip, ret_DiffAtomicDepth, par_Name, par_FileNr);
    }

    int GetCurrentProcIdx (void)
    {
        return InsideProc->GetProcIdx ();
    }

    cProc *GetProcByIdx (int par_Idx)
    {
        return ProcTable->GetProcByIdx (par_Idx);
    }

    char *GetCurrentProcName (void)
    {
        return InsideProc->GetName ();
    }

    cProc *SetCurrentProcByIdx (int par_Idx)
    {
        InsideProc = ProcTable->GetProcByIdx (par_Idx);
        NextIp = InsideProc->GetIp ();
        return InsideProc;
    }

    void IncAtomicCounter (void)
    {
        AtomicCounter++;
    }
    void DecAtomicCounter (void)
    {
        if (AtomicCounter) AtomicCounter--;
    }
    int GetCurrentAtomicDepth (void)
    {
        return AtomicCounter;
    }
    void SetCurrentAtomicDepth (int par_AtomicDepth)
    {
        AtomicCounter = par_AtomicDepth;
    }

    void StartTimeoutControl (int par_Timeout);

    int GetWaitCyclesNeeded (void);

    FILE *GetHtmlReportFile (void)
    {
        return HtmlReportFile;
    }
    int ReopenHtmlReportFile (void);
    int ReopenHtmlReportFile (char *par_HtmlReportFilename);
    int OpenNewHtmlReportFile (char *par_HtmlReportFilename);
    void CloseHtmlReportFile (void);

    void EndOfExecution (void);

    // Breakpoints
    int CheckIfBreakPointIsValid (const char *par_Filename, int par_LineNr, int *ret_FileNr, int *ret_Ip)
    {
        if (CmdTable == NULL) return 0;
        return BreakPoints.CheckIfBreakPointIsValid (par_Filename, par_LineNr, CmdTable, ret_FileNr, ret_Ip, NULL);
    }

    int AddBreakPoint (int par_Active, int par_FileNr, const char *par_Filename,
                       int par_LineNr, int par_Ip, int par_HitCount, const char *par_Condition)
    {
        return BreakPoints.AddBreakPoint (par_Active, par_FileNr, par_Filename, 
                                          par_LineNr, par_Ip, par_HitCount, par_Condition, CmdTable, NULL);
    }

    int DelBreakPoint (const char *par_Filename, int par_LineNr)
    {
        return BreakPoints.DelBreakPoint (par_Filename, par_LineNr, CmdTable);
    }

    int IsThereABreakPoint (const char *par_Filename, int par_LineNr)
    {
        return BreakPoints.IsThereABreakPoint (par_Filename, par_LineNr);
    }

    /*int RemoveBreakPoint (int par_FileNr, int par_LineNr)
    {
        return BreakPoints.RemoveBreakPoint (par_FileNr, par_LineNr);
    }*/

    void ClearAllBreakPoints (void)
    {
        BreakPoints.ClearAllBreakPoints ();
    }
    
    void SetSingleStep (int par_StopAtIp)
    {
        StopAtIp = par_StopAtIp;
        SingleStep = 2;
    }

    void IgnoreBreakpointAtNextStep (void)
    {
        SingleStep = -1;
    }

    int GetCmdFileOffset (int par_Ip)
    {
        return CmdTable->GetFileOffset (par_Ip);
    }

    int GetCmdBeginOfParamOffset (int par_Ip)
    {
        return CmdTable->GetBeginOfParamOffset (par_Ip);
    }

    int GetNextBreakpoint (int par_StartIdx, int par_FileNr, int *ret_Ip, int *ret_LineNr, int *ret_ActiveValid)
    {
        return BreakPoints.GetNext (par_StartIdx, par_FileNr, ret_Ip, ret_LineNr, ret_ActiveValid);
    }

    int GetBreakpointString (int par_Index, char *par_String)
    {
        return BreakPoints.GetBreakpointString (par_Index, par_String);
    }

    int GetBreakpointChangedCounter()
    {
        return BreakPoints.GetBreakpointChangedCounter();
    }
    
    int InitBreakPointsFromIni (void)
    {
        return BreakPoints.InitBreakPointsFromIni (CmdTable);
    }

    int SaveBreakPointsToIni (void)
    {
        return BreakPoints.SaveBreakPointsToIni ();
    }

    int GetExitCodeVid (void)
    {
        return ExitCodeVid;
    }

    void SetStopScriptAtStartDebuggingFlag (int Flag);

    void SetStartExeProcessHandle(HANDLE par_hProces)
    {
        StartExeProcessHandle = par_hProces;
    }

    HANDLE GetStartExeProcessHandle(void)
    {
        return StartExeProcessHandle;
    }

    struct CAN_RECORDER *GetCanRecorder()
    {
        return CanRecorder;
    }

    void SetCanRecorder(struct CAN_RECORDER *par_CanRecorder)
    {
        CanRecorder = par_CanRecorder;
    }

    void DetachAllBbvaris();
};

#endif
