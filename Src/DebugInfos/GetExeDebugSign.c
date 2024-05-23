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
#include <stdlib.h>
#include "Platform.h"

#include "GetExeDebugSign.h"


struct CV_HEADER
{
    DWORD Signature;
    DWORD Offset;
};

struct CV_INFO_PDB20
{
    struct CV_HEADER CvHeader; //CvHeader.Signature = "NB10"
    DWORD Signature;
    DWORD Age;
    BYTE PdbFileName[1];
};

struct CV_INFO_PDB70
{
    DWORD CvSignature; //"RSDS"
    GUID Signature;
    DWORD Age;
    BYTE PdbFileName[1];
};

#define UNUSED(x) (void)(x)

int GetExeDebugSign (MY_FILE_HANDLE hFile, IMAGE_DATA_DIRECTORY *Directory,
                     uint64_t BaseAddr,
                     int32_t NumOfSections,
                     char **SectionNames,
                     uint64_t *SectionVirtualAddress,
                     uint32_t *SectionSizeOfRawData,
                     uint64_t *SectionPointerToRawData,
                     int *ret_SignatureType,
                     DWORD *ret_Signature,
                     GUID *ret_SignatureGuid,
                     DWORD *ret_Age
                     )
{
    UNUSED(BaseAddr);
    UNUSED(SectionNames);
    int x;
    uint64_t BaseOffset, Offset;

    *ret_SignatureType = 0;
    // search in which section?
    for (x = 0; x < NumOfSections; x++) {
        if ((Directory->VirtualAddress >= SectionVirtualAddress[x]) &&
            (Directory->VirtualAddress < (SectionVirtualAddress[x] + SectionSizeOfRawData[x]))) {
            break;
        }
    }
    // it must be inside a section
    if (x < NumOfSections) {
        IMAGE_DEBUG_DIRECTORY Entry;
        BaseOffset = Directory->VirtualAddress - SectionVirtualAddress[x] + SectionPointerToRawData[x];

        for (Offset = 0; (Offset + sizeof(IMAGE_DEBUG_DIRECTORY)) <= Directory->Size; Offset += sizeof(IMAGE_DEBUG_DIRECTORY)) {
            my_lseek (hFile, BaseOffset + Offset);
            my_read (hFile, &Entry, sizeof (IMAGE_DEBUG_DIRECTORY));
            if (Entry.Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
                if (Entry.PointerToRawData > 0) {
                    DWORD Signature;
                    my_lseek (hFile, Entry.PointerToRawData);
                    my_read (hFile, &Signature, sizeof (DWORD));
                    my_lseek (hFile, Entry.PointerToRawData);

                    if(Signature == (((DWORD)'0' << 24) |
                                     ((DWORD)'1' << 16) |
                                     ((DWORD)'B' << 8) |
                                      ((DWORD)'N')) /* '01BN' */) {
                        struct CV_INFO_PDB20 cv;
                        my_read (hFile, &cv, sizeof (struct CV_INFO_PDB20));
                        *ret_SignatureType = 1;
                        *ret_Signature = cv.Signature;
                        *ret_Age = cv.Age;
                        return 0;
                    } else if(Signature == (((DWORD)'S' << 24) |
                                            ((DWORD)'D' << 16) |
                                            ((DWORD)'S' << 8) |
                                            ((DWORD)'R')) /* 'SDSR' */) {
                        struct CV_INFO_PDB70 cv;
                        my_read (hFile, &cv, sizeof (struct CV_INFO_PDB70));
                        *ret_SignatureType = 2;
                        *ret_SignatureGuid = cv.Signature;
                        *ret_Age = cv.Age;
                        return 0;
                    }
                }
            }
        }
    }
    // no signature found
    return -1;
}
