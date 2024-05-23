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


#ifndef PROCTABLE_H
#define PROCTABLE_H

#include "Proc.h"

class cProcTable {
private:
    cProc **Table;
    int SizeOfTable;
    int PosInTable;

    int CheckSizeOf (void)
    {
        if (PosInTable >= SizeOfTable) {
            SizeOfTable += 4 + (SizeOfTable >> 3);
            Table = (cProc **)my_realloc (Table, sizeof (cProc *) * SizeOfTable);
        }
        return ((Table == NULL) ? -1 : 0);
    }

    int Search (char *par_Name)
    {
        int x;
        for (x = 0; x < PosInTable; x++) {
            if (!strcmp (Table[x]->GetName(), par_Name)) {
                return x;
            }
        }
        return -1;
    }

public:
    cProcTable (void)
    {
        Table = NULL;
        SizeOfTable = 0;
        PosInTable = 0;
    }

    ~cProcTable (void)
    {
        int i;
        for (i = 0; i < PosInTable; i++) {
            delete Table[i];
        }
        if (Table != NULL) my_free (Table);
        SizeOfTable = 0;
        PosInTable = 0;

    }

    int AddProc (cProc *par_Proc)
    {
        int Idx;

        if (Search (par_Proc->GetName()) < 0) {
            CheckSizeOf ();
            Idx = PosInTable;
            Table[Idx] = par_Proc;
            par_Proc->SetProcIdx (Idx);
            PosInTable++;
            return Idx;
        } else {
            return -1;   // Procedure existiert schon
        }

    }

    cProc *SerchProcByName (char *par_Name)
    {
        int Idx;

        Idx = Search (par_Name);
        if (Idx < 0) return NULL;
        return Table[Idx];
    }

    cProc *GetProcByIdx (int par_Idx)
    {
        return Table[par_Idx];
    }

    void Reset (void)
    {
        int i;
        for (i = 0; i < PosInTable; i++) {
            delete Table[i];
        }
        PosInTable = 0;
    }

    void DbgPrintAllToFile (char *par_FileName)
    {
        FILE *fh;
        fh = fopen (par_FileName, "wt");
        if (fh == NULL) return;
        fprintf (fh, "\nAlle definierten Prozeduren:\n");
        for (int x = 0; x < PosInTable; x++) {
            fprintf (fh, "%04i %s ", x, Table[x]->GetName ());
            fprintf (fh, "Idx = %i ", Table[x]->GetProcIdx ());
            fprintf (fh, "ParamCount = %i ", Table[x]->GetParamCount ());
            fprintf (fh, "Ip = %i ", Table[x]->GetIp ());
            fprintf (fh, "LineNr = %i ", Table[x]->GetLineNr ());
            fprintf (fh, "FileNr = %i ", Table[x]->GetFileNr ());
            fprintf (fh, "(");
            for (int y = 0; y < Table[x]->GetParamCount (); y++) {
                if (y != 0) fprintf (fh, ", ");
                fprintf (fh, "%s", Table[x]->GetParameterName (y));
            }
            fprintf (fh, ")\n");
            Table[x]->PrintLabelAndGotoTable (fh);
        }
        fclose (fh);
    }
};

#endif