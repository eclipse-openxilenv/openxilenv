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


#include <stdint.h>
#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <alloca.h>
#include <sys/types.h>
#include <netinet/tcp.h>  // CP_NODELAY
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */

#include "Message.h"
#include "Fifos.h"
#include "MyMemory.h"
#include "RealtimeScheduler.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "StructRM.h"
#include "StructsRM_Blackboard.h"
#include "StructsRM_Scheduler.h"

#define UNUSED(x) (void)(x)

int client_socket_for_requests;

void set_client_socket_for_request(int socket)
{
    client_socket_for_requests = socket;
}

int ConnectToClinetForRequests(char *Address, int Port)
{
    struct sockaddr_in server;
    int OptVal;
    socklen_t OptLen;
    
    client_socket_for_requests = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket_for_requests == -1) {
        printf("Could not create socket");
        return -1;
    }
    puts("Socket created");

    OptVal = 0;
    OptLen = sizeof(OptVal);
    if (getsockopt(client_socket_for_requests, IPPROTO_TCP, TCP_NODELAY, (void*)&OptVal, &OptLen)) {
        printf("TCP_NODELAY Value: %i\n", OptVal);
    }
    OptVal = 1;
    if (setsockopt(client_socket_for_requests, IPPROTO_TCP, TCP_NODELAY, &OptVal, sizeof(OptVal))) {
        printf("cannot set TCP_NODELAY\n");
    }
    OptVal = 0;
    OptLen = sizeof(OptVal);
    if(getsockopt(client_socket_for_requests, IPPROTO_TCP, TCP_NODELAY, (char *)&OptVal, &OptLen)) {
        printf("TCP_NODELAY Value: %i\n", OptVal);
    }

    memset(&server.sin_addr, 0, sizeof(server.sin_addr));
    server.sin_addr.s_addr = inet_addr(Address);
    server.sin_family = AF_INET;
    server.sin_port = htons(Port);

    if (connect(client_socket_for_requests, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect failed. Error");
        return -1;
    }
    puts("Connected\n");

    return 0;
}


int SentReqToClient(void *par_Data, int par_Command, int par_len)
{
    int buffer_pos = 0;
    ((RM_PACKAGE_HEADER*)par_Data)->SizeOf = par_len;
    ((RM_PACKAGE_HEADER*)par_Data)->Command = par_Command;
    ((RM_PACKAGE_HEADER*)par_Data)->ThreadId =0;

    do {
        int SendBytes = send(client_socket_for_requests, (unsigned char*)par_Data + buffer_pos, par_len - buffer_pos, 0);
        if (SendBytes != par_len) {
            printf("debug stop\n");
        }
        if (SendBytes < 0) {
            ThrowError(1, "send failed");
            return -1;
        }
        buffer_pos += SendBytes;
    } while (buffer_pos < par_len);

    return 0;
}

int ReceiveAckFromClient(void *ret_Data, int par_len)
{
    int buffer_pos = 0;
    int receive_message_size = 0;
    do {
        int ReadBytes = recv(client_socket_for_requests, (unsigned char*)ret_Data + buffer_pos, par_len - buffer_pos, 0);
        if (ReadBytes > 0) {
            buffer_pos += ReadBytes;
            if (receive_message_size == 0) {
                receive_message_size = *(uint32_t*)ret_Data;
            }
        }
        else if (ReadBytes == 0) {
            ThrowError(1, "Connection closed\n");
            return -1;
        }
        else {
            ThrowError(1, "recv failed");
            return -1;
        }
    } while (buffer_pos < receive_message_size);
    return 0;
}

int TransactRemoteMaster(int par_Command, void *par_DataToRM, int par_LenDataToRM, void *ret_DataFromRM, int par_LenDataFromRM)
{
    int Ret;
    if (SentReqToClient(par_DataToRM, par_Command, par_LenDataToRM) == 0) {
        if ((Ret = ReceiveAckFromClient(ret_DataFromRM, par_LenDataFromRM)) > 0) {
            return Ret;
        }
        else {
            return -1;
        }
    }
    else {
        return -1;
    }
    return Ret;
}

static int ToHostPCFifoHandle = -1;

int rm_read_varinfos_from_ini(int Vid, char *Name, uint32_t ReadReqMask)
{
    RM_BLACKBOARD_READ_BBVARI_FROM_INI_REQ *Req;
    int LenName = strlen(Name) + 1;
    int StructSize = sizeof(RM_BLACKBOARD_READ_BBVARI_FROM_INI_REQ) + LenName;
    Req = alloca(StructSize);
    Req->Vid = Vid;
    Req->ReadReqMask = ReadReqMask;
    Req->OffsetName = sizeof(RM_BLACKBOARD_READ_BBVARI_FROM_INI_REQ);
    MEMCPY(Req + 1, Name, LenName);
    {
        int Status;
        if (ToHostPCFifoHandle < 0) {
            ToHostPCFifoHandle = TxAttachFiFo(0 /*would not be used*/, "ToHostPCFifo");
        }
        Req->PackageHeader.Command = RM_BLACKBOARD_READ_BBVARI_FROM_INI_CMD;
        Req->PackageHeader.SizeOf = StructSize;
        Status = WriteToFiFo(ToHostPCFifoHandle, RM_BLACKBOARD_READ_BBVARI_FROM_INI_CMD, 0 /*would not be used*/, get_timestamp_counter(), StructSize, Req);
        if (Status != 0) {
            printf("fifo full!\n");
        }
    }
    return 0;  
}

int rm_write_varinfos_to_ini(BB_VARIABLE *sp_vari_elem)
{
    RM_BLACKBOARD_WRITE_BBVARI_INFOS_REQ *Req;

    char *Conversion;
    int LenName;
    int LenUnit;
    int LenDisplayName;
    int LenComment;
    size_t LenConversion;
    int StructSize;

    LenName = strlen(sp_vari_elem->pAdditionalInfos->Name) + 1;
    if (sp_vari_elem->pAdditionalInfos->Unit != NULL) {
        LenUnit = strlen(sp_vari_elem->pAdditionalInfos->Unit) + 1;
    } else {
        LenUnit = 0;
    }
    if (sp_vari_elem->pAdditionalInfos->DisplayName != NULL) {
        LenDisplayName = strlen(sp_vari_elem->pAdditionalInfos->DisplayName) + 1;
    } else {
        LenDisplayName = 0;
    }
    if (sp_vari_elem->pAdditionalInfos->Comment != NULL) {
        LenComment = strlen(sp_vari_elem->pAdditionalInfos->Comment) + 1;
    } else {
        LenComment = 0;
    }
    switch (sp_vari_elem->pAdditionalInfos->Conversion.Type) {
    default:
    case BB_CONV_NONE:
    case BB_CONV_FACTOFF:
        LenConversion = 0;
        break;
    case BB_CONV_FORMULA:
        Conversion = (char*)(sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString);
        LenConversion = strlen(Conversion) + 1;
        break;
    case BB_CONV_TEXTREP:
        Conversion = (char*)(sp_vari_elem->pAdditionalInfos->Conversion.Conv.TextReplace.EnumString);
        LenConversion = strlen(Conversion) + 1;
        break;
    case BB_CONV_REF:
        Conversion = (char*)(sp_vari_elem->pAdditionalInfos->Conversion.Conv.Reference.Name);
        LenConversion = strlen(Conversion) + 1;
        break;
    }

    StructSize = sizeof(RM_BLACKBOARD_WRITE_BBVARI_INFOS_REQ) + LenName + LenUnit + LenDisplayName + LenComment + LenConversion;
    Req = (RM_BLACKBOARD_WRITE_BBVARI_INFOS_REQ*)alloca(StructSize);

    Req->BaseInfos = *sp_vari_elem;
    Req->AdditionalInfos = *(sp_vari_elem->pAdditionalInfos);

    Req->OffsetIniPath = 0;
    Req->OffsetName = sizeof(RM_BLACKBOARD_WRITE_BBVARI_INFOS_REQ);
    MEMCPY((char*)Req + Req->OffsetName, sp_vari_elem->pAdditionalInfos->Name,  LenName);
    Req->OffsetUnit = Req->OffsetName + LenName;
    MEMCPY((char*)Req + Req->OffsetUnit, sp_vari_elem->pAdditionalInfos->Unit, LenUnit);
    Req->OffsetConversion = Req->OffsetUnit + LenUnit;
    MEMCPY((char*)Req + Req->OffsetConversion, Conversion, LenConversion);
    if (LenConversion) {
        Req->OffsetDisplayName = Req->OffsetConversion + LenConversion;
        MEMCPY((char*)Req + Req->OffsetDisplayName, sp_vari_elem->pAdditionalInfos->DisplayName, LenDisplayName);
    } else {
        Req->OffsetDisplayName = 0;
    }
    if (LenComment) {
        Req->OffsetComment = Req->OffsetConversion + LenConversion + LenDisplayName;
        MEMCPY((char*)Req + Req->OffsetComment, sp_vari_elem->pAdditionalInfos->Comment, LenComment);
    } else {
        Req->OffsetComment = 0;
    }
    {
        int Status;
        if (ToHostPCFifoHandle < 0) {
            ToHostPCFifoHandle = TxAttachFiFo(0 /*would not be used*/, "ToHostPCFifo");
        }
        Req->PackageHeader.Command = RM_BLACKBOARD_WRITE_BBVARI_INFOS_CMD;
        Req->PackageHeader.SizeOf = StructSize;
        Status = WriteToFiFo(ToHostPCFifoHandle, RM_BLACKBOARD_WRITE_BBVARI_INFOS_CMD, 0 /*would not be used*/, get_timestamp_counter(), StructSize, Req);
        if (Status != 0) {
            printf("fifo full!\n");
        }
    }
    return 0;
}

int DbgInfoTranslatePointerToLabel(void *ptr, char *label, int sizeof_label, int *type_from_debuginfos, short Pid)
{
    UNUSED(ptr);
    UNUSED(label);
    UNUSED(sizeof_label);
    UNUSED(type_from_debuginfos);
    UNUSED(Pid);

    printf ("TODO\n");
    return 0;
}

void AddScriptMessage(char *txt)
{
    RM_ADD_SCRIPT_MESSAGE_REQ *Req;
    int LenText;
    int StructSize;
    int Status;

    LenText = strlen(txt) + 1;
    StructSize = sizeof(RM_ADD_SCRIPT_MESSAGE_REQ) + LenText;
    Req = (RM_ADD_SCRIPT_MESSAGE_REQ*)alloca(StructSize);
    Req->OffsetText = sizeof(RM_ADD_SCRIPT_MESSAGE_REQ);
    MEMCPY((char*)Req + Req->OffsetText, txt, LenText);
    if (ToHostPCFifoHandle < 0) {
        ToHostPCFifoHandle = TxAttachFiFo(0 /*would not be used*/, "ToHostPCFifo");
    }
    Req->PackageHeader.Command = RM_ADD_SCRIPT_MESSAGE_CMD;
    Req->PackageHeader.SizeOf = StructSize;
    Status = WriteToFiFo(ToHostPCFifoHandle, RM_ADD_SCRIPT_MESSAGE_CMD, 0 /*would not be used*/, get_timestamp_counter(), StructSize, Req);
    if (Status != 0) {
        printf("fifo full!\n");
    }
}

int AttachRegisterEquationPid(int Pid, uint64_t UniqueNumber)
{
    RM_BLACKBOARD_ATTACH_REGISTER_EQUATION_REQ Req;
    int Status;

    Req.UniqueNumber = UniqueNumber;
    Req.Pid = Pid;
    if (ToHostPCFifoHandle < 0) {
        ToHostPCFifoHandle = TxAttachFiFo(0 /*would not be used*/, "ToHostPCFifo");
    }
    Req.PackageHeader.Command = RM_BLACKBOARD_ATTACH_REGISTER_EQUATION_CMD;
    Req.PackageHeader.SizeOf = sizeof(Req);
    Status = WriteToFiFo(ToHostPCFifoHandle, RM_BLACKBOARD_ATTACH_REGISTER_EQUATION_CMD, 0 /*would not be used*/, get_timestamp_counter(), sizeof(Req), &Req);
    if (Status != 0) {
        printf("fifo full!\n");
    }

    return 0;
}

int AttachRegisterEquation(uint64_t UniqueNumber)
{
    return AttachRegisterEquationPid(GET_PID(), UniqueNumber);
}

int DetachRegisterEquationPid(int Pid, uint64_t UniqueNumber)
{
    RM_BLACKBOARD_DETACH_REGISTER_EQUATION_REQ Req;
    int Status;

    Req.UniqueNumber = UniqueNumber;
    Req.Pid = Pid;
    if (ToHostPCFifoHandle < 0) {
        ToHostPCFifoHandle = TxAttachFiFo(0 /*would not be used*/, "ToHostPCFifo");
    }
    Req.PackageHeader.Command = RM_BLACKBOARD_DETACH_REGISTER_EQUATION_CMD;
    Req.PackageHeader.SizeOf = sizeof(Req);
    Status = WriteToFiFo(ToHostPCFifoHandle, RM_BLACKBOARD_DETACH_REGISTER_EQUATION_CMD, 0 /*would not be used*/, get_timestamp_counter(), sizeof(Req), &Req);
    if (Status != 0) {
        printf("fifo full!\n");
    }

    return 0;
}

int DetachRegisterEquation(uint64_t UniqueNumber)
{
    return DetachRegisterEquationPid(GET_PID(), UniqueNumber);
}

void rm_ObserationCallbackFunction(void(*ObserationCallbackFunction) (VID Vid, uint32_t ObservationFlags, uint32_t ObservationData), VID Vid, uint32_t ObservationFlags, uint32_t ObservationData)
{
    UNUSED(ObserationCallbackFunction);
    RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_REQ Req;
    Req.Callback = 0;
    Req.Vid = Vid;
    Req.ObservationFlags = ObservationFlags;
    Req.ObservationData = ObservationData;
    {
        int Status;
        if (ToHostPCFifoHandle < 0) {
            ToHostPCFifoHandle = TxAttachFiFo(0 /*would not be used*/, "ToHostPCFifo");
        }
        Req.PackageHeader.Command = RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_CMD;
        Req.PackageHeader.SizeOf = sizeof(Req);
        Status = WriteToFiFo(ToHostPCFifoHandle, RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_CMD, 0 /*would not be used*/, get_timestamp_counter(), sizeof(Req), &Req);
        if (Status != 0) {
            printf("fifo full!\n");
        }
    }
}


void rm_SchedulerCyclicEvent(int SchedulerNr, uint64_t CycleCounter, uint64_t SimulatedTimeSinceStartedInNanoSecond)
{
    RM_SCHEDULER_CYCLIC_EVENT_REQ Req;
    Req.SchedulerNr = SchedulerNr;
    Req.CycleCounter = CycleCounter;
    Req.SimulatedTimeSinceStartedInNanoSecond = SimulatedTimeSinceStartedInNanoSecond;

    SentReqToClient(&Req, RM_SCHEDULER_CYCLIC_EVENT_CMD, sizeof(RM_SCHEDULER_CYCLIC_EVENT_REQ));
}



static int sc_error_internal(int level, char *txt)
{
    RM_ERROR_MESSAGE_REQ *Req;
    int LenText;
    int StructSize;
    int Status;

    LenText = strlen(txt) + 1;
    StructSize = sizeof(RM_ERROR_MESSAGE_REQ) + LenText;
    Req = (RM_ERROR_MESSAGE_REQ*)alloca(StructSize);
    Req->Cycle = get_rt_cycle_counter();
    Req->Level = level;
    Req->Pid = GET_PID();
    Req->OffsetText = sizeof(RM_ERROR_MESSAGE_REQ);
    MEMCPY((char*)Req + Req->OffsetText, txt, LenText);
    if (ToHostPCFifoHandle < 0) {
        ToHostPCFifoHandle = TxAttachFiFo(0 /*would not be used*/, "ToHostPCFifo");
    }
    Req->PackageHeader.Command = RM_ERROR_MESSAGE_CMD;
    Req->PackageHeader.SizeOf = StructSize;
    Status = WriteToFiFo(ToHostPCFifoHandle, RM_ERROR_MESSAGE_CMD, 0 /*would not be used*/, get_timestamp_counter(), StructSize, Req);
    if (Status != 0) {
        printf("fifo full!\n");
    }
    return 0;
}


int ThrowError(int level, const char *format, ...)
{
    char *MessageBuffer = NULL;
    int MaxLen, Len;
    va_list args;

    Len = 1024;
    do {
        MessageBuffer = (char*)my_realloc(MessageBuffer, Len);
        if (MessageBuffer == NULL) return -1;
        MaxLen = Len - 1;
        va_start(args, format);
        Len = vsnprintf(MessageBuffer, MaxLen, format, args);
        va_end(args);
    } while(Len >= MaxLen);

    // this is only for debugging
    //puts(MessageBuffer);

    sc_error_internal(level, MessageBuffer);

    my_free(MessageBuffer);

    return 0;
}

int RealtimeProcessStarted(int Pid, char *ProcessName)
{
    RM_REALTIME_PROCESS_STARTED_REQ *Req;
    int LenProcessName;
    int StructSize;
    int Status;

    LenProcessName = strlen(ProcessName) + 1;
    StructSize = sizeof(RM_REALTIME_PROCESS_STARTED_REQ) + LenProcessName;
    Req = (RM_REALTIME_PROCESS_STARTED_REQ*)alloca(StructSize);
    Req->Pid = Pid;
    Req->OffsetProcessName = sizeof(RM_REALTIME_PROCESS_STARTED_REQ);
    MEMCPY((char*)Req + Req->OffsetProcessName, ProcessName, LenProcessName);
    if (ToHostPCFifoHandle < 0) {
        ToHostPCFifoHandle = TxAttachFiFo(0 /*would not be used*/, "ToHostPCFifo");
    }
    Req->PackageHeader.Command = RM_REALTIME_PROCESS_STARTED_CMD;
    Req->PackageHeader.SizeOf = StructSize;
    Status = WriteToFiFo(ToHostPCFifoHandle, RM_REALTIME_PROCESS_STARTED_CMD, 0 /*would not be used*/, get_timestamp_counter(), StructSize, Req);
    if (Status != 0) {
        printf("fifo full!\n");
    }
    return 0;
}

int RealtimeProcessStopped(int Pid, char *ProcessName)
{
    RM_REALTIME_PROCESS_STOPPED_REQ *Req;
    int LenProcessName;
    int StructSize;
    int Status;

    LenProcessName = strlen(ProcessName) + 1;
    StructSize = sizeof(RM_REALTIME_PROCESS_STOPPED_REQ) + LenProcessName;
    Req = (RM_REALTIME_PROCESS_STOPPED_REQ*)alloca(StructSize);
    Req->Pid = Pid;
    Req->OffsetProcessName = sizeof(RM_REALTIME_PROCESS_STOPPED_REQ);
    MEMCPY((char*)Req + Req->OffsetProcessName, ProcessName, LenProcessName);
    if (ToHostPCFifoHandle < 0) {
        ToHostPCFifoHandle = TxAttachFiFo(0 /*would not be used*/, "ToHostPCFifo");
    }
    Req->PackageHeader.Command = RM_REALTIME_PROCESS_STOPPED_CMD;
    Req->PackageHeader.SizeOf = StructSize;
    Status = WriteToFiFo(ToHostPCFifoHandle, RM_REALTIME_PROCESS_STOPPED_CMD, 0 /*would not be used*/, get_timestamp_counter(), StructSize, Req);
    if (Status != 0) {
        printf("fifo full!\n");
    }
    return 0;
}
