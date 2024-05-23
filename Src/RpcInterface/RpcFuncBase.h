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


#ifndef RPCFUNCBASE_H
#define RPCFUNCBASE_H

#include <stdint.h>
//#include "rpc_socket_server.h"

#define RPC_API_VERSION    1

typedef struct {
    int32_t StructSize;
    int32_t Command;
} RPC_API_BASE_MESSAGE;

typedef struct {
    int32_t StructSize;
    int32_t Command;
    int32_t ReturnValue;
#define RPC_API_LOGIN_SUCCESS                            0
#define RPC_API_LOGIN_WRONG_MESSAGE_SIZE              -101
#define RPC_API_LOGIN_ERROR_WRONG_VERSION             -103

} RPC_API_BASE_MESSAGE_ACK;

#define STRINGIZE_N(a) #a

#define STRINGIZE(a) STRINGIZE_N(a)

#endif // RPCFUNCBASE_H
