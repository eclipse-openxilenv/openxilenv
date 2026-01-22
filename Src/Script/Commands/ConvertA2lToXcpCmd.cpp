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
#include "Message.h"
#include "EquationParser.h"
#include "InterfaceToScript.h"
#include "A2LConvertToXcp.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cConvertA2lToXcpCmd)


int cConvertA2lToXcpCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cConvertA2lToXcpCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    char ErrorString[1024];
    ErrorString[0] = 0;
    int Flags = 0;
    if (par_Parser->GetParameterCounter() == 3) {
        par_Parser->SolveEquationForParameter (2, &Flags , -1);
    }
    if (A2LConvertToXcpOrCpp(par_Parser->GetParameter (0), par_Parser->GetParameter (1), 1, Flags, ErrorString, sizeof(ErrorString))) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Unable to convert \"%s\" to \"%s\" because %s",
                           par_Parser->GetParameter (0), par_Parser->GetParameter (1), ErrorString);
        return -1;
    }
    return 0;
}

int cConvertA2lToXcpCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cConvertA2lToXcpCmd ConvertA2lToXcpCmd ("CONVERT_A2L_TO_XCP",
                     2,
                     3,
                     nullptr,
                     FALSE, 
                     FALSE, 
                     0,  
                     0,
                     CMD_INSIDE_ATOMIC_ALLOWED);


// CCP

DEFINE_CMD_CLASS(cConvertA2lToCcpCmd)


int cConvertA2lToCcpCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cConvertA2lToCcpCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    char ErrorString[1024];
    ErrorString[0] = 0;
    int Flags = 0;
    if (par_Parser->GetParameterCounter() == 3) {
        par_Parser->SolveEquationForParameter (2, &Flags , -1);
    }
    if (A2LConvertToXcpOrCpp(par_Parser->GetParameter (0), par_Parser->GetParameter (1), 0, Flags, ErrorString, sizeof(ErrorString))) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Unable to convert \"%s\" to \"%s\" because %s",
                           par_Parser->GetParameter (0), par_Parser->GetParameter (0), ErrorString);
        return -1;
    }
    return 0;
}

int cConvertA2lToCcpCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

static cConvertA2lToCcpCmd ConvertA2lToCcpCmd ("CONVERT_A2L_TO_CCP",
                     2,
                     3,
                     nullptr,
                     FALSE,
                     FALSE,
                     0,
                     0,
                     CMD_INSIDE_ATOMIC_ALLOWED);
