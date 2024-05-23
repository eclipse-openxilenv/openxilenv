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

DEFINE_CMD_CLASS(cGotoCmd)


int cGotoCmd::SyntaxCheck (cParser *par_Parser)
{
    int Idx;
    int FileOffset;
    int LineNr;

    FileOffset = par_Parser->GetCurrentScriptFile ()->Ftell (&LineNr);
    if (par_Parser->GetNumNoneSolvedEnvVars (0) || par_Parser->GetNumSolvedEnvVars (0)) {
        // GOSUB without solved environment variable should throw an warning
        if (par_Parser->GetNumNoneSolvedEnvVars (0)) {
            par_Parser->Error (SCRIPT_PARSER_WARNING, "GOTO \"%s\" which includes an unknown environment variable at compile time, try to resolve it at run time", par_Parser->GetParameter (0));
        }
        Idx = par_Parser->AddGoto (nullptr, par_Parser->GetCurrentIp (),
                                   static_cast<uint32_t>(LineNr),
                                   static_cast<uint32_t>(FileOffset));
    } else {
        Idx = par_Parser->AddGoto (par_Parser->GetParameter (0), par_Parser->GetCurrentIp (), 
                                   static_cast<uint32_t>(LineNr),
                                   static_cast<uint32_t>(FileOffset));
    }
    par_Parser->SetOptParameter (static_cast<uint32_t>(Idx), 0);
    return (Idx >= 0) ? 0 : -1;
}

int cGotoCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    int FromPos;
    int ToPos;
    int GotoIp;
    int DiffAtomicDepth;

    int Idx = static_cast<int>(par_Executor->GetOptParameterCurrentCmd ());
    if (par_Executor->GetGotoFromToDefLocalsPos (Idx, &FromPos, &ToPos, &GotoIp, &DiffAtomicDepth,
                                                 par_Parser->GetParameter (0), par_Parser->GetCurrentFileNr ())) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot goto \"%s\" the label include a environment variable but this or the resulting label doesn't exist", par_Parser->GetParameter (0));
        return -1;
    }
    if (DiffAtomicDepth) {
        if (par_Parser->GenDebugFile ()) {
            if (DiffAtomicDepth > 0) {
                par_Parser->PrintDebugFile ("/* jump inside %i ATOMIC block(s) */", DiffAtomicDepth);
            } else {
                par_Parser->PrintDebugFile ("/* jump out of %i ATOMIC block(s) */", -DiffAtomicDepth);
            }
        }
    }
    par_Executor->GotoFromToDefLocalsPos (FromPos, ToPos, DiffAtomicDepth, par_Parser, par_Executor);
    par_Executor->SetNextIp (GotoIp - 1);
    return 0;
}

int cGotoCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cGotoCmd GotoCmd ("GOTO",                        
                         1, 
                         1,  
                         nullptr,
                         FALSE, 
                         FALSE,  
                         0,
                         0,
                         CMD_INSIDE_ATOMIC_ALLOWED);
