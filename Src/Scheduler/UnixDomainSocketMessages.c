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
#include <stdint.h>
#include "Platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <signal.h>
#include "Config.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "MemZeroAndCopy.h"
#include "StringMaxChar.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "tcb.h"
#include "Scheduler.h"
#include "GetNextStructEntry.h"
#include "Scheduler.h"
#include "ScBbCopyLists.h"
#include "BaseMessages.h"
#include "UnixDomainSocketMessages.h"

#define SOCKET int

// #define USE_DEBUG_API

static int LoggingFlag;
static FILE *LoggingFile;

static SOCKET CreateSoftcarLoginUnixDomainSocket (char *par_Prefix)
{
    char Name[MAX_PATH + 100];
    SOCKET sock_descriptor;

    struct sockaddr_un serv_addr = {0};

    if (CheckOpenIPCFile(par_Prefix, "unix_domain", Name, sizeof(Name), DIR_CREATE_EXIST, FILENAME_IGNORE) != 0) {
        ThrowError (1, "cannot create unix domain socket file");
        return -1;
    }

    if ((strlen(Name) + 1) > sizeof(serv_addr.sun_path)) {
        ThrowError (1, "the unix domain socket file name \"%s\" are %i chars long there are only %i chars allowed",
               Name, (int)strlen(Name), (int)sizeof(serv_addr.sun_path));
        return -1;
    }

    unlink(Name);

    sock_descriptor = socket(PF_UNIX, SOCK_STREAM, 0);
    if (sock_descriptor < 0) {
        ThrowError (1, "Failed creating socket");
        return -1;
    }

    serv_addr.sun_family = AF_UNIX;

    STRING_COPY_TO_ARRAY (serv_addr.sun_path, Name);

    if (bind(sock_descriptor, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        ThrowError (1, "Failed to bind");
        return -1;
    }

    listen(sock_descriptor, 5);

    return sock_descriptor;
}

static DWORD UnixDomainSocket_ReadFile (HANDLE Socket, void *Buffer, DWORD BufferSize, LPDWORD BytesRead)
{
    ssize_t len;
    len = recv((SOCKET)Socket, Buffer, (size_t)BufferSize, 0);
    if (len <= 0) return FALSE;
    *BytesRead = (DWORD)len;
    return TRUE;
}

static DWORD UnixDomainSocket_WriteFile (HANDLE Socket, void *Buffer, int nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten)
{
    ssize_t len = send((SOCKET)Socket, Buffer, (size_t)nNumberOfBytesToWrite, 0);
    if (len <= 0) {
        return FALSE;
    }
    *lpNumberOfBytesWritten = (DWORD)len;
    return TRUE;
}

static void UnixDomainSocket_CloseConnectionToExternProcess (HANDLE Socket)
{
    close((SOCKET)Socket);
}

static int ThreadWillCallFirstAcceptState;

static void* UnixDomainSocket_SoftcarLoginThreadProc (void* lpParam)
{
    SOCKET Socket;
    DWORD Success, BytesRead, BytesWritten;
    PIPE_API_LOGIN_MESSAGE *LoginMessage;
    PIPE_API_LOGIN_MESSAGE_ACK LoginMessageAck;

    Socket = CreateSoftcarLoginUnixDomainSocket((char *)lpParam);
    if (Socket < 0) return NULL;

    LoginMessage = (PIPE_API_LOGIN_MESSAGE*)my_malloc (PIPE_MESSAGE_BUFSIZE);
    for (;;) {  // Endless loop
        struct sockaddr_un client_addr;
        socklen_t size = sizeof(client_addr);
        SOCKET conn_desc;

        ThreadWillCallFirstAcceptState = 1;
        // this blocks on this call until a client tries to establish connection.
        conn_desc = accept(Socket, (struct sockaddr *)&client_addr, &size);
        if (conn_desc <= 0) {
            ThrowError (1, "Failed accepting connection\n");
        } else {
            Success = UnixDomainSocket_ReadFile ((HANDLE)conn_desc, (void*)LoginMessage, PIPE_MESSAGE_BUFSIZE, &BytesRead);
            if (Success) {
                if (LoggingFlag) MessageLoggingToFile (LoggingFile,__LINE__, 0, -1, LoginMessage);
                if (LoginMessage->Command == PIPE_API_LOGIN_CMD) {
                    if (Success && (BytesRead == sizeof (PIPE_API_LOGIN_MESSAGE))) {  // receive a valid login message
                        short Pid;
                        if (CheckExternProcessComunicationProtocolVersion(LoginMessage)) {
                            if (get_pid_by_name(LoginMessage->ProcessName) > 0) {
                                ThrowError (1, "process \"%s\" already running login ignored", LoginMessage->ProcessName);
                                Pid = -1;
                                LoginMessageAck.ReturnValue = PIPE_API_LOGIN_ERROR_PROCESS_ALREADY_RUNNING;
                            } else {
                                Pid = AddExternPorcessToScheduler (LoginMessage->ProcessName, LoginMessage->ExecutableName, LoginMessage->ProcessNumber, LoginMessage->NumberOfProcesses,
                                                                   LoginMessage->ProcessId, (HANDLE)conn_desc, 0,
                                                                   LoginMessage->Priority, LoginMessage->CycleDivider, LoginMessage->Delay,
                                                                   LoginMessage->IsStartedInDebugger,
                                                                   LoginMessage->Machine,
                                                                   LoginMessage->hAsyncAlivePingReqEvent, LoginMessage->hAsyncAlivePingAckEvent,
                                                                   LoginMessage->DllName,
                                                                   &UnixDomainSocket_ReadFile,
                                                                   &UnixDomainSocket_WriteFile,
                                                                   &UnixDomainSocket_CloseConnectionToExternProcess,
                                                                   LoggingFile,
                                                                   LoginMessage->ExecutableBaseAddress,
                                                                   LoginMessage->DllBaseAddress,
                                                                   LoginMessage->Version);
                                if (Pid < 0) {
                                    LoginMessageAck.ReturnValue = PIPE_API_LOGIN_ERROR_NO_FREE_PID; // Error
                                } else {
                                    LoginMessageAck.ReturnValue = PIPE_API_LOGIN_SUCCESS;
                                }
                            }
                        } else {
                            Pid = -1;
                            LoginMessageAck.ReturnValue = PIPE_API_LOGIN_ERROR_WRONG_VERSION;
                        }
                        LoginMessageAck.Command = PIPE_API_LOGIN_CMD;
                        LoginMessageAck.Pid = Pid;
                        LoginMessageAck.Version = XILENV_VERSION;
                        LoginMessageAck.StructSize = sizeof (LoginMessageAck);
                        if (LoggingFlag) MessageLoggingToFile (LoggingFile,__LINE__, 1, Pid, &LoginMessageAck);
                        Success = UnixDomainSocket_WriteFile ((HANDLE)conn_desc,                  // pipe handle
                                                    (void*)&LoginMessageAck,             // message
                                                    sizeof (LoginMessageAck),              // message length
                                                    &BytesWritten);             // bytes written
                        if (!Success) {
                            ThrowError (1, "Write to socket failed to acknowledge login \"%i\"", errno);
                        }
                    }
                } else if (LoginMessage->Command == PIPE_API_PING_CMD) {
                    // it is not a login it is a ping
                    PIPE_API_PING_MESSAGE_ACK PingAck;
                    PingAck.Command = PIPE_API_PING_CMD;
                    PingAck.ReturnValue = 0;
                    PingAck.Version = XILENV_VERSION;
                    PingAck.StructSize = sizeof (PingAck);
                    if (LoggingFlag) MessageLoggingToFile (LoggingFile ,__LINE__, 1, -1, &PingAck);
                    Success = UnixDomainSocket_WriteFile ((HANDLE)conn_desc,                  // pipe handle
                                                (void*)&PingAck,        // message
                                                sizeof (PingAck),       // message length
                                                &BytesWritten);          // bytes written
                    close(conn_desc);
                }
            }
        }
    }
    return 0;
}

static int UnixDomainSocket_CreateSoftcarLoginThread (char *par_Prefix)
{
    pthread_t Thread;
    pthread_attr_t Attr;

    static char StaticInstanceName[MAX_PATH];

    // This have to be copy to a static location because afterwards the scheduler thread will access this
    STRING_COPY_TO_ARRAY (StaticInstanceName, par_Prefix);

    pthread_attr_init(&Attr);
    if (pthread_create(&Thread, &Attr, UnixDomainSocket_SoftcarLoginThreadProc, (void*)StaticInstanceName) != 0) {
        ThrowError (1, "cannot create softcar login thread\n");
        return -1;
    }
    pthread_attr_destroy(&Attr);

    // wait until the login thread has called accept()
    for(int x = 0; (x < 10) && !ThreadWillCallFirstAcceptState; x++) {
        usleep(10*1000);
    }
    usleep(10*1000);

    return 0;
}

void sig_pipe_handler(void)
{
    printf ("SIGPIPE do nothing\n");
}

int UnixDomainSocket_InitMessages (char *par_Prefix, int par_LogingFlag)
{
    signal(SIGPIPE, (__sighandler_t)sig_pipe_handler);
    if (par_LogingFlag) {
        LoggingFlag = 1;
        LoggingFile = fopen ("/tmp/softcar_unix_domain_socket_logging.txt", "wt");
    }
    return UnixDomainSocket_CreateSoftcarLoginThread (par_Prefix);
}
#else
int dummy_function_no_empty_module(void)
{
    return 0;
}
#endif




