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


#ifndef PIPEMESSAGESSCHED_H
#define PIPEMESSAGESSCHED_H

#include "SharedDataTypes.h"

#define PIPE_NAME  "\\\\.\\pipe\\XilEnv"
#define KILL_ALL_EXTERN_PROCESS_EVENT  "Local\\XilEnvKillAllExternProcessEvent"

#define PIPE_MESSAGE_BUFSIZE (1024*1024)

#define EXTERN_PROCESS_COMUNICATION_PROTOCOL_VERSION    1012

#if defined _M_X64 || defined __linux__
#define ALIVE_PING_HANDLE_ULONG  uint32_t
#else
#define ALIVE_PING_HANDLE_ULONG  HANDLE
#endif

#if defined(_WIN32) || defined(__linux__)
#pragma pack(push,1)
#endif

// should be same as MAX_PATH
#define MAX_PATH_PIPE 260

typedef struct {
    int32_t Command;
#define PIPE_API_LOGIN_CMD     1
    int32_t StructSize;
    int32_t Version;
    int32_t Machine;
#define MACHINE_WIN32_X86  0
#define MACHINE_WIN32_X64  1
#define MACHINE_LINUX_X86  2
#define MACHINE_LINUX_X64  3
    uint32_t ProcessId;
    char ProcessName[MAX_PATH_PIPE+64];
    char ExecutableName[MAX_PATH_PIPE];
    int32_t ProcessNumber;
    int32_t NumberOfProcesses;            // Number of processes inside this executable
    int32_t Priority;                     // if -1 than it is not defined
    int32_t CycleDivider;                 // if -1 than it is not defined
    int32_t Delay;                        // if -1 than it is not defined

    int32_t IsStartedInDebugger;  // This is 1 if the process is started inside a debugger (Remark the process can be attache later)
    ALIVE_PING_HANDLE_ULONG hAsyncAlivePingReqEvent;
    ALIVE_PING_HANDLE_ULONG hAsyncAlivePingAckEvent;

    char DllName[MAX_PATH_PIPE];   // If the process live in DLL/shared library

    uint64_t ExecutableBaseAddress;
    uint64_t DllBaseAddress;

    uint64_t ExtensionsFlags;  // This must be 0
    uint8_t FillSpaceForExtensions[1024];  // this is space for later extensions
} PIPE_API_LOGIN_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
#define PIPE_API_LOGIN_SUCCESS  0
#define PIPE_API_LOGIN_ERROR_NO_FREE_PID               -101
#define PIPE_API_LOGIN_ERROR_PROCESS_ALREADY_RUNNING   -102
#define PIPE_API_LOGIN_ERROR_WRONG_VERSION             -103
    int32_t Version;
    int32_t Pid;
    int32_t conv_error2message;
    uint64_t WrMask;
    char BlackboardName[MAX_PATH_PIPE];
    //...
} PIPE_API_LOGIN_MESSAGE_ACK;

typedef struct {
    int32_t Command;
#define PIPE_API_LOGOUT_CMD     2
    int32_t StructSize;
    int32_t Pid;
    int32_t ImmediatelyFlag;  // Wenn != 0 dann sofort ohne Warten auf CYCLIC_ACK Message und ohne Terminate Funktion aus dem Scheduler loeschen
} PIPE_API_LOGOUT_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
} PIPE_API_LOGOUT_MESSAGE_ACK;

typedef struct {
    int32_t Command;
#define PIPE_API_PING_CMD     6
    int32_t StructSize;
    //...
} PIPE_API_PING_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
    int32_t Version;
    //...
} PIPE_API_PING_MESSAGE_ACK;

typedef struct {
    int32_t Command;
#define PIPE_API_KILL_WITH_NO_RESPONSE_CMD     3
    int32_t StructSize;
} PIPE_API_KILL_WITH_NO_RESPONSE_MESSAGE;

/* Base */
typedef struct {
    int32_t Command;
    int32_t StructSize;
    //...
} PIPE_API_MESSAGE_BUFFER;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    //...
} PIPE_API_BASE_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
    //...
} PIPE_API_BASE_CMD_MESSAGE_ACK;

/*----------------------*/
/* Scheduling functions */
/*----------------------*/

/* Reference-/Init-/Cyclic-/Terminate-Function */
typedef struct {
    int32_t Command;
#define PIPE_API_CALL_REFERENCE_FUNC_CMD     101
#define PIPE_API_CALL_INIT_FUNC_CMD      102
#define PIPE_API_CALL_CYCLIC_FUNC_CMD    103
#define  PIPE_API_CALL_TERMINATE_FUNC_CMD  104
    int32_t StructSize;
    uint64_t Cycle;
    int32_t Fill;
    int32_t SnapShotSize;
    char SnapShotData[1]; // more than 1 Byte!
} PIPE_API_CALL_FUNC_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
    int32_t SnapShotSize;
    char SnapShotData[1]; // more than 1 Byte!
} PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK;

// dereference all
typedef struct {
    int32_t Command;
#define PIPE_API_DEREFERENCE_ALL_CMD     110
    int32_t StructSize;
} PIPE_API_DEREFERENCE_ALL_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
} PIPE_API_DEREFERENCE_ALL_CMD_MESSAGE_ACK;

// get current scheduling cycle, period and process cycle, period, delay
typedef struct {
    int32_t Command;
#define PIPE_API_GET_SCHEDUING_INFORMATION_CMD     111
    int32_t StructSize;
    int32_t Optional;
} PIPE_API_GET_SCHEDUING_INFORMATION_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
    int32_t SeparateCyclesForRefAndInitFunction;
    uint64_t SchedulerCycle;
    int64_t SchedulerPeriod;
    uint64_t MainSchedulerCycleCounter;
    uint64_t ProcessCycle;
    int32_t ProcessPeriod;
    int32_t ProcessDelay;
} PIPE_API_GET_SCHEDUING_INFORMATION_CMD_MESSAGE_ACK;

/* Loopout function */
typedef struct {
    int32_t Command;
#define PIPE_API_LOOP_OUT_CMD     120
    int32_t StructSize;
    char BarrierBeforeName[64];       // MAX_BARRIER_NAME_LENGTH in SchedBarrier.h
    char BarrierBehindName[64];       // MAX_BARRIER_NAME_LENGTH in SchedBarrier.h
    uint64_t Cycle;
    int32_t Fill;
    int32_t SnapShotSize;
    char SnapShotData[1]; // more than 1 Byte!
} PIPE_API_LOOP_OUT_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
    int32_t SnapShotSize;
    char SnapShotData[1]; // more than 1 Byte!
} PIPE_API_LOOP_OUT_CMD_MESSAGE_ACK;

/*----------------------*/
/* Blackboard functions */
/*----------------------*/

union DOUBLE_UNION {
    double d;
    uint64_t ui;
};

/* Add blackboard variable function */
typedef struct {
    int32_t Command;
#define  PIPE_API_ADD_BBVARI_CMD  201
    int32_t StructSize;
    int32_t Pid;
    int32_t Dir;
#define PIPE_API_REFERENCE_VARIABLE_DIR_READ               0x00000001
#define PIPE_API_REFERENCE_VARIABLE_DIR_WRITE              0x00000002
#define PIPE_API_REFERENCE_VARIABLE_DIR_READ_WRITE         0x00000003
#define PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST      0x00010000
#define PIPE_API_REFERENCE_VARIABLE_DIR_IGNORE_REF_FILTER  0x00020000
    int32_t DataType;
    int32_t ConversionType;
    uint64_t Address;
    union DOUBLE_UNION Min;
    union DOUBLE_UNION Max;
    union DOUBLE_UNION Step;
    union DOUBLE_UNION ConversionFactor;
    union DOUBLE_UNION ConversionOffset;
    int32_t Width;
    int32_t Prec;
    int32_t StepType;
    uint32_t RgbColor;
    union BB_VARI Value;
    int32_t ValueValidFlag;
    int32_t AddressValidFlag;
    uint64_t UniqueId;

    int32_t NameStructOffset;
    int32_t DisplayNameStructOffset;
    int32_t UnitStructOffset;
    int32_t ConversionStructOffset;
    int32_t CommentStructOffset;
    int32_t Fill1;
    char Data[1]; // more than 1 Byte!
} PIPE_API_ADD_BBVARI_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t Vid;
    int32_t RealType;
    int32_t ReturnValue;
} PIPE_API_ADD_BBVARI_CMD_MESSAGE_ACK;

/* Attach blackboard variable function */
typedef struct {
    int32_t Command;
#define  PIPE_API_ATTACH_BBVARI_CMD  208
    int32_t StructSize;
    int32_t Pid;
    int32_t Dir;
    char Name[1]; // more than 1 Byte!
} PIPE_API_ATTACH_BBVARI_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t Vid;
    int32_t ReturnValue;
} PIPE_API_ATTACH_BBVARI_CMD_MESSAGE_ACK;

/* Remove blackboard variable function */
typedef struct {
    int32_t Command;
#define  PIPE_API_REMOVE_BBVARI_CMD  202
    int32_t StructSize;
    int32_t Pid;
    int32_t Vid;
    int32_t Dir;
    int32_t DataType;
    uint64_t Addr;
    uint64_t UniqueId;
} PIPE_API_REMOVE_BBVARI_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
} PIPE_API_REMOVE_BBVARI_CMD_MESSAGE_ACK;

/* Write to blackboard variable function (not synchronized) */
typedef struct {
    int32_t Command;
#define  PIPE_API_WRITE_BBVARI_CMD  205
    int32_t StructSize;
    int32_t Pid;
    int32_t Vid;
    int32_t DataType;
    union BB_VARI Value;
} PIPE_API_WRITE_BBVARI_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
} PIPE_API_WRITE_BBVARI_CMD_MESSAGE_ACK;

/* Read from blackboard variable function (not synchronized) */
typedef struct {
    int32_t Command;
#define  PIPE_API_READ_BBVARI_CMD  206
    int32_t StructSize;
    int32_t Pid;
    int32_t Vid;
    int32_t DataType;
} PIPE_API_READ_BBVARI_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
    int32_t DataType;
    union BB_VARI Value;
} PIPE_API_READ_BBVARI_CMD_MESSAGE_ACK;

/* Get Label name by address function */
typedef struct {
    int32_t Command;
#define PIPE_API_GET_LABEL_BY_ADDRESS_CMD              203
    int32_t StructSize;
    int32_t Pid;
    uint64_t Address;
} PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
    int32_t DataType;
    char Label[1];     // more than 1 Byte! (muss das letzte Struktur-Element bleiben!)
    //...
} PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE_ACK;

/* Get Label name by address function */
typedef struct {
    int32_t Command;
#define PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD   207
    int32_t StructSize;
    int32_t Pid;
    int32_t Vid;
} PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
    int32_t DataType;
    char Label[1];     // more than 1 Byte! (muss das letzte Struktur-Element bleiben!)
    //...
} PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE_ACK;

/*  write_to_msg_file */
typedef struct {
    int32_t Command;
#define PIPE_API_WRITE_TO_MSG_FILE_CMD   204
    int32_t StructSize;
    int32_t Pid;
    char Text[1]; // more than 1 Byte! (muss das letzte Struktur-Element bleiben!)
    //...
} PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
} PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE_ACK;

/*  write_to_msg_file */
typedef struct {
    int32_t Command;
#define PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD   209
    int32_t StructSize;
    int32_t Level;
    int32_t Pid;
    char Text[1]; // more than 1 Byte! (muss das letzte Struktur-Element bleiben!)
    //...
} PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
} PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE_ACK;

// Open/close virtuel network channel
typedef struct {
    int32_t Command;
#define PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD   210
    int32_t Type;
    int32_t Channel;
    int32_t StructSize;
} PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
} PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE_ACK;

typedef struct {
    int32_t Command;
#define PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD   211
    int32_t Handle;
    int32_t StructSize;
} PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
} PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE_ACK;



/* Speicher eines externen Prozesses lesen oder beschreiben */
#define MAX_MEMORY_BLOCK_SIZE (32*1024)

typedef struct {
    int32_t Command;
#define PIPE_API_READ_MEMORY_CMD   301
    int32_t StructSize;
    uint64_t Address;
    uint32_t Size;
} PIPE_API_READ_MEMORY_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
    uint32_t Size;
    char Memory[1]; // more than 1 Byte! (muss das letzte Struktur-Element bleiben!)
} PIPE_API_READ_MEMORY_CMD_MESSAGE_ACK;

typedef struct {
    int32_t Command;
#define PIPE_API_WRITE_MEMORY_CMD   302
    int32_t StructSize;
    uint64_t Address;
    uint32_t Size;
    char Memory[1]; // more than 1 Byte! (muss das letzte Struktur-Element bleiben!)
} PIPE_API_WRITE_MEMORY_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
    uint32_t Size;
} PIPE_API_WRITE_MEMORY_CMD_MESSAGE_ACK;

/* referenzieren einer Variable */
typedef struct {
    int32_t Command;
#define PIPE_API_REFERENCE_VARIABLE_CMD   303
    int32_t StructSize;
    uint64_t Address;
    int32_t Type;
    int32_t Dir;    // Defines siehe PIPE_API_ADD_BBVARI_CMD_MESSAGE
    char Name[1]; // more than 1 Byte!
} PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
} PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE_ACK;

typedef struct {
    int32_t Command;
#define PIPE_API_DEREFERENCE_VARIABLE_CMD   304
    int32_t StructSize;
    uint64_t Address;
    int32_t Type;
    int32_t Vid;
    char Name[1]; // more than 1 Byte!
} PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
} PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE_ACK;

// Write Section to EXE
typedef struct {
    int32_t Command;
#define PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD   305
    int32_t StructSize;
    char SectionName[1]; // more than 1 Byte!
} PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD_MESSAGE;

typedef struct {
    int32_t Command;
    int32_t StructSize;
    int32_t ReturnValue;
} PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD_MESSAGE_ACK;



#if defined(_WIN32) || defined(__linux__)
#pragma pack(pop)
#endif

#endif
