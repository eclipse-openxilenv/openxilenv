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


#ifndef READEXEINFOS_H
#define READEXEINFOS_H

#include <stdint.h>

int ReadExeInfos (char *Filename, uint64_t *pBaseAddr, unsigned int *pLinkerVersion,
                  int *pNumOfSections,
                  char ***pRetSectionNames,
                  uint64_t **pRetSectionVirtualAddresses,
                  uint32_t **pRetSectionVirtualSize,
                  uint32_t **pRetSectionSizeOfRawData,
                  uint64_t **pRetSectionPointerToRawData,

                  int *ret_SignatureType,
                  DWORD *ret_Signature,
                  GUID *ret_SignatureGuid,
                  DWORD *ret_Age,

                  int *pDynamicBaseAddress, int IsRealtimeProcess, int MachineType);

#define LINKER_TYPE_UNKNOWN    0
#define LINKER_TYPE_VISUALC7   1
#define LINKER_TYPE_BORLAND    2
#define LINKER_TYPE_GCC        3

int32_t GetExeFileTimeAndChecksum (char *ExeName, uint64_t *RetLastWriteTime, DWORD *RetCheckSum, int32_t *Linker, int IsRealtimeProcess, int MachineType);

int GetElfSectionVersionInfos (const char *Filename, const char *par_Section,
                               int *ret_OwnVersion, int *ret_OwnMinorVersion, int *ret_RemoteMasterLibraryAPIVersion);

#endif
