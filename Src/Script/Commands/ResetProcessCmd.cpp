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
#include <stdio.h>

extern "C" {
#include "Config.h"
#include "MyMemory.h"
#include "Files.h"
#include "Scheduler.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cResetProcessCmd)


int cResetProcessCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cResetProcessCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    PID loc_pid;
    char ProcName[MAX_PATH];
    loc_pid = get_pid_by_name (par_Parser->GetParameter (0));
    get_name_by_pid (loc_pid, ProcName);
    // kill the process
    if (terminate_process (loc_pid) == UNKNOWN_PROCESS) {
        par_Parser->Error (SCRIPT_PARSER_WARNING, "Unknown process \"%s\"", par_Parser->GetParameter (0));
        par_Executor->SetData (0);
    } else {
        /* handover process name to wait-flag-handler, because th process will restart from there */
        strcpy (static_cast<char*>(par_Executor->GetDataPtr ()), ProcName);
        par_Executor->SetData (1);
    }
    return 0;
}

int cResetProcessCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(Cycles);
    if (par_Executor->GetData ()) {
        /* Wait till the process is killed */
        if (get_pid_by_name (static_cast<char*>(par_Executor->GetDataPtr ())) == UNKNOWN_PROCESS) {
            par_Executor->SetData (0);
            /* Than restart the process */
            if (start_process (static_cast<char*>(par_Executor->GetDataPtr ())) == UNKNOWN_PROCESS) {
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot start process \"%s\"", static_cast<char*>(par_Executor->GetDataPtr ()));
                return -1;
            } else {
                return 0;
            }
        } else {
            return 1;
        }
    }
    return 0;
}

static cResetProcessCmd ResetProcessCmd ("RESET_PROCESS",                        
                                         1, 
                                         1,  
                                         nullptr,
                                         FALSE, 
                                         TRUE,  
                                         -1,
                                         0,
                                         CMD_INSIDE_ATOMIC_ALLOWED);
