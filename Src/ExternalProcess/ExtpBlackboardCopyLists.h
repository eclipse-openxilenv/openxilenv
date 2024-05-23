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


#ifndef BBCOPYLISTS_H
#define BBCOPYLISTS_H

#include "ExtpProcessAndTaskInfos.h"

unsigned long long XilEnvInternal_BuildRefUniqueId (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo);

int XilEnvInternal_GetBbVarTypeSize (int Type);

int XilEnvInternal_EnableWriteAccressToMemory (void *par_Address, size_t par_Size);

int XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo, const char *Name, unsigned long long Addr, int TypeBB);

int XilEnvInternal_InsertVariableToCopyLists (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,  const char *Name, int Vid, void *Addr, int TypeBB, int TypePipe, int Dir, unsigned long long UniqueId);

int XilEnvInternal_RemoveVariableFromCopyLists (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, const char *Name, int Vid, void *Addr, int *ret_Dir, int *ret_TypeBB, int *ret_TypePipe, int *ret_RangeControlFlag, unsigned long long *ret_UniqueId);

int XilEnvInternal_CopyFromPipeToRefWithOverlayCheck (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, char *SnapShotData, int SnapShotSize);

int XilEnvInternal_CopyFromPipeToRef (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, char *SnapShotData, int SnapShotSize);

int XilEnvInternal_CopyFromRefToPipe (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, char *SnapShotData);

int XilEnvInternal_DeReferenceAllVariables (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos);

int XilEnvInternal_GetReferencedVariableIdentifierNameByAddress (void *Address);

int XilEnvInternal_AddAddressToOverlayBuffer (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, void *Address, int Size);

#endif
