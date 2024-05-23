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
#include "TimeStamp.h"
#include "Scheduler.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cDelayCmd)


int cDelayCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cDelayCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    int DelayCycles;

    if (par_Parser->SolveEquationForParameter (0, &DelayCycles , -1)) {
        return -1;
    }
    par_Executor->SetData (static_cast<int>(GetCycleCounter ()) + DelayCycles);
    return 0;
}

int cDelayCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(Cycles);
    uint32_t DelayEndCycle = static_cast<uint32_t>(par_Executor->GetData ());
    uint32_t ActualCycle = GetCycleCounter ();
    if (DelayEndCycle <= ActualCycle) {
        return 0;
    } else {
        return static_cast<int>(DelayEndCycle - ActualCycle);
    }
}

static cDelayCmd DelayCmd ("DELAY",                        
                           1, 
                           1,  
                           nullptr,
                           FALSE, 
                           TRUE,  
                           -1,
                           0,
                           CMD_INSIDE_ATOMIC_NOT_ALLOWED);
