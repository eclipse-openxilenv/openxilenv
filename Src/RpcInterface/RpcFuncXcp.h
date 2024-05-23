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


#ifndef RPCFUNCXCP_H
#define RPCFUNCXCP_H

#include <stdint.h>

#include "RpcFuncBase.h"

#ifdef _WIN32
#ifdef __GNUC__
#pragma ms_struct on
#endif
#endif
#pragma pack(push,1)

#define XCP_CMD_OFFSET  390

#define RPC_API_LOAD_XCP_CONFIG_CMD         (XCP_CMD_OFFSET+0)
typedef struct {
#define RPC_API_LOAD_XCP_CONFIG_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Connection;\
    int32_t OffsetXcpFile;\
    char Data[1];  // > 1Byte!
    RPC_API_LOAD_XCP_CONFIG_MESSAGE_MEMBERS
}  RPC_API_LOAD_XCP_CONFIG_MESSAGE;

typedef struct {
#define RPC_API_LOAD_XCP_CONFIG_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_LOAD_XCP_CONFIG_MESSAGE_ACK_MEMBERS
}  RPC_API_LOAD_XCP_CONFIG_MESSAGE_ACK;

#define RPC_API_START_XCP_BEGIN_CMD         (XCP_CMD_OFFSET+1)
typedef struct {
#define RPC_API_START_XCP_BEGIN_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Connection;
    RPC_API_START_XCP_BEGIN_MESSAGE_MEMBERS
} RPC_API_START_XCP_BEGIN_MESSAGE;

typedef struct {
#define RPC_API_START_XCP_BEGIN_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_START_XCP_BEGIN_MESSAGE_ACK_MEMBERS
} RPC_API_START_XCP_BEGIN_MESSAGE_ACK;

#define RPC_API_START_XCP_ADD_VAR_CMD         (XCP_CMD_OFFSET+2)
typedef struct {
#define RPC_API_START_XCP_ADD_VAR_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Connection;\
    int32_t OffsetLabel;\
    char Data[1];  // > 1Byte!
    RPC_API_START_XCP_ADD_VAR_MESSAGE_MEMBERS
} RPC_API_START_XCP_ADD_VAR_MESSAGE;

typedef struct {
#define RPC_API_START_XCP_ADD_VAR_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_START_XCP_ADD_VAR_MESSAGE_ACK_MEMBERS
} RPC_API_START_XCP_ADD_VAR_MESSAGE_ACK;

#define RPC_API_START_XCP_END_CMD         (XCP_CMD_OFFSET+3)
typedef struct {
#define RPC_API_START_XCP_END_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Connection;
    RPC_API_START_XCP_END_MESSAGE_MEMBERS
} RPC_API_START_XCP_END_MESSAGE;

typedef struct {
#define RPC_API_START_XCP_END_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_START_XCP_END_MESSAGE_ACK_MEMBERS
} RPC_API_START_XCP_END_MESSAGE_ACK;

#define RPC_API_STOP_XCP_CMD         (XCP_CMD_OFFSET+4)
typedef struct {
#define RPC_API_STOP_XCP_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Connection;
    RPC_API_STOP_XCP_MESSAGE_MEMBERS
} RPC_API_STOP_XCP_MESSAGE;

typedef struct {
#define RPC_API_STOP_XCP_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_STOP_XCP_MESSAGE_ACK_MEMBERS
} RPC_API_STOP_XCP_MESSAGE_ACK;

#define RPC_API_START_XCP_CAL_BEGIN_CMD         (XCP_CMD_OFFSET+5)
typedef struct {
#define RPC_API_START_XCP_CAL_BEGIN_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Connection;
    RPC_API_START_XCP_CAL_BEGIN_MESSAGE_MEMBERS
} RPC_API_START_XCP_CAL_BEGIN_MESSAGE;

typedef struct {
#define RPC_API_START_XCP_CAL_BEGIN_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_START_XCP_CAL_BEGIN_MESSAGE_ACK_MEMBERS
} RPC_API_START_XCP_CAL_BEGIN_MESSAGE_ACK;

#define RPC_API_START_XCP_CAL_ADD_VAR_CMD         (XCP_CMD_OFFSET+6)
typedef struct {
#define RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Connection;\
    int32_t OffsetLabel;\
    char Data[1];  // > 1Byte!
    RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE_MEMBERS
} RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE;

typedef struct {
#define RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE_ACK_MEMBERS
} RPC_API_START_XCP_CAL_ADD_VAR_MESSAGE_ACK;

#define RPC_API_START_XCP_CAL_END_CMD         (XCP_CMD_OFFSET+7)
typedef struct {
#define RPC_API_START_XCP_CAL_END_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Connection;
    RPC_API_START_XCP_CAL_END_MESSAGE_MEMBERS
} RPC_API_START_XCP_CAL_END_MESSAGE;

typedef struct {
#define RPC_API_START_XCP_CAL_END_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_START_XCP_CAL_END_MESSAGE_ACK_MEMBERS
} RPC_API_START_XCP_CAL_END_MESSAGE_ACK;

#define RPC_API_STOP_XCP_CAL_CMD         (XCP_CMD_OFFSET+8)
typedef struct {
#define RPC_API_STOP_XCP_CAL_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Connection;
    RPC_API_STOP_XCP_CAL_MESSAGE_MEMBERS
} RPC_API_STOP_XCP_CAL_MESSAGE;

typedef struct {
#define RPC_API_STOP_XCP_CAL_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_STOP_XCP_CAL_MESSAGE_ACK_MEMBERS
} RPC_API_STOP_XCP_CAL_MESSAGE_ACK;

#pragma pack(pop)

#ifdef _WIN32
#ifdef __GNUC__
#pragma ms_struct reset
#endif
#endif

int AddXcpFunctionToTable(void);


#endif // RPCFUNCXCP_H
