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


#ifndef GETEXEDEBUGSIGN_H
#define GETEXEDEBUGSIGN_H

#include "Platform.h"

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
                     DWORD *ret_Age);

#endif
