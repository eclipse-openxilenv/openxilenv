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


#ifndef SCBBCOPYLISTS_H
#define SCBBCOPYLISTS_H

#include "tcb.h"

int InsertVariableToCopyLists (int Pid, int Vid, unsigned long long Addr, int TypeBB, int TypePipe, int Dir, unsigned long long UniqueId);

int RemoveVariableFromCopyLists (int Pid, int Vid, unsigned long long UniqueId, unsigned long long Addr);

int RemoveAllVariableFromCopyLists (int Pid);

int CopyFromBlackbardToPipe (TASK_CONTROL_BLOCK *Tcb, char *SnapShotData);

int CopyFromPipeToBlackboard (TASK_CONTROL_BLOCK *Tcb, char *SnapShotData, int SnapShotSize);

int DeReferenceAllVariables (TASK_CONTROL_BLOCK *Tcb);

int GetReferencedLabelNameByVid (int Pid, int Vid, char *LabelName, int MaxChar);

int GetCopyListForProcess (int par_Pid, int par_ListType, unsigned long long **ret_Addresses, int **ret_Vids, short **ret_TypeBB, short **ret_TypePipe);

void FreeCopyLists (PIPE_MESSAGE_REF_COPY_LISTS *par_CopyLists);

int ChangeVariableRangeControlInsideCopyLists (int Pid, int Vid, int Flag);
int ChangeVariableRangeControlFilteredInsideCopyLists (int Pid, char *NameFilter, int Flag);

#endif
