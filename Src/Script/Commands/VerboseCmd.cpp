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
#include "InterfaceToScript.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cVerboseCmd)


int cVerboseCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cVerboseCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    if(!strcmp (par_Parser->GetParameter (0), "ON")) {
        if (par_Parser->GetVerboseMessage () == VERBOSE_ON) {
            par_Parser->Error (SCRIPT_PARSER_WARNING, "VERBOSE(is still ON)");
        }
        par_Parser->SetVerboseMessage (VERBOSE_ON);
    } else if (!strcmp (par_Parser->GetParameter (0), "MESSAGE_PREFIX_OFF")) {
        if (par_Parser->GetVerboseMessage () == VERBOSE_MESSAGE_PREFIX_OFF) {
            par_Parser->Error (SCRIPT_PARSER_WARNING, "VERBOSE(is still MESSAGE_PREFIX_OFF)");
        }
        par_Parser->SetVerboseMessage (VERBOSE_MESSAGE_PREFIX_OFF);
    } else if (!strcmp (par_Parser->GetParameter (0), "MESSAGE_PREFIX_CYCLE_COUNTER")) {
        if (par_Parser->GetVerboseMessage () == VERBOSE_MESSAGE_PREFIX_OFF) {
            par_Parser->Error (SCRIPT_PARSER_WARNING, "VERBOSE(is still MESSAGE_PREFIX_CYCLE_COUNTER)");
        }
        par_Parser->SetVerboseMessage (VERBOSE_MESSAGE_PREFIX_CYCLE_COUNTER);
    } else {
        if (par_Parser->GetVerboseMessage () == VERBOSE_OFF) {
            par_Parser->Error (SCRIPT_PARSER_WARNING, "VERBOSE(is still OFF)");
        }
        par_Parser->SetVerboseMessage (VERBOSE_OFF);
    }
    return 0;
}

int cVerboseCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cVerboseCmd VerboseCmd ("VERBOSE",                        
                               1, 
                               1,  
                               nullptr,
                               FALSE, 
                               FALSE, 
                               0,
                               0,
                               CMD_INSIDE_ATOMIC_ALLOWED);

