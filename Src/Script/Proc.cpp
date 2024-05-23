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
#include "Proc.h"

cProc::cProc (cParser *par_Parser, const char * const par_ProcName, int par_ParamCount, int par_FileNr, int par_LineNr, int par_DeclaratinOrDefinitionFlag)
{
    CurrentAtomicDepth = 0;
    CurrentDefLocalsTreePos = -1;
    FileNr = par_FileNr;
    LineNr = par_LineNr;
    if (par_DeclaratinOrDefinitionFlag) {
        DefinedFlag = 0;
        DelcaredFlag = 1;
    } else {
        DefinedFlag = 1;
        DelcaredFlag = 0;
    }
    int SizeofBuffer = static_cast<int>(strlen (par_ProcName)) + 1;
    for (int x = 1; x < par_Parser->GetParameterCounter (); x++) {  // 1. parameter is procedure name
        SizeofBuffer += static_cast<int>(strlen (par_Parser->GetParameter (x))) + 1;
    }
    ParamBuffer = static_cast<char*>(my_malloc (static_cast<size_t>(SizeofBuffer)));
    Params = static_cast<cParam*>(my_calloc (static_cast<size_t>(par_Parser->GetParameterCounter ()), sizeof (cParam)));
    if ((ParamBuffer != nullptr) && (Params != nullptr)) {
        char *p = Name = ParamBuffer;
        strcpy (Name, par_ProcName);
        p += strlen (Name) + 1;
        for (int x = 1; x < par_Parser->GetParameterCounter (); x++) { // 1. parameter is procedure name
            strcpy (p, par_Parser->GetParameter (x));
            Params[x-1].SetName (p);
            p += strlen (p) + 1;
        }
    } else {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error: out of memory, will crash soon %s (%i)", __FILE__, __LINE__);
    }
    ParamCount = par_ParamCount;
}

cProc::~cProc (void)
{
    if (ParamBuffer != nullptr) my_free (ParamBuffer);
    ParamBuffer = nullptr;
    if (Params != nullptr) my_free (Params);
    Params = nullptr;
}

int cProc::CheckIfAllGotosResoved (cParser *par_Parser)
{
    int Ret = 1;
    for (int x = GotoTable.NextLabel (-1); x >= 0; x = GotoTable.NextLabel (x)) {
        if (!GotoTable.Exist (x)) {
            for (int y = GotoTable.GetLabelFirstGoto (x); y >= 0; y = GotoTable.NextGotoToLabel (y)) {
                par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "there are a GOTO or GOSUB command to a not existing label \"%s\" at line %i", GotoTable.GetLabelName (x), GotoTable.GetGotoLineNr (y));
            }
            Ret = 0;
        }
    }
    return Ret;
}
