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


#include "Platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <float.h>

extern "C" {
#include "MyMemory.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Scheduler.h"
#include "ScriptStartExe.h"
#include "InterfaceToScript.h"
#include "MainValues.h"
}
#include "Script.h"
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cWaitExeCmd)


int cWaitExeCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cWaitExeCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    double dTimeout = DBL_MAX;  // endless
    int WaitType = 0;
    DWORD Timeout = INFINITE;

    if (par_Parser->GetParameterCounter () > 0) {
        if (s_main_ini_val.ConnectToRemoteMaster) {
            // If remote master is active the scheduler should not stoped
            // otherwise a eath lock will happen
            WaitType = 1;
        } else {
            if (!stricmp("ONLY_SCRIPT", par_Parser->GetParameter(0))) {
                WaitType = 1;
            } else if (!stricmp("SCHEDULER", par_Parser->GetParameter(0))) {
                WaitType = 0;
            }
        }
        if (par_Parser->SolveEquationForParameter (0, &dTimeout, -1)) {
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...",
                                   par_Parser->GetParameter (0));
        }
        if (dTimeout < 0.0) Timeout = 0;
        else if (dTimeout > INFINITE) Timeout = INFINITE;
        else Timeout = (DWORD)dTimeout;
    } else {
        if (s_main_ini_val.ConnectToRemoteMaster) {
            // If remote master is active the scheduler should not stoped
            // otherwise a eath lock will happen
            WaitType = 1;
        }
    }
    par_Executor->SetData(0, WaitType);
    par_Executor->SetData(1, GetTickCount64() + Timeout);
    par_Executor->SetData(2, 0);
    if (WaitType == 0) {
        RegisterSchedFunc (cyclic_script);
    }
    return 0;
}

int cWaitExeCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(Cycles);

    int WaitType = (int)par_Executor->GetData(0);
    uint64_t Timeout = par_Executor->GetData(1);
    DWORD ExitCode;

    if (par_Executor->GetData(2) == 0) {
        if (wait_for_end_of_exe (par_Executor->GetStartExeProcessHandle(), 0, &ExitCode)) {
            // extern EXE are properly terminated
            par_Executor->SetData(3, ExitCode);
            par_Executor->SetData(2, 1);
            if (WaitType == 0) UnRegisterSchedFunc ();
        } else if (GetTickCount64() > Timeout) {
            if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("/* WAIT_EXE timeout set ExitCode to 259 */\n");
            par_Executor->SetData(3, 259);
            par_Executor->SetData(2, 1);
            if (WaitType == 0) UnRegisterSchedFunc ();
        }
    } else {
        write_bbvari_dword(par_Executor->GetExitCodeVid(), (int32_t)par_Executor->GetData(3));
        return 0;
    }
    return 1;
}

static cWaitExeCmd WaitExeCmd ("WAIT_EXE",                        
                               0, 
                               2,
                               nullptr,
                               FALSE, 
                               TRUE,  
                               -1,
                               0,
                               CMD_INSIDE_ATOMIC_ALLOWED);
