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
#include "Config.h"
#include "MyMemory.h"
#include "Files.h"
#include "Scheduler.h"
#include "ScriptMessageFile.h"
#include "ParseCommandLine.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cStopSchedulerCmd)


int cStopSchedulerCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cStopSchedulerCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    if (OpenWithNoGui()) {
        par_Parser->Error (SCRIPT_PARSER_WARNING, "STOP_SCHEDULER command will be ignored inside XilEnv without gui (-nogui oder sc_no_gui.exe)");
    } else {
        AddScriptMessage ("STOP_SCHEDULER: Press Button 'Continue' to continue Scriptfile");
        disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_USER, nullptr, nullptr);
    }
    return 0;
}

int cStopSchedulerCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cStopSchedulerCmd StopSchedulerCmd ("STOP_SCHEDULER",
                                       0, 
                                       0,  
                                       nullptr,
                                       FALSE, 
                                       FALSE, 
                                       0,
                                       0,
                                       CMD_INSIDE_ATOMIC_NOT_ALLOWED);
// this is a legacy command
static cStopSchedulerCmd StopSoftcarCmd ("STOP_SOFTCAR",
                                       0,
                                       0,
                                       nullptr,
                                       FALSE,
                                       FALSE,
                                       0,
                                       0,
                                       CMD_INSIDE_ATOMIC_NOT_ALLOWED);
