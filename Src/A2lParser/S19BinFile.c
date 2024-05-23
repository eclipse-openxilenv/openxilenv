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
#include "MyMemory.h"
#include "Blackboard.h"
#include "StringMaxChar.h"
#include "A2LAccessData.h"
#include "S19BinFile.h"

BIN_FILE_IMAGE *LoadS19File(const char *par_FileName, uint64_t par_Offset, uint64_t par_Size)
{
    BIN_FILE_IMAGE *Ret;
    FILE *fh;
    char Line[1024];
    int LineCounter = 0;

    Ret = (BIN_FILE_IMAGE*)my_calloc(1, sizeof(BIN_FILE_IMAGE));
    if (Ret != NULL) {
        Ret->Filename = StringMalloc(par_FileName);
        if (Ret->Filename == NULL) {
            my_free(Ret);
            return NULL;
        }
        fh = fopen(par_FileName, "rt");
        if (fh == NULL) {
            my_free(Ret->Filename);
            my_free(Ret);
            return NULL;
        }
        Ret->Size = par_Size;
        Ret->Offset = par_Offset;
        Ret->Image = (unsigned char*)my_calloc(1, Ret->Size);
        if (Ret->Image == NULL) {
            my_free(Ret->Filename);
            my_free(Ret);
        }

        while (fgets(Line, 1024, fh) != NULL) {
            LineCounter++;
            if ((Line[0] == 'S') && (Line[1] == '3')) {
                char *End;
                uint32_t ByteCount;
                char ByteString[3];
                uint32_t Address;
                char AddressString[9];
                MEMCPY(ByteString, Line + 2, 2);
                ByteString[2] = 0;
                ByteCount = strtoul(ByteString, &End, 16);
                if (End == ByteString + 2) {
                    MEMCPY(AddressString, Line + 4, 8);
                    AddressString[8] = 0;
                    Address = strtoul(AddressString, &End, 16);
                    if (End == AddressString + 8) {
                        uint8_t Byte, Checksum;
                        int ByteNr;
                        ByteCount -= 5;  // 5 Address + checksumm
                        for (ByteNr = 0; ByteNr < (int)ByteCount; ByteNr++) {
                            MEMCPY(ByteString, Line + 12 + (ByteNr << 1), 2);
                            ByteString[2] = 0;
                            Byte = (uint8_t)strtoul(ByteString, &End, 16);

                            if (Address >= par_Offset) {
                                if (Address < (par_Offset + par_Size)) {
                                    Ret->Image[Address - par_Offset] = Byte;
                                }
                            }
                            Address++;
                        }
                        MEMCPY(ByteString, Line + 8 + (ByteNr << 1), 2);
                        ByteString[2] = 0;
                        Checksum = (uint8_t)strtoul(ByteString, &End, 16);
                        // will be ignored
                    }
                }
            }
        }
        fclose(fh);
    }
    return Ret;
}

BIN_FILE_IMAGE *LoadBinFile(const char *par_FileName)
{
    BIN_FILE_IMAGE *Ret;

    Ret = (BIN_FILE_IMAGE*)my_calloc(1, sizeof(BIN_FILE_IMAGE));
    if (Ret != NULL) {
#ifdef _WIN32
        HANDLE hFile;
        DWORD xx, i;
#else
        int hFile;
        struct stat FileStat;
        ssize_t xx, i;
#endif

        Ret->Filename = StringMalloc(par_FileName);
        if (Ret->Filename == NULL) {
            my_free(Ret);
            return NULL;
        }
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
#endif
        Ret->Offset = 0;
#ifdef _WIN32
        Ret->Size = GetFileSize(hFile, NULL);
#else
        if (fstat(hFile, &FileStat) != 0) {
            return NULL;
        }
        Ret->Size = FileStat.st_size;
#endif

        Ret->Image = (unsigned char*)my_malloc(Ret->Size);
        if (Ret->Image == NULL) {
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

            if (!ReadFile(hFile, (Ret->Image + i),
                (DWORD)Ret->Size - i, &xx, NULL)) {
#else
            if ((xx = read(hFile, (Ret->Image + i), Ret->Size - i)) < 0) {
#endif
                my_free(Ret->Filename);
                my_free(Ret->Image);
                my_free(Ret);
#ifdef _WIN32
                CloseHandle(hFile);
#else
                close(hFile);
#endif
                return NULL;
            }
            }

#ifdef _WIN32
        CloseHandle(hFile);
#else
        close(hFile);
#endif
        }
    return Ret;
}

void CloseBinImage(BIN_FILE_IMAGE *par_Image)
{
    if (par_Image != NULL) {
        if (par_Image->Image != NULL) my_free(par_Image->Image);
        if (par_Image->Filename != NULL) my_free(par_Image->Filename);
        my_free(par_Image);
    }
}

int ReadValueFromImage(BIN_FILE_IMAGE *par_Image, uint64_t par_Address, uint32_t par_Flags, int par_Type, A2L_SINGLE_VALUE *ret_Value)
{
    if (par_Address >= par_Image->Offset) {
        int ByteSize;
        ret_Value->Address = par_Address;
        ret_Value->Flags = par_Flags;
        ret_Value->TargetType = par_Type;
        par_Address -= par_Image->Offset;
        switch (par_Type) {
        case BB_BYTE:
            ret_Value->Type = A2L_ELEM_TYPE_INT;
            ByteSize = 1;
            if ((par_Address + ByteSize) > par_Image->Size) return -1;
            ret_Value->Value.Int = *(int8_t*)&(par_Image->Image[par_Address]);
            return 0;
        case BB_UBYTE:
            ret_Value->Type = A2L_ELEM_TYPE_UINT;
            ByteSize = 1;
            if ((par_Address + ByteSize) > par_Image->Size) return -1;
            ret_Value->Value.Int = *(uint8_t*)&(par_Image->Image[par_Address]);
            return 0;
        case BB_WORD:
            ret_Value->Type = A2L_ELEM_TYPE_INT;
            ByteSize = 2;
            if ((par_Address + ByteSize) > par_Image->Size) return -1;
            ret_Value->Value.Int = *(int16_t*)&(par_Image->Image[par_Address]);
            return 0;
        case BB_UWORD:
            ret_Value->Type = A2L_ELEM_TYPE_UINT;
            ByteSize = 2;
            if ((par_Address + ByteSize) > par_Image->Size) return -1;
            ret_Value->Value.Int = *(uint16_t*)&(par_Image->Image[par_Address]);
            return 0;
        case BB_DWORD:
            ret_Value->Type = A2L_ELEM_TYPE_INT;
            ByteSize = 4;
            if ((par_Address + ByteSize) > par_Image->Size) return -1;
            ret_Value->Value.Int = *(int32_t*)&(par_Image->Image[par_Address]);
            return 0;
        case BB_UDWORD:
            ret_Value->Type = A2L_ELEM_TYPE_UINT;
            ByteSize = 4;
            if ((par_Address + ByteSize) > par_Image->Size) return -1;
            ret_Value->Value.Int = *(uint32_t*)&(par_Image->Image[par_Address]);
            return 0;
        case BB_FLOAT:
            ret_Value->Type = A2L_ELEM_TYPE_DOUBLE;
            ByteSize = 8;
            if ((par_Address + 4) > par_Image->Size) return -1;
            ret_Value->Value.Double = (double)*(float*)&(par_Image->Image[par_Address]);
            return 0;
        case BB_DOUBLE:
            ret_Value->Type = A2L_ELEM_TYPE_DOUBLE;
            ByteSize = 8;
            if ((par_Address + ByteSize) > par_Image->Size) return -1;
            ret_Value->Value.Double = *(double*)&(par_Image->Image[par_Address]);
            return 0;
        case BB_QWORD:
            ret_Value->Type = A2L_ELEM_TYPE_INT;
            ByteSize = 8;
            if ((par_Address + ByteSize) > par_Image->Size) return -1;
            ret_Value->Value.Int = *(int64_t*)&(par_Image->Image[par_Address]);
            return 0;
        case BB_UQWORD:
            ret_Value->Type = A2L_ELEM_TYPE_UINT;
            ByteSize = 8;
            if ((par_Address + ByteSize) > par_Image->Size) return -1;
            ret_Value->Value.Int = *(uint64_t*)&(par_Image->Image[par_Address]);
            return 0;
        default:
            return -1;
        }
    }
    return -1;
}
