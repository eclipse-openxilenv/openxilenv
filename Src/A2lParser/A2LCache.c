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
#include <malloc.h>
#include "Platform.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "A2LCache.h"

FILE_CACHE *LoadFileToNewCache(const char *par_FileName)
{
    FILE_CACHE *Ret;

    Ret = my_calloc(1, sizeof(FILE_CACHE));
    if (Ret != NULL) {
#ifdef _WIN32
        HANDLE hFile;
        DWORD xx, i;
#else
        int hFile;
        struct stat FileStat;
        ssize_t xx, i;
#endif
        int Len = strlen(par_FileName) + 1;
        Ret->Filename = (char*)my_malloc(Len);
        if (Ret->Filename == NULL) {
            my_free(Ret);
            return NULL;
        }
        StringCopyMaxCharTruncate(Ret->Filename, par_FileName, Len);
#ifdef _WIN32
        hFile = CreateFile(par_FileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile == (HANDLE)(HFILE_ERROR)) {
            my_free(Ret->Filename);
            my_free(Ret);
            return NULL;
        }
#else
        hFile = open(par_FileName, O_RDONLY);
        if (hFile < 0) {
            return NULL;
        }
#endif
        Ret->RdPos = 0;
#ifdef _WIN32
        Ret->Size = GetFileSize(hFile, NULL);
#else
        if (fstat(hFile, &FileStat) != 0) {
            return NULL;
        }
        Ret->Size = FileStat.st_size;
#endif

        Ret->Buffer = (unsigned char*)my_malloc(Ret->Size + 1);
        if (Ret->Buffer == NULL) {
            my_free(Ret->Filename);
            my_free(Ret);
#ifdef _WIN32
            CloseHandle(hFile);
#else
            close(hFile);
#endif
            return NULL;
        }

        for (i = 0; (uint64_t)i < Ret->Size; i += xx) {
#ifdef _WIN32

            if (!ReadFile(hFile, (Ret->Buffer + i),
                                  Ret->Size - i, &xx, NULL)) {
#else
            if ((xx = read(hFile, (Ret->Buffer + i), Ret->Size - i)) < 0) {
#endif
                my_free(Ret->Filename);
                my_free(Ret->Buffer);
                my_free(Ret);
#ifdef _WIN32
                CloseHandle(hFile);
#else
                close(hFile);
#endif
                return NULL;
            }
        }
        Ret->Buffer[Ret->Size] = 0;

        // Byte order marks:
        if ((Ret->Size > 3) &&
            (Ret->Buffer[0] == 0xEF) &&
            (Ret->Buffer[1] == 0xBB) &&
            (Ret->Buffer[2] == 0xBF)) {   // UTF8
            Ret->RdPos = 3;
        }

#ifdef _WIN32
        CloseHandle(hFile);
#else
        close(hFile);
#endif
    }
    // we start with line 1
    Ret->LineCounter = 1;
    return Ret;
}

void DeleteCache(FILE_CACHE *par_Cache)
{
    if (par_Cache->Filename != NULL) {
        my_free(par_Cache->Filename);
    }
    if (par_Cache->Buffer != NULL) {
        my_free(par_Cache->Buffer);
    }
    my_free(par_Cache);
}

void RemoveAllCarriageReturns(FILE_CACHE *par_Cache)
{
    unsigned char* s = par_Cache->Buffer;
    unsigned char* d = par_Cache->Buffer;
    while (*s != 0) {
        if (*s != '\r') {
            if (*s == '\n') {
                par_Cache->Lines++;
            }
            *d = *s;
            d++;
        }  
        s++;
    }
    *d = 0;
    par_Cache->Size = (unsigned int)(d - par_Cache->Buffer);
}

int CacheSeek(FILE_CACHE *par_Cache, int32_t offset, int origin, int LineNr)
{
    switch (origin) {
    case SEEK_CUR:
        if (offset > 0) {
            if ((par_Cache->Size - par_Cache->RdPos) > (uint32_t)offset) par_Cache->RdPos += (uint32_t)offset;
            else par_Cache->RdPos = par_Cache->Size;
        } else {
            if ((par_Cache->RdPos) > (uint32_t)(-offset)) par_Cache->RdPos += (uint32_t)offset;
            else par_Cache->RdPos = 0;
        }
        par_Cache->LineCounter += LineNr;
        return 0;
    case SEEK_END:
        if (offset > 0) {
            if (par_Cache->RdPos > (uint32_t)offset) par_Cache->RdPos = par_Cache->Size - (uint32_t)offset;
            else par_Cache->RdPos = 0;
        }
        par_Cache->LineCounter = LineNr;
        return 0;
    case SEEK_SET:
        if (offset >= 0) {
            if (par_Cache->Size > (uint32_t)offset) par_Cache->RdPos = (uint32_t)offset;
            else par_Cache->RdPos = par_Cache->Size;
        }
        par_Cache->LineCounter = LineNr;
        return 0;
    }
    return -1;
}


int CacheTell(FILE_CACHE *par_Cache, int *ret_LineNr)
{
    *ret_LineNr = par_Cache->LineCounter;
    return (int)par_Cache->RdPos;
}

int CacheGetc(FILE_CACHE *par_Cache)
{
    int c;

    if (par_Cache->RdPos >= par_Cache->Size) {
        return EOF;  // eof
    } else {
        c = par_Cache->Buffer[par_Cache->RdPos];
        par_Cache->RdPos++;
        // normaly \r\n
        if (c == '\n') {
            par_Cache->LineCounter++;
        } else if (c == '\r') {
            if (par_Cache->RdPos < par_Cache->Size) {
                if (par_Cache->Buffer[par_Cache->RdPos] != '\n') {
                    par_Cache->LineCounter++;
                }
            }
        }
        return c;
    }
}

void CacheUnGetChar(FILE_CACHE *par_Cache)
{
    if (par_Cache->RdPos > 0) {
        par_Cache->RdPos--;
        if (par_Cache->Buffer[par_Cache->RdPos] == '\n') {
            par_Cache->LineCounter--;
        } else if (par_Cache->Buffer[par_Cache->RdPos] == '\r') {
            if ((par_Cache->RdPos+1) < par_Cache->Size) {
                if (par_Cache->Buffer[par_Cache->RdPos + 1] != '\n') {
                    par_Cache->LineCounter++;
                }
            }
        }
    }
}
