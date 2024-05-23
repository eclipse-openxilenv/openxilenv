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


#ifndef REMOTEMASTEROTHER_H
#define REMOTEMASTEROTHER_H

#include <stdint.h>

int rm_Init (int par_PreAllocMemorySize, int par_PidForSchedulersHelperProcess, uint32_t *ret_Version, int32_t *ret_PatchVersion);
int rm_Terminate (void);
void rm_Kill (void);
void rm_KillThread (void);

int rm_Ping (int Value);

int rm_CopyFile(char *par_FileNameSrc, char *par_FileNameDst);

int rm_ReadBytes (int par_Pid, uint64_t par_Address, int par_Size, void *ret_Data);
int rm_WriteBytes(int par_Pid, uint64_t par_Address, int par_Size, void *par_Data);

int rm_ReferenceVariable(int par_Pid, uint64_t par_Address, const char *par_Name, int par_Type, int par_Dir);
int rm_DeReferenceVariable(int par_Pid, uint64_t par_Address, const char *par_Name, int par_Type);

#endif // REMOTEMASTEROTHER_H
