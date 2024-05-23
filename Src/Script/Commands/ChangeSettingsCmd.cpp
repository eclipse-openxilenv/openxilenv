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
#include "ScriptChangeSettings.h"
#include "InterfaceToScript.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cChangeSettingsCmd)


int cChangeSettingsCmd::SyntaxCheck (cParser *par_Parser)
{
     UNUSED(par_Parser);
     return 0;
}

int cChangeSettingsCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    if (ScriptChangeBasicSettings (CHANGE_SETTINGS_COMMAND_SET, 0, par_Parser->GetParameter (0), par_Parser->GetParameter (1))) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "unknown setting in CHANGE_SETTINGS");
        return -1;
    }
    return 0;
}

int cChangeSettingsCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cChangeSettingsCmd ChangeSettingsCmd ("CHANGE_SETTINGS",                        
                                             2, 
                                             2,  
                                             nullptr,
                                             FALSE, 
                                             FALSE, 
                                             0,
                                             0,
                                             CMD_INSIDE_ATOMIC_ALLOWED);


// Push

DEFINE_CMD_CLASS(cChangeSettingsPushCmd)


int cChangeSettingsPushCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cChangeSettingsPushCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    if (ScriptChangeBasicSettings (CHANGE_SETTINGS_COMMAND_PUSH_SET, 0, par_Parser->GetParameter (0), par_Parser->GetParameter (1))) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "unknown setting in CHANGE_SETTINGS_PUSH");
        return -1;
    }
    return 0;
}

int cChangeSettingsPushCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}


static cChangeSettingsPushCmd ChangeSettingsPushCmd ("CHANGE_SETTINGS_PUSH",
                                                     2,
                                                     2,
                                                     nullptr,
                                                     FALSE,
                                                     FALSE,
                                                     0,
                                                     0,
                                                     CMD_INSIDE_ATOMIC_ALLOWED);

// Pop

DEFINE_CMD_CLASS(cChangeSettingsPopCmd)


int cChangeSettingsPopCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cChangeSettingsPopCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    int Err = ScriptChangeBasicSettings (CHANGE_SETTINGS_COMMAND_POP, 0, par_Parser->GetParameter (0), "");
    if (Err == -2) { // -2 -> setting with this namen on the stack
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "no settings on stack to pop with CHANGE_SETTINGS_POP");
        return -1;
    } else if (Err == -1) { // -1 -> unknown setting name
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "unknown setting in CHANGE_SETTINGS_POP");
        return -1;
    } else if (Err != 0) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "unknown error in CHANGE_SETTINGS_POP");
        return -1;
    }
    return 0;
}

int cChangeSettingsPopCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}


static cChangeSettingsPopCmd ChangeSettingsPopCmd ("CHANGE_SETTINGS_POP",
                                                   1,
                                                   1,
                                                   nullptr,
                                                   FALSE,
                                                   FALSE,
                                                   0,
                                                   0,
                                                   CMD_INSIDE_ATOMIC_ALLOWED);
