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


#pragma once
#include "my_stdint.h"

#include "VirtualNetworkShared.h"

int XilEnvInternal_VirtualNetworkInit(EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo);

int XilEnvInternal_VirtualNetworkOpen (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Type, int par_Channel, int par_Size);

int XilEnvInternal_VirtualNetworkCloseByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle);

int XilEnvInternal_VirtualNetworkInReadByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle,
                                                  VIRTUAL_NETWORK_PACKAGE *ret_Data, int par_MaxSize);

int XilEnvInternal_VirtualNetworkOutReadByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle,
                                                   VIRTUAL_NETWORK_PACKAGE *ret_Data, int par_MaxSize);

int XilEnvInternal_VirtualNetworkInWrite (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Type, int par_Channel,
                                           VIRTUAL_NETWORK_PACKAGE *par_Data, int par_Size);

int XilEnvInternal_VirtualNetworkOutWriteByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle,
                                                    VIRTUAL_NETWORK_PACKAGE *par_Data, int par_Size);

int XilEnvInternal_VirtualNetworkGetTypeByChannel (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Channel, int *ret_Handle);

int XilEnvInternal_VirtualNetworkGetRxFillByChannel (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Channel);


// Message buffers
int XilEnvInternal_VirtualNetworkConfigBufferByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle,
                                                        int par_Number, int par_IdSlot, int par_Dir, int par_Size, int par_Flags);

int XilEnvInternal_VirtualNetworkReadBufferByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle, int par_Number,
                                                      unsigned char *ret_Data, int *ret_Id,  unsigned char *pext, int *ret_Size, int par_MaxSize);

int XilEnvInternal_VirtualNetworkPeekBufferByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle, int par_Number,
                                                      unsigned char *ret_Data, int *ret_IdSlot,  unsigned char *ret_Ext, int *ret_Size, int par_MaxSize);

VIRTUAL_NETWORK_BUFFER *XilEnvInternal_VirtualNetworkGetBufferByHandleAndLock (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle, int par_Number);
void XilEnvInternal_VirtualNetworkUnLockBuffer(EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, VIRTUAL_NETWORK_BUFFER *par_Buffer);

int XilEnvInternal_VirtualNetworkCopyFromPipeToFifos (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, char *SnapShotData, int SnapShotSize);

int XilEnvInternal_VirtualNetworkCopyFromFifosToPipe (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, char *SnapShotData, int par_MaxSize);
