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

#include "MainWinowSyncWithOtherThreads.h"
extern "C" {
#include "MyMemory.h"
#include "Scheduler.h"
}
#include "Script.h"  // wegen cyclic_script
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cShowDialogCmd)


int cShowDialogCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cShowDialogCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    ScriptShowDialogFromOtherThread();
    return 0;
}

int cShowDialogCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    if (IsScriptDialogClosedFromOtherThread() == TRUE) {
        return 0;
    }
    return 1;
}

static cShowDialogCmd ShowDialogCmd1 ("SHOW_DIALOG",                        
                                       0, 
                                       0,  
                                       nullptr,
                                       FALSE, 
                                       TRUE,  
                                       -1,
                                       0,
                                       CMD_INSIDE_ATOMIC_NOT_ALLOWED);

static cShowDialogCmd ShowDialogCmd2 ("SHOW_DLG",                        
                                       0, 
                                       0,  
                                       nullptr,
                                       FALSE, 
                                       TRUE,  
                                       -1,
                                       0,
                                       CMD_INSIDE_ATOMIC_NOT_ALLOWED);
