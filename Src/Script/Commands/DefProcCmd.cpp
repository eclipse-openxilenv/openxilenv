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

DEFINE_CMD_CLASS(cDefProcCmd)


int cDefProcCmd::SyntaxCheck (cParser *par_Parser)
{
    int Ret;

    cProc *Proc = par_Parser->SerchProcByName (par_Parser->GetParameter (0)); 
    if (Proc == nullptr) {
        // Procedure is not existing -> creat it
        Proc = new cProc (par_Parser, par_Parser->GetParameter (0), 
                          par_Parser->GetParameterCounter () - 1,
                          par_Parser->GetCurrentFileNr (), 
                          par_Parser->GetStartCmdLineNr (),
                          par_Parser->GetCurrentScriptFile ()->IsOnlyUsing ());
        if (!par_Parser->GetCurrentScriptFile ()->IsOnlyUsing ()) {
            Proc->SetIp (par_Parser->GetCurrentIp () + 1);
        }
        Ret = (par_Parser->AddProc (Proc) < 0) ? -1 : 0;
    } else {
        // Procedure exists
        // check if file and line number are the same
        if ((par_Parser->GetCurrentFileNr () != Proc->GetFileNr ()) ||
            (par_Parser->GetStartCmdLineNr () != Proc->GetLineNr ())) {
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "procedure \"%s\" already exists in file \"%s\" at line %i",
                               Proc->GetName (), cFileCache::GetFilenameByNr (Proc->GetFileNr ()), Proc->GetLineNr ());
            Ret = -1;                       
        } else {
            if (Proc->IsOnlyDeclared ()) {
                // it is only declared with USING
                if (par_Parser->GetCurrentScriptFile ()->IsOnlyUsing ()) {
                    Ret = 0;  // will be ignored
                } else {
                    // it will be now defined
                    Proc->SetIp (par_Parser->GetCurrentIp () + 1);
                    Ret = 0;
                }
            } else {
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error: procedure \"%s\" already exists in file \"%s\" at line %i",
                                   Proc->GetName (), cFileCache::GetFilenameByNr (Proc->GetFileNr ()), Proc->GetLineNr ());
                Ret = -1;
            }
        }
    }
    par_Parser->AddDefProc ();
    par_Parser->PushProcToStack (Proc);
    return Ret;
}

int cDefProcCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Parser);
    // Jump direct to END_DEF_PROC
    par_Executor->SetDefProcFlag (1);
    par_Executor->SetNextIp (static_cast<int>(par_Executor->GetOptParameterCurrentCmd ()));

    return 0;
}

int cDefProcCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cDefProcCmd DefProcCmd ("DEF_PROC",                        
                               1, 
                               MAX_PARAMETERS,  
                               nullptr,
                               FALSE, 
                               FALSE,  
                               0,
                               1,   // This command will be checked also in USING mode
                               CMD_INSIDE_ATOMIC_ALLOWED);
