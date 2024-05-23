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

DEFINE_CMD_CLASS(cWhileCmd)


int cWhileCmd::SyntaxCheck (cParser *par_Parser)
{
    return par_Parser->AddWhile ();
}

int cWhileCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    int Value;

    if (par_Parser->SolveEquationForParameter (0, &Value, -1)) {
        return -1;
    }
    if (Value) {
        // If WHILE is true than execute next command
        if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("/* WHILE is TRUE */");
        par_Executor->SetWhileFlag (0);
    } else {
        // If WHILE is not true set IP to correspondent ENDWHILE command
        if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("/* WHILE is FALSE */");
        par_Executor->SetNextIp (static_cast<int>(par_Executor->GetOptParameterCurrentCmd ()));
        par_Executor->SetWhileFlag (1);
    }
    return 0;
}

int cWhileCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cWhileCmd WhileCmd ("WHILE",                        
                           1, 
                           1,  
                           nullptr,
                           FALSE, 
                           FALSE,  
                           0,
                           0,
                           CMD_INSIDE_ATOMIC_ALLOWED_ONLY_WITHOUT_REMOTE_MASTER);
