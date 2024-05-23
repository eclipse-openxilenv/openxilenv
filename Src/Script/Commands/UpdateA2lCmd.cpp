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
#include "BlackboardConvertFromTo.h"
#include "EquationParser.h"
#include "InterfaceToScript.h"
#include "A2LUpdate.h"
#include "A2LLink.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cUpdatetA2lToXcpCmd)


int cUpdatetA2lToXcpCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

int cUpdatetA2lToXcpCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    UNUSED(par_Executor);
    char ErrorString[1024];
    ErrorString[0] = 0;
    int LineNr;
    const char* Error = "unknown";
    int Flags = A2L_LINK_UPDATE_ZERO_FLAG;
    int ParIdx;
    for(ParIdx = 4; ParIdx < par_Parser->GetParameterCounter(); ParIdx++) {
        if (!strcmpi(par_Parser->GetParameter (ParIdx), "ZERO")) {
            Flags |= A2L_LINK_UPDATE_ZERO_FLAG;
        } else if (!strcmpi(par_Parser->GetParameter (ParIdx), "IGNORE")) {
            Flags |= A2L_LINK_UPDATE_IGNORE_FLAG;
        } else if (!strcmpi(par_Parser->GetParameter (ParIdx), "DLLOFFSET")) {
            Flags |= A2L_LINK_ADDRESS_TRANSLATION_DLL_FLAG;
        } else if (!strcmpi(par_Parser->GetParameter (ParIdx), "MULTIDLLOFFSET")) {
            Flags |= A2L_LINK_ADDRESS_TRANSLATION_MULTI_DLL_FLAG;
        } else {
            break;
        }
    }
    union FloatOrInt64 Value;
    int Type;
    uint64_t MinusOffset = 0;
    uint64_t PlusOffset = 0;
    char *NotUpdatedLabelFile = nullptr;
    if (ParIdx < par_Parser->GetParameterCounter()) {
        par_Parser->SolveEquationForParameter(ParIdx, &Value, &Type, -1);
        MinusOffset = FloatOrInt64_ToUint64(Value, Type);
        ParIdx++;
        if (ParIdx < par_Parser->GetParameterCounter()) {
            par_Parser->SolveEquationForParameter(ParIdx, &Value, &Type, -1);
            PlusOffset = FloatOrInt64_ToUint64(Value, Type);
            ParIdx++;
            if (ParIdx < par_Parser->GetParameterCounter()) {
                NotUpdatedLabelFile = par_Parser->GetParameter (ParIdx);
            }
        }
    }

    ASAP2_DATABASE *Database = LoadAsapFile (par_Parser->GetParameter (0), 0,
                                             ErrorString, sizeof(ErrorString), &LineNr);
    if (Database == NULL) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Unable to load \"%s\" because %s",
                           par_Parser->GetParameter (0), ErrorString);
        return -1;
    }
    if (A2LUpdate(Database, par_Parser->GetParameter (1), // par_OutA2LFile
                  par_Parser->GetParameter (2), // par_SourceType
                  Flags,                        // par_Flags
                  par_Parser->GetParameter (3),  // par_Source
                  MinusOffset, PlusOffset,
                  NotUpdatedLabelFile, &Error)) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Unable to update \"%s\" to \"%s\" because %s",
                           par_Parser->GetParameter (0), par_Parser->GetParameter (1), Error);
        return -1;
    }
    return 0;
}

int cUpdatetA2lToXcpCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
}

/*
 * UPDATE_A2L(Source.A2L,
 *            Destination.A2L,
 *            PROCESS or EXE or ELF
 *            Process name or executable name or elf file name,
 *            ZERO or IGNORE
 *            DLLOFFSET or MULTIDLLOFFSET
 *            Offset minus
 *            Offset plus
 *            Optional file name to list all ot updated labels)
 */

static cUpdatetA2lToXcpCmd UpdatetA2lToXcpCmd ("UPDATE_A2L",
                     4,
                     9,
                     nullptr,
                     FALSE, 
                     FALSE, 
                     0,  
                     0,
                     CMD_INSIDE_ATOMIC_ALLOWED);


