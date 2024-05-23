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
#include "Scheduler.h"
#include "LoadSaveToFile.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cSaveSalCmd)


int cSaveSalCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cSaveSalCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    /* Parameter[0] = File name               */
    /* Parameter[1] = Proces sname            */
    /* Parameter[2] = Label filter            */
    /* Parameter[3] = Optional "INC_POINTER"  */
    if (get_pid_by_name (par_Parser->GetParameter (1)) == UNKNOWN_PROCESS) {
         par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Cannot find process-name '%s' !", 
                            par_Parser->GetParameter (1));
         return -1;
    }
    const char *Filter;
    if (par_Parser->GetParameterCounter () >= 3) {
        Filter = par_Parser->GetParameter (2);
    } else {
        Filter = "*";
    }
    int IncPointer = 0;
    if (par_Parser->GetParameterCounter () == 4) {
        if (!strcmpi("INC_POINTER", par_Parser->GetParameter (3))) {
            IncPointer = 1;
        } else if (strcmpi("NO_POINTER", par_Parser->GetParameter (3))) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "only \"INC_POINTER\" or \"NO_POINTER\" not \"%s\" are allowed !",
                               par_Parser->GetParameter (3));
            return -1;
        }
    }
    if (!ScriptWriteProcessToSAL (par_Parser->GetParameter (0), 
                                  par_Parser->GetParameter (1), 
                                  Filter,
                                  IncPointer)) {
         par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "SAVE_SAL was not successfull");
         return -1;
    }
    return 0;
}

int cSaveSalCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cSaveSalCmd SaveSalCmd ("SAVE_SAL",
                               2, 
                               4,
                               nullptr,
                               FALSE, 
                               FALSE,  
                               0,
                               0,
                               CMD_INSIDE_ATOMIC_ALLOWED);
