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


#include "Platform.h"
#include "StringMaxChar.h"
#include "ExtpMemoryAllocation.h"
#include "ExtpBaseMessages.h"
#include "ExtpVirtualNetwork.h"

int XilEnvInternal_VirtualNetworkInit(EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo)
{
    InitializeCriticalSection (&TaskInfo->FiFosCriticalSection);
    return 0;
}

static void XilEnvInternal_VirtualNetworkOpenFreeMemory(EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Pos)
{
    if (TaskInfo->Fifos != NULL) {
        TaskInfo->Fifos[par_Pos].FiFoHandle = -1;
        if (TaskInfo->Fifos[par_Pos].InData != NULL) XilEnvInternal_free(TaskInfo->Fifos[par_Pos].InData);
        if (TaskInfo->Fifos[par_Pos].OutData != NULL) XilEnvInternal_free(TaskInfo->Fifos[par_Pos].InData);
    }
}

int XilEnvInternal_VirtualNetworkOpen (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Type, int par_Channel, int par_Size)
{
    int32_t x;
    int Ret;
    int AlignedSize;

    AlignedSize = ALIGN(par_Size);

    EnterCriticalSection (&TaskInfo->FiFosCriticalSection);

    if (TaskInfo->VirtualNetworkHandleCount >= 16) {
        LeaveCriticalSection (&TaskInfo->FiFosCriticalSection);
        return -1;
    }

    // first chaeck if already open
    for (x = 0; x < TaskInfo->FifoCounter; x++) {
        if ((TaskInfo->Fifos[x].FiFoHandle >= 0) &&
            (TaskInfo->Fifos[x].Type == par_Type) &&
            (TaskInfo->Fifos[x].Channel == par_Channel)) {  
             LeaveCriticalSection (&TaskInfo->FiFosCriticalSection);
             return -3;  // Channel already open  
        }
    }

    // seach a unused fifo
    x = 0;
    for(;;) {
        for ( ; x < TaskInfo->FifoCounter; x++) {
            if (TaskInfo->Fifos[x].FiFoHandle < 0) {  // free FiFo
                TaskInfo->Fifos[x].FiFoHandle = x;
                break;  // for(;;)
            }
        }
        if (x == TaskInfo->FifoCounter) {
            int i;
            TaskInfo->FifoCounter += 1;  // 1 new fifos
            TaskInfo->Fifos = (VIRTUAL_NETWORK_FIFO*)XilEnvInternal_realloc(TaskInfo->Fifos, sizeof(TaskInfo->Fifos[0]) * TaskInfo->FifoCounter);
            if (TaskInfo->Fifos == NULL) {
                LeaveCriticalSection (&TaskInfo->FiFosCriticalSection);
                ThrowError(1, "cannot create virtual network fifo (out of memory)");
                return -5 ; // Out of memory
            }
            memset(TaskInfo->Fifos + (TaskInfo->FifoCounter - 1), 0, sizeof(TaskInfo->Fifos[0]) * 1);
            for (i = (TaskInfo->FifoCounter - 1); i < TaskInfo->FifoCounter; i++) {
                TaskInfo->Fifos[i].FiFoHandle = -1;
            }
            continue;
        } else {
            break;
        }
    }
    LeaveCriticalSection (&TaskInfo->FiFosCriticalSection);

    TaskInfo->Fifos[x].Size = AlignedSize;
    TaskInfo->Fifos[x].Type = par_Type;
    TaskInfo->Fifos[x].Channel = par_Channel;
    TaskInfo->Fifos[x].InMessagesCounter = 0;
    TaskInfo->Fifos[x].InLostMessagesCounter = 0;
    TaskInfo->Fifos[x].OutMessagesCounter = 0;
    TaskInfo->Fifos[x].OutLostMessagesCounter = 0;

    TaskInfo->Fifos[x].RdBufferCount = 0;
    TaskInfo->Fifos[x].WrBufferCount = 0;
    memset (TaskInfo->Fifos[x].RdPositions, 0, sizeof(TaskInfo->Fifos[x].RdPositions));
    memset (TaskInfo->Fifos[x].WrPositions, 0, sizeof(TaskInfo->Fifos[x].WrPositions));

    if (AlignedSize > 0) {
        TaskInfo->Fifos[x].InData = XilEnvInternal_calloc((size_t)AlignedSize, 1);
        TaskInfo->Fifos[x].InWr = TaskInfo->Fifos[x].InData;
        TaskInfo->Fifos[x].InRd = TaskInfo->Fifos[x].InData;
        TaskInfo->Fifos[x].OutData = XilEnvInternal_calloc((size_t)AlignedSize, 1);
        TaskInfo->Fifos[x].OutWr = TaskInfo->Fifos[x].OutData;
        TaskInfo->Fifos[x].OutRd = TaskInfo->Fifos[x].OutData;
        if ((TaskInfo->Fifos[x].InData == NULL) || (TaskInfo->Fifos[x].OutData == NULL)) {
            XilEnvInternal_VirtualNetworkOpenFreeMemory(TaskInfo, x);
            ThrowError(1, "cannot create virtual network fifo with size %i (out of memory)", AlignedSize);
            return -5 ; // Out of memory
        }
    } else {
        TaskInfo->Fifos[x].InData = NULL;
        TaskInfo->Fifos[x].InWr = NULL;
        TaskInfo->Fifos[x].InRd = NULL;
        TaskInfo->Fifos[x].OutData = NULL;
        TaskInfo->Fifos[x].OutWr = NULL;
        TaskInfo->Fifos[x].OutRd = NULL;
    }

    // now send the open request to XilEnv
    TaskInfo->Fifos[x].MainHandle = XilEnvInternal_OpenVirtualNetworkChannel (TaskInfo, par_Type, par_Channel);
    if (TaskInfo->Fifos[x].MainHandle < 0) {
         XilEnvInternal_VirtualNetworkOpenFreeMemory(TaskInfo, x);
         ThrowError(1, "cannot create virtual network fifo");
         return -5 ; // Out of memory
    }

    EnterCriticalSection (&TaskInfo->FiFosCriticalSection);
    TaskInfo->Fifos[x].IsOnline = 1;
    TaskInfo->VirtualNetworkHandles[TaskInfo->VirtualNetworkHandleCount] = x;
    TaskInfo->VirtualNetworkHandleCount++;
    Ret = TaskInfo->Fifos[x].FiFoHandle;
    LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);
    return Ret;
}

int XilEnvInternal_VirtualNetworkCloseByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle)
{
    int x, y;
    int MainHandle;
    int Ret = -1;

    EnterCriticalSection(&TaskInfo->FiFosCriticalSection);
    for (x = 0; x < TaskInfo->FifoCounter; x++) {
        if (TaskInfo->Fifos[x].IsOnline &&
            (TaskInfo->Fifos[x].FiFoHandle == par_Handle)) {
            MainHandle = TaskInfo->Fifos[x].MainHandle;
            // remove the fifo
            TaskInfo->Fifos[x].IsOnline = 0;
            TaskInfo->Fifos[x].FiFoHandle = -1;
            if (TaskInfo->Fifos[x].InData != NULL) XilEnvInternal_free(TaskInfo->Fifos[x].InData);
            TaskInfo->Fifos[x].InData = NULL;
            if (TaskInfo->Fifos[x].OutData != NULL) XilEnvInternal_free(TaskInfo->Fifos[x].OutData);
            TaskInfo->Fifos[x].OutData = NULL;
            for (y = 0; y < TaskInfo->Fifos[x].RdBufferCount; y++) {
                int i = TaskInfo->Fifos[x].RdPositions[y];
                if (TaskInfo->Fifos[x].RdMessageBuffers[i] != NULL) XilEnvInternal_free(TaskInfo->Fifos[x].RdMessageBuffers[i]);
                TaskInfo->Fifos[x].RdMessageBuffers[i] = NULL;
            }
            for (y = 0; y < TaskInfo->Fifos[x].WrBufferCount; y++) {
                int i = TaskInfo->Fifos[x].WrPositions[y];
                if (TaskInfo->Fifos[x].WrMessageBuffers[i] != NULL) XilEnvInternal_free(TaskInfo->Fifos[x].WrMessageBuffers[i]);
                TaskInfo->Fifos[x].WrMessageBuffers[i] = NULL;
            }
            // and also remove the handle
            for (y = 0; y < TaskInfo->VirtualNetworkHandleCount; y++) {
                if (TaskInfo->VirtualNetworkHandles[y] == par_Handle) {
                    for (y = y + 1; y  < TaskInfo->VirtualNetworkHandleCount; y++) {
                        TaskInfo->VirtualNetworkHandles[y-1] = TaskInfo->VirtualNetworkHandles[y];
                    }
                    TaskInfo->VirtualNetworkHandleCount--;
                    break;
                }
            }
            Ret = 0;
            break;  // for(;;)
        }
    }
    LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);
    // now send the close request to XilEnv
    if (Ret == 0) {
        if (x >= 0) {
            if (XilEnvInternal_CloseVirtualNetworkChannel (TaskInfo, MainHandle) != 0) {
                return -5; // ????
            }
        }
    }

    return Ret;
}


int XilEnvInternal_VirtualNetworkInReadByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle,
                                                  VIRTUAL_NETWORK_PACKAGE *ret_Data, int par_MaxSize)
{
    int Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    int32_t Size;
    int32_t AlignedSize;

    EnterCriticalSection(&TaskInfo->FiFosCriticalSection);
    if ((par_Handle >= 0) && (par_Handle < TaskInfo->FifoCounter)) {
        if (TaskInfo->Fifos[par_Handle].FiFoHandle == par_Handle) {
            if (TaskInfo->Fifos[par_Handle].IsOnline) {
                char *Dest;
                char *Src;
                Src = (char*)TaskInfo->Fifos[par_Handle].InRd;
                Size = *(uint32_t*)Src;
                if (Size) {    // new data inside fifo?
                    int parta, partb;
                    if (Size <= par_MaxSize) {
                        AlignedSize = ALIGN(Size);
                        Dest = (char*)ret_Data;
                        parta = (int)(TaskInfo->Fifos[par_Handle].Size - (Src - (char*)TaskInfo->Fifos[par_Handle].InData));
                        if (parta >= Size) {
                            MEMCPY(Dest, Src, (size_t)Size);
                            Src += AlignedSize;
                        }  else {
                            partb = Size - parta;
                            MEMCPY(Dest, Src, (size_t)parta);
                            MEMCPY(Dest + parta, TaskInfo->Fifos[par_Handle].InData, (size_t)partb);
                            Src = (char*)TaskInfo->Fifos[par_Handle].InData + (AlignedSize - parta);
                        }
                        if ((Src - (char*)TaskInfo->Fifos[par_Handle].InData) >= TaskInfo->Fifos[par_Handle].Size) {
                            Src = (char*)TaskInfo->Fifos[par_Handle].InData;
                        }
                        TaskInfo->Fifos[par_Handle].InRd = Src;
                        TaskInfo->Fifos[par_Handle].InMessagesCounter--;
                        Ret = 1;
                    } else Ret = VIRTUAL_NETWORK_ERR_MESSAGE_TO_BIG;
                } else Ret = 0;
            } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
        } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
    } else Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);

    return Ret;
}

static int XilEnvInternal_VirtualNetworkOutReadByHandleNoLock (int par_Handle,
                                                                EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo,
                                                                VIRTUAL_NETWORK_PACKAGE *ret_Data, int par_MaxSize)
{
    int Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    int32_t Size;
    int32_t AlignedSize;
    if ((par_Handle >= 0) && (par_Handle < TaskInfo->FifoCounter)) {
        if (TaskInfo->Fifos[par_Handle].FiFoHandle == par_Handle) {
            if (TaskInfo->Fifos[par_Handle].IsOnline) {
                char *Dest;
                char *Src;
                Src = (char*)TaskInfo->Fifos[par_Handle].OutRd;
                Size = *(uint32_t*)Src;
                if (Size) {    // new data inside fifo?
                    int parta, partb;
                    if (Size <= par_MaxSize) {
                        AlignedSize = ALIGN(Size);
                        Dest = (char*)ret_Data;
                        parta = (int)(TaskInfo->Fifos[par_Handle].Size - (Src - (char*)TaskInfo->Fifos[par_Handle].OutData));
                        if (parta >= Size) {
                            MEMCPY(Dest, Src, (size_t)Size);
                            Src += AlignedSize;
                        }  else {
                            partb = Size - parta;
                            MEMCPY(Dest, Src, (size_t)parta);
                            MEMCPY(Dest + parta, TaskInfo->Fifos[par_Handle].OutData, (size_t)partb);
                            Src = (char*)TaskInfo->Fifos[par_Handle].OutData + (AlignedSize - parta);
                        }
                        if ((Src - (char*)TaskInfo->Fifos[par_Handle].OutData) >= TaskInfo->Fifos[par_Handle].Size) {
                            Src = (char*)TaskInfo->Fifos[par_Handle].OutData;
                        }
                        TaskInfo->Fifos[par_Handle].OutRd = Src;
                        TaskInfo->Fifos[par_Handle].OutMessagesCounter--;
                        Ret = 0;
                    } else Ret = VIRTUAL_NETWORK_ERR_MESSAGE_TO_BIG;
                } else Ret = VIRTUAL_NETWORK_ERR_NO_DATA;
            } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
        } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
    } else Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;

    return Ret;
}

int XilEnvInternal_VirtualNetworkOutReadByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle,
                                                   VIRTUAL_NETWORK_PACKAGE *ret_Data, int par_MaxSize)
{
    int Ret;
    EnterCriticalSection(&TaskInfo->FiFosCriticalSection);
    Ret = XilEnvInternal_VirtualNetworkOutReadByHandleNoLock(par_Handle, TaskInfo, ret_Data, par_MaxSize);
    LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);
    return Ret;
}

static int XilEnvInternal_VirtualNetworkInWriteByHandleNoLock (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle,
                                                                VIRTUAL_NETWORK_PACKAGE *par_Data, int par_Size)
{
    int Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    int32_t AlignedSize;

    // if not set correct
    par_Data->Size = par_Size;
    par_Data->Timestamp = 0;

    AlignedSize = ALIGN(par_Size);

    if ((par_Handle >= 0) && (par_Handle < TaskInfo->FifoCounter)) {
        if (TaskInfo->Fifos[par_Handle].FiFoHandle == par_Handle) {
            if (TaskInfo->Fifos[par_Handle].IsOnline) {
                int Space;
                do {
                    if (TaskInfo->Fifos[par_Handle].InRd > TaskInfo->Fifos[par_Handle].InWr) {
                        Space = (int)((char*)(TaskInfo->Fifos[par_Handle].InRd) - (char*)(TaskInfo->Fifos[par_Handle].InWr));
                    }
                    else {
                        Space = TaskInfo->Fifos[par_Handle].Size - (int)((char*)(TaskInfo->Fifos[par_Handle].InWr) - (char*)(TaskInfo->Fifos[par_Handle].InRd));
                    }

                    // 4 bytes more as the message long (for the next size)!
                    if (Space < (AlignedSize + 4)) {
                        // no space for new data remove the oldest one before
                        char *Src;
                        int SizeRemove, AlignedSizeRemove;
                        Src = (char*)TaskInfo->Fifos[par_Handle].InRd;
                        SizeRemove = *(uint32_t*)Src;
                        if (SizeRemove) {    // new data inside fifo?
                            int parta;

                            AlignedSizeRemove = ALIGN(SizeRemove);
                            parta = (int)(TaskInfo->Fifos[par_Handle].Size - (Src - (char*)TaskInfo->Fifos[par_Handle].InData));
                            if (parta >= SizeRemove) {
                                Src += AlignedSizeRemove;
                            }  else {
                                Src = (char*)TaskInfo->Fifos[par_Handle].InData + (AlignedSizeRemove - parta);
                            }
                            if ((Src - (char*)TaskInfo->Fifos[par_Handle].InData) >= TaskInfo->Fifos[par_Handle].Size) {
                                Src = (char*)TaskInfo->Fifos[par_Handle].InData;
                            }
                            TaskInfo->Fifos[par_Handle].InRd = Src;
                        } else break;
                        TaskInfo->Fifos[par_Handle].InMessagesCounter--;
                        TaskInfo->Fifos[par_Handle].InLostMessagesCounter++;
                    } else break;
                } while (1);

                if (Space >= (AlignedSize + 4)) {
                    int parta, partb;
                    char *Dst;
                    int32_t *PointerToSize;

                    PointerToSize = (int32_t*)(TaskInfo->Fifos[par_Handle].InWr);

                    Dst = (char*)TaskInfo->Fifos[par_Handle].InWr;
                    parta = (int)(TaskInfo->Fifos[par_Handle].Size - (Dst - (char*)TaskInfo->Fifos[par_Handle].InData));
                    if (parta >= AlignedSize) {
                        MEMCPY(Dst, par_Data, (size_t)par_Size);
                        Dst += AlignedSize;
                    } else {
                        partb = AlignedSize - parta;
                        MEMCPY(Dst, par_Data, (size_t)parta);
                        MEMCPY(TaskInfo->Fifos[par_Handle].InData, (char*)par_Data + parta, (size_t)par_Size - parta);
                        Dst = (char*)TaskInfo->Fifos[par_Handle].InData + partb;
                    }
                    if ((Dst - (char*)TaskInfo->Fifos[par_Handle].InData) >= TaskInfo->Fifos[par_Handle].Size) {
                        Dst = (char*)TaskInfo->Fifos[par_Handle].InData;
                    }
                    TaskInfo->Fifos[par_Handle].InWr = (void*)Dst;
                    *(int32_t*)(TaskInfo->Fifos[par_Handle].InWr) = 0;
                    // Now the fifo entry is valid
                    *PointerToSize = par_Size;
                    TaskInfo->Fifos[par_Handle].InMessagesCounter++;
                } else Ret = VIRTUAL_NETWORK_ERR_MESSAGE_TO_BIG;
            } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
        } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
    } else Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    return Ret;
}


static int XilEnvInternal_VirtualNetworkOutWriteByHandleNoLock (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle,
                                                                 VIRTUAL_NETWORK_PACKAGE *par_Data, int par_Size)
{
    int Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    int32_t AlignedSize;

    // if not set correct
    par_Data->Size = par_Size;

    AlignedSize = ALIGN(par_Size);

    if ((par_Handle >= 0) && (par_Handle < TaskInfo->FifoCounter)) {
        if (TaskInfo->Fifos[par_Handle].FiFoHandle == par_Handle) {
            if (TaskInfo->Fifos[par_Handle].IsOnline) {
                int Space;
                do {
                    if (TaskInfo->Fifos[par_Handle].OutRd > TaskInfo->Fifos[par_Handle].OutWr) {
                        Space = (int)((char*)(TaskInfo->Fifos[par_Handle].OutRd) - (char*)(TaskInfo->Fifos[par_Handle].OutWr));
                    }
                    else {
                        Space = TaskInfo->Fifos[par_Handle].Size - (int)((char*)(TaskInfo->Fifos[par_Handle].OutWr) - (char*)(TaskInfo->Fifos[par_Handle].OutRd));
                    }

                    // 4 bytes more as the message long (for the next size)!
                    if (Space < (AlignedSize + 4)) {
                        // no space for new data remove the oldest one before
                        char *Src;
                        int SizeRemove, AlignedSizeRemove;
                        Src = (char*)TaskInfo->Fifos[par_Handle].OutRd;
                        SizeRemove = *(uint32_t*)Src;
                        if (SizeRemove) {    // new data inside fifo?
                            int parta;

                            AlignedSizeRemove = ALIGN(SizeRemove);
                            parta = (int)(TaskInfo->Fifos[par_Handle].Size - (Src - (char*)TaskInfo->Fifos[par_Handle].OutData));
                            if (parta >= SizeRemove) {
                                Src += AlignedSizeRemove;
                            }  else {
                                Src = (char*)TaskInfo->Fifos[par_Handle].OutData + (AlignedSizeRemove - parta);
                            }
                            if ((Src - (char*)TaskInfo->Fifos[par_Handle].OutData) >= TaskInfo->Fifos[par_Handle].Size) {
                                Src = (char*)TaskInfo->Fifos[par_Handle].OutData;
                            }
                            TaskInfo->Fifos[par_Handle].OutRd = Src;
                        } else break;
                        TaskInfo->Fifos[par_Handle].OutMessagesCounter--;
                        TaskInfo->Fifos[par_Handle].OutLostMessagesCounter++;
                    } else break;
                } while (1);

                if (Space >= (AlignedSize + 4)) {
                    int parta, partb;
                    char *Dst;
                    int32_t *PointerToSize;

                    PointerToSize = (int32_t*)(TaskInfo->Fifos[par_Handle].OutWr);

                    Dst = (char*)TaskInfo->Fifos[par_Handle].OutWr;
                    parta = (int)(TaskInfo->Fifos[par_Handle].Size - (Dst - (char*)TaskInfo->Fifos[par_Handle].OutData));
                    if (parta >= AlignedSize) {
                        MEMCPY(Dst, par_Data, (size_t)par_Size);
                        Dst += AlignedSize;
                    } else {
                        partb = AlignedSize - parta;
                        MEMCPY(Dst, par_Data, (size_t)parta);
                        MEMCPY(TaskInfo->Fifos[par_Handle].OutData, (char*)par_Data + parta, (size_t)par_Size - parta);
                        Dst = (char*)TaskInfo->Fifos[par_Handle].OutData + partb;
                    }
                    if ((Dst - (char*)TaskInfo->Fifos[par_Handle].OutData) >= TaskInfo->Fifos[par_Handle].Size) {
                        Dst = (char*)TaskInfo->Fifos[par_Handle].OutData;
                    }
                    TaskInfo->Fifos[par_Handle].OutWr = (void*)Dst;
                    *(int32_t*)(TaskInfo->Fifos[par_Handle].OutWr) = 0;
                    // Now the fifo entry is valid
                    *PointerToSize = par_Size;
                    TaskInfo->Fifos[par_Handle].OutMessagesCounter++;
                    Ret = 0;
                } else Ret = VIRTUAL_NETWORK_ERR_MESSAGE_TO_BIG;
            } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
        } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
    } else Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    return Ret;
}

int XilEnvInternal_VirtualNetworkOutWriteByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle,
                                                    VIRTUAL_NETWORK_PACKAGE *par_Data, int par_Size)
{
    int Ret;

    EnterCriticalSection(&TaskInfo->FiFosCriticalSection);
    Ret = XilEnvInternal_VirtualNetworkOutWriteByHandleNoLock (TaskInfo, par_Handle, par_Data, par_Size);
    LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);

    return Ret;
}

static int32_t BinarySearchLowestMost(uint16_t *par_Positions, int32_t par_Size, VIRTUAL_NETWORK_BUFFER *par_Buffers[], uint32_t IdSlot)
{
    int32_t l = 0;
    int32_t r = par_Size;
    while (l < r) {
        int32_t m = (l + r) >> 1;
        if (par_Buffers[par_Positions[m]]->IdSlot > IdSlot) {
            r = m;
        } else {
            l = m + 1;
        }
    }
    return l - 1;  // return -1 if value smaller than smallest inside array
}

int XilEnvInternal_VirtualNetworkInWrite (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Type, int par_Channel,
                                           VIRTUAL_NETWORK_PACKAGE *par_Data, int par_Size)
{
    int Ret = 0;
    int i;

    EnterCriticalSection(&TaskInfo->FiFosCriticalSection);
    for (i = 0; i < TaskInfo->VirtualNetworkHandleCount; i++) {
        int x = TaskInfo->VirtualNetworkHandles[i];
        VIRTUAL_NETWORK_FIFO *FiFo = &TaskInfo->Fifos[x];
        if ((FiFo->Type == par_Type) &&
            (FiFo->Channel == par_Channel)) {
            Ret |= XilEnvInternal_VirtualNetworkInWriteByHandleNoLock (TaskInfo, x, par_Data, par_Size);
            if (FiFo->RdBufferCount > 0) {
                // if existing read message buffers check if the package match one
                int Pos = BinarySearchLowestMost(FiFo->RdPositions, FiFo->RdBufferCount, FiFo->RdMessageBuffers, par_Data->Data.CanFd.Id);
                if ((Pos >= 0) && (Pos < FiFo->RdBufferCount)) {
                    for (x = Pos; (x >= 0)  && (FiFo->RdMessageBuffers[FiFo->RdPositions[x]]->IdSlot == par_Data->Data.CanFd.Id); x--) {
                        VIRTUAL_NETWORK_BUFFER *Buffer = FiFo->RdMessageBuffers[FiFo->RdPositions[x]];
                        int Size = (Buffer->Size <= (par_Data->Size - (sizeof(VIRTUAL_NETWORK_PACKAGE) - 1))) ? Buffer->Size : (par_Data->Size - (sizeof(VIRTUAL_NETWORK_PACKAGE) - 1));
                        MEMCPY(Buffer->Data, par_Data->Data.CanFd.Data, Size);
                        if (Buffer->NewData) Buffer->DataLost = 1;
                        Buffer->NewData = 1;
                    }
                }
            }
        }
    }
    LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);

    return Ret;
}

int XilEnvInternal_VirtualNetworkGetTypeByChannel (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Channel, int *ret_Handle)
{
    int i;
    int Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    EnterCriticalSection(&TaskInfo->FiFosCriticalSection);
    for (i = 0; i < TaskInfo->VirtualNetworkHandleCount; i++) {
        int x = TaskInfo->VirtualNetworkHandles[i];
        if (TaskInfo->Fifos[x].Channel == par_Channel) {
            Ret = TaskInfo->Fifos[x].Type;
            *ret_Handle = TaskInfo->Fifos[x].FiFoHandle;
            break; // for(;;)
        }
    }
    LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);
    
    return Ret;
}


int XilEnvInternal_VirtualNetworkGetRxFillByChannel (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Channel)
{
    int i;
    int Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    EnterCriticalSection(&TaskInfo->FiFosCriticalSection);
    for (i = 0; i < TaskInfo->VirtualNetworkHandleCount; i++) {
        int x = TaskInfo->VirtualNetworkHandles[i];
        if (TaskInfo->Fifos[x].Channel == par_Channel) {
            Ret = TaskInfo->Fifos[x].InMessagesCounter;
            break; // for(;;)
        }
    }
    LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);

    return Ret;
}


// Message buffers

int XilEnvInternal_VirtualNetworkConfigBufferByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle,
                                                        int par_Number, int par_IdSlot, int par_Dir, int par_Size, int par_Flags)
{
    int Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    EnterCriticalSection(&TaskInfo->FiFosCriticalSection);
    if ((par_Handle >= 0) && (par_Handle < TaskInfo->FifoCounter) && (par_Number >= 0) && (par_Number < VIRTUAL_NETWORK_MAX_BUFFERS)) {
        if (TaskInfo->Fifos[par_Handle].FiFoHandle == par_Handle) {
            if (TaskInfo->Fifos[par_Handle].IsOnline) {
                VIRTUAL_NETWORK_BUFFER **Buffers;
                uint16_t *RetBufferCount;
                uint16_t *Positions;
                if (par_Dir == 0) { // receive buffer
                     Buffers = TaskInfo->Fifos[par_Handle].RdMessageBuffers;
                     Positions = TaskInfo->Fifos[par_Handle].RdPositions;
                     RetBufferCount = &TaskInfo->Fifos[par_Handle].RdBufferCount;
                } else {  // transmit buffer
                     Buffers = TaskInfo->Fifos[par_Handle].WrMessageBuffers; 
                     Positions = TaskInfo->Fifos[par_Handle].WrPositions;
                     RetBufferCount = &TaskInfo->Fifos[par_Handle].WrBufferCount;
                }
                if ((Buffers[par_Number] == NULL) ||
                    ((int)Buffers[par_Number]->Size < par_Size)) {
                    Buffers[par_Number] = (VIRTUAL_NETWORK_BUFFER*)XilEnvInternal_realloc(Buffers[par_Number], (sizeof(VIRTUAL_NETWORK_BUFFER) - 1) + par_Size);
                }
                if (Buffers[par_Number] != NULL) {
                    int x, y;
                    Buffers[par_Number]->IdSlot = par_IdSlot;
                    Buffers[par_Number]->Size = par_Size;
                    Buffers[par_Number]->Flags = par_Flags;
                    Buffers[par_Number]->DataLost = 0;
                    Buffers[par_Number]->NewData = 0;
                    memset(Buffers[par_Number]->Data, 0, par_Size);
                    for (x = 0; x < *RetBufferCount; x++) {
                        if ((int)Buffers[Positions[x]]->IdSlot > par_IdSlot) break;
                    }
                    for (y = *RetBufferCount; y >= x; y--) {
                        Positions[y+1] = Positions[y];
                    }
                    Positions[x] = par_Number;
                    (*RetBufferCount)++;
                }
                Ret = 0;
            } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
        } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
    } else Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);

    return Ret;
}


int XilEnvInternal_VirtualNetworkReadBufferByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle, int par_Number,
                                                      unsigned char *ret_Data, int *ret_IdSlot,  unsigned char *ret_Ext, int *ret_Size, int par_MaxSize)
{
    int Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    EnterCriticalSection(&TaskInfo->FiFosCriticalSection);
    if ((par_Handle >= 0) && (par_Handle < TaskInfo->FifoCounter) && (par_Number >= 0) && (par_Number < VIRTUAL_NETWORK_MAX_BUFFERS)) {
        if (TaskInfo->Fifos[par_Handle].FiFoHandle == par_Handle) {
            if (TaskInfo->Fifos[par_Handle].RdMessageBuffers[par_Number] != NULL) {
                VIRTUAL_NETWORK_BUFFER *Buffer = TaskInfo->Fifos[par_Handle].RdMessageBuffers[par_Number];
                int Size = ((int)Buffer->Size <= par_MaxSize) ? Buffer->Size : par_MaxSize;
                MEMCPY(ret_Data, Buffer->Data, Size);
                *ret_Size = Size;
                *ret_Ext = Buffer->Flags;
                *ret_IdSlot = Buffer->IdSlot;
                Ret = Buffer->NewData;
                Buffer->NewData = 0;
            } else Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
        } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
    } else Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);

    return Ret;
}

int XilEnvInternal_VirtualNetworkPeekBufferByHandle (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle, int par_Number,
                                                      unsigned char *ret_Data, int *ret_IdSlot,  unsigned char *ret_Ext, int *ret_Size, int par_MaxSize)
{
    int Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    EnterCriticalSection(&TaskInfo->FiFosCriticalSection);
    if ((par_Handle >= 0) && (par_Handle < TaskInfo->FifoCounter) && (par_Number >= 0) && (par_Number < VIRTUAL_NETWORK_MAX_BUFFERS)) {
        if (TaskInfo->Fifos[par_Handle].FiFoHandle == par_Handle) {
            if (TaskInfo->Fifos[par_Handle].RdMessageBuffers[par_Number] != NULL) {
                VIRTUAL_NETWORK_BUFFER *Buffer = TaskInfo->Fifos[par_Handle].RdMessageBuffers[par_Number];
                int Size = ((int)Buffer->Size <= par_MaxSize) ? Buffer->Size : par_MaxSize;
                MEMCPY(ret_Data, Buffer->Data, Size);
                *ret_Size = Size;
                *ret_Ext = Buffer->Flags;
                *ret_IdSlot = Buffer->IdSlot;
                Ret = Buffer->NewData;
            } else Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
        } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
    } else Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);

    return Ret;
}


VIRTUAL_NETWORK_BUFFER *XilEnvInternal_VirtualNetworkGetBufferByHandleAndLock (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, int par_Handle, int par_Number)
{
    VIRTUAL_NETWORK_BUFFER *Ret = NULL;
    EnterCriticalSection(&TaskInfo->FiFosCriticalSection);
    if ((par_Handle >= 0) && (par_Handle < TaskInfo->FifoCounter) && (par_Number >= 0) && (par_Number < VIRTUAL_NETWORK_MAX_BUFFERS) &&
        (TaskInfo->Fifos[par_Handle].FiFoHandle == par_Handle)) {
        Ret = TaskInfo->Fifos[par_Handle].WrMessageBuffers[par_Number];
    } else {
        LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);
    }

    return Ret;
}


void XilEnvInternal_VirtualNetworkUnLockBuffer(EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, VIRTUAL_NETWORK_BUFFER *par_Buffer)
{
    if (par_Buffer != NULL) LeaveCriticalSection(&TaskInfo->FiFosCriticalSection);
}

int XilEnvInternal_VirtualNetworkCopyFromPipeToFifos (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, char *SnapShotData, int SnapShotSize)
{
    int Ret = 0;
    int Pos;
    VIRTUAL_NETWORK_PACKAGE *Package = (VIRTUAL_NETWORK_PACKAGE*)SnapShotData;
    for (Pos = 0; (Pos < SnapShotSize); ) {
        Ret |= XilEnvInternal_VirtualNetworkInWrite(TaskInfo, Package->Type, Package->Channel, Package, Package->Size);
        Pos += Package->Size;
        Package = (VIRTUAL_NETWORK_PACKAGE*)(SnapShotData + Pos);
    }
    return Ret;
}

int XilEnvInternal_VirtualNetworkCopyFromFifosToPipe (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfo, char *SnapShotData, int par_MaxSize)
{
    int x, Pos = 0;
    VIRTUAL_NETWORK_PACKAGE *Package = (VIRTUAL_NETWORK_PACKAGE*)SnapShotData;

    for (x = 0; x < TaskInfo->VirtualNetworkHandleCount; x++) {
        while (XilEnvInternal_VirtualNetworkOutReadByHandleNoLock(TaskInfo->VirtualNetworkHandles[x], TaskInfo, Package, par_MaxSize - Pos) == 0) {
            Pos += Package->Size;
            Package = (VIRTUAL_NETWORK_PACKAGE*)(SnapShotData + Pos);
        }
    }

    return Pos;
}

