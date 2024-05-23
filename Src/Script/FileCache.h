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


#ifndef FILECACHE_H
#define FILECACHE_H

/* Beispiel:
    cFileCache *NewScriptFile;

    NewScriptFile = new cFileCache;
    NewScriptFile->Load (ScriptFilename);
    NewScriptFile->AddToCache ();


*/

#include "Platform.h"
#include <stdint.h>

class cFileCache {
private:
    enum { NO_SCRIPT_FILE_LOADED = 0,
           SCRIPT_FILE_LOADED = 1,
           SCRIPT_FILE_USED = 2,     // Script-File wurde ?ber den Befehl USING geladen (nur die Befehle DEF_PROC wurden ausgewertet
           SCRIPT_FILE_PRECOMPILED = 3}
        State;
    char *Filename;
    unsigned int Size;
    unsigned int RdPos;

    int Lines;
    int LineCounter;
    int CommandCount;

    int IpStart;  // Begin in der CmdTable
    int IpEnd;    // Ende in der CmdTable muss ein EOF Command sein.

    /*int IsMainScriptFlag;
    int CallByRunFlag;
    int CallByUsingFlag;*/
    int OnlyUsingFlag;

    unsigned char *Buffer;
#ifdef _WIN32
    FILETIME LastWriteTime;
#else
    time_t LastWriteTime;
#endif
    static cFileCache **CachedFiles;
    static int SizeOfFileCache;
    static int MaxSizeOfFileCache;

    static int ChangedCounter;

    void RemoveAllCarriageReturns(void);

public:
    cFileCache (void);
    ~cFileCache (void);

    int Load (const char *par_FileName, int par_OnlyUsingFlag);

    const char *GetFilename (void)
    {
        return Filename;
    }
/*
    void IncLineCounter (void)
    {
        LineCounter++;
    }

    void DecLineCounter (void)
    {
        LineCounter--;
    }
*/
    int GetLineCounter (void)
    {
       return LineCounter;
    }
/*
    void SetLineCounter (int par_LineCounter)
    {
       LineCounter = par_LineCounter;
    }
*/
    char *GetPtr (uint32_t par_FileOffset)
    {
        if (par_FileOffset < Size) {
            return reinterpret_cast<char*>(Buffer) + par_FileOffset;
        } else {
            return nullptr;
        }
    }

    int GetIpStart (void)
    {
        return IpStart;
    }

    int GetIpEnd (void)
    {
        return IpEnd;
    }

    void SetIpStart (int par_IpStart)
    {
        IpStart = par_IpStart;
    }

    void SetIpEnd (int par_IpEnd)
    {
        IpEnd = par_IpEnd;
    }

    int Fseek (int32_t offset, int origin, int LineNr);

    int Ftell (int *ret_LineNr);

    unsigned char *Fgets(unsigned char *string, int n);

    int Fgetc (void);
    void UnGetChar (void);

    int WhatIsNextChar (void);

    int AddToCache (void);


    int IsOnlyUsing (void)
    {
        return OnlyUsingFlag;
    }
    void RestUsingFlag (void)
    {
        OnlyUsingFlag = 0;
    }

    void Reset (void);

    int ExtractToFile (char *par_Filename, int par_StartFileOffset, int par_StopFileOffset);

#ifdef _WIN32
    FILETIME GetLastWriteTime (void)
#else
    time_t GetLastWriteTime (void)
#endif
    {
        return LastWriteTime;
    }

    static void InitCache (void);
    static void ClearCache (void);
    static cFileCache *IsFileCached (const char *par_Filename, int *ret_FileNr = nullptr);
    static cFileCache *IsFileCached (int par_FileNr);

    static const char *GetFilenameByNr(int par_FileNr);

    static int GetNumberOfCachedFiles();

    static int GetChangedCounter();

} ;

#if 1
inline int MyIsSpace (int Char)
{
    switch (Char >> 5) {
    case 0:
        return ((0x00003E00 & (1 << (Char & 0x1F))) != 0);
    case 1:
        return ((0x00000001 & (1 << (Char & 0x1F))) != 0);
    default:
        return 0;
    }
}
#else
inline int MyIsSpace (int Char)
{
    static uint32_t Mask[256/32] = {0x00003E00, 0x00000001, 0x00000000, 0x00000000,
                                         0x00000000, 0x00000000, 0x00000000, 0x00000000};
    if (Char > 255) return 0;
    else if (Char < 0) return 0;
    else return Mask[Char >> 5] & (1 << (Char & 0x1F));
}
#endif

#endif
