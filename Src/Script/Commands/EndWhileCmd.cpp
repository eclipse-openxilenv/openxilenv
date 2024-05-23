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

DEFINE_CMD_CLASS(cEndWhileCmd)


int cEndWhileCmd::SyntaxCheck (cParser *par_Parser)
{
    uint32_t Ip;

    // search all BREAK's and insert IP of the des correspond ENDWHILE
    while (par_Parser->RemoveBreakFromWhile (&Ip) > 0) {
        par_Parser->SetOptParameter (static_cast<int>(Ip), static_cast<uint32_t>(par_Parser->GetCurrentIp ()), 0);
    } 
    int Ret = par_Parser->RemoveEndWhile (&Ip);
    par_Parser->SetOptParameter (static_cast<int>(Ip), static_cast<uint32_t>(par_Parser->GetCurrentIp ()), 0);  // insert own IP to correspond WHILE
    par_Parser->SetOptParameter (Ip, 0);    // insert IP of the corresponding WHILE into ENDWHILE
    return Ret;
}

int cEndWhileCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Parser);
    if (par_Executor->GetWhileFlag ()) {
        // Jumped by WHILE because conditions was not true
    } else {
        par_Executor->SetNextIp (static_cast<int>(par_Executor->GetOptParameterCurrentCmd ()));
        par_Executor->NextCmdInSameCycle ();
    }
    par_Executor->SetWhileFlag (0);
    return 0;
}

int cEndWhileCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cEndWhileCmd EndWhileCmd ("ENDWHILE",                        
                                 0, 
                                 0,  
                                 nullptr,
                                 FALSE, 
                                 FALSE,  
                                 0,
                                 0,
                                 CMD_INSIDE_ATOMIC_ALLOWED_ONLY_WITHOUT_REMOTE_MASTER);
