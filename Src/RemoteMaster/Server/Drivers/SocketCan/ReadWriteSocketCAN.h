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

int SocketCAN_open_can(NEW_CAN_SERVER_CONFIG *csc, int channel);

int SocketCAN_close_can(NEW_CAN_SERVER_CONFIG *csc, int channel);

int SocketCAN_ReadNextObjectFromQueue(NEW_CAN_SERVER_CONFIG *csc, int channel,
    uint32_t *pid, unsigned char* data,
    unsigned char* pext, unsigned char* psize,
    uint64_t *pTimeStamp);

int SocketCAN_read_can(NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos);


int SocketCAN_write_can(NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos);

// return 0 CAN object was written into the queueand there is place for one ore more additional objects
// return 1 CAN object was written into the queue but there is no place for additional objects
// return -1 CAN object was not written into the queue (full)
int SocketCAN_WriteObjectToQueue(NEW_CAN_SERVER_CONFIG *csc, int channel,
    uint32_t id, unsigned char* data,
    unsigned char ext, unsigned char size);

int SocketCAN_status_can(NEW_CAN_SERVER_CONFIG *csc, int channel);

int SocketCAN_info_can(struct NEW_CAN_SERVER_CONFIG_STRUCT *csc, int channel, int Info, int MaxReturnSize, void *Return);


