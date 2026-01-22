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
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

extern "C" {
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "IniDataBase.h"
#include "Files.h"
}

#include "FileCache.h"
#include "CmdTable.h"
#include "Breakpoints.h"

#define SCRIPT_BREAKBPOINT_INI_SECTION "Buildin Script Breakpoints" 

int cBreakPoints::CheckIfBreakPointHit (int par_BreakPointIdx)
{
    if (par_BreakPointIdx < PosInBreakPointTable) {
        // Conditons und HitCounter
        return 1;
    }
    return 0;
}

int cBreakPoints::AddBreakPoint (int par_Active, int par_FileNr, const char *par_Filename,
                                 int par_LineNr, int par_Ip, int par_HitCount, const char *par_Condition, cCmdTable *par_CmdTable,
                                 FILETIME *par_LastWriteTime)
{
    if (IsThereABreakPoint (par_Filename, par_LineNr)) {
        return -1; // Breakpoint already exist
    }
    if (CheckSizeOf ()) return -1;
    if (par_Active) BreakPointTable[PosInBreakPointTable].ActiveValid = BREAKPOINT_ACTIVE_AND_VALID_MASK;
    else BreakPointTable[PosInBreakPointTable].ActiveValid = 0;
    BreakPointTable[PosInBreakPointTable].FileNr = par_FileNr;
    BreakPointTable[PosInBreakPointTable].LineNr = par_LineNr;
    BreakPointTable[PosInBreakPointTable].Ip = par_Ip;
    BreakPointTable[PosInBreakPointTable].HitCounter = 0;
    BreakPointTable[PosInBreakPointTable].MaxHitCounter = par_HitCount;
    BreakPointTable[PosInBreakPointTable].Filename = StringMalloc (par_Filename);
    if (BreakPointTable[PosInBreakPointTable].Filename == nullptr) return -1;
    if (par_Condition == nullptr) {
        BreakPointTable[PosInBreakPointTable].Condition = nullptr;
    } else if (strlen (par_Condition) == 0) {
        BreakPointTable[PosInBreakPointTable].Condition = nullptr;
    } else {
        BreakPointTable[PosInBreakPointTable].Condition = StringMalloc (par_Condition);
        if (BreakPointTable[PosInBreakPointTable].Condition == nullptr) return -1;
    }
    if (par_LastWriteTime != nullptr) {
        BreakPointTable[PosInBreakPointTable].LastWriteTime = *par_LastWriteTime;
    } else {
        // Store time of the last write access
        cFileCache *FileCache = cFileCache::IsFileCached (par_FileNr);
        if (FileCache != nullptr) {
            BreakPointTable[PosInBreakPointTable].LastWriteTime = FileCache->GetLastWriteTime ();
        } else {
#ifdef _WIN32
            BreakPointTable[PosInBreakPointTable].LastWriteTime.dwLowDateTime = 0;
            BreakPointTable[PosInBreakPointTable].LastWriteTime.dwHighDateTime = 0;
#else
            BreakPointTable[PosInBreakPointTable].LastWriteTime = 0;
#endif
        }
    }
    if (par_CmdTable != nullptr) par_CmdTable->SetBreakPointAtCmd (static_cast<uint32_t>(par_Ip), PosInBreakPointTable);
    PosInBreakPointTable++;
    BreakpointChangedCounter++;
    return 0;
}

int cBreakPoints::DelBreakPoint (const char *par_Filename, int par_LineNr, cCmdTable *par_CmdTable)
{
    for (int x = 0; x < PosInBreakPointTable; x++) {
#ifdef _WIN32
        if (!strcmpi (BreakPointTable[x].Filename, par_Filename) && (BreakPointTable[x].LineNr == par_LineNr)) {
#else
        if (!strcmp (BreakPointTable[x].Filename, par_Filename) && (BreakPointTable[x].LineNr == par_LineNr)) {
#endif
            if ((BreakPointTable[x].ActiveValid & BREAKPOINT_ACTIVE_AND_VALID_MASK) == BREAKPOINT_ACTIVE_AND_VALID_MASK) {
                if (BreakPointTable[x].Ip >= 0) par_CmdTable->SetBreakPointAtCmd (static_cast<uint32_t>(BreakPointTable[x].Ip), -1);
            }
            my_free (BreakPointTable[x].Filename);
            if (BreakPointTable[x].Condition != nullptr) my_free (BreakPointTable[x].Condition);
            PosInBreakPointTable--;
            BreakpointChangedCounter++; // only for change detection
            if (x < PosInBreakPointTable) {
                memmove (&BreakPointTable[x], &BreakPointTable[x+1], static_cast<size_t>(PosInBreakPointTable - x) * sizeof (BreakPointTable[0]));
            }
            return 0;
        }
    }
    return -1;
}

int cBreakPoints::IsThereABreakPoint (const char *par_Filename, int par_LineNr)
{
    for (int x = 0; x < PosInBreakPointTable; x++) {
#ifdef _WIN32
        if (!strcmpi (BreakPointTable[x].Filename, par_Filename) && (BreakPointTable[x].LineNr == par_LineNr)) {
#else
        if (!strcmp (BreakPointTable[x].Filename, par_Filename) && (BreakPointTable[x].LineNr == par_LineNr)) {
#endif
            return 1;
        }
    }
    return 0;
}


int cBreakPoints::InitBreakPointsFromIni (cCmdTable *par_CmdTable)
{
    char Line[INI_MAX_LINE_LENGTH];
    char Entry[32];
    int Fd = GetMainFileDescriptor();

    for (int x = 0; x < 1000; x++) {
        PrintFormatToString (Entry, sizeof(Entry), "B%i", x);
        if (IniFileDataBaseReadString (SCRIPT_BREAKBPOINT_INI_SECTION, Entry, "", Line, sizeof (Line), Fd)) {
            char *ActiveStr, *FilenameStr, *LineNrStr, *MaxHitCounterStr, *LastWriteTimeHighStr, *LastWriteTimeLowStr, *ConditionStr = nullptr;
            if (StringCommaSeparate (Line, &ActiveStr, &FilenameStr, &LineNrStr, &MaxHitCounterStr, &LastWriteTimeHighStr, &LastWriteTimeLowStr, &ConditionStr, nullptr) >= 4) {
#ifdef _WIN32
                FILETIME LastWriteTime;
                LastWriteTime.dwHighDateTime = strtoul (LastWriteTimeHighStr, nullptr, 10);
                LastWriteTime.dwLowDateTime = strtoul (LastWriteTimeLowStr, nullptr, 10);
#else
                time_t LastWriteTime;
                LastWriteTime = strtoul (LastWriteTimeLowStr, nullptr, 10);
#endif
                if (AddBreakPoint (atol (ActiveStr), -1, FilenameStr, atol (LineNrStr), -1, atol (MaxHitCounterStr), ConditionStr, nullptr, &LastWriteTime)) {
                    return -1;
                }
            }
        } else {
            break;
        }
    }
    CheckAllBreakPoints (par_CmdTable);
    return 0;
}

int cBreakPoints::SaveBreakPointsToIni (void)
{
    char Line[INI_MAX_LINE_LENGTH];
    char Entry[32];
    int Fd = GetMainFileDescriptor();

    IniFileDataBaseWriteString (SCRIPT_BREAKBPOINT_INI_SECTION, nullptr, nullptr, Fd);
    for (int x = 0; x < PosInBreakPointTable; x++) {
        PrintFormatToString (Entry, sizeof(Entry), "B%i", x);
#ifdef _WIN32
        PrintFormatToString (Line, sizeof(Line), "%i, %s, %i, %i, %lu, %lu",
#else
        PrintFormatToString (Line, sizeof(Line), "%i, %s, %i, %i, %u, %lu",
#endif
                 (BreakPointTable[x].ActiveValid & BREAKPOINT_ACTIVE_MASK) == BREAKPOINT_ACTIVE_MASK,
                 BreakPointTable[x].Filename,
                 BreakPointTable[x].LineNr,
                 BreakPointTable[x].MaxHitCounter,
#ifdef _WIN32
                 BreakPointTable[x].LastWriteTime.dwHighDateTime,
                 BreakPointTable[x].LastWriteTime.dwLowDateTime);
#else
                 0,
                 BreakPointTable[x].LastWriteTime);
#endif

        if (BreakPointTable[x].Condition != nullptr) {
            PrintFormatToString (Line + strlen (Line), sizeof(Line) - strlen (Line), ", %s",
                     BreakPointTable[x].Condition);
        }
        IniFileDataBaseWriteString (SCRIPT_BREAKBPOINT_INI_SECTION, Entry, Line, Fd);
    }
    ClearAllBreakPoints ();
    return 0;
}

void cBreakPoints::ClearAllBreakPoints (void)
{
    for (int x = 0; x < PosInBreakPointTable; x++) {
        my_free (BreakPointTable[x].Filename);
        if (BreakPointTable[x].Condition != nullptr) my_free (BreakPointTable[x].Condition);
    }
    PosInBreakPointTable = 0;
    BreakpointChangedCounter++;
}

int cBreakPoints::CheckIfBreakPointIsValid (const char *par_Filename, int par_LineNr, cCmdTable *par_CmdTable,
                                            int *ret_FileNr, int *ret_Ip, FILETIME *par_LastWriteTime)
{
    int Ret = 0;

    for (int x = 0; x < PosInBreakPointTable; x++) {
        if (!strcmp (BreakPointTable[x].Filename, par_Filename) && (BreakPointTable[x].LineNr == par_LineNr)) {
            Ret = 0x2;   // There are already a breakpoint at this line
        }
    }
    int FileNr;
    cFileCache *FileCached = cFileCache::IsFileCached (par_Filename, &FileNr);
    if (FileCached != nullptr) {
        if (par_LastWriteTime != nullptr) {
            FILETIME LastWriteTime = FileCached->GetLastWriteTime ();
#ifdef _WIN32
            if ((par_LastWriteTime->dwHighDateTime != LastWriteTime.dwHighDateTime) ||
                (par_LastWriteTime->dwLowDateTime != LastWriteTime.dwLowDateTime) ) {
#else
            if (*par_LastWriteTime != LastWriteTime) {
#endif
                return Ret;
            }
        }
        for (int Ip = FileCached->GetIpStart (); Ip < FileCached->GetIpEnd (); Ip++) {
            if (par_CmdTable->GetLineNr (Ip) == par_LineNr) {
                if (par_CmdTable->GetFileNr (Ip) != FileNr) {
                    ThrowError (1, "Internal error %s (%i)", __FILE__, __LINE__);
                    return 0;
                } else {
                    if (ret_FileNr != nullptr) *ret_FileNr = FileNr;
                    if (ret_Ip != nullptr) *ret_Ip = Ip;
                    return (Ret + 1);
                }
            }
        }
    }
    return Ret;
}

int cBreakPoints::CheckAllBreakPoints (cCmdTable *par_CmdTable)
{
    int Ret= 0;

    if (par_CmdTable != nullptr) par_CmdTable->CleanAllBreakPoints ();
    for (int x = 0; x < PosInBreakPointTable; x++) {
        if ((BreakPointTable[x].ActiveValid & BREAKPOINT_ACTIVE_MASK) == BREAKPOINT_ACTIVE_MASK) {
            // First mark as invalid
            BreakPointTable[x].Ip = -1;
            BreakPointTable[x].FileNr = -1;
            BreakPointTable[x].ActiveValid &= ~BREAKPOINT_VALID_MASK;
            int Ip;
            int FileNr;
            if (CheckIfBreakPointIsValid (BreakPointTable[x].Filename, BreakPointTable[x].LineNr, par_CmdTable,
                                          &FileNr, &Ip, &BreakPointTable[x].LastWriteTime) & 0x1) {
                // Breakpoint is valid
                BreakPointTable[x].Ip = Ip;
                BreakPointTable[x].FileNr = FileNr;
                BreakPointTable[x].ActiveValid |= BREAKPOINT_VALID_MASK;
                par_CmdTable->SetBreakPointAtCmd (static_cast<uint32_t>(Ip), x);
                Ret++;
            }
        }
    }
    return Ret;  // Number of valid breakpoints
}

int cBreakPoints::GetBreakpointString (int par_Index, char *ret_String, int par_Maxc)
{
    if (par_Index < PosInBreakPointTable) {
        const char *p;
        switch (BreakPointTable[par_Index].ActiveValid) {
        case BREAKPOINT_ACTIVE_MASK:
            p = "XA";
            break;
        case BREAKPOINT_VALID_MASK:
            p = "XV";
            break;
        case BREAKPOINT_ACTIVE_AND_VALID_MASK:
            p = "V ";
            break;
        default:
            p = "X ";
            break;
        }
        PrintFormatToString (ret_String, par_Maxc, "%s \t%s\t%i\th=%i", p,
                 BreakPointTable[par_Index].Filename,
                 BreakPointTable[par_Index].LineNr,
                 BreakPointTable[par_Index].MaxHitCounter);
        if (BreakPointTable[par_Index].Condition != nullptr) {
            PrintFormatToString (ret_String + strlen (ret_String), par_Maxc - strlen (ret_String), ", c=%s", BreakPointTable[par_Index].Condition);
        }
        return 0;
    }
    return -1;
}

int cBreakPoints::GetBreakpointChangedCounter()
{
    return BreakpointChangedCounter;
}


int cBreakPoints::GetNext (int par_StartIdx, int par_FileNr, int *ret_Ip, int *ret_LineNr, int *ret_ActiveValid)
{
    for (int x = par_StartIdx + 1; x < PosInBreakPointTable; x++) {
        if (BreakPointTable[x].FileNr == par_FileNr) {
            if (ret_LineNr != nullptr) *ret_LineNr = BreakPointTable[x].LineNr;
            if (ret_ActiveValid != nullptr) *ret_ActiveValid = BreakPointTable[x].ActiveValid;
            if (ret_Ip != nullptr) *ret_Ip = BreakPointTable[x].Ip;
            return x;
        }
    }
    return -1;
}

cBreakPoints::cBreakPoints (void)
{
    BreakPointTable = nullptr;
    SizeOfBreakPointTable = 0;
    PosInBreakPointTable = 0;
    BreakpointChangedCounter = 0;
}


cBreakPoints::~cBreakPoints (void)
{
    ClearAllBreakPoints ();
}
