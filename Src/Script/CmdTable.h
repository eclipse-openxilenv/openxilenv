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


#ifndef CMDTABLE_H
#define CMDTABLE_H

extern "C" {
#include "ThrowError.h"
#include "MyMemory.h"
}

class cCmdTable {
private:
    struct CMD_TABLE {
        short CmdIdx;
        short BreakPointIdx;
        int BeginOfParamOffset; 
        int EndOfParamOffset; 
        int FileNr;
        int LineNr;
        int FileOffset;
        uint32_t OptParameter;
        // beim IF ist das die Position (Ip) vom ELSE oder ENDIF
        // beim ELSE ist das die Position (Ip) vom ENDIF
        // beim WHILE ist das die Position (Ip) vom ENDWHILE
        // beim BREAK ist das die Position (Ip) vom ENDWHILE 
        // beim GOTO ist das der Index in der Goto-Tabelle (diese enthaelt FileNr, Position (Ip), )
        // beim GOSUB ist das der Index in der Goto-Tabelle (diese enthaelt FileNr, Position)
        // beim CALL_PROC ist das der Index in der Proc-Tabelle (diese enthaelt FileNr, Position)
        // beim RETURN ist das ...
        // beim DEF_LOCALS ist das die Position in dem DefLocalsTree
        // beim END_DEF_LOCALS ist das die ...
        // bei DEF_PROC ist das die Position von END_DEF_PROC
        uint32_t Reserved;
    } *Table;  /* 32Byte por Befehl */
    uint32_t SizeOfTable;
    uint32_t PosInTable;

    int CheckSizeOf (void)
    {
        if (PosInTable >= SizeOfTable) {
            SizeOfTable += 64 + (SizeOfTable >> 3);
            Table = static_cast<struct CMD_TABLE*>(my_realloc (Table, sizeof (struct CMD_TABLE) * SizeOfTable));
            if (Table == nullptr) {
                ThrowError (1, "out of memory at %S (%i)", __FILE__, __LINE__);
                return -1;
            }
        }
        return 0;
    }

public:
    cCmdTable (void)
    {
        SizeOfTable = 0;
        PosInTable = 0;
        Table = nullptr;
        CheckSizeOf ();
    }

    ~cCmdTable (void)
    {
        if (Table != nullptr) my_free (Table);
        Table = nullptr;
        SizeOfTable = 0;
        PosInTable = 0;
    }

    int GetCmdIdx (uint32_t Ip)
    {
        return Table[Ip].CmdIdx;
    }


    int AddCmd (int par_CmdIdx, int par_LineNr, int par_FileNr, int par_FileOffset, uint32_t par_BeginOfParamOffset,
                uint32_t par_OptParameter, uint32_t par_Reserved)
    {
        if (CheckSizeOf ()) return -1;
        Table[PosInTable].CmdIdx = static_cast<short>(par_CmdIdx);
        Table[PosInTable].BreakPointIdx = -1;   // kein Breakpoint
        Table[PosInTable].LineNr = par_LineNr;
        Table[PosInTable].FileNr = par_FileNr;
        Table[PosInTable].FileOffset = par_FileOffset;
        Table[PosInTable].BeginOfParamOffset = static_cast<int>(par_BeginOfParamOffset);
        Table[PosInTable].OptParameter = par_OptParameter;
        Table[PosInTable].Reserved = par_Reserved;
        PosInTable++;
        return static_cast<int>(PosInTable);
    }

    int SetCmdParam (uint32_t Ip, uint32_t par_Param, uint32_t par_Reserved)
    {
        if (Ip >= PosInTable) return -1;
        Table[Ip].OptParameter = par_Param;
        Table[Ip].Reserved = par_Reserved;
        return 0;
    }

    int SetCmdReservedParam (uint32_t Ip, uint32_t par_Reserved)
    {
        if (Ip >= PosInTable) return -1;
        Table[Ip].Reserved = par_Reserved;
        return 0;
    }

    uint32_t GetCmdParam (uint32_t Ip)
    {
        if (Ip >= PosInTable) return 0;
        return Table[Ip].OptParameter;
    }

    // um im Debug-Fenster was Darzustellen
    int GetCmdInfos (uint32_t Ip, int *ret_FileNr, int *ret_LineNr, uint32_t *ret_FileOffset)
    {
        *ret_FileNr = Table[Ip].FileNr;
        *ret_LineNr = Table[Ip].LineNr;
        *ret_FileOffset = Table[Ip].FileOffset;
        return Table[Ip].CmdIdx;
    }

    uint32_t  GetOptParameterCmd (uint32_t Ip)
    {
        return Table[Ip].OptParameter;
    }

    uint32_t GetReservedCmd (uint32_t Ip)
    {
        return Table[Ip].Reserved;
    }

    int GetFileOffset (int par_Ip)
    {
        return Table[par_Ip].FileOffset;
    }

    int GetBeginOfParamOffset (int par_Ip)
    {
        return Table[par_Ip].BeginOfParamOffset;
    }

    int GetFileNr (int par_Ip)
    {
        return Table[par_Ip].FileNr;
    }

    int GetLineNr (int par_Ip)
    {
        return Table[par_Ip].LineNr;
    }

    void DebugPrintTableToFile (char *par_Filename);

    void Reset (void)
    {
        PosInTable = 0;
    }
    void OutCommandToFile (FILE *fh, int par_CmdIdx);

    int GetSizeOf (void)
    {
        return PosInTable;
    }

    int SetBreakPointAtCmd (uint32_t Ip, int par_BreakPointIdx)
    {
        if (Ip >= PosInTable) return -1;
        Table[Ip].BreakPointIdx = (short)par_BreakPointIdx;
        return 0;
    }

    int GetBreakPointAtCmd (uint32_t Ip)
    {
        if (Ip >= PosInTable) return -1;
        return Table[Ip].BreakPointIdx;
    }

    void CleanAllBreakPoints (void)
    {
        for (uint32_t x = 0; x < PosInTable; x++) {
            Table[x].BreakPointIdx = -1;
        }
    }

};

#endif
