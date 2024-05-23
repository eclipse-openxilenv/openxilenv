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


#ifndef TOPARSEFILESTACK_H
#define TOPARSEFILESTACK_H

#include <stdio.h>

class cToParseFileStack {
private:
    struct FILE_TABLE {
        char *Name;
        int FirstRun;
    } *FileTable;

    int SizeOfFileTable;
    int WrPosInFileTable;
    int RdPosInFileTable;

    struct RUN_TABLE {
        int LineNr;
        int Ip;
        int WrPosInFileTable;
        int NextRun;
    } *RunTable;

    int SizeOfRunTable;
    int PosInRunTable;

    int CheckSizeOf (void)
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

    int AddRunToFile (int par_FileIdx, int par_Ip, unsigned long par_LineNr)
    {
        if (CheckSizeOf ()) return -1;
        RunTable[PosInRunTable].Ip = par_Ip;
        RunTable[PosInRunTable].LineNr = par_LineNr;
        RunTable[PosInRunTable].WrPosInFileTable = par_FileIdx;

        RunTable[PosInRunTable].NextRun = FileTable[par_FileIdx].FirstRun;
        FileTable[par_FileIdx].FirstRun = PosInRunTable;
        return PosInRunTable++;
    }

public:
    cToParseFileStack (void)
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

    ~cToParseFileStack (void)
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

    int SearchFile (char *par_Name)
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


    int AddFile (char *par_Name)
    {
        int Idx;

        Idx = SearchFile (par_Name);
        if (Idx <= 0) {
            // noch kein Label mit diesem Namen vorhanden
            if (CheckSizeOf ()) return -2;
            Idx = WrPosInFileTable;
            FileTable[Idx].Name = (char*)my_malloc (strlen (par_Name) + 1);
            if (FileTable[Idx].Name == NULL) return -1;
            strcpy (FileTable[Idx].Name, par_Name);
            FileTable[Idx].FirstRun = -1;
            return WrPosInFileTable++;
        } else {
            // Es gibt schon ein Label mit diesm Namen -> Fehler
            return -1;
        }
    }

    int AddRun (char *par_Name, int par_Ip, unsigned long par_LineNr, int par_UsingFlag)
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

    int GetNextFile (void)
    {
        if (RdPosInFileTable >= WrPosInFileTable) return -1;
        return RdPosInFileTable++;
    }

    char *GetFileName (int par_Idx)
    {
        return FileTable[par_Idx].Name;
    }

    int GetFileFirstRun (int par_Idx)
    {
        return FileTable[par_Idx].FirstRun;
    }

    int NextRunFile (int par_Idx)
    {
        return RunTable[par_Idx].NextRun;
    }

    int GetRunLineNr (int par_Idx)
    {
        return RunTable[par_Idx].LineNr;
    }

    int GetRunIp (int par_Idx)
    {
        return RunTable[par_Idx].Ip;
    }

    void Reset (void)
    {
        for (int x = 0; x < WrPosInFileTable; x++) {
            my_free (FileTable[x].Name);
            FileTable[x].Name = NULL;
        }
        WrPosInFileTable = 0;
        RdPosInFileTable = 0;
        PosInRunTable = 0;
    }
};

#endif
