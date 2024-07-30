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


#define WIN32_LEAN_AND_MEAN
#include <stdint.h>
#include "Platform.h"
#ifdef _WIN32
#include <WinSock2.h>
#include <iphlpapi.h>
#include <WS2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#define SOCKET int
#endif
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ConfigurablePrefix.h"

#include "RemoteMasterCopyStartExecutable.h"

#ifdef _WIN32
// wegen SendARP
#pragma comment(lib, "iphlpapi.lib")
#endif

#ifdef _WIN32
#define MY_SOCKET UINT_PTR
#else
#define MY_SOCKET int
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define WSAGetLastError() strerror(errno)
#define closesocket(s) close(s)
#define Sleep(t) usleep(t * 1000)
#endif

#define BUFFER_SIZE (1024 * 1024)
#define PROTOCOL_VERSION  1
#define COMMAND_PING                   0
#define COMMAND_IS_RUNNING             1
#define COMMAND_COPY_FILE_START        2
#define COMMAND_COPY_FILE_NEXT         3
#define COMMAND_COPY_FILE_END          4
#define COMMAND_EXECUTE_COMMAND        5
#define COMMAND_EXECUTE_FILE           6
#define COMMAND_KILL_PROCESS           7
#define COMMAND_CLOSE_CONNECTION       8

typedef struct {
    uint32_t Size;
    uint32_t Version;
    uint32_t Command;
    uint32_t Counter;
    uint32_t DataSize;
    uint32_t Filler;
    char Data[8];           // remark: this can be longer than 8 byte
} PACKAGE_HEADER_REQ;

typedef struct {
    uint32_t Size;
    uint32_t Version;
    uint32_t Command;
    uint32_t Counter;
    uint32_t Ret;
    uint32_t Filler;
    char ErrorString[8];     // remark: this can be longer than 8 byte
} PACKAGE_HEADER_ACK;


static int SentToRemoteMasterStartService (SOCKET Socket, PACKAGE_HEADER_REQ *par_Data, int par_PackageSize)
{
    int buffer_pos = 0;
    do {
        int SendBytes = send (Socket, (const char*)par_Data + buffer_pos, par_PackageSize - buffer_pos, 0);
#ifdef _WIN32
        if (SendBytes == SOCKET_ERROR) {
            ThrowError (1, "send failed with error: %d\n", WSAGetLastError());
            return -1;
        }
#else
        if (SendBytes <= 0) {
            ThrowError (1, "send failed with error: %d\n", errno);
            return -1;
        }
#endif
        buffer_pos += SendBytes;
    } while (buffer_pos < par_PackageSize);

    return 0;
}

static int ReceiveFromeRemoteMasterStartService (SOCKET Socket, PACKAGE_HEADER_ACK *ret_Data, int par_len)
{
    int buffer_pos = 0;
    int receive_message_size = 0;
    do {
        int ReadBytes = recv (Socket, (char*)ret_Data + buffer_pos, par_len - buffer_pos, 0);
        if (ReadBytes > 0) {
            buffer_pos += ReadBytes;
            if (receive_message_size == 0) {
                receive_message_size = (int)*(uint32_t*)ret_Data;
            }
        } else if (ReadBytes == 0) {
            //printf ("0Bytes\n");
        } else {
#ifdef _WIN32
            ThrowError (1, "recv failed with error: %d\n", WSAGetLastError());
#else
            ThrowError (1, "recv failed with error: %d\n", errno);
#endif
            return -1;
        }
    } while (buffer_pos < receive_message_size);
    return receive_message_size;
}

static int TransactRemoteMasterStartService (SOCKET Socket, uint32_t par_Command, uint32_t par_Counter, uint32_t par_DataSize, const void *par_Data,
                                             PACKAGE_HEADER_REQ *Req,
                                             PACKAGE_HEADER_ACK *Ack,
                                             char *ret_ErrString, uint32_t par_MaxErrSize)
{
    int Ret;
    uint32_t PackageSize = par_DataSize + sizeof (PACKAGE_HEADER_REQ) - 8;
    Req->Size = (uint32_t)PackageSize;
    Req->DataSize = par_DataSize;
    Req->Command =par_Command;
    Req->Version = PROTOCOL_VERSION;
    Req->Counter = (uint32_t)par_Counter;
    MEMCPY(Req->Data, par_Data, par_DataSize);

    if (SentToRemoteMasterStartService (Socket, Req, (int)PackageSize) == 0) {
        if ((Ret = ReceiveFromeRemoteMasterStartService (Socket, Ack, (int)(par_MaxErrSize + sizeof (PACKAGE_HEADER_ACK) - 8))) > 0) {
            if (Ack->Ret != 0) {
                uint32_t i;
                uint32_t Len = (uint32_t)Ret - ((uint32_t)sizeof (PACKAGE_HEADER_ACK) - 8U);
                if (par_MaxErrSize < Len) Len = par_MaxErrSize;
                for (i = 0; (i < (Len - 1)) && (Ack->ErrorString[i] != 0); i++) {
                    ret_ErrString[i] = Ack->ErrorString[i];
                }
                ret_ErrString[i] = 0;
                return -1;
            } else {
                if (par_Counter != Ack->Counter) {
                    sprintf(ret_ErrString, "Req Counter != Ack Counter (%u != %u)", par_Counter, Ack->Counter);
                    return -1;
                } else {
                    if (par_Command != Ack->Command) {
                        sprintf(ret_ErrString, "Req Command != Ack Command (%u != %u)", par_Counter, Ack->Command);
                        return -1;
                    } else {
                        ret_ErrString[0] = 0;
                        return 0;   // all OK
                    }
                }
            }
        } else {
            sprintf(ret_ErrString, "receive error");
            return -1;
        }
    } else {
        sprintf(ret_ErrString, "transmit error");
        return -1;
    }
}

#define FILE_BUFFER_SIZE (64*1024)

int RemoteMasterCopyAndStartExecutable(const char *par_ExecutableNameClient, const char *par_ExecutableNameServer,
                                       const char *par_SharedLibraryDir,
                                       const char *par_RemoteMasterAddr, int par_RemoteMasterPort, int par_ExecFlag)
{
    MY_SOCKET Socket;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    int iResult;
    char PortString[32];
    PACKAGE_HEADER_REQ *Req;
    PACKAGE_HEADER_ACK *Ack;
    char ErrorString[1024];
    uint32_t Counter;
    int RetOfCall;
    HANDLE hFileExe;
    void *FileBuffer;
    int Ret = -1;

    Socket = INVALID_SOCKET;
    hFileExe = INVALID_HANDLE_VALUE;
    Req = NULL;
    Ack = NULL;
    FileBuffer = my_malloc(FILE_BUFFER_SIZE);
    if (FileBuffer == NULL) goto __ERROR;

    hFileExe = CreateFile (par_ExecutableNameClient,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL_INT_OR_PTR);
    if (hFileExe == (HANDLE)HFILE_ERROR) {
        ThrowError (1, "Unable to open %s file\n", par_ExecutableNameClient);
        goto __ERROR;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    sprintf (PortString, "%i", par_RemoteMasterPort);
    // Resolve the server address and port
    iResult = getaddrinfo(par_RemoteMasterAddr, PortString, &hints, &result);
    if (iResult != 0) {
        ThrowError (1, "getaddrinfo failed with error: %d (%s)\n", iResult, gai_strerror(iResult));
        goto __ERROR;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        Socket = socket(ptr->ai_family, ptr->ai_socktype,  ptr->ai_protocol);
        if (Socket == INVALID_SOCKET) {
            ThrowError (1, "socket failed with error: %ld\n", WSAGetLastError());
            goto __ERROR;
        }

        // Connect to server.
        iResult = connect (Socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(Socket);
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (Socket == INVALID_SOCKET) {
        ThrowError (1, "Unable to connect to server!\n");
        goto __ERROR;
    }

    Req = my_malloc(BUFFER_SIZE);
    Ack = my_malloc(BUFFER_SIZE);

    if ((Req == NULL) || (Ack == NULL)) goto __ERROR;

    Counter = 0;
    RetOfCall = TransactRemoteMasterStartService (Socket, COMMAND_PING, Counter, 0, NULL, Req, Ack, ErrorString, sizeof(ErrorString));
    if (RetOfCall) {
        ThrowError (1, "cannot ping to remote server \"%s\" error: %s", par_RemoteMasterAddr, ErrorString);
        goto __ERROR;
    } else {
        Counter++;
        RetOfCall = TransactRemoteMasterStartService (Socket, COMMAND_IS_RUNNING, Counter, 0, NULL, Req, Ack, ErrorString, sizeof(ErrorString));
        if (RetOfCall) {
            ThrowError (1, "%s already running error: %s", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME), ErrorString);
            goto __ERROR;
        } else {
            Counter++;
            RetOfCall = TransactRemoteMasterStartService (Socket, COMMAND_COPY_FILE_START, Counter,
                                                          (uint32_t)strlen(par_ExecutableNameServer) + 1, par_ExecutableNameServer,
                                                          Req, Ack, ErrorString, sizeof(ErrorString));
            if (RetOfCall) {
                ThrowError (1, "cannot open \"%s\" on target \"%s\": %s", par_ExecutableNameServer, par_RemoteMasterAddr, ErrorString);
                goto __ERROR;
            } else {
                DWORD NumberOfBytesRead;
                Counter = 0;
                while (1) {
                    if (!ReadFile (hFileExe, FileBuffer, FILE_BUFFER_SIZE, &NumberOfBytesRead, NULL)) {
                        CloseHandle (hFileExe);
                        ThrowError (1, "cannot read from executable file %s", par_ExecutableNameServer);
                        goto __ERROR;
                    }
                    if (NumberOfBytesRead == 0) break; // End of file
                    RetOfCall = TransactRemoteMasterStartService (Socket, COMMAND_COPY_FILE_NEXT, Counter,
                                                      NumberOfBytesRead, FileBuffer,
                                                      Req, Ack, ErrorString, sizeof(ErrorString));
                    if (RetOfCall) {
                        ThrowError (1, "cannot copy part %i of \"%s\" on target \"%s\": %s", Counter, par_ExecutableNameServer, par_RemoteMasterAddr, ErrorString);
                        goto __ERROR;
                    }
                    Counter++;
                }
                Counter++;
                RetOfCall = TransactRemoteMasterStartService (Socket, COMMAND_COPY_FILE_END, Counter,
                                                  0, NULL,
                                                  Req, Ack, ErrorString, sizeof(ErrorString));
                if (RetOfCall) {
                    ThrowError (1, "cannot end copy \"%s\" on target \"%s\": %s", par_ExecutableNameServer, par_RemoteMasterAddr, ErrorString);
                    goto __ERROR;
                } else {
                    Counter++;
                    sprintf (FileBuffer, "chmod gou+x %s", par_ExecutableNameServer);
                    RetOfCall = TransactRemoteMasterStartService (Socket, COMMAND_EXECUTE_COMMAND, Counter,
                                                      (uint32_t)strlen(FileBuffer) + 1, FileBuffer,
                                                      Req, Ack, ErrorString, sizeof(ErrorString));
                    if (RetOfCall) {
                        ThrowError (1, "execute \"%s\" on target \"%s\": %s", FileBuffer, par_RemoteMasterAddr, ErrorString);
                        goto __ERROR;
                    } else if (par_ExecFlag) {
                        Counter++;
                        sprintf (FileBuffer, "export LD_LIBRARY_PATH=%s;%s", par_SharedLibraryDir, par_ExecutableNameServer);
                        RetOfCall = TransactRemoteMasterStartService (Socket, COMMAND_EXECUTE_FILE, Counter,
                                                          (uint32_t)strlen(FileBuffer) + 1, FileBuffer,
                                                          Req, Ack, ErrorString, sizeof(ErrorString));
                        if (RetOfCall) {
                            ThrowError (1, "execute \"%s\" on target \"%s\": %s", FileBuffer, par_RemoteMasterAddr, ErrorString);
                            goto __ERROR;
                        } else {
                            Sleep (1000);
                        }

                    }
                }

            }

        }
    }
    Ret = 0; // all Ok
__ERROR:
    if ((Req != NULL) && (Socket != INVALID_SOCKET)) {
        Req->Size = sizeof(PACKAGE_HEADER_REQ);
        Req->DataSize = 0;
        Req->Command = COMMAND_CLOSE_CONNECTION;
        Req->Version = PROTOCOL_VERSION;
        Req->Counter = 0;
        SentToRemoteMasterStartService (Socket, Req, (int)Req->Size);
    }
    if (Req != NULL) my_free (Req);
    if (Ack != NULL) my_free (Ack);
    if (FileBuffer != NULL)  my_free(FileBuffer);
    if (hFileExe != INVALID_HANDLE_VALUE) CloseHandle(hFileExe);
    if (Socket != INVALID_SOCKET) closesocket(Socket);
    return Ret;
}


int RemoteMasterExecuteCommand(char *par_RemoteMasterAddr, int par_RemoteMasterPort, const char *par_Command, int par_SilentFlag)
{
    MY_SOCKET Socket;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    int iResult;
    char PortString[32];
    PACKAGE_HEADER_REQ *Req;
    PACKAGE_HEADER_ACK *Ack;
    char ErrorString[1024];
    uint32_t Counter = 0;
    int Ret = -1;
    Socket = INVALID_SOCKET;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    Req = my_malloc(BUFFER_SIZE);
    Ack = my_malloc(BUFFER_SIZE);

    if ((Req == NULL) || (Ack == NULL)) goto __ERROR;

    sprintf (PortString, "%i", par_RemoteMasterPort);
    // Resolve the server address and port
    iResult = getaddrinfo(par_RemoteMasterAddr, PortString, &hints, &result);
    if (iResult != 0) {
        ThrowError (1, "getaddrinfo failed with error: %d (%s)\n", iResult, gai_strerror(iResult));
        goto __ERROR;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        Socket = socket(ptr->ai_family, ptr->ai_socktype,  ptr->ai_protocol);
        if (Socket == INVALID_SOCKET) {
            ThrowError (1, "socket failed with error: %ld\n", WSAGetLastError());
            goto __ERROR;
        }

        // Connect to server.
        iResult = connect (Socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(Socket);
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (Socket == INVALID_SOCKET) {
        ThrowError (1, "Unable to connect to server!\n");
        goto __ERROR;
    }

    Ret = TransactRemoteMasterStartService (Socket, COMMAND_EXECUTE_COMMAND, Counter, (uint32_t)strlen(par_Command)+1, par_Command, Req, Ack, ErrorString, sizeof(ErrorString));
    if (Ret) {
        if (!par_SilentFlag) ThrowError (1, "cannot execute command \"%s\" on remote server \"%s (%i)\" error: %s", par_Command, par_RemoteMasterAddr, par_RemoteMasterPort, ErrorString);
        goto __ERROR;
    }

    Ret = 0; // all Ok

__ERROR:
    if ((Req != NULL) && (Socket != INVALID_SOCKET)) {
        Req->Size = sizeof(PACKAGE_HEADER_REQ);
        Req->DataSize = 0;
        Req->Command = COMMAND_CLOSE_CONNECTION;
        Req->Version = PROTOCOL_VERSION;
        Req->Counter = 0;
        SentToRemoteMasterStartService (Socket, Req, (int)Req->Size);
    }
    if (Req != NULL) my_free (Req);
    if (Ack != NULL) my_free (Ack);
    if (Socket != INVALID_SOCKET) closesocket(Socket);
    return Ret;
}

int RemoteMasterSyncDateTime(char *par_RemoteMasterAddr, int par_RemoteMasterPort)
{
#ifdef _WIN32
    char CommandLine[1024];
    SYSTEMTIME SystemTime;
    int Ret;

    sprintf (CommandLine, "date --set=");
    GetLocalTime(&SystemTime);
    Ret = GetDateFormat (LOCALE_USER_DEFAULT,
                   0,
                   &SystemTime,
                   "yyyy-MM-dd",
                   CommandLine + strlen(CommandLine),
                   (int)(sizeof(CommandLine) - strlen(CommandLine)));
    strcat(CommandLine, "T");
    Ret = GetTimeFormat (LOCALE_USER_DEFAULT,
                   TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT,
                   &SystemTime,
                   "HH:mm:ss",
                   CommandLine + strlen(CommandLine),
                   (int)(sizeof(CommandLine) - strlen(CommandLine)));

    Ret =  RemoteMasterExecuteCommand(par_RemoteMasterAddr, par_RemoteMasterPort, CommandLine, 0);
    sprintf (CommandLine, "hwclock --systohc");
    Ret =  RemoteMasterExecuteCommand(par_RemoteMasterAddr, par_RemoteMasterPort, CommandLine, 0);
    return Ret;
#else
    return 0;
#endif
}

int RemoteMasterKillAll(char *par_RemoteMasterAddr, int par_RemoteMasterPort, const char *par_Name)
{
    char CommandLine[1024];
    int Ret;
    const char *p;

    p = par_Name;
    while (*p != 0) p++;
    while ((p > par_Name) && (*p != '/')) p--;
    if (*p == '/') p++;

    sprintf (CommandLine, "killall -q %.15s", p);

    Ret =  RemoteMasterExecuteCommand(par_RemoteMasterAddr, par_RemoteMasterPort, CommandLine, 1);
    return Ret;
}


int RemoteMasterRemoveVarLogMessages(char *par_RemoteMasterAddr, int par_RemoteMasterPort)
{
    char CommandLine[64];
    int Ret;

    sprintf (CommandLine, "rm /var/log/messages");

    Ret =  RemoteMasterExecuteCommand(par_RemoteMasterAddr, par_RemoteMasterPort, CommandLine, 1);
    return Ret;
}

int RemoteMasterPing(char *par_RemoteMasterAddr, int par_RemoteMasterPort)
{
    MY_SOCKET Socket;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    int iResult;
    char PortString[32];
    PACKAGE_HEADER_REQ Req;
    PACKAGE_HEADER_ACK Ack;
    char ErrorString[1024];
    uint32_t Counter = 0;
    int Ret = -1;
    Socket = INVALID_SOCKET;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    sprintf (PortString, "%i", par_RemoteMasterPort);
    // Resolve the server address and port
    iResult = getaddrinfo(par_RemoteMasterAddr, PortString, &hints, &result);
    if (iResult != 0) {
        ThrowError (1, "getaddrinfo failed with error: %d (%s)\n", iResult, gai_strerror(iResult));
        goto __ERROR;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        Socket = socket(ptr->ai_family, ptr->ai_socktype,  ptr->ai_protocol);
        if (Socket == INVALID_SOCKET) {
            ThrowError (1, "socket failed with error: %ld\n", WSAGetLastError());
            goto __ERROR;
        }

        // Connect to server.
        iResult = connect (Socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(Socket);
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (Socket == INVALID_SOCKET) {
        ThrowError (1, "Unable to connect to server!\n");
        goto __ERROR;
    }
    Ret = TransactRemoteMasterStartService (Socket, COMMAND_PING, Counter, 0, ErrorString, &Req, &Ack, ErrorString, sizeof(ErrorString));
    if (Ret) {
        goto __ERROR;
    }

    Ret = 0; // all Ok

__ERROR:
    if (Socket != INVALID_SOCKET) {
        Req.Size = sizeof(PACKAGE_HEADER_REQ);
        Req.DataSize = 0;
        Req.Command = COMMAND_CLOSE_CONNECTION;
        Req.Version = PROTOCOL_VERSION;
        Req.Counter = 0;
        SentToRemoteMasterStartService (Socket, &Req, (int)Req.Size);
    }
    if (Socket != INVALID_SOCKET) closesocket(Socket);
    return Ret;
}

int RemoteMasterShutdown(char *par_RemoteMasterAddr, int par_RemoteMasterPort)
{
    char CommandLine[1024];
    int Ret;

    sprintf (CommandLine, "shutdown --poweroff now");

    Ret =  RemoteMasterExecuteCommand(par_RemoteMasterAddr, par_RemoteMasterPort, CommandLine, 0);

    return Ret;

}
