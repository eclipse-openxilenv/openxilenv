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

DEFINE_CMD_CLASS(cEndDefProcCmd)


int cEndDefProcCmd::SyntaxCheck (cParser *par_Parser)
{
    int CorrespondingDefProcIp;
    
    par_Parser->CheckIfAllGotosResovedInCurrentProc ();

    int Ret = par_Parser->RemoveEndDefProc (&CorrespondingDefProcIp);
    par_Parser->SetOptParameter (CorrespondingDefProcIp, static_cast<uint32_t>(par_Parser->GetCurrentIp ()), 0);
    par_Parser->PopProcFromStack ();
    return Ret;
}

int cEndDefProcCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    int ReturnToProcIdx;
    int ReturnToIp;

    if (par_Executor->GetDefProcFlag ()) {
        // Wurde direkt von DEF_PROC angesprungen d.h Funktion wurde nicht ausgefuehrt
        par_Executor->SetDefProcFlag (0);
    } else {
        par_Executor->Stack.RemoveProcFromStack (par_Parser, &ReturnToProcIdx, &ReturnToIp);
        par_Executor->SetCurrentProcByIdx (ReturnToProcIdx);
        par_Executor->SetNextIp (ReturnToIp);
    }
    return 0;
}

int cEndDefProcCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cEndDefProcCmd EndDefProcCmd ("END_DEF_PROC",                        
                                     0, 
                                     0,  
                                     nullptr,
                                     FALSE, 
                                     FALSE,  
                                     0,
                                     1,   //  This command will be checked also in USING mode
                                     CMD_INSIDE_ATOMIC_ALLOWED);
