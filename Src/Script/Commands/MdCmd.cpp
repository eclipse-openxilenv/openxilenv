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
#ifdef _WIN32
#include <direct.h>
#endif

extern "C" {
#include "Config.h"
#include "MyMemory.h"
#include "Message.h"
#include "EquationParser.h"
#include "InterfaceToScript.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cMdCmd)


int cMdCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

#ifdef _WIN32
#define MKDIR(d) mkdir(d)
#else
#define MKDIR(d) mkdir(d, 0777)
#endif

int cMdCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    if (MKDIR (par_Parser->GetParameter (0))) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Unable to create directory \"%s\"", 
                           par_Parser->GetParameter (0));
        return -1;
    }

    return 0;
}

int cMdCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cMdCmd MdCmd ("MD",                        
                     1, 
                     1,  
                     nullptr,
                     FALSE, 
                     FALSE, 
                     0,  
                     0,
                     CMD_INSIDE_ATOMIC_ALLOWED);
