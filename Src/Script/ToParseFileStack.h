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

    int CheckSizeOf (void);
    int AddRunToFile (int par_FileIdx, int par_Ip, unsigned long par_LineNr);

public:
    cToParseFileStack (void);
    ~cToParseFileStack (void);
    int SearchFile (char *par_Name);
    int AddFile (char *par_Name);
    int AddRun (char *par_Name, int par_Ip, unsigned long par_LineNr, int par_UsingFlag);
    int GetNextFile (void);
    char *GetFileName (int par_Idx);
    int GetFileFirstRun (int par_Idx);
    int NextRunFile (int par_Idx);
    int GetRunLineNr (int par_Idx);
    int GetRunIp (int par_Idx);
    void Reset (void);
};

#endif
