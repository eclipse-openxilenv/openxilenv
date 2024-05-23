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


#ifndef _VIRT_CAN_DEV_H
#define _VIRT_CAN_DEV_H

#include <stdint.h>
#include "tcb.h"
#include "ReadCanCfg.h"

int virtdev_open_can (NEW_CAN_SERVER_CONFIG *csc, int channel);

int virtdev_close_can (NEW_CAN_SERVER_CONFIG *csc, int channel);

int virtdev_ReadNextObjectFromQueue (NEW_CAN_SERVER_CONFIG *csc, int channel,
                                                                 uint32_t *pid, unsigned char* data,
                                                                 unsigned char* pext, unsigned char* psize,
                                                                 uint64_t *pTimeStamp);

int virtdev_read_can (NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos);

int virtdev_write_can (NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos);

// return 0 CAN object was written into the queueand there is place for one ore more additional objects
// return 1 CAN object was written into the queue but there is no place for additional objects
// return -1 CAN object was not written into the queue (full)
int virtdev_WriteObjectToQueue (NEW_CAN_SERVER_CONFIG *csc, int channel,
                                uint32_t id, unsigned char* data,
                                unsigned char ext, unsigned char size);
int virtdev_WriteObjectToQueueInner (int channel,
                                     uint32_t id, unsigned char* data,
                                     unsigned char ext, unsigned char size,
                                     int NotSendToCanControllerIdx);

int virtdev_status_can (NEW_CAN_SERVER_CONFIG *csc, int channel);

void CleanVirtualCanExternalProcess (TASK_CONTROL_BLOCK *par_Tcb);

int VirtualCanInsertCanError(int par_Pid, int par_Channel, int par_Id,
                             int par_Startbit, int par_Bitsize, const char *par_Byteorder,
                             uint32_t par_Cycles, uint64_t par_BitErrValue);
int VirtualCanResetCanError(int par_Pid, int par_Channel);

#endif




