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
#include <stdint.h>
#include "tcb.h"
#include "VirtualNetworkShared.h"

int VirtualNetworkInit(void);
void VirtualNetworkTerminate(void);

void VirtualNetworkCloseAllChannelsForProcess(TASK_CONTROL_BLOCK *par_Tcb);

int VirtualNetworkOpen (int par_Pid, int par_Type, int par_Channel, int par_Size);

//int VirtualNetworkClose (int par_Pid, int par_Type, int par_Channel);
int VirtualNetworkCloseByHandle (int par_Pid, int par_Handle);

int VirtualNetworkRead (int par_Pid, int par_Type, int par_Channel,
                        VIRTUAL_NETWORK_PACKAGE *ret_Data, int par_MaxSize);

int VirtualNetworkReadByHandle (int par_Handle,
                                VIRTUAL_NETWORK_PACKAGE *ret_Data, int par_MaxSize);

int VirtualNetworkWrite (int par_Pid, int par_Type, int par_Channel,
                         VIRTUAL_NETWORK_PACKAGE *par_Data, int par_Size);

int VirtualNetworkWriteByHandle (int par_Handle,
                                 VIRTUAL_NETWORK_PACKAGE *par_Data, int par_Size);

int VirtualNetworkCopyFromPipeToFifos (TASK_CONTROL_BLOCK *Tcb, uint64_t Timestamp, char *SnapShotData, int SnapShotSize);

int VirtualNetworkCopyFromFifosToPipe (TASK_CONTROL_BLOCK *Tcb, char *SnapShotData, int par_MaxSize);

int VirtualNetworkSetInsertErrorHookFunction(int par_Pid, int par_Type, int par_Channel,
                                             int (*par_InsertErrorHookFunction)(int par_Handle,
                                                                                VIRTUAL_NETWORK_PACKAGE *par_Data, VIRTUAL_NETWORK_PACKAGE *ret_Data,
                                                                                int par_MaxSize, int par_Dir));
