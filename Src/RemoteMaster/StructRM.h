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

typedef struct {
    uint32_t SizeOf;
    uint32_t Command;
    uint32_t ThreadId;
    uint16_t ChannelNumber;
    uint16_t PackageCounter;
    //uint32_t Fill1;
} RM_PACKAGE_HEADER;

#define RM_INIT_CMD  (1)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int64_t PreAllocMemorySize;
    int32_t PidForSchedulersHelperProcess;
    uint32_t OffsetConfigurablePrefix[32];
    // now the data folowing
}  RM_INIT_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t Version;
    int32_t PatchVersion;  // negative are pre releases
    int32_t Ret;
}  RM_INIT_ACK;

#define RM_TERMINATE_CMD  (2)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
}  RM_TERMINATE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_TERMINATE_ACK;

#define RM_KILL_CMD  (3)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
}  RM_KILL_REQ;

#define RM_KILL_THREAD_CMD  (0)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
}  RM_KILL_THREAD_REQ;

// Sonstiges
#define RM_PING_CMD  (4)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Value;
}  RM_PING_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_PING_ACK;


#define RM_COPY_FILE_START_CMD     (5)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t OffsetFileName;
    // ... the target file name will follow
} RM_COPY_FILE_START_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t FileHandle;
    uint32_t OffsetErrorString;
    int32_t Ret;
    // ... optional a error message can follow
} RM_COPY_FILE_START_ACK;

#define RM_COPY_FILE_NEXT_CMD      (6)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t FileHandle;
    uint32_t BlockSize;
    uint32_t OffsetData;
    // ... followed by datas
} RM_COPY_FILE_NEXT_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t OffsetErrorString;
    int32_t Ret;
    // ... optional a error message can follow
} RM_COPY_FILE_NEXT_ACK;

#define RM_COPY_FILE_END_CMD       (7)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t FileHandle;
} RM_COPY_FILE_END_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t OffsetErrorString;
    int32_t Ret;
    // ... optional a error message can follow
} RM_COPY_FILE_END_ACK;

#define RM_READ_DATA_BYTES_CMD       (8)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    uint32_t Size;
    uint64_t Address;
} RM_READ_DATA_BYTES_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t OffsetData;
    int32_t Ret;
    // ... followed by readed data bytes
} RM_READ_DATA_BYTES_ACK;

#define RM_WRITE_DATA_BYTES_CMD       (9)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    uint32_t Size;
    uint64_t Address;
    uint32_t OffsetData;
    // ... followed by readed data bytes
} RM_WRITE_DATA_BYTES_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
} RM_WRITE_DATA_BYTES_ACK;

#define RM_REFERENCE_VARIABLE_CMD       (10)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    uint64_t Address;
    uint32_t OffsetName;
    int32_t Type;
    int32_t Dir;
    // ... danach kommen die gelesenen Daten Bytes
} RM_REFERENCE_VARIABLE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
} RM_REFERENCE_VARIABLE_ACK;

#define RM_DEREFERENCE_VARIABLE_CMD       (11)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    uint64_t Address;
    uint32_t OffsetName;
    int32_t Type;
    // ... danach kommen die gelesenen Daten Bytes
} RM_DEREFERENCE_VARIABLE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
} RM_DEREFERENCE_VARIABLE_ACK;

// The following messages will be transmitted through FiFo to the RemoteControlProcess (without ACK)

#define RM_ADD_SCRIPT_MESSAGE_CMD  (10)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t OffsetText;
    // ... followed by text string
}  RM_ADD_SCRIPT_MESSAGE_REQ;

#define RM_ERROR_MESSAGE_CMD  (11)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint64_t Cycle;
    int32_t Level;
    int32_t Pid;
    uint32_t OffsetText;
    // ... followed by text string
}  RM_ERROR_MESSAGE_REQ;

#define RM_REALTIME_PROCESS_STARTED_CMD  (12)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    uint32_t OffsetProcessName;
    // ... followed by process name
}  RM_REALTIME_PROCESS_STARTED_REQ;

#define RM_REALTIME_PROCESS_STOPPED_CMD  (13)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    uint32_t OffsetProcessName;
    // ... followed by process name
}  RM_REALTIME_PROCESS_STOPPED_REQ;
