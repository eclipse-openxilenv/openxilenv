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
#include "Blackboard.h"
#include "CanFifo.h"
#ifndef REMOTE_MASTER
#include "MainValues.h"
#include "RemoteMasterCanFifo.h"
#else
#include "RealtimeScheduler.h"
#include "RemoteMasterLock.h"
#endif

#define UNUSED(x) (void)(x)

typedef struct {
    union {
        CAN_FIFO_ELEM *noFd;
        CAN_FD_FIFO_ELEM *Fd;
    } pElems;
    int Depth;
    int ElemCount;
    int ReadPtr;
    int WritePtr;
} CAN_FIFO;


typedef struct {
    int AssignedHandle;  // Consists a 8bit wide index (0...9) and a 16bit counter this will be incremented by one for each new fifo
    CAN_FIFO RxQueue;
    CAN_FIFO TxQueue[MAX_CAN_CHANNELS];
    CAN_ACCEPT_MASK *CanAcceptMasks;
    int Channel;
    int CanFdFlag;
    int OverFlowBehaviour;  // 0 -> lose the oldest message immediately,
                            // 1...32767 -> wait the number of 10ms than lose the oldest message
                            // -1 -> wait endless no message will be lost
} CAN_FIFO_NODE;

static CAN_FIFO_NODE *CanFifoNodes[MAX_CAN_FIFO_COUNT];

#ifdef REMOTE_MASTER
static int LagacyCanFifoGlobalHandle;
REMOTE_MASTER_LOCK CANFifoGlobalSpinlock;
#define x__AcquireSpinlock(s,l,f) RemoteMasterLock (s,l,f);
#define x__ReleaseSpinlock(s) RemoteMasterUnlock (s);

#else
static CRITICAL_SECTION CANFifoGlobalSpinlock;
#define x__AcquireSpinlock(s,l,f) EnterCriticalSection (s);
#define x__ReleaseSpinlock(s) LeaveCriticalSection (s);
#endif

static int GenNewUniqueHandle (int Node)
{
   static unsigned int UniqueHandleCounter;

    UniqueHandleCounter++;
    return (int)((UniqueHandleCounter & 0xFFFF) << 8) + Node;
}

#define DEFAULT_PROCESS_PARAMETER   -1

/* Setup a FIFO with the max. 'Depth' CAN objects */
int CreateCanFifos (int Depth, int FdFlag)
{
    CAN_FIFO_NODE *pcf;
    int x, node, i, y;
    int Ret = -2;    // not -1 because DEFAULT_PROCESS_PARAMETER is -1

#ifdef REMOTE_MASTER
    int UseAsLagacyCanFifoGlobalHandle = 0;
    if (FdFlag == DEFAULT_PROCESS_PARAMETER) {
        FdFlag = 0;
        UseAsLagacyCanFifoGlobalHandle = 1;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       return rm_CreateCanFifos (Depth);
    }
#endif

    x__AcquireSpinlock(&CANFifoGlobalSpinlock, __LINE__, __FILE__);
    for (node = 0; node < MAX_CAN_FIFO_COUNT; node++) {
        if (CanFifoNodes[node] == NULL) break;
    }
    if (node == MAX_CAN_FIFO_COUNT) {
        goto __OUT;
    }
    if ((pcf = (CAN_FIFO_NODE*)my_calloc (1, sizeof (CAN_FIFO_NODE))) == NULL) {
        goto __OUT;
    }
    pcf->AssignedHandle = GenNewUniqueHandle (node);
    pcf->CanFdFlag = (FdFlag & 0x1);
    pcf->OverFlowBehaviour = (FdFlag >> 16);
    pcf->CanAcceptMasks = NULL;
    if (FdFlag) {
        if ((pcf->RxQueue.pElems.Fd = (CAN_FD_FIFO_ELEM*)my_calloc ((size_t)Depth, sizeof (CAN_FD_FIFO_ELEM))) == NULL) {
            my_free (pcf);
            goto __OUT;
        }
    } else {
        if ((pcf->RxQueue.pElems.noFd = (CAN_FIFO_ELEM*)my_calloc ((size_t)Depth, sizeof (CAN_FIFO_ELEM))) == NULL) {
            my_free (pcf);
            goto __OUT;
        }
    }
    pcf->RxQueue.Depth = Depth;
    pcf->RxQueue.ElemCount = 0;
    pcf->RxQueue.ReadPtr = pcf->RxQueue.WritePtr = 0;
    if (FdFlag) {
        for (x = 0; x < Depth; x++) pcf->RxQueue.pElems.Fd[x].flag = 0;
    } else {
        for (x = 0; x < Depth; x++) pcf->RxQueue.pElems.noFd[x].flag = 0;
    }
    // MAX_CAN_CHANNELS TX-Queues
    for (i = 0; i < MAX_CAN_CHANNELS; i++) {
        if (FdFlag) {
            if ((pcf->TxQueue[i].pElems.Fd = (CAN_FD_FIFO_ELEM*)my_calloc ((size_t)Depth, sizeof (CAN_FD_FIFO_ELEM))) == NULL) {
                for (y = 0; y < i; y++) {
                    my_free (pcf->TxQueue[y].pElems.Fd);
                }
                my_free (pcf->RxQueue.pElems.Fd);
                my_free (pcf);
                goto __OUT;
            }
        } else {
            if ((pcf->TxQueue[i].pElems.noFd = (CAN_FIFO_ELEM*)my_calloc ((size_t)Depth, sizeof (CAN_FIFO_ELEM))) == NULL) {
                for (y = 0; y < i; y++) {
                    my_free (pcf->TxQueue[y].pElems.noFd);
                }
                my_free (pcf->RxQueue.pElems.noFd);
                my_free (pcf);
                goto __OUT;
            }
        }
        pcf->TxQueue[i].Depth = Depth;
        pcf->TxQueue[i].ElemCount = 0;
        pcf->TxQueue[i].ReadPtr = pcf->TxQueue[i].WritePtr = 0;
        if (FdFlag) {
            for (x = 0; x < Depth; x++) pcf->TxQueue[i].pElems.Fd[x].flag = 0;
        } else {
            for (x = 0; x < Depth; x++) pcf->TxQueue[i].pElems.noFd[x].flag = 0;
        }
    }

    CanFifoNodes[node] = pcf;
#ifdef REMOTE_MASTER
    if (UseAsLagacyCanFifoGlobalHandle) {
        LagacyCanFifoGlobalHandle = pcf->AssignedHandle;
    }
#endif
    // all Ok
    Ret = pcf->AssignedHandle;
 __OUT:
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return Ret;
}

/* Delete the FIFO */
int DeleteCanFifos (int Handle)
{
    int x, i;
    CAN_FIFO_NODE *pcf;

#ifdef REMOTE_MASTER
    if (Handle == DEFAULT_PROCESS_PARAMETER) {
        Handle = LagacyCanFifoGlobalHandle;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       return rm_DeleteCanFifos (Handle);
    }
#endif

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
    for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            if (pcf->AssignedHandle == Handle) {
                my_free (pcf->RxQueue.pElems.noFd);
                for (i = 0; i < MAX_CAN_CHANNELS; i++) my_free (pcf->TxQueue[i].pElems.noFd);
                if (pcf->CanAcceptMasks != NULL) {
                    my_free (pcf->CanAcceptMasks);
                }
                my_free (pcf);
                for (; x < 9; x++) {
                    CanFifoNodes[x] = CanFifoNodes[x+1];
                }
                CanFifoNodes[x] = NULL;
                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                return 0;
            }
        }
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return -1;
}

/* Check if a CAN message should be accepted */
static int CheckAcceptMask (CAN_ACCEPT_MASK *pMask, unsigned long Id, int Channel)
{
    int x;

    if (pMask == NULL) return 0;
    for (x = 0; pMask[x].Channel >= 0; x++) {
        if ((Channel == pMask[x].Channel) &&
            (Id >= pMask[x].Start) &&
            (Id <= pMask[x].Stop)) {
            return 1;
        }
    }
    return 0;
}

static void SleepALittleBit(void)
{
#ifdef _WIN32
    Sleep(10);
#else
    usleep(10*1000);
#endif
}

// in th SIL simualtion it can happen that the fifo will be filled very fast
#ifdef REMOTE_MASTER
#define IF_NO_SPACE_WAIT_A_LITTLE_BIT_JUMP_POINT
#define IF_NO_SPACE_WAIT_A_LITTLE_BIT_LOOP_COUNTER
#define IF_NO_SPACE_WAIT_A_LITTLE_BIT(a, t)
#else
#define IF_NO_SPACE_WAIT_A_LITTLE_BIT_JUMP_POINT  __RESTART:
#define IF_NO_SPACE_WAIT_A_LITTLE_BIT_LOOP_COUNTER  int LoopCounter = 0;
#define IF_NO_SPACE_WAIT_A_LITTLE_BIT(a, t) \
if (a) { \
        if ((t) != 0) { \
            if (((t) < 0) || (LoopCounter < (t))) { \
                x__ReleaseSpinlock (&CANFifoGlobalSpinlock); \
                SleepALittleBit(); \
                x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__); \
                if ((t) > 0) LoopCounter++; \
                goto __RESTART; \
        } \
    } \
}
#endif

// CAN->FIFOs
int WriteCanMessageFromBus2Fifos (int Channel, uint32_t Id,
                                  unsigned char *Data, unsigned char Ext,
                                  unsigned char Size, unsigned char Node,
                                  uint64_t Timestamp)
{
    UNUSED(Timestamp);
    int x, wrptr_old;
    CAN_FIFO_NODE *pcf;
    CAN_FIFO *prxcf;
    IF_NO_SPACE_WAIT_A_LITTLE_BIT_LOOP_COUNTER

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
IF_NO_SPACE_WAIT_A_LITTLE_BIT_JUMP_POINT
    for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            //ThrowError (1, "write 0x%X", Id);
            if (CheckAcceptMask (pcf->CanAcceptMasks, Id, Channel)) {
                if (pcf->CanFdFlag){
                    CAN_FD_FIFO_ELEM CanMessage;
                    CanMessage.id = Id;
                    MEMCPY (CanMessage.data, Data, Size);
                    CanMessage.size = Size;
                    CanMessage.ext = Ext;
                    CanMessage.channel = (unsigned char)Channel;
                    CanMessage.flag = 0;
                    CanMessage.node = Node;
                #ifdef REMOTE_MASTER
                    CanMessage.timestamp = Timestamp;
                #else
                    CanMessage.timestamp = Timestamp;
                #endif
                    prxcf = &(pcf->RxQueue);
                    IF_NO_SPACE_WAIT_A_LITTLE_BIT (prxcf->pElems.Fd[prxcf->WritePtr].flag,
                                                   pcf->OverFlowBehaviour);
                    if (prxcf->pElems.Fd[prxcf->WritePtr].flag) {  // There are no free element inside the FIFO any more -> overwrite oldest entry
                        int rdptr_old = prxcf->ReadPtr;
                        prxcf->ReadPtr++;
                        if (prxcf->ReadPtr >= prxcf->Depth) prxcf->ReadPtr = 0;
                        prxcf->ElemCount--;
                        prxcf->pElems.Fd[rdptr_old].flag = 0;
                    }
                    prxcf->pElems.Fd[prxcf->WritePtr] = CanMessage;
                    wrptr_old = prxcf->WritePtr;
                    prxcf->WritePtr++;
                    if (prxcf->WritePtr >= prxcf->Depth) prxcf->WritePtr = 0;
                    prxcf->ElemCount++;
                    prxcf->pElems.Fd[wrptr_old].flag = 1;
                } else if (Size <= 8) {
                    CAN_FIFO_ELEM CanMessage;
                    CanMessage.id = Id;
                    MEMCPY (CanMessage.data, Data, Size);
                    CanMessage.size = Size;
                    CanMessage.ext = Ext;
                    CanMessage.channel = (unsigned char)Channel;
                    CanMessage.flag = 0;
                    CanMessage.node = Node;
                #ifdef REMOTE_MASTER
                    CanMessage.timestamp = Timestamp;
                #else
                    CanMessage.timestamp = (uint64_t)blackboard_infos.ActualCycleNumber;
                #endif
                    prxcf = &(pcf->RxQueue);
                    IF_NO_SPACE_WAIT_A_LITTLE_BIT (prxcf->pElems.noFd[prxcf->WritePtr].flag,
                                                   pcf->OverFlowBehaviour);
                    if (prxcf->pElems.noFd[prxcf->WritePtr].flag) {  // There are no free element inside the FIFO any more -> overwrite oldest entry
                        int rdptr_old = prxcf->ReadPtr;
                        prxcf->ReadPtr++;
                        if (prxcf->ReadPtr >= prxcf->Depth) prxcf->ReadPtr = 0;
                        prxcf->ElemCount--;
                        prxcf->pElems.noFd[rdptr_old].flag = 0;
                    }
                    prxcf->pElems.noFd[prxcf->WritePtr] = CanMessage;
                    wrptr_old = prxcf->WritePtr;
                    prxcf->WritePtr++;
                    if (prxcf->WritePtr >= prxcf->Depth) prxcf->WritePtr = 0;
                    prxcf->ElemCount++;
                    prxcf->pElems.noFd[wrptr_old].flag = 1;
                }
            }
        } else break;
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return 0;
}


// FIFO->Process
int ReadCanMessageFromFifo2Process (int Handle, CAN_FIFO_ELEM *pCanMessage)
{
    int x, rdptr_old;
    CAN_FIFO_NODE *pcf;
    CAN_FIFO *prxcf;

#ifdef REMOTE_MASTER
    if (Handle == DEFAULT_PROCESS_PARAMETER) {
        Handle = LagacyCanFifoGlobalHandle;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       return rm_ReadCanMessageFromFifo2Process (Handle, pCanMessage);
    }
#endif

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
    for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            if ((pcf->AssignedHandle == Handle) &&
                (pcf->CanFdFlag == 0)) {
                prxcf = &(pcf->RxQueue);
                if (prxcf->pElems.noFd[prxcf->ReadPtr].flag) {  // There are at least one element inside the FIFO
                    *pCanMessage = prxcf->pElems.noFd[prxcf->ReadPtr];
                    rdptr_old = prxcf->ReadPtr;
                    prxcf->ReadPtr++;
                    if (prxcf->ReadPtr >= prxcf->Depth) prxcf->ReadPtr = 0;
                    prxcf->ElemCount--;
                    prxcf->pElems.noFd[rdptr_old].flag = 0;
                    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                    return 1;
                }
                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                return 0;
            }
        }
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return -1;
}

int ReadCanFdMessageFromFifo2Process (int Handle, CAN_FD_FIFO_ELEM *pCanMessage)
{
    int x, rdptr_old;
    CAN_FIFO_NODE *pcf;
    CAN_FIFO *prxcf;

#ifdef REMOTE_MASTER
    if (Handle == DEFAULT_PROCESS_PARAMETER) {
        Handle = LagacyCanFifoGlobalHandle;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       return rm_ReadCanFdMessageFromFifo2Process (Handle, pCanMessage);
    }
#endif

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
    for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            if ((pcf->AssignedHandle == Handle) &&
                (pcf->CanFdFlag == 1)) {
                prxcf = &(pcf->RxQueue);
                if (prxcf->pElems.Fd[prxcf->ReadPtr].flag) {  // There are at least one element inside the FIFO
                    *pCanMessage = prxcf->pElems.Fd[prxcf->ReadPtr];
                    rdptr_old = prxcf->ReadPtr;
                    prxcf->ReadPtr++;
                    if (prxcf->ReadPtr >= prxcf->Depth) prxcf->ReadPtr = 0;
                    prxcf->ElemCount--;
                    prxcf->pElems.Fd[rdptr_old].flag = 0;
                    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                    return 1;
                }
                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                return 0;
            }
        }
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return -1;
}

int ReadCanMessagesFromFifo2Process (int Handle, CAN_FIFO_ELEM *pCanMessage, int MaxMessages)
{
    int x, rdptr_old;
    CAN_FIFO_NODE *pcf;
    CAN_FIFO *prxcf;
    int CopyMessageCounter = 0;

#ifdef REMOTE_MASTER
    if (Handle == DEFAULT_PROCESS_PARAMETER) {
        Handle = LagacyCanFifoGlobalHandle;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       return rm_ReadCanMessagesFromFifo2Process (Handle, pCanMessage, MaxMessages);
    }
#endif

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
    for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            if ((pcf->AssignedHandle == Handle) &&
                (pcf->CanFdFlag == 0)) {
                prxcf = &(pcf->RxQueue);
                while ((prxcf->pElems.noFd[prxcf->ReadPtr].flag) && (CopyMessageCounter < MaxMessages)) {  // There are at least one element inside the FIFO
                    pCanMessage[CopyMessageCounter++] = prxcf->pElems.noFd[prxcf->ReadPtr];
                    rdptr_old = prxcf->ReadPtr;
                    prxcf->ReadPtr++;
                    if (prxcf->ReadPtr >= prxcf->Depth) prxcf->ReadPtr = 0;
                    prxcf->ElemCount--;
                    prxcf->pElems.noFd[rdptr_old].flag = 0;
                }
                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                return CopyMessageCounter;  // Count of readed messages
            }
        }
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return -1;
}

int ReadCanFdMessagesFromFifo2Process (int Handle, CAN_FD_FIFO_ELEM *pCanMessage, int MaxMessages)
{
    int x, rdptr_old;
    CAN_FIFO_NODE *pcf;
    CAN_FIFO *prxcf;
    int CopyMessageCounter = 0;

#ifdef REMOTE_MASTER
    if (Handle == DEFAULT_PROCESS_PARAMETER) {
        Handle = LagacyCanFifoGlobalHandle;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       return rm_ReadCanFdMessagesFromFifo2Process (Handle, pCanMessage, MaxMessages);
    }
#endif

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
    for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            if ((pcf->AssignedHandle == Handle) &&
                (pcf->CanFdFlag == 1)) {
                prxcf = &(pcf->RxQueue);
                while ((prxcf->pElems.Fd[prxcf->ReadPtr].flag) && (CopyMessageCounter < MaxMessages)) {  // There are at least one element inside the FIFO
                    pCanMessage[CopyMessageCounter++] = prxcf->pElems.Fd[prxcf->ReadPtr];
                    rdptr_old = prxcf->ReadPtr;
                    prxcf->ReadPtr++;
                    if (prxcf->ReadPtr >= prxcf->Depth) prxcf->ReadPtr = 0;
                    prxcf->ElemCount--;
                    prxcf->pElems.Fd[rdptr_old].flag = 0;
                }
                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                return CopyMessageCounter;  // Count of readed messages
            }
        }
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return -1;
}

int FlushCanFifo (int Handle, int Flags)
{
    int x, y, rdptr_old;
    CAN_FIFO_NODE *pcf;
    CAN_FIFO *prxcf, *ptxcf;

#ifdef REMOTE_MASTER
    if (Handle == DEFAULT_PROCESS_PARAMETER) {
        Handle = LagacyCanFifoGlobalHandle;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       return rm_FlushCanFifo (Handle, Flags);
    }
#endif

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
    for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            if (pcf->AssignedHandle == Handle) {
                if ((Flags & 0x00000001) == 0x00000001) {
                    prxcf = &(pcf->RxQueue);
                    if (pcf->CanFdFlag) {
                        while (prxcf->pElems.Fd[prxcf->ReadPtr].flag) {  // There are at least one element inside the FIFO
                            rdptr_old = prxcf->ReadPtr;
                            prxcf->ReadPtr++;
                            if (prxcf->ReadPtr >= prxcf->Depth) prxcf->ReadPtr = 0;
                            prxcf->ElemCount--;
                            prxcf->pElems.Fd[rdptr_old].flag = 0;
                        }
                    } else {
                        while (prxcf->pElems.noFd[prxcf->ReadPtr].flag) {  // There are at least one element inside the FIFO
                            rdptr_old = prxcf->ReadPtr;
                            prxcf->ReadPtr++;
                            if (prxcf->ReadPtr >= prxcf->Depth) prxcf->ReadPtr = 0;
                            prxcf->ElemCount--;
                            prxcf->pElems.noFd[rdptr_old].flag = 0;
                        }
                    }
                }
                if ((Flags & 0x00000002) == 0x00000002) {
                    for (y = 0; y < MAX_CAN_CHANNELS; y++) {
                        ptxcf = &(pcf->TxQueue[y]);
                        if (pcf->CanFdFlag) {
                            while (ptxcf->pElems.Fd[ptxcf->ReadPtr].flag) {  // There are at least one element inside the FIFO
                                rdptr_old = ptxcf->ReadPtr;
                                ptxcf->ReadPtr++;
                                if (ptxcf->ReadPtr >= ptxcf->Depth) ptxcf->ReadPtr = 0;
                                ptxcf->ElemCount--;
                                ptxcf->pElems.Fd[rdptr_old].flag = 0;
                            }
                        } else {
                            while (ptxcf->pElems.noFd[ptxcf->ReadPtr].flag) {  // There are at least one element inside the FIFO
                                rdptr_old = ptxcf->ReadPtr;
                                ptxcf->ReadPtr++;
                                if (ptxcf->ReadPtr >= ptxcf->Depth) ptxcf->ReadPtr = 0;
                                ptxcf->ElemCount--;
                                ptxcf->pElems.noFd[rdptr_old].flag = 0;
                            }
                        }
                    }
                }
                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                return 0;
            }
        }
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return -1;
}


/* Setup an acceptance filter */
int SetAcceptMask4CanFifo (int Handle, CAN_ACCEPT_MASK *cam, int Size)
{
    int x;
    CAN_FIFO_NODE *pcf;
    CAN_ACCEPT_MASK *cam_old, *cam_new;

#ifdef REMOTE_MASTER
    if (Handle == DEFAULT_PROCESS_PARAMETER) {
        Handle = LagacyCanFifoGlobalHandle;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       return rm_SetAcceptMask4CanFifo (Handle, cam, Size);
    }
#endif

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
    for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            if (pcf->AssignedHandle == Handle) {
                cam_old = pcf->CanAcceptMasks;
                cam_new = (CAN_ACCEPT_MASK*)my_calloc ((size_t)Size, sizeof (CAN_ACCEPT_MASK));
                if (cam_new != NULL) {
                    MEMCPY (cam_new, cam, (size_t)Size * sizeof (CAN_ACCEPT_MASK));
                    cam_new[Size-1].Channel = -1;
#if 0
                    for (i = 0; /*(cam_new[i].Channel >= 0) &&*/ (i < 2); i++) {
                        ThrowError (1, "Set accept mask: channel = %i, start ID = 0x%X, stop ID = 0x%X",
                                 cam_new[i].Channel,
                                 cam_new[i].Start,
                                 cam_new[i].Stop);
                    }
#endif
                }
                pcf->CanAcceptMasks = cam_new;   // Now the filter is active
                if (cam_old != NULL) {           // we can now delete the old one
                    my_free (cam_old);
                }
                break;
            }
        }
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return 0;
}

// Process->FIFO
int WriteCanMessageFromProcess2Fifo (int Handle, CAN_FIFO_ELEM *pCanMessage)
{
    int x, wrptr_old;
    CAN_FIFO_NODE *pcf;
    CAN_FIFO *ptxcf;
    IF_NO_SPACE_WAIT_A_LITTLE_BIT_LOOP_COUNTER

#ifdef REMOTE_MASTER
    if (Handle == DEFAULT_PROCESS_PARAMETER) {
        Handle = LagacyCanFifoGlobalHandle;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       return rm_WriteCanMessageFromProcess2Fifo (Handle, pCanMessage);
    }
#endif

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
IF_NO_SPACE_WAIT_A_LITTLE_BIT_JUMP_POINT
     for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            if ((pcf->AssignedHandle == Handle) &&
                (pcf->CanFdFlag == 0)) {
                if (pCanMessage->channel < MAX_CAN_CHANNELS) {
                    ptxcf = &(pcf->TxQueue[pCanMessage->channel]);
                    IF_NO_SPACE_WAIT_A_LITTLE_BIT (ptxcf->pElems.noFd[ptxcf->WritePtr].flag,
                                                  pcf->OverFlowBehaviour);
                    if (ptxcf->pElems.noFd[ptxcf->WritePtr].flag) {  // There are no free element inside the FIFO any more -> overwrite oldest entry
                        int rdptr_old = ptxcf->ReadPtr;
                        ptxcf->ReadPtr++;
                        if (ptxcf->ReadPtr >= ptxcf->Depth) ptxcf->ReadPtr = 0;
                        ptxcf->ElemCount--;
                        ptxcf->pElems.noFd[rdptr_old].flag = 0;
                    }
                    ptxcf->pElems.noFd[ptxcf->WritePtr] = *pCanMessage;
                    wrptr_old = ptxcf->WritePtr;
                    ptxcf->WritePtr++;
                    if (ptxcf->WritePtr >= ptxcf->Depth) ptxcf->WritePtr = 0;
                    ptxcf->ElemCount++;
                    ptxcf->pElems.noFd[wrptr_old].flag = 1;
                    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                    return 0;
                }
            }
        } else break;
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return -1;
}

int WriteCanFdMessageFromProcess2Fifo (int Handle, CAN_FD_FIFO_ELEM *pCanMessage)
{
    int x, wrptr_old;
    CAN_FIFO_NODE *pcf;
    CAN_FIFO *ptxcf;
    IF_NO_SPACE_WAIT_A_LITTLE_BIT_LOOP_COUNTER

#ifdef REMOTE_MASTER
    if (Handle == DEFAULT_PROCESS_PARAMETER) {
        Handle = LagacyCanFifoGlobalHandle;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       return rm_WriteCanFdMessageFromProcess2Fifo (Handle, pCanMessage);
    }
#endif

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
IF_NO_SPACE_WAIT_A_LITTLE_BIT_JUMP_POINT
     for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            if ((pcf->AssignedHandle == Handle) &&
                (pcf->CanFdFlag == 1)) {
                if (pCanMessage->channel < MAX_CAN_CHANNELS) {
                    ptxcf = &(pcf->TxQueue[pCanMessage->channel]);
                    IF_NO_SPACE_WAIT_A_LITTLE_BIT (ptxcf->pElems.Fd[ptxcf->WritePtr].flag,
                                                   pcf->OverFlowBehaviour);
                    if (ptxcf->pElems.Fd[ptxcf->WritePtr].flag) {  // There are no free element inside the FIFO any more -> overwrite oldest entry
                        int rdptr_old = ptxcf->ReadPtr;
                        ptxcf->ReadPtr++;
                        if (ptxcf->ReadPtr >= ptxcf->Depth) ptxcf->ReadPtr = 0;
                        ptxcf->ElemCount--;
                        ptxcf->pElems.Fd[rdptr_old].flag = 0;
                    }
                    ptxcf->pElems.Fd[ptxcf->WritePtr] = *pCanMessage;
                    wrptr_old = ptxcf->WritePtr;
                    ptxcf->WritePtr++;
                    if (ptxcf->WritePtr >= ptxcf->Depth) ptxcf->WritePtr = 0;
                    ptxcf->ElemCount++;
                    ptxcf->pElems.Fd[wrptr_old].flag = 1;
                    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                    return 0;
                }
            }
        } else break;
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return -1;
}

// Process->FIFO
int WriteCanMessagesFromProcess2Fifo (int Handle, CAN_FIFO_ELEM *pCanMessage, int MaxMessages)
{
    int x, wrptr_old;
    CAN_FIFO_NODE *pcf;
    CAN_FIFO *ptxcf;
    int CopyMessageCounter = 0;
    IF_NO_SPACE_WAIT_A_LITTLE_BIT_LOOP_COUNTER

#ifdef REMOTE_MASTER
    if (Handle == DEFAULT_PROCESS_PARAMETER) {
        Handle = LagacyCanFifoGlobalHandle;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       return rm_WriteCanMessagesFromProcess2Fifo (Handle, pCanMessage, MaxMessages);
    }
#endif

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
IF_NO_SPACE_WAIT_A_LITTLE_BIT_JUMP_POINT
     for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            if ((pcf->AssignedHandle == Handle) &&
                (pcf->CanFdFlag == 0)) {
                if (pCanMessage->channel < MAX_CAN_CHANNELS) {
                    ptxcf = &(pcf->TxQueue[pCanMessage->channel]);
                    while (CopyMessageCounter < MaxMessages) {
                        IF_NO_SPACE_WAIT_A_LITTLE_BIT (ptxcf->pElems.noFd[ptxcf->WritePtr].flag,
                                                      pcf->OverFlowBehaviour);
                        if (ptxcf->pElems.noFd[ptxcf->WritePtr].flag) { // There are no free element inside the FIFO any more -> overwrite oldest entry
                            int rdptr_old = ptxcf->ReadPtr;
                            ptxcf->ReadPtr++;
                            if (ptxcf->ReadPtr >= ptxcf->Depth) ptxcf->ReadPtr = 0;
                            ptxcf->ElemCount--;
                            ptxcf->pElems.noFd[rdptr_old].flag = 0;
                        }
                        ptxcf->pElems.noFd[ptxcf->WritePtr] = pCanMessage[CopyMessageCounter++];
                        wrptr_old = ptxcf->WritePtr;
                        ptxcf->WritePtr++;
                        if (ptxcf->WritePtr >= ptxcf->Depth) ptxcf->WritePtr = 0;
                        ptxcf->ElemCount++;
                        ptxcf->pElems.noFd[wrptr_old].flag = 1;
                    }
                    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                    return CopyMessageCounter;  // Anzahl der gelesenen Messages
                }
            }
        } else break;
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return 0;
}

int WriteCanFdMessagesFromProcess2Fifo (int Handle, CAN_FD_FIFO_ELEM *pCanMessage, int MaxMessages)
{
    int x, wrptr_old;
    CAN_FIFO_NODE *pcf;
    CAN_FIFO *ptxcf;
    int CopyMessageCounter = 0;
    IF_NO_SPACE_WAIT_A_LITTLE_BIT_LOOP_COUNTER

#ifdef REMOTE_MASTER
    if (Handle == DEFAULT_PROCESS_PARAMETER) {
        Handle = LagacyCanFifoGlobalHandle;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       return rm_WriteCanFdMessagesFromProcess2Fifo (Handle, pCanMessage, MaxMessages);
    }
#endif

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
IF_NO_SPACE_WAIT_A_LITTLE_BIT_JUMP_POINT
     for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            if ((pcf->AssignedHandle == Handle) &&
                (pcf->CanFdFlag == 1)) {
                if (pCanMessage->channel < MAX_CAN_CHANNELS) {
                    ptxcf = &(pcf->TxQueue[pCanMessage->channel]);
                    while ((CopyMessageCounter < MaxMessages)) {
                        IF_NO_SPACE_WAIT_A_LITTLE_BIT (ptxcf->pElems.Fd[ptxcf->WritePtr].flag,
                                                       pcf->OverFlowBehaviour);
                        if (ptxcf->pElems.Fd[ptxcf->WritePtr].flag) { // There are no free element inside the FIFO any more -> overwrite oldest entry
                            int rdptr_old = ptxcf->ReadPtr;
                            ptxcf->ReadPtr++;
                            if (ptxcf->ReadPtr >= ptxcf->Depth) ptxcf->ReadPtr = 0;
                            ptxcf->ElemCount--;
                            ptxcf->pElems.Fd[rdptr_old].flag = 0;
                        }
                        ptxcf->pElems.Fd[ptxcf->WritePtr] = pCanMessage[CopyMessageCounter++];
                        wrptr_old = ptxcf->WritePtr;
                        ptxcf->WritePtr++;
                        if (ptxcf->WritePtr >= ptxcf->Depth) ptxcf->WritePtr = 0;
                        ptxcf->ElemCount++;
                        ptxcf->pElems.Fd[wrptr_old].flag = 1;
                    }
                    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                    return CopyMessageCounter;  // Count of readed messages
                }
            }
        } else break;
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return 0;
}

// FIFO->CAN
int WriteCanMessageFromFifo2Can (NEW_CAN_SERVER_CONFIG *csc, int Channel)
{
    int x, x2, rdptr_old, wrptr_old;
    CAN_FIFO_NODE *pcf, *pcf2;
    CAN_FIFO *ptxcf;
    CAN_FIFO *prxcf;
    IF_NO_SPACE_WAIT_A_LITTLE_BIT_LOOP_COUNTER

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
IF_NO_SPACE_WAIT_A_LITTLE_BIT_JUMP_POINT
    for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            ptxcf = &(pcf->TxQueue[Channel]);
            if (pcf->CanFdFlag) {
                CAN_FD_FIFO_ELEM *pfe;
                while (1) {
                    pfe = &(ptxcf->pElems.Fd[ptxcf->ReadPtr]);
                    if (pfe->flag) {  // There are at least one element inside the FIFO
                        if (csc->channels[Channel].queue_write_can (csc, Channel,
                                                                    pfe->id,
                                                                    pfe->data,
                                                                    pfe->ext,
                                                                    pfe->size) != -1) {
                            for (x2 = 0; x2 < MAX_CAN_FIFO_COUNT; x2++) {
                                if (x2 != x) {
                                    pcf2 = CanFifoNodes[x2];
                                    if (pcf2 != NULL) {
                                        if (CheckAcceptMask (pcf2->CanAcceptMasks, pfe->id, Channel)) {
                                        #ifdef REMOTE_MASTER
                                            pfe->timestamp = (uint64_t)get_rt_cycle_counter(); // << 32;
                                                            // (unsigned __int64)GetT4Master () << 16 |
                                                            // Timestamp;
                                        #else
                                            pfe->timestamp = (uint64_t)blackboard_infos.ActualCycleNumber << 32;
                                        #endif

                                            prxcf = &(pcf2->RxQueue);
                                            if (pcf2->CanFdFlag) {
                                                // CAN FD -> CAN FD
                                                IF_NO_SPACE_WAIT_A_LITTLE_BIT (prxcf->pElems.Fd[prxcf->WritePtr].flag,
                                                                               pcf->OverFlowBehaviour);
                                                if (!prxcf->pElems.Fd[prxcf->WritePtr].flag) {  // There are at least one element inside the FIFO leer
                                                    prxcf->pElems.Fd[prxcf->WritePtr] = *pfe;
                                                    // will be transmitted oneself
                                                    prxcf->pElems.Fd[prxcf->WritePtr].node = 1;
                                                    wrptr_old = prxcf->WritePtr;
                                                    prxcf->WritePtr++;
                                                    if (prxcf->WritePtr >= prxcf->Depth) prxcf->WritePtr = 0;
                                                    prxcf->ElemCount++;
                                                    prxcf->pElems.Fd[wrptr_old].flag = 1;
                                                }
                                            } else {
                                                // CAN FD -> CAN
                                                if (pfe->size <= 8) {  // only if message are smaller or equal 8 bytes long
                                                    IF_NO_SPACE_WAIT_A_LITTLE_BIT (prxcf->pElems.noFd[prxcf->WritePtr].flag,
                                                                                   pcf->OverFlowBehaviour);
                                                    if (!prxcf->pElems.noFd[prxcf->WritePtr].flag) {  // There are at least one element inside the FIFO leer
                                                        prxcf->pElems.noFd[prxcf->WritePtr].channel = pfe->channel;
                                                        MEMCPY(prxcf->pElems.noFd[prxcf->WritePtr].data, pfe->data, 8);
                                                        prxcf->pElems.noFd[prxcf->WritePtr].ext = pfe->ext;
                                                        prxcf->pElems.noFd[prxcf->WritePtr].id = pfe->id;
                                                        prxcf->pElems.noFd[prxcf->WritePtr].size = pfe->size;
                                                        prxcf->pElems.noFd[prxcf->WritePtr].timestamp = pfe->timestamp;
                                                        // will be transmitted oneself
                                                        prxcf->pElems.noFd[prxcf->WritePtr].node = 1;
                                                        wrptr_old = prxcf->WritePtr;
                                                        prxcf->WritePtr++;
                                                        if (prxcf->WritePtr >= prxcf->Depth) prxcf->WritePtr = 0;
                                                        prxcf->ElemCount++;
                                                        prxcf->pElems.noFd[wrptr_old].flag = 1;
                                                    }
                                                }
                                            }
                                        }
                                    } else break;
                                }
                            }

                            rdptr_old = ptxcf->ReadPtr;
                            ptxcf->ReadPtr++;
                            if (ptxcf->ReadPtr >= ptxcf->Depth) ptxcf->ReadPtr = 0;
                            ptxcf->ElemCount--;
                            ptxcf->pElems.Fd[rdptr_old].flag = 0;
                        } else {
                            break;    // while(1)
                        }
                    } else break;     // while(1)
                }
            } else {
                CAN_FIFO_ELEM *pfe;
                while (1) {
                    pfe = &(ptxcf->pElems.noFd[ptxcf->ReadPtr]);
                    if (pfe->flag) {  // There are at least one element inside the FIFO
                        if (csc->channels[Channel].queue_write_can (csc, Channel,
                                                                    pfe->id,
                                                                    pfe->data,
                                                                    pfe->ext,
                                                                    pfe->size) != -1) {
                            for (x2 = 0; x2 < MAX_CAN_FIFO_COUNT; x2++) {
                                if (x2 != x) {
                                    pcf2 = CanFifoNodes[x2];
                                    if (pcf2 != NULL) {
                                        if (CheckAcceptMask (pcf2->CanAcceptMasks, pfe->id, Channel)) {
                                        #ifdef REMOTE_MASTER
                                            pfe->timestamp = (uint64_t)get_rt_cycle_counter(); // << 32;
                                                            // (unsigned __int64)GetT4Master () << 16 |
                                                            // Timestamp;
                                        #else
                                            pfe->timestamp = (uint64_t)blackboard_infos.ActualCycleNumber << 32;
                                        #endif

                                            prxcf = &(pcf2->RxQueue);
                                            if (!pcf2->CanFdFlag) {
                                                // CAN -> CAN
                                                IF_NO_SPACE_WAIT_A_LITTLE_BIT (prxcf->pElems.noFd[prxcf->WritePtr].flag,
                                                                               pcf->OverFlowBehaviour);
                                                if (!prxcf->pElems.noFd[prxcf->WritePtr].flag) {  // There are at least one element inside the FIFO leer
                                                    prxcf->pElems.noFd[prxcf->WritePtr] = *pfe;
                                                    // will be transmitted oneself
                                                    prxcf->pElems.noFd[prxcf->WritePtr].node = 1;
                                                    wrptr_old = prxcf->WritePtr;
                                                    prxcf->WritePtr++;
                                                    if (prxcf->WritePtr >= prxcf->Depth) prxcf->WritePtr = 0;
                                                    prxcf->ElemCount++;
                                                    prxcf->pElems.noFd[wrptr_old].flag = 1;
                                                }
                                            } else {
                                                // CAN -> CAN-FD
                                                IF_NO_SPACE_WAIT_A_LITTLE_BIT (prxcf->pElems.Fd[prxcf->WritePtr].flag,
                                                                               pcf->OverFlowBehaviour);
                                                if (!prxcf->pElems.Fd[prxcf->WritePtr].flag) {  // There are at least one element inside the FIFO leer
                                                    prxcf->pElems.Fd[prxcf->WritePtr].channel = pfe->channel;
                                                    MEMCPY(prxcf->pElems.Fd[prxcf->WritePtr].data, pfe->data, 8);
                                                    prxcf->pElems.Fd[prxcf->WritePtr].ext = pfe->ext;
                                                    prxcf->pElems.Fd[prxcf->WritePtr].id = pfe->id;
                                                    prxcf->pElems.Fd[prxcf->WritePtr].size = pfe->size;
                                                    prxcf->pElems.Fd[prxcf->WritePtr].timestamp = pfe->timestamp;
                                                    // will be transmitted oneself
                                                    prxcf->pElems.Fd[prxcf->WritePtr].node = 1;
                                                    wrptr_old = prxcf->WritePtr;
                                                    prxcf->WritePtr++;
                                                    if (prxcf->WritePtr >= prxcf->Depth) prxcf->WritePtr = 0;
                                                    prxcf->ElemCount++;
                                                    prxcf->pElems.Fd[wrptr_old].flag = 1;
                                                }

                                            }
                                        }
                                    } else break;
                                }
                            }

                            rdptr_old = ptxcf->ReadPtr;
                            ptxcf->ReadPtr++;
                            if (ptxcf->ReadPtr >= ptxcf->Depth) ptxcf->ReadPtr = 0;
                            ptxcf->ElemCount--;
                            ptxcf->pElems.noFd[rdptr_old].flag = 0;
                        } else {
                            break;    // while(1)
                        }
                    } else break;     // while(1)
                }
            }
        } else break;
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return 0;
}

void InitCANFifoCriticalSection(void)
{
#ifdef REMOTE_MASTER
	RemoteMasterInitLock(&CANFifoGlobalSpinlock);
#else
	InitializeCriticalSection(&CANFifoGlobalSpinlock);
#endif

}
