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
#include "Executor.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cReturnCmd)


int cReturnCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cReturnCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    int ReturnTpIp;
    int ReturnToAtomicDepth;

    if (par_Executor->Stack.RemoveGosubFromStack (par_Parser, &ReturnTpIp, &ReturnToAtomicDepth)) {
        return -1;
    }
    int DiffAtomicDepth = ReturnToAtomicDepth - par_Executor->GetCurrentAtomicDepth ();
    if (DiffAtomicDepth) {
        par_Executor->SetCurrentAtomicDepth (ReturnToAtomicDepth);
        if (par_Parser->GenDebugFile ()) {
            if (DiffAtomicDepth > 0) {
                par_Parser->PrintDebugFile ("/* jump inside %i ATOMIC block(s) */", DiffAtomicDepth);
            } else {
                par_Parser->PrintDebugFile ("/* jump out of %i ATOMIC block(s) */", -DiffAtomicDepth);
            }
        }
    }
    par_Executor->SetNextIp (ReturnTpIp + 1);
    return 0;
}

int cReturnCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cReturnCmd ReturnCmd ("RETURN",                        
                             0, 
                             0,  
                             nullptr,
                             FALSE, 
                             FALSE,  
                             0,
                             0,
                             CMD_INSIDE_ATOMIC_ALLOWED);
