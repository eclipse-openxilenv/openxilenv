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
#include "CanDataBase.h"
#include "InterfaceToScript.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cDelCanVariantCmd)


int cDelCanVariantCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cDelCanVariantCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    double erg;
    int Channel;

    if (par_Parser->SolveEquationForParameter (1, &erg, -1)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...",
                           par_Parser->GetParameter (1));
        return -1;
    } else {
        if (erg < 1.0) {
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "equation '%s' result are less than 1 use channel 1",
                               par_Parser->GetParameter (1));
            Channel = 1;
        } else if (erg > 8.0) {
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "equation '%s' result are greater than 8 use channel 8",
                               par_Parser->GetParameter (1));
            Channel = 8;
        } else Channel = static_cast<int>(erg);
        DelCANVarianteScriptCommand (Channel);
    }
    return 0;
}

int cDelCanVariantCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cDelCanVariantCmd DelCanVariantCmd ("DEL_CAN_VARIANT",                        
                                           1, 
                                           1,  
                                           nullptr,
                                           FALSE, 
                                           FALSE,  
                                           0,
                                           0,
                                           CMD_INSIDE_ATOMIC_ALLOWED);
