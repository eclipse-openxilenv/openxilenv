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


#ifndef EXTPBASEMESSAGES_H
#define EXTPBASEMESSAGES_H

#include "PipeMessagesShared.h"
#include "ExtpProcessAndTaskInfos.h"

 EXTERN_PROCESS_INFOS_STRUCT *XilEnvInternal_GetGlobalProcessInfos (void);
 void XilEnvInternal_LockProcessPtr (EXTERN_PROCESS_INFOS_STRUCT* ProcessInfos);
 void XilEnvInternal_ReleaseProcessPtr (EXTERN_PROCESS_INFOS_STRUCT* ProcessInfos);


 int XilEnvInternal_GetNoGuiFlag (void);
 int XilEnvInternal_GetErr2MsgFlag(void);
 int XilEnvInternal_GetNoXcpFlag(void);


int XilEnvInternal_PipeAddBlackboardVariableAllInfos (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo,
                                                       const char *Name, int Type, const char *Unit, int Dir,
                                                        int ConvType, char *Conversion,
                                                        double Min, double Max,
                                                        int Width, int Prec,
                                                        unsigned int RgbColor,
                                                        int StepType, double Step,
                                                        int ValueIsValid,
                                                        union BB_VARI Value,
                                                        int AddressIsValid,
                                                        unsigned long long Address,
                                                        unsigned long long UniqueId,
                                                        int *ret_Type);

int XilEnvInternal_PipeAttachBlackboardVariable (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskPtr, const char *Name);

int XilEnvInternal_PipeRemoveBlackboardVariable (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int Vid, int Dir, int DataType, unsigned long long Addr, unsigned long long UniqueId);

int XilEnvInternal_PipeWriteToBlackboardVariable (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int Vid, int DataType, union BB_VARI Value);
int XilEnvInternal_PipeReadFromBlackboardVariable (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int Vid, int DataType, union BB_VARI *ret_Value, int *ret_DataType);

int PipeGetLabelnameByAddress (void *Addr, char *RetName, int Maxc);

int  XilEnvInternal_PipeWriteToMessageFile (const char *Text);

int  XilEnvInternal_PipeErrorPopupMessageAndWait (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int Level, const char *Text);

int XilEnvInternal_OpenVirtualNetworkChannel (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int par_Type, int par_Channel);
int XilEnvInternal_CloseVirtualNetworkChannel (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int par_Handle);


int XilEnvInternal_StartCommunicationThread (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos);

EXPORT_OR_IMPORT int  __FUNC_CALL_CONVETION__ CheckIfConnectedTo (EXTERN_PROCESS_TASKS_LIST *ExternProcessTasksList, int ExternProcessTasksListElementCount);

#define XilEnvInternal_GetTaskPtr() XilEnvInternal_GetTaskPtrFileLine(__FILE__, __LINE__)
EXTERN_PROCESS_TASK_INFOS_STRUCT* XilEnvInternal_GetTaskPtrFileLine(const char *FiLe, int Line);

#define XilEnvInternal_LockTaskPtr(TaskInfos) XilEnvInternal_LockTaskPtrFileLine(TaskInfos, __FILE__, __LINE__)
void XilEnvInternal_LockTaskPtrFileLine (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfos, const char *FiLe, int Line);

#define XilEnvInternal_ReleaseTaskPtr(TaskInfos) XilEnvInternal_ReleaseTaskPtrFileLine(TaskInfos, __FILE__, __LINE__)
void XilEnvInternal_ReleaseTaskPtrFileLine (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfos, const char *File, int Line);

EXTERN_PROCESS_TASK_INFOS_STRUCT* XilEnvInternal_GetTaskPtrByNumber (int par_TaskNumber);

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ SetHwndMainWindow (void *Hwnd);
void KillExternProcessHimSelf (void);

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ LoopOutAndSyncWithBarrier (char *par_BarrierName);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ DisconnectFrom (EXTERN_PROCESS_TASK_HANDLE Process, int ImmediatelyFlag);

void *TranslateXCPOverEthernetAddress (size_t par_Address);

#endif
