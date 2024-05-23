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


#ifndef _CANFIFO_H
#define _CANFIFO_H

#include <stdint.h>

#include "ReadCanCfg.h"

#define MAX_CAN_FIFO_COUNT 16
#define MAX_CAN_ACCEPTANCE_MASK_ELEMENTS 16

// hint: same structure inside XilEnvRpc.h!!
typedef struct {
    uint32_t id;            // 4
    uint8_t size;           // 1
    uint8_t ext;            // 1
    uint8_t flag;           // 1
    uint8_t channel;        // 1
    uint8_t node;           // 1      1 -> oneself transmitted 0 -> external
    uint8_t fill[7];        // 7
    uint64_t timestamp;     // 8
    uint8_t data[8];        // 8
}  CAN_FIFO_ELEM;                  // 32 Byte

typedef struct {
    uint32_t id;            // 4
    uint8_t size;           // 1
    uint8_t ext;            // 1
    uint8_t flag;           // 1
    uint8_t channel;        // 1
    uint8_t node;           // 1      1 -> oneself transmitted 0 -> external
    uint8_t fill[7];        // 7
    uint64_t timestamp;     // 8
    uint8_t data[64];       // 64
}  CAN_FD_FIFO_ELEM;               // 88 Byte


typedef struct {
    int32_t Channel;
    uint32_t Start;
    uint32_t Stop;
    int32_t Fill1;
}  CAN_ACCEPT_MASK;

int CreateCanFifos (int Depth, int FdFlag);
int CreateCanFdFifos (int Depth);
int DeleteCanFifos (int Handle);

int FlushCanFifo (int Handle, int Flags);

// FIFO->Process

// Return value 1 -> one message was received from the fifo,
//             0 -> there are no message inside the fifo,
//            -1 -> not a valid fifo handle
int ReadCanMessageFromFifo2Process (int Handle, CAN_FIFO_ELEM *pCanMessage);
int ReadCanFdMessageFromFifo2Process (int Handle, CAN_FD_FIFO_ELEM *pCanMessage);

// Return value 1...n -> one or more message was received from the Fifo,
//             0 -> there are no message inside the fifo,
//            -1 -> not a valid fifo handle
int ReadCanMessagesFromFifo2Process (int Handle, CAN_FIFO_ELEM *pCanMessage, int MaxMessages);
int ReadCanFdMessagesFromFifo2Process (int Handle, CAN_FD_FIFO_ELEM *pCanMessage, int MaxMessages);

// FIFO->CAN
int WriteCanMessageFromFifo2Can (NEW_CAN_SERVER_CONFIG *csc, int Channel);


// Return value 1...n -> one or more message was transmitted to the Fifo,
//             0 -> no message was transmitted to the fifo
int WriteCanMessagesFromProcess2Fifo (int Handle, CAN_FIFO_ELEM *pCanMessage, int MaxMessages);
int WriteCanFdMessagesFromProcess2Fifo (int Handle, CAN_FD_FIFO_ELEM *pCanMessage, int MaxMessages);

int SetAcceptMask4CanFifo (int Handle, CAN_ACCEPT_MASK *cam, int Size);

// Return value 0 -> Message was succesfully transmitted to the fifo,
//            -1 -> not a valid fifo handle oder Fifo voll
int WriteCanMessageFromProcess2Fifo (int Handle, CAN_FIFO_ELEM *pCanMessage);
int WriteCanFdMessageFromProcess2Fifo (int Handle, CAN_FD_FIFO_ELEM *pCanMessage);

// CAN->FIFOs
int WriteCanMessageFromBus2Fifos (int Channel, uint32_t Id,
                                  unsigned char *Data, unsigned char Ext,
                                  unsigned char Size, unsigned char Node,
                                  uint64_t Timestamp);

void InitCANFifoCriticalSection(void);

#endif
