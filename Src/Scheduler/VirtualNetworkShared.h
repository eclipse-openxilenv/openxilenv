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
#include "my_stdint.h"

#define VIRTUAL_NETWORK_TYPE_CAN        1
#define VIRTUAL_NETWORK_TYPE_CANFD      2
#define VIRTUAL_NETWORK_TYPE_FLEXRAY    3
#define VIRTUAL_NETWORK_TYPE_ETHERNET   4

#define VIRTUAL_NETWORK_ERR_OUT_OF_MEMORY     -1101
#define VIRTUAL_NETWORK_ERR_INVALID_PARAMETER -1102
#define VIRTUAL_NETWORK_ERR_UNKNOWN_HANDLE    -1103
#define VIRTUAL_NETWORK_ERR_WRONG_THREAD      -1104
#define VIRTUAL_NETWORK_ERR_NOT_ENOUGH_SPACE  -1105
#define VIRTUAL_NETWORK_ERR_MESSAGE_TO_BIG    -1106
#define VIRTUAL_NETWORK_ERR_NO_DATA           -1107
#define VIRTUAL_NETWORK_ERR_NO_FREE_FIFO      -1108

#define ALIGN_BY(x, a) (((x) + (a - 1)) & ~(a - 1))
#define ALIGN(x) ALIGN_BY(x, 4)

#if defined(_WIN32) || defined(__linux__)
#pragma pack(push,1)
#endif

typedef struct {
    uint32_t Id;
    uint8_t Flags;
    uint8_t Data[1];  // 1...8
} VIRTUAL_NETWORK_CAN_PACKAGE;

typedef struct {
    uint32_t Id;
    uint8_t Flags;
    uint8_t Data[1]; // 1...64
} VIRTUAL_NETWORK_CANFD_PACKAGE;

typedef struct {
    uint16_t Slot;
    uint8_t Frame;
    uint8_t Flags;
    uint8_t Data[1];   // 1...256
} VIRTUAL_NETWORK_FLEXRAY_PACKAGE;

typedef struct {
    uint8_t Data[1];  // 1...?
} VIRTUAL_NETWORK_ETH_PACKAGE;


typedef struct {
    uint32_t Size;  // 0 is empty
    uint8_t Type;
    uint8_t Channel;
    uint16_t Internal;
    uint64_t Timestamp;
  union {
        VIRTUAL_NETWORK_CAN_PACKAGE Can;
        VIRTUAL_NETWORK_CANFD_PACKAGE CanFd;
        VIRTUAL_NETWORK_FLEXRAY_PACKAGE Flexray;
        VIRTUAL_NETWORK_ETH_PACKAGE Eth;

  } Data;
} VIRTUAL_NETWORK_PACKAGE;

typedef struct {
    uint32_t Size;  
    uint32_t IdSlot;
    uint8_t Flags;
    uint8_t NewData;
    uint8_t DataLost;
    uint8_t Data[1];   // 1...256
} VIRTUAL_NETWORK_BUFFER;

#if defined(_WIN32) || defined(__linux__)
#pragma pack(pop)
#endif
