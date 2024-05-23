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

#include "Fifos.h"
#include "StructRM.h"

#define RM_FIFO_OFFSET  250

#define RM_REGISTER_NEW_RX_FIFO_NAME_CMD  (RM_FIFO_OFFSET+0)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t RxPid;
    uint32_t Offset_Name;
    // ... danach kommt der Name
}  RM_REGISTER_NEW_RX_FIFO_NAME_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_REGISTER_NEW_RX_FIFO_NAME_ACK;

#define RM_UNREGISTER_RX_FIFO_CMD  (RM_FIFO_OFFSET+1)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t FiFoId;
    uint32_t RxPid;
}  RM_UNREGISTER_RX_FIFO_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_UNREGISTER_RX_FIFO_ACK;

#define RM_RX_ATTACH_FIFO_CMD  (RM_FIFO_OFFSET+2)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t RxPid;
    uint32_t Offset_Name;
    // ... danach kommt der Name
}  RM_RX_ATTACH_FIFO_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_RX_ATTACH_FIFO_ACK;

#define RM_SYNC_FIFOS_CMD  (RM_FIFO_OFFSET+3)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t Len;
    uint32_t Offset_Buffer;
    // ... danach kommen die Daten
}  RM_SYNC_FIFOS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Len;
    uint32_t Offset_Buffer;
    int32_t Ret;
    // ... danach kommen die Daten
}  RM_SYNC_FIFOS_ACK;

