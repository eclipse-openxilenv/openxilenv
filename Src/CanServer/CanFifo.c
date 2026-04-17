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
    int size;              /* Size of the message buffer */
    int rp;                /* Current read position */
    int wp;                /* Current write position */
    int count;             /* Number of messages inside the queue */
    char *buffer;          /* Pointer to the memory storing the messages */
} CAN_FIFO;

typedef struct {
    int AssignedHandle;  // Consists a 8bit wide index (0...9) and a 16bit counter this will be incremented by one for each new fifo
    CAN_FIFO RxQueue;
    CAN_ACCEPT_MASK *CanAcceptMasks;
    //int Channel;
    int CanFdFlag;
    int OverFlowBehaviour;  // 0 -> lose the oldest message immediately,
                            // 1...32767 -> wait the number of 10ms than lose the oldest message
                            // -1 -> wait endless no message will be lost
} CAN_FIFO_NODE;

static CAN_FIFO_NODE *CanFifoNodes[MAX_CAN_FIFO_COUNT];
static CAN_FIFO TxQueue[MAX_CAN_CHANNELS];

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
    Depth *= 32;

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
    if ((pcf->RxQueue.buffer = (char*)my_calloc ((size_t)Depth, sizeof (char))) == NULL) {
        my_free (pcf);
        goto __OUT;
    }
    pcf->RxQueue.size = Depth;
    pcf->RxQueue.count = 0;
    pcf->RxQueue.rp = pcf->RxQueue.wp = 0;

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
                my_free (pcf->RxQueue.buffer);
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
static int CheckAcceptMask (CAN_ACCEPT_MASK *pMask, uint32_t Id, uint8_t Ext, int Channel)
{
    int x;

    if (pMask == NULL) return 0;
    for (x = 0; pMask[x].Channel >= 0; x++) {
        if ((Channel == pMask[x].Channel) &&
            (Id >= pMask[x].Start) &&
            (Id <= pMask[x].Stop)) {
            // now check the flags:
            // if ext = 0 -> normal 11 bit identifier
            // if ext = 1 -> extended 19 bit identifier
            // if ext = 2 -> FD + normal 11 bit identifier
            // if ext = 3 -> FD + extended 19 bit identifier
            // if ext = 6 -> FD + normal 11 bit identifier + bit rate switch
            // if ext = 7 -> FD + extended 19 bit identifier + bit rate switch
            // if ext = 0x10 -> J1939MP
            // if Flags == 0 it should be accepted all except J1939MP
            if (((~(uint8_t)pMask[x].Flags & Ext & (uint8_t)0xF) != 0) ||
                 (((pMask[x].Flags & 0x100) == 0) && (Ext == 0)) ||    // are normal 11 bit identifier not switched off?
                 (((pMask[x].Flags & 0x200) != 0) && (Ext == 0x10))) { // are J1939MP switched on?
                return 1;
            }
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

static void ReadFromFifo(char *dst,  CAN_FIFO *fifo, int size)
{
    char *src;
    int parta, partb;

    src = (char*)&fifo->buffer[fifo->rp];

    parta = fifo->size - fifo->rp;
    partb = size - parta;

    if (parta >= size) {
        MEMCPY (dst, src, size);
        fifo->rp += size;
    } else {
        MEMCPY (dst, src, (size_t)parta);
        MEMCPY (&dst[parta], fifo->buffer, (size_t)partb);
        fifo->rp = partb;
    }
}

static int PeakFromFifo(char *dst,  CAN_FIFO *fifo, int size)
{
    char *src;
    int parta, partb;
    int ret;

    src = (char*)&fifo->buffer[fifo->rp];

    parta = fifo->size - fifo->rp;
    partb = size - parta;

    if (parta >= size) {
        MEMCPY (dst, src, size);
        ret = fifo->rp + size;
    } else {
        MEMCPY (dst, src, (size_t)parta);
        MEMCPY (&dst[parta], fifo->buffer, (size_t)partb);
        ret = partb;
    }
    return ret;
}

static void SetReadPosFifo(CAN_FIFO *fifo, int pos)
{
    fifo->rp = pos;
}

static void RemoveFromFifo(CAN_FIFO *fifo, int size)
{
    int parta, partb;

    parta = fifo->size - fifo->rp;
    partb = size - parta;

    if (parta >= size) {
        fifo->rp += size;
    } else {
        fifo->rp = partb;
    }
}

static void RemoveOneMessageFromFifo(CAN_FIFO *fifo)
{
    CAN_FIFO_ELEM_HEADER Head;
    ReadFromFifo((char*)&Head, fifo, sizeof(Head));
    RemoveFromFifo(fifo, Head.size);
    fifo->count--;
}

static int CalculateFreeSpace(CAN_FIFO *fifo)
{
    int free_space;
    if (fifo->rp > fifo->wp) {
        free_space = fifo->rp - fifo->wp;
    } else {
        free_space = fifo->size - (fifo->wp - fifo->rp);
    }
    return free_space;
}

static int FiFoIsEmpty(CAN_FIFO *fifo)
{
    return (fifo->rp == fifo->wp);
}

static void AddMessagetoFifo(CAN_FIFO *fifo, CAN_FIFO_ELEM_HEADER *Head, unsigned char *Data, int Size)
{
    char *src;
    char *dst;
    int parta, partb;

    dst = (char*)&fifo->buffer[fifo->wp];
    src = (char*)Head;
    parta = fifo->size - fifo->wp;
    partb = (int)sizeof (CAN_FIFO_ELEM_HEADER) - parta;

    if (parta >= (int)sizeof (CAN_FIFO_ELEM_HEADER)) {
        MEMCPY (dst, src, sizeof (CAN_FIFO_ELEM_HEADER));
        fifo->wp += sizeof (CAN_FIFO_ELEM_HEADER);
    } else {
        MEMCPY (dst, src, (size_t)parta);
        MEMCPY (fifo->buffer, &src[parta], (size_t)partb);
        fifo->wp = partb;
    }

    /* Than the data */
    dst = (char*)&fifo->buffer[fifo->wp];
    parta = fifo->size - fifo->wp;
    partb = Size - parta;

    if (parta >= Size) {
        MEMCPY (dst, Data, (size_t)Size);
        fifo->wp += Size;
    } else {
        MEMCPY (dst, Data, (size_t)parta);
        MEMCPY (fifo->buffer, &Data[parta], (size_t)partb);
        fifo->wp = partb;
    }
    fifo->count++;
}

// CAN->FIFOs
int WriteCanMessageFromBus2Fifos (int Channel, uint32_t Id,
                                  unsigned char *Data, unsigned char Ext,
                                  int Size, unsigned char Node,
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
            if (CheckAcceptMask (pcf->CanAcceptMasks, Id, Ext, Channel)) {
                int need_space;
                int free_space;
                CAN_FIFO_ELEM_HEADER Head;

                Head.id = Id;
                Head.size = Size;
                Head.ext = Ext;
                Head.channel = (unsigned char)Channel;
                Head.flag = 0;
                Head.node = Node;
            #ifdef REMOTE_MASTER
                Head.timestamp = Timestamp;
            #else
                Head.timestamp = Timestamp;
            #endif
                prxcf = &(pcf->RxQueue);

                /* Check if there is enough space for this message */
                need_space = Size + (int)sizeof(CAN_FIFO_ELEM_HEADER);
                if (need_space > prxcf->size) {
                    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                    return -1;
                }
                free_space = CalculateFreeSpace(prxcf);
                IF_NO_SPACE_WAIT_A_LITTLE_BIT ((need_space >= free_space),
                                               pcf->OverFlowBehaviour);
                // recalculate the free space it can be changed
                free_space = CalculateFreeSpace(prxcf);
                // remove the oldest message till we have enough space that we can store the new message
                while (need_space >= free_space) {
                    RemoveOneMessageFromFifo(prxcf);
                    free_space = CalculateFreeSpace(prxcf);
                }
                AddMessagetoFifo(prxcf, &Head, Data, Size);
            } else {
                break;  // for(;;)
            }
        }
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return 0;
}

// FIFO->Process (CAN, no CAN FD/XL
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
            if (pcf->AssignedHandle == Handle) {
                prxcf = &(pcf->RxQueue);
                while (!FiFoIsEmpty(prxcf)) {
                    CAN_FIFO_ELEM_HEADER Head;
                    ReadFromFifo((char*)&Head, prxcf, sizeof(Head));
                    if ((Head.size > 8) || ((Head.ext & 0x10) == 0x10)) {
                        // it is a CAN FD/XL or J1939MP message -> ignore this message
                        RemoveFromFifo(prxcf, Head.size);
                        prxcf->count--;
                    } else {
                        pCanMessage->channel = Head.channel;
                        pCanMessage->ext = Head.ext;
                        pCanMessage->id = Head.id;
                        pCanMessage->node = Head.node;
                        pCanMessage->size = Head.size;
                        pCanMessage->timestamp = Head.timestamp;
                        ReadFromFifo((char*)&pCanMessage->data[0], prxcf, Head.size);
                        prxcf->count--;
                        x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                        return 1;
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
            if (pcf->AssignedHandle == Handle) {
                prxcf = &(pcf->RxQueue);
                while (!FiFoIsEmpty(prxcf)) {
                    CAN_FIFO_ELEM_HEADER Head;
                    ReadFromFifo((char*)&Head, prxcf, sizeof(Head));
                    if ((Head.size > 64) || ((Head.ext & 0x10) == 0x10)) {
                        // it is a CAN XL or J1939MP message -> ignore this message
                        RemoveFromFifo(prxcf, Head.size);
                        prxcf->count--;
                    } else {
                        pCanMessage->channel = Head.channel;
                        pCanMessage->ext = Head.ext;
                        pCanMessage->id = Head.id;
                        pCanMessage->node = Head.node;
                        pCanMessage->size = Head.size;
                        pCanMessage->timestamp = Head.timestamp;
                        ReadFromFifo((char*)&pCanMessage->data[0], prxcf, Head.size);
                        prxcf->count--;
                        x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                        return 1;
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
            if (pcf->AssignedHandle == Handle) {
                prxcf = &(pcf->RxQueue);
                while (!FiFoIsEmpty(prxcf) && (CopyMessageCounter < MaxMessages)) {
                    CAN_FIFO_ELEM_HEADER Head;
                    ReadFromFifo((char*)&Head, prxcf, sizeof(Head));
                    if ((Head.size > 8) || ((Head.ext & 0x10) == 0x10)) {
                        // it is a CAN FD/XL J1939MP message -> ignore this message
                        RemoveFromFifo(prxcf, Head.size);
                        prxcf->count--;
                    } else {
                        pCanMessage[CopyMessageCounter].channel = Head.channel;
                        pCanMessage[CopyMessageCounter].ext = Head.ext;
                        pCanMessage[CopyMessageCounter].id = Head.id;
                        pCanMessage[CopyMessageCounter].node = Head.node;
                        pCanMessage[CopyMessageCounter].size = Head.size;
                        pCanMessage[CopyMessageCounter].timestamp = Head.timestamp;
                        ReadFromFifo((char*)&pCanMessage[CopyMessageCounter].data[0], prxcf, Head.size);
                        CopyMessageCounter++;
                        prxcf->count--;
                    }
                }
                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                return CopyMessageCounter;
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
            if (pcf->AssignedHandle == Handle) {
                prxcf = &(pcf->RxQueue);
                while (!FiFoIsEmpty(prxcf) && (CopyMessageCounter < MaxMessages)) {
                    CAN_FIFO_ELEM_HEADER Head;
                    ReadFromFifo((char*)&Head, prxcf, sizeof(Head));
                    if ((Head.size > 64) || ((Head.ext & 0x10) == 0x10)) {
                        // it is a CAN XL or J1939MP message -> ignore this message
                        RemoveFromFifo(prxcf, Head.size);
                        prxcf->count--;
                    } else {
                        pCanMessage[CopyMessageCounter].channel = Head.channel;
                        pCanMessage[CopyMessageCounter].ext = Head.ext;
                        pCanMessage[CopyMessageCounter].id = Head.id;
                        pCanMessage[CopyMessageCounter].node = Head.node;
                        pCanMessage[CopyMessageCounter].size = Head.size;
                        pCanMessage[CopyMessageCounter].timestamp = Head.timestamp;
                        ReadFromFifo((char*)&pCanMessage[CopyMessageCounter].data[0], prxcf, Head.size);
                        CopyMessageCounter++;
                        prxcf->count--;
                    }
                }
                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                return CopyMessageCounter;
            }
        }
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return -1;
}

int ReadCanFlexMessagesFromFifo2Process (int Handle, CAN_FIFO_ELEM_HEADER *pCanMessage, int MaxMessages,
                                         uint8_t **Data, uint8_t *Buffer, int BufferSize)
{
    int x, rdptr_old;
    CAN_FIFO_NODE *pcf;
    CAN_FIFO *prxcf;
    int CopyMessageCounter = 0;
    int BufferPos = 0;

#ifdef REMOTE_MASTER
    if (Handle == DEFAULT_PROCESS_PARAMETER) {
        Handle = LagacyCanFifoGlobalHandle;
    }
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
       // TODO:
       //return rm_ReadCanFdMessagesFromFifo2Process (Handle, pCanMessage, MaxMessages);
    }
#endif

    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
    for (x = 0; x < MAX_CAN_FIFO_COUNT; x++) {
        pcf = CanFifoNodes[x];
        if (pcf != NULL) {
            if (pcf->AssignedHandle == Handle) {
                prxcf = &(pcf->RxQueue);
                while (!FiFoIsEmpty(prxcf) && (CopyMessageCounter < MaxMessages)) {
                    CAN_FIFO_ELEM_HEADER Head;
                    int ReadPos = PeakFromFifo((char*)&Head, prxcf, sizeof(Head));
                    if (Head.size > (BufferSize - BufferPos)) {
                        break;  // while()
                    }
                    SetReadPosFifo(prxcf, ReadPos);

                    pCanMessage[CopyMessageCounter].channel = Head.channel;
                    pCanMessage[CopyMessageCounter].ext = Head.ext;
                    pCanMessage[CopyMessageCounter].id = Head.id;
                    pCanMessage[CopyMessageCounter].node = Head.node;
                    pCanMessage[CopyMessageCounter].size = Head.size;
                    pCanMessage[CopyMessageCounter].timestamp = Head.timestamp;
                    Data[CopyMessageCounter] = Buffer + BufferPos;
                    ReadFromFifo((char*)Data[CopyMessageCounter], prxcf, Head.size);
                    CopyMessageCounter++;
                    BufferPos += Head.size;
                    prxcf->count--;
                }
                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                return CopyMessageCounter;
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
                    prxcf->rp = prxcf->wp = 0;
                }
                if ((Flags & 0x00000002) == 0x00000002) {
                    for (y = 0; y < MAX_CAN_CHANNELS; y++) {
                        ptxcf = &(TxQueue[y]);
                        ptxcf->rp = 0;
                        ptxcf->wp = 0;
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
    return ((WriteCanMessagesFromProcess2Fifo (Handle, pCanMessage, 1) == 1) ? 0 : -1);
}

int WriteCanFdMessageFromProcess2Fifo (int Handle, CAN_FD_FIFO_ELEM *pCanMessage)
{
    return ((WriteCanFdMessagesFromProcess2Fifo (Handle, pCanMessage, 1) == 1) ? 0 : -1);
}

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
                while (CopyMessageCounter < MaxMessages) {
                    CAN_FIFO_ELEM_HEADER Head;
                    int need_space, free_space;
                    if (pCanMessage->channel < MAX_CAN_CHANNELS) {
                        ptxcf = &(TxQueue[pCanMessage->channel]);
                        if (ptxcf->buffer != NULL) {
                            /* Check if there is enough space for this message */
                            need_space = pCanMessage->size + (int)sizeof(CAN_FIFO_ELEM_HEADER);
                            if (need_space > ptxcf->size) {
                                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                                return -1;
                            }
                            free_space = CalculateFreeSpace(ptxcf);
                            IF_NO_SPACE_WAIT_A_LITTLE_BIT ((need_space >= free_space),
                                                           pcf->OverFlowBehaviour);
                            // recalculate the free space it can be changed
                            free_space = CalculateFreeSpace(ptxcf);
                            // remove the oldest message till we have enough space that we can store the new message
                            while (need_space >= free_space) {
                                RemoveOneMessageFromFifo(ptxcf);
                                free_space = CalculateFreeSpace(ptxcf);
                            }
                            Head.channel = pCanMessage[CopyMessageCounter].channel;
                            Head.ext = pCanMessage[CopyMessageCounter].ext;
                            Head.flag = 0;
                            Head.id = pCanMessage[CopyMessageCounter].id;
                            Head.node = 1; // 1 -> oneself pCanMessage[CopyMessageCounter].node;
                            Head.size = pCanMessage[CopyMessageCounter].size;
                            Head.timestamp = pCanMessage[CopyMessageCounter].timestamp;
                            AddMessagetoFifo(ptxcf, &Head, pCanMessage[CopyMessageCounter].data, pCanMessage[CopyMessageCounter].size);
                        }
                    }
                    CopyMessageCounter++;
                }
                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                return CopyMessageCounter;  // Number of written messages
            }
        } else {
            // End of list
            break;
        }
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
                while (CopyMessageCounter < MaxMessages) {
                    CAN_FIFO_ELEM_HEADER Head;
                    int need_space, free_space;
                    if (pCanMessage->channel < MAX_CAN_CHANNELS) {
                        ptxcf = &(TxQueue[pCanMessage->channel]);
                        if (ptxcf->buffer != NULL) {
                            /* Check if there is enough space for this message */
                            need_space = pCanMessage->size + (int)sizeof(CAN_FIFO_ELEM_HEADER);
                            if (need_space > ptxcf->size) {
                                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                                return -1;
                            }
                            free_space = CalculateFreeSpace(ptxcf);
                            IF_NO_SPACE_WAIT_A_LITTLE_BIT ((need_space >= free_space),
                                                           pcf->OverFlowBehaviour);
                            // recalculate the free space it can be changed
                            free_space = CalculateFreeSpace(ptxcf);
                            // remove the oldest message till we have enough space that we can store the new message
                            while (need_space >= free_space) {
                                RemoveOneMessageFromFifo(ptxcf);
                                free_space = CalculateFreeSpace(ptxcf);
                            }
                            Head.channel = pCanMessage[CopyMessageCounter].channel;
                            Head.ext = pCanMessage[CopyMessageCounter].ext;
                            Head.flag = 0;
                            Head.id = pCanMessage[CopyMessageCounter].id;
                            Head.node = 1; // 1 -> oneself pCanMessage[CopyMessageCounter].node;
                            Head.size = pCanMessage[CopyMessageCounter].size;
                            Head.timestamp = pCanMessage[CopyMessageCounter].timestamp;
                            AddMessagetoFifo(ptxcf, &Head, pCanMessage[CopyMessageCounter].data, pCanMessage[CopyMessageCounter].size);
                        }
                    }
                    CopyMessageCounter++;
                }
                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                return CopyMessageCounter;  // Number of written messages
            }
        } else {
            // End of list
            break;
        }
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
    for (x = 0; x < csc->channel_count; x++) {
        ptxcf = &(TxQueue[Channel]);
        while (!FiFoIsEmpty(ptxcf)) {
            CAN_FIFO_ELEM_HEADER Head;
            char *Data;
            ReadFromFifo((char*)&Head, ptxcf, sizeof(Head));
            Data = alloca(Head.size);
            ReadFromFifo(Data, ptxcf, Head.size);
            ptxcf->count--;
            if (csc->channels[Channel].queue_write_can (csc, Channel,
                                                        Head.id,
                                                        (unsigned char*)Data,
                                                        Head.ext,
                                                        Head.size) != -1) {
                for (x2 = 0; x2 < MAX_CAN_FIFO_COUNT; x2++) {
                    //if (x2 != x) {
                    pcf2 = CanFifoNodes[x2];
                    if (pcf2 != NULL) {
                        if (CheckAcceptMask (pcf2->CanAcceptMasks, Head.id, Head.ext, Channel)) {
                            int need_space;
                            int free_space;
                        #ifdef REMOTE_MASTER
                            Head.timestamp = (uint64_t)get_rt_cycle_counter(); // << 32;
                        #else
                            Head.timestamp = (uint64_t)blackboard_infos.ActualCycleNumber << 32;
                        #endif
                            prxcf = &(pcf2->RxQueue);
                            /* Check if there is enough space for this message */
                            need_space = Head.size + (int)sizeof(CAN_FIFO_ELEM_HEADER);
                            if (need_space > prxcf->size) {
                                x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
                                return -1;
                            }
                            free_space = CalculateFreeSpace(prxcf);
                            IF_NO_SPACE_WAIT_A_LITTLE_BIT ((need_space >= free_space),
                                                           pcf2->OverFlowBehaviour);
                            // recalculate the free space it can be changed
                            free_space = CalculateFreeSpace(prxcf);
                            // remove the oldest message till we have enough space that we can store the new message
                            while (need_space >= free_space) {
                                RemoveOneMessageFromFifo(prxcf);
                                free_space = CalculateFreeSpace(prxcf);
                            }
                            AddMessagetoFifo(prxcf, &Head, (unsigned char*)Data, Head.size);
                        }
                    }
                }
           }
        }
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
    return 0;
}

void SetupCanTxFiFos(NEW_CAN_SERVER_CONFIG *csc)
{
    int x;
    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
    for (x = 0; x < csc->channel_count; x++) {
        TxQueue[x].rp = TxQueue[x].wp = 0;
        if (TxQueue[x].buffer == NULL) {
            TxQueue[x].size = 64*1024;
            TxQueue[x].buffer = my_malloc(TxQueue[x].size);
        }
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
}

void CleanupCanTxFiFos(void)
{
    int x;
    x__AcquireSpinlock (&CANFifoGlobalSpinlock, __LINE__, __FILE__);
    for (x = 0; x < MAX_CAN_CHANNELS; x++) {
        TxQueue[x].size = 0;
        TxQueue[x].rp = TxQueue[x].wp = 0;
        if (TxQueue[x].buffer != NULL) my_free(TxQueue[x].buffer);
        TxQueue[x].buffer = NULL;
    }
    x__ReleaseSpinlock (&CANFifoGlobalSpinlock);
}

void InitCANFifoCriticalSection(void)
{
#ifdef REMOTE_MASTER
	RemoteMasterInitLock(&CANFifoGlobalSpinlock);
#else
	InitializeCriticalSection(&CANFifoGlobalSpinlock);
#endif

}

