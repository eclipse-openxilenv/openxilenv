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
#include <malloc.h>

extern "C" {
#include "Config.h"
#include "MyMemory.h"
#include "ConfigurablePrefix.h"
#include "Message.h"
#include "EquationParser.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cExitToDosCmd)


int cExitToDosCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cExitToDosCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    if(par_Parser->GetParameterCounter () == 1) { // If there wa transferd an exit value than use that
        int Len = strlen(GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD));
        Len += strlen("ExitCode=");
        Len += strlen(par_Parser->GetParameter (0));
        Len += 1; // for termination character
        char *Buffer = static_cast<char*>(my_malloc(Len));
        sprintf (Buffer, "%sExitCode=%s", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD), par_Parser->GetParameter (0));
        direct_solve_equation (Buffer);
        my_free(Buffer);
    }
    /* terminate oneself */
    int Len = strlen(GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD));
    Len += strlen("exit=1");
    Len += 1; // for termination character
    char *Buffer = static_cast<char*>(my_malloc(Len));
    sprintf (Buffer, "%sexit=1", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SHORT_BLACKBOARD));
    direct_solve_equation (Buffer);
    my_free(Buffer);
    return 0xE0F;  // 0xEOF not EOF
}

int cExitToDosCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}


static cExitToDosCmd QuitCmd ("QUIT",
                              0,
                              1,
                              nullptr,
                              FALSE,
                              FALSE,
                              0,
                              0,
                              CMD_INSIDE_ATOMIC_ALLOWED);

// old name
static cExitToDosCmd ExitToDosCmd ("EXIT_TO_DOS",
                                   0, 
                                   1,  
                                   nullptr,
                                   FALSE, 
                                   FALSE,  
                                   0,
                                   0,
                                   CMD_INSIDE_ATOMIC_ALLOWED);
