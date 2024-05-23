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


#ifndef _WIN32
// nicht direkt stdint.h da es den Header unter VS 2003 nicht gibt
#include "my_stdint.h"
#include "Platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define INVALID_HANDLE_VALUE (-1)
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define SOCKET int

#include "XilEnvExtProc.h"
#include "ExtpProcessAndTaskInfos.h"

#include "ExtpUnixDomainSocketMessages.h"


static HANDLE XilEnvInternal_ConnectToUnixDomainSocket (char *par_InstanceName, char *par_ServerName, int par_Timout_ms, int par_ErrMessageFlag)
{
    SOCKET Socket;
    struct sockaddr_un address;
    int iResult;
    char Name[MAX_PATH + 100];

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;

    if (CheckOpenIPCFile(par_InstanceName, "unix_domain", Name, DIR_MUST_EXIST, FILENAME_MUST_EXIST) != 0) {
        ThrowError(1, "cannot open the unix_domain file for XilEnv instance \"%s\"", par_InstanceName);
        return INVALID_HANDLE_VALUE;
    }
    strcpy(address.sun_path, Name);

    // Create a SOCKET for connecting to server
    Socket = socket(PF_UNIX, SOCK_STREAM, 0);
    if (Socket == INVALID_SOCKET) {
        if (par_ErrMessageFlag) ThrowError(1, "socket failed with error: %i\n", errno);
        return INVALID_HANDLE_VALUE;
    }

    // Connect to server.
    iResult = connect (Socket, (__CONST_SOCKADDR_ARG)&address, sizeof(address));
    if (iResult == SOCKET_ERROR) {
        close(Socket);
        Socket = (SOCKET)INVALID_HANDLE_VALUE;
        return INVALID_HANDLE_VALUE;
    }

    return (HANDLE)Socket;
}


static void XilEnvInternal_DisConnectFromUnixDomainSocket (HANDLE Socket)
{
    close(Socket);
}


static BOOL XilEnvInternal_TransactMessageUnixDomainSocket (HANDLE Socket,
                                                             LPVOID lpInBuffer,
                                                             DWORD nInBufferSize,
                                                             LPVOID lpOutBuffer,
                                                             DWORD nOutBufferSize,
                                                             LPDWORD lpBytesRead)
{
    DWORD buffer_pos = 0;

    do {
        int SendBytes = send ((SOCKET)Socket, (const char*)lpInBuffer + buffer_pos, nInBufferSize - buffer_pos, 0);
        if (SendBytes == SOCKET_ERROR) {
            return FALSE;
        }
        buffer_pos += SendBytes;
    } while (buffer_pos < nInBufferSize);

    buffer_pos = 0;

    /*do*/ {
        int ReceiveBytes = recv ((SOCKET)Socket, (char*)lpOutBuffer + buffer_pos, nOutBufferSize - buffer_pos, 0);
        if (ReceiveBytes == SOCKET_ERROR) {
            return FALSE;
        }
        buffer_pos += ReceiveBytes;
    } /*while (buffer_pos < nOutBufferSize);*/
    *lpBytesRead = buffer_pos;
    return TRUE;
}


BOOL XilEnvInternal_ReadMessageUnixDomainSocket (HANDLE Socket,
							                      LPVOID lpBuffer,
							                      DWORD nNumberOfBytesToRead,
							                      LPDWORD lpNumberOfBytesRead)
{
    int ReceiveBytes;
    ReceiveBytes = recv ((SOCKET)Socket, (char*)lpBuffer, nNumberOfBytesToRead, 0);
    if (ReceiveBytes <= 0) {
        return FALSE;
    } else {
        *lpNumberOfBytesRead = (DWORD)ReceiveBytes;
        return TRUE;
    }
}

BOOL XilEnvInternal_WriteMessageUnixDomainSocket (HANDLE Socket,
                                                   LPCVOID lpBuffer,
								                   DWORD nNumberOfBytesToWrite,
								                   LPDWORD lpNumberOfBytesWritten)
{
    int SendBytes;
    SendBytes = send ((SOCKET)Socket, (const char*)lpBuffer, nNumberOfBytesToWrite, 0);
    if (SendBytes <= 0) {
        return FALSE;
    } else {
        *lpNumberOfBytesWritten = SendBytes;
        return TRUE;
    }
}


int XilEnvInternal_InitUnixDomainSocketCommunication(EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo)
{
    TaskInfo->ConnectTo = XilEnvInternal_ConnectToUnixDomainSocket;
    TaskInfo->DisconnectFrom = XilEnvInternal_DisConnectFromUnixDomainSocket;
	TaskInfo->TransactMessage = XilEnvInternal_TransactMessageUnixDomainSocket;
	TaskInfo->ReadMessage = XilEnvInternal_ReadMessageUnixDomainSocket;
    TaskInfo->WriteMessage = XilEnvInternal_WriteMessageUnixDomainSocket;

    return 0;
}

int XilEnvInternal_IsUnixDomainSocketInstanceIsRunning(char *par_InstanceName, char *par_ServerName, int par_Timout_ms)
{
    PIPE_API_PING_MESSAGE Req;
    PIPE_API_PING_MESSAGE_ACK Ack;
    DWORD BytesRead;
    int Ret = 0;

    HANDLE Socket = XilEnvInternal_ConnectToUnixDomainSocket(par_InstanceName, par_ServerName, par_Timout_ms, 0);
    if (Socket == INVALID_HANDLE_VALUE) {
        return 0;   // is not running
    }
    Req.Command = PIPE_API_PING_CMD;
    Ret = XilEnvInternal_TransactMessageUnixDomainSocket(Socket, &Req, sizeof(Req), &Ack, sizeof(Ack), &BytesRead);
    XilEnvInternal_DisConnectFromUnixDomainSocket(Socket);
    return Ret;
}

int XilEnvInternal_WaitTillUnixDomainSocketInstanceIsRunning(char *par_InstanceName, char *par_ServerName, int par_Timout_ms)
{
    PIPE_API_PING_MESSAGE Req;
    PIPE_API_PING_MESSAGE_ACK Ack;
    DWORD BytesRead;
    int Ret = 0;

    uint64_t End = GetTickCount64() + par_Timout_ms;

    while (1) {
        HANDLE Socket = XilEnvInternal_ConnectToUnixDomainSocket(par_InstanceName, par_ServerName, par_Timout_ms, 0);
        if (Socket == INVALID_HANDLE_VALUE) {
            if (GetTickCount() > End) break;   // Timeout expired
            usleep(100 * 1000);
            continue;   // is not running
        }
        Req.Command = PIPE_API_PING_CMD;
        Ret = XilEnvInternal_TransactMessageUnixDomainSocket(Socket, &Req, sizeof(Req), &Ack, sizeof(Ack), &BytesRead);
        XilEnvInternal_DisConnectFromUnixDomainSocket(Socket);
        if (Ret) break;
    }
    return Ret;
}
#endif
