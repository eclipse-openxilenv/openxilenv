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
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cSetbbvariCmd)


int cSetbbvariCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cSetbbvariCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    int Value;
    if (par_Parser->SolveEquationForParameter (0, &Value, -1)) {
        if (par_Parser->get_flag_add_bbvari_automatic () == ADD_BBVARI_AUTOMATIC_STOP) {
            return -1;
        }
    }
    // Value will be ignored
    return 0;
}

int cSetbbvariCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cSetbbvariCmd SetbbvariCmd1 ("SET_BBVARI",                        
                                   1, 
                                   1,  
                                   nullptr,
                                   FALSE, 
                                   FALSE,  
                                   0,
                                   0,
                                   CMD_INSIDE_ATOMIC_ALLOWED);

static cSetbbvariCmd SetbbvariCmd2 ("SET",                        
                                   1, 
                                   1,  
                                   nullptr,
                                   FALSE, 
                                   FALSE,  
                                   0,
                                   0,
                                   CMD_INSIDE_ATOMIC_ALLOWED);

