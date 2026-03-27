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

DEFINE_CMD_CLASS(cEof)


int cEof::SyntaxCheck (cParser *par_Parser)
{
    par_Parser->EndOfScriptFileReached ();
    return 0;
}

int cEof::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    if (par_Parser->GetCurrentFileNr () == 0) {
        if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("/* end of main script */");
        return 0xE0F;    // End of main Script
    } else {
        int ReturnToIp;
        char Filename[MAX_PATH];
        par_Executor->Stack.RemoveRunReturnToScript (par_Parser, &ReturnToIp, Filename, sizeof(Filename));
        if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("/* go back to script file \"%s\" (%i)*/", Filename, ReturnToIp);
        par_Executor->SetNextIp (ReturnToIp + 1);
        return 0;
    }
}

int cEof::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cEof Eof ("-----EOF-----",   // This command must be the first after sorting!
                 0,  
                 0,  
                 nullptr,
                 FALSE, 
                 FALSE,  
                 0,
                 1,   //   This command will be checked also in USING mode
                 CMD_INSIDE_ATOMIC_ALLOWED);
