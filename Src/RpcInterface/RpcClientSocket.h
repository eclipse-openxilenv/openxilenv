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


#ifndef RPC_CLIENT_SOCKET_H
#define RPC_CLIENT_SOCKET_H

#include "RpcFuncBase.h"

HANDLE ConnectToRemoteProcedureCallServer(char *par_ServerName, int par_Port);
#ifdef _WIN32
HANDLE NamedPipeConnectToRemoteProcedureCallServer(const char *par_ServerName, const char *par_Instance, int par_Timout_ms);
#endif

void DisconnectFromRemoteProcedureCallServer(int par_SocketOrNamedPipe, HANDLE par_Socket);

int SentToRemoteProcedureCallServer (int par_SocketOrNamedPipe, HANDLE par_Socket, int par_Command, RPC_API_BASE_MESSAGE *par_Req, int par_PackageSize);
int RemoteProcedureCallTransact (int par_SocketOrNamedPipe, HANDLE par_Socket, uint32_t par_Command, 
                                 RPC_API_BASE_MESSAGE *par_Req,
                                 int par_BytesToWrite,
                                 RPC_API_BASE_MESSAGE_ACK *ret_Ack,
                                 int par_MaxReceiveBytes);


void *RemoteProcedureGetReceiveBuffer(int par_NeededSize);
void *RemoteProcedureGetTransmitBuffer(int par_NeededSize);

int RemoteProcedureCallTransactDynBuf (int par_SocketOrNamedPipe, HANDLE par_Socket, int par_Command, RPC_API_BASE_MESSAGE *par_TransmitData, int par_BytesToTransmit, RPC_API_BASE_MESSAGE_ACK **ret_ReceiveData);

#endif
