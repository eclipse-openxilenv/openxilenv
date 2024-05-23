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


#ifndef PROC_H
#define PROC_H

#include "DefLocalsTree.h"
#include "GotoTable.h"

class cParser;

class cProc {
private:
    char *Name;
    int Idx;
    int FileNr;
    int LineNr;
    int DelcaredFlag;   // This will be set if file will load by USING command, das heisst alle
                        // that means all bember expect Params und ParamCount are not used
                        // If loaded by USING command only the DEF_PROC commands will be evaluate
    int DefinedFlag;    // This will be set if the file will ne loaded by RUN or INCLUDE command
    int Ip;
    int IpLast;
    class cParam {
    private:
        enum {VALUE_PARAMETER, REFERENCE_PARAMETER} Type;
        union {
            double Value;
            int Ref;
        } Data;
        char *Name;
    public:
        void SetName (char *par_Name) 
        { 
            if (*par_Name == '*') {   // it is a reference
                Type = REFERENCE_PARAMETER;
            } else {
                Type = VALUE_PARAMETER;
            }
            Name = par_Name; 
        }
        char *GetName (void) { return Name; }
        void SetRef (int par_Ref) { Data.Ref = par_Ref; Type = REFERENCE_PARAMETER; }
        void SetValue (double par_Value) { Data.Value = par_Value; Type = VALUE_PARAMETER; }
        int IsRefOrValue (void) { return (int)Type; }
        int GetRef (void) { return Data.Ref; }
        double GetValue (void) { return Data.Value; }
    };
    cParam *Params;
    int ParamCount;
    char *ParamBuffer;

    cDefLocalsTree DefLocalsTree;
    int CurrentDefLocalsTreePos;
    int CurrentAtomicDepth;

    cGotoTable GotoTable;

public:

    cProc (cParser *par_Parser, const char *const par_ProcName, int par_ParamCount, int FileNr, int LineNr, int par_DeclaratinOrDefinitionFlag);

    ~cProc (void);

    void SetProcIdx (int par_Idx)
    {
        Idx = par_Idx;
    }

    int GetProcIdx (void)
    {
        return Idx;
    }

    char *GetName (void) 
    { 
        return Name; 
    }

    int GetParamCount (void)
    {
        return ParamCount;
    }

    char *GetParameterName (int par_Idx)
    {
        return Params[par_Idx].GetName ();
    }

    int GetParameterType (int par_Idx)
    {
        return Params[par_Idx].IsRefOrValue ();
    }

    int GetIp (void)
    {
        return Ip;
    }

    void SetIp (int par_Ip)
    {
        Ip = par_Ip;
    }

    int IsParamRefOrValue (int par_Number) 
    { 
        return Params[par_Number].IsRefOrValue ();
    }

    int IsOnlyDeclared (void)
    {
        return (DelcaredFlag && !DefinedFlag);
    }

    int GetLineNr (void)
    {
        return LineNr;
    }
    int GetFileNr (void)
    {
        return FileNr;
    }

    int AddLabel (char *par_Name, int par_FileNr, int par_Ip, unsigned long par_LineNr, unsigned long par_FileOffset)
    {
        return GotoTable.AddLabel (par_Name, par_FileNr, par_Ip, par_LineNr, par_FileOffset, CurrentDefLocalsTreePos, CurrentAtomicDepth);
    }

    int AddGoto (char *par_Name, int par_FileNr, int par_Ip, unsigned long par_LineNr, unsigned long par_FileOffset)
    {
        return GotoTable.AddGoto (par_Name, par_FileNr, par_Ip, par_LineNr, par_FileOffset, CurrentDefLocalsTreePos, CurrentAtomicDepth);
    }

    int GetLineNrOfLabel (char *par_Name, int par_FileNr)
    {
        return GotoTable.GetGotoLineNr (GotoTable.SearchLabel (par_Name, par_FileNr));
    }

    int AddDefLocals (int par_Ip)
    {
        return CurrentDefLocalsTreePos = DefLocalsTree.AddDefLocals (par_Ip);
    }

    int EndDefLocals (int par_Ip)
    {
        return CurrentDefLocalsTreePos = DefLocalsTree.EndDefLocals (par_Ip);
    }

    int GotoFromToDefLocalsPos (int par_FromPos, int par_ToPos, cParser *par_Parser, cExecutor *par_Executor)
    {
        return DefLocalsTree.GotoFromToDefLocalsPos (par_FromPos, par_ToPos, par_Parser, par_Executor);
    }

    int GetGotoFromToDefLocalsPos (int par_Idx, int *ret_FromPos, int *ret_ToPos, int *ret_Ip, int *ret_DiffAtomicDepth,
                                   char *par_Name, int par_FileNr)
    {
        return GotoTable.GetGotoFromToDefLocalsPos (par_Idx, ret_FromPos, ret_ToPos, ret_Ip, ret_DiffAtomicDepth, par_Name, par_FileNr);
    }

    int CheckIfAllGotosResoved (cParser *par_Parser);

    void IncAtomicDepth (void)
    {
        CurrentAtomicDepth++;
    }
    void DecAtomicDepth (void)
    {
        // todo: < 0 error message
        CurrentAtomicDepth--;
    }

    int PrintLabelAndGotoTable (FILE *par_FileHandle)
    {
        return GotoTable.PrintLabelAndGotoTable (par_FileHandle);
    }

};

#endif
