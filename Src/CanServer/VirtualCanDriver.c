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


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Platform.h"
#include "Config.h"
#include "ConfigurablePrefix.h"
#include "StringMaxChar.h"
#include "BlackboardAccess.h"
#include "ReadCanCfg.h"
#include "ThrowError.h"
#include "MainValues.h"
#include "Scheduler.h"
#include "VirtualNetwork.h"
#include "VirtualCanDriver.h"

#ifndef _WIN32
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#define UNUSED(x) (void)(x)


int virtdev_open_can (NEW_CAN_SERVER_CONFIG *csc, int channel)
{
    if ((channel >= MAX_CAN_CHANNELS) || (channel < 0)) return -1;

    csc->channels[channel].VirtualNetworkId = VirtualNetworkOpen(csc->CanServerPid,
                                                                  (csc->channels[channel].can_fd_enabled) ? VIRTUAL_NETWORK_TYPE_CANFD : VIRTUAL_NETWORK_TYPE_CAN,
                                                                  channel, 64*1024);
    if (csc->channels[channel].VirtualNetworkId < 0) return -1;
    return 0;
}

int virtdev_close_can (NEW_CAN_SERVER_CONFIG *csc, int channel)
{
    if ((channel >= MAX_CAN_CHANNELS) || (channel < 0)) return -1;
    VirtualNetworkCloseByHandle(GET_PID(), csc->channels[channel].VirtualNetworkId);
    csc->channels[channel].VirtualNetworkId = -1;
    return 0;
}

int virtdev_ReadNextObjectFromQueue (NEW_CAN_SERVER_CONFIG *csc, int channel,
                                     uint32_t *pid, unsigned char* data,
                                     unsigned char* pext, unsigned char* psize,
                                     uint64_t *pTimeStamp)
{
    union {
        VIRTUAL_NETWORK_PACKAGE Package;
        char Raw[sizeof(VIRTUAL_NETWORK_PACKAGE)-1 + 64];
    } Package;
    int Ret;

    if ((channel >= MAX_CAN_CHANNELS) || (channel < 0)) return -1;
    Ret = VirtualNetworkReadByHandle(csc->channels[channel].VirtualNetworkId, &Package.Package, sizeof(VIRTUAL_NETWORK_PACKAGE) - 1 + 64);
    if (Ret == 0) {
        *pid = Package.Package.Data.CanFd.Id;
        *pext = Package.Package.Data.CanFd.Flags;
        *psize = (unsigned char)(Package.Package.Size - (sizeof(VIRTUAL_NETWORK_PACKAGE) - 1));
        *pTimeStamp = Package.Package.Timestamp;
        MEMCPY(data, Package.Package.Data.CanFd.Data, *psize);
        return 1;
    }
    return 0;
}

int virtdev_read_can (NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos)
{
    UNUSED(channel);
    if (csc->objects[o_pos].flag) {
        csc->objects[o_pos].flag = 0;
        return 0;
    } else return 2;
}

int virtdev_write_can (NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos)
{
    virtdev_WriteObjectToQueue (csc, channel,
                                csc->objects[o_pos].id, csc->objects[o_pos].DataPtr,
                                (unsigned char)csc->objects[o_pos].ext, (unsigned char)csc->objects[o_pos].size);
    return 0;
}


// return 0 CAN object was written into the queue and there is place for one ore more additional objects
// return 1 CAN object was written into the queue but there is no place for additional objects
// return -1 CAN object was not written into the queue (full)
int virtdev_WriteObjectToQueue (NEW_CAN_SERVER_CONFIG *csc, int channel,
                                uint32_t id, unsigned char* data,
                                unsigned char ext, unsigned char size)
{
    union {
        VIRTUAL_NETWORK_PACKAGE Package;
        char Raw[sizeof(VIRTUAL_NETWORK_PACKAGE)-1 + 64];
    } Package;

    Package.Package.Size = (sizeof(VIRTUAL_NETWORK_PACKAGE) - 1) + size;
    Package.Package.Type = (csc->channels[channel].can_fd_enabled) ? VIRTUAL_NETWORK_TYPE_CANFD : VIRTUAL_NETWORK_TYPE_CAN;
    Package.Package.Channel = channel;
    Package.Package.Data.CanFd.Id = id;
    Package.Package.Data.CanFd.Flags = ext;
    MEMCPY(Package.Package.Data.CanFd.Data, data, size);

    return VirtualNetworkWrite(csc->CanServerPid, (csc->channels[channel].can_fd_enabled) ? VIRTUAL_NETWORK_TYPE_CANFD : VIRTUAL_NETWORK_TYPE_CAN,
                               channel, &Package.Package, Package.Package.Size);
}

int virtdev_status_can (NEW_CAN_SERVER_CONFIG *csc, int channel)
{
    UNUSED(csc);
    UNUSED(channel);
    return 0;
}


void CleanVirtualCanExternalProcess (TASK_CONTROL_BLOCK *par_Tcb)
{
    VirtualNetworkCloseAllChannelsForProcess(par_Tcb);
}


static CAN_BIT_ERROR VirualCanBitError;
static VID CanBitErrorVid;
static uint64_t CycleAtStartPoint;

int CanInsertErrorHookFunction (int par_Handle,
                                VIRTUAL_NETWORK_PACKAGE *par_Data, VIRTUAL_NETWORK_PACKAGE *ret_Data,
                                int par_MaxSize, int par_Dir)
{
    UNUSED(par_Handle);
    UNUSED(par_Dir);
    int Ret = 0;
    if ((par_Data->Type == VIRTUAL_NETWORK_TYPE_CANFD) ||
        (par_Data->Type == VIRTUAL_NETWORK_TYPE_CAN)) {
        // only CAN will handle here
        uint64_t Cycle = GetCycleCounter64();
        if (CycleAtStartPoint + VirualCanBitError.Counter > Cycle) {
            if (VirualCanBitError.Channel == par_Data->Channel) {
                if (VirualCanBitError.Id == (int32_t)par_Data->Data.CanFd.Id) {
                    switch (VirualCanBitError.Command) {
                    case OVERWRITE_DATA_BYTES:
                        MEMCPY(ret_Data, par_Data, par_Data->Size);
                        int Size = par_Data->Size - (sizeof(VIRTUAL_NETWORK_PACKAGE) - 1);
                        if (Size > CAN_BIT_ERROR_MAX_SIZE) Size = CAN_BIT_ERROR_MAX_SIZE;
                        if (VirualCanBitError.ByteOrder) {
                            for (int x = 0; x < Size; x++) {
                                int xx = (Size - 1) - x;
                                uint8_t Data = par_Data->Data.CanFd.Data[x];
                                Data &= VirualCanBitError.AndMask[xx];
                                Data |= VirualCanBitError.OrMask[xx];
                                ret_Data->Data.CanFd.Data[x] = Data;
                            }
                        } else {
                            for (int x = 0; x < Size; x++) {
                                uint8_t Data = par_Data->Data.CanFd.Data[x];
                                uint8_t AndMask = VirualCanBitError.AndMask[x];
                                uint8_t OrMask = VirualCanBitError.OrMask[x];
                                Data &= AndMask;
                                Data |= OrMask;
                                ret_Data->Data.CanFd.Data[x] = Data;
                            }
                        }
                        Ret = 1;    // Data has changed
                        break;
                    case CHANGE_DATA_LENGTH:
                        if (VirualCanBitError.Size < par_MaxSize) {
                            MEMCPY(ret_Data, par_Data, par_Data->Size);
                            ret_Data->Size = (sizeof(VIRTUAL_NETWORK_PACKAGE) - 1) + VirualCanBitError.Size;
                            Ret = 1;    // Data has changed
                        }
                        break;
                    case SUSPEND_TRANSMITION:
                        Ret = 2;    // This Object should not be transmitted
                        break;
                    default:
                        Ret = 0; // Data has not changed
                        break;
                    }
                } else {
                    Ret = 0; // Data has not changed
                }
            } else {
                Ret = 0; // Data has not changed
            }
        } else {
            write_bbvari_udword(CanBitErrorVid, 0);
            VirualCanBitError.Command = 0; // Timeout reset the error condition
        }
    } else {
        Ret = 0; // Data has not changed
    }
    return Ret;
}

static void BuildMasks(uint8_t *par_AndMask, uint8_t *par_OrMask,
                       int par_Startbit, int par_Bitsize, int par_Size, uint64_t par_BitErrValue)
{
    int x, y;
    int Endbit = par_Startbit + par_Bitsize;
    for (x = 0; x < par_Size; x++) {
        par_AndMask[x] = 0xFF;
        par_OrMask[x] = 0x00;
    }
    // look for each bit inside the mask;
    for (x = 0; x < par_Size; x++) {
        for (y = 0; y < 8; y++) {
            int BitPos = (x << 3) + y;
            if ((BitPos >= par_Startbit) && (BitPos < Endbit)) {
                uint8_t Bit = (par_BitErrValue >> (BitPos - par_Startbit)) & 0x1;
                par_OrMask[x] |= Bit << y;
                par_AndMask[x] &= ~(1 << y);
            }
        }
    }
}

int VirtualCanInsertCanError(int par_Pid, int par_Channel, int par_Id,
                             int par_Startbit, int par_Bitsize, const char *par_Byteorder,
                             uint32_t par_Cycles, uint64_t par_BitErrValue)
{
    VirualCanBitError.Id = par_Id;
    VirualCanBitError.Counter = par_Cycles;
    VirualCanBitError.Channel = par_Channel;
    if (par_Startbit == -1) {    // negative Startbit -> The byte length of the CAN object should be changed
        VirualCanBitError.Command = CHANGE_DATA_LENGTH;
        if (par_Bitsize < 0) VirualCanBitError.Size = 0;
        else if (par_Bitsize > 64) VirualCanBitError.Size = 64;
        else VirualCanBitError.Size = par_Bitsize;
        VirualCanBitError.SizeSave = -1;
    } else if (par_Startbit == -2) {    // negative Startbit -> The number of transmit cycles should be changed
        VirualCanBitError.Command = SUSPEND_TRANSMITION;
    } else  {
        VirualCanBitError.Command = OVERWRITE_DATA_BYTES;
        if (!strcmpi ("msb_first", par_Byteorder)) {
            VirualCanBitError.ByteOrder = 1;
        } else {
            VirualCanBitError.ByteOrder = 0;
        }
        BuildMasks(VirualCanBitError.AndMask, VirualCanBitError.OrMask, par_Startbit, par_Bitsize, CAN_BIT_ERROR_MAX_SIZE, par_BitErrValue);
    }
    if(CanBitErrorVid <= 0) {
        char Name[BBVARI_NAME_SIZE];
        CanBitErrorVid = add_bbvari(ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES, ".CANBitError.Counter", Name, sizeof(Name)),
                                    BB_UDWORD, "");
    }
    CycleAtStartPoint = GetCycleCounter64();

    int Ret = VirtualNetworkSetInsertErrorHookFunction(par_Pid, VIRTUAL_NETWORK_TYPE_CAN, par_Channel,
                                                       CanInsertErrorHookFunction);
    Ret |= VirtualNetworkSetInsertErrorHookFunction(par_Pid, VIRTUAL_NETWORK_TYPE_CANFD, par_Channel,
                                                    CanInsertErrorHookFunction);
    return Ret;
}

int VirtualCanResetCanError(int par_Pid, int par_Channel)
{
    int Ret = VirtualNetworkSetInsertErrorHookFunction(par_Pid, VIRTUAL_NETWORK_TYPE_CAN, par_Channel,
                                                       NULL);
    Ret |= VirtualNetworkSetInsertErrorHookFunction(par_Pid, VIRTUAL_NETWORK_TYPE_CANFD, par_Channel,
                                                    NULL);
    VirualCanBitError.Command = 0;
    VirualCanBitError.Id = -1;
    VirualCanBitError.Counter = 0;
    VirualCanBitError.Channel = 0;
    for (int x = 0; x < CAN_BIT_ERROR_MAX_SIZE; x++) {
        VirualCanBitError.AndMask[x] = 0;
        VirualCanBitError.OrMask[x] = 0;
    }
    VirualCanBitError.Size = -1;
    VirualCanBitError.SizeSave = -1;
    return Ret;
}
