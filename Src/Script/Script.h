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


#ifndef SCRIPT_H
#define SCRIPT_H

#ifdef __cplusplus

#define UNUSED(x) (void)(x)

#include "Parser.h"
#include "Executor.h"
#include "ScriptErrorMsgDlg.h"

double ReadVariableFromBlackboard (const char *name);

class cScript {
private:
    int State;
#define SCRIPT_STATE_STOP      0
#define SCRIPT_STATE_START     1
#define SCRIPT_STATE_SLEEP     2
#define SCRIPT_STATE_RUNNING   3
#define SCRIPT_STATE_READY     4
#define SCRIPT_STATE_FINISHED  5
#define SCRIPT_STATE_ERROR_SCT 6
#define SCRIPT_STATE_DEBUG_HOLD 7

    unsigned long CycleCounter;

    cCmdTable CmdTable;

    cProcTable ProcTable;

    cParser Parser;
    cExecutor Executor;

    int StatusFlagVid;

    void CleanUpEndOfScriptReached (void);

    CRITICAL_SECTION CriticalSection;
public:
    cScript (void);
    ~cScript (void);

    int Start (char *par_Filename);

    int GetState (void)
    {
        return State;
    }

    int Cyclic (int par_script_status_flag);

    int Init (void)
    {
        return 0;
    }

    int Terminate (void)
    {
        return 0;
    }

    int GetLocalVariableValue (char *Name, double *pRet, int StackPos)
    {
        return Executor.Stack.GetLocalVariableValue (Name, pRet, StackPos);
    }

    int GetValueOfRefToLocalVariable (char *RefName, double *pRet, int StackPos, int Cmd)
    {
        return Executor.Stack.GetValueOfRefToLocalVariable (RefName, pRet, StackPos, Cmd);
    }
    int SetLocalVariableValue (char *Name, double Value)
    {
        return Executor.Stack.SetLocalVariableValue (Name, Value);
    }

    int SetValueOfLocalVariableRefTo (char *RefName, double Value, int Cmd)
    {
        return Executor.Stack.SetValueOfLocalVariableRefTo (RefName, Value, Cmd);
    }

    void SendResponseToDebugWindow (void);

    const char *GetCurrentScriptPath (void)
    {
        return Parser.GetCurrentScriptPath ();
    }

    int GetCurrentLineNr (void)
    {
        return Parser.GetCurrentLineNr ();
    }

    void FileOfDebugWindowHasChangedByUser (void)
    {
    }

    void ReceiveScriptDebugWindowUpdateRequest (uint32_t par_Flags)
    {
        UNUSED(par_Flags);
    }

    int AddBreakPoint (int par_Active, int par_FileNr, const char *par_Filename,
                       int par_LineNr, int par_Ip, int par_HitCount, const char *par_Condition)
    {
        return Executor.AddBreakPoint (par_Active, par_FileNr, par_Filename, 
                                       par_LineNr, par_Ip, par_HitCount, par_Condition);
    }

    int DelBreakPoint (const char *par_Filename, int par_LineNr)
    {
        return Executor.DelBreakPoint (par_Filename, par_LineNr);
    }

    int IsThereABreakPoint (const char *par_Filename, int par_LineNr)
    {
        return Executor.IsThereABreakPoint (par_Filename, par_LineNr);
    }

    int CheckIfBreakPointIsValid (const char *par_Filename, int par_LineNr, int *ret_FileNr, int *ret_Ip)
    {
        return Executor.CheckIfBreakPointIsValid (par_Filename, par_LineNr, ret_FileNr, ret_Ip);
    }


    int DebugStopScriptExecution (void);
    int DebugContScriptExecution (void);
    int DebugStepOverScriptExecution (void);
    int DebugStepIntoScriptExecution (void);
    int DebugStepOutScriptExecution (void);
    int DebugRunToScriptExecution (int par_FileNr, char *par_Filename, 
                                   int par_LineNr, int par_Ip);
    int UpdateBreakPointsHighlight (int par_Flag);
    int OpenScriptDebugWindow (void);
    int CloseScriptDebugWindow (void);
    int LoadFileContent (int par_FileNr);

    void ScriptEnterCriticalSection(void);
    bool ScriptTryEnterCriticalSection(void);
    void ScriptLeaveCriticalSection(void);
    int ScriptDebugGetCurrentState (int *ret_StartSelOffset, int *ret_EndSelOffset, int *ret_FileNr, int *ret_LineNr, int *ret_DelayCounter, cExecutor **ret_Executor, cParser **ret_Parser);
};

int ScriptDebugGetCurrentState (int *ret_StartSelOffset, int *ret_EndSelOffset, int *ret_FileNr, int *ret_LineNr, int *ret_DelayCounter, cExecutor **ret_Executor, cParser **ret_Parser);

extern "C" {
#endif //  __cplusplus

    int init_script (void);
    void cyclic_script (void);
    void terminate_script (void);

    int GetLocalVariableValue (char *Name, double *pRet, int StackPos);
    int GetValueOfRefToLocalVariable (char *RefName, double *pRet, int StackPos, int Cmd);
    int SetLocalVariableValue (char *Name, double Value);
    int SetValueOfLocalVariableRefTo (char *RefName, double Value, int Cmd);

    void TimerUpdateScriptDebugWindow (void);

    const char *GetCurrentScriptPath(void);
    int GetCurrentLineNr (void);

    void FileOfDebugWindowHasChangedByUser (void);
    void ReceiveScriptDebugWindowUpdateRequest (unsigned long par_Flags);
    int CheckIfBreakPointIsValid (const char *par_Filename, int par_LineNr, int *ret_FileNr, int *ret_Ip);
    int AddBreakPoint (int par_Active, int par_FileNr, const char *par_Filename,
                       int par_LineNr, int par_Ip, int par_HitCount, const char *par_Condition);
    int DelBreakPoint (const char *par_Filename, int par_LineNr);
    int IsThereABreakPoint (const char *par_Filename, int par_LineNr);
    int DebugRunToScriptExecution (int par_FileNr, char *par_Filename, 
                                   int par_LineNr, int par_Ip);
    int DebugStopScriptExecution (void);
    int DebugContScriptExecution (void);
    int DebugStepOverScriptExecution (void);
    int DebugStepIntoScriptExecution (void);
    int DebugStepOutScriptExecution (void);
    int UpdateBreakPointsHighlight (int par_Flag);
    int OpenScriptDebugWindow_GUIToSchedThread (void);
    int CloseScriptDebugWindow_GUIToSchedThread (void);
    int LoadFileContent (int par_FileNr);

    int ScriptEnterCriticalSection();
    void ScriptLeaveCriticalSection();

    //int DebugScriptGetCachedFiles(void);
#ifdef __cplusplus
}
#endif

#endif
