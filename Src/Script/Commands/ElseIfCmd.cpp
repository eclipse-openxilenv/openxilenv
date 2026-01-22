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

DEFINE_CMD_CLASS(cElseIfCmd)


int cElseIfCmd::SyntaxCheck (cParser *par_Parser)
{
    int CorrespondingIfIp;
    int Ret = par_Parser->AddElseIf (par_Parser->GetCurrentIp (), &CorrespondingIfIp);
    par_Parser->SetOptParameter (CorrespondingIfIp, static_cast<uint32_t>(par_Parser->GetCurrentIp ()), 0);
    return Ret;
}

int cElseIfCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    if (par_Executor->GetIfFlag ()) {
        // Directly jumped from 'IF'/'ELSEIF' to this 'ELSEIF' because condition was not true.
        // so the 'ELSEIF' condition of this block should be checked
        par_Executor->SetIfFlag (0);

        int Value;
        if (par_Parser->SolveEquationForParameter (0, &Value, -1)) {
            return -1;
        }
        if (Value) {  
            // 'ELSEIF' is true -> continue with next command
            if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("/* ELSEIF is TRUE */");
            par_Executor->SetIfFlag (0);
        } else {
            // 'ELSEIF' is false -> Set the IP to the next (ELSE, ELSEIF or ENDIF)
            if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("/* ELSEIF is FALSE */");
            par_Executor->SetNextIp (static_cast<int>(par_Executor->GetOptParameterCurrentCmd ()));
            par_Executor->SetIfFlag (1);
        }

    } else {
        // the corresponding 'IF' was true -> jump over the 'ELSE' block
        par_Executor->SetNextIp (static_cast<int>(par_Executor->GetOptParameterCurrentCmd ()));
    }
    return 0;
}

int cElseIfCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cElseIfCmd ElseIfCmd ("ELSEIF",                        
                             1, 
                             1,  
                             nullptr,
                             FALSE, 
                             FALSE,  
                             0,
                             0,
                             CMD_INSIDE_ATOMIC_ALLOWED);
