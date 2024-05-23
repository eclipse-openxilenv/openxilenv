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
#include <stdint.h>

#include "CanFifo.h"
#include "StructRM.h"

#define RM_CAN_FIFO_OFFSET  200

#define RM_CAN_FIFO_CREATE_CAN_FIFOS_CMD  (RM_CAN_FIFO_OFFSET+0)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Depth;
    int32_t FdFlag;
}  RM_CAN_FIFO_CREATE_CAN_FIFOS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_CAN_FIFO_CREATE_CAN_FIFOS_ACK;

#define RM_CAN_FIFO_DELETE_CAN_FIFOS_CMD  (RM_CAN_FIFO_OFFSET+1)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Handle;
}  RM_CAN_FIFO_DELETE_CAN_FIFOS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_CAN_FIFO_DELETE_CAN_FIFOS_ACK;

#define RM_CAN_FIFO_FLUSH_CAN_FIFO_CMD  (RM_CAN_FIFO_OFFSET+2)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Handle;
    int32_t Flags;
}  RM_CAN_FIFO_FLUSH_CAN_FIFO_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_CAN_FIFO_FLUSH_CAN_FIFO_ACK;

#define RM_CAN_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_CMD  (RM_CAN_FIFO_OFFSET+3)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Handle;
}  RM_CAN_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    CAN_FIFO_ELEM ret_CanMessage;
    int32_t Ret;
}  RM_CAN_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_ACK;

#define RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_CMD  (RM_CAN_FIFO_OFFSET+4)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Handle;
    int32_t MaxMessages;
}  RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t Offset_CanMessage;
    int32_t Ret;
    // ... danach kommt der CanMessage Inhalt
}  RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK;

#define RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_CMD  (RM_CAN_FIFO_OFFSET+5)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Handle;
    uint32_t Offset_CanMessage;
    int32_t MaxMessages;
        // ... danach kommt der CanMessage Inhalt
}  RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_ACK;

#define RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_CMD  (RM_CAN_FIFO_OFFSET+6)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Handle;
    uint32_t Offset_cam;
    int32_t Size;
        // ... danach kommt der CAN_ACCEPT_MASK Inhalt
}  RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_ACK;

#define RM_CAN_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_CMD  (RM_CAN_FIFO_OFFSET+7)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Handle;
    CAN_FIFO_ELEM CanMessage;
}  RM_CAN_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_CAN_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_ACK;


#define RM_CAN_FD_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_CMD  (RM_CAN_FIFO_OFFSET+8)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Handle;
}  RM_CAN_FD_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    CAN_FD_FIFO_ELEM ret_CanMessage;
    int32_t Ret;
}  RM_CAN_FD_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_ACK;

#define RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_CMD  (RM_CAN_FIFO_OFFSET+9)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Handle;
    int32_t MaxMessages;
}  RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t Offset_CanMessage;
    int32_t Ret;
    // ... danach kommt der CanMessage Inhalt
}  RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK;

#define RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_CMD  (RM_CAN_FIFO_OFFSET+10)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Handle;
    uint32_t Offset_CanMessage;
    int32_t MaxMessages;
        // ... danach kommt der CanMessage Inhalt
}  RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_ACK;

#define RM_CAN_FD_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_CMD  (RM_CAN_FIFO_OFFSET+11)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Handle;
    CAN_FD_FIFO_ELEM CanMessage;
}  RM_CAN_FD_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_CAN_FD_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_ACK;

