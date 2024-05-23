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
#include "MyMemory.h"
#include "Blackboard.h"
#include "Scheduler.h"
#include "InterfaceToScript.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cWrBbvariEnableCmd)


int cWrBbvariEnableCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cWrBbvariEnableCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    PID pid;

    if (strlen (par_Parser->GetParameter (1)) >= MAX_PATH) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "WR_BBVARI_ENABLE(%s, %s) process-name to long!", 
                           par_Parser->GetParameter (0), par_Parser->GetParameter (1));
        return -1;
    }
    pid = get_pid_by_name(par_Parser->GetParameter (1));
    if (pid == UNKNOWN_PROCESS) {
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "Cannot find process-name '%s' for this variable %s!", 
                           par_Parser->GetParameter (1), par_Parser->GetParameter (0));
        return 0;
    }
    int returnwert = enable_bbvari_access (pid, get_bbvarivid_by_name (par_Parser->GetParameter (0)));
    if (returnwert != 0) {
        par_Parser->Error (SCRIPT_PARSER_WARNING, "Cannot find variable-name '%s' for WR_BBVARI_ENABLE() !", par_Parser->GetParameter (0));
    }
    return 0;
}

int cWrBbvariEnableCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cWrBbvariEnableCmd WrBbvariEnableCmd ("WR_BBVARI_ENABLE",                        
                                             2, 
                                             2,  
                                             nullptr,
                                             FALSE, 
                                             FALSE, 
                                             0,
                                             0,
                                             CMD_INSIDE_ATOMIC_ALLOWED);
