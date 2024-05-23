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

#include "Message.h"
#include "Blackboard.h"
#include "StructRM.h"

#define RM_MESSAGE_OFFSET  150

#define RM_MESSAGE_WRITE_MESSAGE_TS_AS_CMD  (RM_MESSAGE_OFFSET+0)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t pid;
    int32_t mid;
    uint64_t timestamp;
    int32_t msize;
    uint32_t Offset_mblock;
    int32_t tx_pid;
    // ... danach kommt der m_block Inhalt
}  RM_MESSAGE_WRITE_MESSAGE_TS_AS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_MESSAGE_WRITE_MESSAGE_TS_AS_ACK;

#define RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_CMD  (RM_MESSAGE_OFFSET+1)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t pid;
    int32_t mid;
    uint64_t timestamp;
    int32_t msize;
    uint32_t Offset_mblock;
    int32_t tx_pid;
    // ... danach kommt der m_block Inhalt
}  RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_ACK;

#define RM_MESSAGE_TEST_MESSAGE_AS_CMD  (RM_MESSAGE_OFFSET+2)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t pid;
}  RM_MESSAGE_TEST_MESSAGE_AS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    MESSAGE_HEAD ret_mhead;
    int32_t Ret;
}  RM_MESSAGE_TEST_MESSAGE_AS_ACK;

#define RM_MESSAGE_READ_MESSAGE_AS_CMD  (RM_MESSAGE_OFFSET+3)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t pid;
    int32_t maxlen;
}  RM_MESSAGE_READ_MESSAGE_AS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    MESSAGE_HEAD ret_mhead;
    int32_t Ret;
    // ... danach kommt der m_block Inhalt
}  RM_MESSAGE_READ_MESSAGE_AS_ACK;

#define RM_MESSAGE_REMOVE_MESSAGE_AS_CMD  (RM_MESSAGE_OFFSET+4)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t pid;
}  RM_MESSAGE_REMOVE_MESSAGE_AS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_MESSAGE_REMOVE_MESSAGE_AS_ACK;
