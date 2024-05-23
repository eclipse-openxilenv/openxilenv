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


#ifndef RPCFUNCMISC_H
#define RPCFUNCMISC_H

#include <stdint.h>

#include "RpcFuncBase.h"

#ifdef _WIN32
#ifdef __GNUC__
#pragma ms_struct on
#endif
#endif

#pragma pack(push,1)

#define MISC_CMD_OFFSET  410

#define RPC_API_CREATE_FILE_WITH_CONTENT_CMD         (MISC_CMD_OFFSET+1)
typedef struct {
#define RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetFilename;\
    int32_t OffsetContent;\
    char Data[1];  // > 1Byte!
    RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE_MEMBERS
} RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE;

typedef struct {
#define RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE_ACK_MEMBERS
} RPC_API_CREATE_FILE_WITH_CONTENT_MESSAGE_ACK;


#define RPC_API_GET_ENVIRONMENT_VARIABLE_CMD         (MISC_CMD_OFFSET+3)
typedef struct {
#define RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t MaxSize;\
    int32_t OffsetVariableName;\
    char Data[1];  // > 1Byte!
    RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_MEMBERS
} RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE;

typedef struct {
#define RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;\
    int32_t OffsetVariableValue;\
    char Data[1];  // > 1Byte!
    RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_ACK_MEMBERS
} RPC_API_GET_ENVIRONMENT_VARIABLE_MESSAGE_ACK;

#define RPC_API_SET_ENVIRONMENT_VARIABLE_CMD         (MISC_CMD_OFFSET+4)
typedef struct {
#define RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetVariableName;\
    int32_t OffsetVariableValue;\
    char Data[1];  // > 1Byte!
    RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE_MEMBERS
} RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE;

typedef struct {
#define RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE_ACK_MEMBERS
} RPC_API_SET_ENVIRONMENT_VARIABLE_MESSAGE_ACK;

#define RPC_API_CHANGE_SETTINGS_CMD                  (MISC_CMD_OFFSET+5)
typedef struct {
#define RPC_API_SET_CHANGE_SETTINGS_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffseSettingName;\
    int32_t OffsetSettingValue;\
    char Data[1];  // > 1Byte!
    RPC_API_SET_CHANGE_SETTINGS_MESSAGE_MEMBERS
} RPC_API_SET_CHANGE_SETTINGS_MESSAGE;

typedef struct {
#define RPC_API_SET_CHANGE_SETTINGS_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_SET_CHANGE_SETTINGS_MESSAGE_ACK_MEMBERS
} RPC_API_SET_CHANGE_SETTINGS_MESSAGE_ACK;

#define RPC_API_TEXT_OUT_CMD                         (MISC_CMD_OFFSET+6)
typedef struct {
#define RPC_API_TEXT_OUT_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetText;\
    char Data[1];  // > 1Byte!
    RPC_API_TEXT_OUT_MESSAGE_MEMBERS
} RPC_API_TEXT_OUT_MESSAGE;

typedef struct {
#define RPC_API_TEXT_OUT_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_TEXT_OUT_MESSAGE_ACK_MEMBERS
} RPC_API_TEXT_OUT_MESSAGE_ACK;

#define RPC_API_ERROR_TEXT_OUT_CMD                   (MISC_CMD_OFFSET+7)
typedef struct {
#define RPC_API_ERROR_TEXT_OUT_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t ErrorLevel;\
    int32_t OffsetText;\
    char Data[1];  // > 1Byte!
    RPC_API_ERROR_TEXT_OUT_MESSAGE_MEMBERS
} RPC_API_ERROR_TEXT_OUT_MESSAGE;

typedef struct {
#define RPC_API_ERROR_TEXT_OUT_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_ERROR_TEXT_OUT_MESSAGE_ACK_MEMBERS
} RPC_API_ERROR_TEXT_OUT_MESSAGE_ACK;

#define RPC_API_CREATE_FILE_CMD         (MISC_CMD_OFFSET+9)
typedef struct {
#define RPC_API_CREATE_FILE_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t OffsetFilename;\
    uint32_t dwDesiredAccess;\
    uint32_t dwShareMode;\
    uint32_t dwCreationDisposition;\
    uint32_t dwFlagsAndAttributes;\
    char Data[1];  // > 1Byte!
    RPC_API_CREATE_FILE_MESSAGE_MEMBERS
} RPC_API_CREATE_FILE_MESSAGE;

typedef struct {
#define RPC_API_CREATE_FILE_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;\
    uint64_t Handle;
    RPC_API_CREATE_FILE_MESSAGE_ACK_MEMBERS
} RPC_API_CREATE_FILE_MESSAGE_ACK;

#define RPC_API_CLOSE_HANDLE_CMD         (MISC_CMD_OFFSET+10)
typedef struct {
#define RPC_API_CLOSE_HANDLE_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    uint64_t Handle;
    RPC_API_CLOSE_HANDLE_MESSAGE_MEMBERS
} RPC_API_CLOSE_HANDLE_MESSAGE;

typedef struct {
#define RPC_API_CLOSE_HANDLE_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_CLOSE_HANDLE_MESSAGE_ACK_MEMBERS
} RPC_API_CLOSE_HANDLE_MESSAGE_ACK;

#define RPC_API_READ_FILE_CMD         (MISC_CMD_OFFSET+11)
typedef struct {
#define RPC_API_READ_FILE_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    uint64_t Handle;\
    uint32_t nNumberOfBytesToRead;
    RPC_API_READ_FILE_MESSAGE_MEMBERS
} RPC_API_READ_FILE_MESSAGE;

typedef struct {
#define RPC_API_READ_FILE_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;\
    uint32_t NumberOfBytesRead;\
    uint32_t Offset_uint8_NumberOfBytesRead_Buffer;\
    char Data[1];  // > 1Byte!
    RPC_API_READ_FILE_MESSAGE_ACK_MEMBERS
} RPC_API_READ_FILE_MESSAGE_ACK;

#define RPC_API_WRITE_FILE_CMD         (MISC_CMD_OFFSET+12)
typedef struct {
#define RPC_API_WRITE_FILE_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    uint64_t Handle;\
    uint32_t Offset_uint8_nNumberOfBytesToWrite_Buffer;\
    uint32_t nNumberOfBytesToWrite;\
    char Data[1];  // > 1Byte!
    RPC_API_WRITE_FILE_MESSAGE_MEMBERS
} RPC_API_WRITE_FILE_MESSAGE;

typedef struct {
#define RPC_API_WRITE_FILE_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;\
    uint32_t NumberOfBytesWritten;
    RPC_API_WRITE_FILE_MESSAGE_ACK_MEMBERS
} RPC_API_WRITE_FILE_MESSAGE_ACK;

#pragma pack(pop)
#ifdef _WIN32
#ifdef __GNUC__
#pragma ms_struct reset
#endif
#endif

int AddMiscFunctionToTable(void);

#endif // RPCFUNCMISC_H
