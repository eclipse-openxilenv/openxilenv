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


#ifndef RPCFUNCLOGIN_H
#define RPCFUNCLOGIN_H

#include <stdint.h>

#include "RpcFuncBase.h"

#ifdef _WIN32
#ifdef __GNUC__
#pragma ms_struct on
#endif
#endif
#pragma pack(push,1)

#define RPC_API_LOGIN_CMD                   1
typedef struct {
#define RPC_API_LOGIN_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t InterfaceVersion;\
    int32_t LibraryVersion;\
    int32_t Machine;
    RPC_API_LOGIN_MESSAGE_MEMBERS
#define MACHINE_WIN32    0
#define MACHINE_WIN64    1
#define MACHINE_LINUX64  2

} RPC_API_LOGIN_MESSAGE;

typedef struct {
#define RPC_API_LOGIN_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;\
    int32_t InterfaceVersion;\
    int32_t Version;\
    int32_t Pid;
    RPC_API_LOGIN_MESSAGE_ACK_MEMBERS
} RPC_API_LOGIN_MESSAGE_ACK;

#define RPC_API_LOGOUT_CMD                  2
typedef struct {
#define RPC_API_LOGOUT_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t ShouldTerminate;\
    int32_t SetExitCode;\
    uint32_t ExitCode;
    RPC_API_LOGOUT_MESSAGE_MEMBERS
} RPC_API_LOGOUT_MESSAGE;

typedef struct {
#define RPC_API_LOGOUT_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_LOGOUT_MESSAGE_ACK_MEMBERS
} RPC_API_LOGOUT_MESSAGE_ACK;


#define RPC_API_GET_VERSION_CMD     3
typedef struct {
#define RPC_API_GET_VERSION_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;
    RPC_API_GET_VERSION_MESSAGE_MEMBERS
} RPC_API_GET_VERSION_MESSAGE;

typedef struct {
#define RPC_API_GET_VERSION_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;\
    int32_t InterfaceVersion;\
    int32_t Version;\
    int32_t MinorVersion;
    RPC_API_GET_VERSION_MESSAGE_ACK_MEMBERS
} RPC_API_GET_VERSION_MESSAGE_ACK;

#define RPC_API_PING_CMD                    4
typedef struct {
#define RPC_API_PING_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;\
    int32_t Data[8];
    RPC_API_PING_MESSAGE_MEMBERS
} RPC_API_PING_MESSAGE;

typedef struct {
#define RPC_API_PING_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;\
    int32_t Data[8];
    RPC_API_PING_MESSAGE_ACK_MEMBERS
} RPC_API_PING_MESSAGE_ACK;

#define RPC_API_SHOULD_BE_TERMINATED_CMD          5
typedef struct {
#define RPC_API_SHOULD_BE_TERMINATED_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;
    RPC_API_SHOULD_BE_TERMINATED_MESSAGE_MEMBERS
} RPC_API_SHOULD_BE_TERMINATED_MESSAGE;

typedef struct {
#define RPC_API_SHOULD_BE_TERMINATED_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_SHOULD_BE_TERMINATED_MESSAGE_ACK_MEMBERS
} RPC_API_SHOULD_BE_TERMINATED_MESSAGE_ACK;

#define RPC_API_PING_TO_CLINT_CMD                    6
typedef struct {
#define RPC_API_PING_TO_CLINT_MESSAGE_MEMBERS \
    RPC_API_BASE_MESSAGE Header;
    RPC_API_PING_TO_CLINT_MESSAGE_MEMBERS
} RPC_API_PING_TO_CLINT_MESSAGE;

typedef struct {
#define RPC_API_PING_TO_CLINT_MESSAGE_ACK_MEMBERS \
    RPC_API_BASE_MESSAGE_ACK Header;
    RPC_API_PING_TO_CLINT_MESSAGE_ACK_MEMBERS
} RPC_API_PING_TO_CLINT_MESSAGE_ACK;

#pragma pack(pop)
#ifdef _WIN32
#ifdef __GNUC__
#pragma ms_struct reset
#endif
#endif

int AddLoginFunctionToTable(void);

#endif // RPCFUNCLOGIN_H
