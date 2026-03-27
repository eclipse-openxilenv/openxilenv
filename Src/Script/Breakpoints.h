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


#ifndef BREAKPOINTS_H
#define BREAKPOINTS_H

class cCmdTable;

class cBreakPoints {
private:
    struct BREAKPOINT_TABLE {
        int ActiveValid;
#define BREAKPOINT_VALID_MASK             0x2
#define BREAKPOINT_ACTIVE_MASK            0x1
#define BREAKPOINT_ACTIVE_AND_VALID_MASK  0x3
        int Ip;
        int FileNr;
        char *Filename;
        int LineNr;
        unsigned long FileOffset;
        int HitCounter;
        int MaxHitCounter;
        char *Condition;
#ifdef _WIN32
    FILETIME LastWriteTime;
#else
    time_t LastWriteTime;
#endif
    } *BreakPointTable;

    int SizeOfBreakPointTable;
    int PosInBreakPointTable;
    int BreakpointChangedCounter;

    int CheckSizeOf (void)
    {
        if (PosInBreakPointTable >= SizeOfBreakPointTable) {
            SizeOfBreakPointTable += 4 + (SizeOfBreakPointTable >> 3);
            BreakPointTable = (struct BREAKPOINT_TABLE*)my_realloc (BreakPointTable, sizeof (struct BREAKPOINT_TABLE) * SizeOfBreakPointTable);
        }
        return ((BreakPointTable == NULL) ? -1 : 0);
    }
     
public:
    cBreakPoints (void);
    ~cBreakPoints (void);

    int CheckIfBreakPointHit (int par_BreakPointIdx);
    int AddBreakPoint (int par_Active, int par_FileNr, const char *par_Filename,
                       int par_LineNr, int par_Ip, int par_HitCount, const char *par_Condition, cCmdTable *par_CmdTable,
                       FILETIME *par_LastWriteTime);
    int DelBreakPoint (const char *par_Filename, int par_LineNr, cCmdTable *par_CmdTable);
    int IsThereABreakPoint (const char *par_Filename, int par_LineNr);
    int InitBreakPointsFromIni (cCmdTable *par_CmdTable);
    int SaveBreakPointsToIni (void);
    void ClearAllBreakPoints (void);
    int CheckIfBreakPointIsValid (const char *par_Filename, int par_LineNr, cCmdTable *par_CmdTable,
                                  int *ret_FileNr, int *ret_Ip, FILETIME *par_LastWriteTime);
    int CheckAllBreakPoints (cCmdTable *par_CmdTable);
    int GetBreakpointString (int par_Index, char *ret_String, int par_Maxc);
    int GetBreakpointChangedCounter (void);
    int GetNext (int par_StartIdx, int par_FileNr, int *ret_Ip, int *ret_LineNr, int *ret_ActiveValid);
};


#endif
