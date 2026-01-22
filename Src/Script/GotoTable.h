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


#ifndef GOTOTABLE_H
#define GOTOTABLE_H

#include <string.h>

extern "C"{
#include "MyMemory.h"
#include "StringMaxChar.h"
}

class cGotoTable {
private:
    struct LABEL_TABLE {
        char *Name;
        int FileNr;
        int LineNr;
        unsigned int FileOffset;
        int Ip;
        struct {
            unsigned int Counter : 31;
            unsigned int JumpTargetFlag : 1;
        } Ref;
        int DefLocalsTreePos;  // Position inside the DefLocals tree
        int FirstGoto;
        int AtomicDepth;
    } *LabelTable;

    int SizeOfLabelTable;
    int PosInLabelTable;

    struct GOTO_TABLE {
        int LineNr;
        int Ip;
        int DefLocalsTreePos;  // Position inside the DefLocals tree
        int PosInLabelTable;
        int NextGoto;
        int AtomicDepth;
    } *GotoTable;

    int SizeOfGotoTable;
    int PosInGotoTable;

    int CheckSizeOf (void)
    {
        if (PosInLabelTable >= SizeOfLabelTable) {
            SizeOfLabelTable += 4 + (SizeOfLabelTable >> 3);
            LabelTable = (struct LABEL_TABLE*)my_realloc (LabelTable, sizeof (struct LABEL_TABLE) * SizeOfLabelTable);
        }
        if (PosInGotoTable >= SizeOfGotoTable) {
            SizeOfGotoTable += 4 + (SizeOfGotoTable >> 3);
            GotoTable = (struct GOTO_TABLE*)my_realloc (GotoTable, sizeof (struct GOTO_TABLE) * SizeOfGotoTable);
        }

        return (((LabelTable == NULL) || (GotoTable == NULL)) ? -1 : 0);
    }

    int AddRefToLabel (int par_LabelIdx, int par_Ip, unsigned int par_LineNr, int par_DefLocalsTreePos, int par_AtomicDepth)
    {
        GotoTable[PosInGotoTable].Ip = par_Ip;
        GotoTable[PosInGotoTable].LineNr = par_LineNr;
        GotoTable[PosInGotoTable].DefLocalsTreePos = par_DefLocalsTreePos;
        GotoTable[PosInGotoTable].PosInLabelTable = par_LabelIdx;
        GotoTable[PosInGotoTable].AtomicDepth = par_AtomicDepth;
        if (par_LabelIdx >= 0) {
            GotoTable[PosInGotoTable].NextGoto = LabelTable[par_LabelIdx].FirstGoto;
            LabelTable[par_LabelIdx].FirstGoto = PosInGotoTable;
        }
        return PosInGotoTable++;
    }

public:
    cGotoTable (void)
    {
        SizeOfLabelTable = 0;
        PosInLabelTable = 0;
        LabelTable = NULL;
        SizeOfGotoTable = 0;
        PosInGotoTable = 0;
        GotoTable = NULL;
        CheckSizeOf ();
    }

    ~cGotoTable (void)
    {
        for (int x = 0; x < PosInLabelTable; x++) {
            my_free (LabelTable[x].Name);
        }
        if (LabelTable != NULL) my_free (LabelTable);
        LabelTable = NULL;
        SizeOfLabelTable = 0;
        PosInLabelTable = 0;
        if (GotoTable != NULL) my_free (GotoTable);
        GotoTable = NULL;
        SizeOfGotoTable = 0;
        PosInGotoTable = 0;
    }

    int SearchLabel (char *par_Name, int par_FileNr)
    {
        int x;

        for (x = 0; x < PosInLabelTable; x++) {
            if (LabelTable[x].FileNr == par_FileNr) {
                if (!strcmp (LabelTable[x].Name, par_Name)) {
                    return x;
                }
            }
        }
        return -1;
    }


    int AddLabel (char *par_Name, int par_FileNr, int par_Ip, unsigned int par_LineNr, unsigned int par_FileOffset, int par_DefLocalsTreePos, int par_AtomicDepth)
    {
        int Idx;

        Idx = SearchLabel (par_Name, par_FileNr);
        if (Idx < 0) {
            // There exist no lable with this name
            if (CheckSizeOf ()) return -2;
            Idx = PosInLabelTable;
            LabelTable[Idx].Name = StringMalloc (par_Name);
            if (LabelTable[Idx].Name == NULL) return -1;
            LabelTable[Idx].Ip = par_Ip;
            LabelTable[Idx].FileNr = par_FileNr;
            LabelTable[Idx].LineNr = par_LineNr;
            LabelTable[Idx].FileOffset = par_FileOffset;
            LabelTable[Idx].DefLocalsTreePos = par_DefLocalsTreePos;
            LabelTable[Idx].AtomicDepth = par_AtomicDepth;
            LabelTable[Idx].Ref.JumpTargetFlag = 1;
            LabelTable[Idx].Ref.Counter = 0;
            LabelTable[Idx].FirstGoto = -1;
            return PosInLabelTable++;
        } else {
            // There exist no lable with this name
            if (LabelTable[Idx].Ref.JumpTargetFlag) {
                return -1;
            } else {
                LabelTable[Idx].Ip = par_Ip;
                LabelTable[Idx].FileNr = par_FileNr;
                LabelTable[Idx].LineNr = par_LineNr;
                LabelTable[Idx].FileOffset = par_FileOffset;
                LabelTable[Idx].DefLocalsTreePos = par_DefLocalsTreePos;
                LabelTable[Idx].AtomicDepth = par_AtomicDepth;
                LabelTable[Idx].Ref.JumpTargetFlag = 1;
                return Idx;
            }
        }
    }

    int AddGoto (char *par_Name, int par_FileNr, int par_Ip, unsigned int par_LineNr, unsigned int par_FileOffset, int par_DefLocalsTreePos, int par_AtomicDepth)
    {
        int Idx;
        int Ret;

        if (CheckSizeOf ()) return -2;
        if (par_Name == NULL) {
            // GOTO label with environment variable
            Ret = AddRefToLabel (-1, par_Ip, par_LineNr, par_DefLocalsTreePos, par_AtomicDepth);
        } else {
            Idx = SearchLabel (par_Name, par_FileNr);
            if (Idx < 0) {
                // There exist no lable with this name
                Idx = PosInLabelTable;
                LabelTable[Idx].Name = StringMalloc (par_Name);
                if (LabelTable[Idx].Name == NULL) return -1;
                LabelTable[Idx].FileNr = par_FileNr;
                LabelTable[Idx].LineNr = par_LineNr;
                LabelTable[Idx].FileOffset = par_FileOffset;
                LabelTable[Idx].Ref.JumpTargetFlag = 0;
                LabelTable[Idx].Ref.Counter = 1;
                LabelTable[Idx].FirstGoto = -1;
                Ret = AddRefToLabel (Idx, par_Ip, par_LineNr, par_DefLocalsTreePos, par_AtomicDepth);
                PosInLabelTable++;
            } else {
                LabelTable[Idx].Ref.Counter++;
                if (!LabelTable[Idx].Ref.JumpTargetFlag) {
                    LabelTable[Idx].FileNr = par_FileNr;
                    LabelTable[Idx].LineNr = par_LineNr;
                    LabelTable[Idx].FileOffset = par_FileOffset;
                }
                Ret = AddRefToLabel (Idx, par_Ip, par_LineNr, par_DefLocalsTreePos, par_AtomicDepth);
            }
        }
        return Ret;
    }

    int NextLabel (int par_Idx)
    {
        if (par_Idx >= (PosInLabelTable - 1)) return -1;  // no more lables
        if (par_Idx < 0) {
            if (PosInLabelTable) return 0;   // if -1 it is the first label
            return -1;                       // there are no lable inside this Proc
        }
        return par_Idx + 1;   // next lable


    }

    int Exist (int par_idx)
    {
        return LabelTable[par_idx].Ref.JumpTargetFlag;
    }

    char *GetLabelName (int par_Idx)
    {
        return LabelTable[par_Idx].Name;
    }

    int GetLabelLineNr (int par_Idx)
    {
        return LabelTable[par_Idx].LineNr;
    }

    int GetLabelFileOffset (int par_Idx)
    {
        return LabelTable[par_Idx].FileOffset;
    }

    int GetLabelDefLocalsTreePos (int par_Idx)
    {
        return LabelTable[par_Idx].DefLocalsTreePos;
    }

    int GetAtomicDepthLabel (int par_Idx)
    {
        return LabelTable[par_Idx].AtomicDepth;
    }

    int GetAtomicDepthGoto (int par_Idx)
    {
        return GotoTable[par_Idx].AtomicDepth;
    }

    int GetLabelFirstGoto (int par_Idx)
    {
        return LabelTable[par_Idx].FirstGoto;
    }

    int NextGotoToLabel (int par_Idx)
    {
        return GotoTable[par_Idx].NextGoto;
    }

    int GetGotoLineNr (int par_Idx)
    {
        if ((par_Idx >= 0) && (par_Idx < PosInGotoTable)) {
            return GotoTable[par_Idx].LineNr;
        } else {
            return -1;
        }
    }

    int GetGotoIp (int par_Idx)
    {
        return GotoTable[par_Idx].Ip;
    }

    int GetGotoFromToDefLocalsPos (int par_Idx, int *ret_FromPos, int *ret_ToPos, int *ret_Ip, int *ret_DiffAtomicDepth,
                                   char *par_Name, int par_FileNr)
    {
        *ret_FromPos = GotoTable[par_Idx].DefLocalsTreePos; 
        int IdxLabelTable = GotoTable[par_Idx].PosInLabelTable;
        if (IdxLabelTable < 0) {
            // GOTO Label mit Umgebungsvariable
            IdxLabelTable = SearchLabel (par_Name, par_FileNr);
            if (IdxLabelTable < 0) return -1;
        }
        *ret_ToPos = LabelTable[IdxLabelTable].DefLocalsTreePos;
        *ret_Ip = LabelTable[IdxLabelTable].Ip;
        *ret_DiffAtomicDepth = LabelTable[IdxLabelTable].AtomicDepth - GotoTable[par_Idx].AtomicDepth;
        return 0;
    }

    int PrintLabelAndGotoTable (FILE *par_FileHandle)
    {
        int x;

        fprintf (par_FileHandle, "Label Tabelle:\n");
        for (x = 0; x < PosInLabelTable; x++) {
            fprintf (par_FileHandle, "  %s: FileNr = %i, LineNr = %i, FileOffset = %i, Ip = %i, "
                     "Ref.Counter = %i, Ref.JumpTargetFlag = %i, DefLocalsTreePos = %i, FirstGoto = %i, AtomicDepth = %i\n", 
                     LabelTable[x].Name, LabelTable[x].FileNr, LabelTable[x].LineNr, LabelTable[x].FileOffset, LabelTable[x].Ip,
                     (int)LabelTable[x].Ref.Counter, (int)LabelTable[x].Ref.JumpTargetFlag, LabelTable[x].DefLocalsTreePos, LabelTable[x].FirstGoto, LabelTable[x].AtomicDepth);
        }
        fprintf (par_FileHandle, "Goto Tabelle:\n");
        for (x = 0; x < PosInGotoTable; x++) {
            fprintf (par_FileHandle, "  LineNr = %i, Ip = %i, "
                     "DefLocalsTreePos = %i, PosInLabelTable = %i, NextGoto = %i, AtomicDepth = %i\n", 
                     GotoTable[x].LineNr, GotoTable[x].Ip,
                     GotoTable[x].DefLocalsTreePos, GotoTable[x].PosInLabelTable, GotoTable[x].NextGoto, GotoTable[x].AtomicDepth);
        }
        return 0;
    }

};

#endif
