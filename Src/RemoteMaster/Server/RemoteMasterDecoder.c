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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "Config.h"
#include "StructRM.h"
#include "StructsRM_Blackboard.h"
#include "StructsRM_Message.h"
#include "StructsRM_FiFo.h"
#include "StructsRM_Scheduler.h"
#include "StructsRM_CanFifo.h"

#include "StringMaxChar.h"
#include "ConfigurablePrefix.h"
#include "Blackboard.h"
#include "ExecutionStack.h"
#include "BlackboardAccess.h"
#include "ThrowError.h"
#include "CanFifo.h"
#include "RealtimeScheduler.h"
#include "RealtimeProcessEquations.h"
#include "MemoryAllocation.h"
#include "SearchAndInitHardware.h"
#include "BlackboardIniCache.h"
#include "RemoteMasterReadWriteMemory.h"
#include "RemoteMasterMessage.h"
#include "RemoteMasterModelInterface.h"
#include "RemoteMasterDecoder.h"

#define MIN_MEMORY_SIZE  (256*1024*1024)

// same in RemoteMasterServer.c!!!
#define BUFFER_SIZE  (1024 * 1024)

#define UNUSED(x) (void)(x)

static uint32_t Func_undefined(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Req);
    UNUSED(par_Ack);
    return 0;
}

static uint32_t Func_Init(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_INIT_REQ *Req = (RM_INIT_REQ*)par_Req;
    RM_INIT_ACK *Ack = (RM_INIT_ACK*)par_Ack;

    if (Req->PreAllocMemorySize < MIN_MEMORY_SIZE) {
        Req->PreAllocMemorySize = MIN_MEMORY_SIZE;
    }
    Ack->Ret = InitMemoryAllocator(Req->PreAllocMemorySize);

    if (!Ack->Ret) {
        int Pid;

        RemoteMasterInitConfigurablePrefix((const char*)Req, Req->OffsetConfigurablePrefix);

		InitFiFos();
        
		SearchAndInitHardware();

        #define GENERATE_UNIQUE_PID_COMMAND       0
        #define GENERATE_UNIQUE_RT_PID_COMMAND    4
        Pid = GetOrFreeUniquePid(GENERATE_UNIQUE_RT_PID_COMMAND, 0, "schedulers_internal");

        InitSchedulers(Pid);

        Ack->Ret = Pid;
    }
    Ack->Version = XILENV_VERSION;
    Ack->PatchVersion = XILENV_MINOR_VERSION;
    return sizeof(RM_INIT_ACK);
}

static uint32_t Func_Terminate(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Req);
    RM_TERMINATE_ACK *Ack = (RM_TERMINATE_ACK*)par_Ack;
    Ack->Ret = StopAllRealtimeScheduler();
    TerminateAllHardware();
    return sizeof(RM_TERMINATE_ACK);
}

static uint32_t Func_Kill(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Req);
    UNUSED(par_Ack);
    return -1;  // no ACK and terminte immediately
}

static uint32_t Func_KillThread(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Req);
    UNUSED(par_Ack);
    return -2;  // no ACK and terminte thread immediately
}

static uint32_t Func_Ping(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_PING_REQ *Req = (RM_PING_REQ*)par_Req;
    RM_PING_ACK *Ack = (RM_PING_ACK*)par_Ack;
    Ack->Ret = Req->Value;
    return sizeof(RM_PING_ACK);
}

static uint32_t Func_StartCopyFile(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    int SizeOfStruct;
    RM_COPY_FILE_START_REQ *Req = (RM_COPY_FILE_START_REQ*)par_Req;
    RM_COPY_FILE_START_ACK *Ack = (RM_COPY_FILE_START_ACK*)par_Ack;
    char *FileName = (char*)Req + Req->OffsetFileName;
    Ack->FileHandle = open(FileName, O_WRONLY | O_CREAT | O_TRUNC);
    if (Ack->FileHandle < 0) {
        Ack->Ret = -1;
        strcpy((char*)(Ack + 1), strerror(errno));
        Ack->OffsetErrorString = sizeof(RM_COPY_FILE_START_ACK);
        SizeOfStruct = sizeof(RM_COPY_FILE_START_ACK) + strlen((char*)(Ack + 1)) + 1;
    } else {
        Ack->Ret = 0;
        strcpy((char*)(Ack + 1), "");
        Ack->OffsetErrorString = sizeof(RM_COPY_FILE_START_ACK);
        SizeOfStruct = sizeof(RM_COPY_FILE_START_ACK) + 1;

    }
    return SizeOfStruct;
}

static uint32_t Func_NextCopyFile(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    int SizeOfStruct;
    RM_COPY_FILE_NEXT_REQ *Req = (RM_COPY_FILE_NEXT_REQ*)par_Req;
    RM_COPY_FILE_NEXT_ACK *Ack = (RM_COPY_FILE_NEXT_ACK*)par_Ack;

    if (write(Req->FileHandle, (char*)Req + Req->OffsetData, Req->BlockSize) != Req->BlockSize) {
        Ack->Ret = -1;
        strcpy((char*)(Ack + 1), strerror(errno));
        Ack->OffsetErrorString = sizeof(RM_COPY_FILE_NEXT_ACK);
        SizeOfStruct = sizeof(RM_COPY_FILE_NEXT_ACK) + strlen((char*)(Ack + 1)) + 1;
    } else {
        Ack->Ret = 0;
        strcpy((char*)(Ack + 1), "");
        Ack->OffsetErrorString = sizeof(RM_COPY_FILE_NEXT_ACK);
        SizeOfStruct = sizeof(RM_COPY_FILE_NEXT_ACK) + 1;

    }
    return SizeOfStruct;
}

static uint32_t Func_EndCopyFile(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    int SizeOfStruct;
    RM_COPY_FILE_END_REQ *Req = (RM_COPY_FILE_END_REQ*)par_Req;
    RM_COPY_FILE_END_ACK *Ack = (RM_COPY_FILE_END_ACK*)par_Ack;

    if (close(Req->FileHandle)) {
        Ack->Ret = -1;
        strcpy((char*)(Ack + 1), strerror(errno));
        Ack->OffsetErrorString = sizeof(RM_COPY_FILE_END_ACK);
        SizeOfStruct = sizeof(RM_COPY_FILE_END_ACK) + strlen((char*)(Ack + 1)) + 1;
    }
    else {
        Ack->Ret = 0;
        strcpy((char*)(Ack + 1), "");
        Ack->OffsetErrorString = sizeof(RM_COPY_FILE_END_ACK);
        SizeOfStruct = sizeof(RM_COPY_FILE_END_ACK) + 1;

    }
    return SizeOfStruct;
}

static uint32_t Func_ReadBytes(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    int SizeOfStruct;
    RM_READ_DATA_BYTES_REQ *Req = (RM_READ_DATA_BYTES_REQ*)par_Req;
    RM_READ_DATA_BYTES_ACK *Ack = (RM_READ_DATA_BYTES_ACK*)par_Ack;

    Ack->Ret = CheckAccessAndReadMemory((void*)Req->Address, Req->Size, (void*)(Ack + 1));
    Ack->OffsetData = sizeof(RM_READ_DATA_BYTES_ACK);
    if (Ack->Ret <= 0) {
        SizeOfStruct = sizeof(RM_READ_DATA_BYTES_ACK);
    }
    else {
        SizeOfStruct = sizeof(RM_READ_DATA_BYTES_ACK) + Req->Size;
    }
    return SizeOfStruct;
}

static uint32_t Func_WriteBytes(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_WRITE_DATA_BYTES_REQ *Req = (RM_WRITE_DATA_BYTES_REQ*)par_Req;
    RM_WRITE_DATA_BYTES_ACK *Ack = (RM_WRITE_DATA_BYTES_ACK*)par_Ack;

    Ack->Ret = CheckAccessAndWriteMemory((void*)Req->Address, Req->Size, (char*)Req + Req->OffsetData);

    return sizeof(RM_WRITE_DATA_BYTES_ACK);
}

static uint32_t Func_RefrenceVariable(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_REFERENCE_VARIABLE_REQ *Req = (RM_REFERENCE_VARIABLE_REQ*)par_Req;
    RM_REFERENCE_VARIABLE_ACK *Ack = (RM_REFERENCE_VARIABLE_ACK*)par_Ack;
    int vid;

    if ((vid = add_bbvari_pid_type((char*)(Req + 1), Req->Type, "", Req->Pid, Req->Dir, 1, (union BB_VARI*)(Req->Address), NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL)) <= 0) {
        referece_error((char*)(Req + 1), vid, Req->Type, "", (void*)Req->Address, 0, NULL, Req->Dir);
        Ack->Ret = -1;
    } else {
        add_vari_ref_list((void*)Req->Address, vid, Req->Dir);
        Ack->Ret = 0;
    }

    return sizeof(RM_REFERENCE_VARIABLE_ACK);
}

static uint32_t Func_DeRefrenceVariable(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_DEREFERENCE_VARIABLE_REQ *Req = (RM_DEREFERENCE_VARIABLE_REQ*)par_Req;
    RM_DEREFERENCE_VARIABLE_ACK *Ack = (RM_DEREFERENCE_VARIABLE_ACK*)par_Ack;
    int vid;

    vid = get_bbvarivid_by_name((char*)(Req + 1));
    Ack->Ret = dereference_var((void*)Req->Address, vid);

    return sizeof(RM_DEREFERENCE_VARIABLE_ACK);
}

static uint32_t Func_init_blackboard(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_INIT_BLACKBOARD_REQ *Req = (RM_BLACKBOARD_INIT_BLACKBOARD_REQ*)par_Req;
    RM_BLACKBOARD_INIT_BLACKBOARD_ACK *Ack = (RM_BLACKBOARD_INIT_BLACKBOARD_ACK*)par_Ack;
    Ack->Ret = init_blackboard(Req->blackboard_size, (char)Req->CopyBB2ProcOnlyIfWrFlagSet, (char)Req->AllowBBVarsWithSameAddr, (char)Req->conv_error2message);
    return sizeof(RM_BLACKBOARD_INIT_BLACKBOARD_ACK);
}

static uint32_t Func_close_blackboard(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Req);
    RM_BLACKBOARD_CLOSE_BLACKBOARD_ACK *Ack = (RM_BLACKBOARD_CLOSE_BLACKBOARD_ACK*)par_Ack;
    Ack->Ret = close_blackboard();
    return sizeof(RM_BLACKBOARD_CLOSE_BLACKBOARD_ACK);
}


static uint32_t Func_add_bbvari_pid_type(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_REQ *Req = (RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_REQ*)par_Req;
    RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_ACK *Ack = (RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_ACK*)par_Ack;
    char *Unit;
    char *Name = (char*)Req + Req->NameOffset;
    if (Req->UnitOffset) {
        Unit = (char*)Req + Req->UnitOffset;
    } else {
        Unit = NULL;
    }
    Ack->Ret = (int32_t)add_bbvari_pid_type(Name, Req->Type, Unit, Req->Pid, Req->Dir, Req->ValueValidFlag, (union BB_VARI *)&(Req->ValueUnion), &Ack->RetType, Req->ReadReqMask, NULL, NULL, NULL);
    return sizeof(RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_ACK);
}

static uint32_t Func_attach_bbvari(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_ATTACH_BBVARI_REQ *Req = (RM_BLACKBOARD_ATTACH_BBVARI_REQ*)par_Req;
    RM_BLACKBOARD_ATTACH_BBVARI_ACK *Ack = (RM_BLACKBOARD_ATTACH_BBVARI_ACK*)par_Ack;
    if (Req->unknown_wait_flag) {
        Ack->Ret = attach_bbvari_unknown_wait_pid(Req->Vid, Req->Pid);
    }
    else {
        Ack->Ret = attach_bbvari_pid(Req->Vid, Req->Pid);
    }
    return sizeof(RM_BLACKBOARD_ATTACH_BBVARI_ACK);
}

static uint32_t Func_attach_bbvari_by_name(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_REQ *Req = (RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_REQ*)par_Req;
    RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_ACK *Ack = (RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_ACK*)par_Ack;
    char *Name = (char*)(Req + 1);
    Ack->Ret = (int32_t)attach_bbvari_by_name(Name, Req->Pid);
    return sizeof(RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_ACK);
}

static uint32_t Func_remove_bbvari(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_REMOVE_BBVARI_REQ *Req = (RM_BLACKBOARD_REMOVE_BBVARI_REQ*)par_Req;
    RM_BLACKBOARD_REMOVE_BBVARI_ACK *Ack = (RM_BLACKBOARD_REMOVE_BBVARI_ACK*)par_Ack;
    if (Req->unknown_wait_flag) {
        Ack->Ret = remove_bbvari_unknown_wait_for_process_cs(Req->Vid, 1, Req->Pid);
    } else {
        Ack->Ret = remove_bbvari_for_process_cs(Req->Vid, 1, Req->Pid);
    }
    return sizeof(RM_BLACKBOARD_REMOVE_BBVARI_ACK);
}

static uint32_t Func_remove_all_bbvari(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_REMOVE_ALL_BBVARI_REQ *Req = (RM_BLACKBOARD_REMOVE_ALL_BBVARI_REQ*)par_Req;
    RM_BLACKBOARD_REMOVE_ALL_BBVARI_ACK *Ack = (RM_BLACKBOARD_REMOVE_ALL_BBVARI_ACK*)par_Ack;
    Ack->Ret = remove_all_bbvari((PID)Req->Pid);
    return sizeof(RM_BLACKBOARD_REMOVE_ALL_BBVARI_ACK);
}

static uint32_t Func_get_num_of_bbvaris(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Req);
    RM_BLACKBOARD_GET_NUM_OF_BBVARIS_ACK *Ack = (RM_BLACKBOARD_GET_NUM_OF_BBVARIS_ACK*)par_Ack;
    Ack->Ret = get_num_of_bbvaris();
    return sizeof(RM_BLACKBOARD_GET_NUM_OF_BBVARIS_ACK);
}

static uint32_t Func_get_bbvarivid_by_name(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_REQ *Req = (RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_ACK *Ack = (RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_ACK*)par_Ack;
    char *Name = (char*)Req + Req->NameOffset;
    Ack->Ret = (int32_t)get_bbvarivid_by_name(Name);
    return sizeof(RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_ACK);
}

static uint32_t Func_get_bbvaritype_by_name(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_REQ *Req = (RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_ACK *Ack = (RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_ACK*)par_Ack;
    char *Name = (char*)(Req + 1);
    Ack->Ret = get_bbvaritype_by_name(Name);
    return sizeof(RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_ACK);
}

static uint32_t Func_get_bbvari_attachcount(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_ATTACHCOUNT_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_ATTACHCOUNT_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_ATTACHCOUNT_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_ATTACHCOUNT_ACK*)par_Ack;
    Ack->Ret = get_bbvari_attachcount(Req->Vid);
    return sizeof(RM_BLACKBOARD_GET_BBVARI_ATTACHCOUNT_ACK);
}

static uint32_t Func_get_bbvaritype(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARITYPE_REQ *Req = (RM_BLACKBOARD_GET_BBVARITYPE_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARITYPE_ACK *Ack = (RM_BLACKBOARD_GET_BBVARITYPE_ACK*)par_Ack;
    Ack->Ret = get_bbvaritype(Req->Vid);
    return sizeof(RM_BLACKBOARD_GET_BBVARITYPE_ACK);
}

static uint32_t Func_GetBlackboardVariableName(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_NAME_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_NAME_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_NAME_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_NAME_ACK*)par_Ack;
    int SizeOfStruct;

    SizeOfStruct = sizeof(RM_BLACKBOARD_GET_BBVARI_NAME_REQ);

    char *RetName = (char*)(Ack + 1);
    Ack->Ret = GetBlackboardVariableName(Req->Vid, RetName, Req->MaxChar);
    return sizeof(RM_BLACKBOARD_GET_BBVARI_NAME_ACK) + strlen (RetName) + 1;
}


static uint32_t Func_GetBlackboardVariableNameAndTypes(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_ACK*)par_Ack;
    char *RetName = (char*)(Ack + 1);
    Ack->Ret = GetBlackboardVariableNameAndTypes(Req->Vid, RetName, Req->MaxChar, (enum BB_DATA_TYPES*)&(Ack->data_type),
                                                 (enum BB_CONV_TYPES*)&(Ack->conversion_type));
    Ack->NameOffset = sizeof(RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_ACK);
    return (uint32_t)(sizeof(RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_ACK) + strlen(RetName) + 1);
}

static uint32_t Func_get_process_bbvari_attach_count(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_PROCESS_BBVARI_ATTACH_COUNT_REQ *Req = (RM_BLACKBOARD_GET_PROCESS_BBVARI_ATTACH_COUNT_REQ*)par_Req;
    RM_BLACKBOARD_GET_PROCESS_BBVARI_ATTACH_COUNT_ACK *Ack = (RM_BLACKBOARD_GET_PROCESS_BBVARI_ATTACH_COUNT_ACK*)par_Ack;
    Ack->Ret = get_process_bbvari_attach_count_pid(Req->Vid, (PID)Req->Pid);
    return sizeof(RM_BLACKBOARD_GET_PROCESS_BBVARI_ATTACH_COUNT_ACK);
}

static uint32_t Func_set_bbvari_unit(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_SET_BBVARI_UNIT_REQ *Req = (RM_BLACKBOARD_SET_BBVARI_UNIT_REQ*)par_Req;
    RM_BLACKBOARD_SET_BBVARI_UNIT_ACK *Ack = (RM_BLACKBOARD_SET_BBVARI_UNIT_ACK*)par_Ack;
    char *Unit = (char*)(Req + 1);
    Ack->Ret = set_bbvari_unit(Req->Vid, Unit);
    return sizeof(RM_BLACKBOARD_SET_BBVARI_UNIT_ACK);
}

static uint32_t Func_get_bbvari_unit(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_UNIT_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_UNIT_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_UNIT_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_UNIT_ACK*)par_Ack;
    char *RetUnit = (char*)(Ack + 1);
    Ack->Ret = get_bbvari_unit(Req->Vid, RetUnit, Req->max_c);
    return (uint32_t)(sizeof(RM_BLACKBOARD_GET_BBVARI_UNIT_ACK) + strlen(RetUnit) + 1);
}

static uint32_t Func_set_bbvari_conversion(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_SET_BBVARI_CONVERSION_REQ *Req = (RM_BLACKBOARD_SET_BBVARI_CONVERSION_REQ*)par_Req;
    RM_BLACKBOARD_SET_BBVARI_CONVERSION_ACK *Ack = (RM_BLACKBOARD_SET_BBVARI_CONVERSION_ACK*)par_Ack;
    char *Conversion = (char*)Req + Req->ConversionOffset;
    struct EXEC_STACK_ELEM *exec_stack = (struct EXEC_STACK_ELEM*)((char*)Req + Req->BinConversionOffset);
    Ack->Ret = set_bbvari_conversion_x(Req->Vid, Req->ConversionType, Conversion, Req->sizeof_bin_conversion, exec_stack, NULL, NULL, NULL);
    return sizeof(RM_BLACKBOARD_SET_BBVARI_CONVERSION_ACK);
}

static uint32_t Func_get_bbvari_conversion(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_CONVERSION_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_CONVERSION_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_CONVERSION_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_CONVERSION_ACK*)par_Ack;
    char *RetConversion = (char*)(Ack + 1);
    Ack->Ret = get_bbvari_conversion(Req->Vid, RetConversion, Req->maxc);
    Ack->ConversionOffset = sizeof(RM_BLACKBOARD_GET_BBVARI_CONVERSION_ACK);
    return (uint32_t)(sizeof(RM_BLACKBOARD_GET_BBVARI_CONVERSION_ACK) + strlen(RetConversion) + 1);
}

static uint32_t Func_get_bbvari_conversiontype(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_CONVERSIONTYPE_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_CONVERSIONTYPE_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_CONVERSIONTYPE_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_CONVERSIONTYPE_ACK*)par_Ack;
    Ack->Ret = get_bbvari_conversiontype(Req->Vid);
    return sizeof(RM_BLACKBOARD_GET_BBVARI_CONVERSIONTYPE_ACK);
}

static uint32_t Func_get_bbvari_infos(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_INFOS_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_INFOS_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_INFOS_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_INFOS_ACK*)par_Ack;
    char *Data = (char*)(Ack + 1);
    char *Conversion;
    int LenName;
    int LenUnit;
    int LenDisplayName;
    int LenComment;
    int LenConversion;

    int SizeOfStruct;

    // olny for debugging
    SizeOfStruct = sizeof(RM_BLACKBOARD_GET_BBVARI_INFOS_ACK);


    Ack->BaseInfos.pAdditionalInfos = &(Ack->AdditionalInfos);
    Ack->Ret = get_bbvari_infos(Req->Vid, &(Ack->BaseInfos), &(Ack->AdditionalInfos), Data, Req->SizeOfBuffer);
    Ack->OffsetName = (uint32_t)sizeof(RM_BLACKBOARD_GET_BBVARI_INFOS_ACK) + (uint32_t)(Ack->AdditionalInfos.Name - Data);
    LenName = strlen(Ack->AdditionalInfos.Name) + 1;
    Ack->OffsetUnit = (uint32_t)sizeof(RM_BLACKBOARD_GET_BBVARI_INFOS_ACK) + (uint32_t)(Ack->AdditionalInfos.Unit - Data);
    if (Ack->AdditionalInfos.Unit != NULL) {
        LenUnit = strlen(Ack->AdditionalInfos.Unit) + 1;
        Ack->OffsetUnit = (uint32_t)sizeof(RM_BLACKBOARD_GET_BBVARI_INFOS_ACK) + (uint32_t)(Ack->AdditionalInfos.Unit - Data);
    } else {
        LenUnit = 0;
        Ack->OffsetUnit = 0;
    }
    if (Ack->AdditionalInfos.DisplayName != NULL) {
        Ack->OffsetDisplayName = (uint32_t)sizeof(RM_BLACKBOARD_GET_BBVARI_INFOS_ACK) + (uint32_t)(Ack->AdditionalInfos.DisplayName - Data);
        LenDisplayName = strlen(Ack->AdditionalInfos.DisplayName) + 1;
    } else {
        Ack->OffsetDisplayName = 0;
        LenDisplayName = 0;
    }
    if (Ack->AdditionalInfos.Comment != NULL) {
        Ack->OffsetComment = (uint32_t)sizeof(RM_BLACKBOARD_GET_BBVARI_INFOS_ACK) + (uint32_t)(Ack->AdditionalInfos.Comment - Data);
        LenComment = strlen(Ack->AdditionalInfos.Comment) + 1;
    } else {
        Ack->OffsetComment = 0;
        LenComment = 0;
    }
    switch (Ack->AdditionalInfos.Conversion.Type) {
    default:
    case BB_CONV_NONE:
    case BB_CONV_FACTOFF:
        Conversion = Data;
        LenConversion = 0;
        break;
    case BB_CONV_FORMULA:
        Conversion = (char*)(Ack->AdditionalInfos.Conversion.Conv.Formula.FormulaString);
        LenConversion = strlen(Conversion) + 1;
        break;
    case BB_CONV_TEXTREP:
        Conversion = (char*)(Ack->AdditionalInfos.Conversion.Conv.TextReplace.EnumString);
        LenConversion = strlen(Conversion) + 1;
        break;
    case BB_CONV_REF:
        Conversion = (char*)(Ack->AdditionalInfos.Conversion.Conv.Reference.Name);
        LenConversion = strlen(Conversion) + 1;
        break;
    }
    if (LenConversion) {
        Ack->OffsetConversion = (uint32_t)sizeof(RM_BLACKBOARD_GET_BBVARI_INFOS_ACK) + (uint32_t)(Conversion - Data);
    } else {
        Ack->OffsetConversion = 0;
    }
    return (uint32_t)(sizeof(RM_BLACKBOARD_GET_BBVARI_INFOS_ACK) +
        LenName +
        LenUnit +
        LenDisplayName +
        LenComment +
        LenConversion);
}

static uint32_t Func_set_bbvari_color(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_SET_BBVARI_COLOR_REQ *Req = (RM_BLACKBOARD_SET_BBVARI_COLOR_REQ*)par_Req;
    RM_BLACKBOARD_SET_BBVARI_COLOR_ACK *Ack = (RM_BLACKBOARD_SET_BBVARI_COLOR_ACK*)par_Ack;
    Ack->Ret = set_bbvari_color(Req->Vid, (unsigned long)Req->rgb_color);
    return sizeof(RM_BLACKBOARD_SET_BBVARI_COLOR_ACK);
}

static uint32_t Func_get_bbvari_color(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_COLOR_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_COLOR_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_COLOR_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_COLOR_ACK*)par_Ack;
    Ack->rgb_color = (int32_t)get_bbvari_color(Req->Vid);
    return sizeof(RM_BLACKBOARD_GET_BBVARI_COLOR_ACK);
}

static uint32_t Func_set_bbvari_step(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_SET_BBVARI_STEP_REQ *Req = (RM_BLACKBOARD_SET_BBVARI_STEP_REQ*)par_Req;
    RM_BLACKBOARD_SET_BBVARI_STEP_ACK *Ack = (RM_BLACKBOARD_SET_BBVARI_STEP_ACK*)par_Ack;
    Ack->Ret = set_bbvari_step(Req->Vid, Req->steptype, Req->step);
    return sizeof(RM_BLACKBOARD_SET_BBVARI_STEP_ACK);
}

static uint32_t Func_get_bbvari_step(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_STEP_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_STEP_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_STEP_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_STEP_ACK*)par_Ack;
    Ack->Ret = get_bbvari_step(Req->Vid, &(Ack->steptype), &(Ack->step));
    return sizeof(RM_BLACKBOARD_GET_BBVARI_STEP_ACK);
}

static uint32_t Func_set_bbvari_min(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_SET_BBVARI_MIN_REQ *Req = (RM_BLACKBOARD_SET_BBVARI_MIN_REQ*)par_Req;
    RM_BLACKBOARD_SET_BBVARI_MIN_ACK *Ack = (RM_BLACKBOARD_SET_BBVARI_MIN_ACK*)par_Ack;
    Ack->Ret = set_bbvari_min(Req->Vid, Req->min);
    return sizeof(RM_BLACKBOARD_SET_BBVARI_MIN_ACK);
}

static uint32_t Func_set_bbvari_max(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_SET_BBVARI_MAX_REQ *Req = (RM_BLACKBOARD_SET_BBVARI_MAX_REQ*)par_Req;
    RM_BLACKBOARD_SET_BBVARI_MAX_ACK *Ack = (RM_BLACKBOARD_SET_BBVARI_MAX_ACK*)par_Ack;
    Ack->Ret = set_bbvari_max(Req->Vid, Req->max);
    return sizeof(RM_BLACKBOARD_SET_BBVARI_MAX_ACK);
}

static uint32_t Func_get_bbvari_min(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_MIN_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_MIN_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_MIN_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_MIN_ACK*)par_Ack;
    Ack->min = get_bbvari_min(Req->Vid);
    return sizeof(RM_BLACKBOARD_GET_BBVARI_MIN_ACK);
}

static uint32_t Func_get_bbvari_max(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_MAX_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_MAX_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_MAX_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_MAX_ACK*)par_Ack;
    Ack->max = get_bbvari_max(Req->Vid);
    return sizeof(RM_BLACKBOARD_GET_BBVARI_MAX_ACK);
}

static uint32_t Func_set_bbvari_format(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_SET_BBVARI_FORMAT_REQ *Req = (RM_BLACKBOARD_SET_BBVARI_FORMAT_REQ*)par_Req;
    RM_BLACKBOARD_SET_BBVARI_FORMAT_ACK *Ack = (RM_BLACKBOARD_SET_BBVARI_FORMAT_ACK*)par_Ack;
    Ack->Ret = set_bbvari_format(Req->Vid, Req->width, Req->prec);
    return sizeof(RM_BLACKBOARD_SET_BBVARI_FORMAT_ACK);
}

static uint32_t Func_get_bbvari_format_width(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_FORMAT_WIDTH_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_FORMAT_WIDTH_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_FORMAT_WIDTH_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_FORMAT_WIDTH_ACK*)par_Ack;
    Ack->width = get_bbvari_format_width(Req->Vid);
    return sizeof(RM_BLACKBOARD_GET_BBVARI_FORMAT_WIDTH_ACK);
}

static uint32_t Func_get_bbvari_format_prec(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BBVARI_FORMAT_PREC_REQ *Req = (RM_BLACKBOARD_GET_BBVARI_FORMAT_PREC_REQ*)par_Req;
    RM_BLACKBOARD_GET_BBVARI_FORMAT_PREC_ACK *Ack = (RM_BLACKBOARD_GET_BBVARI_FORMAT_PREC_ACK*)par_Ack;
    Ack->prec = get_bbvari_format_prec(Req->Vid);
    return sizeof(RM_BLACKBOARD_GET_BBVARI_FORMAT_PREC_ACK);
}

static uint32_t Func_get_free_wrflag(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_FREE_WRFLAG_REQ *Req = (RM_BLACKBOARD_GET_FREE_WRFLAG_REQ*)par_Req;
    RM_BLACKBOARD_GET_FREE_WRFLAG_ACK *Ack = (RM_BLACKBOARD_GET_FREE_WRFLAG_ACK*)par_Ack;
    Ack->Ret = get_free_wrflag((PID)Req->Pid, &(Ack->wrflag));
    return sizeof(RM_BLACKBOARD_GET_FREE_WRFLAG_ACK);
}

static uint32_t Func_reset_wrflag(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_RESET_WRFLAG_REQ *Req = (RM_BLACKBOARD_RESET_WRFLAG_REQ*)par_Req;
    RM_BLACKBOARD_RESET_WRFLAG_ACK *Ack = (RM_BLACKBOARD_RESET_WRFLAG_ACK*)par_Ack;
    reset_wrflag(Req->Vid, Req->w); 
    Ack->Ret = 0;
    return sizeof(RM_BLACKBOARD_RESET_WRFLAG_ACK);
}

static uint32_t Func_test_wrflag(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_TEST_WRFLAG_REQ *Req = (RM_BLACKBOARD_TEST_WRFLAG_REQ*)par_Req;
    RM_BLACKBOARD_TEST_WRFLAG_ACK *Ack = (RM_BLACKBOARD_TEST_WRFLAG_ACK*)par_Ack;
    Ack->Ret = test_wrflag(Req->Vid, Req->w);
    return sizeof(RM_BLACKBOARD_TEST_WRFLAG_ACK);
}

static uint32_t Func_read_next_blackboard_vari(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_REQ *Req = (RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_REQ*)par_Req;
    RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_ACK *Ack = (RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_ACK*)par_Ack;
    char* RetName = (char*)(Ack + 1);
    Ack->Ret = read_next_blackboard_vari(Req->index, RetName, Req->max_c);
    return (uint32_t)(sizeof(RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_ACK) + strlen (RetName) + 1);
}

static uint32_t Func_ReadNextVariableProcessAccess(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_NEXT_VARI_PROCESS_ACCESS_REQ *Req = (RM_BLACKBOARD_READ_NEXT_VARI_PROCESS_ACCESS_REQ*)par_Req;
    RM_BLACKBOARD_READ_NEXT_VARI_PROCESS_ACCESS_ACK *Ack = (RM_BLACKBOARD_READ_NEXT_VARI_PROCESS_ACCESS_ACK*)par_Ack;
    char *RetName = (char*)(Ack + 1);
    Ack->Ret = ReadNextVariableProcessAccess(Req->index, (PID)Req->Pid, Req->access, RetName, Req->maxc);
    return (uint32_t)(sizeof(RM_BLACKBOARD_READ_NEXT_VARI_PROCESS_ACCESS_ACK) + strlen(RetName) + 1);
}

static uint32_t Func_enable_bbvari_access(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_ENABLE_BBVARI_ACCESS_REQ *Req = (RM_BLACKBOARD_ENABLE_BBVARI_ACCESS_REQ*)par_Req;
    RM_BLACKBOARD_ENABLE_BBVARI_ACCESS_ACK *Ack = (RM_BLACKBOARD_ENABLE_BBVARI_ACCESS_ACK*)par_Ack;
    Ack->Ret = enable_bbvari_access((PID)Req->Pid, Req->Vid);
    return sizeof(RM_BLACKBOARD_ENABLE_BBVARI_ACCESS_ACK);
}

static uint32_t Func_disable_bbvari_access(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_DISABLE_BBVARI_ACCESS_REQ *Req = (RM_BLACKBOARD_DISABLE_BBVARI_ACCESS_REQ*)par_Req;
    RM_BLACKBOARD_DISABLE_BBVARI_ACCESS_ACK *Ack = (RM_BLACKBOARD_DISABLE_BBVARI_ACCESS_ACK*)par_Ack;
    Ack->Ret = disable_bbvari_access((PID)Req->Pid, Req->Vid);
    return sizeof(RM_BLACKBOARD_DISABLE_BBVARI_ACCESS_ACK);
}

static uint32_t Func_SetObserationCallbackFunction(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_SET_OBSERVATION_CALLBACK_FUNCTION_REQ *Req = (RM_BLACKBOARD_SET_OBSERVATION_CALLBACK_FUNCTION_REQ*)par_Req;
    SetObserationCallbackFunction((void (*) (VID, uint32_t, uint32_t))Req->Callback);
    return sizeof(RM_BLACKBOARD_SET_OBSERVATION_CALLBACK_FUNCTION_ACK);
}

static uint32_t Func_SetBbvariObservation(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_SET_BBVARI_OBSERVATION_REQ *Req = (RM_BLACKBOARD_SET_BBVARI_OBSERVATION_REQ*)par_Req;
    RM_BLACKBOARD_SET_BBVARI_OBSERVATION_ACK *Ack = (RM_BLACKBOARD_SET_BBVARI_OBSERVATION_ACK*)par_Ack;
    Ack->Ret = SetBbvariObservation(Req->Vid, Req->ObservationFlags, Req->ObservationData);
    return sizeof(RM_BLACKBOARD_SET_BBVARI_OBSERVATION_ACK);
}

static uint32_t Func_SetGlobalBlackboradObservation(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_REQ *Req = (RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_REQ*)par_Req;
    RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_ACK *Ack = (RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_ACK*)par_Ack;
    int32_t *Vids; 
    int32_t Count;
    Ack->Ret = SetGlobalBlackboradObservation(Req->ObservationFlags, Req->ObservationData, &Vids, &Count);
    Ack->PackageHeader.Command = RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_CMD;
    MEMCPY(Ack + 1, Vids, Count * sizeof(int32_t));
    Ack->ret_Count = Count;
    Ack->OffsetVids = sizeof(RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_ACK);
    my_free(Vids);
    return sizeof(RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_ACK) + Count * sizeof(int32_t);
}

static uint32_t Func_get_bb_accessmask(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_BB_ACCESSMASK_REQ *Req = (RM_BLACKBOARD_GET_BB_ACCESSMASK_REQ*)par_Req;
    RM_BLACKBOARD_GET_BB_ACCESSMASK_ACK *Ack = (RM_BLACKBOARD_GET_BB_ACCESSMASK_ACK*)par_Ack;
    if (Req->BBPrefixOffset) {
        Ack->Ret = get_bb_accessmask(Req->pid, &Ack->ret_mask, (char*)Req + Req->BBPrefixOffset);
    } else {
        Ack->Ret = get_bb_accessmask(Req->pid, &Ack->ret_mask, NULL);
    }
    return sizeof(RM_BLACKBOARD_GET_BB_ACCESSMASK_ACK);
}

static uint32_t Func_AddVariableSectionCacheEntry(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    char *Name;
    char *Unit;
    char *Conversion;
    RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ *Req = (RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ*)par_Req;
    RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_ACK *Ack = (RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_ACK*)par_Ack;
    if (Req->VariableNameOffset == 0) {
        Name = NULL;
    } else {
        Name = (char*)Req + Req->VariableNameOffset;
    }
    if (Req->UnitOffset == 0) {
        Unit = NULL;
    }
    else {
        Unit = (char*)Req + Req->UnitOffset;
    }
    if (Req->ConversionOffset == 0) {
        Conversion = NULL;
    }
    else {
        Conversion = (char*)Req + Req->ConversionOffset;
    }
    Ack->Ret = BlackboardIniCache_AddEntry(Name,
                                           Unit,
                                           Conversion,
                                           Req->Min, Req->Max, Req->Step,
                                           Req->Width, Req->Prec, Req->StepType, Req->ConversionType,
                                           Req->RgbColor, Req->Flags);
    Ack->PackageHeader.Command = RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_CMD;
    return sizeof(RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_ACK);
}

static uint32_t Func_GetNextVariableSectionCacheEntry(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    char *VariableName;
    BBVARI_INI_CACHE_ENTRY *Entry;
    int LenVariableName;
    int LenUnit;
    int LenConversion;
    RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_REQ *Req = (RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_REQ*)par_Req;
    RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_ACK *Ack = (RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_ACK*)par_Ack;

    BlackboardIniCache_Lock();
    Ack->Ret = BlackboardIniCache_GetNextEntry(Req->Index, &VariableName, &Entry);

    if (Ack->Ret >= 0) {
        Ack->VariableNameOffset = (uint32_t)sizeof(RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_ACK);
        LenVariableName = strlen(VariableName) + 1;
        MEMCPY((char*)Ack + Ack->VariableNameOffset, VariableName, LenVariableName);
        if (Entry->Unit == NULL) {
            Ack->UnitOffset = 0;
            LenUnit = 0;
        } else {
            Ack->UnitOffset = (uint32_t)sizeof(RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_ACK) + LenVariableName;
            LenUnit = strlen(Entry->Unit) + 1;
            MEMCPY((char*)Ack + Ack->UnitOffset, Entry->Unit, LenUnit);
        }
        if (Entry->Conversion == NULL) {
            Ack->ConversionOffset = 0;
            LenConversion = 0;
        } else {
            Ack->ConversionOffset = (uint32_t)sizeof(RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_ACK) + LenVariableName + LenUnit;
            LenConversion = strlen(Entry->Conversion) + 1;
            MEMCPY((char*)Ack + Ack->ConversionOffset, Entry->Conversion, LenConversion);
        }
        Ack->Min = Entry->Min;
        Ack->Max = Entry->Max;
        Ack->Step = Entry->Step;
        Ack->Width = Entry->Width;
        Ack->Prec = Entry->Prec;
        Ack->StepType = Entry->StepType;
        Ack->ConversionType = Entry->ConversionType;
        Ack->RgbColor = Entry->RgbColor;
    } else {
        LenVariableName = LenUnit = LenConversion = 0;
    }
    BlackboardIniCache_Unlock();
    Ack->PackageHeader.Command = RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_CMD;
    return sizeof(RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_ACK) + LenVariableName + LenUnit + LenConversion;
}

// Read/Write Fuctions

static uint32_t Func_write_bbvari_byte(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_BYTE_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_BYTE_REQ*)par_Req;
    write_bbvari_byte_x((PID)Req->Pid, Req->Vid, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_ubyte(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_UBYTE_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_UBYTE_REQ*)par_Req;
    write_bbvari_ubyte_x((PID)Req->Pid, Req->Vid, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_word(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_WORD_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_WORD_REQ*)par_Req;
    write_bbvari_word_x((PID)Req->Pid, Req->Vid, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_uword(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_UWORD_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_UWORD_REQ*)par_Req;
    write_bbvari_uword_x((PID)Req->Pid, Req->Vid, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_dword(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_DWORD_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_DWORD_REQ*)par_Req;
    write_bbvari_dword_x((PID)Req->Pid, Req->Vid, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_udword(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_UDWORD_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_UDWORD_REQ*)par_Req;
    write_bbvari_udword_x((PID)Req->Pid, Req->Vid, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_qword(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_QWORD_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_QWORD_REQ*)par_Req;
    write_bbvari_qword_x((PID)Req->Pid, Req->Vid, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_uqword(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_UQWORD_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_UQWORD_REQ*)par_Req;
    write_bbvari_uqword_x((PID)Req->Pid, Req->Vid, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_float(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_FLOAT_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_FLOAT_REQ*)par_Req;
    write_bbvari_float_x((PID)Req->Pid, Req->Vid, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_double(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_DOUBLE_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_DOUBLE_REQ*)par_Req;
    write_bbvari_double_x((PID)Req->Pid, Req->Vid, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_union(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_UNION_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_UNION_REQ*)par_Req;
    write_bbvari_union_x((PID)Req->Pid, Req->Vid, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_union_pid(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_UNION_PID_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_UNION_PID_REQ*)par_Req;
    write_bbvari_union_pid((PID)Req->Pid, Req->Vid, Req->DataType, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_minmax_check(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
    RM_BLACKBOARD_WRITE_BBVARI_MIN_MAX_CHECK_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_MIN_MAX_CHECK_REQ*)par_Req;
    write_bbvari_minmax_check_pid((PID)Req->Pid, Req->Vid, Req->Value);
    return 0;
}

static uint32_t Func_write_bbvari_convert_to(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_REQ*)par_Req;
    RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_ACK *Ack = (RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_ACK*)par_Ack;
    Ack->Ret = write_bbvari_convert_to((PID)Req->Pid, Req->Vid, Req->convert_from_type, &(Req->Value));
    return sizeof(RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_ACK);
}

static uint32_t Func_write_bbvari_phys_minmax_check(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_WRITE_BBVARI_PHYS_MINMAX_CHECK_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_PHYS_MINMAX_CHECK_REQ*)par_Req;
    RM_BLACKBOARD_WRITE_BBVARI_PHYS_MINMAX_CHECK_ACK *Ack = (RM_BLACKBOARD_WRITE_BBVARI_PHYS_MINMAX_CHECK_ACK*)par_Ack;
    Ack->Ret = write_bbvari_phys_minmax_check_pid((PID)Req->Pid, Req->Vid, Req->Value);
    return sizeof(RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_ACK);
}

static uint32_t Func_get_phys_value_for_raw_value(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_PHYS_VALUE_FOR_RAW_VALUE_REQ *Req = (RM_BLACKBOARD_GET_PHYS_VALUE_FOR_RAW_VALUE_REQ*)par_Req;
    RM_BLACKBOARD_GET_PHYS_VALUE_FOR_RAW_VALUE_ACK *Ack = (RM_BLACKBOARD_GET_PHYS_VALUE_FOR_RAW_VALUE_ACK*)par_Ack;
    Ack->Ret = get_phys_value_for_raw_value(Req->Vid, Req->raw_value, &(Ack->ret_phys_value));
    return sizeof(RM_BLACKBOARD_GET_PHYS_VALUE_FOR_RAW_VALUE_ACK);
}

static uint32_t Func_get_raw_value_for_phys_value(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_GET_RAW_VALUE_FOR_PHYS_VALUE_REQ *Req = (RM_BLACKBOARD_GET_RAW_VALUE_FOR_PHYS_VALUE_REQ*)par_Req;
    RM_BLACKBOARD_GET_RAW_VALUE_FOR_PHYS_VALUE_ACK *Ack = (RM_BLACKBOARD_GET_RAW_VALUE_FOR_PHYS_VALUE_ACK*)par_Ack;
    Ack->Ret = get_raw_value_for_phys_value(Req->Vid, Req->phys_value, &(Ack->ret_raw_value), &(Ack->ret_phys_value));
    return sizeof(RM_BLACKBOARD_GET_RAW_VALUE_FOR_PHYS_VALUE_ACK);
}

static uint32_t Func_read_bbvari_byte(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_BYTE_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_BYTE_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_BYTE_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_BYTE_ACK*)par_Ack;
    Ack->Ret = read_bbvari_byte(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_BYTE_ACK);
}

static uint32_t Func_read_bbvari_ubyte(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_UBYTE_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_UBYTE_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_UBYTE_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_UBYTE_ACK*)par_Ack;
    Ack->Ret = read_bbvari_ubyte(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_UBYTE_ACK);
}

static uint32_t Func_read_bbvari_word(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_WORD_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_WORD_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_WORD_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_WORD_ACK*)par_Ack;
    Ack->Ret = read_bbvari_word(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_WORD_ACK);
}

static uint32_t Func_read_bbvari_uword(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_UWORD_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_UWORD_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_UWORD_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_UWORD_ACK*)par_Ack;
    Ack->Ret = read_bbvari_uword(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_UWORD_ACK);
}

static uint32_t Func_read_bbvari_dword(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_DWORD_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_DWORD_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_DWORD_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_DWORD_ACK*)par_Ack;
    Ack->Ret = read_bbvari_dword(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_DWORD_ACK);
}

static uint32_t Func_read_bbvari_udword(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_UDWORD_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_UDWORD_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_UDWORD_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_UDWORD_ACK*)par_Ack;
    Ack->Ret = read_bbvari_udword(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_UDWORD_ACK);
}

static uint32_t Func_read_bbvari_qword(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_QWORD_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_QWORD_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_QWORD_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_QWORD_ACK*)par_Ack;
    Ack->Ret = read_bbvari_qword(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_QWORD_ACK);
}

static uint32_t Func_read_bbvari_uqword(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_UQWORD_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_UQWORD_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_UQWORD_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_UQWORD_ACK*)par_Ack;
    Ack->Ret = read_bbvari_uqword(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_UQWORD_ACK);
}

static uint32_t Func_read_bbvari_float(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_FLOAT_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_FLOAT_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_FLOAT_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_FLOAT_ACK*)par_Ack;
    Ack->Ret = read_bbvari_float(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_FLOAT_ACK);
}

static uint32_t Func_read_bbvari_double(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_DOUBLE_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_DOUBLE_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_DOUBLE_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_DOUBLE_ACK*)par_Ack;
    Ack->Ret = read_bbvari_double(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_DOUBLE_ACK);
}

static uint32_t Func_read_bbvari_union(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_UNION_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_UNION_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_UNION_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_UNION_ACK*)par_Ack;
    Ack->Ret = read_bbvari_union(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_UNION_ACK);
}

static uint32_t Func_read_bbvari_union_type(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_ACK*)par_Ack;
    Ack->Ret = read_bbvari_union_type(Req->Vid, &(Ack->ret_Value));
    return sizeof(RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_ACK);
}

static uint32_t Func_read_bbvari_union_type_frame(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_ACK*)par_Ack;
    Ack->OffsetTypes = sizeof(RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_ACK);
    Ack->OffsetValues = Ack->OffsetTypes + Req->Number * sizeof(int32_t);
    Ack->Ret = read_bbvari_union_type_frame(Req->Number, (VID*)((char*)Req + Req->OffsetVids), 
                                            (enum BB_DATA_TYPES*)((char*)Ack + Ack->OffsetTypes),
                                            (union BB_VARI*)((char*)Ack + Ack->OffsetValues));
    return sizeof(RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_ACK) + (size_t)Req->Number * sizeof(int32_t) + (size_t)Req->Number * sizeof(union BB_VARI);
}

static uint32_t Func_read_bbvari_equ(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_EQU_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_EQU_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_EQU_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_EQU_ACK*)par_Ack;
    Ack->Ret = read_bbvari_equ(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_EQU_ACK);
}

static uint32_t Func_read_bbvari_convert_double(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_CONVERT_DOUBLE_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_CONVERT_DOUBLE_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_CONVERT_DOUBLE_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_CONVERT_DOUBLE_ACK*)par_Ack;
    Ack->Ret = read_bbvari_convert_double(Req->Vid);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_CONVERT_DOUBLE_ACK);
}

static uint32_t Func_read_bbvari_convert_to(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_CONVERT_TO_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_CONVERT_TO_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_CONVERT_TO_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_CONVERT_TO_ACK*)par_Ack;
    Ack->Ret = read_bbvari_convert_to(Req->Vid, Req->convert_to_type, (union BB_VARI *)&(Ack->ret_Value));
    return sizeof(RM_BLACKBOARD_READ_BBVARI_CONVERT_TO_ACK);
}

static uint32_t Func_read_bbvari_textreplace(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_ACK*)par_Ack;
    char *EnumString = (char *)(Ack + 1);
    Ack->Ret = read_bbvari_textreplace(Req->Vid, EnumString, Req->maxc, &(Ack->ret_color));
    return sizeof(RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_ACK) + strlen (EnumString) + 1;
}

static uint32_t Func_convert_value_textreplace (RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_CONVERT_TEXTREPLACE_REQ *Req = (RM_BLACKBOARD_CONVERT_TEXTREPLACE_REQ*)par_Req;
    RM_BLACKBOARD_CONVERT_TEXTREPLACE_ACK *Ack = (RM_BLACKBOARD_CONVERT_TEXTREPLACE_ACK*)par_Ack;
    char *EnumString = (char *)(Ack + 1);
    Ack->Ret = convert_value_textreplace(Req->Vid, Req->Value, EnumString, Req->maxc, &(Ack->ret_color));
    return sizeof(RM_BLACKBOARD_CONVERT_TEXTREPLACE_ACK) + strlen(EnumString) + 1;
}

static uint32_t Func_convert_textreplace_value(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_REQ *Req = (RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_REQ*)par_Req;
    RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_ACK *Ack = (RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_ACK*)par_Ack;
    char *Text = (char*)Req + Req->OffsetText;
    Ack->Ret = convert_textreplace_value(Req->Vid, Text, &(Ack->ret_from), &(Ack->ret_to));
    return sizeof(RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_ACK);
}

static uint32_t Func_read_bbvari_frame(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_FRAME_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_FRAME_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_FRAME_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_FRAME_ACK*)par_Ack;
    VID *Vids = (VID*)((char*)Req + Req->OffsetVids);
    double *Values = (double*)(Ack + 1);
    Ack->Ret = (int32_t)read_bbvari_frame (Vids, Values, Req->Size);
    Ack->OffsetValues = sizeof(RM_BLACKBOARD_READ_BBVARI_FRAME_ACK);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_FRAME_ACK) + Req->Size * sizeof(double);
}

static uint32_t Func_write_bbvari_frame(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_WRITE_BBVARI_FRAME_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_FRAME_REQ*)par_Req;
    RM_BLACKBOARD_WRITE_BBVARI_FRAME_ACK *Ack = (RM_BLACKBOARD_WRITE_BBVARI_FRAME_ACK*)par_Ack;
    VID *Vids = (VID*)((char*)Req + Req->OffsetVids);
    double *Values = (double*)((char*)Req + Req->OffsetValues);
    Ack->Ret = (int32_t)write_bbvari_frame_pid(Req->Pid, Vids, Values, Req->Size);
    return sizeof(RM_BLACKBOARD_WRITE_BBVARI_FRAME_ACK);
}

static uint32_t Func_read_bbvari_convert_to_FloatAndInt64(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
	RM_READ_BBVARI_CONVERT_TO_FLOATANDINT64_REQ *Req = (RM_READ_BBVARI_CONVERT_TO_FLOATANDINT64_REQ*)par_Req;
	RM_READ_BBVARI_CONVERT_TO_FLOATANDINT64_ACK *Ack = (RM_READ_BBVARI_CONVERT_TO_FLOATANDINT64_ACK*)par_Ack;
	Ack->Ret = (int32_t)read_bbvari_convert_to_FloatAndInt64(Req->vid, &(Ack->RetValue));
	return sizeof(RM_READ_BBVARI_CONVERT_TO_FLOATANDINT64_ACK);
}

static uint32_t Func_write_bbvari_convert_with_FloatAndInt64(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Ack);
	RM_WRITE_BBVARI_CONVERT_WITH_FLOATANDINT64_REQ *Req = (RM_WRITE_BBVARI_CONVERT_WITH_FLOATANDINT64_REQ*)par_Req;
	write_bbvari_convert_with_FloatAndInt64_pid(Req->pid, Req->vid, Req->value, Req->type);
	return sizeof(RM_WRITE_BBVARI_CONVERT_WITH_FLOATANDINT64_ACK);
}

static uint32_t Func_read_bbvari_by_name(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_READ_BBVARI_BY_NAME_REQ *Req = (RM_BLACKBOARD_READ_BBVARI_BY_NAME_REQ*)par_Req;
    RM_BLACKBOARD_READ_BBVARI_BY_NAME_ACK *Ack = (RM_BLACKBOARD_READ_BBVARI_BY_NAME_ACK*)par_Ack;
    Ack->Ret = read_bbvari_by_name((char*)(Req) + Req->OffsetText, &(Ack->value), &(Ack->ret_byte_width), Req->read_type);
    return sizeof(RM_BLACKBOARD_READ_BBVARI_BY_NAME_ACK);
}

static uint32_t Func_write_bbvari_by_name(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_REQ *Req = (RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_REQ*)par_Req;
    RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_ACK *Ack = (RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_ACK*)par_Ack;
    Ack->Ret = write_bbvari_by_name(Req->Pid, (char*)(Req)+Req->OffsetText, Req->value, Req->type, Req->bb_type,  Req->add_if_not_exist, Req->read_type, 1);
    return sizeof(RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_ACK);
}

// Messages

static uint32_t Func_write_message_ts_as(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_MESSAGE_WRITE_MESSAGE_TS_AS_REQ *Req = (RM_MESSAGE_WRITE_MESSAGE_TS_AS_REQ*)par_Req;
    RM_MESSAGE_WRITE_MESSAGE_TS_AS_ACK *Ack = (RM_MESSAGE_WRITE_MESSAGE_TS_AS_ACK*)par_Ack;

    Ack->Ret = write_message_ts_as(Req->pid, Req->mid, Req->timestamp, Req->msize, (char*)Req + Req->Offset_mblock, Req->tx_pid);
    return sizeof(RM_MESSAGE_WRITE_MESSAGE_TS_AS_ACK);
}

static uint32_t Func_write_important_message_ts_as(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_REQ *Req = (RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_REQ*)par_Req;
    RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_ACK *Ack = (RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_ACK*)par_Ack;

    Ack->Ret = write_important_message_ts_as(Req->pid, Req->mid, Req->timestamp, Req->msize, (char*)Req + Req->Offset_mblock, Req->tx_pid);
    return sizeof(RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_ACK);
}

static uint32_t Func_test_message_as(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_MESSAGE_TEST_MESSAGE_AS_REQ *Req = (RM_MESSAGE_TEST_MESSAGE_AS_REQ*)par_Req;
    RM_MESSAGE_TEST_MESSAGE_AS_ACK *Ack = (RM_MESSAGE_TEST_MESSAGE_AS_ACK*)par_Ack;

    Ack->Ret = test_message_as(&(Ack->ret_mhead), Req->pid);
    return sizeof(RM_MESSAGE_TEST_MESSAGE_AS_ACK);
}

static uint32_t Func_read_message_as(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_MESSAGE_READ_MESSAGE_AS_REQ *Req = (RM_MESSAGE_READ_MESSAGE_AS_REQ*)par_Req;
    RM_MESSAGE_READ_MESSAGE_AS_ACK *Ack = (RM_MESSAGE_READ_MESSAGE_AS_ACK*)par_Ack;

    Ack->Ret = read_message_as(&(Ack->ret_mhead), (char*)(Ack + 1), Req->maxlen, Req->pid);
    if (Ack->Ret >= 0) return sizeof(RM_MESSAGE_READ_MESSAGE_AS_ACK) + Ack->ret_mhead.size;
    else return sizeof(RM_MESSAGE_READ_MESSAGE_AS_ACK);
}

static uint32_t Func_remove_message_as(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_MESSAGE_REMOVE_MESSAGE_AS_REQ *Req = (RM_MESSAGE_REMOVE_MESSAGE_AS_REQ*)par_Req;
    RM_MESSAGE_REMOVE_MESSAGE_AS_ACK *Ack = (RM_MESSAGE_REMOVE_MESSAGE_AS_ACK*)par_Ack;

    Ack->Ret = remove_message_as(Req->pid);
    return sizeof(RM_MESSAGE_REMOVE_MESSAGE_AS_ACK);
}


// Scheduler

static uint32_t Func_AddRealtimeScheduler(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_ADD_REALTIME_SCHEDULER_REQ *Req = (RM_SCHEDULER_ADD_REALTIME_SCHEDULER_REQ*)par_Req;
    RM_SCHEDULER_ADD_REALTIME_SCHEDULER_ACK *Ack = (RM_SCHEDULER_ADD_REALTIME_SCHEDULER_ACK*)par_Ack;

    Ack->Ret = AddRealtimeScheduler((char*)Req + Req->Offset_Name, Req->CyclePeriod, Req->SyncWithFlexray, &(Ack->CyclePeriodInNanoSecond));
    return sizeof(RM_SCHEDULER_ADD_REALTIME_SCHEDULER_ACK);
}

static uint32_t Func_StopRealtimeScheduler(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_STOP_REALTIME_SCHEDULER_REQ *Req = (RM_SCHEDULER_STOP_REALTIME_SCHEDULER_REQ*)par_Req;
    RM_SCHEDULER_STOP_REALTIME_SCHEDULER_ACK *Ack = (RM_SCHEDULER_STOP_REALTIME_SCHEDULER_ACK*)par_Ack;

    Ack->Ret = StopRealtimeScheduler((char*)Req + Req->Offset_Name);
    return sizeof(RM_SCHEDULER_STOP_REALTIME_SCHEDULER_ACK);
}

static uint32_t Func_StartRealtimeProcess(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_START_REALTIME_PROCESS_REQ *Req = (RM_SCHEDULER_START_REALTIME_PROCESS_REQ*)par_Req;
    RM_SCHEDULER_START_REALTIME_PROCESS_ACK *Ack = (RM_SCHEDULER_START_REALTIME_PROCESS_ACK*)par_Ack;

    Ack->Ret = StartRealtimeProcess(Req->Pid, (char*)Req + Req->Offset_Name, (char*)Req + Req->Offset_Scheduler, Req->Prio, Req->Cycles, Req->Delay);
    return sizeof(RM_SCHEDULER_START_REALTIME_PROCESS_ACK);
}

static uint32_t Func_StopRealtimeProcess(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_STOP_REALTIME_PROCESS_REQ *Req = (RM_SCHEDULER_STOP_REALTIME_PROCESS_REQ*)par_Req;
    RM_SCHEDULER_STOP_REALTIME_PROCESS_ACK *Ack = (RM_SCHEDULER_STOP_REALTIME_PROCESS_ACK*)par_Ack;

    Ack->Ret = StopRealtimeProcess(Req->Pid);
    return sizeof(RM_SCHEDULER_STOP_REALTIME_PROCESS_ACK);
}

static uint32_t Func_GetNextRealtimeProcess(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_REQ *Req = (RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_REQ*)par_Req;
    RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_ACK *Ack = (RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_ACK*)par_Ack;
    int Len;

    Ack->Ret = GetNextRealtimeProcess(Req->Index, Req->Flags, (char*)(Ack + 1), Req->maxc);
    if (Ack->Ret > 0) {
        Len = strlen((char*)(Ack + 1)) + 1;
        Ack->Offset_Name = sizeof(RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_ACK);
    } else {
        Len = 0;
        Ack->Offset_Name = 0;
    }
    return sizeof(RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_ACK) + Len;
}

static uint32_t Func_IsRealtimeProcess(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_IS_REALTIME_PROCESS_REQ *Req = (RM_SCHEDULER_IS_REALTIME_PROCESS_REQ*)par_Req;
    RM_SCHEDULER_IS_REALTIME_PROCESS_ACK *Ack = (RM_SCHEDULER_IS_REALTIME_PROCESS_ACK*)par_Ack;

    Ack->Ret = IsRealtimeProcess((char*)Req + Req->Offset_Name);
    return sizeof(RM_SCHEDULER_IS_REALTIME_PROCESS_ACK);
}

static uint32_t Func_AddProcessToPidNameArray(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_REQ *Req = (RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_REQ*)par_Req;
    RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_ACK *Ack = (RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_ACK*)par_Ack;

    Ack->Ret = AddProcessToPidNameArray(Req->Pid, (char*)Req + Req->Offset_Name, Req->MessageQueueSize);
    return sizeof(RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_ACK);
}

static uint32_t Func_RemoveProcessFromPidNameArray(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_REMOVE_PROCESS_FROM_PID_NAME_ARRAY_REQ *Req = (RM_SCHEDULER_REMOVE_PROCESS_FROM_PID_NAME_ARRAY_REQ*)par_Req;
    RM_SCHEDULER_REMOVE_PROCESS_FROM_PID_NAME_ARRAY_ACK *Ack = (RM_SCHEDULER_REMOVE_PROCESS_FROM_PID_NAME_ARRAY_ACK*)par_Ack;

    Ack->Ret = RemoveProcessFromPidNameArray(Req->Pid);
    return sizeof(RM_SCHEDULER_REMOVE_PROCESS_FROM_PID_NAME_ARRAY_ACK);
}

static uint32_t Func_get_pid_by_name(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_GET_PID_BY_NAME_REQ *Req = (RM_SCHEDULER_GET_PID_BY_NAME_REQ*)par_Req;
    RM_SCHEDULER_GET_PID_BY_NAME_ACK *Ack = (RM_SCHEDULER_GET_PID_BY_NAME_ACK*)par_Ack;

    Ack->Ret = get_pid_by_name((char*)Req + Req->Offset_Name);
    return sizeof(RM_SCHEDULER_GET_PID_BY_NAME_ACK);
}

static uint32_t Func_get_timestamp_counter(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    UNUSED(par_Req);
    RM_SCHEDULER_GET_TIMESTAMPCOUNTER_ACK *Ack = (RM_SCHEDULER_GET_TIMESTAMPCOUNTER_ACK*)par_Ack;

    Ack->Ret = get_timestamp_counter();
    return sizeof(RM_SCHEDULER_GET_TIMESTAMPCOUNTER_ACK);
}

static uint32_t Func_get_process_info_ex(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_GET_PROCESS_INFO_EX_REQ *Req = (RM_SCHEDULER_GET_PROCESS_INFO_EX_REQ*)par_Req;
    RM_SCHEDULER_GET_PROCESS_INFO_EX_ACK *Ack = (RM_SCHEDULER_GET_PROCESS_INFO_EX_ACK*)par_Ack;
    int Len;
    char *ProcessName;
    char *AssignedSchedule;

    if (Req->maxc_Name != 0) {
        ProcessName = (char*)(Ack + 1);
    } else {
        ProcessName = NULL;
    }
    if (Req->maxc_AssignedScheduler != 0) {
        AssignedSchedule = (char*)(Ack + 1) + Req->maxc_Name;
    }
    else {
        AssignedSchedule = NULL;
    }
    Ack->Ret = get_process_info_ex(Req->pid, (char*)(Ack + 1), Req->maxc_Name, &(Ack->ret_Type), &(Ack->ret_Prio), &(Ack->ret_Cycles), &(Ack->ret_Delay), &(Ack->ret_MessageSize),
                                   AssignedSchedule, Req->maxc_AssignedScheduler, &(Ack->ret_bb_access_mask),
                                   &(Ack->ret_State), &(Ack->ret_CyclesCounter), &(Ack->ret_CyclesTime), &(Ack->ret_CyclesTimeMax), &(Ack->ret_CyclesTimeMin));
    if (Ack->Ret == 0) {
        Ack->Offset_Name = sizeof(RM_SCHEDULER_GET_PROCESS_INFO_EX_ACK);
        Ack->Offset_AssignedScheduler = Ack->Offset_Name + Req->maxc_Name;
        Len = Req->maxc_Name + Req->maxc_AssignedScheduler;
    } else {
        Ack->Offset_Name = 0;
        Ack->Offset_AssignedScheduler = 0;
        Len = 0;
    }
    return sizeof(RM_SCHEDULER_GET_PROCESS_INFO_EX_ACK) + Len;
}

static uint32_t Func_GetOrFreeUniquePid(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_REQ *Req = (RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_REQ*)par_Req;
    RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_ACK *Ack = (RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_ACK*)par_Ack;

    Ack->Ret = GetOrFreeUniquePid(Req->Command, Req->Pid, (char*)Req + Req->Offset_Name);
    return sizeof(RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_ACK);
}


static uint32_t Func_AddBeforeProcessEquation(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_REQ *Req = (RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_REQ*)par_Req;
    RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_ACK *Ack = (RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_ACK*)par_Ack;

    Ack->Ret = AddBeforeProcessEquation(Req->Nr, Req->Pid, (char*)Req + Req->Offset_ExecStack);
    return sizeof(RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_ACK);
}

static uint32_t Func_AddBehindProcessEquation(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_REQ *Req = (RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_REQ*)par_Req;
    RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_ACK *Ack = (RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_ACK*)par_Ack;

    Ack->Ret = AddBehindProcessEquation(Req->Nr, Req->Pid, (char*)Req + Req->Offset_ExecStack);
    return sizeof(RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_ACK);
}

static uint32_t Func_DelBeforeProcessEquations(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_DEL_BEFORE_PROCESS_EQUATIONS_REQ *Req = (RM_SCHEDULER_DEL_BEFORE_PROCESS_EQUATIONS_REQ*)par_Req;
    RM_SCHEDULER_DEL_BEFORE_PROCESS_EQUATIONS_ACK *Ack = (RM_SCHEDULER_DEL_BEFORE_PROCESS_EQUATIONS_ACK*)par_Ack;

    Ack->Ret = DelBeforeProcessEquations(Req->Nr, Req->Pid);
    return sizeof(RM_SCHEDULER_DEL_BEFORE_PROCESS_EQUATIONS_ACK);
}

static uint32_t Func_DelBehindProcessEquations(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_DEL_BEHIND_PROCESS_EQUATIONS_REQ *Req = (RM_SCHEDULER_DEL_BEHIND_PROCESS_EQUATIONS_REQ*)par_Req;
    RM_SCHEDULER_DEL_BEHIND_PROCESS_EQUATIONS_ACK *Ack = (RM_SCHEDULER_DEL_BEHIND_PROCESS_EQUATIONS_ACK*)par_Ack;

    Ack->Ret = DelBehindProcessEquations(Req->Nr, Req->Pid);
    return sizeof(RM_SCHEDULER_DEL_BEHIND_PROCESS_EQUATIONS_ACK);
}

static uint32_t Func_IsRealtimeProcessPid(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_SCHEDULER_IS_REALTIME_PROCESS_PID_REQ *Req = (RM_SCHEDULER_IS_REALTIME_PROCESS_PID_REQ*)par_Req;
    RM_SCHEDULER_IS_REALTIME_PROCESS_PID_ACK *Ack = (RM_SCHEDULER_IS_REALTIME_PROCESS_PID_ACK*)par_Ack;

    Ack->Ret = IsRealtimeProcessPid(Req->Pid);
    return sizeof(RM_SCHEDULER_IS_REALTIME_PROCESS_PID_ACK);
}

static uint32_t Func_ResetProcessMinMaxRuntimeMeasurement(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_RESET_PROCESS_MIN_MAX_RUNTIME_MEASUREMENT_REQ *Req = (RM_RESET_PROCESS_MIN_MAX_RUNTIME_MEASUREMENT_REQ*)par_Req;
    RM_RESET_PROCESS_MIN_MAX_RUNTIME_MEASUREMENT_ACK *Ack = (RM_RESET_PROCESS_MIN_MAX_RUNTIME_MEASUREMENT_ACK*)par_Ack;

    Ack->Ret = ResetProcessMinMaxRuntimeMeasurement(Req->Pid);
    return sizeof(RM_RESET_PROCESS_MIN_MAX_RUNTIME_MEASUREMENT_ACK);
}

// CAN Fifos

static uint32_t Func_CreateCanFifos(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_CAN_FIFO_CREATE_CAN_FIFOS_REQ *Req = (RM_CAN_FIFO_CREATE_CAN_FIFOS_REQ*)par_Req;
    RM_CAN_FIFO_CREATE_CAN_FIFOS_ACK *Ack = (RM_CAN_FIFO_CREATE_CAN_FIFOS_ACK*)par_Ack;

    Ack->Ret = CreateCanFifos(Req->Depth, Req->FdFlag);
    return sizeof(RM_CAN_FIFO_CREATE_CAN_FIFOS_ACK);
}

static uint32_t Func_DeleteCanFifos(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_CAN_FIFO_DELETE_CAN_FIFOS_REQ *Req = (RM_CAN_FIFO_DELETE_CAN_FIFOS_REQ*)par_Req;
    RM_CAN_FIFO_DELETE_CAN_FIFOS_ACK *Ack = (RM_CAN_FIFO_DELETE_CAN_FIFOS_ACK*)par_Ack;

    Ack->Ret = DeleteCanFifos(Req->Handle);
    return sizeof(RM_CAN_FIFO_DELETE_CAN_FIFOS_ACK);
}

static uint32_t Func_FlushCanFifo(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_CAN_FIFO_FLUSH_CAN_FIFO_REQ *Req = (RM_CAN_FIFO_FLUSH_CAN_FIFO_REQ*)par_Req;
    RM_CAN_FIFO_FLUSH_CAN_FIFO_ACK *Ack = (RM_CAN_FIFO_FLUSH_CAN_FIFO_ACK*)par_Ack;

    Ack->Ret = FlushCanFifo(Req->Handle, Req->Flags);
    return sizeof(RM_CAN_FIFO_FLUSH_CAN_FIFO_ACK);
}

static uint32_t Func_ReadCanMessageFromFifo2Process(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_CAN_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_REQ *Req = (RM_CAN_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_REQ*)par_Req;
    RM_CAN_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_ACK *Ack = (RM_CAN_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_ACK*)par_Ack;

    Ack->Ret = ReadCanMessageFromFifo2Process(Req->Handle, &(Ack->ret_CanMessage));
    return sizeof(RM_CAN_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_ACK);
}

static uint32_t Func_ReadCanMessagesFromFifo2Process(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    int Len;
    RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_REQ *Req = (RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_REQ*)par_Req;
    RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK *Ack = (RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK*)par_Ack;

    Ack->Ret = ReadCanMessagesFromFifo2Process(Req->Handle, (CAN_FIFO_ELEM*)(Ack + 1), Req->MaxMessages);
    if (Ack->Ret > 0) {
        Len = sizeof(CAN_FIFO_ELEM) * Ack->Ret;
        Ack->Offset_CanMessage = sizeof(RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK);
    }
    else {
        Len = 0;
    }
    return sizeof(RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK) + Len;
}

static uint32_t Func_WriteCanMessagesFromProcess2Fifo(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ *Req = (RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ*)par_Req;
    RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_ACK *Ack = (RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_ACK*)par_Ack;

    Ack->Ret = WriteCanMessagesFromProcess2Fifo(Req->Handle, (CAN_FIFO_ELEM*)(Req + 1), Req->MaxMessages);
    return sizeof(RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_ACK);
}

static uint32_t Func_SetAcceptMask4CanFifo(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_REQ *Req = (RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_REQ*)par_Req;
    RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_ACK *Ack = (RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_ACK*)par_Ack;

    Ack->Ret = SetAcceptMask4CanFifo(Req->Handle, (CAN_ACCEPT_MASK*)(Req + 1), Req->Size);
    return sizeof(RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_ACK);
}

static uint32_t Func_WriteCanMessageFromProcess2Fifo(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_CAN_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_REQ *Req = (RM_CAN_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_REQ*)par_Req;
    RM_CAN_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_ACK *Ack = (RM_CAN_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_ACK*)par_Ack;

    Ack->Ret = WriteCanMessageFromProcess2Fifo(Req->Handle, &(Req->CanMessage));
    return sizeof(RM_CAN_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_ACK);
}

// CAN FD

static uint32_t Func_ReadCanFdMessageFromFifo2Process(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_CAN_FD_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_REQ *Req = (RM_CAN_FD_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_REQ*)par_Req;
    RM_CAN_FD_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_ACK *Ack = (RM_CAN_FD_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_ACK*)par_Ack;

    Ack->Ret = ReadCanFdMessageFromFifo2Process(Req->Handle, &(Ack->ret_CanMessage));
    return sizeof(RM_CAN_FD_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_ACK);
}

static uint32_t Func_ReadCanFdMessagesFromFifo2Process(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    int Len;
    RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_REQ *Req = (RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_REQ*)par_Req;
    RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK *Ack = (RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK*)par_Ack;

    Ack->Ret = ReadCanFdMessagesFromFifo2Process(Req->Handle, (CAN_FD_FIFO_ELEM*)(Ack + 1), Req->MaxMessages);
    if (Ack->Ret > 0) {
        Len = sizeof(CAN_FD_FIFO_ELEM) * Ack->Ret;
        Ack->Offset_CanMessage = sizeof(RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK);
    }
    else {
        Len = 0;
    }
    return sizeof(RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_ACK) + Len;
}

static uint32_t Func_WriteCanFdMessagesFromProcess2Fifo(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ *Req = (RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_REQ*)par_Req;
    RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_ACK *Ack = (RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_ACK*)par_Ack;

    Ack->Ret = WriteCanFdMessagesFromProcess2Fifo(Req->Handle, (CAN_FD_FIFO_ELEM*)(Req + 1), Req->MaxMessages);
    return sizeof(RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_ACK);
}

static uint32_t Func_WriteCanFdMessageFromProcess2Fifo(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
    RM_CAN_FD_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_REQ *Req = (RM_CAN_FD_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_REQ*)par_Req;
    RM_CAN_FD_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_ACK *Ack = (RM_CAN_FD_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_ACK*)par_Ack;

    Ack->Ret = WriteCanFdMessageFromProcess2Fifo(Req->Handle, &(Req->CanMessage));
    return sizeof(RM_CAN_FD_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_ACK);
}

static uint32_t Func_RegisterNewRxFiFoName(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
	RM_REGISTER_NEW_RX_FIFO_NAME_REQ *Req = (RM_REGISTER_NEW_RX_FIFO_NAME_REQ*)par_Req;
	RM_REGISTER_NEW_RX_FIFO_NAME_ACK *Ack = (RM_REGISTER_NEW_RX_FIFO_NAME_ACK*)par_Ack;

	Ack->Ret = RegisterNewRxFiFoName(Req->RxPid, (char*)(Req + 1));
	return sizeof(RM_REGISTER_NEW_RX_FIFO_NAME_ACK);
}

static uint32_t Func_UnRegisterRxFiFo(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
	RM_UNREGISTER_RX_FIFO_REQ *Req = (RM_UNREGISTER_RX_FIFO_REQ*)par_Req;
	RM_UNREGISTER_RX_FIFO_ACK *Ack = (RM_UNREGISTER_RX_FIFO_ACK*)par_Ack;

	Ack->Ret = UnRegisterRxFiFo(Req->FiFoId, Req->RxPid);
	return sizeof(RM_UNREGISTER_RX_FIFO_ACK);
}

static uint32_t Func_RxAttachFiFo(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
	RM_RX_ATTACH_FIFO_REQ *Req = (RM_RX_ATTACH_FIFO_REQ*)par_Req;
	RM_RX_ATTACH_FIFO_ACK *Ack = (RM_RX_ATTACH_FIFO_ACK*)par_Ack;

	Ack->Ret = TxAttachFiFo(Req->RxPid, (char*)(Req + 1));
	return sizeof(RM_RX_ATTACH_FIFO_ACK);
	return sizeof(RM_RX_ATTACH_FIFO_ACK);
}

static uint32_t Func_SyncFiFos(RM_PACKAGE_HEADER *par_Req, RM_PACKAGE_HEADER *par_Ack)
{
	RM_SYNC_FIFOS_REQ *Req = (RM_SYNC_FIFOS_REQ*)par_Req;
	RM_SYNC_FIFOS_ACK *Ack = (RM_SYNC_FIFOS_ACK*)par_Ack;

	SyncRxFiFos(Req + 1, Req->Len);
	Ack->Ret = SyncTxFiFos(Ack + 1, BUFFER_SIZE - sizeof(RM_SYNC_FIFOS_ACK), &(Ack->Len));
	Ack->Offset_Buffer = sizeof(RM_SYNC_FIFOS_ACK);
	return sizeof(RM_SYNC_FIFOS_ACK) + Ack->Len;
}


typedef uint32_t (*FUNC_PTR)(RM_PACKAGE_HEADER *Req, RM_PACKAGE_HEADER *Ack);

struct ENTRY_FUNC_PTR {
    FUNC_PTR FuncPtr;
    int32_t FuncNr;
    uint32_t CallCounter;
};

struct ENTRY_FUNC_PTR FuntionPointerTable[] = {
    /* 000 */ { Func_KillThread, RM_KILL_THREAD_CMD, 0 },
    /* 001 */ { Func_Init, RM_INIT_CMD, 0 },
    /* 002 */ { Func_Terminate, RM_TERMINATE_CMD, 0 },
    /* 003 */ { Func_Kill, RM_KILL_CMD, 0 },
    /* 004 */ { Func_Ping, RM_PING_CMD, 0 },
    /* 005 */ { Func_StartCopyFile, RM_COPY_FILE_START_CMD, 0 },
    /* 006 */ { Func_NextCopyFile, RM_COPY_FILE_NEXT_CMD, 0 },
    /* 007 */ { Func_EndCopyFile, RM_COPY_FILE_END_CMD, 0 },
    /* 008 */ { Func_ReadBytes, RM_READ_DATA_BYTES_CMD, 0 },
    /* 009 */ { Func_WriteBytes, RM_WRITE_DATA_BYTES_CMD, 0 },
    /* 010 */ { Func_RefrenceVariable, RM_REFERENCE_VARIABLE_CMD, 0 },
    /* 011 */ { Func_DeRefrenceVariable, RM_DEREFERENCE_VARIABLE_CMD, 0 },
    /* 012 */ { Func_undefined, 0, 0 },
    /* 013 */ { Func_undefined, 0, 0 },
    /* 014 */ { Func_undefined, 0, 0 },
    /* 015 */ { Func_undefined, 0, 0 },

    /* 016 */ { Func_init_blackboard, RM_BLACKBOARD_INIT_BLACKBOARD_CMD, 0 },
    /* 017 */ { Func_close_blackboard, RM_BLACKBOARD_CLOSE_BLACKBOARD_CMD, 0 },
    /* 018 */{ Func_add_bbvari_pid_type, RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_CMD, 0 },
    /* 019 */{ Func_attach_bbvari, RM_BLACKBOARD_ATTACH_BBVARI_CMD, 0 },
    /* 020 */{ Func_attach_bbvari_by_name, RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_CMD, 0 },
    /* 021 */{ Func_remove_bbvari, RM_BLACKBOARD_REMOVE_BBVARI_CMD, 0 },
    /* 022 */{ Func_remove_all_bbvari, RM_BLACKBOARD_REMOVE_ALL_BBVARI_CMD, 0 },
    /* 023 */{ Func_get_num_of_bbvaris, RM_BLACKBOARD_GET_NUM_OF_BBVARIS_CMD, 0 },
    /* 024 */{ Func_get_bbvarivid_by_name, RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_CMD, 0 },
    /* 025 */{ Func_get_bbvaritype_by_name, RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_CMD, 0 },
    /* 026 */{ Func_get_bbvari_attachcount, RM_BLACKBOARD_GET_BBVARI_ATTACHCOUNT_CMD, 0 },
    /* 027 */{ Func_get_bbvaritype, RM_BLACKBOARD_GET_BBVARITYPE_CMD, 0 },
    /* 028 */{ Func_GetBlackboardVariableName, RM_BLACKBOARD_GET_BBVARI_NAME_CMD, 0 },
    /* 029 */{ Func_GetBlackboardVariableNameAndTypes, RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_CMD, 0 },
    /* 030 */{ Func_get_process_bbvari_attach_count, RM_BLACKBOARD_GET_PROCESS_BBVARI_ATTACH_COUNT_CMD, 0 },
    /* 031 */{ Func_set_bbvari_unit, RM_BLACKBOARD_SET_BBVARI_UNIT_CMD, 0 },
    /* 032 */{ Func_get_bbvari_unit, RM_BLACKBOARD_GET_BBVARI_UNIT_CMD, 0 },
    /* 033 */{ Func_set_bbvari_conversion, RM_BLACKBOARD_SET_BBVARI_CONVERSION_CMD, 0 },
    /* 034 */{ Func_get_bbvari_conversion, RM_BLACKBOARD_GET_BBVARI_CONVERSION_CMD, 0 },
    /* 035 */{ Func_get_bbvari_conversiontype, RM_BLACKBOARD_GET_BBVARI_CONVERSIONTYPE_CMD, 0 },
    /* 036 */{ Func_get_bbvari_infos, RM_BLACKBOARD_GET_BBVARI_INFOS_CMD, 0 },
    /* 037 */{ Func_set_bbvari_color, RM_BLACKBOARD_SET_BBVARI_COLOR_CMD, 0 },
    /* 038 */{ Func_get_bbvari_color, RM_BLACKBOARD_GET_BBVARI_COLOR_CMD, 0 },
    /* 039 */{ Func_set_bbvari_step, RM_BLACKBOARD_SET_BBVARI_STEP_CMD, 0 },
    /* 040 */{ Func_get_bbvari_step, RM_BLACKBOARD_GET_BBVARI_STEP_CMD, 0 },
    /* 041 */{ Func_set_bbvari_min, RM_BLACKBOARD_SET_BBVARI_MIN_CMD, 0 },
    /* 042 */{ Func_set_bbvari_max, RM_BLACKBOARD_SET_BBVARI_MAX_CMD, 0 },
    /* 043 */{ Func_get_bbvari_min, RM_BLACKBOARD_GET_BBVARI_MIN_CMD, 0 },
    /* 044 */{ Func_get_bbvari_max, RM_BLACKBOARD_GET_BBVARI_MAX_CMD, 0 },
    /* 045 */{ Func_set_bbvari_format, RM_BLACKBOARD_SET_BBVARI_FORMAT_CMD, 0 },
    /* 046 */{ Func_get_bbvari_format_width, RM_BLACKBOARD_GET_BBVARI_FORMAT_WIDTH_CMD, 0 },
    /* 047 */{ Func_get_bbvari_format_prec, RM_BLACKBOARD_GET_BBVARI_FORMAT_PREC_CMD, 0 },
    /* 048 */{ Func_get_free_wrflag, RM_BLACKBOARD_GET_FREE_WRFLAG_CMD, 0 },
    /* 049 */{ Func_reset_wrflag, RM_BLACKBOARD_RESET_WRFLAG_CMD, 0 },
    /* 050 */{ Func_test_wrflag, RM_BLACKBOARD_TEST_WRFLAG_CMD, 0 },
    /* 051 */{ Func_read_next_blackboard_vari, RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_CMD, 0 },
    /* 052 */{ Func_ReadNextVariableProcessAccess, RM_BLACKBOARD_READ_NEXT_VARI_PROCESS_ACCESS_CMD, 0 },
    /* 053 */{ Func_enable_bbvari_access, RM_BLACKBOARD_ENABLE_BBVARI_ACCESS_CMD, 0 },
    /* 054 */{ Func_disable_bbvari_access, RM_BLACKBOARD_DISABLE_BBVARI_ACCESS_CMD, 0 },
    /* 055 */{ Func_SetObserationCallbackFunction, RM_BLACKBOARD_SET_OBSERVATION_CALLBACK_FUNCTION_CMD, 0 },
    /* 056 */{ Func_SetBbvariObservation, RM_BLACKBOARD_SET_BBVARI_OBSERVATION_CMD, 0 },
    /* 057 */{ Func_SetGlobalBlackboradObservation, RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_CMD, 0 },
    /* 058 */{ Func_get_bb_accessmask, RM_BLACKBOARD_GET_BB_ACCESSMASK_CMD, 0 },
    /* 059 */{ Func_AddVariableSectionCacheEntry, RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_CMD, 0 },
    /* 060 */{ Func_GetNextVariableSectionCacheEntry, RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_CMD, 0 },
    /* 061 */{ Func_undefined, 0, 0 },
    /* 062 */{ Func_undefined, 0, 0 },
    /* 063 */{ Func_undefined, 0, 0 },
    /* 064 */{ Func_undefined, 0, 0 },
    /* 065 */{ Func_undefined, 0, 0 },
    /* 066 */{ Func_undefined, 0, 0 },
    /* 067 */{ Func_undefined, 0, 0 },
    /* 068 */{ Func_undefined, 0, 0 },
    /* 069 */{ Func_undefined, 0, 0 },
    /* 070 */{ Func_undefined, 0, 0 },
    /* 071 */{ Func_undefined, 0, 0 },
    /* 072 */{ Func_undefined, 0, 0 },
    /* 073 */{ Func_undefined, 0, 0 },
    /* 074 */{ Func_undefined, 0, 0 },
    /* 075 */{ Func_undefined, 0, 0 },
    /* 076 */{ Func_undefined, 0, 0 },
    /* 077 */{ Func_undefined, 0, 0 },
    /* 078 */{ Func_undefined, 0, 0 },
    /* 079 */{ Func_undefined, 0, 0 },
    /* 080 */{ Func_undefined, 0, 0 },
    /* 081 */{ Func_undefined, 0, 0 },
    /* 082 */{ Func_undefined, 0, 0 },
    /* 083 */{ Func_undefined, 0, 0 },
    /* 084 */{ Func_undefined, 0, 0 },
    /* 085 */{ Func_undefined, 0, 0 },
    /* 086 */{ Func_undefined, 0, 0 },
    /* 087 */{ Func_undefined, 0, 0 },
    /* 088 */{ Func_undefined, 0, 0 },
    /* 089 */{ Func_undefined, 0, 0 },
    /* 090 */{ Func_undefined, 0, 0 },
    /* 091 */{ Func_undefined, 0, 0 },
    /* 092 */{ Func_undefined, 0, 0 },
    /* 093 */{ Func_undefined, 0, 0 },
    /* 094 */{ Func_undefined, 0, 0 },
    /* 095 */{ Func_undefined, 0, 0 },
    /* 096 */{ Func_undefined, 0, 0 },
    /* 097 */{ Func_undefined, 0, 0 },
    /* 098 */{ Func_undefined, 0, 0 },
    /* 099 */{ Func_undefined, 0, 0 },
    /* 100 */{ Func_write_bbvari_byte, RM_BLACKBOARD_WRITE_BBVARI_BYTE_CMD, 0 },
    /* 101 */{ Func_write_bbvari_ubyte, RM_BLACKBOARD_WRITE_BBVARI_UBYTE_CMD, 0 },
    /* 102 */{ Func_write_bbvari_word, RM_BLACKBOARD_WRITE_BBVARI_WORD_CMD, 0 },
    /* 103 */{ Func_write_bbvari_uword, RM_BLACKBOARD_WRITE_BBVARI_UWORD_CMD, 0 },
    /* 104 */{ Func_write_bbvari_dword, RM_BLACKBOARD_WRITE_BBVARI_DWORD_CMD, 0 },
    /* 105 */{ Func_write_bbvari_udword, RM_BLACKBOARD_WRITE_BBVARI_UDWORD_CMD, 0 },

    /* 106 */{ Func_write_bbvari_qword, RM_BLACKBOARD_WRITE_BBVARI_QWORD_CMD, 0 },
    /* 107 */{ Func_write_bbvari_uqword, RM_BLACKBOARD_WRITE_BBVARI_UQWORD_CMD, 0 },

    /* 108 */{ Func_write_bbvari_float, RM_BLACKBOARD_WRITE_BBVARI_FLOAT_CMD, 0 },
    /* 109 */{ Func_write_bbvari_double, RM_BLACKBOARD_WRITE_BBVARI_DOUBLE_CMD, 0 },
    /* 110 */{ Func_write_bbvari_union, RM_BLACKBOARD_WRITE_BBVARI_UNION_CMD, 0 },
    /* 111 */{ Func_write_bbvari_union_pid, RM_BLACKBOARD_WRITE_BBVARI_UNION_PID_CMD, 0 },
    /* 112 */{ Func_write_bbvari_minmax_check, RM_BLACKBOARD_WRITE_BBVARI_MIN_MAX_CHECK_CMD, 0 },
    /* 113 */{ Func_write_bbvari_convert_to, RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_CMD, 0 },
    /* 114 */{ Func_write_bbvari_phys_minmax_check, RM_BLACKBOARD_WRITE_BBVARI_PHYS_MINMAX_CHECK_CMD, 0 },
    /* 115 */{ Func_get_phys_value_for_raw_value, RM_BLACKBOARD_GET_PHYS_VALUE_FOR_RAW_VALUE_CMD, 0 },
    /* 116 */{ Func_get_raw_value_for_phys_value, RM_BLACKBOARD_GET_RAW_VALUE_FOR_PHYS_VALUE_CMD, 0 },
    /* 117 */{ Func_read_bbvari_byte, RM_BLACKBOARD_READ_BBVARI_BYTE_CMD, 0 },
    /* 118 */{ Func_read_bbvari_ubyte, RM_BLACKBOARD_READ_BBVARI_UBYTE_CMD, 0 },
    /* 119 */{ Func_read_bbvari_word, RM_BLACKBOARD_READ_BBVARI_WORD_CMD, 0 },
    /* 120 */{ Func_read_bbvari_uword, RM_BLACKBOARD_READ_BBVARI_UWORD_CMD, 0 },
    /* 121 */{ Func_read_bbvari_dword, RM_BLACKBOARD_READ_BBVARI_DWORD_CMD, 0 },
    /* 122 */{ Func_read_bbvari_udword, RM_BLACKBOARD_READ_BBVARI_UDWORD_CMD, 0 },

    /* 123 */{ Func_read_bbvari_qword, RM_BLACKBOARD_READ_BBVARI_QWORD_CMD, 0 },
    /* 124 */{ Func_read_bbvari_uqword, RM_BLACKBOARD_READ_BBVARI_UQWORD_CMD, 0 },

    /* 125 */{ Func_read_bbvari_float, RM_BLACKBOARD_READ_BBVARI_FLOAT_CMD, 0 },
    /* 126 */{ Func_read_bbvari_double, RM_BLACKBOARD_READ_BBVARI_DOUBLE_CMD, 0 },
    /* 127 */{ Func_read_bbvari_union, RM_BLACKBOARD_READ_BBVARI_UNION_CMD, 0 },
    /* 128 */{ Func_read_bbvari_union_type, RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_CMD, 0 },
    /* 129 */{ Func_read_bbvari_union_type_frame, RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_CMD, 0 },
    /* 130 */{ Func_read_bbvari_equ, RM_BLACKBOARD_READ_BBVARI_EQU_CMD, 0 },
    /* 131 */{ Func_read_bbvari_convert_double, RM_BLACKBOARD_READ_BBVARI_CONVERT_DOUBLE_CMD, 0 },
    /* 132 */{ Func_read_bbvari_convert_to, RM_BLACKBOARD_READ_BBVARI_CONVERT_TO_CMD, 0 },
    /* 133 */{ Func_read_bbvari_textreplace, RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_CMD, 0 },
    /* 134 */{ Func_convert_value_textreplace, RM_BLACKBOARD_CONVERT_TEXTREPLACE_CMD, 0 },
    /* 135 */{ Func_convert_textreplace_value, RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_CMD, 0 },
    /* 136 */{ Func_read_bbvari_frame, RM_BLACKBOARD_READ_BBVARI_FRAME_CMD, 0 },
    /* 137 */{ Func_write_bbvari_frame, RM_BLACKBOARD_WRITE_BBVARI_FRAME_CMD, 0 },
    /* 138 */{ Func_read_bbvari_convert_to_FloatAndInt64, RM_READ_BBVARI_CONVERT_TO_FLOATANDINT64_CMD, 0 },
    /* 139 */{ Func_write_bbvari_convert_with_FloatAndInt64, RM_WRITE_BBVARI_CONVERT_WITH_FLOATANDINT64_CMD, 0 },
    /* 140 */{ Func_read_bbvari_by_name, RM_BLACKBOARD_READ_BBVARI_BY_NAME_CMD, 0 },
    /* 141 */{ Func_write_bbvari_by_name, RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_CMD, 0 },
    /* 142 */{ Func_undefined, 0, 0 },
    /* 143 */{ Func_undefined, 0, 0 },
    /* 144 */{ Func_undefined, 0, 0 },
    /* 145 */{ Func_undefined, 0, 0 },
    /* 146 */{ Func_undefined, 0, 0 },
    /* 147 */{ Func_undefined, 0, 0 },
    /* 148 */{ Func_undefined, 0, 0 },
    /* 149 */{ Func_undefined, 0, 0 },
    /* 150 */{ Func_write_message_ts_as, RM_MESSAGE_WRITE_MESSAGE_TS_AS_CMD, 0 },
    /* 151 */{ Func_write_important_message_ts_as, RM_MESSAGE_WRITE_IMPORTANT_MESSAGE_TS_AS_CMD, 0 },
    /* 152 */{ Func_test_message_as, RM_MESSAGE_TEST_MESSAGE_AS_CMD, 0 },
    /* 153 */{ Func_read_message_as, RM_MESSAGE_READ_MESSAGE_AS_CMD, 0 },
    /* 154 */{ Func_remove_message_as, RM_MESSAGE_REMOVE_MESSAGE_AS_CMD, 0 },
    /* 155 */{ Func_undefined, 0, 0 },
    /* 156 */{ Func_undefined, 0, 0 },
    /* 157 */{ Func_undefined, 0, 0 },
    /* 158 */{ Func_undefined, 0, 0 },
    /* 159 */{ Func_undefined, 0, 0 },
    /* 160 */{ Func_AddRealtimeScheduler, RM_SCHEDULER_ADD_REALTIME_SCHEDULER_CMD, 0 },
    /* 161 */{ Func_StopRealtimeScheduler, RM_SCHEDULER_STOP_REALTIME_SCHEDULER_CMD, 0 },
    /* 162 */{ Func_StartRealtimeProcess, RM_SCHEDULER_START_REALTIME_PROCESS_CMD, 0 },
    /* 163 */{ Func_StopRealtimeProcess, RM_SCHEDULER_STOP_REALTIME_PROCESS_CMD, 0 },
    /* 164 */{ Func_GetNextRealtimeProcess, RM_SCHEDULER_GET_NEXT_REALTIME_PROCESS_CMD, 0 },
    /* 165 */{ Func_IsRealtimeProcess, RM_SCHEDULER_IS_REALTIME_PROCESS_CMD, 0 },
    /* 166 */{ Func_AddProcessToPidNameArray, RM_SCHEDULER_ADD_PROCESS_TO_PID_NAME_ARRAY_CMD, 0 },
    /* 167 */{ Func_RemoveProcessFromPidNameArray, RM_SCHEDULER_REMOVE_PROCESS_FROM_PID_NAME_ARRAY_CMD, 0 },
    /* 168 */{ Func_get_pid_by_name, RM_SCHEDULER_GET_PID_BY_NAME_CMD, 0 },
    /* 169 */{ Func_get_timestamp_counter, RM_SCHEDULER_GET_TIMESTAMPCOUNTER_CMD, 0 },
    /* 170 */{ Func_get_process_info_ex, RM_SCHEDULER_GET_PROCESS_INFO_EX_CMD, 0 },
    /* 171 */{ Func_GetOrFreeUniquePid, RM_SCHEDULER_GET_OR_FREE_UNIQUE_PID_CMD, 0 },
    /* 172 */{ Func_AddBeforeProcessEquation, RM_SCHEDULER_ADD_BEFORE_PROCESS_EQUATION_CMD, 0 },
    /* 173 */{ Func_AddBehindProcessEquation, RM_SCHEDULER_ADD_BEHIND_PROCESS_EQUATION_CMD, 0 },
    /* 174 */{ Func_DelBeforeProcessEquations, RM_SCHEDULER_DEL_BEFORE_PROCESS_EQUATIONS_CMD, 0 },
    /* 175 */{ Func_DelBehindProcessEquations, RM_SCHEDULER_DEL_BEHIND_PROCESS_EQUATIONS_CMD, 0 },
    /* 176 */{ Func_IsRealtimeProcessPid, RM_SCHEDULER_IS_REALTIME_PROCESS_PID_CMD, 0 },
    /* 177 */{ Func_ResetProcessMinMaxRuntimeMeasurement, RM_RESET_PROCESS_MIN_MAX_RUNTIME_MEASUREMENT_CMD, 0 },
    /* 178 */{ Func_undefined, 0, 0 },
    /* 179 */{ Func_undefined, 0, 0 },
    /* 180 */{ Func_undefined, 0, 0 },
    /* 181 */{ Func_undefined, 0, 0 },
    /* 182 */{ Func_undefined, 0, 0 },
    /* 183 */{ Func_undefined, 0, 0 },
    /* 184 */{ Func_undefined, 0, 0 },
    /* 185 */{ Func_undefined, 0, 0 },
    /* 186 */{ Func_undefined, 0, 0 },
    /* 187 */{ Func_undefined, 0, 0 },
    /* 188 */{ Func_undefined, 0, 0 },
    /* 189 */{ Func_undefined, 0, 0 },
    /* 190 */{ Func_undefined, 0, 0 },
    /* 191 */{ Func_undefined, 0, 0 },
    /* 192 */{ Func_undefined, 0, 0 },
    /* 193 */{ Func_undefined, 0, 0 },
    /* 194 */{ Func_undefined, 0, 0 },
    /* 195 */{ Func_undefined, 0, 0 },
    /* 196 */{ Func_undefined, 0, 0 },
    /* 197 */{ Func_undefined, 0, 0 },
    /* 198 */{ Func_undefined, 0, 0 },
    /* 199 */{ Func_undefined, 0, 0 },
    /* 200 */{ Func_CreateCanFifos, RM_CAN_FIFO_CREATE_CAN_FIFOS_CMD, 0 },
    /* 201 */{ Func_DeleteCanFifos, RM_CAN_FIFO_DELETE_CAN_FIFOS_CMD, 0 },
    /* 202 */{ Func_FlushCanFifo, RM_CAN_FIFO_FLUSH_CAN_FIFO_CMD , 0 },
    /* 203 */{ Func_ReadCanMessageFromFifo2Process, RM_CAN_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_CMD, 0 },
    /* 204 */{ Func_ReadCanMessagesFromFifo2Process, RM_CAN_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_CMD, 0 },
    /* 205 */{ Func_WriteCanMessagesFromProcess2Fifo, RM_CAN_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_CMD, 0 },
    /* 206 */{ Func_SetAcceptMask4CanFifo, RM_CAN_FIFO_SET_ACCEPT_MASK4CANFIFO_CMD, 0 },
    /* 207 */{ Func_WriteCanMessageFromProcess2Fifo, RM_CAN_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_CMD, 0 },
    /* 208 */{ Func_ReadCanFdMessageFromFifo2Process, RM_CAN_FD_FIFO_READ_CAN_MESSAGE_FROM_FIFO2PROCESS_CMD, 0 },
    /* 209 */{ Func_ReadCanFdMessagesFromFifo2Process, RM_CAN_FD_FIFO_READ_CAN_MESSAGES_FROM_FIFO2PROCESS_CMD, 0 },
	/* 210 */{ Func_WriteCanFdMessagesFromProcess2Fifo, RM_CAN_FD_FIFO_WRITE_CAN_MESSAGES_FROM_PROCESS2FIFO_CMD, 0 },
	/* 211 */{ Func_WriteCanFdMessageFromProcess2Fifo, RM_CAN_FD_FIFO_WRITE_CAN_MESSAGE_FROM_PROCESS2FIFO_CMD, 0 },
	/* 212 */{ Func_undefined, 0, 0 },
	/* 213 */{ Func_undefined, 0, 0 },
	/* 214 */{ Func_undefined, 0, 0 },
	/* 215 */{ Func_undefined, 0, 0 },
	/* 216 */{ Func_undefined, 0, 0 },
	/* 217 */{ Func_undefined, 0, 0 },
	/* 218 */{ Func_undefined, 0, 0 },
	/* 219 */{ Func_undefined, 0, 0 },
	/* 220 */{ Func_undefined, 0, 0 },
	/* 221 */{ Func_undefined, 0, 0 },
	/* 222 */{ Func_undefined, 0, 0 },
	/* 223 */{ Func_undefined, 0, 0 },
	/* 224 */{ Func_undefined, 0, 0 },
	/* 225 */{ Func_undefined, 0, 0 },
	/* 226 */{ Func_undefined, 0, 0 },
	/* 227 */{ Func_undefined, 0, 0 },
	/* 228 */{ Func_undefined, 0, 0 },
	/* 229 */{ Func_undefined, 0, 0 },
	/* 230 */{ Func_undefined, 0, 0 },
	/* 231 */{ Func_undefined, 0, 0 },
	/* 232 */{ Func_undefined, 0, 0 },
	/* 233 */{ Func_undefined, 0, 0 },
	/* 234 */{ Func_undefined, 0, 0 },
	/* 235 */{ Func_undefined, 0, 0 },
	/* 236 */{ Func_undefined, 0, 0 },
	/* 237 */{ Func_undefined, 0, 0 },
	/* 238 */{ Func_undefined, 0, 0 },
	/* 239 */{ Func_undefined, 0, 0 },
	/* 240 */{ Func_undefined, 0, 0 },
	/* 241 */{ Func_undefined, 0, 0 },
	/* 242 */{ Func_undefined, 0, 0 },
	/* 243 */{ Func_undefined, 0, 0 },
	/* 244 */{ Func_undefined, 0, 0 },
	/* 245 */{ Func_undefined, 0, 0 },
	/* 246 */{ Func_undefined, 0, 0 },
	/* 247 */{ Func_undefined, 0, 0 },
	/* 248 */{ Func_undefined, 0, 0 },
	/* 249 */{ Func_undefined, 0, 0 },
	/* 250 */{ Func_RegisterNewRxFiFoName, RM_REGISTER_NEW_RX_FIFO_NAME_CMD, 0 },
    /* 251 */{ Func_UnRegisterRxFiFo, RM_UNREGISTER_RX_FIFO_CMD, 0 },
	/* 252 */{ Func_RxAttachFiFo, RM_RX_ATTACH_FIFO_CMD, 0 },
	/* 253 */{ Func_SyncFiFos, RM_SYNC_FIFOS_CMD, 0 },
	/* 254 */{ Func_undefined, 0, 0 },
	/* 255 */{ Func_undefined, 0, 0 },
	/* 256 */{ Func_undefined, 0, 0 },
	/* 257 */{ Func_undefined, 0, 0 },
	/* 258 */{ Func_undefined, 0, 0 },
	/* 259 */{ Func_undefined, 0, 0 }
};

uint32_t DecodeCommand(RM_PACKAGE_HEADER *Req, RM_PACKAGE_HEADER *Ack)
{
    int Command = (int)Req->Command;
    if ((Command >= 0) && (Command < (int)(sizeof(FuntionPointerTable) / sizeof(FuntionPointerTable[0])))) {
        if (Command != FuntionPointerTable[Command].FuncNr) {
            printf ("Command table missmatch \"Command=%i != FuntionPointerTable[Command].FuncNr=%i\"", Command, FuntionPointerTable[Command].FuncNr);
            //error(1, "Command table missmatch \"Command=%i != FuntionPointerTable[Command].FuncNr=%i\"", Command, FuntionPointerTable[Command].FuncNr);
            return 0;
        } else {
            int Ret = FuntionPointerTable[Command].FuncPtr(Req, Ack);
            FuntionPointerTable[Command].CallCounter++;
            Ack->Command = Req->Command;
            Ack->PackageCounter = Req->PackageCounter;
            Ack->ChannelNumber = Req->ChannelNumber;
            return Ret;
        }
    } else {
        ThrowError(1, "Command outside command table %i >= %i", Command, (sizeof(FuntionPointerTable) / sizeof(FuntionPointerTable[0])));
        return 0;
    }
    return 0;
}
