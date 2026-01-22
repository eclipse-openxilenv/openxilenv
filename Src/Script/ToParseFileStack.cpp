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

#include <stdio.h>
extern "C" {
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ToParseFileStack.h"
}

int cToParseFileStack::CheckSizeOf (void)
{
    if (WrPosInFileTable >= SizeOfFileTable) {
        SizeOfFileTable += 4 + (SizeOfFileTable >> 3);
        FileTable = (struct FILE_TABLE*)my_realloc (FileTable, sizeof (struct FILE_TABLE) * SizeOfFileTable);
    }
    if (PosInRunTable >= SizeOfRunTable) {
        SizeOfRunTable += 4 + (SizeOfRunTable >> 3);
        RunTable = (struct RUN_TABLE*)my_realloc (RunTable, sizeof (struct RUN_TABLE) * SizeOfRunTable);
    }
    return (((FileTable == NULL) || (RunTable == NULL)) ? -1 : 0);
}

int cToParseFileStack::AddRunToFile (int par_FileIdx, int par_Ip, unsigned long par_LineNr)
{
    if (CheckSizeOf ()) return -1;
    RunTable[PosInRunTable].Ip = par_Ip;
    RunTable[PosInRunTable].LineNr = par_LineNr;
    RunTable[PosInRunTable].WrPosInFileTable = par_FileIdx;

    RunTable[PosInRunTable].NextRun = FileTable[par_FileIdx].FirstRun;
    FileTable[par_FileIdx].FirstRun = PosInRunTable;
    return PosInRunTable++;
}

cToParseFileStack::cToParseFileStack (void)
{
    SizeOfFileTable = 0;
    WrPosInFileTable = 0;
    RdPosInFileTable = 0;
    FileTable = NULL;
    SizeOfRunTable = 0;
    PosInRunTable = 0;
    RunTable = NULL;
    CheckSizeOf ();
}

cToParseFileStack::~cToParseFileStack (void)
{
    for (int x = 0; x < WrPosInFileTable; x++) {
        my_free (FileTable[x].Name);
    }
    if (FileTable != NULL) my_free (FileTable);
    FileTable = NULL;
    SizeOfFileTable = 0;
    WrPosInFileTable = 0;
    RdPosInFileTable = 0;
    if (RunTable != NULL) my_free (RunTable);
    RunTable = NULL;
    SizeOfRunTable = 0;
    PosInRunTable = 0;
}

int cToParseFileStack::SearchFile (char *par_Name)
{
    int x;

    for (x = 0; x < WrPosInFileTable; x++) {
#if _WIN32
        if (!strcmpi (FileTable[x].Name, par_Name)) {
#else
        if (!strcmp (FileTable[x].Name, par_Name)) {
#endif
            return x;
        }
    }
    return -1;
}


int cToParseFileStack::AddFile (char *par_Name)
{
    int Idx;

    Idx = SearchFile (par_Name);
    if (Idx <= 0) {
        // noch kein Label mit diesem Namen vorhanden
        if (CheckSizeOf ()) return -2;
        Idx = WrPosInFileTable;
        FileTable[Idx].Name = StringMalloc (par_Name);
        if (FileTable[Idx].Name == NULL) return -1;
        FileTable[Idx].FirstRun = -1;
        return WrPosInFileTable++;
    } else {
        // Es gibt schon ein Label mit diesm Namen -> Fehler
        return -1;
    }
}

int cToParseFileStack::AddRun (char *par_Name, int par_Ip, unsigned long par_LineNr, int par_UsingFlag)
{
    int Idx;
    int Ret;

    Idx = SearchFile (par_Name);
    if (Idx < 0) {
        // noch kein Label mit diesem Namen vorhanden
        Idx = AddFile (par_Name);
        if (Idx < 0) return -1;
    }
    if (par_UsingFlag) return 0;
    Ret = AddRunToFile (Idx, par_Ip, par_LineNr);
    return Ret;
}

int cToParseFileStack::GetNextFile (void)
{
    if (RdPosInFileTable >= WrPosInFileTable) return -1;
    return RdPosInFileTable++;
}

char *cToParseFileStack::GetFileName (int par_Idx)
{
    return FileTable[par_Idx].Name;
}

int cToParseFileStack::GetFileFirstRun (int par_Idx)
{
    return FileTable[par_Idx].FirstRun;
}

int cToParseFileStack::NextRunFile (int par_Idx)
{
    return RunTable[par_Idx].NextRun;
}

int cToParseFileStack::GetRunLineNr (int par_Idx)
{
    return RunTable[par_Idx].LineNr;
}

int cToParseFileStack::GetRunIp (int par_Idx)
{
    return RunTable[par_Idx].Ip;
}

void cToParseFileStack::Reset (void)
{
    for (int x = 0; x < WrPosInFileTable; x++) {
        my_free (FileTable[x].Name);
        FileTable[x].Name = NULL;
    }
    WrPosInFileTable = 0;
    RdPosInFileTable = 0;
    PosInRunTable = 0;
}
