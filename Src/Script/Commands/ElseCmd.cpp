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

DEFINE_CMD_CLASS(cElseCmd)


int cElseCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    int CorrespondingIfIp;
    int Ret = par_Parser->AddElse (par_Parser->GetCurrentIp (), &CorrespondingIfIp);
    par_Parser->SetOptParameter (CorrespondingIfIp, static_cast<uint32_t>(par_Parser->GetCurrentIp ()), 0);
    return Ret;
}

int cElseCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Parser);
    if (par_Executor->GetIfFlag ()) {
        // Directly jumped from 'IF' to this 'ELSE' because condition was not true.
        // so the 'ELSE' block should be executed
        par_Executor->SetIfFlag (0);
    } else {
        // The corresponding 'IF' was true -> jump over the 'ELSE' block
        par_Executor->SetNextIp (static_cast<int>(par_Executor->GetOptParameterCurrentCmd ()));
    }
    par_Executor->SetIfFlag (0);
    return 0;
}

int cElseCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cElseCmd ElseCmd ("ELSE",                        
                         0, 
                         0,  
                         nullptr,
                         FALSE, 
                         FALSE,  
                         0,
                         0,
                         CMD_INSIDE_ATOMIC_ALLOWED);
