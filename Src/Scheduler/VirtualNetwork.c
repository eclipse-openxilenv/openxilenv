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
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "tcb.h"
#include "Scheduler.h"
#include "VirtualNetwork.h"

typedef struct {
    uint32_t IsOnline;
    int32_t FiFoHandle;
    int32_t Size;
    int32_t Type;
    void *Data;
    void *Wr;
    void *Rd;
    uint64_t DeleteTimeStamp;
    int32_t Channel;
    uint32_t LostMessagesCounter;
    int32_t Pid;
    int (*InsertErrorHookFunction) (int par_Handle,
                                    VIRTUAL_NETWORK_PACKAGE *par_Data, VIRTUAL_NETWORK_PACKAGE *ret_Data,
                                    int par_MaxSize, int par_Dir);
} VIRTUAL_NETWORK_FIFO;

static VIRTUAL_NETWORK_FIFO *Fifos;
static int FifoCounter;

static CRITICAL_SECTION FiFosCriticalSection;


int VirtualNetworkInit(void)
{
    InitializeCriticalSection (&FiFosCriticalSection);
    return 0;
}

void VirtualNetworkTerminate(void)
{
    if (Fifos != NULL) my_free(Fifos);
    Fifos = NULL;
    FifoCounter = 0;
}

int VirtualNetworkOpen (int par_Pid, int par_Type, int par_Channel, int par_Size)
{
    int32_t x;
    int AlignedSize;
    VIRTUAL_NETWORK_FIFO *Fifo;

#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "CreateNewRxFifo(RxPid = %i, Size = %i, Name = \"%s\")\n", RxPid, Size, Name);
#endif

    AlignedSize = ALIGN(par_Size);

    EnterCriticalSection (&FiFosCriticalSection);
    x = 0;
    for(;;) {
        for ( ; x < FifoCounter; x++) {
            if (Fifos[x].FiFoHandle < 0) {  // free FiFo

                STRUCT_ZERO_INIT(Fifos[x], VIRTUAL_NETWORK_FIFO);
                Fifos[x].FiFoHandle = x;
                break;  // for(;;)
            }
        }
        if (x == FifoCounter) {
            FifoCounter += 10;  // 10 new fifos
            Fifos = (VIRTUAL_NETWORK_FIFO*)my_realloc(Fifos, sizeof(Fifos[0]) * FifoCounter);
            MEMSET(Fifos + (FifoCounter - 10), 0, sizeof(Fifos[0]) * 10);
            for (int i = (FifoCounter - 10); i < FifoCounter; i++) {
                Fifos[i].FiFoHandle = -1;
            }
            continue;
        } else {
            break;
        }
    }
    LeaveCriticalSection (&FiFosCriticalSection);

#ifdef FIFO_LOG_TO_FILE
    fprintf (FifoLogFh, "    FiFoId[%i] = 0x%X\n", x, Fifos[x].FiFoId);
#endif
    Fifo = &Fifos[x];
    Fifo->Size = AlignedSize;
    Fifo->Type = par_Type;
    Fifo->Channel = par_Channel;
    Fifo->Pid = par_Pid;
    Fifo->InsertErrorHookFunction = NULL;

    if (AlignedSize > 0) {
        Fifo->Data = my_calloc((size_t)AlignedSize, 1);
        Fifo->Wr = Fifo->Data;
        Fifo->Rd = Fifo->Data;
        if (Fifo->Data == NULL) {
            Fifo->FiFoHandle = 0;
            ThrowError(1, "cannot create virtual network fifo with size %i", AlignedSize);
        }
    } else {
        Fifo->Data = NULL;
        Fifo->Wr = NULL;
        Fifo->Rd = NULL;
    }
    EnterCriticalSection(&FiFosCriticalSection);
    // add the fifo to the TCB
    TASK_CONTROL_BLOCK *pTcb =  GetPointerToTaskControlBlock (par_Pid);
    if (pTcb != NULL) {
        if (pTcb->VirtualNetworkHandleCount < 16) {
            pTcb->VirtualNetworkHandles[pTcb->VirtualNetworkHandleCount] = Fifo->FiFoHandle;
            pTcb->VirtualNetworkHandleCount++;
        }
    }
    Fifo->IsOnline = 1;
    LeaveCriticalSection(&FiFosCriticalSection);
    return Fifo->FiFoHandle;
}

static void VirtualNetworkCloseRemoveFromTcb(int par_Pid, int par_Handle)
{
    TASK_CONTROL_BLOCK *pTcb =  GetPointerToTaskControlBlock (par_Pid);
    if (pTcb != NULL) {
        for (int x = 0; x < pTcb->VirtualNetworkHandleCount; x++) {
            if (pTcb->VirtualNetworkHandles[x] == par_Handle) {
                for (x = x + 1; x < pTcb->VirtualNetworkHandleCount; x++) {
                    pTcb->VirtualNetworkHandles[x-1] = pTcb->VirtualNetworkHandles[x];
                }
                pTcb->VirtualNetworkHandleCount--;
                break;
            }
        }
    }
}

void VirtualNetworkCloseAllChannelsForProcess(TASK_CONTROL_BLOCK *par_Tcb)
{
    if (par_Tcb != NULL) {
        EnterCriticalSection(&FiFosCriticalSection);
        for (int x = 0; x < par_Tcb->VirtualNetworkHandleCount; x++) {
            int Handle = par_Tcb->VirtualNetworkHandles[x];
            if ((Fifos[Handle].IsOnline) &&
                (Fifos[Handle].Pid == par_Tcb->pid) &&
                (Fifos[Handle].FiFoHandle == Handle)) {
                Fifos[Handle].IsOnline = 0;
                Fifos[Handle].FiFoHandle = -1;
                if (Fifos[Handle].Data != NULL) my_free(Fifos[Handle].Data);
            }
        }
        par_Tcb->VirtualNetworkHandleCount = 0;
        LeaveCriticalSection(&FiFosCriticalSection);
    }
}


int VirtualNetworkCloseByHandle (int par_Pid, int par_Handle)
{
    int x;
    int Ret = -1;
    EnterCriticalSection(&FiFosCriticalSection);
    for (x = 0; x < FifoCounter; x++) {
        VIRTUAL_NETWORK_FIFO *Fifo = &Fifos[x];
        if (Fifo->IsOnline &&
            (Fifo->Pid == par_Pid) &&
            (Fifo->FiFoHandle == par_Handle)) {
            VirtualNetworkCloseRemoveFromTcb(Fifo->Pid, Fifo->FiFoHandle);
            Fifo->IsOnline = 0;
            Fifo->FiFoHandle = -1;
            if (Fifo->Data != NULL) my_free(Fifo->Data);
            Ret = 0;
            break;  // for(;;)
        }
    }
    LeaveCriticalSection(&FiFosCriticalSection);
    return Ret;
}

int VirtualNetworkRead (int par_Pid, int par_Type, int par_Channel,
                         VIRTUAL_NETWORK_PACKAGE *ret_Data, int par_MaxSize);

int VirtualNetworkReadByHandle (int par_Handle,
                                VIRTUAL_NETWORK_PACKAGE *ret_Data, int par_MaxSize)
{
    int Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    int32_t Size;
    int32_t AlignedSize;

    EnterCriticalSection(&FiFosCriticalSection);
    if ((par_Handle >= 0) && (par_Handle < FifoCounter)) {
        if (Fifos[par_Handle].FiFoHandle == par_Handle) {
            if (Fifos[par_Handle].IsOnline) {
                char *Dest;
                char *Src;
__NEXT_ONE:
                Src = (char*)Fifos[par_Handle].Rd;
                Size = *(uint32_t*)Src;
                if (Size) {    // new data inside fifo?
                    int parta, partb;
                    if (Size <= par_MaxSize) {
                        AlignedSize = ALIGN(Size);
                        if (Fifos[par_Handle].InsertErrorHookFunction != NULL) {
                            Dest = (char*)alloca(Size);
                        } else {
                            Dest = (char*)ret_Data;
                        }
                        parta = (int)(Fifos[par_Handle].Size - (Src - (char*)Fifos[par_Handle].Data));
                        if (parta >= Size) {
                            MEMCPY(Dest, Src, (size_t)Size);
                            Src += AlignedSize;
                        }  else {
                            partb = Size - parta;
                            MEMCPY(Dest, Src, (size_t)parta);
                            MEMCPY(Dest + parta, Fifos[par_Handle].Data, (size_t)partb);
                            Src = (char*)Fifos[par_Handle].Data + (AlignedSize - parta);
                        }
                        if ((Src - (char*)Fifos[par_Handle].Data) >= Fifos[par_Handle].Size) {
                            Src = (char*)Fifos[par_Handle].Data;
                        }
                        Fifos[par_Handle].Rd = Src;
                        if (Fifos[par_Handle].InsertErrorHookFunction != NULL) {
                            switch(Fifos[par_Handle].InsertErrorHookFunction(par_Handle,
                                                                             (VIRTUAL_NETWORK_PACKAGE*)Dest, ret_Data,
                                                                             par_MaxSize, 1)) {
                            default:
                            case 0:  // use the original data
                                MEMCPY(ret_Data, Dest, Size);
                                break;
                            case 1:  // the data has changend
                                break;
                            case 2: // message should ignord
                                goto __NEXT_ONE;
                            }
                        }
                        Ret = 0;
                    } else Ret = VIRTUAL_NETWORK_ERR_MESSAGE_TO_BIG;
                } else Ret = VIRTUAL_NETWORK_ERR_NO_DATA;
            } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
        } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
    } else Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    LeaveCriticalSection(&FiFosCriticalSection);
    return Ret;
}

static int VirtualNetworkWriteByHandleNoLock (int par_Handle,
                                              VIRTUAL_NETWORK_PACKAGE *par_Data, int par_Size)
{
    int Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    int32_t AlignedSize;

    // if not set correct
    par_Data->Size = par_Size;
    AlignedSize = ALIGN(par_Size);

    if ((par_Handle >= 0) && (par_Handle < FifoCounter)) {
        VIRTUAL_NETWORK_FIFO *Fifo = &Fifos[par_Handle];
        if (Fifo->FiFoHandle == par_Handle) {
            if (Fifo->IsOnline) {
                VIRTUAL_NETWORK_PACKAGE *Data;
                if (Fifos[par_Handle].InsertErrorHookFunction != NULL) {
                    Data = (VIRTUAL_NETWORK_PACKAGE*)alloca(par_Size);
                    switch(Fifos[par_Handle].InsertErrorHookFunction(par_Handle,
                                                                     par_Data, Data,
                                                                     par_Size, 0)) {
                    default:
                    case 0:  // use the original data
                        Data = par_Data;
                        break;
                    case 1:  // the data has changed
                        break;
                    case 2:  // message should ignored
                        return 0;
                    }
                } else {
                    Data = par_Data;
                }

                int Space;
                do {
                    if (Fifo->Rd > Fifo->Wr) {
                        Space = (int)((char*)(Fifo->Rd) - (char*)(Fifo->Wr));
                    }
                    else {
                        Space = Fifo->Size - (int)((char*)(Fifo->Wr) - (char*)(Fifo->Rd));
                    }

                    // 4 bytes more as the message long (for the next size)!
                    if (Space < (AlignedSize + 4)) {
                        // no space for new data remove the oldest one before
                        char *Src;
                        int SizeRemove, AlignedSizeRemove;
                        Src = (char*)Fifo->Rd;
                        SizeRemove = *(uint32_t*)Src;
                        if (SizeRemove) {    // new data inside fifo?
                            int parta;
                            AlignedSizeRemove = ALIGN(SizeRemove);
                            parta = (int)(Fifo->Size - (Src - (char*)Fifo->Data));
                            if (parta >= SizeRemove) {
                                Src += AlignedSizeRemove;
                            }  else {
                                Src = (char*)Fifo->Data + (AlignedSizeRemove - parta);
                            }
                            if ((Src - (char*)Fifo->Data) >= Fifo->Size) {
                                Src = (char*)Fifo->Data;
                            }
                            Fifo->Rd = Src;
                        } else break;
                        Fifo->LostMessagesCounter++;
                    } else break;
                } while (1);

                if (Space >= (AlignedSize + 4)) {
                    int parta, partb;
                    char *Dst;
                    int32_t *PointerToSize;

                    PointerToSize = (int32_t*)(Fifo->Wr);

                    Dst = (char*)Fifo->Wr;
                    parta = (int)(Fifo->Size - (Dst - (char*)Fifo->Data));
                    if (parta >= AlignedSize) {
                        MEMCPY(Dst, Data, (size_t)par_Size);
                        Dst += AlignedSize;
                    } else {
                        partb = AlignedSize - parta;
                        MEMCPY(Dst, Data, (size_t)parta);
                        MEMCPY(Fifo->Data, (char*)Data + parta, (size_t)par_Size - parta);
                        Dst = (char*)Fifo->Data + partb;
                    }
                    if ((Dst - (char*)Fifo->Data) >= Fifo->Size) {
                        Dst = (char*)Fifo->Data;
                    }
                    Fifo->Wr = (void*)Dst;
                    *(int32_t*)(Fifo->Wr) = 0;
                    // erst hier den FiFo Eintrag gueltig setzen
                    *PointerToSize = par_Size;
                    Ret = 0;
                } else Ret = VIRTUAL_NETWORK_ERR_MESSAGE_TO_BIG;
            } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
        } else Ret = VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE;
    } else Ret = VIRTUAL_NETWORK_ERR_INVALID_PARAMETER;
    return Ret;
}

int VirtualNetworkWriteByHandle (int par_Handle,
                                 VIRTUAL_NETWORK_PACKAGE *par_Data, int par_Size)
{
    int Ret;
    EnterCriticalSection(&FiFosCriticalSection);
    Ret = VirtualNetworkWriteByHandleNoLock (par_Handle, par_Data, par_Size);
    LeaveCriticalSection(&FiFosCriticalSection);
    return Ret;
}


int VirtualNetworkWrite (int par_Pid, int par_Type, int par_Channel,
                         VIRTUAL_NETWORK_PACKAGE *par_Data, int par_Size)
{
    int Ret = 0;
    int x;
    EnterCriticalSection(&FiFosCriticalSection);
    for (x = 0; x < FifoCounter; x++) {
        VIRTUAL_NETWORK_FIFO *FiFoPtr = &Fifos[x];
        if ((FiFoPtr->FiFoHandle >= 0) &&
            FiFoPtr->IsOnline &&
            (FiFoPtr->Pid != par_Pid) &&   // do not write to own FiFos
            ((FiFoPtr->Type == par_Type) ||
             // CAN messages can be transmitted to a CANFD bus
             ((FiFoPtr->Type == VIRTUAL_NETWORK_TYPE_CANFD) && (par_Type == VIRTUAL_NETWORK_TYPE_CAN)) ||
             // CANFD messages can be tranmitted to CAN if CANFD_BTS_MASK and CANFD_FDF_MASK are not set
             ((FiFoPtr->Type == VIRTUAL_NETWORK_TYPE_CAN) && (par_Type == VIRTUAL_NETWORK_TYPE_CANFD) && ((par_Data->Data.CanFd.Flags & 6) == 0))) &&
            (FiFoPtr->Channel == par_Channel)) {
            Ret |= VirtualNetworkWriteByHandleNoLock (x, par_Data, par_Size);
        }
    }
    LeaveCriticalSection(&FiFosCriticalSection);
    return Ret;
}


int VirtualNetworkCopyFromPipeToFifos (TASK_CONTROL_BLOCK *Tcb, uint64_t Timestamp, char *SnapShotData, int SnapShotSize)
{
    int Ret = 0;
    int Pos;
    VIRTUAL_NETWORK_PACKAGE *Package = (VIRTUAL_NETWORK_PACKAGE*)SnapShotData;
    for (Pos = 0; (Pos < SnapShotSize) && (Package->Size > 0); ) {
        Package->Timestamp = Timestamp;
        Ret |= VirtualNetworkWrite(Tcb->pid, Package->Type, Package->Channel, Package, Package->Size);
        Pos += Package->Size;
        Package = (VIRTUAL_NETWORK_PACKAGE*)(SnapShotData + Pos);
    }
    return Ret;
}

int VirtualNetworkCopyFromFifosToPipe (TASK_CONTROL_BLOCK *Tcb, char *SnapShotData, int par_MaxSize)
{
    int Pos = 0;
    int x;
    VIRTUAL_NETWORK_PACKAGE *Package = (VIRTUAL_NETWORK_PACKAGE*)SnapShotData;
    for (x = 0; x < Tcb->VirtualNetworkHandleCount; x++) {
        while (VirtualNetworkReadByHandle(Tcb->VirtualNetworkHandles[x], Package, par_MaxSize - Pos) == 0) {
            Pos += Package->Size;
            Package = (VIRTUAL_NETWORK_PACKAGE*)(SnapShotData + Pos);
        }
    }
    return Pos;
}

static int VirtualNetworkSetInsertErrorHookFunctionNoLock(int par_Handle,
                                                          int (*par_InsertErrorHookFunction)(int par_Handle,
                                                                                             VIRTUAL_NETWORK_PACKAGE *par_Data, VIRTUAL_NETWORK_PACKAGE *ret_Data,
                                                                                             int par_MaxSize, int par_Dir))
{
    int Ret = -1;
    if ((par_Handle >= 0) && (par_Handle < FifoCounter)) {
        VIRTUAL_NETWORK_FIFO *Fifo = &Fifos[par_Handle];
        if (Fifo->FiFoHandle == par_Handle) {
            Fifo->InsertErrorHookFunction = par_InsertErrorHookFunction;
            Ret = 0;
        }
    }
    return Ret;
}

int VirtualNetworkSetInsertErrorHookFunction(int par_Pid, int par_Type, int par_Channel,
                                             int (*par_InsertErrorHookFunction)(int par_Handle,
                                                                                VIRTUAL_NETWORK_PACKAGE *par_Data, VIRTUAL_NETWORK_PACKAGE *ret_Data,
                                                                                int par_MaxSize, int par_Dir))
{
    int Ret = 0;
    int x;
    EnterCriticalSection(&FiFosCriticalSection);
    for (x = 0; x < FifoCounter; x++) {
        VIRTUAL_NETWORK_FIFO *FiFoPtr = &Fifos[x];
        if ((FiFoPtr->FiFoHandle >= 0) &&
            FiFoPtr->IsOnline &&
            ((FiFoPtr->Pid == par_Pid) || (par_Pid == 0)) &&
            (FiFoPtr->Type == par_Type) &&
            (FiFoPtr->Channel == par_Channel)) {
            Ret |= VirtualNetworkSetInsertErrorHookFunctionNoLock(x, par_InsertErrorHookFunction);
        }
    }
    LeaveCriticalSection(&FiFosCriticalSection);
    return Ret;
}

