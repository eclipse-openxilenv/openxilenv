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
#include <string.h>
#include "Platform.h"
#include "FileCache.h"


extern "C" {
#include "Files.h"
#include "MyMemory.h"
}

cFileCache::cFileCache (void)
{
    State = NO_SCRIPT_FILE_LOADED;
    Filename = nullptr;
    Size = 0;
    RdPos = 0;
    Lines = 0;
    LineCounter = 1;  // Lines would be counted from 1
    CommandCount = 0;
    IpStart = 0; 
    IpEnd = 0;
    Buffer = nullptr;
}

void cFileCache::Reset (void)
{
    RdPos = 0;
    Lines = 0;
    LineCounter = 1;   // Lines would be counted from 1
    CommandCount = 0;
    IpStart = 0; 
    IpEnd = 0;
}

cFileCache::~cFileCache (void)
{
    if (Filename != nullptr) {
        my_free (Filename);
        Filename = nullptr;
    }
    if (Buffer != nullptr) {
        my_free (Buffer);
        Buffer = nullptr;
    }
}


void cFileCache::RemoveAllCarriageReturns(void)
{
    unsigned char* s = Buffer;
    unsigned char* d = Buffer;
    while (*s != 0) {
        if (*s != '\r') {
            *d = *s;
            d++;
        }
        s++;
    }
    *d = 0;
    Size = static_cast<uint32_t>(d - Buffer);
}

int cFileCache::Load (const char *par_FileName, int par_OnlyUsingFlag)
{
#ifdef _WIN32
    HANDLE hFile;
    DWORD xx, i;
#else
    int hFile;
    struct stat FileStat;
    ssize_t xx, i;
#endif

    OnlyUsingFlag = par_OnlyUsingFlag;
    Filename = static_cast<char*>(my_malloc (strlen (par_FileName)+1));
    if (Filename == nullptr) {
        return -1; 
    }
    strcpy (Filename, par_FileName);
#ifdef _WIN32
    hFile = CreateFile (par_FileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        nullptr,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        nullptr);

    if (hFile == reinterpret_cast<HANDLE>(HFILE_ERROR)) {
        my_free (Filename);
        Filename = nullptr;
        return -1;
    } 
#else
    hFile = open(par_FileName, O_RDONLY);
#endif
    LogFileAccess (Filename);

    RdPos = 0;
#ifdef _WIN32
    Size = GetFileSize (static_cast<HANDLE>(hFile), nullptr);
#else
    if (fstat(hFile, &FileStat) != 0) return -1;
    Size = FileStat.st_size;
#endif

    Buffer = static_cast<unsigned char*>(my_malloc (Size + 1));
    if (Buffer == nullptr) {
        my_free (Filename);
        Filename = nullptr;
#ifdef _WIN32
        CloseHandle (hFile);
#else
        close(hFile);
#endif
        return -1; 
    }

    for (i = 0; i < Size; i += xx) {
#ifdef _WIN32

        if (!ReadFile (hFile, (Buffer + i),
                       Size - i, &xx, nullptr)) {
#else
        if ((xx = read(hFile, (Buffer + i), Size - i)) < 0) {
#endif
            my_free (Filename);
            Filename = nullptr;
            my_free (Buffer);
            Buffer = nullptr;
#ifdef _WIN32
            CloseHandle (hFile);
#else
            close(hFile);
#endif
            return -1;
        }
    }
    Buffer[Size] = 0;

    RemoveAllCarriageReturns();

    State = SCRIPT_FILE_LOADED;

#ifdef _WIN32
    GetFileTime (hFile, nullptr, nullptr, &LastWriteTime);
    CloseHandle (hFile);
#else
    LastWriteTime = FileStat.st_mtime;
    close(hFile);
#endif
    return 0;
}

int cFileCache::AddToCache (void)
{
    int Idx;

    if (SizeOfFileCache >= MaxSizeOfFileCache) {
        MaxSizeOfFileCache += 4 + (MaxSizeOfFileCache >> 3);
        CachedFiles = static_cast<cFileCache**>(my_realloc (CachedFiles, sizeof (cFileCache*) * static_cast<size_t>(MaxSizeOfFileCache)));
        if (CachedFiles == nullptr) return -1;
    }
    Idx = SizeOfFileCache;
    SizeOfFileCache++;
    ChangedCounter++;
    CachedFiles[Idx] = this;
    return Idx;
}


void cFileCache::InitCache (void)
{
    SizeOfFileCache = 0;
}

void cFileCache::ClearCache (void)
{
    int x;

    for (x = 0; x < SizeOfFileCache; x++) {
        delete CachedFiles[x];
        CachedFiles[x] = nullptr;
    }
    SizeOfFileCache = 0;
    ChangedCounter++;
}


cFileCache *cFileCache::IsFileCached (const char *par_Filename, int *ret_FileNr)
{
    int x;

    if (CachedFiles != nullptr) {
        // Search inside the list of cached files
        for (x = 0; x < SizeOfFileCache; x++) {
            if (CachedFiles[x]->GetFilename() != nullptr) {
#ifdef WIN32
                if (!stricmp (CachedFiles[x]->GetFilename(), par_Filename)) {
#else
                if (!strcmp (CachedFiles[x]->GetFilename(), par_Filename)) {
#endif
                    if (ret_FileNr != nullptr) *ret_FileNr = x;
                    return CachedFiles[x];
                }
            }
        }
    }
    return nullptr;
}

cFileCache *cFileCache::IsFileCached (int par_FileNr)
{
    if ((CachedFiles == nullptr) || (par_FileNr >= SizeOfFileCache) || (par_FileNr < 0)) {
        return nullptr;
    } else {
        return CachedFiles[par_FileNr];
    }
}

const char *cFileCache::GetFilenameByNr (int par_FileNr)
{
    cFileCache *FileChached = cFileCache::IsFileCached (par_FileNr);
    if (FileChached == nullptr) {
        return "unknown";
    } else {
        return CachedFiles[par_FileNr]->GetFilename ();
    }
}



int cFileCache::Fseek (int32_t offset, int origin, int LineNr)
{
    switch (origin) {
    case SEEK_CUR:
        if (offset > 0) {
            if ((Size - RdPos) > static_cast<uint32_t>(offset)) RdPos += static_cast<uint32_t>(offset);
            else RdPos = Size;
        } else {
            if ((RdPos) > static_cast<uint32_t>(-offset)) RdPos += static_cast<uint32_t>(offset);
            else RdPos = 0;
        }
        LineCounter += LineNr;
        return 0;
    case SEEK_END:
        if (offset > 0) {
            if (RdPos > static_cast<uint32_t>(offset)) RdPos = Size - static_cast<uint32_t>(offset);
            else RdPos = 0;
        }
        LineCounter = LineNr;
        return 0;
    case SEEK_SET:
        if (offset >= 0) {
            if (Size > static_cast<uint32_t>(offset)) RdPos = static_cast<uint32_t>(offset);
            else RdPos = Size;
        }
        LineCounter = LineNr;
        return 0;
    }
    return -1;
}


int cFileCache::Ftell (int *ret_LineNr)
{
    *ret_LineNr = LineCounter;
    return static_cast<int>(RdPos);
}

unsigned char *cFileCache::Fgets (unsigned char *string, int n)
{
    unsigned char c;
    unsigned char *p = string;

    if (RdPos == Size) return nullptr;  // eof

    while ((RdPos < Size) &&
           (c = Buffer[RdPos]) != '\n') {
        *p++ = c;
        if ((p - string) >= n) break;
    }
    *p = 0;
    if (Buffer[RdPos] == '\n') LineCounter++;
    return string;
}


#if 1
int cFileCache::Fgetc (void)
{
    int c;

    while (1) {
        if (RdPos >= Size) return EOF;  // eof
        else {
__XYZ:
            c = Buffer[RdPos++];
            if (c == '\n') {
                LineCounter++;
                return c;
            } else {
                if (c == '\\') {
                    if (RdPos < Size) {
                        if (MyIsSpace (Buffer[RdPos])) {
                            uint32_t Pos = RdPos;
                            while (MyIsSpace (Buffer[Pos])) {
                                if (Buffer[Pos] == '\n') { 
                                    LineCounter++;
                                    // Remove whitespaces at the beginning of the next line
                                    while (MyIsSpace (Buffer[Pos])) {
                                        Pos++;
                                        if (Pos >= Size) {
                                            RdPos = Pos;
                                            return EOF;  // eof
                                        }
                                    }
                                    RdPos = Pos;
                                    goto __XYZ;
                                }
                                Pos++;
                                if (Pos >= Size) {
                                    RdPos = Pos;
                                    return EOF;  // eof
                                }
                            }
                        }
                    }
                    return c;
                } else if ((c != 26) && (c != '\r')) {
                    return c;       // ^Z (end of file)
                }
            }
        }
    }
}
#else
int cFileCache::Fgetc (void)
{
    int c;

    while (1) {
        if (RdPos >= Size) return EOF;  // eof
        else {
            c = Buffer[RdPos++];
            if (c == '\n') LineCounter++;
            if ((c != 26) && (c != '\r')) return c;       // ^Z (end of file)
        }
    }
}
#endif

int cFileCache::WhatIsNextChar (void)
{
    int c;
    uint32_t RdPosHelp;

    RdPosHelp = RdPos;
    while (1) {
        if (RdPosHelp == Size) return EOF;  // eof
        else {
            c = Buffer[RdPosHelp++];
            if ((c != 26) && (c != '\r')) return c;       // ^Z (end of file)
        }
    }
}

int cFileCache::ExtractToFile (char *par_Filename, int par_StartFileOffset, int par_StopFileOffset)
{
    FILE *fh;
    int x;

    if ((static_cast<int>(Size) < par_StopFileOffset) || (par_StartFileOffset > par_StopFileOffset)) return -1;
    if ((fh = fopen (par_Filename, "wt")) == nullptr) return -1;
    for (x = par_StartFileOffset; x < par_StopFileOffset; x++) {
        int c = Fgetc ();
        fputc (c, fh);
    }
    fclose (fh);
    return 0;
}


void cFileCache::UnGetChar (void)
{
    if (RdPos != 0) {
        do {
            RdPos--;
        } while ((RdPos > 0) && ((Buffer[RdPos] == 26) || (Buffer[RdPos] == '\r')));
        if (Buffer[RdPos] == '\n') LineCounter--;
    }
}


int cFileCache::GetNumberOfCachedFiles (void)
{
    return SizeOfFileCache;
}

 int cFileCache::GetChangedCounter(void)
 {
     return ChangedCounter;
 }

cFileCache **cFileCache::CachedFiles;
int cFileCache::SizeOfFileCache;
int cFileCache::MaxSizeOfFileCache;
int cFileCache::ChangedCounter;
