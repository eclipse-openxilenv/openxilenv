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
#include <winsock2.h>
#include <ws2tcpip.h>
#else
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
#define SOCKET int
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET  (SOCKET)(~0)
#define closesocket(s) close(s)
#endif
#include <stdlib.h>
#include <stdio.h>

#include "MemZeroAndCopy.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "RpcFuncBase.h"
#include "RpcFuncLogin.h"
#include "RpcClientSocket.h"

#ifdef __linux__ 
    //linux code goes here
    #define THREAD_LOCAL  __thread
#elif _WIN32
    #define RPC_PIPE_NAME  "XilEnvRemoteProcedureCall"
    #ifdef __GNUC__
        #define THREAD_LOCAL  __thread
    #else
        #define THREAD_LOCAL __declspec(thread)
    #endif
#else
    #error "no target defined"
#endif

#define MAX_RPC_THREAD_BUFFERS  64
static struct {
    unsigned int ThreadId;
    void *ReceiveBuffer;
    void *TransmitBuffer;
    int ReceiveBufferSize;
    int TransmitBufferSize;
#if 0
#ifdef _WIN32
    static HANDLE WaitForSem;
#else
    static sem_t WaitForSem;
#endif
#endif
} RpcThreadBuffers[MAX_RPC_THREAD_BUFFERS];
static int RpcThreadBuffersPos;

#ifdef _WIN32
static HANDLE RpcMutex;
#else
static pthread_mutex_t RpcMutex = PTHREAD_MUTEX_INITIALIZER;
#endif
static int RpcMutexInitFlag;

static void InitMutex()
{
    if (!RpcMutexInitFlag) {
        RpcMutexInitFlag = 1;
#ifdef _WIN32
        RpcMutex = CreateMutex(NULL, 0, NULL);
#else \
    // nothing todo
#endif
    }
}

static void LockMutex()
{
#ifdef _WIN32
    DWORD WaitResult = WaitForSingleObject (RpcMutex,    // handle to mutex
                                           INFINITE);    // no time-out interval
#else
    pthread_mutex_lock(&RpcMutex);
#endif
}

static void UnlockMutex()
{
#ifdef _WIN32
    ReleaseMutex (RpcMutex);
#else
    pthread_mutex_unlock(&RpcMutex);
#endif
}

static int GetThreadReceiveBufferNo(void)
{
    int x;
    int Ret = -1;
    unsigned int ThreadId = GetCurrentThreadId();
    LockMutex();
    for (x = 0; x < RpcThreadBuffersPos; x++) {
        if(RpcThreadBuffers[x].ThreadId == ThreadId) {
            Ret = x;
            goto __OUT;
        }
    }
    // check if an old entry can reuse
    for (x = 0; x < RpcThreadBuffersPos; x++) {
#ifdef _WIN32
        HANDLE ThreadHandle = OpenThread(READ_CONTROL, 0, RpcThreadBuffers[x].ThreadId);
        if (ThreadHandle  == NULL) {
            // Thread doesn't exist anymore -> reuse it
            Ret = x;
            goto __OUT;
        } else {
            CloseHandle(ThreadHandle);
        }
#else
        char ThreadProcFolderName[32];
        PrintFormatToString (ThreadProcFolderName, sizeof(ThreadProcFolderName), "/proc/self/%i", RpcThreadBuffers[x].ThreadId);
        struct stat s;
        if (stat(ThreadProcFolderName, &s) != 0) {
            // Thread doesn't exist anymore -> reuse it
            Ret = x;
            goto __OUT;
        }
#endif
    }
    // was not found add a new entry
    if (RpcThreadBuffersPos < MAX_RPC_THREAD_BUFFERS) {
        RpcThreadBuffers[RpcThreadBuffersPos].ThreadId = ThreadId;
#if 0
#ifdef _WIN32
        RpcThreadBuffers[RpcThreadBuffersPos].WaitForSem = CreateSemaphore(NULL, 1, 1, NULL);
#else
        sem_init(&RpcThreadBuffersPos[RpcThreadBuffersPos].WaitForSem, 0, 1);
#endif
#endif
        Ret = RpcThreadBuffersPos;
        RpcThreadBuffersPos++;
        goto __OUT;
    }
__OUT:
    UnlockMutex();
    return Ret;
}

#if 0
int SwapThreadReceiveBufferAndWakeup(int par_CurrntThreadNo, int par_WakeThreadNo)
{
    void *ReceiveBuffer;
    int ReceiveBufferSize;
    LockMutex();
    // swap the buffers
    ReceiveBuffer = RpcThreadBuffers[par_CurrntThreadNo].ReceiveBuffer;
    ReceiveBufferSize = RpcThreadBuffers[par_CurrntThreadNo].ReceiveBufferSize;
    RpcThreadBuffers[par_CurrntThreadNo].ReceiveBuffer =  RpcThreadBuffers[par_WakeThreadNo].ReceiveBuffer;
    RpcThreadBuffers[par_CurrntThreadNo].ReceiveBufferSize = RpcThreadBuffers[par_WakeThreadNo].ReceiveBufferSize;
    RpcThreadBuffers[par_WakeThreadNo].ReceiveBuffer = ReceiveBuffer;
    RpcThreadBuffers[par_WakeThreadNo].ReceiveBufferSize = ReceiveBufferSize;
#ifdef _WIN32
    ReleaseSemaphore(RpcThreadBuffers[par_WakeThreadNo].WaitForSem, 1, NULL);
#else
    sem_post(RpcThreadBuffers[par_WakeThreadNo].WaitForSem);
#endif
    UnlockMutex();
}
#endif

static void FreeAllThreadBuffers(void)
{
    int x;
    LockMutex();
    for (x = 0; x < RpcThreadBuffersPos; x++) {
        if (RpcThreadBuffers[x].ReceiveBuffer != NULL) free(RpcThreadBuffers[x].ReceiveBuffer);
        RpcThreadBuffers[x].ReceiveBuffer = NULL;
        RpcThreadBuffers[x].ReceiveBufferSize = 0;
        if (RpcThreadBuffers[x].TransmitBuffer != NULL) free(RpcThreadBuffers[x].TransmitBuffer);
        RpcThreadBuffers[x].TransmitBuffer = NULL;
        RpcThreadBuffers[x].TransmitBufferSize = 0;
    }
    RpcThreadBuffersPos = 0;
    UnlockMutex();
}


HANDLE ConnectToRemoteProcedureCallServer(char *par_ServerName, int par_Port)
{
    SOCKET Socket;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    int iResult;
    char PortString[32];
#ifdef _WIN32
    WSADATA wsaData;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        return INVALID_HANDLE_VALUE;
    }
#endif

    InitMutex();

    MEMSET(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    PrintFormatToString (PortString, sizeof(PortString), "%i", par_Port);
    // Resolve the server address and port
    iResult = getaddrinfo(par_ServerName, PortString, &hints, &result);
    if (iResult != 0) {
        return INVALID_HANDLE_VALUE;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        Socket = socket(ptr->ai_family, ptr->ai_socktype,  ptr->ai_protocol);
        if (Socket == INVALID_SOCKET) {
            freeaddrinfo(result);
            return INVALID_HANDLE_VALUE;
        }

        // Connect to the server
        iResult = connect (Socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(Socket);
            Socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (Socket == INVALID_SOCKET) {
        return INVALID_HANDLE_VALUE;
    }
    return (HANDLE)Socket;
}

#ifdef _WIN32
HANDLE NamedPipeConnectToRemoteProcedureCallServer(const char *par_ServerName, const char *par_Instance, int par_Timout_ms)
{
    int LoopCounter = 0;
    HANDLE hPipe;
    DWORD Status;
    DWORD Mode;
    char Pipename[MAX_PATH];

    InitMutex();

    PrintFormatToString (Pipename, sizeof(Pipename), "\\\\%s\\pipe\\" RPC_PIPE_NAME "_%s", (strlen(par_ServerName) == 0) ? "." : par_ServerName, par_Instance);
    while (1) {
        WaitNamedPipe(Pipename, 100);
        hPipe = CreateFile (Pipename,       // pipe name
                            GENERIC_READ |  // read and write access
                            GENERIC_WRITE,
                            0,              // no sharing
                            NULL,           // default security attributes
                            OPEN_EXISTING,  // opens existing pipe
                            0,              // default attributes
                            NULL);          // no template file

        // Break if the pipe handle is valid.
        if (hPipe != INVALID_HANDLE_VALUE) {
            break;
        }
        // Exit if an error other than ERROR_PIPE_BUSY occurs.
        switch (GetLastError()) {
        case ERROR_PIPE_BUSY:
        case ERROR_FILE_NOT_FOUND:
            if (LoopCounter > (par_Timout_ms / 100)) {
                return INVALID_HANDLE_VALUE;
            }
            Sleep (100);
            LoopCounter++;
            break;
        default:
            return INVALID_HANDLE_VALUE;
        }
    }

    Mode = PIPE_READMODE_BYTE;
    Status = SetNamedPipeHandleState (hPipe,    // pipe handle
                                      &Mode,    // new pipe mode
                                      NULL,     // don't set maximum bytes
                                      NULL);    // don't set maximum time
    if (!Status) {
        return INVALID_HANDLE_VALUE;
    }

    return hPipe;
}
#else
HANDLE UnixDomainSocketConnectToRemoteProcedureCallServer(const char *par_ServerName, const char *par_Instance, int par_Timout_ms)
{
    SOCKET Socket;
    struct sockaddr_un address;
    int iResult;
    char Name[MAX_PATH + 100];

    MEMSET(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;

    if (CheckOpenIPCFile(par_Instance, "unix_domain_rpc", Name, sizeof(Name), DIR_MUST_EXIST, FILENAME_MUST_EXIST) != 0) {
        return INVALID_HANDLE_VALUE;
    }
    STRING_COPY_TO_ARRAY(address.sun_path, Name);

    // Create a SOCKET for connecting to server
    Socket = socket(PF_UNIX, SOCK_STREAM, 0);
    if (Socket == INVALID_SOCKET) {
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
#endif

void DisconnectFromRemoteProcedureCallServer(int par_SocketOrNamedPipe, HANDLE Socket)
{
    if (par_SocketOrNamedPipe) {
        closesocket((SOCKET)Socket);
#ifdef _WIN32
    } else {
        CloseHandle(Socket);
#endif
    }
    FreeAllThreadBuffers();
}


int SentToRemoteProcedureCallServer (int par_SocketOrNamedPipe, HANDLE par_Socket, int par_Command, RPC_API_BASE_MESSAGE *par_Req, int par_PackageSize)
{
    int buffer_pos = 0;

    par_Req->StructSize = par_PackageSize;
    par_Req->Command = par_Command;
    do {
        int SendBytes;
#ifdef _WIN32
        if (par_SocketOrNamedPipe) {
#endif
            SendBytes = send ((SOCKET)par_Socket, (const char*)par_Req + buffer_pos, par_PackageSize - buffer_pos, 0);
#ifdef _WIN32
        } else {
            DWORD NumberOfBytesWritten;
            BOOL Ret;
            Ret = WriteFile((HANDLE)par_Socket, (void*)((const char*)par_Req + buffer_pos), (DWORD)(par_PackageSize - buffer_pos), &NumberOfBytesWritten, NULL);
            if (Ret == 0) SendBytes = SOCKET_ERROR;
            else SendBytes = NumberOfBytesWritten;
        }
#endif
        if (SendBytes == SOCKET_ERROR) {
            return -1;
        }
        buffer_pos += SendBytes;
    } while (buffer_pos < par_PackageSize);

    return 0;
}

static int ReceiveFromRemoteProcedureCallServer (int par_SocketOrNamedPipe, HANDLE par_Socket, RPC_API_BASE_MESSAGE_ACK *ret_Data, int par_len)
{
    int receive_message_size = 0;
    int buffer_pos = 0;
    receive_message_size = 0;
    do {
        int ReadBytes;
__RETRAY:
#ifdef _WIN32
        if (par_SocketOrNamedPipe) {
#endif
            ReadBytes = recv ((SOCKET)par_Socket, (char*)ret_Data + buffer_pos, par_len - buffer_pos, 0);
#ifdef _WIN32
        } else {
            BOOL Ret;
            DWORD BytesRead;
            Ret = ReadFile ((HANDLE)par_Socket, (char*)ret_Data + buffer_pos, par_len - buffer_pos, &BytesRead, NULL);
            if (Ret == 0) ReadBytes = 0;
            else ReadBytes = BytesRead;
        }
#endif
        if (ReadBytes > 0) {
            if ((ReadBytes == sizeof(RPC_API_PING_TO_CLINT_MESSAGE)) && (ret_Data->Command == RPC_API_PING_TO_CLINT_CMD)) {
                // answer to the alive ping
                RPC_API_PING_TO_CLINT_MESSAGE_ACK Ack;
                MEMSET (&Ack, 0, sizeof(Ack));
                if (SentToRemoteProcedureCallServer (par_SocketOrNamedPipe, par_Socket, RPC_API_PING_TO_CLINT_CMD, (RPC_API_BASE_MESSAGE*)&Ack, sizeof(Ack)) != 0) {
                    return -1;
                }
                goto __RETRAY;
            } else {
                buffer_pos += ReadBytes;
                if (receive_message_size == 0) {
                    receive_message_size = (int)*(uint32_t*)ret_Data;
                }
            }
        } else if (ReadBytes == 0) {
            // can this happen?
        } else {
            return -1;
        }
    } while (buffer_pos < receive_message_size);
    return receive_message_size;
}

int RemoteProcedureCallTransact (int par_SocketOrNamedPipe, HANDLE par_Socket, uint32_t par_Command, 
                                 RPC_API_BASE_MESSAGE *par_Req,
                                 int par_BytesToWrite,
                                 RPC_API_BASE_MESSAGE_ACK *ret_Ack,
                                 int par_MaxReceiveBytes)
{
    int Ret;

    LockMutex();
    if (SentToRemoteProcedureCallServer (par_SocketOrNamedPipe, par_Socket, par_Command, par_Req, par_BytesToWrite) == 0) {
        if ((Ret = ReceiveFromRemoteProcedureCallServer (par_SocketOrNamedPipe, par_Socket, ret_Ack, par_MaxReceiveBytes)) > 0) {
            UnlockMutex();
            return Ret;
        }
    }
    UnlockMutex();
    return -1;
}

void *RemoteProcedureGetReceiveBuffer(int par_NeededSize)
{
    int BufferNo = GetThreadReceiveBufferNo();
    if (BufferNo < 0) return NULL;
    if (par_NeededSize > RpcThreadBuffers[BufferNo].ReceiveBufferSize) {
        RpcThreadBuffers[BufferNo].ReceiveBufferSize = par_NeededSize;
        RpcThreadBuffers[BufferNo].ReceiveBuffer = realloc (RpcThreadBuffers[BufferNo].ReceiveBuffer, RpcThreadBuffers[BufferNo].ReceiveBufferSize);
        if (RpcThreadBuffers[BufferNo].ReceiveBuffer == NULL) {
            RpcThreadBuffers[BufferNo].ReceiveBufferSize = 0;
        }
    }
    return RpcThreadBuffers[BufferNo].ReceiveBuffer;
}

void *RemoteProcedureGetTransmitBuffer(int par_NeededSize)
{
    int BufferNo = GetThreadReceiveBufferNo();
    if (BufferNo < 0) return NULL;
    if (par_NeededSize > RpcThreadBuffers[BufferNo].TransmitBufferSize) {
        RpcThreadBuffers[BufferNo].TransmitBufferSize = par_NeededSize;
        RpcThreadBuffers[BufferNo].TransmitBuffer = realloc (RpcThreadBuffers[BufferNo].TransmitBuffer, RpcThreadBuffers[BufferNo].TransmitBufferSize);
        if (RpcThreadBuffers[BufferNo].TransmitBuffer == NULL) {
            RpcThreadBuffers[BufferNo].TransmitBufferSize = 0;
        }
    }
    return RpcThreadBuffers[BufferNo].TransmitBuffer;
}

int ReceiveFromRemoteProcedureCallServerDynBuf (int par_SocketOrNamedPipe, HANDLE par_Socket, RPC_API_BASE_MESSAGE_ACK **ret_ptrToData)
{
    char *dyn_buffer;
    int receive_message_size = 0;
    int buffer_pos = 0;

    dyn_buffer = (char*)RemoteProcedureGetReceiveBuffer(sizeof(RPC_API_BASE_MESSAGE_ACK));
    if (dyn_buffer == NULL) return -1;

    // First read only the header
    do {
        int ReadBytes;
__RETRAY:
#ifdef _WIN32
        if (par_SocketOrNamedPipe) {
#endif
            ReadBytes = recv ((SOCKET)par_Socket, dyn_buffer + buffer_pos, sizeof(RPC_API_BASE_MESSAGE_ACK) - buffer_pos, 0);
#ifdef _WIN32
        } else {
            BOOL Ret;
            DWORD BytesRead;
            Ret = ReadFile ((HANDLE)par_Socket, dyn_buffer + buffer_pos, sizeof(RPC_API_BASE_MESSAGE_ACK) - buffer_pos, &BytesRead, NULL);
            if (Ret == 0) ReadBytes = 0;
            else ReadBytes = BytesRead;
        }
#endif
        if (ReadBytes > 0) {
            if ((ReadBytes == sizeof(RPC_API_PING_TO_CLINT_MESSAGE)) && (((RPC_API_BASE_MESSAGE_ACK*)dyn_buffer)->Command == RPC_API_PING_TO_CLINT_CMD)) {
                // answer to the alive ping
                RPC_API_PING_TO_CLINT_MESSAGE_ACK Ack;
                MEMSET (&Ack, 0, sizeof(Ack));
                if (SentToRemoteProcedureCallServer (par_SocketOrNamedPipe, par_Socket, RPC_API_PING_TO_CLINT_CMD, (RPC_API_BASE_MESSAGE*)&Ack, sizeof(Ack)) != 0) {
                    return -1;
                }
                goto __RETRAY;
            } else {
                buffer_pos += ReadBytes;
                if (receive_message_size == 0) {
                    receive_message_size = *(uint32_t*)dyn_buffer;
                }
            }
        } else {
            return -1;
        }
    } while (buffer_pos < (int)sizeof(RPC_API_BASE_MESSAGE_ACK));

    // Read the rest of the message into the dynamic part
    dyn_buffer = (char*)RemoteProcedureGetReceiveBuffer(receive_message_size);
    if (dyn_buffer == NULL) return -1;

    buffer_pos = sizeof(RPC_API_BASE_MESSAGE_ACK);

    do {
        int ReadBytes;
#ifdef _WIN32
        if (par_SocketOrNamedPipe) {
#endif
            ReadBytes = recv ((SOCKET)par_Socket, dyn_buffer + buffer_pos, receive_message_size - buffer_pos, 0);
#ifdef _WIN32
        } else {
            BOOL Ret;
            DWORD BytesRead;
            Ret = ReadFile ((HANDLE)par_Socket, dyn_buffer + buffer_pos, receive_message_size - buffer_pos, &BytesRead, NULL);
            if (Ret == 0) ReadBytes = 0;
            else ReadBytes = BytesRead;
        }
#endif
        if (ReadBytes > 0) {
            buffer_pos += ReadBytes;
        } else {
            return -1;
        }
    } while (buffer_pos < receive_message_size);

    *ret_ptrToData = (RPC_API_BASE_MESSAGE_ACK*)dyn_buffer;
    return receive_message_size;
}


int RemoteProcedureCallTransactDynBuf (int par_SocketOrNamedPipe, HANDLE par_Socket, int par_Command, RPC_API_BASE_MESSAGE *par_TransmitData, int par_BytesToTransmit, RPC_API_BASE_MESSAGE_ACK **ret_ReceiveData)
{
    int Ret;

    LockMutex();
    if (SentToRemoteProcedureCallServer (par_SocketOrNamedPipe, par_Socket, par_Command, par_TransmitData, par_BytesToTransmit) == 0) {
        if ((Ret = ReceiveFromRemoteProcedureCallServerDynBuf (par_SocketOrNamedPipe, par_Socket, ret_ReceiveData)) > 0) {
            UnlockMutex();
            return Ret;
        } else {
            UnlockMutex();
            return -1;
        }
    } else {
        UnlockMutex();
        return -1;
    }
    UnlockMutex();
    return -1;
}

#define UNUSED(x) (void)(x)

void *__my_malloc (const char * const file, int line, size_t size)
{
    UNUSED(file);
    UNUSED(line);
    return malloc (size);
}

void *__my_calloc (const char * const file, int line, size_t nitems, size_t size)
{
    UNUSED(file);
    UNUSED(line);
    return calloc (nitems, size);
}

void my_free (void* block)
{
    free (block);
}

void *__my_realloc(const char * const file, int line, void *block, size_t size)
{
    UNUSED(file);
    UNUSED(line);
    return realloc (block, size);
}

int ThrowError(int level, const char *format, ...)
{
    UNUSED(level);
    UNUSED(format);
    return 0;
}
