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
#include <memory.h>
#include <fcntl.h>

#include "MemZeroAndCopy.h"
#include "Wildcards.h"
#include "Scheduler.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "EquationParser.h"
#include "ExtProcessReferences.h"
#include "MainValues.h"
#include "ImExportVarProperties.h"
#include "A2LLink.h"

#include "RpcControlProcess.h"

#include "RpcSocketServer.h"
#include "RpcFuncBlackboard.h"

#define UNUSED(x) (void)(x)

static int RPCFunc_AddBbvari(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_ADD_BBVARI_MESSAGE *In = (RPC_API_ADD_BBVARI_MESSAGE*)par_DataIn;
    RPC_API_ADD_BBVARI_MESSAGE_ACK *Out = (RPC_API_ADD_BBVARI_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_ADD_BBVARI_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_ADD_BBVARI_MESSAGE_ACK);

    Out->Header.ReturnValue = add_bbvari_pid ((char*)In + In->OffsetLabel, In->Type, (char*)In + In->OffsetUnit, GetRPCControlPid());

    return Out->Header.StructSize;
}

static int RPCFunc_RemoveBbvari(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_REMOVE_BBVARI_MESSAGE *In = (RPC_API_REMOVE_BBVARI_MESSAGE*)par_DataIn;
    RPC_API_REMOVE_BBVARI_MESSAGE_ACK *Out = (RPC_API_REMOVE_BBVARI_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_REMOVE_BBVARI_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_REMOVE_BBVARI_MESSAGE_ACK);

    Out->Header.ReturnValue = remove_bbvari_pid (In->Vid, GetRPCControlPid());

    return Out->Header.StructSize;
}

static int RPCFunc_AttachBbvari(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_ATTACH_BBVARI_MESSAGE *In = (RPC_API_ATTACH_BBVARI_MESSAGE*)par_DataIn;
    RPC_API_ATTACH_BBVARI_MESSAGE_ACK *Out = (RPC_API_ATTACH_BBVARI_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_ATTACH_BBVARI_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_ATTACH_BBVARI_MESSAGE_ACK);

    Out->Header.ReturnValue = attach_bbvari_by_name((char*)In + In->OffsetLabel, GetRPCControlPid());
    if (Out->Header.ReturnValue > 0) {
        // Only visable variables exists
        if (get_bbvaritype (Out->Header.ReturnValue) >= BB_UNKNOWN) Out->Header.ReturnValue = UNKNOWN_VARIABLE;
    }

    return Out->Header.StructSize;
}

static int RPCFunc_Get(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_MESSAGE *In = (RPC_API_GET_MESSAGE*)par_DataIn;
    RPC_API_GET_MESSAGE_ACK *Out = (RPC_API_GET_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_MESSAGE_ACK);

    Out->ReturnValue = read_bbvari_convert_double(In->Vid);

    Out->Header.ReturnValue = 0;

    return Out->Header.StructSize;
}

static int RPCFunc_GetPhys(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_PHYS_MESSAGE *In = (RPC_API_GET_PHYS_MESSAGE*)par_DataIn;
    RPC_API_GET_PHYS_MESSAGE_ACK *Out = (RPC_API_GET_PHYS_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_PHYS_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_PHYS_MESSAGE_ACK);

    Out->ReturnValue = read_bbvari_equ(In->Vid);

    Out->Header.ReturnValue = 0;

    return Out->Header.StructSize;
}

static int RPCFunc_Set(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_MESSAGE *In = (RPC_API_SET_MESSAGE*)par_DataIn;
    RPC_API_SET_MESSAGE_ACK *Out = (RPC_API_SET_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SET_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_MESSAGE_ACK);

    write_bbvari_minmax_check_pid (GetRPCControlPid(), In->Vid, In->Value);
    Out->Header.ReturnValue = 0;

    return Out->Header.StructSize;
}

static int RPCFunc_SetPhys(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_PHYS_MESSAGE *In = (RPC_API_SET_PHYS_MESSAGE*)par_DataIn;
    RPC_API_SET_PHYS_MESSAGE_ACK *Out = (RPC_API_SET_PHYS_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SET_PHYS_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_PHYS_MESSAGE_ACK);

    Out->Header.ReturnValue = write_bbvari_phys_minmax_check_pid (GetRPCControlPid(), In->Vid, In->Value);

    return Out->Header.StructSize;
}

static int RPCFunc_Equ(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_EQU_MESSAGE *In = (RPC_API_EQU_MESSAGE*)par_DataIn;
    RPC_API_EQU_MESSAGE_ACK *Out = (RPC_API_EQU_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_EQU_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_EQU_MESSAGE_ACK);

    Out->ReturnValue = direct_solve_equation_no_add_bbvari_pid((char*)In + In->OffsetEquation, GetRPCControlPid());
    Out->Header.ReturnValue = 0;

    return Out->Header.StructSize;
}

static int RPCFunc_WrVariEnable(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_WR_VARI_ENABLE_MESSAGE *In = (RPC_API_WR_VARI_ENABLE_MESSAGE*)par_DataIn;
    RPC_API_WR_VARI_ENABLE_MESSAGE_ACK *Out = (RPC_API_WR_VARI_ENABLE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_WR_VARI_ENABLE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_WR_VARI_ENABLE_MESSAGE_ACK);

    Out->Header.ReturnValue = enable_bbvari_access(get_pid_by_name((char*)In + In->OffsetProcess),
                                                   get_bbvarivid_by_name ((char*)In + In->OffsetLabel));

    return Out->Header.StructSize;
}

static int RPCFunc_WrVariDisable(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_WR_VARI_DISABLE_MESSAGE *In = (RPC_API_WR_VARI_DISABLE_MESSAGE*)par_DataIn;
    RPC_API_WR_VARI_DISABLE_MESSAGE_ACK *Out = (RPC_API_WR_VARI_DISABLE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_WR_VARI_DISABLE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_WR_VARI_DISABLE_MESSAGE_ACK);

    Out->Header.ReturnValue = disable_bbvari_access(get_pid_by_name((char*)In + In->OffsetProcess),
                                                    get_bbvarivid_by_name ((char*)In + In->OffsetLabel));

    return Out->Header.StructSize;
}

static int RPCFunc_IsWrVariEnsable(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    PID pid;
    char Help[BBVARI_NAME_SIZE];
    int index;
    RPC_API_IS_WR_VARI_ENABLED_MESSAGE *In = (RPC_API_IS_WR_VARI_ENABLED_MESSAGE*)par_DataIn;
    RPC_API_IS_WR_VARI_ENABLED_MESSAGE_ACK *Out = (RPC_API_IS_WR_VARI_ENABLED_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_IS_WR_VARI_ENABLED_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_IS_WR_VARI_ENABLED_MESSAGE_ACK);


    pid = get_pid_by_name ((char*)In + In->OffsetProcess);
    if (pid <= 0) {
        Out->Header.ReturnValue = UNKNOWN_PROCESS;
        return Out->Header.StructSize;
    }
    // Loop through all variables the process has write access
    index = 0;
    while ((index = ReadNextVariableProcessAccess (index, pid, 1, Help, sizeof(Help))) > 0) {
        if (!strcmp (Help, (char*)In + In->OffsetLabel)) {
            Out->Header.ReturnValue = 1;
            return Out->Header.StructSize;
        }
    }

    // Loop through all variables the process only has read access
    index = 0;
    while ((index = ReadNextVariableProcessAccess (index, pid, 2, Help, sizeof(Help))) > 0) {
        if (!strcmp (Help, (char*)In + In->OffsetLabel)) {
            Out->Header.ReturnValue = 0;
            return Out->Header.StructSize;
        }
    }
    Out->Header.ReturnValue = UNKNOWN_VARIABLE;
    return Out->Header.StructSize;
}

static int RPCFunc_LoadRefList(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_LOAD_REF_LIST_MESSAGE *In = (RPC_API_LOAD_REF_LIST_MESSAGE*)par_DataIn;
    RPC_API_LOAD_REF_LIST_MESSAGE_ACK *Out = (RPC_API_LOAD_REF_LIST_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_LOAD_REF_LIST_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_LOAD_REF_LIST_MESSAGE_ACK);

    Out->Header.ReturnValue = ImportVariableReferenceList (get_pid_by_name((char*)In + In->OffsetProcess),
                                                          (char*)In + In->OffsetRefList);

    return Out->Header.StructSize;
}

static int RPCFunc_AddRefList(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_ADD_REF_LIST_MESSAGE *In = (RPC_API_ADD_REF_LIST_MESSAGE*)par_DataIn;
    RPC_API_ADD_REF_LIST_MESSAGE_ACK *Out = (RPC_API_ADD_REF_LIST_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_ADD_REF_LIST_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_ADD_REF_LIST_MESSAGE_ACK);

    Out->Header.ReturnValue = AddVariableReferenceList (get_pid_by_name((char*)In + In->OffsetProcess),
                                                       (char*)In + In->OffsetRefList);

    return Out->Header.StructSize;
}

static int RPCFunc_SaveRefList(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_LOAD_REF_LIST_MESSAGE *In = (RPC_API_LOAD_REF_LIST_MESSAGE*)par_DataIn;
    RPC_API_LOAD_REF_LIST_MESSAGE_ACK *Out = (RPC_API_LOAD_REF_LIST_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_LOAD_REF_LIST_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_LOAD_REF_LIST_MESSAGE_ACK);

    Out->Header.ReturnValue = ExportVariableReferenceList (get_pid_by_name((char*)In + In->OffsetProcess),
                                                           (char*)In + In->OffsetRefList, s_main_ini_val.SaveRefListInNewFormatAsDefault);

    return Out->Header.StructSize;
}

static int RPCFunc_GetVariConversionType(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE *In = (RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE*)par_DataIn;
    RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE_ACK *Out = (RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE_ACK);

    Out->Header.ReturnValue = get_bbvari_conversiontype (In->Vid);

    return Out->Header.StructSize;
}

static int RPCFunc_GetVariConversionString(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE *In = (RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE*)par_DataIn;
    RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE_ACK *Out = (RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE_ACK);

    Out->OffsetConversionString = sizeof(RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE_ACK) - 1;
    Out->Header.ReturnValue =  get_bbvari_conversion(In->Vid, Out->Data, RPC_API_MAX_MESSAGE_SIZE - sizeof(RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE_ACK));
    if (Out->Header.ReturnValue == 1) Out->Header.StructSize += (int32_t)strlen(Out->Data);
    else if (Out->Header.ReturnValue == 2) Out->Header.StructSize += (int32_t)strlen(Out->Data);
    else Out->Data[0] = 0;
    return Out->Header.StructSize;
}

static int RPCFunc_SetVariConversion(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_VARI_CONVERSION_MESSAGE *In = (RPC_API_SET_VARI_CONVERSION_MESSAGE*)par_DataIn;
    RPC_API_SET_VARI_CONVERSION_MESSAGE_ACK *Out = (RPC_API_SET_VARI_CONVERSION_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SET_VARI_CONVERSION_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_VARI_CONVERSION_MESSAGE_ACK);

    Out->Header.ReturnValue =  set_bbvari_conversion(In->Vid, In->Type, (char*)In + In->OffsetConversionString);

    return Out->Header.StructSize;
}

static int RPCFunc_GetVariType(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_VARI_TYPE_MESSAGE *In = (RPC_API_GET_VARI_TYPE_MESSAGE*)par_DataIn;
    RPC_API_GET_VARI_TYPE_MESSAGE_ACK *Out = (RPC_API_GET_VARI_TYPE_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_VARI_TYPE_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_VARI_TYPE_MESSAGE_ACK);

    Out->Header.ReturnValue =  get_bbvaritype(In->Vid);

    return Out->Header.StructSize;
}

static int RPCFunc_GetVariUnit(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_VARI_UNIT_MESSAGE *In = (RPC_API_GET_VARI_UNIT_MESSAGE*)par_DataIn;
    RPC_API_GET_VARI_UNIT_MESSAGE_ACK *Out = (RPC_API_GET_VARI_UNIT_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_VARI_UNIT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_VARI_UNIT_MESSAGE_ACK);

    Out->OffsetUnit = sizeof(RPC_API_GET_VARI_UNIT_MESSAGE_ACK) - 1;
    Out->Header.ReturnValue =  get_bbvari_unit(In->Vid, Out->Data, RPC_API_MAX_MESSAGE_SIZE - sizeof(RPC_API_GET_VARI_UNIT_MESSAGE_ACK));
    if (Out->Header.ReturnValue == 0) Out->Header.StructSize += (int32_t)strlen(Out->Data);

    return Out->Header.StructSize;
}

static int RPCFunc_SetVariUnit(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_VARI_UNIT_MESSAGE *In = (RPC_API_SET_VARI_UNIT_MESSAGE*)par_DataIn;
    RPC_API_SET_VARI_UNIT_MESSAGE_ACK *Out = (RPC_API_SET_VARI_UNIT_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SET_VARI_UNIT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_VARI_UNIT_MESSAGE_ACK);

    Out->Header.ReturnValue =  set_bbvari_unit(In->Vid, (char*)In + In->OffsetUnit);

    return Out->Header.StructSize;
}

static int RPCFunc_GetVariMin(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_VARI_MIN_MESSAGE *In = (RPC_API_GET_VARI_MIN_MESSAGE*)par_DataIn;
    RPC_API_GET_VARI_MIN_MESSAGE_ACK *Out = (RPC_API_GET_VARI_MIN_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_VARI_MIN_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_VARI_MIN_MESSAGE_ACK);

    Out->ReturnValue =  get_bbvari_min(In->Vid);

    return Out->Header.StructSize;
}

static int RPCFunc_GetVariMax(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_VARI_MAX_MESSAGE *In = (RPC_API_GET_VARI_MAX_MESSAGE*)par_DataIn;
    RPC_API_GET_VARI_MAX_MESSAGE_ACK *Out = (RPC_API_GET_VARI_MAX_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_VARI_MAX_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_VARI_MAX_MESSAGE_ACK);

    Out->ReturnValue =  get_bbvari_max(In->Vid);

    return Out->Header.StructSize;
}

static int RPCFunc_SetVariMin(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_VARI_MIN_MESSAGE *In = (RPC_API_SET_VARI_MIN_MESSAGE*)par_DataIn;
    RPC_API_SET_VARI_MIN_MESSAGE_ACK *Out = (RPC_API_SET_VARI_MIN_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SET_VARI_MIN_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_VARI_MIN_MESSAGE_ACK);

    Out->Header.ReturnValue =  set_bbvari_min(In->Vid, In->Min);

    return Out->Header.StructSize;
}

static int RPCFunc_SetVariMax(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_VARI_MAX_MESSAGE *In = (RPC_API_SET_VARI_MAX_MESSAGE*)par_DataIn;
    RPC_API_SET_VARI_MAX_MESSAGE_ACK *Out = (RPC_API_SET_VARI_MAX_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SET_VARI_MAX_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_VARI_MAX_MESSAGE_ACK);

    Out->Header.ReturnValue =  set_bbvari_max(In->Vid, In->Max);

    return Out->Header.StructSize;
}

static int RPCFunc_GetNextVari(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    char *Filter;
    char *ReturnLabelBuffer;
    RPC_API_GET_NEXT_VARI_MESSAGE *In = (RPC_API_GET_NEXT_VARI_MESSAGE*)par_DataIn;
    RPC_API_GET_NEXT_VARI_MESSAGE_ACK *Out = (RPC_API_GET_NEXT_VARI_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_NEXT_VARI_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_NEXT_VARI_MESSAGE_ACK);

    Out->OffsetLabel = sizeof(RPC_API_GET_NEXT_VARI_MESSAGE_ACK) - 1;
    ReturnLabelBuffer = (char*)Out + Out->OffsetLabel;
    Filter = (char*)In + In->OffsetFilter;
    if (In->Flag) par_Connection->GetNextIndex = 0;
    while ((par_Connection->GetNextIndex = read_next_blackboard_vari (par_Connection->GetNextIndex, ReturnLabelBuffer, 512)) > 0) {
        if (!Compare2StringsWithWildcards(ReturnLabelBuffer, Filter)) {
            Out->Header.StructSize += (int32_t)strlen(ReturnLabelBuffer);
            Out->Header.ReturnValue = 1;
            return Out->Header.StructSize;
        }
    }
    Out->Header.ReturnValue = 0; // nothing more found
    return Out->Header.StructSize;
}

static int RPCFunc_GetNextVariEx(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    char *Filter;
    int Pid;
    char *ReturnLabelBuffer;
    RPC_API_GET_NEXT_VARI_EX_MESSAGE *In = (RPC_API_GET_NEXT_VARI_EX_MESSAGE*)par_DataIn;
    RPC_API_GET_NEXT_VARI_EX_MESSAGE_ACK *Out = (RPC_API_GET_NEXT_VARI_EX_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_NEXT_VARI_EX_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_NEXT_VARI_EX_MESSAGE_ACK);

    Out->OffsetLabel = sizeof(RPC_API_GET_NEXT_VARI_EX_MESSAGE_ACK) - 1;
    ReturnLabelBuffer = (char*)Out + Out->OffsetLabel;

    Filter = (char*)In + In->OffsetFilter;

    if (In->Flag) par_Connection->GetNextIndex = 0;
    Pid = get_pid_by_name ((char*)In + In->OffsetProcess);
    if (Pid > 0) {
        while ((par_Connection->GetNextIndex = ReadNextVariableProcessAccess (par_Connection->GetNextIndex, Pid, In->AccessFlags, ReturnLabelBuffer, BBVARI_NAME_SIZE)) > 0) {
            if (!Compare2StringsWithWildcards(ReturnLabelBuffer, Filter)) {
                Out->Header.StructSize += (int32_t)strlen(ReturnLabelBuffer);
                Out->Header.ReturnValue = 1;
                return Out->Header.StructSize;
            }
        }
    }
    Out->Header.ReturnValue = 0; // nothing more found
    return Out->Header.StructSize;
}

static int RPCFunc_GetVariEnum(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    char *ReturnEnumBuffer;
    RPC_API_GET_VARI_ENUM_MESSAGE *In = (RPC_API_GET_VARI_ENUM_MESSAGE*)par_DataIn;
    RPC_API_GET_VARI_ENUM_MESSAGE_ACK *Out = (RPC_API_GET_VARI_ENUM_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_VARI_ENUM_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_VARI_ENUM_MESSAGE_ACK);

    Out->OffsetEnum = sizeof(RPC_API_GET_VARI_ENUM_MESSAGE_ACK) - 1;
    ReturnEnumBuffer =(char*)Out + Out->OffsetEnum;

    convert_value_textreplace (In->Vid, (long)In->Value, ReturnEnumBuffer, RPC_API_MAX_MESSAGE_SIZE - sizeof(RPC_API_GET_VARI_ENUM_MESSAGE_ACK), NULL);
    if (*ReturnEnumBuffer == 0) Out->Header.ReturnValue = -1;
    else Out->Header.ReturnValue = 0;
    Out->Header.StructSize += (int32_t)strlen(ReturnEnumBuffer);
    return Out->Header.StructSize;
}

static int RPCFunc_GetVariDisplayFormatWidth(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE *In = (RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE*)par_DataIn;
    RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE_ACK *Out = (RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE_ACK);

    Out->Header.ReturnValue =  get_bbvari_format_width(In->Vid);

    return Out->Header.StructSize;
}

static int RPCFunc_GetVariDisplayFormatPrec(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE *In = (RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE*)par_DataIn;
    RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE_ACK *Out = (RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE_ACK);

    Out->Header.ReturnValue =  get_bbvari_format_prec(In->Vid);

    return Out->Header.StructSize;
}

static int RPCFunc_SetVariDisplayFormat(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE *In = (RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE*)par_DataIn;
    RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE_ACK *Out = (RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE_ACK);

    Out->Header.ReturnValue =  set_bbvari_format(In->Vid, In->Width, In->Prec);

    return Out->Header.StructSize;
}

static int RPCFunc_ImportVariProperties(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_IMPORT_VARI_PROPERTIES_MESSAGE *In = (RPC_API_IMPORT_VARI_PROPERTIES_MESSAGE*)par_DataIn;
    RPC_API_IMPORT_VARI_PROPERTIES_MESSAGE_ACK *Out = (RPC_API_IMPORT_VARI_PROPERTIES_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_IMPORT_VARI_PROPERTIES_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_IMPORT_VARI_PROPERTIES_MESSAGE_ACK);

    Out->Header.ReturnValue =  ScriptImportVariablesProperties ((char*)In + In->OffsetFilename);

    return Out->Header.StructSize;
}

static int RPCFunc_EnableRangeControl(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_ENABLE_RANGE_CONTROL_MESSAGE *In = (RPC_API_ENABLE_RANGE_CONTROL_MESSAGE*)par_DataIn;
    RPC_API_ENABLE_RANGE_CONTROL_MESSAGE_ACK *Out = (RPC_API_ENABLE_RANGE_CONTROL_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_ENABLE_RANGE_CONTROL_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_ENABLE_RANGE_CONTROL_MESSAGE_ACK);

    Out->Header.ReturnValue = enable_bbvari_range_control ((char*)In + In->OffsetProcessNameFilter, (char*)In + In->OffsetVariableNameFilter);

    return Out->Header.StructSize;
}

static int RPCFunc_DisableRangeControl(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_DISABLE_RANGE_CONTROL_MESSAGE *In = (RPC_API_DISABLE_RANGE_CONTROL_MESSAGE*)par_DataIn;
    RPC_API_DISABLE_RANGE_CONTROL_MESSAGE_ACK *Out = (RPC_API_DISABLE_RANGE_CONTROL_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_DISABLE_RANGE_CONTROL_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_DISABLE_RANGE_CONTROL_MESSAGE_ACK);

    Out->Header.ReturnValue = disable_bbvari_range_control ((char*)In + In->OffsetProcessNameFilter, (char*)In + In->OffsetVariableNameFilter);

    return Out->Header.StructSize;
}

static int RPCFunc_WriteFrame(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    int8_t *PhysOrRaw;
    RPC_API_WRITE_FRAME_MESSAGE *In = (RPC_API_WRITE_FRAME_MESSAGE*)par_DataIn;
    RPC_API_WRITE_FRAME_MESSAGE_ACK *Out = (RPC_API_WRITE_FRAME_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_WRITE_FRAME_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_WRITE_FRAME_MESSAGE_ACK);

    if (In->Offset_int8_Elements_PhysOrRaw <= 0) {
        PhysOrRaw = NULL;
    } else {
        PhysOrRaw = (int8_t*)((char*)In + In->Offset_int8_Elements_PhysOrRaw);
    }
    Out->Header.ReturnValue = write_bbvari_frame_pid (GetRPCControlPid(),
                                                     (VID*)(void*)((char*)In + In->Offset_int32_Elements_Vids),
                                                     PhysOrRaw,
                                                     (double*)(void*)((char*)In + In->Offset_double_Elements_ValueFrame),
                                                     In->Elements);

    return Out->Header.StructSize;
}

static int RPCFunc_GetFrame(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    int8_t *PhysOrRaw;
    RPC_API_GET_FRAME_MESSAGE *In = (RPC_API_GET_FRAME_MESSAGE*)par_DataIn;
    RPC_API_GET_FRAME_MESSAGE_ACK *Out = (RPC_API_GET_FRAME_MESSAGE_ACK*)par_DataOut;
    if (In->Offset_int8_Elements_PhysOrRaw <= 0) {
        PhysOrRaw = NULL;
    } else {
        PhysOrRaw = (int8_t*)((char*)In + In->Offset_int8_Elements_PhysOrRaw);
    }
    MEMSET (Out, 0, sizeof (RPC_API_GET_FRAME_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_FRAME_MESSAGE_ACK);
    Out->Offset_double_Elements_ValueFrame = sizeof(RPC_API_GET_FRAME_MESSAGE_ACK) - 1;
    Out->Header.ReturnValue = read_bbvari_frame ((VID*)(void*)((char*)In + In->Offset_int32_Elements_Vids),
                                                PhysOrRaw,
                                                (double*)(void*)((char*)Out + Out->Offset_double_Elements_ValueFrame),
                                                In->Elements);
    if (Out->Header.ReturnValue == 0) {
        Out->Elements = In->Elements;
        Out->Header.StructSize += In->Elements * (int)sizeof(double);
    } else {
        Out->Elements = 0;
    }
    return Out->Header.StructSize;
}

static void __SCWriteFrameWaitReadFramCallBack (void *Parameter)
{
    RPC_CONNECTION *Connection = (RPC_CONNECTION*)Parameter;
    Connection->SchedulerDisableCounter++;
    Connection->DoNextCycleFlag = 0;
    RemoteProcedureWakeWaitForConnection(Connection);
}


static int RPCFunc_WriteFrameWaitReadFrame(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    int8_t *PhysOrRaw;
    RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE *In = (RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE*)par_DataIn;
    RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE_ACK *Out = (RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE_ACK);
    Out->Offset_double_Read_ValuesRet = sizeof(RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE_ACK) - 1;
    if (par_Connection->SchedulerDisableCounter == 1) {
        par_Connection->SchedulerDisableCounter--;
        par_Connection->DoNextCycleFlag = 1;
        if (In->Offset_int8_Write_PhysOrRaw <= 0) {
            PhysOrRaw = NULL;
        } else {
            PhysOrRaw = (int8_t*)((char*)In + In->Offset_int8_Write_PhysOrRaw);
        }
        write_bbvari_frame_pid (GetRPCControlPid(),
                               (VID*)(void*)((char*)In + In->Offset_int32_Write_Vids),
                               PhysOrRaw,
                               (double*)(void*)((char*)In + In->Offset_double_Write_Values), In->WriteSize);
        RemoteProcedureMarkedForWaitForConnection(par_Connection);
        switch(make_n_next_cycles(SCHEDULER_CONTROLED_BY_RPC, 1, NULL, __SCWriteFrameWaitReadFramCallBack, par_Connection)) {
        case 0: // stop scheduler request are added during the scheduler is running.
        case 1: // stop scheduler request are added but the scheduler was already stopped.
            RemoteProcedureWaitForConnection(par_Connection);
            break;
        default: // all others
            __SCWriteFrameWaitReadFramCallBack(par_Connection);
            break;
        }
        if (In->Offset_int8_Read_PhysOrRaw <= 0) {
            PhysOrRaw = NULL;
        } else {
            PhysOrRaw = (int8_t*)((char*)In + In->Offset_int8_Read_PhysOrRaw);
        }
        Out->Header.ReturnValue = read_bbvari_frame((VID*)(void*)((char*)In + In->Offset_int32_Read_Vids),
                                                    PhysOrRaw,
                                                    (double*)(void*)((char*)Out + Out->Offset_double_Read_ValuesRet), In->ReadSize);
        if (Out->Header.ReturnValue == 0) {
            Out->ReadSize = In->ReadSize;
            Out->Header.StructSize += In->ReadSize * (int)sizeof(double);
        } else {
            Out->ReadSize = 0;
        }
    } else {
        Out->Header.ReturnValue = -1;
    }
    return Out->Header.StructSize;
}

static int RPCFunc_ReferenceSymbol(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_REFERENCE_SYMBOL_MESSAGE *In = (RPC_API_REFERENCE_SYMBOL_MESSAGE*)par_DataIn;
    RPC_API_REFERENCE_SYMBOL_MESSAGE_ACK *Out = (RPC_API_REFERENCE_SYMBOL_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_REFERENCE_SYMBOL_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_REFERENCE_SYMBOL_MESSAGE_ACK);

    Out->Header.ReturnValue = ReferenceOneSymbol((char*)In + In->OffsetSymbol,
                       (char*)In + In->OffsetDisplayName,
                       (char*)In + In->OffsetProcess,
                       (char*)In + In->OffsetUnit,
                       In->ConversionType,
                       (char*)In + In->OffsetConversion,
                       In->Min, In->Max, In->Color, In->Width, In->Precision,
                       (In->Flags & 0xFFFF) | 0x10000);    // 0x10000 -> no call of ThrowError!

    return Out->Header.StructSize;
}

static int RPCFunc_DereferenceSymbol(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_DEREFERENCE_SYMBOL_MESSAGE *In = (RPC_API_DEREFERENCE_SYMBOL_MESSAGE*)par_DataIn;
    RPC_API_DEREFERENCE_SYMBOL_MESSAGE_ACK *Out = (RPC_API_DEREFERENCE_SYMBOL_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_DEREFERENCE_SYMBOL_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_DEREFERENCE_SYMBOL_MESSAGE_ACK);

    Out->Header.ReturnValue = DereferenceOneSymbol((char*)In + In->OffsetSymbol,
                       (char*)In + In->OffsetProcess,
                       (In->Flags & 0xFFFF) | 0x10000);    // 0x10000 -> no call of ThrowError!

    return Out->Header.StructSize;
}

static int RPCFunc_GetRaw(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_GET_RAW_MESSAGE *In = (RPC_API_GET_RAW_MESSAGE*)par_DataIn;
    RPC_API_GET_RAW_MESSAGE_ACK *Out = (RPC_API_GET_RAW_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_GET_RAW_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_GET_RAW_MESSAGE_ACK);

    Out->Header.ReturnValue = read_bbvari_union_type (In->Vid, &(Out->Value));

    return Out->Header.StructSize;
}

static int RPCFunc_SetRaw(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_SET_RAW_MESSAGE *In = (RPC_API_SET_RAW_MESSAGE*)par_DataIn;
    RPC_API_SET_RAW_MESSAGE_ACK *Out = (RPC_API_SET_RAW_MESSAGE_ACK*)par_DataOut;
    MEMSET (Out, 0, sizeof (RPC_API_SET_RAW_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_SET_RAW_MESSAGE_ACK);

    Out->Header.ReturnValue = 0;
    write_bbvari_convert_to(GetRPCControlPid(), In->Vid, In->Type, &(In->Value));

    return Out->Header.StructSize;
}

static int RPCFunc_ExportA2lMeasurementList(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_EXPORT_A2L_MEASUREMENT_LIST_MESSAGE *In = (RPC_API_EXPORT_A2L_MEASUREMENT_LIST_MESSAGE*)par_DataIn;
    RPC_API_EXPORT_A2L_MEASUREMENT_LIST_MESSAGE_ACK *Out = (RPC_API_EXPORT_A2L_MEASUREMENT_LIST_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_EXPORT_A2L_MEASUREMENT_LIST_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_EXPORT_A2L_MEASUREMENT_LIST_MESSAGE_ACK);

    Out->Header.ReturnValue = ExportMeasurementReferencesListForProcess (get_pid_by_name((char*)In + In->OffsetProcess),
                                                                        (char*)In + In->OffsetRefList);

    return Out->Header.StructSize;
}

static int RPCFunc_ImportA2lMeasurementList(RPC_CONNECTION *par_Connection, RPC_API_BASE_MESSAGE *par_DataIn, RPC_API_BASE_MESSAGE_ACK *par_DataOut)
{
    UNUSED(par_Connection);
    RPC_API_IMPORT_A2L_MEASUREMENT_LIST_MESSAGE *In = (RPC_API_IMPORT_A2L_MEASUREMENT_LIST_MESSAGE*)par_DataIn;
    RPC_API_IMPORT_A2L_MEASUREMENT_LIST_MESSAGE_ACK *Out = (RPC_API_IMPORT_A2L_MEASUREMENT_LIST_MESSAGE_ACK*)par_DataOut;
    memset (Out, 0, sizeof (RPC_API_IMPORT_A2L_MEASUREMENT_LIST_MESSAGE_ACK));
    Out->Header.StructSize = sizeof(RPC_API_IMPORT_A2L_MEASUREMENT_LIST_MESSAGE_ACK);

    Out->Header.ReturnValue = ImportMeasurementReferencesListForProcess (get_pid_by_name((char*)In + In->OffsetProcess),
                                                                        (char*)In + In->OffsetRefList);

    return Out->Header.StructSize;
}

int AddBlackboardFunctionToTable(void)
{
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_ADD_BBVARI_CMD, 1, RPCFunc_AddBbvari, sizeof(RPC_API_ADD_BBVARI_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_ADD_BBVARI_MESSAGE_MEMBERS), STRINGIZE(RPC_API_ADD_BBVARI_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_REMOVE_BBVARI_CMD, 1, RPCFunc_RemoveBbvari, sizeof(RPC_API_REMOVE_BBVARI_MESSAGE), sizeof(RPC_API_REMOVE_BBVARI_MESSAGE), STRINGIZE(RPC_API_REMOVE_BBVARI_MESSAGE_MEMBERS), STRINGIZE(RPC_API_REMOVE_BBVARI_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_ATTACH_BBVARI_CMD, 1, RPCFunc_AttachBbvari, sizeof(RPC_API_ATTACH_BBVARI_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_ATTACH_BBVARI_MESSAGE_MEMBERS), STRINGIZE(RPC_API_ATTACH_BBVARI_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_CMD, 1, RPCFunc_Get, sizeof(RPC_API_GET_MESSAGE), sizeof(RPC_API_GET_MESSAGE), STRINGIZE(RPC_API_GET_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_PHYS_CMD, 1, RPCFunc_GetPhys, sizeof(RPC_API_GET_PHYS_MESSAGE), sizeof(RPC_API_GET_PHYS_MESSAGE), STRINGIZE(RPC_API_GET_PHYS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_PHYS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_CMD, 1, RPCFunc_Set, sizeof(RPC_API_SET_MESSAGE), sizeof(RPC_API_SET_MESSAGE), STRINGIZE(RPC_API_SET_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_PHYS_CMD, 1, RPCFunc_SetPhys, sizeof(RPC_API_SET_PHYS_MESSAGE), sizeof(RPC_API_SET_PHYS_MESSAGE), STRINGIZE(RPC_API_SET_PHYS_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_PHYS_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_EQU_CMD, 1, RPCFunc_Equ, sizeof(RPC_API_EQU_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_EQU_MESSAGE_MEMBERS), STRINGIZE(RPC_API_EQU_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_WR_VARI_ENABLE_CMD, 1, RPCFunc_WrVariEnable, sizeof(RPC_API_WR_VARI_ENABLE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_WR_VARI_ENABLE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_WR_VARI_ENABLE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_WR_VARI_DISABLE_CMD, 1, RPCFunc_WrVariDisable, sizeof(RPC_API_WR_VARI_DISABLE_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_WR_VARI_DISABLE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_WR_VARI_DISABLE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_IS_WR_VARI_ENABLED_CMD, 1, RPCFunc_IsWrVariEnsable, sizeof(RPC_API_IS_WR_VARI_ENABLED_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_IS_WR_VARI_ENABLED_MESSAGE_MEMBERS), STRINGIZE(RPC_API_IS_WR_VARI_ENABLED_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_LOAD_REF_LIST_CMD, 1, RPCFunc_LoadRefList, sizeof(RPC_API_LOAD_REF_LIST_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_LOAD_REF_LIST_MESSAGE_MEMBERS), STRINGIZE(RPC_API_LOAD_REF_LIST_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_ADD_REF_LIST_CMD, 1, RPCFunc_AddRefList, sizeof(RPC_API_ADD_REF_LIST_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_ADD_REF_LIST_MESSAGE_MEMBERS), STRINGIZE(RPC_API_ADD_REF_LIST_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SAVE_REF_LIST_CMD, 1, RPCFunc_SaveRefList, sizeof(RPC_API_SAVE_REF_LIST_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SAVE_REF_LIST_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SAVE_REF_LIST_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_VARI_CONVERSION_TYPE_CMD, 1, RPCFunc_GetVariConversionType, sizeof(RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE), sizeof(RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE), STRINGIZE(RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_VARI_CONVERSION_TYPE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_VARI_CONVERSION_STRING_CMD, 1, RPCFunc_GetVariConversionString, sizeof(RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE), sizeof(RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE), STRINGIZE(RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_VARI_CONVERSION_STRING_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_VARI_CONVERSION_CMD, 1, RPCFunc_SetVariConversion, sizeof(RPC_API_SET_VARI_CONVERSION_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SET_VARI_CONVERSION_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_VARI_CONVERSION_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_VARI_TYPE_CMD, 1, RPCFunc_GetVariType, sizeof(RPC_API_GET_VARI_TYPE_MESSAGE), sizeof(RPC_API_GET_VARI_TYPE_MESSAGE), STRINGIZE(RPC_API_GET_VARI_TYPE_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_VARI_TYPE_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_VARI_UNIT_CMD, 1, RPCFunc_GetVariUnit, sizeof(RPC_API_GET_VARI_UNIT_MESSAGE), sizeof(RPC_API_GET_VARI_UNIT_MESSAGE), STRINGIZE(RPC_API_GET_VARI_UNIT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_VARI_UNIT_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_VARI_UNIT_CMD, 1, RPCFunc_SetVariUnit, sizeof(RPC_API_SET_VARI_UNIT_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_SET_VARI_UNIT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_VARI_UNIT_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_VARI_MIN_CMD, 1, RPCFunc_GetVariMin, sizeof(RPC_API_GET_VARI_MIN_MESSAGE), sizeof(RPC_API_GET_VARI_MIN_MESSAGE), STRINGIZE(RPC_API_GET_VARI_MIN_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_VARI_MIN_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_VARI_MAX_CMD, 1, RPCFunc_GetVariMax, sizeof(RPC_API_GET_VARI_MAX_MESSAGE), sizeof(RPC_API_GET_VARI_MAX_MESSAGE), STRINGIZE(RPC_API_GET_VARI_MAX_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_VARI_MAX_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_VARI_MIN_CMD, 1, RPCFunc_SetVariMin, sizeof(RPC_API_SET_VARI_MIN_MESSAGE), sizeof(RPC_API_SET_VARI_MIN_MESSAGE), STRINGIZE(RPC_API_SET_VARI_MIN_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_VARI_MIN_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_VARI_MAX_CMD, 1, RPCFunc_SetVariMax, sizeof(RPC_API_SET_VARI_MAX_MESSAGE), sizeof(RPC_API_SET_VARI_MAX_MESSAGE), STRINGIZE(RPC_API_SET_VARI_MAX_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_VARI_MAX_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_NEXT_VARI_CMD, 1, RPCFunc_GetNextVari, sizeof(RPC_API_GET_NEXT_VARI_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_GET_NEXT_VARI_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_NEXT_VARI_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_NEXT_VARI_EX_CMD, 1, RPCFunc_GetNextVariEx, sizeof(RPC_API_GET_NEXT_VARI_EX_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_GET_NEXT_VARI_EX_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_NEXT_VARI_EX_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_VARI_ENUM_CMD, 1, RPCFunc_GetVariEnum, sizeof(RPC_API_GET_VARI_ENUM_MESSAGE), sizeof(RPC_API_GET_VARI_ENUM_MESSAGE), STRINGIZE(RPC_API_GET_VARI_ENUM_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_VARI_ENUM_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_CMD, 1, RPCFunc_GetVariDisplayFormatWidth, sizeof(RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE), sizeof(RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE), STRINGIZE(RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_VARI_DISPLAY_FORMAT_WIDTH_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_CMD, 1, RPCFunc_GetVariDisplayFormatPrec, sizeof(RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE), sizeof(RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE), STRINGIZE(RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_VARI_DISPLAY_FORMAT_PREC_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_VARI_DISPLAY_FORMAT_CMD, 1, RPCFunc_SetVariDisplayFormat, sizeof(RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE), sizeof(RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE), STRINGIZE(RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_VARI_DISPLAY_FORMAT_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_IMPORT_VARI_PROPERTIES_CMD, 1, RPCFunc_ImportVariProperties, sizeof(RPC_API_IMPORT_VARI_PROPERTIES_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_IMPORT_VARI_PROPERTIES_MESSAGE_MEMBERS), STRINGIZE(RPC_API_IMPORT_VARI_PROPERTIES_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_ENABLE_RANGE_CONTROL_CMD, 1, RPCFunc_EnableRangeControl, sizeof(RPC_API_ENABLE_RANGE_CONTROL_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_ENABLE_RANGE_CONTROL_MESSAGE_MEMBERS), STRINGIZE(RPC_API_ENABLE_RANGE_CONTROL_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_DISABLE_RANGE_CONTROL_CMD, 1, RPCFunc_DisableRangeControl, sizeof(RPC_API_DISABLE_RANGE_CONTROL_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_DISABLE_RANGE_CONTROL_MESSAGE_MEMBERS), STRINGIZE(RPC_API_DISABLE_RANGE_CONTROL_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_WRITE_FRAME_CMD, 1, RPCFunc_WriteFrame, sizeof(RPC_API_WRITE_FRAME_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_WRITE_FRAME_MESSAGE_MEMBERS), STRINGIZE(RPC_API_WRITE_FRAME_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_FRAME_CMD, 1, RPCFunc_GetFrame, sizeof(RPC_API_GET_FRAME_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_GET_FRAME_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_FRAME_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_WRITE_FRAME_WAIT_READ_FRAME_CMD, 0, RPCFunc_WriteFrameWaitReadFrame, sizeof(RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE_MEMBERS), STRINGIZE(RPC_API_WRITE_FRAME_WAIT_READ_FRAME_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_REFERENCE_SYMBOL_CMD, 0, RPCFunc_ReferenceSymbol, sizeof(RPC_API_REFERENCE_SYMBOL_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_REFERENCE_SYMBOL_MESSAGE_MEMBERS), STRINGIZE(RPC_API_REFERENCE_SYMBOL_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_DEREFERENCE_SYMBOL_CMD, 0, RPCFunc_DereferenceSymbol, sizeof(RPC_API_DEREFERENCE_SYMBOL_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_DEREFERENCE_SYMBOL_MESSAGE_MEMBERS), STRINGIZE(RPC_API_DEREFERENCE_SYMBOL_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_GET_RAW_CMD, 0, RPCFunc_GetRaw, sizeof(RPC_API_GET_RAW_MESSAGE), sizeof(RPC_API_GET_RAW_MESSAGE), STRINGIZE(RPC_API_GET_RAW_MESSAGE_MEMBERS), STRINGIZE(RPC_API_GET_RAW_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_SET_RAW_CMD, 0, RPCFunc_SetRaw, sizeof(RPC_API_SET_RAW_MESSAGE), sizeof(RPC_API_SET_RAW_MESSAGE), STRINGIZE(RPC_API_SET_RAW_MESSAGE_MEMBERS), STRINGIZE(RPC_API_SET_RAW_MESSAGE_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_EXPORT_A2L_MEASUREMENT_LIST_CMD, 0, RPCFunc_ExportA2lMeasurementList, sizeof(RPC_API_EXPORT_A2L_MEASUREMENT_LIST_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_EXPORT_A2L_MEASUREMENT_LIST_MESSAGE_MEMBERS), STRINGIZE(RPC_API_EXPORT_A2L_MEASUREMENT_LIST_ACK_MEMBERS));
    AddFunctionToRemoteAPIFunctionTable2(RPC_API_IMPORT_A2L_MEASUREMENT_LIST_CMD, 0, RPCFunc_ImportA2lMeasurementList, sizeof(RPC_API_IMPORT_A2L_MEASUREMENT_LIST_MESSAGE), RPC_API_MAX_MESSAGE_SIZE, STRINGIZE(RPC_API_IMPORT_A2L_MEASUREMENT_LIST_MESSAGE_MEMBERS), STRINGIZE(RPC_API_IMPORT_A2L_MEASUREMENT_LIST_ACK_MEMBERS));
    return 0;
}
