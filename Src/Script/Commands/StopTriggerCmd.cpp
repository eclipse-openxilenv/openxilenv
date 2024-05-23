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
#include "Message.h"
#include "ProcessEquations.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cStopTriggerCmd)


int cStopTriggerCmd::SyntaxCheck (cParser *par_Parser)
{
    switch (par_Parser->GetParameterCounter ()) {
    case 0:
    case 3:
        break;
    default:
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "STOP_TRIGGER need none or 3 parameters and not %i", 
                           par_Parser->GetParameterCounter ());
        return -1;
    }
    return 0;
}

int cStopTriggerCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    switch (par_Parser->GetParameterCounter ()) {
    case 0:
        par_Parser->SendMessageToProcess (nullptr, 0, PN_EQUATION_COMPILER, TRIGGER_STOP_MESSAGE);
        break;
    case 3:
        int EquNr;
        if (par_Parser->SolveEquationForParameter (0, &EquNr, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...", 
                               par_Parser->GetParameter (0));
            return -1;
        }
        int ReturnValue;
        if (!stricmp ("before", par_Parser->GetParameter (1))) {
            ReturnValue = DelBeforeProcessEquations (EquNr, par_Parser->GetParameter (2));
        } else if (!stricmp("behind", par_Parser->GetParameter (1))) {
            ReturnValue = DelBehindProcessEquations (EquNr, par_Parser->GetParameter (2));
        } else {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Only BEFORE or BEHIND is allowed");
            return -1;
        }
        if (ReturnValue) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation block number %d or processname not found %s", 
                               EquNr, par_Parser->GetParameter (2));
            return -1;
        }

        break;
    default:
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "this shoud never happen %s (%i)", __FILE__, __LINE__);
        return -1;
    }
    return 0;
}

int cStopTriggerCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cStopTriggerCmd StopTriggerCmd ("STOP_TRIGGER",                        
                                       0, 
                                       3,  
                                       nullptr,
                                       FALSE, 
                                       FALSE,  
                                       0,
                                       0,
                                       CMD_INSIDE_ATOMIC_ALLOWED);

static cStopTriggerCmd StopTriggerCmd2 ("STOP_EQUATION",                        
                                        0, 
                                        3,  
                                        nullptr,
                                        FALSE, 
                                        FALSE,  
                                        0,
                                        0,
                                        CMD_INSIDE_ATOMIC_ALLOWED);
