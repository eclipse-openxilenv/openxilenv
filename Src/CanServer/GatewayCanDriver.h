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


#ifndef GATEWAYCANDRIVER_H
#define GATEWAYCANDRIVER_H

#include <stdint.h>

#include "ReadCanCfg.h"

int CanGatewayDevice_open_can (NEW_CAN_SERVER_CONFIG *csc, int channel);

int CanGatewayDevice_close_can (NEW_CAN_SERVER_CONFIG *csc, int channel);

int CanGatewayDevice_ReadNextObjectFromQueue (NEW_CAN_SERVER_CONFIG *csc, int channel,
                                              uint32_t *pid, unsigned char* data,
                                              unsigned char* pext, unsigned char* psize,
                                              uint64_t *pTimeStamp);

int CanGatewayDevice_read_can (NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos);

int CanGatewayDevice_write_can (NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos);

int CanGatewayDevice_WriteObjectToQueue (NEW_CAN_SERVER_CONFIG *csc, int channel,
                                         uint32_t id, unsigned char* data,
                                         unsigned char ext, unsigned char size);

int CanGatewayDevice_status_can (NEW_CAN_SERVER_CONFIG *csc, int channel);

int InitCanGatewayDevice(int par_Flags);

int GetCanGatewayDeviceCount(void);
int GetCanGatewayDeviceInfos(int par_Channel, char *ret_Name, int par_MaxChars, uint32_t *ret_DriverVersion, uint32_t *ret_DllVersion, uint32_t *ret_InterfaceVersion);
int GetCanGatewayDeviceFdSupport(int par_Channel);

#endif




