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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Platform.h"
#ifdef _WIN32
#include <direct.h>  //  because getcwd
#if (_MSC_VER <= 1600)
// Till Visual Studio 2010 the psapi must be dynamiclly loaded
#define DYNAMIC_LOAD_PSAPI_DLL
#endif

#include <Psapi.h>   // because GetModuleInformation
#endif

#include "PipeMessagesShared.h"
#include "XilEnvExtProc.h"
#include "StringMaxChar.h"
#include "ExtpProcessAndTaskInfos.h"
#include "ExtpBlackboardCopyLists.h"
#ifdef EXTP_CONFIG_HAVE_KILL_EVENT
#include "ExtpKillExternProcessEvent.h"
#endif
#ifdef EXTP_CONFIG_HAVE_COMMANDLINE
#include "ExtpParseCmdLine.h"
#endif
#include "ExtpBlackboard.h"
#include "ExtpReferenceVariables.h"
#ifdef EXTP_CONFIG_HAVE_XCP
#include "ExtpXcpCopyPages.h"
#endif
#ifdef _WIN32
#include "ExtpPipeMessages.h"
#endif
#ifdef EXTP_CONFIG_HAVE_SOCKETS
#include "ExtpSocketMessages.h"
#endif
#ifdef EXTP_CONFIG_HAVE_QEMU_SIL_CON
#include "QemuMessages.h"
#endif
#include "ExtpVirtualNetwork.h"
#include "ExtpMemoryAllocation.h"
#include "ExtpError.h"
#include "ExtpUnixDomainSocketMessages.h"
#include "ExtpBaseMessages.h"

//#define DEBUG_COMMANDLINE
//#define WITH_ALIVE_PING_EVENTS

#define EnterProtectionAgainXCPAccess(a)
#define LeaveProtectionAgainXCPAccess(a)

#define ENVIRONMENT_VARNAME_CALLFROM    "ExternalProcess_CallFrom"
#define ENVIRONMENT_VARNAME_INSTANCE    "ExternalProcess_Instance"

#if defined(_WIN32) || defined(__linux__)
int get_image_base_and_size (unsigned long *ret_base_address, unsigned long *ret_size);

int XilEnvInternal_TryAndCatchWriteToMemCopy (void *Dst, void *Src, int Size);
int XilEnvInternal_TryAndCatchReadFromMemCopy (void *Dst, void *Src, int Size);
#else
int XilEnvInternal_TryAndCatchWriteToMemCopy (void *Dst, void *Src, int Size)
{
	MEMCPY(Dst, Src, Size);
	return 0;
}
int XilEnvInternal_TryAndCatchReadFromMemCopy (void *Dst, void *Src, int Size)
{
	MEMCPY(Dst, Src, Size);
	return 0;
}
#endif

static EXTERN_PROCESS_INFOS_STRUCT ProcessInfos;

 EXTERN_PROCESS_INFOS_STRUCT *XilEnvInternal_GetGlobalProcessInfos (void)
 {
     return  &ProcessInfos;
 }

void XilEnvInternal_LockProcessPtr (EXTERN_PROCESS_INFOS_STRUCT* ProcessInfos)
{
    if (ProcessInfos != NULL) {
#ifdef _WIN32
        DWORD WaitResult = WaitForSingleObject (ProcessInfos->ProcessMutex,    // handle to mutex
                                                INFINITE);         // no time-out interval
#elif defined(__linux__)
        pthread_mutex_lock(&ProcessInfos->ProcessMutex);
#endif
    }
}

void XilEnvInternal_ReleaseProcessPtr (EXTERN_PROCESS_INFOS_STRUCT* ProcessInfos)
{
    if (ProcessInfos != NULL) {
#ifdef _WIN32
        ReleaseMutex (ProcessInfos->ProcessMutex);
#elif defined(__linux__)
        pthread_mutex_unlock(&ProcessInfos->ProcessMutex);
#endif
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ SetExternalProcessExecutableName(const char *par_ExecutableName)
{
    strncpy(ProcessInfos.ExecutableName, par_ExecutableName, sizeof(ProcessInfos.ExecutableName) - 1);
    ProcessInfos.UserSetExecutableName = 1;   // set to 1 to avoid calling GetModuleName
}

 int XilEnvInternal_GetNoGuiFlag(void)
{
    return ProcessInfos.NoGuiFlag;
}

int XilEnvInternal_GetErr2MsgFlag(void)
{
    return ProcessInfos.Err2Msg;
}

int XilEnvInternal_GetNoXcpFlag(void)
{
    return ProcessInfos.NoXcpFlag;
}

void KillExternProcessHimSelf (void)
{
    exit(0);
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ SetHwndMainWindow (void *Hwnd)
{
#ifdef _WIN32
    ProcessInfos.HwndMainWindow = (HWND)Hwnd;
#endif
}

int get_process_identifier (void)
{
#if defined(_WIN32) || defined(__linux__)
    int x, c = 0;
    DWORD ThreadID = GetCurrentThreadId ();
    for (x = 0; x < MAX_PROCESSES_INSIDE_ONE_EXECUTABLE; x++) {
        EXTERN_PROCESS_TASK_INFOS_STRUCT * volatile TaskInfos;
        TaskInfos = ProcessInfos.TasksInfos[x];
        if (TaskInfos != NULL) {
            c++;
            if (TaskInfos->ThreadID == ThreadID) {
                return TaskInfos->sc_process_identifier;
            }
        }
    }
    if (c == 1) 
#endif
        if (ProcessInfos.TasksInfos[0] != NULL) {
            return ProcessInfos.TasksInfos[0]->sc_process_identifier;
        }
    return -1;
}

EXPORT_OR_IMPORT unsigned long long __FUNC_CALL_CONVETION__ get_scheduler_cycle_count(void)
{
#if defined(_WIN32) || defined(__linux__)
    int x, c = 0;
    DWORD ThreadID = GetCurrentThreadId ();
    for (x = 0; x < MAX_PROCESSES_INSIDE_ONE_EXECUTABLE; x++) {
        EXTERN_PROCESS_TASK_INFOS_STRUCT * volatile TaskInfos;
        TaskInfos = ProcessInfos.TasksInfos[x];
        if (TaskInfos != NULL) {
            c++;
            if (TaskInfos->ThreadID == ThreadID) {
                return TaskInfos->CycleCount;
            }
        }
    }
    if (c == 1) 
#endif
        if (ProcessInfos.TasksInfos[0] != NULL) {
            return ProcessInfos.TasksInfos[0]->CycleCount;
        }
    return 1;
}

EXPORT_OR_IMPORT int  __FUNC_CALL_CONVETION__ get_cycle_period(void)
{
#if defined(_WIN32) || defined(__linux__)
	int x, c = 0;
    DWORD ThreadID = GetCurrentThreadId ();
    for (x = 0; x < MAX_PROCESSES_INSIDE_ONE_EXECUTABLE; x++) {
        EXTERN_PROCESS_TASK_INFOS_STRUCT * volatile TaskInfos;
        TaskInfos = ProcessInfos.TasksInfos[x];
        if (TaskInfos != NULL) {
            c++;
            if (TaskInfos->ThreadID == ThreadID) {
                return (int)(TaskInfos->CycleCount - TaskInfos->LastCycleCount);
            }
        }
    }
    if (c == 1) 
#endif
    	if (ProcessInfos.TasksInfos[0] != NULL) {
            return (int)(ProcessInfos.TasksInfos[0]->CycleCount - ProcessInfos.TasksInfos[0]->LastCycleCount);
        }
    return 1;
}

EXTERN_PROCESS_TASK_INFOS_STRUCT* XilEnvInternal_GetTaskPtrFileLine (const char *File, int Line)
{
    int x;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *Ret;
#if defined(_WIN32) || defined(__linux__)
    DWORD ThreadID = GetCurrentThreadId ();
    for (x = 0; x < MAX_PROCESSES_INSIDE_ONE_EXECUTABLE; x++) {
        EXTERN_PROCESS_TASK_INFOS_STRUCT * TaskInfos;
        TaskInfos = ProcessInfos.TasksInfos[x];
        if (TaskInfos != NULL) {
            if (TaskInfos->ThreadID == ThreadID) {
                Ret = TaskInfos;
                goto __OUT;
            }
        }
    }
#endif
    Ret = ProcessInfos.TasksInfos[0];  // If none found use the first one
__OUT:
    if (Ret != NULL) {
#ifdef _WIN32
        DWORD WaitResult = WaitForSingleObject (Ret->TaskMutex,    // handle to mutex
                                                INFINITE);         // no time-out interval
#elif defined(__linux__)
        pthread_mutex_lock(&Ret->TaskMutex);
#endif
        if ((Ret->TaskMutexFile != NULL) && (Ret->MutexCount == 0)) {
            XilEnvInternal_ThrowError (Ret, 1, "Internal lock error %s (%i) /  %s (%i)", Ret->TaskMutexFile, Ret->TaskMutexLine,  Ret->TaskMutexWaitForCallFromFile, Ret->TaskMutexWaitForCallFromLine);
        }
        Ret->MutexCount++;
        Ret->TaskMutexFile = __FILE__;
        Ret->TaskMutexLine = __LINE__;
        Ret->TaskMutexWaitForCallFromFile = File;
        Ret->TaskMutexWaitForCallFromLine = Line;
    }
    return Ret;
}


void XilEnvInternal_LockTaskPtrFileLine (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfos, const char *FiLe, int Line)
{
    if (TaskInfos != NULL) {
#ifdef _WIN32
        DWORD WaitResult = WaitForSingleObject (TaskInfos->TaskMutex,    // handle to mutex
                                                INFINITE);         // no time-out interval
#elif defined(__linux__)
        pthread_mutex_lock(&TaskInfos->TaskMutex);
#endif
        if ((TaskInfos->TaskMutexFile != NULL) && (TaskInfos->MutexCount == 0)) {
            XilEnvInternal_ThrowError (TaskInfos, 1, "Internal lock error %s (%i) /  %s (%i)", TaskInfos->TaskMutexFile, TaskInfos->TaskMutexLine,  TaskInfos->TaskMutexWaitForCallFromFile, TaskInfos->TaskMutexWaitForCallFromLine);
        }
        TaskInfos->MutexCount++;
        TaskInfos->TaskMutexFile = __FILE__;
        TaskInfos->TaskMutexLine = __LINE__;
        TaskInfos->TaskMutexWaitForCallFromFile = FiLe;
        TaskInfos->TaskMutexWaitForCallFromLine = Line;
    }
}

void XilEnvInternal_ReleaseTaskPtrFileLine (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfos, const char *File, int Line)
{
    if (TaskInfos != NULL) {
        if (TaskInfos->MutexCount < 1) {
            XilEnvInternal_ThrowError (TaskInfos, 1, "Internal unlock error %s (%i)\n"
                                                    "locked here %s (%i)",
                                       File, Line,
                                       TaskInfos->TaskMutexWaitForCallFromFile, TaskInfos->TaskMutexWaitForCallFromLine);
        } else {
            TaskInfos->MutexCount--;
        }
        TaskInfos->TaskMutexFile = NULL;
        TaskInfos->TaskMutexLine = __LINE__;
        TaskInfos->TaskMutexReleaseCallFromFile = File;
        TaskInfos->TaskMutexReleaseCallFromLine = Line;

#ifdef _WIN32
        ReleaseMutex (TaskInfos->TaskMutex);
#elif defined(__linux__)
        pthread_mutex_unlock(&TaskInfos->TaskMutex);
#endif
    }
}

EXTERN_PROCESS_TASK_INFOS_STRUCT* XilEnvInternal_GetTaskPtrByNumber (int par_TaskNumber)
{
    // No mutex this will only used by XCP
    if (par_TaskNumber >= ProcessInfos.ThreadCount) {
        return NULL;
    } else {
        return ProcessInfos.TasksInfos[par_TaskNumber];
    }
}


#ifdef WITH_ALIVE_PING_EVENTS
static DWORD WINAPI XilEnvInternal_AsyncCommunicationThreadFunction (LPVOID lpParam)
{
    ASYNC_ALIVE_PING_THREAD_PARAM *AsyncAlivePing = (ASYNC_ALIVE_PING_THREAD_PARAM*)lpParam;

    while (AsyncAlivePing->TerminateFlag == 0) {  // sauber beenden !!!
        switch (WaitForSingleObject (AsyncAlivePing->ReqEvent, 1000)) {
        case WAIT_OBJECT_0:
            SetEvent (AsyncAlivePing->AckEvent);
            break;
        }
    }
    AsyncAlivePing->TerminateFlag = 2;    // Terniert!
    return 0;
}

static HANDLE XilEnvInternal_StartAsyncCommunicationThread (ASYNC_ALIVE_PING_THREAD_PARAM *pAsyncAlivePing)
{
    HANDLE hThread;
    DWORD dwThreadId = 0;

    hThread = CreateThread (NULL,              // no security attribute
                            0,                 // default stack size
                            XilEnvInternal_AsyncCommunicationThreadFunction,    // thread proc
                            (void*)pAsyncAlivePing,    // thread parameter
                            0,                 // not suspended
                            &dwThreadId);      // returns thread ID

    if (hThread == NULL)  {
        char *lpMsgBuf;
        DWORD dw = GetLastError ();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &lpMsgBuf,
                       0, NULL );
        ThrowError (1, "cannot create asynchron communication thread: %s", lpMsgBuf);
        LocalFree (lpMsgBuf);
        return NULL;
    }
    return hThread;
}
#endif

#ifndef _WIN32
static char *_strupr(char *Src)
{
    /*char *Dst;
    for (Dst = Src; *Dst != 0; Dst++) {
        *Dst = toupper(*Dst);
    }*/
    return Src;
}
#endif

static BOOL XilEnvInternal_TransactMessage(EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr,
                                            void *TransmitMessage, DWORD TransmitMessageSize,
                                            void *ReceiveMessage, DWORD ReceiveMessageSize, DWORD *ReceivedBytes)
{
    DWORD ReceivedPartBytes = 0;
    BOOL Status;

    *ReceivedBytes = 0;

    Status = TaskPtr->TransactMessage(TaskPtr->PipeOrSocketHandle,
                                      TransmitMessage, TransmitMessageSize,
                                      ReceiveMessage, ReceiveMessageSize, ReceivedBytes);
    if (!Status) {
        return FALSE;
    }

    while ((*ReceivedBytes < sizeof(PIPE_API_BASE_CMD_MESSAGE)) ||
           (*ReceivedBytes < (DWORD)((PIPE_API_BASE_CMD_MESSAGE*)ReceiveMessage)->StructSize)) {
        Status = TaskPtr->ReadMessage(TaskPtr->PipeOrSocketHandle, (void*)((char*)ReceiveMessage + *ReceivedBytes), PIPE_MESSAGE_BUFSIZE - *ReceivedBytes, &ReceivedPartBytes);
        if (!Status) {
            return FALSE;
        }
        *ReceivedBytes += ReceivedPartBytes;
    }
    return TRUE;
}

static HANDLE XilEnvInternal_ConnectToAndLogin (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,
                                                int par_ErrMessageFlag)
{
    HANDLE hPipe;
    BOOL Status;
    DWORD BytesRead;
    PIPE_API_LOGIN_MESSAGE LoginMessage;
    PIPE_API_LOGIN_MESSAGE_ACK LoginMessageAck;

    //XilEnvInternal_QemuLog ("XilEnvInternal_ConnectToAndLogin()\n");
#if defined(_WIN32) || defined(__linux__)
    TaskInfos->ThreadID = GetCurrentThreadId ();
#else
    TaskInfos->ThreadID = 0x12345678; // not importand only single core posible
#endif
    hPipe = TaskInfos->ConnectTo(TaskInfos->InstanceName, TaskInfos->ServerName, TaskInfos->LoginTimeout, par_ErrMessageFlag);
    if (hPipe == INVALID_HANDLE_VALUE) {
        return INVALID_HANDLE_VALUE;
    }
    TaskInfos->PipeOrSocketHandle = hPipe;

    memset(&LoginMessage, 0, sizeof(LoginMessage));
    strcpy (LoginMessage.ExecutableName, TaskInfos->ExecutableName);
    _strupr (LoginMessage.ExecutableName);
    strcpy (LoginMessage.ProcessName, TaskInfos->ProcessAtTaskName);
    _strupr (LoginMessage.ProcessName);
    strcpy (LoginMessage.DllName, TaskInfos->DllName);
    _strupr (LoginMessage.DllName);
    LoginMessage.ProcessNumber = TaskInfos->Number;
    LoginMessage.NumberOfProcesses = TaskInfos->ThreadCount;
    LoginMessage.Command = PIPE_API_LOGIN_CMD;
    LoginMessage.StructSize = sizeof (LoginMessage);
    LoginMessage.Version = EXTERN_PROCESS_COMUNICATION_PROTOCOL_VERSION;

    LoginMessage.Priority = TaskInfos->Priority;
    LoginMessage.CycleDivider = TaskInfos->CycleDivider;
    LoginMessage.Delay = TaskInfos->Delay;

#ifdef _WIN32
#ifdef _M_X64
    LoginMessage.Machine = MACHINE_WIN32_X64;
#else
    LoginMessage.Machine = MACHINE_WIN32_X86;
#endif
#else
    if (sizeof(long) == 8) {
        LoginMessage.Machine = MACHINE_LINUX_X64;
    } else {
        LoginMessage.Machine = MACHINE_LINUX_X86;
    }
#endif

#ifdef _WIN32
    LoginMessage.ProcessId = GetCurrentProcessId ();
#elif defined(__linux__)
    LoginMessage.ProcessId = (uint32_t)getpid();
#else
    LoginMessage.ProcessId = 0x1234;
#endif
#if (!defined _WIN32 || (_MSC_VER == 1310))
	LoginMessage.IsStartedInDebugger = 0;  // VS2003 dont know IsDebuggerPresent
#else
    LoginMessage.IsStartedInDebugger = IsDebuggerPresent ();  // If the current process is running in the context of a debugger, the return value is nonzero
#endif

#ifdef WITH_ALIVE_PING_EVENTS
    // Zwei Events fuer einen Alive-Ping ob der externe Prozess noch lebt
    TaskInfos->AlivePing.ReqEvent = (ALIVE_PING_HANDLE_ULONG)CreateEvent (NULL, FALSE, FALSE, NULL);
    LoginMessage.hAsyncAlivePingReqEvent = (ALIVE_PING_HANDLE_ULONG)TaskInfos->AlivePing.ReqEvent;
    if ((HANDLE)LoginMessage.hAsyncAlivePingReqEvent == NULL) {
        char *lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        dw,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR) &lpMsgBuf,
                        0, NULL );
        ThrowError (1, "CreateEvent for asynchron alive request event failed: %s", lpMsgBuf);
        LocalFree (lpMsgBuf);
        TaskInfos->DisconnectFrom(hPipe);
        return INVALID_HANDLE_VALUE;
    } else {
        TaskInfos->AlivePing.AckEvent = (ALIVE_PING_HANDLE_ULONG)CreateEvent (NULL, FALSE, FALSE, NULL);
        LoginMessage.hAsyncAlivePingAckEvent = (ALIVE_PING_HANDLE_ULONG)TaskInfos->AlivePing.AckEvent;
        if ((HANDLE)LoginMessage.hAsyncAlivePingAckEvent == NULL) {
            char *lpMsgBuf;
            DWORD dw = GetLastError();
            FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL,
                            dw,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPTSTR) &lpMsgBuf,
                            0, NULL );
            ThrowError (1, "CreateEvent for asynchron alive request event failed: %s", lpMsgBuf);
            LocalFree (lpMsgBuf);
            TaskInfos->DisconnectFrom(hPipe);
            return INVALID_HANDLE_VALUE;
        } else {
            TaskInfos->AlivePing.AsyncCommunicationThreadHandle = XilEnvInternal_StartAsyncCommunicationThread (&(TaskInfos->AlivePing));
        }
    }
#endif
    LoginMessage.ExecutableBaseAddress = (uint64_t)ProcessInfos.ExecutableBaseAddress;
    LoginMessage.DllBaseAddress = (uint64_t)TaskInfos->DllBaseAddress;

    // No extensions
    LoginMessage.ExtensionsFlags = 0;

    Status = TaskInfos->TransactMessage (hPipe,
                                         (LPVOID) &LoginMessage,
                                         sizeof (LoginMessage),
                                         (LPVOID) &LoginMessageAck,
                                         sizeof (LoginMessageAck),
                                         &BytesRead);
    if (!Status) {
#ifdef _WIN32
        char *lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL,
                        dw,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR) &lpMsgBuf,
                        0, NULL );
        ThrowError (1, "TransactNamedPipe failed: %s", lpMsgBuf);
        LocalFree (lpMsgBuf);
#else
        ThrowError(1, "TransactMessage failed: %s", strerror(errno));
#endif
        TaskInfos->DisconnectFrom(hPipe);
        return INVALID_HANDLE_VALUE;
    }
    switch (LoginMessageAck.ReturnValue) {
    case PIPE_API_LOGIN_SUCCESS:
         break;  // OK!
    case PIPE_API_LOGIN_ERROR_NO_FREE_PID:
        ThrowError (1, "cannot login to XilEnv (login are rejected) because no free PID");
        TaskInfos->DisconnectFrom(hPipe);
        return INVALID_HANDLE_VALUE;
    default:
        ThrowError (1, "cannot login to XilEnv (login are rejected) because %i", LoginMessageAck.ReturnValue);
        TaskInfos->DisconnectFrom(hPipe);
        return INVALID_HANDLE_VALUE;
    }
    TaskInfos->sc_process_identifier = LoginMessageAck.Pid;
    TaskInfos->State = 1;
    //XilEnvInternal_QemuLog ("XilEnvInternal_ConnectToAndLogin done\n");

    return hPipe;
}

static void XilEnvInternal_CopyDataFromBlackboardToAddressAsInitData (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo, unsigned long long Address, int Vid, int DataType)
{
    int RealDataType;
    union BB_VARI Value;

    if (XilEnvInternal_PipeReadFromBlackboardVariable (TaskInfo, Vid, DataType, &Value, &RealDataType)) {
        XilEnvInternal_ThrowError (TaskInfo, 1, "internal error cannot read from variable %i", Vid);
    } else {
        switch (DataType) {
        case BB_BYTE:
        case BB_UNKNOWN_BYTE:
        case BB_UNKNOWN_WAIT_BYTE:
            *(signed char*)Address = Value.b;
            break;
        case BB_UBYTE:
        case BB_UNKNOWN_UBYTE:
        case BB_UNKNOWN_WAIT_UBYTE:
            *(unsigned char*)Address = Value.ub;
            break;
        case BB_WORD:
        case BB_UNKNOWN_WORD:
        case BB_UNKNOWN_WAIT_WORD:
            *(signed short*)Address = Value.w;
            break;
        case BB_UWORD:
        case BB_UNKNOWN_UWORD:
        case BB_UNKNOWN_WAIT_UWORD:
            *(unsigned short*)Address = Value.uw;
            break;
        case BB_DWORD:
        case BB_UNKNOWN_DWORD:
        case BB_UNKNOWN_WAIT_DWORD:
            *(signed int*)Address = Value.dw;
            break;
        case BB_UDWORD:
        case BB_UNKNOWN_UDWORD:
        case BB_UNKNOWN_WAIT_UDWORD:
            *(unsigned int*)Address = Value.udw;
            break;
        case BB_QWORD:
        case BB_UNKNOWN_QWORD:
        case BB_UNKNOWN_WAIT_QWORD:
            *(signed long long*)Address = Value.qw;
            break;
        case BB_UQWORD:
        case BB_UNKNOWN_UQWORD:
        case BB_UNKNOWN_WAIT_UQWORD:
            *(unsigned long long*)Address = Value.uqw;
            break;
        case BB_FLOAT:
        case BB_UNKNOWN_FLOAT:
        case BB_UNKNOWN_WAIT_FLOAT:
            *(float*)Address = Value.f;
            break;
        case BB_DOUBLE:
        case BB_UNKNOWN_DOUBLE:
        case BB_UNKNOWN_WAIT_DOUBLE:
            *(double*)Address = Value.d;
            break;
        default:
            XilEnvInternal_ThrowError (TaskInfo, 1, "internal error unknown data type %i", DataType);
            break;
        }
    }
}

#ifndef EXTP_CONFIG_HAVE_DOUBLE
void ConverFloatToDouble(union DOUBLE_UNION *Ret, float In)
{
    uint32_t float_bits;
    uint64_t double_bits;
    uint32_t exp;
    float_bits = *(uint32_t*)&In;

    double_bits =  (uint64_t)(float_bits & 0x7FFFFF) << 29;  // mantissa 23 bits
    if (float_bits & 0x40000000UL) {
        exp = (float_bits >> 23) & 0x7F;
        exp |= 0x400;
    } else {
        exp = (float_bits >> 23) & 0x7F;
        if (exp) {
            exp |= 0x380;
        }
    }
    double_bits |= (uint64_t)exp << 52;
    double_bits |= (uint64_t)((float_bits >> 31) & 0x1) << 63;  // sign

    Ret->ui = double_bits;
}
#else 
#define ConverFloatToDouble(Ret, In) ((*Ret.d) = In)
#endif

int XilEnvInternal_PipeAddBlackboardVariableAllInfos (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo,
                                                        const char *Name, int Type, const char *Unit, int Dir,
                                                        int ConvType, char *Conversion,
                                                        double Min, double Max,
                                                        int Width, int Prec,
                                                        unsigned int RgbColor,
                                                        int StepType, double Step,
                                                        int ValueIsValid,
                                                        union BB_VARI Value,
                                                        int AddressIsValid,
                                                        unsigned long long Address,
                                                        unsigned long long UniqueId,
                                                        int *ret_Type)
{
    int NameLen;
    int UnitLen;
    int ConversionLen;
    int StructSize;
    PIPE_API_ADD_BBVARI_CMD_MESSAGE *pReq;
    PIPE_API_ADD_BBVARI_CMD_MESSAGE_ACK Ack;
    BOOL Status;
    DWORD BytesRead;
    char NullString[1];
    int LocalVid;
    //EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;

    //XilEnvInternal_QemuLog("XilEnvInternal_PipeAddBlackboardVariableAllInfos()\n");
    if ((Dir & ONLY_ATTACH_EXITING) == ONLY_ATTACH_EXITING) {
        // Check if the variable already exists
        LocalVid = XilEnvInternal_PipeAttachBlackboardVariable (TaskInfo, Name);
        if (LocalVid < 0) {
            return LocalVid;
        }
    }
    NullString[0] = 0;
    NameLen = (int)strlen (Name) + 1;
    if (Unit == NULL) {
        Unit = NullString;
    }
    if (Conversion == NULL) {
        Conversion = NullString;
    }
    UnitLen = (int)strlen (Unit) + 1;
    ConversionLen = (int)strlen (Conversion) + 1;

    StructSize = (int)sizeof (PIPE_API_ADD_BBVARI_CMD_MESSAGE) + NameLen + UnitLen + ConversionLen;
    pReq = (PIPE_API_ADD_BBVARI_CMD_MESSAGE*)XilEnvInternal_malloc ((size_t)StructSize);
    pReq->Command = PIPE_API_ADD_BBVARI_CMD;
    pReq->StructSize = StructSize;
    pReq->Pid = (int16_t)get_process_identifier ();
    pReq->DataType = Type;
    pReq->Dir = Dir;
    pReq->ConversionType = ConvType;
    pReq->AddressValidFlag = AddressIsValid;
    pReq->Address = Address;
    ConverFloatToDouble(&(pReq->Min), Min);
    ConverFloatToDouble(&(pReq->Max), Max);
    ConverFloatToDouble(&(pReq->Step), Step);
    ConverFloatToDouble(&(pReq->ConversionFactor), 1.0);
    ConverFloatToDouble(&(pReq->ConversionOffset), 0.0);
    pReq->Width = Width;
    pReq->Prec = Prec;
    pReq->StepType = StepType;
    pReq->RgbColor = RgbColor;
    pReq->Value = Value;
    pReq->ValueValidFlag = ValueIsValid;
    pReq->UniqueId = UniqueId;
    pReq->NameStructOffset = 0;  // kommt immer als erstes
    pReq->DisplayNameStructOffset = 0; // gibt es noch nicht
    pReq->UnitStructOffset = NameLen;
    pReq->ConversionStructOffset = NameLen + UnitLen;
    pReq->CommentStructOffset = 0;
    MEMCPY (pReq->Data, Name, (size_t)NameLen);
    MEMCPY (pReq->Data + NameLen, Unit, (size_t)UnitLen);
    MEMCPY (pReq->Data + NameLen + UnitLen, Conversion, (size_t)ConversionLen);

    Status = XilEnvInternal_TransactMessage(TaskInfo,
                                       (LPVOID)pReq,
                                       (DWORD)StructSize,
                                       (LPVOID)&Ack,
                                       (DWORD)sizeof (Ack),
                                       &BytesRead);
    
    if (!Status || (BytesRead != sizeof (Ack))) {
        XilEnvInternal_ThrowError (TaskInfo, 1, "cannot add blackboard variable %i or %i != %i\n",  Status, BytesRead, sizeof (Ack));
    }
    if (ret_Type != NULL) *ret_Type = Ack.RealType;
    XilEnvInternal_free (pReq);

    if ((Dir & ONLY_ATTACH_EXITING) == ONLY_ATTACH_EXITING) {
        XilEnvInternal_remove_bbvari_not_cached(TaskInfo, LocalVid);
    }

    if (AddressIsValid &&
        ((Dir & USE_BLACKBOARD_VALUE_AS_INIT) == USE_BLACKBOARD_VALUE_AS_INIT)) {
        XilEnvInternal_CopyDataFromBlackboardToAddressAsInitData(TaskInfo, Address, Ack.Vid, Type);
    }

    return Ack.Vid;
}

int XilEnvInternal_PipeAttachBlackboardVariable (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskPtr, const char *Name)
{
    PIPE_API_ATTACH_BBVARI_CMD_MESSAGE *pReq;
    PIPE_API_ATTACH_BBVARI_CMD_MESSAGE_ACK Ack;
    int StructSize;
    int NameLen;
    BOOL Status;
    DWORD BytesRead;

    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    NameLen = (int)strlen (Name) + 1;
    StructSize = (int)sizeof (PIPE_API_ATTACH_BBVARI_CMD_MESSAGE) + NameLen;

    pReq = (PIPE_API_ATTACH_BBVARI_CMD_MESSAGE*)XilEnvInternal_malloc ((size_t)StructSize);
    pReq->Command = PIPE_API_ATTACH_BBVARI_CMD;
    pReq->StructSize = StructSize;
    pReq->Pid = (int16_t)get_process_identifier ();
    MEMCPY (pReq->Name, Name, (size_t)NameLen);
    Status = XilEnvInternal_TransactMessage(TaskPtr,
                                       (LPVOID)pReq,
                                       (DWORD)StructSize,
                                       (LPVOID)&Ack,
                                       (DWORD)sizeof (Ack),
                                       &BytesRead);

    if (!Status || (BytesRead != sizeof (Ack))) {
        XilEnvInternal_ThrowError (TaskPtr, 1, "cannot attach blackboard variable %i or %i != %i\n",  Status, BytesRead, sizeof (Ack));
    }
    XilEnvInternal_free(pReq);
    return Ack.Vid;
}


int XilEnvInternal_PipeRemoveBlackboardVariable (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int Vid, int Dir, int DataType, unsigned long long Addr, unsigned long long UniqueId)
{
    PIPE_API_REMOVE_BBVARI_CMD_MESSAGE Req;
    PIPE_API_REMOVE_BBVARI_CMD_MESSAGE_ACK Ack;
    BOOL Status = 0;
    DWORD BytesRead;

    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Req.Command = PIPE_API_REMOVE_BBVARI_CMD;
    Req.StructSize = (int)sizeof (PIPE_API_REMOVE_BBVARI_CMD_MESSAGE);
    Req.Pid = (int16_t)get_process_identifier ();
    Req.Vid = Vid;
    Req.Dir = Dir;
    Req.DataType = DataType;
    Req.UniqueId = UniqueId;
    Req.Addr = Addr;
    Status = XilEnvInternal_TransactMessage(TaskPtr,
                                       (LPVOID)&Req,
                                       (DWORD)sizeof (Req),
                                       (LPVOID)&Ack,
                                       (DWORD)sizeof (Ack),
                                       &BytesRead);

    if (!Status || (BytesRead != sizeof (Ack))) {
        XilEnvInternal_ThrowError (TaskPtr, 1, "cannot remove blackboard variable %i or %i != %i\n",  Status, BytesRead, sizeof (Ack));
    }
    return Ack.ReturnValue;
}


int XilEnvInternal_PipeWriteToBlackboardVariable (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int Vid, int DataType, union BB_VARI Value)
{
    PIPE_API_WRITE_BBVARI_CMD_MESSAGE Req;
    PIPE_API_WRITE_BBVARI_CMD_MESSAGE_ACK Ack;
    BOOL Status;
    DWORD BytesRead;

    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Req.Command = PIPE_API_WRITE_BBVARI_CMD;
    Req.StructSize = (int)sizeof (PIPE_API_WRITE_BBVARI_CMD_MESSAGE);
    Req.Pid = (int16_t)get_process_identifier ();
    Req.Vid = Vid;
    Req.DataType = DataType;
    Req.Value = Value;

    Status = XilEnvInternal_TransactMessage(TaskPtr,
                                       (LPVOID)&Req,
                                       (DWORD)sizeof (Req),
                                       (LPVOID)&Ack,
                                       (DWORD)sizeof (Ack),
                                       &BytesRead);

    if (!Status || (BytesRead != sizeof (Ack))) {
        XilEnvInternal_ThrowError (TaskPtr, 1, "cannot write to blackboard variable %i or %i != %i\n",  Status, BytesRead, sizeof (Ack));
    }
    return Ack.ReturnValue;
}

int XilEnvInternal_PipeReadFromBlackboardVariable (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int Vid, int DataType, union BB_VARI *ret_Value, int *ret_DataType)
{
    PIPE_API_READ_BBVARI_CMD_MESSAGE Req;
    PIPE_API_READ_BBVARI_CMD_MESSAGE_ACK Ack;
    BOOL Status;
    DWORD BytesRead;

    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Req.Command = PIPE_API_READ_BBVARI_CMD;
    Req.StructSize = (int)sizeof (PIPE_API_READ_BBVARI_CMD_MESSAGE);
    Req.Pid = (int16_t)get_process_identifier ();
    Req.Vid = Vid;
    Status = XilEnvInternal_TransactMessage(TaskPtr,
                                       (LPVOID)&Req,
                                       (DWORD)sizeof (Req),
                                       (LPVOID)&Ack,
                                       (DWORD)sizeof (Ack),
                                       &BytesRead);

    if (!Status || (BytesRead != sizeof (Ack))) {
        XilEnvInternal_ThrowError (TaskPtr, 1, "cannot read from blackboard variable %i or %i != %i\n",  Status, BytesRead, sizeof (Ack));
    } else {
        *ret_Value = Ack.Value;
        *ret_DataType = Ack.DataType;
    }
    return Ack.ReturnValue;
}


int PipeGetLabelnameByAddress (void *Addr, char *RetName, int Maxc)
{
    PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE Req;
    PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE_ACK *Ack;
    BOOL Status;
    DWORD BytesRead;
    int Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;

    TaskPtr = XilEnvInternal_GetTaskPtr ();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Ack = (PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE_ACK*)XilEnvInternal_malloc (sizeof (PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE_ACK) + SC_MAX_LABEL_SIZE);

    Req.Command = PIPE_API_GET_LABEL_BY_ADDRESS_CMD;
    Req.StructSize = (int)sizeof (PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE);
    Req.Pid = (int16_t)get_process_identifier ();
    Req.Address = (unsigned long long)Addr;
    Status = XilEnvInternal_TransactMessage(TaskPtr,
                                       (LPVOID)&Req,
                                       (DWORD)sizeof (Req),
                                       (LPVOID)Ack,
                                       (DWORD)(sizeof (PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE_ACK) + SC_MAX_LABEL_SIZE),
                                       &BytesRead);

    XilEnvInternal_ReleaseTaskPtr(TaskPtr);

    if (!Status || (BytesRead < sizeof (PIPE_API_GET_LABEL_BY_ADDRESS_CMD_MESSAGE_ACK))) {
        ThrowError (1, "cannot get label by address %i or %i != %i\n",  Status, BytesRead, sizeof (Ack));
        strcpy (RetName, "");
        Ack->ReturnValue = -1;
    } else {
        strncpy (RetName, Ack->Label, (size_t)Maxc);
        RetName[Maxc - 1] = 0;
    }
    Ret = Ack->ReturnValue;
    XilEnvInternal_free (Ack);
    return Ret;
}

EXPORT_OR_IMPORT enum SC_GET_LABELNAME_RETURN_CODE  __FUNC_CALL_CONVETION__  get_not_referenced_labelname_by_address (void *Addr, char *pName, int maxc)
{
    return (enum SC_GET_LABELNAME_RETURN_CODE)PipeGetLabelnameByAddress (Addr, pName, maxc);
}



int PipeGetReferencedLabelnameByVid (int Vid, char *RetName, int Maxc)
{
    PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE Req;
    PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE_ACK *Ack;
    BOOL Status;
    DWORD BytesRead;
    int Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;

    TaskPtr = XilEnvInternal_GetTaskPtr ();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Ack = (PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE_ACK*)XilEnvInternal_malloc (sizeof (PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE_ACK) + SC_MAX_LABEL_SIZE);

    Req.Command = PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD;
    Req.StructSize = sizeof (PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE);
    Req.Pid = (int16_t)get_process_identifier ();
    Req.Vid = Vid;

    Status = XilEnvInternal_TransactMessage(TaskPtr,
                                       (LPVOID)&Req,
                                       (DWORD)sizeof (Req),
                                       (LPVOID)Ack,
                                       (DWORD)(sizeof (PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE_ACK) + SC_MAX_LABEL_SIZE),
                                       &BytesRead);

    XilEnvInternal_ReleaseTaskPtr(TaskPtr);

    if (!Status || (BytesRead < sizeof (PIPE_API_GET_REFERENCED_LABEL_BY_VID_CMD_MESSAGE_ACK))) {
        ThrowError (1, "cannot get label by variable identifier %i or %i != %i\n",  Status, BytesRead, sizeof (Ack));
        strcpy (RetName, "");
        Ack->ReturnValue = -1;
    } else {
        strncpy (RetName, Ack->Label, (size_t)Maxc);
        RetName[Maxc - 1] = 0;
    }
    Ret = Ack->ReturnValue;
    XilEnvInternal_free (Ack);
    return Ret;
}

EXPORT_OR_IMPORT enum SC_GET_LABELNAME_RETURN_CODE __FUNC_CALL_CONVETION__ get_labelname_by_address (void *Addr, char *pName, int maxc)
{
    int Vid;
    if (Addr == NULL) {
        return SC_INVALID_ADDRESS;
    }
    Vid = XilEnvInternal_GetReferencedVariableIdentifierNameByAddress (Addr);
    if (Vid <= 0) {
        return SC_VARIABLES_NOT_REFERENCED;
    }
    if (PipeGetReferencedLabelnameByVid (Vid, pName, maxc)) {
        return SC_VARIABLES_NOT_REFERENCED;
    }
    return SC_OK;
}


int  XilEnvInternal_PipeWriteToMessageFile (const char *Text)
{
    PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE *Req;
    PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE_ACK Ack;
    BOOL Status;
    DWORD BytesRead;
    int TextLen;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;

    TaskPtr = XilEnvInternal_GetTaskPtr ();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    TextLen = (int)strlen (Text);
    if (TextLen >= (PIPE_MESSAGE_BUFSIZE - sizeof (PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE) - 1)) {
        TextLen = PIPE_MESSAGE_BUFSIZE - sizeof (PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE) - 1;
    }
    Req = (PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE*)XilEnvInternal_malloc (sizeof (PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE) + (size_t)TextLen);
    MEMCPY (Req->Text, Text, (size_t)TextLen);
    Req->Text[TextLen] = 0;
    Req->Command = PIPE_API_WRITE_TO_MSG_FILE_CMD;
    Req->StructSize = (int)sizeof (PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE) + TextLen;
    Req->Pid = (int16_t)get_process_identifier ();
    Status = XilEnvInternal_TransactMessage (TaskPtr,
                                       (LPVOID)Req,
                                       (DWORD)sizeof (PIPE_API_WRITE_TO_MSG_FILE_CMD_MESSAGE) + (DWORD)TextLen,
                                       (LPVOID)&Ack,
                                       (DWORD)sizeof (Ack),
                                       &BytesRead);

    XilEnvInternal_ReleaseTaskPtr(TaskPtr);

    if (!Status || (BytesRead != sizeof (Ack))) {
        ThrowError (1, "cannot write to XilEnv message file %i or %i != %i\n", Status, BytesRead, sizeof (Ack));
        Ack.ReturnValue = -1;
    }
    XilEnvInternal_free (Req);
    return Ack.ReturnValue;
}

int  XilEnvInternal_PipeErrorPopupMessageAndWait(EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int Level, const char *Text)
{
    PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE *Req;
    PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE_ACK Ack;
    BOOL Status;
    DWORD BytesRead;
    int TextLen;

    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    TextLen = (int)strlen(Text);
    if (TextLen >= (PIPE_MESSAGE_BUFSIZE - (int)sizeof(PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE) - 1)) {
        TextLen = PIPE_MESSAGE_BUFSIZE - (int)sizeof(PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE) - 1;
    }
    Req = (PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE*)XilEnvInternal_malloc(sizeof(PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE) + (size_t)TextLen);
    Req->Level = Level;
    MEMCPY(Req->Text, Text, (size_t)TextLen);
    Req->Text[TextLen] = 0;
    Req->Command = PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD;
    Req->StructSize = (int)sizeof(PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE) + TextLen;
    Req->Pid = (int16_t)get_process_identifier();
    Status = XilEnvInternal_TransactMessage(TaskPtr,
        (LPVOID)Req,
        (DWORD)sizeof(PIPE_API_ERROR_POPUP_MESSAGE_AND_WAIT_CMD_MESSAGE) + (DWORD)TextLen,
        (LPVOID)&Ack,
        (DWORD)sizeof(Ack),
        &BytesRead);

    if (!Status || (BytesRead != sizeof(Ack))) {
        ThrowError(1, "cannot write to XilEnv message file %i or %i != %i\n", Status, BytesRead, sizeof(Ack));
        Ack.ReturnValue = -1;
    }
    XilEnvInternal_free(Req);
    return Ack.ReturnValue;
}

int XilEnvInternal_OpenVirtualNetworkChannel (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int par_Type, int par_Channel)
{
    PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE Req;
    PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE_ACK Ack;
    BOOL Status;
    DWORD BytesRead;

    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Req.Command = PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD;
    Req.StructSize = sizeof(PIPE_API_OPEN_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE);
    Req.Type = par_Type;
    Req.Channel = par_Channel;
    Status = XilEnvInternal_TransactMessage(TaskPtr,
                                             (LPVOID)&Req,
                                             (DWORD)sizeof (Req),
                                             (LPVOID)&Ack,
                                             (DWORD)sizeof (Ack),
                                             &BytesRead);

    if (!Status || (BytesRead != sizeof (Ack))) {
        XilEnvInternal_ThrowError (TaskPtr, 1, "cannot open virtual network channel %i of type %i: error code = %i", par_Channel, par_Type, Status);
    }
    return Ack.ReturnValue;
}

int XilEnvInternal_CloseVirtualNetworkChannel (EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int par_Handle)
{
    PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE Req;
    PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD_MESSAGE_ACK Ack;
    BOOL Status;
    DWORD BytesRead;

    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    Req.Command = PIPE_API_CLOSE_VIRTUEL_NETWORK_CHANNEL_CMD;
    Req.Handle = par_Handle;
    Status = XilEnvInternal_TransactMessage(TaskPtr,
                                             (LPVOID)&Req,
                                             (DWORD)sizeof (Req),
                                             (LPVOID)&Ack,
                                             (DWORD)sizeof (Ack),
                                             &BytesRead);

    if (!Status || (BytesRead != sizeof (Ack))) {
        XilEnvInternal_ThrowError (TaskPtr, 1, "cannot close virtual network channel: handle = %, error code = %i", par_Handle, Status);
    }
    return Ack.ReturnValue;
}


static void ReadFromPipe(EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, PIPE_API_CALL_FUNC_CMD_MESSAGE *pCallFuncCmdMessage)
{
    int VirtualNetworkSnapShotSize;

    TaskInfos->LastCycleCount = TaskInfos->CycleCount;
    TaskInfos->CycleCount = pCallFuncCmdMessage->Cycle;
    if (TaskInfos->OverlayBuffers.Count > 0) {
        XilEnvInternal_CopyFromPipeToRefWithOverlayCheck (TaskInfos, &(pCallFuncCmdMessage->SnapShotData[0]), pCallFuncCmdMessage->SnapShotSize);
    } else {
        XilEnvInternal_CopyFromPipeToRef (TaskInfos, &(pCallFuncCmdMessage->SnapShotData[0]), pCallFuncCmdMessage->SnapShotSize);
    }
    // Check if there are virtual network data added to the message
    VirtualNetworkSnapShotSize = pCallFuncCmdMessage->StructSize - pCallFuncCmdMessage->SnapShotSize - (int32_t)sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE);
    if (VirtualNetworkSnapShotSize > 0) {
        XilEnvInternal_VirtualNetworkCopyFromPipeToFifos (TaskInfos, &(pCallFuncCmdMessage->SnapShotData[pCallFuncCmdMessage->SnapShotSize]), VirtualNetworkSnapShotSize);
    }
}

static int WriteToPipe(EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,  PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK *pCallFuncCmdMessageAck)
{
    int VirtualNetworkSnapShotSize;

    pCallFuncCmdMessageAck->SnapShotSize = XilEnvInternal_CopyFromRefToPipe (TaskInfos, &(pCallFuncCmdMessageAck->SnapShotData[0]));

    // check if the extern process has open a virtual network channel
    if (TaskInfos->VirtualNetworkHandleCount > 0) {
        int MaxSize = (PIPE_MESSAGE_BUFSIZE - sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK) - 1) - pCallFuncCmdMessageAck->SnapShotSize;
        VirtualNetworkSnapShotSize = XilEnvInternal_VirtualNetworkCopyFromFifosToPipe (TaskInfos, &(pCallFuncCmdMessageAck->SnapShotData[pCallFuncCmdMessageAck->SnapShotSize]), MaxSize);
    } else {
        VirtualNetworkSnapShotSize = 0;
    }
    return (int)sizeof (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK) + pCallFuncCmdMessageAck->SnapShotSize + VirtualNetworkSnapShotSize;
}


int PipeCallReferenceFunctionCmdMessage (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,
                                         PIPE_API_CALL_FUNC_CMD_MESSAGE *pCallReferenceFuncCmdMessage,
                                         PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK *pCallReferenceFuncCmdMessageAck)
{
    int Ret = 0;
    int Size;

    ReadFromPipe(TaskInfos, pCallReferenceFuncCmdMessage);

    // Catch up all collected referenzen during CStartup routine
    XilEnvInternal_DelayReferenceAllCStartupReferences (TaskInfos->TaskName);

    if (TaskInfos->CallConvention == CALL_CONVENTION_STDCALL) {
        if (TaskInfos->FuncPtrs.Stdcall.reference != NULL) {
            TaskInfos->FuncPtrs.Stdcall.reference ();
        }
    } else {
        if (TaskInfos->FuncPtrs.Cdecl.reference != NULL) {
            TaskInfos->FuncPtrs.Cdecl.reference ();
        }
    }

    Size = WriteToPipe(TaskInfos, pCallReferenceFuncCmdMessageAck);

    pCallReferenceFuncCmdMessageAck->Command = PIPE_API_CALL_REFERENCE_FUNC_CMD;
    pCallReferenceFuncCmdMessageAck->StructSize = Size;
    pCallReferenceFuncCmdMessageAck->ReturnValue = Ret;
    return Size;
}



int PipeCallIntFunctionCmdMessage (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,
                                   PIPE_API_CALL_FUNC_CMD_MESSAGE *pCallInitFuncCmdMessage,
                                   PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK *pCallInitFuncCmdMessageAck)
{
    int Ret = 0;
    int Size;

    ReadFromPipe(TaskInfos, pCallInitFuncCmdMessage);

    if (TaskInfos->CallConvention == CALL_CONVENTION_STDCALL) {
        if (TaskInfos->FuncPtrs.Stdcall.init != NULL) {
            Ret = TaskInfos->FuncPtrs.Stdcall.init ();
        }
    } else {
        if (TaskInfos->FuncPtrs.Cdecl.init != NULL) {
            Ret = TaskInfos->FuncPtrs.Cdecl.init ();
        }
    }

    Size = WriteToPipe(TaskInfos, pCallInitFuncCmdMessageAck);

    pCallInitFuncCmdMessageAck->Command = PIPE_API_CALL_INIT_FUNC_CMD;
    pCallInitFuncCmdMessageAck->StructSize = Size;
    pCallInitFuncCmdMessageAck->ReturnValue = Ret;
    return Size;
}

int PipeCallCyclicFunctionCmdMessage (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,
                                      PIPE_API_CALL_FUNC_CMD_MESSAGE *pCallCyclicFuncCmdMessage,
                                      PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK *pCallCyclicFuncCmdMessageAck)
{
    int Size;

    ReadFromPipe(TaskInfos, pCallCyclicFuncCmdMessage);

    if (TaskInfos->CallConvention == CALL_CONVENTION_STDCALL) {
        if (TaskInfos->FuncPtrs.Stdcall.cyclic != NULL) {
            TaskInfos->FuncPtrs.Stdcall.cyclic ();
        }
    } else {
        if (TaskInfos->FuncPtrs.Cdecl.cyclic != NULL) {
            TaskInfos->FuncPtrs.Cdecl.cyclic ();
        }
    }

    Size = WriteToPipe(TaskInfos, pCallCyclicFuncCmdMessageAck);

    pCallCyclicFuncCmdMessageAck->Command = PIPE_API_CALL_CYCLIC_FUNC_CMD;
    pCallCyclicFuncCmdMessageAck->StructSize = Size;
    pCallCyclicFuncCmdMessageAck->ReturnValue = 0;
    return Size;
}

int PipeCallTerminateFunctionCmdMessage (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,
                                         PIPE_API_CALL_FUNC_CMD_MESSAGE *pCallTerminateFuncCmdMessage,
                                         PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK *pCallTerminateFuncCmdMessageAck)
{
    int Ret = 0;

    int Size;

    ReadFromPipe(TaskInfos, pCallTerminateFuncCmdMessage);

    if (TaskInfos->CallConvention == CALL_CONVENTION_STDCALL) {
        if (TaskInfos->FuncPtrs.Stdcall.terminate != NULL) {
            TaskInfos->FuncPtrs.Stdcall.terminate ();
        }
    } else {
        if (TaskInfos->FuncPtrs.Cdecl.terminate != NULL) {
            TaskInfos->FuncPtrs.Cdecl.terminate ();
        }
    }

    Size = WriteToPipe(TaskInfos, pCallTerminateFuncCmdMessageAck);

    pCallTerminateFuncCmdMessageAck->Command = PIPE_API_CALL_TERMINATE_FUNC_CMD;
    pCallTerminateFuncCmdMessageAck->StructSize = Size;
    pCallTerminateFuncCmdMessageAck->ReturnValue = Ret;
    return Size;
}

int PipeDeReferenceAllCmdMessage (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,
                                  PIPE_API_DEREFERENCE_ALL_CMD_MESSAGE *pDereferenceAllCmdMessage,
                                  PIPE_API_DEREFERENCE_ALL_CMD_MESSAGE_ACK *pDereferenceAllMessageAck)
{
    int Ret = 0;

    Ret = XilEnvInternal_DeReferenceAllVariables (TaskInfos);

    pDereferenceAllMessageAck->Command = PIPE_API_DEREFERENCE_ALL_CMD;
    pDereferenceAllMessageAck->StructSize = sizeof (PIPE_API_DEREFERENCE_ALL_CMD_MESSAGE_ACK);
    pDereferenceAllMessageAck->ReturnValue = Ret;
    return (int)sizeof (PIPE_API_DEREFERENCE_ALL_CMD_MESSAGE_ACK);
}


int PipeReadMemoryCmdMessage (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,
                              PIPE_API_READ_MEMORY_CMD_MESSAGE *pReadMemoryCmdMessage,
                              PIPE_API_READ_MEMORY_CMD_MESSAGE_ACK *pReadMemoryCmdMessageAck)
{
    pReadMemoryCmdMessageAck->Command = PIPE_API_READ_MEMORY_CMD;
    if (pReadMemoryCmdMessage->Size <= MAX_MEMORY_BLOCK_SIZE) {
        if (!XilEnvInternal_TryAndCatchReadFromMemCopy (pReadMemoryCmdMessageAck->Memory, (void*)pReadMemoryCmdMessage->Address, (int)pReadMemoryCmdMessage->Size)) {
            pReadMemoryCmdMessageAck->Size = pReadMemoryCmdMessage->Size;
            pReadMemoryCmdMessageAck->ReturnValue = 0;
        } else {
            pReadMemoryCmdMessageAck->Size = 0;
            pReadMemoryCmdMessageAck->ReturnValue = -1;
        }
    } else {
         pReadMemoryCmdMessageAck->Size = 0;
         pReadMemoryCmdMessageAck->ReturnValue = -1;
    }
    pReadMemoryCmdMessageAck->StructSize = (int)sizeof (PIPE_API_READ_MEMORY_CMD_MESSAGE_ACK) + (int)pReadMemoryCmdMessageAck->Size;
    return pReadMemoryCmdMessageAck->StructSize;
}

int PipeWriteMemoryCmdMessage (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,
                               PIPE_API_WRITE_MEMORY_CMD_MESSAGE *pWriteMemoryCmdMessage,
                               PIPE_API_WRITE_MEMORY_CMD_MESSAGE_ACK *pWriteMemoryCmdMessageAck)
{
    pWriteMemoryCmdMessageAck->Command = PIPE_API_WRITE_MEMORY_CMD;
    pWriteMemoryCmdMessageAck->StructSize = sizeof (PIPE_API_WRITE_MEMORY_CMD_MESSAGE_ACK);
    if (pWriteMemoryCmdMessage->Size <= MAX_MEMORY_BLOCK_SIZE) {
        if (!XilEnvInternal_TryAndCatchWriteToMemCopy ((void*)pWriteMemoryCmdMessage->Address, pWriteMemoryCmdMessage->Memory, (int)pWriteMemoryCmdMessage->Size)) {
            XilEnvInternal_AddAddressToOverlayBuffer (TaskInfos, (void*)pWriteMemoryCmdMessage->Address, (int)pWriteMemoryCmdMessage->Size);
            pWriteMemoryCmdMessageAck->Size = pWriteMemoryCmdMessage->Size;
            pWriteMemoryCmdMessageAck->ReturnValue = 0;
    	} else {
            pWriteMemoryCmdMessageAck->Size = 0;
            pWriteMemoryCmdMessageAck->ReturnValue = -1;
        }
    } else {
         pWriteMemoryCmdMessageAck->Size = 0;
         pWriteMemoryCmdMessageAck->ReturnValue = -1;
    }
    return (int)sizeof (PIPE_API_WRITE_MEMORY_CMD_MESSAGE_ACK);
}


int PipeReferenceVariableCmdMessage (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,
                                     PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE *pReferenceVariableCmdMessage,
                                     PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE_ACK *pReferenceVariableCmdMessageAck)
{
    int Size;
    char Help[8];

    pReferenceVariableCmdMessageAck->Command = PIPE_API_REFERENCE_VARIABLE_CMD;
    pReferenceVariableCmdMessageAck->StructSize = sizeof (PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE_ACK);

    switch (pReferenceVariableCmdMessage->Type) {
    case BB_BYTE:
    case BB_UBYTE:
        Size = 1;
        break;
    case BB_WORD:
    case BB_UWORD:
        Size = 2;
        break;
    case BB_DWORD:
    case BB_UDWORD:
    case BB_FLOAT:
        Size = 4;
        break;
    case BB_QWORD:
    case BB_UQWORD:
    case BB_DOUBLE:
        Size = 8;
        break;
    default:
        Size = 0;
        pReferenceVariableCmdMessage->Address = 0;
    }

    // Check write/read access
    if (!XilEnvInternal_TryAndCatchReadFromMemCopy ((void*)Help, (void*)pReferenceVariableCmdMessage->Address, Size) &&
        !XilEnvInternal_TryAndCatchWriteToMemCopy ((void*)pReferenceVariableCmdMessage->Address, (void*)Help, Size)) {
        pReferenceVariableCmdMessageAck->ReturnValue = 0;
	} else {
        pReferenceVariableCmdMessageAck->ReturnValue = -1;
    }
    if (pReferenceVariableCmdMessageAck->ReturnValue == 0) {
        pReferenceVariableCmdMessageAck->ReturnValue = 
            XilEnvInternal_ReferenceVariable ((void*)pReferenceVariableCmdMessage->Address, pReferenceVariableCmdMessage->Name,
                                               pReferenceVariableCmdMessage->Type, pReferenceVariableCmdMessage->Dir);
    }
    return sizeof (PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE_ACK);
}

int PipeDeReferenceVariableCmdMessage (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,
                                       PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE *pDeReferenceVariableCmdMessage,
                                       PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE_ACK *pDeReferenceVariableCmdMessageAck)
{
    pDeReferenceVariableCmdMessageAck->Command = PIPE_API_DEREFERENCE_VARIABLE_CMD;
    pDeReferenceVariableCmdMessageAck->StructSize = sizeof (PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE_ACK);

    pDeReferenceVariableCmdMessageAck->ReturnValue = XilEnvInternal_DereferenceVariable ((void*)pDeReferenceVariableCmdMessage->Address,
                                                                      pDeReferenceVariableCmdMessage->Vid);
    pDeReferenceVariableCmdMessageAck->StructSize = sizeof (PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE_ACK);
    return sizeof (PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE_ACK);
}

int PipeWriteSectionBackToExeCmdMessage (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,
                                         PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD_MESSAGE *pWriteSectionBackToExeCmdMessage,
                                         PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD_MESSAGE_ACK *pWriteSectionBackToExeCmdMessageAck)
{
    //printf ("write section back to exe\n");
    pWriteSectionBackToExeCmdMessageAck->Command = PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD;
    pWriteSectionBackToExeCmdMessageAck->StructSize = sizeof (PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD_MESSAGE_ACK);
#ifdef EXTP_CONFIG_HAVE_XCP
    pWriteSectionBackToExeCmdMessageAck->ReturnValue = XilEnvInternal_write_section_to_exe_file (TaskInfos, pWriteSectionBackToExeCmdMessage->SectionName);
#else
    pWriteSectionBackToExeCmdMessageAck->ReturnValue = -1;
#endif
    return sizeof (PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD_MESSAGE_ACK);
}

int DecodePipeCmdMessage (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos,
                          PIPE_API_BASE_CMD_MESSAGE *pCmdMessage, int Len,
                          PIPE_API_BASE_CMD_MESSAGE_ACK *pCmdMessageAckBuffer)
{
    int ResponseLen;

    switch (pCmdMessage->Command) {
    case PIPE_API_CALL_CYCLIC_FUNC_CMD:
        //XilEnvInternal_QemuLog ("PIPE_API_CALL_CYCLIC_FUNC_CMD\n");
        ResponseLen = PipeCallCyclicFunctionCmdMessage (TaskInfos,
                                                        (PIPE_API_CALL_FUNC_CMD_MESSAGE *)pCmdMessage,
                                                        (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK *)pCmdMessageAckBuffer);
        break;
    case PIPE_API_CALL_REFERENCE_FUNC_CMD:
        //XilEnvInternal_QemuLog ("PIPE_API_CALL_REFERENCE_FUNC_CMD\n");
        ResponseLen = PipeCallReferenceFunctionCmdMessage (TaskInfos,
                                                           (PIPE_API_CALL_FUNC_CMD_MESSAGE *)pCmdMessage,
                                                           (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK *)pCmdMessageAckBuffer);
        break;
    case PIPE_API_CALL_INIT_FUNC_CMD:
        //XilEnvInternal_QemuLog ("PIPE_API_CALL_INIT_FUNC_CMD\n");
        ResponseLen = PipeCallIntFunctionCmdMessage (TaskInfos,
                                                     (PIPE_API_CALL_FUNC_CMD_MESSAGE *)pCmdMessage,
                                                     (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK *)pCmdMessageAckBuffer);
        break;
    case PIPE_API_CALL_TERMINATE_FUNC_CMD:
        //XilEnvInternal_QemuLog ("PIPE_API_CALL_TERMINATE_FUNC_CMD\n");
        ResponseLen = PipeCallTerminateFunctionCmdMessage (TaskInfos,
                                                           (PIPE_API_CALL_FUNC_CMD_MESSAGE *)pCmdMessage,
                                                           (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK *)pCmdMessageAckBuffer);
        break;

    case PIPE_API_DEREFERENCE_ALL_CMD:
        //XilEnvInternal_QemuLog ("PIPE_API_DEREFERENCE_ALL_CMD\n");
        ResponseLen = PipeDeReferenceAllCmdMessage (TaskInfos,
                                                    (PIPE_API_DEREFERENCE_ALL_CMD_MESSAGE *)pCmdMessage,
                                                    (PIPE_API_DEREFERENCE_ALL_CMD_MESSAGE_ACK *)pCmdMessageAckBuffer);
        break;
    case PIPE_API_READ_MEMORY_CMD:
        //XilEnvInternal_QemuLog ("PIPE_API_READ_MEMORY_CMD\n");
        ResponseLen = PipeReadMemoryCmdMessage (TaskInfos,
                                                (PIPE_API_READ_MEMORY_CMD_MESSAGE *)pCmdMessage,
                                                (PIPE_API_READ_MEMORY_CMD_MESSAGE_ACK *)pCmdMessageAckBuffer);
        break;
    case PIPE_API_WRITE_MEMORY_CMD:
        //XilEnvInternal_QemuLog ("PIPE_API_WRITE_MEMORY_CMD\n");
        ResponseLen = PipeWriteMemoryCmdMessage (TaskInfos,
                                                 (PIPE_API_WRITE_MEMORY_CMD_MESSAGE *)pCmdMessage,
                                                 (PIPE_API_WRITE_MEMORY_CMD_MESSAGE_ACK *)pCmdMessageAckBuffer);
        break;
    case PIPE_API_REFERENCE_VARIABLE_CMD:
        //XilEnvInternal_QemuLog ("PIPE_API_REFERENCE_VARIABLE_CMD\n");
        ResponseLen = PipeReferenceVariableCmdMessage (TaskInfos,
                                                       (PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE *)pCmdMessage,
                                                       (PIPE_API_REFERENCE_VARIABLE_CMD_MESSAGE_ACK *)pCmdMessageAckBuffer);
        break;
    case PIPE_API_DEREFERENCE_VARIABLE_CMD:
        //XilEnvInternal_QemuLog ("PIPE_API_DEREFERENCE_VARIABLE_CMD\n");
        ResponseLen = PipeDeReferenceVariableCmdMessage (TaskInfos,
                                                         (PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE *)pCmdMessage,
                                                         (PIPE_API_DEREFERENCE_VARIABLE_CMD_MESSAGE_ACK *)pCmdMessageAckBuffer);
        break;
    case PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD:
        //XilEnvInternal_QemuLog ("PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD\n");
        ResponseLen = PipeWriteSectionBackToExeCmdMessage (TaskInfos,
                                                           (PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD_MESSAGE *)pCmdMessage,
                                                           (PIPE_API_WRITE_SECTION_BACK_TO_EXE_CMD_MESSAGE_ACK *)pCmdMessageAckBuffer);
        break;
    case PIPE_API_KILL_WITH_NO_RESPONSE_CMD:
        //XilEnvInternal_QemuLog ("PIPE_API_KILL_WITH_NO_RESPONSE_CMD\n");
        ResponseLen = -1;  // No response and terminate
        break;
    default:
        //XilEnvInternal_QemuLog ("default\n");
        ResponseLen = 0;  // Unknown message do nothing, that must be handled into a upper function before
        break;
    }
    return ResponseLen;
}

static void XilEnvInternal_KillExternProcessFrom (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos)
{
    int x;
    int AllThreadStoped = 1;

    TaskInfos->DisconnectFrom (TaskInfos->PipeOrSocketHandle);
    TaskInfos->State = 0;
    if (TaskInfos->pCmdMessageBuffer != NULL) {
        XilEnvInternal_free (TaskInfos->pCmdMessageBuffer);
        TaskInfos->pCmdMessageBuffer = NULL;
    }
    if (TaskInfos->pCmdMessageAckBuffer != NULL) {
        XilEnvInternal_free (TaskInfos->pCmdMessageAckBuffer);
        TaskInfos->pCmdMessageAckBuffer = NULL;
    }

    for (x = 0; x < ProcessInfos.ThreadCount; x++) {
        if (ProcessInfos.TasksInfos[x] != NULL) {
            if (ProcessInfos.TasksInfos[x]->State) AllThreadStoped = 0;
        }
    }
    if (AllThreadStoped) {
        KillExternProcessHimSelf ();
    }
}

static int RemoveAndDeleteTaskInfoStruct (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos)
{
    int x;

    for (x = 0; x < ProcessInfos.ThreadPos; x++) {
        if (ProcessInfos.TasksInfos[x] == TaskInfos) {
            for (x = x + 1; x < ProcessInfos.ThreadPos; x++) {
                ProcessInfos.TasksInfos[x - 1] = ProcessInfos.TasksInfos[x];
            }
            ProcessInfos.ThreadPos--;
            XilEnvInternal_free (TaskInfos);
            return 0;
        }
    }
    return -1;
}


EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ WaitUntilFunctionCallRequest (EXTERN_PROCESS_TASK_HANDLE Process)
{
    DWORD BytesRead;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos = (EXTERN_PROCESS_TASK_INFOS_STRUCT*)Process;
    PIPE_API_CALL_FUNC_CMD_MESSAGE *pReq = (PIPE_API_CALL_FUNC_CMD_MESSAGE*)TaskInfos->pCmdMessageBuffer;
    BytesRead = 0;
    for (;;) {
        BOOL Success;
        DWORD BytesReadPart;
        Success = TaskInfos->ReadMessage (TaskInfos->PipeOrSocketHandle, (void*)((char*)TaskInfos->pCmdMessageBuffer + BytesRead), PIPE_MESSAGE_BUFSIZE, &BytesReadPart);
        if (!Success) {
#ifdef _WIN32
            char *lpMsgBuf;
            DWORD dw = GetLastError();
            FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL,
                            dw,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPTSTR) &lpMsgBuf,
                            0, NULL );
            ThrowError (1, "Connection lost: %s", lpMsgBuf);
            LocalFree (lpMsgBuf);
#else
            ThrowError(1, "Connection to lost: %s", strerror(errno));
#endif
            // Terminate process
            return 0;
        }
        BytesRead += BytesReadPart;
        if ((BytesRead >= sizeof (PIPE_API_BASE_CMD_MESSAGE)) &&
            (BytesRead >= (DWORD)pReq->StructSize)) {

            if (BytesRead != pReq->StructSize) {
                ThrowError(1, "command %i:  BytesRead=%i != pReq->StructSize=%i", pReq->Command, BytesRead, pReq->StructSize);
            }
            switch (pReq->Command) {
            case PIPE_API_CALL_REFERENCE_FUNC_CMD:
                ReadFromPipe(TaskInfos, pReq);
                // Catch up all collected referenzen during CStartup routine
                XilEnvInternal_DelayReferenceAllCStartupReferences (TaskInfos->TaskName);
                return 1;
            case PIPE_API_CALL_INIT_FUNC_CMD:
                ReadFromPipe(TaskInfos, pReq);
                return 2;
            case PIPE_API_CALL_CYCLIC_FUNC_CMD:
                ReadFromPipe(TaskInfos, pReq);
                return 3;
            case PIPE_API_CALL_TERMINATE_FUNC_CMD:
                ReadFromPipe(TaskInfos, pReq);
                return 4;
            default:
                {
                    int ResponseLen;
                    ResponseLen = DecodePipeCmdMessage (TaskInfos, TaskInfos->pCmdMessageBuffer, (int)BytesRead, TaskInfos->pCmdMessageAckBuffer);
                    if (ResponseLen > 0) {
                        DWORD BytesWritten;
                        Success = TaskInfos->WriteMessage (TaskInfos->PipeOrSocketHandle,                  // pipe handle
                                                           (void*)TaskInfos->pCmdMessageAckBuffer,             // message
                                                           (DWORD)ResponseLen,              // message length
                                                           &BytesWritten);             // bytes written
                        if (!Success) {
#ifdef _WIN32
                            ThrowError (1, "WriteFile to pipe failed. GLE=%d", GetLastError());
#else
                            ThrowError (1, "WriteFile to pipe failed. GLE=%d", errno);
#endif
                        }
                    } else if (ResponseLen == 0) {
                        ThrowError (1, "not expected message command=%i", TaskInfos->pCmdMessageBuffer->Command);
                    } else {
                        // external process will be terminated from master (for loop)
                        TaskInfos->AlivePing.TerminateFlag = 1;
#ifdef WITH_ALIVE_PING_EVENTS
                        SetEvent (TaskInfos->AlivePing.ReqEvent);
                        while (TaskInfos->AlivePing.TerminateFlag != 2) {
#ifdef _WIN32
                            Sleep(10);
#else
                            usleep(1000*10);
#endif
                        }
#endif
                        // and close the pipe handle
                        TaskInfos->DisconnectFrom (TaskInfos->PipeOrSocketHandle);
#ifdef WITH_ALIVE_PING_EVENTS
                        CloseHandle (TaskInfos->AlivePing.ReqEvent);
                        CloseHandle (TaskInfos->AlivePing.AckEvent);
                        CloseHandle (TaskInfos->AlivePing.AsyncCommunicationThreadHandle);
#endif
                        RemoveAndDeleteTaskInfoStruct (TaskInfos);

                        return 5;
                    }
                }
            }
            BytesRead = 0;
        }
    }
}

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ AcknowledgmentOfFunctionCallRequest (EXTERN_PROCESS_TASK_HANDLE Process, int Ret)
{
    int Size;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos = (EXTERN_PROCESS_TASK_INFOS_STRUCT*)Process;
    PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK* pAck = (PIPE_API_CALL_FUNC_CMD_MESSAGE_ACK*)TaskInfos->pCmdMessageAckBuffer;

    switch (TaskInfos->pCmdMessageBuffer->Command) {
    case PIPE_API_CALL_CYCLIC_FUNC_CMD:
        pAck->Command = PIPE_API_CALL_CYCLIC_FUNC_CMD;
        break;
    case PIPE_API_CALL_REFERENCE_FUNC_CMD:
        pAck->Command = PIPE_API_CALL_REFERENCE_FUNC_CMD;
        break;
    case PIPE_API_CALL_INIT_FUNC_CMD:
        pAck->Command = PIPE_API_CALL_INIT_FUNC_CMD;
        pAck->ReturnValue = Ret;
        break;
    case PIPE_API_CALL_TERMINATE_FUNC_CMD:
        pAck->Command = PIPE_API_CALL_TERMINATE_FUNC_CMD;
        pAck->ReturnValue = XilEnvInternal_DeReferenceAllVariables (TaskInfos);
        break;
    default:
        return -1;   // Error!
    }

    Size = WriteToPipe(TaskInfos, pAck);

    pAck->SnapShotSize = XilEnvInternal_CopyFromRefToPipe (TaskInfos, &(pAck->SnapShotData[0]));
    pAck->StructSize = Size;

    if (1) {
        DWORD BytesWritten, BytesWrittenPart;
        BOOL Success;

        BytesWritten = 0;
        while (BytesWritten < (DWORD)Size) {
            Success = TaskInfos->WriteMessage(TaskInfos->PipeOrSocketHandle,  // pipe handle
                                              (void*)pAck,                    // message
                                              (DWORD)Size,                    // message length
                                              &BytesWrittenPart);             // bytes written
            if (!Success) {
#ifdef _WIN32
                ThrowError(1, "WriteFile to pipe failed. GLE=%d", GetLastError());
#else
                ThrowError(1, "WriteFile to pipe failed. GLE=%d", errno);
#endif
                return -1;
            }
            BytesWritten += BytesWrittenPart;
        }
    }
    return 0;
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ EnterAndExecuteOneProcessLoop (EXTERN_PROCESS_TASK_HANDLE ProcessHandle)
{
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos = (EXTERN_PROCESS_TASK_INFOS_STRUCT*)ProcessHandle;
    HANDLE hPipe = TaskInfos->PipeOrSocketHandle;
    DWORD BytesRead = 0;
    //XilEnvInternal_QemuLog("EnterAndExecuteOneProcessLoop()\n");
    for (;;) {
        BOOL Success;
        DWORD BytesReadPart;
        Success = TaskInfos->ReadMessage (hPipe, (void*)((char*)TaskInfos->pCmdMessageBuffer + BytesRead), PIPE_MESSAGE_BUFSIZE, &BytesReadPart);
        if (!Success) {
#ifdef _WIN32
            char *lpMsgBuf;
            DWORD dw = GetLastError();
            FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL,
                            dw,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPTSTR) &lpMsgBuf,
                            0, NULL );
            ThrowError (1, "Connection lost: %s", lpMsgBuf);
            LocalFree (lpMsgBuf);
#else
            ThrowError(1, "Connection to lost: %s", strerror(errno));
#endif
            // terminate process
            KillExternProcessHimSelf();
        }
        BytesRead += BytesReadPart;
        if ((BytesRead >= sizeof (PIPE_API_BASE_CMD_MESSAGE)) &&
            (BytesRead >= (DWORD)TaskInfos->pCmdMessageBuffer->StructSize)) {
            int ResponseLen;
            //XilEnvInternal_QemuLog("call DecodePipeCmdMessage\n");
            ResponseLen = DecodePipeCmdMessage (TaskInfos, TaskInfos->pCmdMessageBuffer, (int)BytesRead, TaskInfos->pCmdMessageAckBuffer);
            if (ResponseLen > 0) {
                DWORD BytesWritten;
                Success = TaskInfos->WriteMessage (hPipe,                                     // pipe handle
                                                    (void*)TaskInfos->pCmdMessageAckBuffer,   // message
                                                    (DWORD)ResponseLen,                       // message length
                                                    &BytesWritten);                           // bytes written
                if (!Success) {
#ifdef _WIN32
                    ThrowError (1, "WriteFile to pipe failed. GLE=%d", GetLastError());
#else
                    ThrowError (1, "WriteFile to pipe failed. GLE=%d", errno);
#endif
                }
            } else if (ResponseLen == 0) {
                ThrowError (1, "not expected message command=%i", TaskInfos->pCmdMessageBuffer->Command);
            } else {
                // external prozess will be terminated from master
                XilEnvInternal_KillExternProcessFrom (TaskInfos);
                break;
            }
            BytesRead = 0;
        }
    }
}

#ifdef _WIN32
static DWORD WINAPI XilEnvInternal_CommunicationThreadFunction (LPVOID lpParam)
#else
static void *XilEnvInternal_CommunicationThreadFunction(void* lpParam)
#endif
{
    HANDLE hPipe;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos = (EXTERN_PROCESS_TASK_INFOS_STRUCT*)lpParam;

    hPipe = XilEnvInternal_ConnectToAndLogin (TaskInfos,  1);
    if (hPipe != INVALID_HANDLE_VALUE) {
        EnterAndExecuteOneProcessLoop (TaskInfos);
    } else {
        KillExternProcessHimSelf ();
    }
    return 0;
}


int XilEnvInternal_StartCommunicationThread (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos)
{
#ifdef _WIN32
    HANDLE hThread;
    DWORD dwThreadId = 0;

    hThread = CreateThread (
            NULL,              // no security attribute
            0,                 // default stack size
            XilEnvInternal_CommunicationThreadFunction,    // thread proc
            (void*)TaskInfos,    // thread parameter
            0,                 // not suspended
            &dwThreadId);      // returns thread ID

    if (hThread == NULL)  {
        char *lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &lpMsgBuf,
                       0, NULL );
        ThrowError (1, "cannot create communication thread: %s", lpMsgBuf);
        LocalFree (lpMsgBuf);
        return -1;
    }
#elif defined(__linux__)
    int ret;
    pthread_t Thread;
    pthread_attr_t Attr;

    pthread_attr_init(&Attr);

    /* Set a specific stack size  */
    ret = pthread_attr_setstacksize(&Attr, 256 * 1024);
    if (ret) {
        ThrowError (1, "pthread setstacksize failed\n");
        return -1;
    }
    if (pthread_create(&Thread, &Attr, XilEnvInternal_CommunicationThreadFunction, (void*)TaskInfos) != 0) {
        ThrowError(1, "cannot create communication thread %s", strerror(errno));
        return -1;
    }
    pthread_attr_destroy(&Attr);
#endif
    return 0;
}


static int GetShortExeFilename (char *par_ProcessName, char *ret_ExeFilename, int par_MaxChars)
{
    char *p = par_ProcessName;
    char *ppo = NULL;
    char *pbs = NULL;

    while (*p != 0) {
        if (*p == '.') ppo = p;
        else if (*p == '\\') pbs = p + 1;
        p++;
    }
    if (ppo != NULL) {  // last '.' inside process name
        if (((ppo[1] == 'e') || (ppo[1] == 'E')) &&
            ((ppo[2] == 'x') || (ppo[2] == 'X')) &&
            ((ppo[3] == 'e') || (ppo[3] == 'E'))) {  // followed by "EXE"
            int Len;
            if (pbs != NULL) {
                par_ProcessName = pbs;
            }
            Len = (int)(p - par_ProcessName);
            if ((Len + 1) >= par_MaxChars) {
                return -1;   // Error: to long name!
            }
            MEMCPY (ret_ExeFilename, par_ProcessName, (size_t)Len);
            ret_ExeFilename[Len] = 0;
            return 0;
        }
    }
    return -1; // It is not an external process
}


static char *GetErrCodeToString (int ErrorCode)
{
#ifdef _WIN32
    static LPVOID lpMsgBuf;

    if (lpMsgBuf == NULL) LocalFree (lpMsgBuf);
    lpMsgBuf = NULL;
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   ErrorCode,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                   (LPTSTR) &lpMsgBuf,
                   0, NULL);
    if (lpMsgBuf == NULL) return "no explanation";
    return (char*)lpMsgBuf;
#elif defined(__linux__)
    return strerror(ErrorCode);
#else
    return "no explanation";
#endif
}

#ifdef _WIN32
typedef BOOL (WINAPI *p_EnumProcessModules_type)(HANDLE,
		                                         HMODULE *,
	                                             DWORD,
		                                         PDWORD);

typedef BOOL (WINAPI *p_GetModuleInformation_type)(HANDLE,
		                                           HMODULE,
		                                           LPMODULEINFO,
	                                               DWORD);

static int XilEnvInternal_GetExternProcessBaseAddress (void **ret_Address, HMODULE hModule)
{
    DWORD cbNeeded;
    HANDLE hProcess;
    HMODULE hMods[1024];
    MODULEINFO ModInfo;
    p_EnumProcessModules_type p_EnumProcessModules;
    p_GetModuleInformation_type p_GetModuleInformation;

#ifdef DYNAMIC_LOAD_PSAPI_DLL
    // This API functions not available for gcc, dynamic load the DLL.
    HINSTANCE h = LoadLibrary("psapi.dll");
    if (h) {
    	p_EnumProcessModules = (p_EnumProcessModules_type)GetProcAddress(h, "EnumProcessModules");
    	p_GetModuleInformation = (p_GetModuleInformation_type)GetProcAddress(h, "GetModuleInformation");
        if ((p_EnumProcessModules == NULL) || (p_GetModuleInformation == NULL)) {
            ThrowError (1, "cannot resolve method EnumProcessModules or GetModuleInformation inside psapi.dll");
        	FreeLibrary (h);
        	return -1;
        }
    } else {
        ThrowError (1, "cannot load DLL papi.dll");
    	return -1;
    }
    #define MyFreeLibrary(h) FreeLibrary(h)
#else
    p_EnumProcessModules = EnumProcessModules;
    p_GetModuleInformation = GetModuleInformation;
    #define MyFreeLibrary(h)
#endif

    hProcess = GetCurrentProcess ();
    if (NULL == hProcess) {
        ThrowError (1, "cannot get current process handle: %s", GetErrCodeToString (GetLastError ()));
        MyFreeLibrary (h);
        return -1;
    }
    if (hModule == INVALID_HANDLE_VALUE) {
        // If no modul (DLL) specify than use the executable
        if (!p_EnumProcessModules (hProcess, hMods, sizeof(hMods), &cbNeeded)) {
            ThrowError (1, "cannot get process module handles: %s", GetErrCodeToString (GetLastError ()));
            MyFreeLibrary (h);
            return -1;
        }
        hModule = hMods[0];
    }
    if (!p_GetModuleInformation (hProcess, hModule, &ModInfo, sizeof (ModInfo))) {
        ThrowError (1, "cannot get process module infos: %s", GetErrCodeToString (GetLastError ()));
        MyFreeLibrary (h);
        return -1;
    }
    *ret_Address = ModInfo.lpBaseOfDll;
    MyFreeLibrary (h);
    return 0;
}
#else
#define HMODULE    HANDLE
static int XilEnvInternal_GetExternProcessBaseAddress(void **ret_Address, HMODULE hModule)
{
    *ret_Address = NULL;
    return 0;
}
#endif


static EXTERN_PROCESS_TASK_INFOS_STRUCT *BuildAndAddNewTaskInfoStruct (const char *ProcessName,
                                                                       const char *InstanceName,
                                                                       const char *ServerName,
                                                                       const char *TaskName,
                                                                       const char *ExecutableName,
                                                                       int Priority,
                                                                       int CycleDivider,
                                                                       int Delay,
                                                                       int LoginTimeout,
                                                                       void (*reference) (void),
                                                                       int (*init) (void),
                                                                       void (*cyclic) (void),
                                                                       void (*terminate) (void),
                                                                       const char *DllName)
{
    EXTERN_PROCESS_TASK_INFOS_STRUCT *NewTaskInfo;
    HMODULE hModulDll;

    //XilEnvInternal_QemuLog ("BuildAndAddNewTaskInfoStruct()\n");
    if  ((ProcessInfos.ThreadPos >= ProcessInfos.ThreadCount) || (ProcessInfos.ThreadPos >= MAX_PROCESSES_INSIDE_ONE_EXECUTABLE)) {
        return NULL;
    }

    NewTaskInfo = (EXTERN_PROCESS_TASK_INFOS_STRUCT*)XilEnvInternal_calloc (1, sizeof (EXTERN_PROCESS_TASK_INFOS_STRUCT));
    if (NewTaskInfo == NULL) return NULL;

    NewTaskInfo->Priority = Priority;
    NewTaskInfo->CycleDivider = CycleDivider;
    NewTaskInfo->Delay = Delay;
    NewTaskInfo->LoginTimeout = LoginTimeout;

    // If process is inside a DLL, load the DLL here
    if (DllName != NULL) {
        #ifdef _WIN32
        //ListDLLFunctions(DllName);
        hModulDll = LoadLibrary (DllName);
        if (hModulDll == NULL) {
            char *lpMsgBuf = NULL;
            DWORD dw = GetLastError ();
            FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL,
                           dw,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPTSTR) &lpMsgBuf,
                           0, NULL);
            ThrowError (1, "cannot load DLL \"%s\": %s", DllName, lpMsgBuf);
            LocalFree (lpMsgBuf);
            free (NewTaskInfo);
            return NULL;
        }
        // Store base addresse
        XilEnvInternal_GetExternProcessBaseAddress (&(NewTaskInfo->DllBaseAddress), hModulDll);
        NewTaskInfo->DllSize = 0;
        if ((reference == NULL) && (init == NULL) && (cyclic == NULL) && (terminate == NULL)) {
            // Functions are inside the  DLL (call convetion DLL __stdcall not __cdecl)!
            NewTaskInfo->CallConvention = CALL_CONVENTION_STDCALL;
            NewTaskInfo->FuncPtrs.Stdcall.reference = (void (__stdcall *)(void))GetProcAddress(hModulDll, "reference_varis");  // default
            if ( NewTaskInfo->FuncPtrs.Stdcall.reference == NULL) {
                NewTaskInfo->FuncPtrs.Stdcall.reference = (void (__stdcall *)(void))GetProcAddress(hModulDll, "_reference_varis@0");  // mit _ ist VisualC
            }
            if ( NewTaskInfo->FuncPtrs.Stdcall.reference == NULL) {
                NewTaskInfo->FuncPtrs.Stdcall.reference = (void (__stdcall *)(void))GetProcAddress(hModulDll, "reference_varis@0");  // ohne _ ist GCC
            }
            if ( NewTaskInfo->FuncPtrs.Stdcall.reference == NULL) {
                char DllNameWithPath[MAX_PATH];
                GetModuleFileName(hModulDll,  DllNameWithPath, sizeof (DllNameWithPath));
                ThrowError (1, "cannot find function \"reference_varis\" inside DLL \"%s\"", DllNameWithPath);
                free (NewTaskInfo);
                return NULL;
            }
            NewTaskInfo->FuncPtrs.Stdcall.init = (int (__stdcall *)(void))GetProcAddress(hModulDll, "init_test_object");  // default
            if ( NewTaskInfo->FuncPtrs.Stdcall.init == NULL) {
                NewTaskInfo->FuncPtrs.Stdcall.init = (int (__stdcall *)(void))GetProcAddress(hModulDll, "_init_test_object@0"); // mit _ ist VisualC
            }
            if ( NewTaskInfo->FuncPtrs.Stdcall.init == NULL) {
                NewTaskInfo->FuncPtrs.Stdcall.init = (int (__stdcall *)(void))GetProcAddress(hModulDll, "init_test_object@0");  // ohne _ ist GCC
            }
            if ( NewTaskInfo->FuncPtrs.Stdcall.init == NULL) {
                char DllNameWithPath[MAX_PATH];
                GetModuleFileName(hModulDll,  DllNameWithPath, sizeof (DllNameWithPath));
                ThrowError (1, "cannot find function \"init_test_object\" inside DLL \"%s\"", DllNameWithPath);
                free (NewTaskInfo);
                return NULL;
            }
            NewTaskInfo->FuncPtrs.Stdcall.cyclic = (void (__stdcall *)(void))GetProcAddress(hModulDll, "cyclic_test_object");  // default
            if ( NewTaskInfo->FuncPtrs.Stdcall.cyclic == NULL) {
                NewTaskInfo->FuncPtrs.Stdcall.cyclic = (void (__stdcall *)(void))GetProcAddress(hModulDll, "_cyclic_test_object@0"); // mit _ ist VisualC
            }
            if ( NewTaskInfo->FuncPtrs.Stdcall.cyclic == NULL) {
                NewTaskInfo->FuncPtrs.Stdcall.cyclic = (void (__stdcall *)(void))GetProcAddress(hModulDll, "cyclic_test_object@0");  // ohne _ ist GCC
            }
            if ( NewTaskInfo->FuncPtrs.Stdcall.cyclic == NULL) {
                char DllNameWithPath[MAX_PATH];
                GetModuleFileName(hModulDll,  DllNameWithPath, sizeof (DllNameWithPath));
                ThrowError (1, "cannot find function \"cyclic_test_object\" inside DLL \"%s\"", DllNameWithPath);
                free (NewTaskInfo);
                return NULL;
            }
            NewTaskInfo->FuncPtrs.Stdcall.terminate = (void (__stdcall *)(void))GetProcAddress(hModulDll, "terminate_test_object");  // default
            if ( NewTaskInfo->FuncPtrs.Stdcall.terminate == NULL) {
                NewTaskInfo->FuncPtrs.Stdcall.terminate = (void (__stdcall *)(void))GetProcAddress(hModulDll, "_terminate_test_object@0"); // mit _ ist VisualC
            }
            if ( NewTaskInfo->FuncPtrs.Stdcall.terminate == NULL) {
                NewTaskInfo->FuncPtrs.Stdcall.terminate = (void (__stdcall *)(void))GetProcAddress(hModulDll, "terminate_test_object@0");  // ohne _ ist GCC
            }
            if ( NewTaskInfo->FuncPtrs.Stdcall.terminate == NULL) {
                char DllNameWithPath[MAX_PATH];
                GetModuleFileName(hModulDll,  DllNameWithPath, sizeof (DllNameWithPath));
                ThrowError (1, "cannot find function \"terminate_test_object\" inside DLL \"%s\"", DllNameWithPath);
                free (NewTaskInfo);
                return NULL;
            }
        } else {
            // Functions are inside the EXE
            NewTaskInfo->CallConvention = CALL_CONVENTION_CDECL;
            NewTaskInfo->FuncPtrs.Cdecl.reference = reference;
            NewTaskInfo->FuncPtrs.Cdecl.init = init;
            NewTaskInfo->FuncPtrs.Cdecl.cyclic = cyclic;
            NewTaskInfo->FuncPtrs.Cdecl.terminate = terminate;
        }
#elif defined(__linux__)
        if ((reference == NULL) && (init == NULL) && (cyclic == NULL) && (terminate == NULL)) {
            ThrowError(1, "cannot load shared librarys (todo)");
            return NULL;
        } else {
            // Funktionen sind innerhalb der EXE
            NewTaskInfo->CallConvention = CALL_CONVENTION_CDECL;
            NewTaskInfo->FuncPtrs.Cdecl.reference = reference;
            NewTaskInfo->FuncPtrs.Cdecl.init = init;
            NewTaskInfo->FuncPtrs.Cdecl.cyclic = cyclic;
            NewTaskInfo->FuncPtrs.Cdecl.terminate = terminate;
        }
#else
        // QEMU DLL are the elf file name
#endif
    } else {
        hModulDll = NULL_INT_OR_PTR;
        NewTaskInfo->DllBaseAddress = NULL;
        NewTaskInfo->DllSize = 0;
            // Functions are outside the EXE
        NewTaskInfo->CallConvention = CALL_CONVENTION_CDECL;
        NewTaskInfo->FuncPtrs.Cdecl.reference = reference;
        NewTaskInfo->FuncPtrs.Cdecl.init = init;
        NewTaskInfo->FuncPtrs.Cdecl.cyclic = cyclic;
        NewTaskInfo->FuncPtrs.Cdecl.terminate = terminate;
    }
    NewTaskInfo->ThreadCount = ProcessInfos.ThreadCount;
    NewTaskInfo->Number = ProcessInfos.ThreadPos;
    strcpy (NewTaskInfo->InstanceName, InstanceName);
    strcpy (NewTaskInfo->ServerName, ServerName);
    if ((ExecutableName != NULL) && (strlen (ExecutableName) > 0)) {
        strcpy (NewTaskInfo->ExecutableName, ExecutableName);
    } else {
        //GetModuleFileName (NULL, NewTaskInfo->ExecutableName, sizeof (NewTaskInfo->ExecutableName));
        strcpy (NewTaskInfo->ExecutableName, ProcessInfos.ExecutableName);
    }
    strcpy (NewTaskInfo->ProcessAtTaskName, ProcessName);
    strcpy (NewTaskInfo->TaskName, TaskName);
    if (strlen (NewTaskInfo->TaskName)) {
        strcat (NewTaskInfo->ProcessAtTaskName, "@");
        strcat (NewTaskInfo->ProcessAtTaskName, TaskName);
    }
#ifdef _WIN32
    if (hModulDll != NULL) {
        GetModuleFileName(hModulDll,  NewTaskInfo->DllName, sizeof ( NewTaskInfo->DllName));
    } else {
        if (DllName != NULL) {
            strncpy(NewTaskInfo->DllName, DllName, sizeof(NewTaskInfo->DllName));
        } else {
            NewTaskInfo->DllName[0] = 0;
        }
    }
#elif defined(__linux__)
#else
    // QEMU the DLL are the elf file name
    if (DllName != NULL) {
        strncpy(NewTaskInfo->DllName, DllName, sizeof(NewTaskInfo->DllName));
    } else {
        NewTaskInfo->DllName[0] = 0;  // ist wegen calloc eigentlich nicht noetig
    }
#endif
    NewTaskInfo->pCmdMessageBuffer = (PIPE_API_BASE_CMD_MESSAGE*)XilEnvInternal_malloc (PIPE_MESSAGE_BUFSIZE);
    NewTaskInfo->pCmdMessageAckBuffer = (PIPE_API_BASE_CMD_MESSAGE_ACK*)XilEnvInternal_malloc (PIPE_MESSAGE_BUFSIZE);
#ifdef _WIN32
    NewTaskInfo->TaskMutex = CreateMutex (NULL,              // default security attributes
                                          FALSE,             // initially not owned
                                          NULL);             // unnamed mutex
    NewTaskInfo->XcpMutex = CreateMutex (NULL,              // default security attributes
                                         FALSE,             // initially not owned
                                         NULL);             // unnamed mutex
#elif defined(__linux__)
    {
        pthread_mutexattr_t Attr;
        pthread_mutexattr_init(&Attr);
        pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE); 
        pthread_mutex_init(&NewTaskInfo->TaskMutex, &Attr);
        pthread_mutex_init(&NewTaskInfo->XcpMutex, &Attr);
    }
#else
    // QEMU no multi core
#endif

    XilEnvInternal_VirtualNetworkInit(NewTaskInfo);

    ProcessInfos.TasksInfos[ProcessInfos.ThreadPos] = NewTaskInfo;
    ProcessInfos.ThreadPos++;

    //XilEnvInternal_QemuLog ("BuildAndAddNewTaskInfoStruct done\n");
    return NewTaskInfo;
}

#ifndef _WIN32
static char **BuildParameterArray(const char *par_StartExePath, const char *par_ParameterString)
{
    char **Ret = NULL;
    int Elements = 1;  // Start with 1 for program name
    int Pos = 0;
    int TotalLen = strlen(par_StartExePath) + 1 + strlen(par_ParameterString) + 1;
    char *Buffer = XilEnvInternal_malloc(TotalLen);
    char *p;
    
    // Build the command string
    strcpy(Buffer, par_StartExePath);
    strcat(Buffer, " ");
    strcat(Buffer, par_ParameterString);
    
    // First element is the program name
    Ret = XilEnvInternal_malloc(sizeof(char*) * 2);  // At least prog name + NULL
    Ret[0] = (char*)par_StartExePath;
    Pos = 1;
    
    // Parse additional parameters
    p = Buffer + strlen(par_StartExePath);
    while (*p != 0) {
        while (isascii(*p) && isspace(*p)) p++;
        if (*p != 0) {
            Elements++;
            Ret = XilEnvInternal_realloc(Ret, (size_t)(Elements + 1) * sizeof(char*));
            Ret[Pos] = p;
            Pos++;
            while (*p != 0 && !(isascii(*p) && isspace(*p))) p++;
            if (*p != 0) {
                *p = 0;
                p++;
            }
        }
    }
    Ret[Pos] = NULL;
    return Ret;
}
#endif

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ CheckIfConnectedToEx (EXTERN_PROCESS_TASKS_LIST *ExternProcessTasksList, int ExternProcessTasksListElementCount,
                                                                   EXTERN_PROCESS_TASK_HANDLE *ret_ProcessHandle)
{
    static int AlreadyLoggedInFlag;
    int x;
    if (!AlreadyLoggedInFlag) {
        char *CommandLine;
        char ServerName[MAX_PATH];
        char InstanceName[MAX_PATH];
        char CallFrom[8*MAX_PATH];   // Max. 8 * nested prozess calls: Extproc1 calls Extproc2 calls ... Extproc8 calls XilEnv
        int Priority = -1;
        int CycleDivider = -1;
        int Delay = -1;
        int LoginTimeout = 100 * 1000;   // default is 100s
        int Quiet;
        int MaxWait;
        char *StartExePath;
        char *StartExeCmdLine;
        int Count;
        int DontBreakOutOfJob;
        int SocketOrPipe;    // 1 -> Socket, 0 -> Pipe
        int IsRunning;
        int NoGui;
        int Err2Msg;
        int NoXcp;


#ifdef DEBUG_COMMANDLINE
        char Name[MAX_PATH+4];
        FILE *fh;
#endif

        AlreadyLoggedInFlag = 1;
        XilEnvInternal_CStartupIsFinished();   // from now on directly reference
        InitAddReferenceToBlackboardAtEndOfFunction();  // this must call once at start

#ifdef EXTP_CONFIG_HAVE_COMMANDLINE
        CommandLine = GetCommandLine ();

#ifdef DEBUG_COMMANDLINE
        GetModuleFileName (NULL, Name, MAX_PATH);
        strcat (Name, ".txt");
        fh = fopen (Name, "wt");
        if (fh != NULL) {
            fprintf (fh, "Called with command line:\n%s\n", CommandLine);
            fflush(fh);
        }
#endif
        XilEnvInternal_ParseCmdLine (CommandLine, CallFrom, sizeof (CallFrom), InstanceName, sizeof (InstanceName), ServerName, sizeof(ServerName),
                                     &Priority, &CycleDivider, &Delay, &Quiet, &MaxWait, &StartExePath, &StartExeCmdLine,
                                     &DontBreakOutOfJob, &NoGui, &Err2Msg, &NoXcp, ProcessInfos.WriteBackExeToDir, sizeof(ProcessInfos.WriteBackExeToDir));
#endif
        if (!ProcessInfos.UserSetExecutableName) {
#if defined(_WIN32) || defined(__linux__)
             GetModuleFileName (NULL, ProcessInfos.ExecutableName, sizeof (ProcessInfos.ExecutableName));
#endif
        }

        XilEnvInternal_GetExternProcessBaseAddress(&ProcessInfos.ExecutableBaseAddress, INVALID_HANDLE_VALUE);

        // If a server is defined use a socket connection.
        // If not use on Windows a named pipe and on Linux local domain sockets
        SocketOrPipe = (strlen(ServerName) > 0);

        if (SocketOrPipe) {
#ifdef EXTP_CONFIG_HAVE_SOCKETS
            IsRunning = XilEnvInternal_IsSocketInstanceIsRunning(InstanceName, ServerName, 500);
#else
            IsRunning = 0;
            ThrowError(1, "No sockets support");
#endif
        } else {
#ifdef _WIN32
            IsRunning = XilEnvInternal_IsPipeInstanceIsRunning(InstanceName, ServerName, 500);
#elif defined(__linux__)
            IsRunning = XilEnvInternal_IsUnixDomainSocketInstanceIsRunning(InstanceName, ServerName, 500);
#else
            IsRunning = 0;
            // QEMU cannot start any program on host!
#endif
        }

#ifdef EXTP_CONFIG_HAVE_START_XILENV
        if (!IsRunning) {
            // not running
            if (StartExePath != NULL) {   // -w oder -q as a parameter
                char WorkDir[MAX_PATH];
                char ExecutableName[MAX_PATH];

                if (GetShortExeFilename (ProcessInfos.ExecutableName, ExecutableName, sizeof (ExecutableName)) != 0) {
                    strcpy (ExecutableName, ProcessInfos.ExecutableName);
                }
#ifdef _WIN32
                _getcwd (WorkDir, sizeof (WorkDir));
#elif defined(__linux__)
                getcwd(WorkDir, sizeof(WorkDir));
#endif

#ifdef DEBUG_COMMANDLINE
                if (fh != NULL) {
                    fprintf (fh, "\nCalled %s with paramater:\n%s\n", StartExePath, CmdLine);
                    fflush(fh);
                }
#endif
                {
                    int Ret;
#ifdef _WIN32
                    Ret = GetEnvironmentVariable(ENVIRONMENT_VARNAME_CALLFROM, CallFrom, sizeof(CallFrom));
                    if (Ret >= sizeof(CallFrom)) {
                        ThrowError (1, "too many call froms (will be ignored)");
                        CallFrom[0] = 0;
                    } else if (Ret == 0) {
                        CallFrom[0] = 0;
                    }
#else
                    char *EnvVar;
                    EnvVar = getenv(ENVIRONMENT_VARNAME_CALLFROM);
                    if (EnvVar != NULL) {
                        Ret = (int)strlen(EnvVar);
                        if (Ret >= sizeof(CallFrom)) {
                            ThrowError(1, "too many call froms (will be ignored)");
                            CallFrom[0] = 0;
                        }
                        else {
                            strcpy(CallFrom, EnvVar);
                        }
                    } else {
                        CallFrom[0] = 0;
                    }
#endif

                    Ret = (int)strlen(CallFrom);
                    if ((Ret + (int)strlen(ExecutableName) + 2) > (int)sizeof(CallFrom)) {
                        ThrowError (1, "too many call froms (will be ignored)");
                    } else {
                        if (Ret > 0) {
                            strcat(CallFrom, ",");
                        }
                        strcat (CallFrom, ExecutableName);
                        if (!SetEnvironmentVariable(ENVIRONMENT_VARNAME_CALLFROM, CallFrom)) {
                            ThrowError (1, "cannot set call froms (will be ignored)");
                        }
                    }
                    if (strlen(InstanceName) == 0) {
                        // If no instance name was given as a transfer parameter
                        // look inside the environ variable for an instance name
                        Ret = GetEnvironmentVariable(ENVIRONMENT_VARNAME_INSTANCE, InstanceName, sizeof(InstanceName));
                        if (Ret >= sizeof(InstanceName)) {
                            ThrowError (1, "too long instance name (will be ignored)");
                            InstanceName[0] = 0;
                        } else if (Ret == 0) {
                            InstanceName[0] = 0;
                        }
                    }
                    if (!SetEnvironmentVariable(ENVIRONMENT_VARNAME_INSTANCE, InstanceName)) {
                        ThrowError (1, "cannot set instance (will be ignored)");
                    }
                }
#ifdef _WIN32
                {
                    STARTUPINFO StartupInfo;
                    PROCESS_INFORMATION ProcessInformation;
                    BOOL Status;
                    char *CmdBuffer;

                    CmdBuffer = malloc (strlen(StartExePath) + strlen(StartExeCmdLine) + 2);
                    if (CmdBuffer == NULL) {
                        ThrowError (1, "cannot alloc memory for \"%s\" \"%s\"", StartExePath, StartExeCmdLine);
                        return -1;
                    }
                    strcpy (CmdBuffer, StartExePath);
                    if (strlen(StartExeCmdLine)) {
                        strcat (CmdBuffer, " ");
                        strcat (CmdBuffer, StartExeCmdLine);
                    }
                    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
                    if (!DontBreakOutOfJob) {
                        // Try to breakout of an job
                        Status = CreateProcess(NULL, CmdBuffer, NULL, NULL, TRUE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &StartupInfo, &ProcessInformation);
                    }
                    if (DontBreakOutOfJob || (!DontBreakOutOfJob &&  !Status)) {
                        // Start without breakout of an job
                        Status = CreateProcess(NULL, CmdBuffer, NULL, NULL, TRUE, 0, NULL, NULL, &StartupInfo, &ProcessInformation);
                    }
                    if (!Status) {
                        ThrowError (1, "cannot start \"%s\" \"%s\"", StartExePath, StartExeCmdLine);
                        KillExternProcessHimSelf ();
                        return -1;
                    }
                }
#elif defined(__linux__)
                {
                    pid_t pid;

                    pid = fork();
                    if (pid == 0) {
                        // Child
                        if (execvp(StartExePath, BuildParameterArray(StartExePath, StartExeCmdLine)) < 0) {
                            ThrowError(1, "cannot start \"%s\" \"%s\"", StartExePath, StartExeCmdLine);
                            KillExternProcessHimSelf();
                            exit(-1);
                        }
                    }
                }
#endif
                if (SocketOrPipe) {
#ifdef EXTP_CONFIG_HAVE_SOCKETS
                    IsRunning = XilEnvInternal_WaitTillSocketInstanceIsRunning(InstanceName, ServerName, 100*1000);
#else
                    IsRunning = 0;
                    ThrowError(1, "sockets are not supported");
#endif
                } else {
#ifdef _WIN32
                    IsRunning = XilEnvInternal_WaitTillPipeInstanceIsRunning(InstanceName, ServerName, 100*1000);
#else
                    IsRunning = XilEnvInternal_WaitTillUnixDomainSocketInstanceIsRunning(InstanceName, ServerName, 100*1000);
#endif
                }
                if (!IsRunning) {
                    ThrowError (1, "Timeout starting \"%s\" \"%s\" give up", StartExePath, StartExeCmdLine);
                    KillExternProcessHimSelf ();
                    return -1;
                }
            } else {
                ThrowError (1, "XilEnv is not running and should not be started (instance = \"%s\" server = \"%s\")", InstanceName, ServerName);
                return -1;
            }
        }
#endif // EXTP_CONFIG_HAVE_START_XILENV

#ifdef DEBUG_COMMANDLINE
        if (fh != NULL) fclose (fh);
#endif

#ifdef _WIN32
        ProcessInfos.XcpMutex = CreateMutex (NULL,              // default security attributes
                                             FALSE,             // initially not owned
                                             NULL);             // unnamed mutex
#elif defined(__linux__)
        pthread_mutex_init(&ProcessInfos.XcpMutex, NULL);
#endif

#ifdef _WIN32
        ProcessInfos.ProcessMutex = CreateMutex (NULL,              // default security attributes
                                                 FALSE,             // initially not owned
                                                 NULL);             // unnamed mutex
#elif defined(__linux__)
        pthread_mutex_init(&ProcessInfos.ProcessMutex, NULL);
#endif

        strncpy (ProcessInfos.Prefix, InstanceName, sizeof(ProcessInfos.Prefix));
        ProcessInfos.Prefix[sizeof(ProcessInfos.Prefix)-1] = 0;

#ifdef EXTP_CONFIG_HAVE_KILL_EVENT
        XilEnvInternal_StartKillEventThread (InstanceName);
#endif

        // first only counting
        for (Count = 0; (Count < 32) && (Count < ExternProcessTasksListElementCount) && ExternProcessTasksList[Count].TaskName != NULL; Count++) {
        }
        ProcessInfos.ThreadCount = Count;
        for (x = 0; x < Count; x++) {
            EXTERN_PROCESS_TASK_INFOS_STRUCT *NewTask = BuildAndAddNewTaskInfoStruct (ProcessInfos.ExecutableName,
                                                                                      ProcessInfos.Prefix,
                                                                                      ServerName,
                                                                                      ExternProcessTasksList[x].TaskName,
                                                                                      ProcessInfos.ExecutableName,
                                                                                      Priority,
                                                                                      CycleDivider,
                                                                                      Delay,
                                                                                      LoginTimeout,
                                                                                      ExternProcessTasksList[x].reference,         // Variablen-Referenzierungs-Funk.
                                                                                      ExternProcessTasksList[x].init,              // Init-Funk.
                                                                                      ExternProcessTasksList[x].cyclic,            // Zyklische Funk.
                                                                                      ExternProcessTasksList[x].terminate,         // Beenden-Funk.
                                                                                      ExternProcessTasksList[x].DllName);
            if (SocketOrPipe) {
#ifdef EXTP_CONFIG_HAVE_SOCKETS
                XilEnvInternal_InitSocketCommunication(NewTask);
#else
                ThrowError (1, "have no socket support");
#endif
            } else {
#ifdef EXTP_CONFIG_HAVE_NAMEDPIPES
                XilEnvInternal_InitPipeCommunication(NewTask);
#endif
#ifdef EXTP_CONFIG_HAVE_UNIX_DOMAIN_SOCKETS
                XilEnvInternal_InitUnixDomainSocketCommunication(NewTask);
#endif
#ifdef EXTP_CONFIG_HAVE_QEMU_SIL_CON
                XilEnvInternal_InitQemuCommunication(NewTask);
#endif
            }

            if (NewTask != NULL) {
                if ((ret_ProcessHandle != NULL) && (x == 0)) {
                    // First process should not execute inside an own thread
                    if (XilEnvInternal_ConnectToAndLogin (NewTask, 1) != INVALID_HANDLE_VALUE) {
                        *ret_ProcessHandle = NewTask;
                    } else {
                        return -1;
                    }
                } else if (XilEnvInternal_StartCommunicationThread (NewTask)) {
                    return -1;
                }
            }
        }
        //XilEnvInternal_ReadSectionInfosFromAllExecutable(); // Only for testing
    }
    return 0;
}

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ CheckIfConnectedTo (EXTERN_PROCESS_TASKS_LIST *ExternProcessTasksList, int ExternProcessTasksListElementCount)
{
    return CheckIfConnectedToEx (ExternProcessTasksList, ExternProcessTasksListElementCount, NULL);
}

// Get scheduling cycle, period and process cycle, period, delay:
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ GetSchedulingInformation (int *ret_SeparateCyclesForRefAndInitFunction,
                                                                       unsigned long long *ret_SchedulerCycleCounter,
                                                                       long long *ret_SchedulerPeriod,
                                                                       unsigned long long *ret_MainSchedulerCycleCounter,
                                                                       unsigned long long *ret_ProcessCycleCounter,
                                                                       int *ret_ProcessPerid,
                                                                       int *ret_ProcessDelay)
{
    PIPE_API_GET_SCHEDUING_INFORMATION_CMD_MESSAGE Req;
    PIPE_API_GET_SCHEDUING_INFORMATION_CMD_MESSAGE_ACK Ack;
    BOOL Status;
    DWORD BytesRead;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfos;
    int Ret;

    TaskInfos = XilEnvInternal_GetTaskPtr ();
    Req.Command = PIPE_API_GET_SCHEDUING_INFORMATION_CMD;
    Req.StructSize = (int)sizeof (PIPE_API_GET_SCHEDUING_INFORMATION_CMD_MESSAGE);
    Req.Optional = 0;
    Status = XilEnvInternal_TransactMessage(TaskInfos,
                                            (LPVOID)&Req,
                                            sizeof(Req),
                                            (LPVOID)&Ack,
                                            sizeof(Ack),
                                            &BytesRead);

    if (!Status || (BytesRead != sizeof(Ack))) {
        XilEnvInternal_ThrowError (TaskInfos, 1, "get scheduler cycle, period and process cycle, delay message error %i or %i != %i\n",  Status, BytesRead, sizeof(Ack));
        Ret = -1;
    } else {
        if (ret_SeparateCyclesForRefAndInitFunction != NULL) *ret_SeparateCyclesForRefAndInitFunction = Ack.SeparateCyclesForRefAndInitFunction;
        if (ret_SchedulerCycleCounter != NULL) *ret_SchedulerCycleCounter = Ack.SchedulerCycle;
        if (ret_SchedulerPeriod != NULL) *ret_SchedulerPeriod = Ack.SchedulerPeriod;
        if (ret_MainSchedulerCycleCounter != NULL) *ret_MainSchedulerCycleCounter =Ack.MainSchedulerCycleCounter;
        if (ret_ProcessCycleCounter != NULL) *ret_ProcessCycleCounter = Ack.ProcessCycle;
        if (ret_ProcessPerid != NULL) *ret_ProcessPerid = Ack.ProcessPeriod;
        if (ret_ProcessDelay != NULL) *ret_ProcessDelay =Ack.ProcessDelay;
        Ret = 0;
    }
    XilEnvInternal_ReleaseTaskPtr(TaskInfos);
    return Ret;
}

// Loop out funkcion
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ LoopOutAndWaitForBarriers (const char *par_BarrierBeforeName, const char *par_BarrierBehindName)
{
    PIPE_API_LOOP_OUT_CMD_MESSAGE *Req;
    PIPE_API_LOOP_OUT_CMD_MESSAGE_ACK *Ack;
    BOOL Status;
    DWORD BytesRead;
    DWORD ReqSize;
    DWORD AckSize;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskInfos;
    int Ret;

    TaskInfos = XilEnvInternal_GetTaskPtr ();
    ReqSize = (DWORD)(sizeof (PIPE_API_LOOP_OUT_CMD_MESSAGE) - 1) + (DWORD)(TaskInfos->CopyLists.WrList8.SizeInBytes + TaskInfos->CopyLists.WrList4.SizeInBytes + TaskInfos->CopyLists.WrList2.SizeInBytes + TaskInfos->CopyLists.WrList1.SizeInBytes);
    AckSize = (DWORD)(sizeof (PIPE_API_LOOP_OUT_CMD_MESSAGE_ACK) - 1) + (DWORD)(TaskInfos->CopyLists.RdList8.SizeInBytes + TaskInfos->CopyLists.RdList4.SizeInBytes + TaskInfos->CopyLists.RdList2.SizeInBytes + TaskInfos->CopyLists.RdList1.SizeInBytes);
    Req = (PIPE_API_LOOP_OUT_CMD_MESSAGE*)XilEnvInternal_malloc ((size_t)ReqSize);
    Ack = (PIPE_API_LOOP_OUT_CMD_MESSAGE_ACK*)XilEnvInternal_malloc ((size_t)AckSize);
    Req->Command = PIPE_API_LOOP_OUT_CMD;
    strncpy (Req->BarrierBeforeName, par_BarrierBeforeName, sizeof (Req->BarrierBeforeName));
    Req->BarrierBeforeName[sizeof (Req->BarrierBeforeName)-1] = 0;
    strncpy (Req->BarrierBehindName, par_BarrierBehindName, sizeof (Req->BarrierBehindName));
    Req->BarrierBehindName[sizeof (Req->BarrierBehindName)-1] = 0;
    Req->SnapShotSize = XilEnvInternal_CopyFromRefToPipe (TaskInfos, &(Req->SnapShotData[0]));
    Req->StructSize = (int)sizeof (PIPE_API_LOOP_OUT_CMD_MESSAGE);

    Status = XilEnvInternal_TransactMessage(TaskInfos,
                                         (LPVOID)Req,
                                         ReqSize,
                                         (LPVOID)Ack,
                                         AckSize,
                                         &BytesRead);

    if (!Status || (BytesRead != AckSize)) {
        XilEnvInternal_ThrowError (TaskInfos, 1, "loop out message error %i or %i != %i\n",  Status, BytesRead, AckSize);
        Ret = -1;
    } else {
        if (TaskInfos->OverlayBuffers.Count > 0) {
            XilEnvInternal_CopyFromPipeToRefWithOverlayCheck (TaskInfos, &(Ack->SnapShotData[0]), Ack->SnapShotSize);
        } else {
            XilEnvInternal_CopyFromPipeToRef (TaskInfos, &(Ack->SnapShotData[0]), Ack->SnapShotSize);
        }
        Ret = 0;
    }
    XilEnvInternal_ReleaseTaskPtr(TaskInfos);

    XilEnvInternal_free (Req);
    XilEnvInternal_free (Ack);
    return Ret;
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ DisconnectFrom (EXTERN_PROCESS_TASK_HANDLE Process, int ImmediatelyFlag)
{
    PIPE_API_LOGOUT_MESSAGE Req;
    PIPE_API_LOGOUT_MESSAGE_ACK Ack;
    BOOL Status;
    DWORD BytesRead;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos = (EXTERN_PROCESS_TASK_INFOS_STRUCT*)Process;

    if (TaskInfos == NULL) {
        TaskInfos = XilEnvInternal_GetTaskPtr();
        if (TaskInfos == NULL) {
            ThrowError (1, "cannot disconnect from XilEnv (no valid process handle)");
            return;
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfos);
    }

    Req.Command = PIPE_API_LOGOUT_CMD;
    Req.StructSize = sizeof (PIPE_API_LOGOUT_MESSAGE);
    Req.Pid = (int16_t)get_process_identifier ();
    Req.ImmediatelyFlag = ImmediatelyFlag;

    Status = TaskInfos->TransactMessage (TaskInfos->PipeOrSocketHandle,
                                         (LPVOID)&Req,
                                         sizeof (Req),
                                         (LPVOID)&Ack,
                                         sizeof (Ack),
                                         &BytesRead);
    if (!Status || (BytesRead != sizeof (Ack))) {
#ifdef _WIN32
        char *lpMsgBuf;
        DWORD dw = GetLastError ();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &lpMsgBuf,
                       0, NULL );
        ThrowError (1, "cannot logout form XilEnv: %s", lpMsgBuf);
        LocalFree (lpMsgBuf);
#else
        ThrowError(1, "cannot logout form XilEnv: %s", strerror(errno));
#endif
    }
    if (ImmediatelyFlag) {
        TaskInfos->AlivePing.TerminateFlag = 1;
#ifdef WITH_ALIVE_PING_EVENTS
        SetEvent (TaskInfos->AlivePing.ReqEvent);
        while (TaskInfos->AlivePing.TerminateFlag != 2) {
#ifdef _WIN32
            Sleep(10);
#else
            usleep(1000*10);
#endif
        }
#endif
        // And close the pipe handle
        TaskInfos->DisconnectFrom (TaskInfos->PipeOrSocketHandle);
#ifdef WITH_ALIVE_PING_EVENTS
        CloseHandle (TaskInfos->AlivePing.ReqEvent);
        CloseHandle (TaskInfos->AlivePing.AckEvent);
        CloseHandle (TaskInfos->AlivePing.AsyncCommunicationThreadHandle);
#endif
#ifdef _WIN32
        CloseHandle (TaskInfos->TaskMutex);
        CloseHandle (TaskInfos->XcpMutex);
#elif defined(__linux__)
        pthread_mutex_destroy(&TaskInfos->TaskMutex);
        pthread_mutex_destroy(&TaskInfos->XcpMutex);
#endif
        RemoveAndDeleteTaskInfoStruct (TaskInfos);
    }
}

EXTERN_PROCESS_TASK_HANDLE __FUNC_CALL_CONVETION__ BuildNewProcessAndConnectToEx (const char *ProcessName,
                                                                                  const char *InstanceName,
                                                                                  const char *ServerName,
                                                                                  const char *ProcessNamePostfix,
                                                                                  const char *ExecutableName,
                                                                                  const char *DllName,
                                                                                  int Priority,
                                                                                  int CycleDivider,
                                                                                  int Delay,
                                                                                  int LoginTimeout,
                                                                                  void (*reference) (void),
                                                                                  int (*init) (void),
                                                                                  void (*cyclic) (void),
                                                                                  void (*terminate) (void))
{
    EXTERN_PROCESS_TASK_INFOS_STRUCT *Ret;
    int loc_ErrorMsgFlag = 1;

    //XilEnvInternal_QemuLog ("BuildNewProcessAndConnectToEx()\n");

    if (LoginTimeout < 0) {  // if timeout is negative no error message
        LoginTimeout = -LoginTimeout;
        loc_ErrorMsgFlag = 0;
    }

    ProcessInfos.ThreadCount = 1;
    Ret = BuildAndAddNewTaskInfoStruct (ProcessName, InstanceName, ServerName, ProcessNamePostfix, ExecutableName,
                                        Priority,CycleDivider, Delay, LoginTimeout,
                                        reference, init, cyclic, terminate, DllName);
    if (Ret != NULL) {
#ifdef EXTP_CONFIG_HAVE_NAMEDPIPES
        XilEnvInternal_InitPipeCommunication(Ret);
#endif
#ifdef EXTP_CONFIG_HAVE_UNIX_DOMAIN_SOCKETS
        // Nur Unix Domain Sockets moeglich!
        XilEnvInternal_InitUnixDomainSocketCommunication(Ret);
#endif
#ifdef EXTP_CONFIG_HAVE_QEMU_SIL_CON
        XilEnvInternal_InitQemuCommunication(Ret);
#endif
        XilEnvInternal_CStartupIsFinished();   // Now directly reference!
        InitAddReferenceToBlackboardAtEndOfFunction();  // this must be called one time
        if (XilEnvInternal_ConnectToAndLogin (Ret, loc_ErrorMsgFlag) == INVALID_HANDLE_VALUE) {
            return NULL;
        }
    }
    return (EXTERN_PROCESS_TASK_HANDLE)Ret;
}

EXTERN_PROCESS_TASK_HANDLE __FUNC_CALL_CONVETION__ BuildNewProcessAndConnectTo (const char *ProcessName, const char *InstanceName, const char *ProcessNamePostfix, const char *ExecutableName, int Timeout_ms)
{
    //XilEnvInternal_QemuLog ("BuildNewProcessAndConnectTo()\n");
    return BuildNewProcessAndConnectToEx(ProcessName, InstanceName, "", ProcessNamePostfix, ExecutableName, NULL,
                                         -1, -1, -1, Timeout_ms,
                                         NULL, NULL, NULL, NULL);
}

void *TranslateXCPOverEthernetAddress (size_t par_Address)
{
    uint32_t Address = par_Address;
    if ((Address & 0x80000000) == 0x80000000) {
        // It is a DLL address because the first bit is set:
        // This is a defenition here: The following 3 bits wold be represent an index into the process tabele.
        // Therefore max. 28 bits a elative address inside the XCP implementation are possible
        int Index;
        Index = (Address >> 28) & 0x7;
        if (Index > ProcessInfos.ThreadCount) {
            ThrowError (1, "cannot translate address 0x%08X because 3 bit index %i inside address is out of range (0...%i)", Address, Index, ProcessInfos.ThreadCount);
            return (void*)par_Address;
        }
        // calculate new DLL address
        return (void*)((char*)ProcessInfos.TasksInfos[Index]->DllBaseAddress + (Address & 0x0FFFFFFF));
    } else {
        return (void*)par_Address;
    }
}
