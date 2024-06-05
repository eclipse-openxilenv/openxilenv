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


#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "Platform.h"
#include "Config.h"
#include "StringMaxChar.h"
#include "ExtpBaseMessages.h"
#include "XilEnvExtProcCan.h"
#include "ReadCanCfg.h"
#include "XilEnvExtProc.h"

#include "ExtpVirtualNetwork.h"


// This will open a CAN channel all CAN message buffer will be deleted
// channel = 0...15
// fd_enable = 1 -> CAN-FD channel, 0 -> CAN
// return value:
//    0 -> all OK
//    otherwise
//   -1 -> cannot find the virtual CAN controller interface
//   -2 -> invalid parameter
//   -3 -> CAN channel already open
//   -4 -> XilEnv and library have not the correct version
//   -5 -> Out off memory
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_open_virt_can_fd (int channel, int fd_enable)
{
    int Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    
    // channel 0...15
    if ((channel >= MAX_CAN_CHANNELS) || (channel < 0)) return -2;

    TaskPtr = XilEnvInternal_GetTaskPtr();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }
    Ret = (XilEnvInternal_VirtualNetworkOpen(TaskPtr, (fd_enable) ? VIRTUAL_NETWORK_TYPE_CANFD : VIRTUAL_NETWORK_TYPE_CAN,  channel, 64*1024) < 0) ? -1 : 0;   // 64KByte
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);
    return Ret;
}

// This will open a CAN channel all CAN message buffer will be deleted
// channel = 0...15
// return value:
//    0 -> all OK
//    otherwise
//   -1 -> cannot find the virtual CAN controller interface
//   -2 -> invalid parameter
//   -3 -> CAN channel already open
//   -4 -> XilEnv and library have not the correct version
//   -5 -> Out off memory
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_open_virt_can (int channel)
{
    return sc_open_virt_can_fd(channel, 0);
}

// This will write a CAN message into the CAN transmitting fifo of the virtual CAN controller
// channel = 0...15
// id = CAN-ID
// ext = 0 standart ID's
//     = 1 extendet ID's
// size = 1...8
// data = Daten der zu sendenden CAN-Message
// return value:
//    0 -> Data was written and there is space for an additional message iside the fifo
//    1 -> Data was written and there is NO space for an additional message iside the fifo
//   -1 -> Data was lost
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_fifo_write_virt_can (int channel, WSC_TYPE_UINT32 id, unsigned char ext,
                                                                     unsigned char size, unsigned char *data)
{
    int Handle;
    VIRTUAL_NETWORK_PACKAGE *Package;
    int PackageSize;
    int Type;
    int Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    
    // channel 0...15
    if ((channel >= MAX_CAN_CHANNELS) || (channel < 0)) return -2;

    TaskPtr = XilEnvInternal_GetTaskPtr();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Type = XilEnvInternal_VirtualNetworkGetTypeByChannel(TaskPtr, channel, &Handle);
    switch (Type) {
    case VIRTUAL_NETWORK_TYPE_CAN:
    case VIRTUAL_NETWORK_TYPE_CANFD:
        PackageSize = (sizeof(VIRTUAL_NETWORK_PACKAGE) - 1) + size;
        Package = (VIRTUAL_NETWORK_PACKAGE*)alloca(PackageSize);
        Package->Channel = channel;
        Package->Data.CanFd.Id = id;
        Package->Data.CanFd.Flags = ext;
        Package->Size = PackageSize;
        Package->Timestamp = 0;
        Package->Type = Type;
        Package->Internal = 0;
        MEMCPY(Package->Data.CanFd.Data, data, size);
        Ret = XilEnvInternal_VirtualNetworkOutWriteByHandle(TaskPtr, Handle, Package, PackageSize);
        break;
    default:
        Ret = -2;
        break;
    }
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);
    return Ret;
}

// This will read a CAN message from the receiving fifo of the virtual CAN controller
// channel = 0...15
// pid = returns the CAN Id
// pext = returns the Id type: =0 standart Id's, =1 extendet Id's
// psize = returns the size of the data 1...8
// data = returns the data of the received CAN message
// return value:
//    0 -> There are no CAN messages inside the receiving fifo of the virtual CAN controller
//    1 -> There was received one CAN message from the fifo
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_fifo_read_virt_can (int channel, WSC_TYPE_UINT32 *pid, unsigned char *pext,
                                                                    unsigned char *psize, unsigned char *data)
{
    int Handle;
    VIRTUAL_NETWORK_PACKAGE *Package;
    int PackageSize;
    int Type;
    int Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    
    // channel 0...15
    if ((channel >= MAX_CAN_CHANNELS) || (channel < 0)) return -2;

    TaskPtr = XilEnvInternal_GetTaskPtr();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Type = XilEnvInternal_VirtualNetworkGetTypeByChannel(TaskPtr, channel, &Handle);
    switch (Type) {
    case VIRTUAL_NETWORK_TYPE_CAN:
    case VIRTUAL_NETWORK_TYPE_CANFD:
        PackageSize = (sizeof(VIRTUAL_NETWORK_PACKAGE) - 1) + 64;
        Package = (VIRTUAL_NETWORK_PACKAGE*)alloca(PackageSize);
        Ret = XilEnvInternal_VirtualNetworkInReadByHandle(TaskPtr, Handle, Package, PackageSize);
        if (Ret == 1) {
            if (pid != NULL) *pid = Package->Data.CanFd.Id;
            if (psize != NULL) *psize = Package->Size - (sizeof(VIRTUAL_NETWORK_PACKAGE) - 1);
            if (pext != NULL) *pext = Package->Data.CanFd.Flags;
            MEMCPY(data, Package->Data.CanFd.Data, Package->Size - (sizeof(VIRTUAL_NETWORK_PACKAGE) - 1));
        }
        break;
    default:
        Ret = -1;
        break;
    }
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);
    return Ret;
}

// This will return the filling level of the receiving fifo of the virtual CAN controller
// channel = 0...15
// return value:
//    0...n -> number of Objects inside the fifo
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_get_fifo_fill_level (int channel)
{
    int Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    
    // channel 0...15
    if ((channel >= MAX_CAN_CHANNELS) || (channel < 0)) return -2;

    TaskPtr = XilEnvInternal_GetTaskPtr();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Ret = XilEnvInternal_VirtualNetworkGetRxFillByChannel(TaskPtr, channel);
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);
    return Ret;
}

// This will close the CAN channel afterwards no receiving or transmitting will be possible
// channel = 0...15
// return value:
//    0 -> CAN channel was closed successful
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_close_virt_can (int channel)
{
    int Handle;
    int Type;
    int Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    
    // channel 0...15
    if ((channel >= MAX_CAN_CHANNELS) || (channel < 0)) return -2;

    TaskPtr = XilEnvInternal_GetTaskPtr();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Type = XilEnvInternal_VirtualNetworkGetTypeByChannel(TaskPtr, channel, &Handle);
    switch (Type) {
    case VIRTUAL_NETWORK_TYPE_CAN:
    case VIRTUAL_NETWORK_TYPE_CANFD:
        Ret = XilEnvInternal_VirtualNetworkCloseByHandle(TaskPtr, Handle);
        break;
    default:
        Ret = -1;
        break;
    }
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);
    return Ret;
}

// This will configure one of the CAN message object buffers
// channel = 0...15
// buffer_idx = 0...n
// id = CAN-Id
// ext = 0 standart Id's
//     = 1 extendet Id's
// size = 1...8
// dir = this will define if it receiving buffer (dir=0) or a transmitting buffer (dir=1)
// return value:
//    0 -> CAN message object buffer successful configured
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_cfg_virt_can_msg_buff (int channel, int buffer_idx, WSC_TYPE_UINT32 id,
                                                                       unsigned char ext, unsigned char size, unsigned char dir)
{
    int Handle;
    int Type;
    int Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    
    // channel 0...15
    if ((channel >= MAX_CAN_CHANNELS) || (channel < 0)) return -2;

    TaskPtr = XilEnvInternal_GetTaskPtr();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Type = XilEnvInternal_VirtualNetworkGetTypeByChannel(TaskPtr, channel, &Handle);
    switch (Type) {
    case VIRTUAL_NETWORK_TYPE_CAN:
    case VIRTUAL_NETWORK_TYPE_CANFD:
        Ret = XilEnvInternal_VirtualNetworkConfigBufferByHandle(TaskPtr, Handle, buffer_idx, id, dir, size, ext);
        break;
    default:
        Ret = -1;
        break;
    }
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);
    return Ret;
}

// This will read a CAN message from the CAN message object-buffer of the virtual CAN controller
// channel = 0...15
// buffer_idx = 0...n
// return value:
//    0 -> The data are old
//    1 -> The data are new
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_read_virt_can_msg_buff (int channel, int buffer_idx, unsigned char *pext,
                                                                        unsigned char *psize, unsigned char *data)
{
    int Handle;
    int Type;
    int SlotId;
    int Size;
    unsigned char Flags;
    int Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    
    // channel 0...15
    if ((channel >= MAX_CAN_CHANNELS) || (channel < 0)) return -2;

    TaskPtr = XilEnvInternal_GetTaskPtr();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Type = XilEnvInternal_VirtualNetworkGetTypeByChannel(TaskPtr, channel, &Handle);
    switch (Type) {
    case VIRTUAL_NETWORK_TYPE_CAN:
    case VIRTUAL_NETWORK_TYPE_CANFD:
        Ret = XilEnvInternal_VirtualNetworkReadBufferByHandle(TaskPtr, Handle, buffer_idx, data, &SlotId, &Flags, &Size, 64);
        if (psize != NULL) *psize = Size;
        if (pext != NULL) *pext = Flags;
        break;
    default:
        Ret = -1;
        break;
    }
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);
    return Ret;
}

// This will read a CAN message from the CAN message object-buffer of the virtual CAN controller
// without marked them a readed
// channel = 0...15
// buffer_idx = 0...n
// pext = returns thes ID type: =0 standart ID's, =1 extendet ID's
//        this can be NULL if not needed
// psize = returns the size 1...8
//        this can be NULL if not needed
// data = return buffer for the received CAN data bytes
// return value:
//    0 -> The data are old
//    1 -> The data are new
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_peek_virt_can_msg_buff (int channel, int buffer_idx, unsigned char *pext,
                                                                        unsigned char *psize, unsigned char *data)
{
    int Handle;
    int Type;
    int SlotId;
    int Size;
    unsigned char Flags;
    int Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    
    // channel 0...15
    if ((channel >= MAX_CAN_CHANNELS) || (channel < 0)) return -2;

    TaskPtr = XilEnvInternal_GetTaskPtr();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Type = XilEnvInternal_VirtualNetworkGetTypeByChannel(TaskPtr, channel, &Handle);
    switch (Type) {
    case VIRTUAL_NETWORK_TYPE_CAN:
    case VIRTUAL_NETWORK_TYPE_CANFD:
        Ret = XilEnvInternal_VirtualNetworkPeekBufferByHandle(TaskPtr, Handle, buffer_idx, data, &SlotId, &Flags, &Size, 64);
        if (psize != NULL) *psize = Size;
        if (pext != NULL) *pext = Flags;
        break;
    default:
        Ret = -1;
        break;
    }
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);
    return Ret;
}

// This will write a CAN message into a CAN message object buffer of the virtual CAN controller
// channel = 0...15
// buffer_idx = 0...n
// data = Data that should be written
// Return value:
//    0 -> Data was successful written into the buffer
//   -2 -> invalid parameter
//   -3 -> CAN channel was not opened
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ sc_write_virt_can_msg_buff (int channel, int buffer_idx, unsigned char *data)
{
    int Handle;
    VIRTUAL_NETWORK_BUFFER *Buffer;
    VIRTUAL_NETWORK_PACKAGE *Package;
    int PackageSize;
    int Type;
    int Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    
    // channel 0...15
    if ((channel >= MAX_CAN_CHANNELS) || (channel < 0)) return -2;

    TaskPtr = XilEnvInternal_GetTaskPtr();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Type = XilEnvInternal_VirtualNetworkGetTypeByChannel(TaskPtr, channel, &Handle);
    switch (Type) {
    case VIRTUAL_NETWORK_TYPE_CAN:
    case VIRTUAL_NETWORK_TYPE_CANFD:
        Buffer = XilEnvInternal_VirtualNetworkGetBufferByHandleAndLock(TaskPtr, Handle, buffer_idx);
        if (Buffer != NULL) {
            MEMCPY(Buffer->Data, data, Buffer->Size);

            PackageSize = (sizeof(VIRTUAL_NETWORK_PACKAGE) - 1) + Buffer->Size;
            Package = (VIRTUAL_NETWORK_PACKAGE*)alloca(PackageSize);
            Package->Channel = channel;
            Package->Data.CanFd.Id = Buffer->IdSlot;
            Package->Data.CanFd.Flags = Buffer->Flags;
            Package->Size = PackageSize;
            Package->Timestamp = 0;
            Package->Type = Type;
            Package->Internal = 0;
            MEMCPY(Package->Data.CanFd.Data, data, Buffer->Size);

            XilEnvInternal_VirtualNetworkUnLockBuffer(TaskPtr, Buffer);

            Ret = XilEnvInternal_VirtualNetworkOutWriteByHandle(TaskPtr, Handle, Package, PackageSize);
        } else {
            Ret = -1;
        }
        break;
    default:
        Ret = -1;
        break;
    }
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);
    return Ret;
}


