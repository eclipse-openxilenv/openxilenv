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


#include<stdio.h>
#ifdef _WIN32
#include<WinSock2.h>
#else
#endif

#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Scheduler.h"

#include "IniDataBase.h"
#include "Blackboard.h"
#include "BlackboardIniCache.h"
#include "EquationList.h"
#include "EquationParser.h"
#include "ExecutionStack.h"
#include "StructsRM_Blackboard.h"
#include "RemoteMasterNet.h"
#include "BlackboardIniCache.h"

#include "RemoteMasterBlackboard.h"

#define CHECK_ANSWER(Req,Ack)

#define UNUSED(x) (void)(x)

int rm_init_blackboard (int blackboard_size, char CopyBB2ProcOnlyIfWrFlagSet, char AllowBBVarsWithSameAddr, char conv_error2message)
{
    RM_BLACKBOARD_INIT_BLACKBOARD_REQ Req;
    RM_BLACKBOARD_INIT_BLACKBOARD_ACK Ack;

    Req.blackboard_size = blackboard_size;
    Req.CopyBB2ProcOnlyIfWrFlagSet = CopyBB2ProcOnlyIfWrFlagSet;
    Req.AllowBBVarsWithSameAddr = AllowBBVarsWithSameAddr;
    Req.conv_error2message = conv_error2message;
    TransactRemoteMaster (RM_BLACKBOARD_INIT_BLACKBOARD_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_close_blackboard (void)
{
    RM_BLACKBOARD_CLOSE_BLACKBOARD_REQ Req;
    RM_BLACKBOARD_CLOSE_BLACKBOARD_ACK Ack;
    TransactRemoteMaster (RM_BLACKBOARD_CLOSE_BLACKBOARD_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}


VID rm_add_bbvari_pid_type (const char *name, enum BB_DATA_TYPES type, const char *unit, int Pid, int Dir, int ValueValidFlag, union BB_VARI *Value, int *ret_Type,
                            uint32_t ReadReqMask)
{
    UNUSED(Dir);
    RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_REQ *Req;
    RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_ACK Ack;
    size_t LenName;
    size_t LenUnit;
    size_t SizeOfStruct;
    char *NameDst;
    char *UnitDst;

    if (Pid <= 0) {
        Pid = GET_PID();
    }

    LenName = strlen (name) + 1;
    if (unit == NULL) {
        LenUnit = 0;
    } else {
        LenUnit = strlen (unit) + 1;
    }
    SizeOfStruct = sizeof (RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_REQ) + LenName + LenUnit;
    Req = (RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_REQ*)_alloca (SizeOfStruct);
    NameDst = (char*)(Req + 1);
    MEMCPY (NameDst, name, LenName);
    Req->NameOffset = sizeof (RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_REQ);
    UnitDst = (char*)(Req + 1) + LenName;
    if (LenUnit) {
        MEMCPY (UnitDst, unit, LenUnit);
        Req->UnitOffset = (uint32_t)(sizeof (RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_REQ) + LenName);
    } else {
        Req->UnitOffset = 0;
    }
    Req->Type = (uint32_t)type;
    Req->Pid = Pid;
    Req->ValueValidFlag = ValueValidFlag;
    if (Value != NULL) {
        Req->ValueUnion = *(uint64_t*)Value;
    }
    Req->ReadReqMask = ReadReqMask;
    TransactRemoteMaster (RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_CMD, Req, (int)SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    if (ret_Type != NULL) {
        *ret_Type = Ack.RetType;
    }
    return Ack.Ret;
}

int rm_attach_bbvari (VID vid, int unknown_wait_flag, int pid)
{
    RM_BLACKBOARD_ATTACH_BBVARI_REQ Req;
    RM_BLACKBOARD_ATTACH_BBVARI_ACK Ack;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.unknown_wait_flag = unknown_wait_flag;
    TransactRemoteMaster (RM_BLACKBOARD_ATTACH_BBVARI_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

VID rm_attach_bbvari_by_name (const char *name, int pid)
{
    RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_REQ *Req;
    RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_ACK Ack;
    int LenName;
    int SizeOfStruct;

    LenName = (int)strlen (name) + 1;
    SizeOfStruct = (int)sizeof (RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_REQ) + LenName;
    Req = (RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_REQ*)_alloca ((size_t)SizeOfStruct);
    MEMCPY (Req + 1, name, (size_t)LenName);
    Req->NameOffset = sizeof (RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_REQ);
    Req->Pid = pid;
    TransactRemoteMaster (RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_remove_bbvari (VID vid, int unknown_wait_flag, int pid)
{
    RM_BLACKBOARD_REMOVE_BBVARI_REQ Req;
    RM_BLACKBOARD_REMOVE_BBVARI_ACK Ack;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.unknown_wait_flag = unknown_wait_flag;
    TransactRemoteMaster (RM_BLACKBOARD_REMOVE_BBVARI_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_remove_all_bbvari (PID pid)
{
    RM_BLACKBOARD_REMOVE_ALL_BBVARI_REQ Req;
    RM_BLACKBOARD_REMOVE_ALL_BBVARI_ACK Ack;

    Req.Pid = pid;
    TransactRemoteMaster (RM_BLACKBOARD_REMOVE_ALL_BBVARI_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

unsigned int rm_get_num_of_bbvaris (void)
{
    RM_BLACKBOARD_GET_NUM_OF_BBVARIS_REQ Req;
    RM_BLACKBOARD_GET_NUM_OF_BBVARIS_ACK Ack;

    TransactRemoteMaster (RM_BLACKBOARD_GET_NUM_OF_BBVARIS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

VID rm_get_bbvarivid_by_name (const char *name)
{
    RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_REQ *Req;
    RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_ACK Ack;
    int LenName;
    int SizeOfStruct;

    LenName = (int)strlen (name) + 1;
    SizeOfStruct = (int)sizeof (RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_REQ) + LenName;
    Req = (RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_REQ*)_alloca ((size_t)SizeOfStruct);
    MEMCPY (Req + 1, name, (size_t)LenName);
    Req->NameOffset = sizeof (RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_REQ);
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_get_bbvaritype_by_name (const char *name)
{
    RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_REQ *Req;
    RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_ACK Ack;
    int LenName;
    int SizeOfStruct;

    LenName = (int)strlen (name) + 1;
    SizeOfStruct = (int)sizeof (RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_REQ) + LenName;
    Req = (RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_REQ*)_alloca ((size_t)SizeOfStruct);
    MEMCPY (Req + 1, name, (size_t)LenName);
    Req->NameOffset = sizeof (RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_REQ);
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_get_bbvari_attachcount (VID vid)
{
    RM_BLACKBOARD_GET_BBVARI_ATTACHCOUNT_REQ Req;
    RM_BLACKBOARD_GET_BBVARI_ATTACHCOUNT_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_ATTACHCOUNT_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_get_bbvaritype (VID vid)
{
    RM_BLACKBOARD_GET_BBVARITYPE_REQ Req;
    RM_BLACKBOARD_GET_BBVARITYPE_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARITYPE_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_GetBlackboardVariableName (VID vid, char *txt, int maxc)
{
    RM_BLACKBOARD_GET_BBVARI_NAME_REQ Req;
    struct {
        RM_BLACKBOARD_GET_BBVARI_NAME_ACK Ack;
        char Name[BBVARI_NAME_SIZE];
    } Ack;

    if (maxc > BBVARI_NAME_SIZE) maxc = BBVARI_NAME_SIZE;
    Req.Vid = vid;
    Req.MaxChar = maxc;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_NAME_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    StringCopyMaxCharTruncate(txt, Ack.Name, maxc);
    return Ack.Ack.Ret;
}

int rm_GetBlackboardVariableNameAndTypes (VID vid, char *txt, int maxc, enum BB_DATA_TYPES *ret_data_type, enum BB_CONV_TYPES *ret_conversion_type)
{
    RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_REQ Req;
    struct {
        RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_ACK Ack;
        char Name[BBVARI_NAME_SIZE];
    } Ack;

    Req.Vid = vid;
    if (maxc > BBVARI_NAME_SIZE) maxc = BBVARI_NAME_SIZE;
    Req.MaxChar = maxc;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    StringCopyMaxCharTruncate (txt, Ack.Name, maxc);
    *ret_data_type = (enum BB_DATA_TYPES)Ack.Ack.data_type;
    *ret_conversion_type = (enum BB_CONV_TYPES)Ack.Ack.conversion_type;
    return Ack.Ack.Ret;
}

int rm_get_process_bbvari_attach_count_pid (VID vid, PID pid)
{
    RM_BLACKBOARD_GET_PROCESS_BBVARI_ATTACH_COUNT_REQ Req;
    RM_BLACKBOARD_GET_PROCESS_BBVARI_ATTACH_COUNT_ACK Ack;

    Req.Vid = vid;
    Req.Pid = pid;
    TransactRemoteMaster (RM_BLACKBOARD_GET_PROCESS_BBVARI_ATTACH_COUNT_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_set_bbvari_unit (VID vid, const char *unit)
{
    RM_BLACKBOARD_SET_BBVARI_UNIT_REQ *Req;
    RM_BLACKBOARD_SET_BBVARI_UNIT_ACK Ack;
    int LenUnit;
    int SizeOfStruct;

    LenUnit = (int)strlen (unit) + 1;
    SizeOfStruct = (int)sizeof (RM_BLACKBOARD_SET_BBVARI_UNIT_REQ) + LenUnit;
    Req = (RM_BLACKBOARD_SET_BBVARI_UNIT_REQ*)_alloca ((size_t)SizeOfStruct);
    Req->Vid = vid;
    MEMCPY (Req + 1, unit, (size_t)LenUnit);
    Req->UnitOffset = sizeof (RM_BLACKBOARD_SET_BBVARI_UNIT_REQ);
    TransactRemoteMaster (RM_BLACKBOARD_SET_BBVARI_UNIT_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_get_bbvari_unit (VID vid, char *unit, int maxc)
{
    RM_BLACKBOARD_GET_BBVARI_UNIT_REQ Req;
    struct {
        RM_BLACKBOARD_GET_BBVARI_UNIT_ACK Ack;
        char Unit[BBVARI_UNIT_SIZE];
    } Ack;

    Req.Vid = vid;
    Req.max_c = maxc;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_UNIT_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    strcpy (unit, Ack.Unit);
    return Ack.Ack.Ret;
}

int rm_set_bbvari_conversion (VID vid, int convtype, const char *conversion)
{
    RM_BLACKBOARD_SET_BBVARI_CONVERSION_REQ *Req;
    RM_BLACKBOARD_SET_BBVARI_CONVERSION_ACK Ack;
    int LenConversion;
    int SizeOfStruct;
    struct EXEC_STACK_ELEM *ExecStack = NULL;
    int SizeOfExecStack;

    if (convtype == BB_CONV_FORMULA) {
        if ((ExecStack = solve_equation_replace_parameter (conversion)) == NULL) {
            return -1;
        }
        SizeOfExecStack = sizeof_exec_stack (ExecStack);
    } else {
        SizeOfExecStack = 0;
    }

    LenConversion = (int)strlen (conversion) + 1;
    SizeOfStruct = (int)sizeof (RM_BLACKBOARD_SET_BBVARI_CONVERSION_REQ) + LenConversion + SizeOfExecStack;
    Req = (RM_BLACKBOARD_SET_BBVARI_CONVERSION_REQ*)_alloca ((size_t)SizeOfStruct);
    Req->Vid = vid;
    Req->ConversionType = convtype;
    MEMCPY (Req + 1, conversion, (size_t)LenConversion);
    Req->ConversionOffset = sizeof (RM_BLACKBOARD_SET_BBVARI_CONVERSION_REQ);

    if (convtype == BB_CONV_FORMULA) {
        char Name[BBVARI_NAME_SIZE];
        if (GetBlackboardVariableName(vid, Name, sizeof(Name))) {
            StringAppendMaxCharTruncate (Name, "unknown", sizeof(Name));
        }
        MEMCPY (((char*)(Req + 1) + LenConversion), ExecStack, (size_t)SizeOfExecStack);
        RegisterEquation (0, conversion, ExecStack,
                         Name, EQU_TYPE_BLACKBOARD);
        Req->sizeof_bin_conversion = SizeOfExecStack;
        Req->BinConversionOffset = (uint32_t)sizeof (RM_BLACKBOARD_SET_BBVARI_CONVERSION_REQ) + (uint32_t)LenConversion;
    } else {
        Req->sizeof_bin_conversion = 0;
        Req->BinConversionOffset = 0;
    }
    TransactRemoteMaster (RM_BLACKBOARD_SET_BBVARI_CONVERSION_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_get_bbvari_conversion (VID vid, char *conversion, int maxc)
{
    RM_BLACKBOARD_GET_BBVARI_CONVERSION_REQ Req;
    RM_BLACKBOARD_GET_BBVARI_CONVERSION_ACK *Ack;
    size_t SizeOfStruct;

    SizeOfStruct = sizeof (RM_BLACKBOARD_GET_BBVARI_CONVERSION_ACK) + (size_t)maxc + 1;  // remark: +1 more than requested
    Req.Vid = vid;
    Req.maxc = maxc;
    Ack = _alloca (SizeOfStruct);
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_CONVERSION_CMD, &Req, sizeof(Req), Ack, (int)SizeOfStruct);
    CHECK_ANSWER(Req, Ack);
    if (conversion != NULL) strcpy (conversion, (char*)(Ack + 1));
    return Ack->Ret;
}

int rm_get_bbvari_conversiontype (VID vid)
{
    RM_BLACKBOARD_GET_BBVARI_CONVERSIONTYPE_REQ Req;
    RM_BLACKBOARD_GET_BBVARI_CONVERSIONTYPE_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_CONVERSIONTYPE_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_get_bbvari_infos (VID par_Vid, BB_VARIABLE *ret_BaseInfos, BB_VARIABLE_ADDITIONAL_INFOS *ret_AdditionalInfos, char *ret_Buffer, int par_SizeOfBuffer)
{
    RM_BLACKBOARD_GET_BBVARI_INFOS_REQ Req;
    RM_BLACKBOARD_GET_BBVARI_INFOS_ACK *Ack;
    int SizeOfStruct;
    char *Conversion;

    // olny for debugging
    //SizeOfStruct = sizeof (RM_BLACKBOARD_GET_BBVARI_INFOS_ACK);

    SizeOfStruct = (int)sizeof (RM_BLACKBOARD_GET_BBVARI_INFOS_ACK) + par_SizeOfBuffer;
    Ack = (RM_BLACKBOARD_GET_BBVARI_INFOS_ACK*)_alloca((size_t)SizeOfStruct);
    Req.Vid = par_Vid;
    Req.SizeOfBuffer = par_SizeOfBuffer;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_INFOS_CMD, &Req, sizeof(Req), Ack, SizeOfStruct);
    CHECK_ANSWER(Req, Ack_1);
    if (Ack->Ret == 0) {
        *ret_BaseInfos = Ack->BaseInfos;
        *ret_AdditionalInfos = Ack->AdditionalInfos;
        MEMCPY (ret_Buffer, Ack + 1, Ack->PackageHeader.SizeOf - sizeof (RM_BLACKBOARD_GET_BBVARI_INFOS_ACK));
        ret_BaseInfos->pAdditionalInfos = ret_AdditionalInfos;
        ret_AdditionalInfos->Name = ret_Buffer + (Ack->OffsetName - sizeof(RM_BLACKBOARD_GET_BBVARI_INFOS_ACK));
        if (Ack->OffsetUnit) {
            ret_AdditionalInfos->Unit = ret_Buffer + (Ack->OffsetUnit - sizeof(RM_BLACKBOARD_GET_BBVARI_INFOS_ACK));
        } else {
            ret_AdditionalInfos->Unit = NULL;
        }
        switch (ret_AdditionalInfos->Conversion.Type) {
        //default:
        case BB_CONV_NONE:
        case BB_CONV_FACTOFF:
            break;
        case BB_CONV_FORMULA:
            ret_AdditionalInfos->Conversion.Conv.Formula.FormulaString = Conversion = ret_Buffer + (Ack->OffsetConversion - sizeof (RM_BLACKBOARD_GET_BBVARI_INFOS_ACK));
            break;
        case BB_CONV_TEXTREP:
            ret_AdditionalInfos->Conversion.Conv.TextReplace.EnumString = Conversion = ret_Buffer + (Ack->OffsetConversion - sizeof (RM_BLACKBOARD_GET_BBVARI_INFOS_ACK));
            break;
        case BB_CONV_REF:
            ret_AdditionalInfos->Conversion.Conv.Reference.Name = Conversion = ret_Buffer + (Ack->OffsetConversion - sizeof (RM_BLACKBOARD_GET_BBVARI_INFOS_ACK));
            break;
        }
        if (Ack->OffsetDisplayName) {
            ret_AdditionalInfos->DisplayName = ret_Buffer + (Ack->OffsetDisplayName - sizeof (RM_BLACKBOARD_GET_BBVARI_INFOS_ACK));
        } else {
            ret_AdditionalInfos->DisplayName = NULL;
        }
        if (Ack->OffsetComment) {
            ret_AdditionalInfos->Comment = ret_Buffer + (Ack->OffsetComment - sizeof (RM_BLACKBOARD_GET_BBVARI_INFOS_ACK));
        } else {
            ret_AdditionalInfos->Comment = NULL;
        }
    }
    return Ack->Ret;

}

int rm_set_bbvari_color (VID vid, int rgb_color)
{
    RM_BLACKBOARD_SET_BBVARI_COLOR_REQ Req;
    RM_BLACKBOARD_SET_BBVARI_COLOR_ACK Ack;

    Req.Vid = vid;
    Req.rgb_color = rgb_color;
    TransactRemoteMaster (RM_BLACKBOARD_SET_BBVARI_COLOR_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_get_bbvari_color (VID vid)
{
    RM_BLACKBOARD_GET_BBVARI_COLOR_REQ Req;
    RM_BLACKBOARD_GET_BBVARI_COLOR_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_COLOR_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.rgb_color;
}

int rm_set_bbvari_step (VID vid, unsigned char steptype, double step)
{
    RM_BLACKBOARD_SET_BBVARI_STEP_REQ Req;
    RM_BLACKBOARD_SET_BBVARI_STEP_ACK Ack;

    Req.Vid = vid;
    Req.steptype = steptype;
    Req.step = step;
    TransactRemoteMaster (RM_BLACKBOARD_SET_BBVARI_STEP_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_get_bbvari_step (VID vid, unsigned char *steptype, double *step)
{
    RM_BLACKBOARD_GET_BBVARI_STEP_REQ Req;
    RM_BLACKBOARD_GET_BBVARI_STEP_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_STEP_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    *steptype = Ack.steptype;
    *step = Ack.step;
    return Ack.Ret;
}

int rm_set_bbvari_min (VID vid, double min)
{
    RM_BLACKBOARD_SET_BBVARI_MIN_REQ Req;
    RM_BLACKBOARD_SET_BBVARI_MIN_ACK Ack;

    Req.Vid = vid;
    Req.min = min;
    TransactRemoteMaster (RM_BLACKBOARD_SET_BBVARI_MIN_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_set_bbvari_max (VID vid, double max)
{
    RM_BLACKBOARD_SET_BBVARI_MAX_REQ Req;
    RM_BLACKBOARD_SET_BBVARI_MAX_ACK Ack;

    Req.Vid = vid;
    Req.max = max;
    TransactRemoteMaster (RM_BLACKBOARD_SET_BBVARI_MAX_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

double rm_get_bbvari_min (VID vid)
{
    RM_BLACKBOARD_GET_BBVARI_MIN_REQ Req;
    RM_BLACKBOARD_GET_BBVARI_MIN_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_MIN_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.min;
}

double rm_get_bbvari_max (VID vid)
{
    RM_BLACKBOARD_GET_BBVARI_MAX_REQ Req;
    RM_BLACKBOARD_GET_BBVARI_MAX_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_MAX_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.max;
}

int rm_set_bbvari_format (VID vid, int width, int prec)
{
    RM_BLACKBOARD_SET_BBVARI_FORMAT_REQ Req;
    RM_BLACKBOARD_SET_BBVARI_FORMAT_ACK Ack;

    Req.Vid = vid;
    Req.width = width;
    Req.prec = prec;
    TransactRemoteMaster (RM_BLACKBOARD_SET_BBVARI_FORMAT_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_get_bbvari_format_width (VID vid)
{
    RM_BLACKBOARD_GET_BBVARI_FORMAT_WIDTH_REQ Req;
    RM_BLACKBOARD_GET_BBVARI_FORMAT_WIDTH_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_FORMAT_WIDTH_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.width;
}

int rm_get_bbvari_format_prec (VID vid)
{
    RM_BLACKBOARD_GET_BBVARI_FORMAT_PREC_REQ Req;
    RM_BLACKBOARD_GET_BBVARI_FORMAT_PREC_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_GET_BBVARI_FORMAT_PREC_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.prec;
}

int rm_get_free_wrflag (PID pid, uint64_t *wrflag)
{
    RM_BLACKBOARD_GET_FREE_WRFLAG_REQ Req;
    RM_BLACKBOARD_GET_FREE_WRFLAG_ACK Ack;

    Req.Pid = pid;
    TransactRemoteMaster (RM_BLACKBOARD_GET_FREE_WRFLAG_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    *wrflag = Ack.wrflag;
    return Ack.Ret;
}

void rm_reset_wrflag (VID vid, uint64_t w)
{
    RM_BLACKBOARD_RESET_WRFLAG_REQ Req;
    RM_BLACKBOARD_RESET_WRFLAG_ACK Ack;

    Req.Vid = vid;
    Req.w = w;
    TransactRemoteMaster (RM_BLACKBOARD_RESET_WRFLAG_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
}

int rm_test_wrflag (VID vid, uint64_t w)
{
    RM_BLACKBOARD_TEST_WRFLAG_REQ Req;
    RM_BLACKBOARD_TEST_WRFLAG_ACK Ack;

    Req.Vid = vid;
    Req.w = w;
    TransactRemoteMaster (RM_BLACKBOARD_TEST_WRFLAG_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_read_next_blackboard_vari (int index, char *ret_NameBuffer, int max_c)
{
    RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_REQ Req;
    RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_ACK *Ack;
    size_t SizeOfStruct = sizeof(RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_ACK) + (size_t)max_c;

    Ack = (RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_ACK*)_alloca(SizeOfStruct);
    Req.index = index;
    Req.max_c = max_c;
    TransactRemoteMaster (RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_CMD, &Req, sizeof(Req), Ack, (int)SizeOfStruct);
    CHECK_ANSWER(Req, Ack);
    strcpy (ret_NameBuffer, (char*)(Ack + 1));
    return Ack->Ret;
}

char *rm_read_next_blackboard_vari_old (int flag)
{
    UNUSED(flag);
    ThrowError (1, "don't call \"rm_read_next_blackboard_vari_old()\"");
    return "error";
}

int rm_ReadNextVariableProcessAccess (int index, PID pid, int access, char *name, int maxc)
{
    RM_BLACKBOARD_READ_NEXT_VARI_PROCESS_ACCESS_REQ Req;
    struct {
        RM_BLACKBOARD_READ_NEXT_VARI_PROCESS_ACCESS_ACK Ack;
        char Name[BBVARI_NAME_SIZE];
    } Ack;

    if (maxc > BBVARI_NAME_SIZE) maxc = BBVARI_NAME_SIZE;
    Req.Pid = pid;
    Req.access = access;
    Req.index = index;
    Req.maxc = maxc;
    TransactRemoteMaster (RM_BLACKBOARD_READ_NEXT_VARI_PROCESS_ACCESS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    StringCopyMaxCharTruncate(name, Ack.Name, maxc);
    return Ack.Ack.Ret;
}

int rm_enable_bbvari_access (PID pid, VID vid)
{
    RM_BLACKBOARD_ENABLE_BBVARI_ACCESS_REQ Req;
    RM_BLACKBOARD_ENABLE_BBVARI_ACCESS_ACK Ack;

    Req.Pid = pid;
    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_ENABLE_BBVARI_ACCESS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_disable_bbvari_access (PID pid, VID vid)
{
    RM_BLACKBOARD_DISABLE_BBVARI_ACCESS_REQ Req;
    RM_BLACKBOARD_DISABLE_BBVARI_ACCESS_ACK Ack;

    Req.Pid = pid;
    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_DISABLE_BBVARI_ACCESS_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

static void (*ObserationCallbackFunction) (VID Vid, uint32_t ObservationFlags, uint32_t ObservationData);

void rm_SetObserationCallbackFunction (void (*Callback)(VID Vid, uint32_t ObservationFlags, uint32_t ObservationData))
{
    RM_BLACKBOARD_SET_OBSERVATION_CALLBACK_FUNCTION_REQ Req;
    RM_BLACKBOARD_SET_OBSERVATION_CALLBACK_FUNCTION_ACK Ack;

    ObserationCallbackFunction = Callback;
    Req.Callback = (uint64_t)Callback;
    TransactRemoteMaster (RM_BLACKBOARD_SET_OBSERVATION_CALLBACK_FUNCTION_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
}

int rm_SetBbvariObservation (VID Vid, uint32_t ObservationFlags, uint32_t ObservationData)
{
    RM_BLACKBOARD_SET_BBVARI_OBSERVATION_REQ Req;
    RM_BLACKBOARD_SET_BBVARI_OBSERVATION_ACK Ack;

    Req.Vid = Vid;
    Req.ObservationFlags = ObservationFlags;
    Req.ObservationData = ObservationData;
    TransactRemoteMaster (RM_BLACKBOARD_SET_BBVARI_OBSERVATION_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_SetGlobalBlackboradObservation (uint32_t ObservationFlags, uint32_t ObservationData, int **ret_Vids, int *ret_Count)
{
    RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_REQ Req;
    RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_ACK Ack;
    int *loc_Vids;

    Req.ObservationFlags = ObservationFlags;
    Req.ObservationData = ObservationData;
    int BufferSize;
    TransactRemoteMasterDynBuf (RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack), (void**)&loc_Vids, &BufferSize);
    CHECK_ANSWER(Req, Ack);
    if (ret_Count != NULL) *ret_Count = Ack.ret_Count;
    if (ret_Vids != NULL) *ret_Vids = loc_Vids;
    return (int)Ack.Ret;
}

int rm_get_bb_accessmask (PID pid, uint64_t *mask, char *BBPrefix)
{
    RM_BLACKBOARD_GET_BB_ACCESSMASK_REQ *Req;
    RM_BLACKBOARD_GET_BB_ACCESSMASK_ACK Ack;
    int LenBBPrefix;
    int SizeOfStruct;

    if (BBPrefix == NULL) {
        LenBBPrefix = 0;
    } else {
        LenBBPrefix = (int)strlen (BBPrefix) + 1;
    }
    SizeOfStruct = (int)sizeof (RM_BLACKBOARD_GET_BB_ACCESSMASK_REQ) + LenBBPrefix;
    Req = (RM_BLACKBOARD_GET_BB_ACCESSMASK_REQ*)_alloca ((size_t)SizeOfStruct);
    Req->pid = pid;
    if (LenBBPrefix) {
        MEMCPY (Req + 1, BBPrefix, (size_t)LenBBPrefix);
        Req->BBPrefixOffset = sizeof (RM_BLACKBOARD_GET_BB_ACCESSMASK_REQ);
    } else {
        Req->BBPrefixOffset = 0;
    }
    TransactRemoteMaster (RM_BLACKBOARD_GET_BB_ACCESSMASK_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    *mask = Ack.ret_mask;
    return Ack.Ret;
}

int rm_WriteToBlackboardIniCache(BB_VARIABLE *sp_vari_elem,
                                 uint32_t WriteReqFlags)
{
    RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ *Req;
    RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_ACK Ack;
    size_t LenVariableName, LenUnit, LenConversion;
    size_t SizeOfStruct;

    if ((sp_vari_elem->pAdditionalInfos->Name == NULL) || ((WriteReqFlags & INI_CACHE_CLEAR) != INI_CACHE_CLEAR)) {
        LenVariableName = 0;
    } else  {
        LenVariableName = strlen(sp_vari_elem->pAdditionalInfos->Name) + 1;
    }
    if ((sp_vari_elem->pAdditionalInfos->Unit == NULL) || ((WriteReqFlags & INI_CACHE_ENTRY_FLAG_UNIT) != INI_CACHE_ENTRY_FLAG_UNIT)) {
        LenUnit = 0;
    } else  {
        LenUnit = strlen(sp_vari_elem->pAdditionalInfos->Unit) + 1;
    }
    if ((sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString == NULL) || ((WriteReqFlags & INI_CACHE_ENTRY_FLAG_CONVERSION) != INI_CACHE_ENTRY_FLAG_CONVERSION)) {
        LenConversion = 0;
    } else  {
        LenConversion = strlen(sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString) + 1;
    }
    SizeOfStruct = sizeof(RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ) + LenVariableName + LenUnit + LenConversion;
    Req = (RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ*)_alloca(SizeOfStruct);

    if (LenUnit > 0) {
        Req->VariableNameOffset = sizeof (RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ);
        MEMCPY ((char*)Req + Req->VariableNameOffset, sp_vari_elem->pAdditionalInfos->Name, LenVariableName);
    } else Req->VariableNameOffset = 0;
    if (LenUnit > 0) {
        Req->UnitOffset = (uint32_t)(sizeof (RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ) + LenVariableName);
        MEMCPY ((char*)Req + Req->UnitOffset, sp_vari_elem->pAdditionalInfos->Unit, LenUnit);
    } else Req->UnitOffset =  0;
    if (LenConversion > 0) {
        Req->ConversionOffset = (uint32_t)(sizeof (RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ) + LenVariableName + LenUnit);
        MEMCPY ((char*)Req + Req->ConversionOffset, sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString, LenConversion);
    } else Req->ConversionOffset =0;
    Req->Min = sp_vari_elem->pAdditionalInfos->Min;
    Req->Max = sp_vari_elem->pAdditionalInfos->Max;
    Req->Step = sp_vari_elem->pAdditionalInfos->Step;
    Req->Width = sp_vari_elem->pAdditionalInfos->Width;
    Req->Prec = sp_vari_elem->pAdditionalInfos->Prec;
    Req->StepType = sp_vari_elem->pAdditionalInfos->StepType;
    Req->ConversionType = (uint8_t)sp_vari_elem->pAdditionalInfos->Conversion.Type;
    Req->Flags = WriteReqFlags;
    TransactRemoteMaster (RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_CMD, Req, (int)SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    if (Ack.Ret != 0) {
        ThrowError (1, "cannot init INI variable section cache");
        return -1;
    }
    return Ack.Ret;
}

int rm_InitVariableSectionCache (void)
{
    int IniIndex;
    RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ *Req;
    RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_ACK Ack;
    int LenVariableName, LenUnit, LenConversion;
    char *VariableName, *Line;
    int SizeOfStruct;
    int MaxSizeOfStruct = sizeof(RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ) + INI_MAX_LONGLINE_LENGTH;
    int Fd = GetMainFileDescriptor();

    // 128KByte are the max size of an INI file line
    Req = (RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ*)my_malloc((size_t)MaxSizeOfStruct);
    Line = my_malloc((size_t)INI_MAX_LONGLINE_LENGTH);
    if ((Req == NULL) || (Line == NULL)) {
        ThrowError (1, "out of memory");
        return -1;
    }

    // Clear the cache
    memset(Req, 0, sizeof(RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ));
    Req->Flags = INI_CACHE_CLEAR;
    SizeOfStruct = (int)sizeof(RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ);
    TransactRemoteMaster (RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    if (Ack.Ret != 0) {
        ThrowError (1, "cannot clear INI variable section cache");
        return -1;
    }

    IniIndex = 0;
    do {
        Req->VariableNameOffset = sizeof (RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ);
        VariableName = (char*)Req + Req->VariableNameOffset;
        IniIndex = IniFileDataBaseGetNextEntryName(IniIndex, VARI_SECTION, VariableName, 512, Fd);
        if (IniIndex >= 0) {
            BB_VARIABLE BbVariElem;
            BB_VARIABLE_ADDITIONAL_INFOS AdditionalInfos;
            LenVariableName = (int)strlen (VariableName) + 1;

            if (IniFileDataBaseReadString(VARI_SECTION, VariableName, "", Line,
                                          INI_MAX_LONGLINE_LENGTH,
                                          Fd) <= 0) {
                ThrowError (1, "cannot read blackboard INI entry to cache for variable %s", VariableName);
                continue;
            }
            memset (&BbVariElem, 0, sizeof (BbVariElem));
            memset (&AdditionalInfos, 0, sizeof (AdditionalInfos));
            BbVariElem.pAdditionalInfos = &AdditionalInfos;
            set_default_varinfo (&BbVariElem, BB_UNKNOWN);
            set_varinfo_to_inientrys (&BbVariElem, Line);

            if (BbVariElem.pAdditionalInfos->Unit == NULL) {
                Req->UnitOffset = 0;
                LenUnit = 0;
            } else  {
                Req->UnitOffset = (uint32_t)sizeof(RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ) + (uint32_t)LenVariableName;
                LenUnit = (int)strlen(BbVariElem.pAdditionalInfos->Unit) + 1;
                MEMCPY((char*)Req + Req->UnitOffset, BbVariElem.pAdditionalInfos->Unit, (size_t)LenUnit);
                my_free(BbVariElem.pAdditionalInfos->Unit);
            }
            if (BbVariElem.pAdditionalInfos->Conversion.Conv.Formula.FormulaString == NULL) {
                Req->ConversionOffset = 0;
                LenConversion = 0;
            } else  {
                Req->ConversionOffset = (uint32_t)sizeof(RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ) + (uint32_t)LenVariableName + (uint32_t)LenUnit;
                LenConversion = (int)strlen(BbVariElem.pAdditionalInfos->Conversion.Conv.Formula.FormulaString) + 1;
                MEMCPY((char*)Req + Req->ConversionOffset, BbVariElem.pAdditionalInfos->Conversion.Conv.Formula.FormulaString, (size_t)LenConversion);
                my_free(BbVariElem.pAdditionalInfos->Conversion.Conv.Formula.FormulaString);
            }
            Req->Min = BbVariElem.pAdditionalInfos->Min;
            Req->Max = BbVariElem.pAdditionalInfos->Max;
            Req->Step = BbVariElem.pAdditionalInfos->Step;
            Req->Width = BbVariElem.pAdditionalInfos->Width;
            Req->Prec = BbVariElem.pAdditionalInfos->Prec;
            Req->StepType = BbVariElem.pAdditionalInfos->StepType;
            Req->ConversionType = (uint8_t)BbVariElem.pAdditionalInfos->Conversion.Type;
            Req->Flags =INI_CACHE_ENTRY_FLAG_ALL_INFOS;
            SizeOfStruct = (int)sizeof(RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ) + LenVariableName + LenUnit + LenConversion;
            TransactRemoteMaster (RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_CMD, Req, SizeOfStruct, &Ack, sizeof(Ack));
            CHECK_ANSWER(Req, Ack);
            if (Ack.Ret != 0) {
                ThrowError (1, "cannot init INI variable section cache");
                return -1;
            }
        }
    } while (IniIndex >= 0);
    my_free(Req);
    my_free(Line);
    return 0;
}

int rm_WriteBackVariableSectionCache (void)
{
    RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_REQ Req;
    RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_ACK *Ack;
    int Index;
    char *Line;
    int SizeOfStruct = sizeof(RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_ACK) + INI_MAX_LONGLINE_LENGTH;
    int Ret = 0;
    int Fd = GetMainFileDescriptor();

    // 128KByte are the max size of an INI file line
    Ack = (RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_ACK*)my_malloc((size_t)SizeOfStruct);
    Line = my_malloc((size_t)INI_MAX_LONGLINE_LENGTH);

    IniFileDataBaseWriteString(VARI_SECTION, NULL, NULL, Fd);
    Index = 0;
    do {
        Req.Index = Index;
        TransactRemoteMaster (RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_CMD, &Req, sizeof(Req), Ack, SizeOfStruct);
        CHECK_ANSWER(Req, Ack);
        if (Ack->Ret >= 0) {
            char *Name, *Unit, *Converstion;
            size_t len_convertion;
            Name = (char*)Ack + Ack->VariableNameOffset;
            if (Ack->UnitOffset <= 0) {
                Unit = "";
            } else {
                Unit = (char*)Ack + Ack->UnitOffset;
            }
            if (Ack->ConversionOffset <= 0) {
                Converstion = "";
            } else {
                Converstion = (char*)Ack + Ack->ConversionOffset;
            }
            int len_first_part = sprintf (Line,
                                          "%d,%s,%.8g,%.8g,%d,%d,%d,%lf,%d,",
                                          (int)BB_UNKNOWN, Unit,
                                          Ack->Min,
                                          Ack->Max,
                                          (int)Ack->Width, (int)Ack->Prec,
                                          (int)Ack->StepType, Ack->Step,
                                          (int)Ack->ConversionType);

            if (((Ack->ConversionType != BB_CONV_FORMULA) &&
                 (Ack->ConversionType != BB_CONV_TEXTREP) &&
                 (Ack->ConversionType != BB_CONV_REF)) ||
                (Converstion == NULL)) {
                len_convertion = 0;
            } else {
                len_convertion = strlen (Converstion);
            }
            if (len_convertion > 0) {
                MEMCPY (Line + len_first_part,
                        Converstion,
                        len_convertion);
            }
            sprintf (Line + len_first_part + len_convertion, ",(%d,%d,%d)",
                     GetRValue(Ack->RgbColor),
                     GetGValue(Ack->RgbColor),
                     GetBValue(Ack->RgbColor));

            // Write string into INI file
            if (!IniFileDataBaseWriteString (VARI_SECTION,
                                              Name,
                                              Line,
                                              Fd)) {
                Ret |= -1;
            }
        }
        Index = Ack->Ret;
    } while (Index >= 0);
    my_free (Ack);
    my_free (Line);
    return Ret;
}


// Write/Read functions

void rm_write_bbvari_byte (PID pid, VID vid, int8_t v)
{
    RM_BLACKBOARD_WRITE_BBVARI_BYTE_REQ Req;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_BYTE_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}

void rm_write_bbvari_ubyte (PID pid, VID vid, uint8_t v)
{
    RM_BLACKBOARD_WRITE_BBVARI_UBYTE_REQ Req;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_UBYTE_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}


void rm_write_bbvari_word (PID pid, VID vid, int16_t v)
{
    RM_BLACKBOARD_WRITE_BBVARI_WORD_REQ Req;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_WORD_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}

void rm_write_bbvari_uword (PID pid, VID vid, uint16_t v)
{
    RM_BLACKBOARD_WRITE_BBVARI_UWORD_REQ Req;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_UWORD_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}

void rm_write_bbvari_dword (PID pid, VID vid, int32_t v)
{
    RM_BLACKBOARD_WRITE_BBVARI_DWORD_REQ Req;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_DWORD_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}

void rm_write_bbvari_udword (PID pid, VID vid, uint32_t v)
{
    RM_BLACKBOARD_WRITE_BBVARI_UDWORD_REQ Req;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_UDWORD_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}

void rm_write_bbvari_qword (PID pid, VID vid, int64_t v)
{
    RM_BLACKBOARD_WRITE_BBVARI_QWORD_REQ Req;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_QWORD_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}

void rm_write_bbvari_uqword (PID pid, VID vid, uint64_t v)
{
    RM_BLACKBOARD_WRITE_BBVARI_UQWORD_REQ Req;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_UQWORD_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}

void rm_write_bbvari_float (PID pid, VID vid, float v)
{
    RM_BLACKBOARD_WRITE_BBVARI_FLOAT_REQ Req;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_FLOAT_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}

void rm_write_bbvari_double (PID pid, VID vid, double v)
{
    RM_BLACKBOARD_WRITE_BBVARI_DOUBLE_REQ Req;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_DOUBLE_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}

void rm_write_bbvari_union (PID pid, VID vid, union BB_VARI v)
{
    RM_BLACKBOARD_WRITE_BBVARI_UNION_REQ Req;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_UNION_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}

void rm_write_bbvari_union_pid (PID Pid, VID vid, int DataType, union BB_VARI v)
{
    RM_BLACKBOARD_WRITE_BBVARI_UNION_PID_REQ Req;

    Req.Pid = Pid;
    Req.Vid = vid;
    Req.DataType = DataType;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_UNION_PID_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}

void rm_write_bbvari_minmax_check (PID Pid, VID vid, double v)
{
    RM_BLACKBOARD_WRITE_BBVARI_MIN_MAX_CHECK_REQ Req;

    Req.Pid = Pid;
    Req.Vid = vid;
    Req.Value = v;
    TransmitToRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_MIN_MAX_CHECK_CMD, &Req, sizeof(Req));
    CHECK_ANSWER(Req, Ack);
}

int rm_write_bbvari_convert_to (PID pid, VID vid, int convert_from_type, uint64_t *ret_Ptr)
{
    RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_REQ Req;
    RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_ACK Ack;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.convert_from_type = convert_from_type;
    Req.Value = *ret_Ptr;
    TransactRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_write_bbvari_phys_minmax_check (PID pid, VID vid, double new_phys_value)
{
    RM_BLACKBOARD_WRITE_BBVARI_PHYS_MINMAX_CHECK_REQ Req;
    RM_BLACKBOARD_WRITE_BBVARI_PHYS_MINMAX_CHECK_ACK Ack;

    Req.Pid = pid;
    Req.Vid = vid;
    Req.Value = new_phys_value;
    TransactRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_PHYS_MINMAX_CHECK_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_get_phys_value_for_raw_value (VID vid, double raw_value, double *ret_phys_value)
{
    RM_BLACKBOARD_GET_PHYS_VALUE_FOR_RAW_VALUE_REQ Req;
    RM_BLACKBOARD_GET_PHYS_VALUE_FOR_RAW_VALUE_ACK Ack;

    Req.Vid = vid;
    Req.raw_value = raw_value;
    TransactRemoteMaster (RM_BLACKBOARD_GET_PHYS_VALUE_FOR_RAW_VALUE_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    *ret_phys_value = Ack.ret_phys_value;
    return Ack.Ret;
}

int rm_get_raw_value_for_phys_value (VID vid, double phys_value, double *ret_raw_value, double *ret_phys_value)
{
    RM_BLACKBOARD_GET_RAW_VALUE_FOR_PHYS_VALUE_REQ Req;
    RM_BLACKBOARD_GET_RAW_VALUE_FOR_PHYS_VALUE_ACK Ack;

    Req.Vid = vid;
    Req.phys_value = phys_value;
    TransactRemoteMaster (RM_BLACKBOARD_GET_RAW_VALUE_FOR_PHYS_VALUE_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    *ret_raw_value = Ack.ret_raw_value;
    *ret_phys_value = Ack.ret_phys_value;
    return Ack.Ret;
}

int8_t rm_read_bbvari_byte(VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_BYTE_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_BYTE_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_BYTE_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

uint8_t rm_read_bbvari_ubyte(VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_UBYTE_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_UBYTE_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_UBYTE_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int16_t rm_read_bbvari_word(VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_WORD_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_WORD_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_WORD_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

uint16_t rm_read_bbvari_uword(VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_UWORD_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_UWORD_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_UWORD_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int32_t rm_read_bbvari_dword(VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_DWORD_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_DWORD_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_DWORD_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

uint32_t rm_read_bbvari_udword(VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_UDWORD_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_UDWORD_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_UDWORD_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int64_t rm_read_bbvari_qword (VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_QWORD_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_QWORD_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_QWORD_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

uint64_t rm_read_bbvari_uqword (VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_UQWORD_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_UQWORD_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_UQWORD_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}


float rm_read_bbvari_float (VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_FLOAT_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_FLOAT_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_FLOAT_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

double rm_read_bbvari_double (VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_DOUBLE_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_DOUBLE_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_DOUBLE_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

union BB_VARI rm_read_bbvari_union (VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_UNION_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_UNION_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_UNION_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

enum BB_DATA_TYPES rm_read_bbvari_union_type (VID vid, union BB_VARI *ret_Value)
{
    RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    *ret_Value = Ack.ret_Value;
    return Ack.Ret;
}

int rm_read_bbvari_union_type_frame (int Number, VID *Vids, enum BB_DATA_TYPES *ret_Types, union BB_VARI *ret_Values)
{
    RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_REQ *Req;
    RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_ACK *Ack;
    size_t ReqStructSize;
    size_t AckStructSize;

    ReqStructSize = sizeof(RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_REQ) + (size_t)Number * sizeof(int32_t);
    AckStructSize = sizeof(RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_ACK) + (size_t)Number * sizeof(int32_t) + (size_t)Number * sizeof(union BB_VARI);
    Req = (RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_REQ*)_alloca (ReqStructSize);
    Ack = (RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_ACK*)_alloca (AckStructSize);

    Req->Number = Number;
    Req->OffsetVids = sizeof(RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_REQ);
    MEMCPY(Req + 1, Vids, (size_t)Number * sizeof(int32_t));
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_CMD, Req, (int)ReqStructSize, Ack, (int)AckStructSize);
    CHECK_ANSWER(Req, Ack);
    MEMCPY(ret_Types, (char*)Ack + Ack->OffsetTypes, (size_t)Number * sizeof(int32_t));
    MEMCPY(ret_Values, (char*)Ack + Ack->OffsetValues, (size_t)Number * sizeof(union BB_VARI));
    return Ack->Ret;
}


double rm_read_bbvari_equ (VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_EQU_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_EQU_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_EQU_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

double rm_read_bbvari_convert_double (VID vid)
{
    RM_BLACKBOARD_READ_BBVARI_CONVERT_DOUBLE_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_CONVERT_DOUBLE_ACK Ack;

    Req.Vid = vid;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_CONVERT_DOUBLE_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_read_bbvari_convert_to (VID vid, int convert_to_type, union BB_VARI *ret_Ptr)
{
    RM_BLACKBOARD_READ_BBVARI_CONVERT_TO_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_CONVERT_TO_ACK Ack;

    Req.Vid = vid;
    Req.convert_to_type = convert_to_type;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_CONVERT_TO_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    *ret_Ptr = Ack.ret_Value;
    return Ack.Ret;
}

int rm_read_bbvari_textreplace (VID vid, char *txt, int maxc, int *pcolor)
{
    RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_REQ Req;
    RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_ACK *Ack;

    Ack = (RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_ACK*)_alloca (sizeof(RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_ACK) + (size_t)maxc);
    Req.Vid = vid;
    Req.maxc = maxc;
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_CMD, &Req, sizeof(Req), Ack, (int)sizeof(RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_ACK) + maxc);
    CHECK_ANSWER(Req, Ack);
    if (pcolor != NULL) *pcolor = Ack->ret_color;
    strcpy (txt, (char*)(Ack + 1));
    return Ack->Ret;
}

int rm_convert_value_textreplace (VID vid, int32_t value, char *txt, int maxc, int *pcolor)
{
    RM_BLACKBOARD_CONVERT_TEXTREPLACE_REQ Req;
    RM_BLACKBOARD_CONVERT_TEXTREPLACE_ACK *Ack;

    int StructSize = (int)sizeof(RM_BLACKBOARD_CONVERT_TEXTREPLACE_ACK) + maxc;
    Ack = (RM_BLACKBOARD_CONVERT_TEXTREPLACE_ACK*)_alloca ((size_t)StructSize);
    Req.Vid = vid;
    Req.Value = value;
    Req.maxc = maxc;
    TransactRemoteMaster (RM_BLACKBOARD_CONVERT_TEXTREPLACE_CMD, &Req, sizeof(Req), Ack, StructSize);
    CHECK_ANSWER(Req, Ack);
    *pcolor = Ack->ret_color;
    strcpy (txt, (char*)(Ack + 1));
    return Ack->Ret;
}

int rm_convert_textreplace_value (VID vid, char *txt, long *pfrom, long *pto)
{
    RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_REQ *Req;
    RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_ACK Ack;

    int EnumTextLen = (int)strlen (txt) + 1;
    int StructSize = (int)sizeof(RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_REQ) + EnumTextLen;

    Req = (RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_REQ*)_alloca ((size_t)StructSize);
    Req->Vid = vid;
    Req->OffsetText = sizeof (RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_REQ);
    MEMCPY (Req + 1, txt, (size_t)EnumTextLen);
    TransactRemoteMaster (RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_CMD, Req, StructSize, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    if (Ack.Ret == 0) {
        *pfrom = Ack.ret_from;
        *pto = Ack.ret_to;
    }
    return Ack.Ret;
}


int rm_read_bbvari_convert_to_FloatAndInt64 (VID vid, union FloatOrInt64 *ret_Value)
{
    RM_READ_BBVARI_CONVERT_TO_FLOATANDINT64_REQ Req;
    RM_READ_BBVARI_CONVERT_TO_FLOATANDINT64_ACK Ack;

    Req.vid = vid;
    TransactRemoteMaster (RM_READ_BBVARI_CONVERT_TO_FLOATANDINT64_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    *ret_Value = Ack.RetValue;
    return Ack.Ret;
}

void rm_write_bbvari_convert_with_FloatAndInt64 (PID pid, VID vid, union FloatOrInt64 value, int type)
{
    RM_WRITE_BBVARI_CONVERT_WITH_FLOATANDINT64_REQ Req;
    RM_WRITE_BBVARI_CONVERT_WITH_FLOATANDINT64_ACK Ack;

    Req.pid = pid;
    Req.vid = vid;
    Req.value = value;
    Req.type = (uint32_t)type;
    TransactRemoteMaster (RM_WRITE_BBVARI_CONVERT_WITH_FLOATANDINT64_CMD, &Req, sizeof(Req), &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
}

int rm_read_bbvari_by_name(const char *name, union FloatOrInt64 *ret_value, int *ret_byte_width, int read_type)
{
    RM_BLACKBOARD_READ_BBVARI_BY_NAME_REQ *Req;
    RM_BLACKBOARD_READ_BBVARI_BY_NAME_ACK Ack;
    size_t NameLen = strlen (name) + 1;
    size_t StructSize = sizeof(RM_BLACKBOARD_READ_BBVARI_BY_NAME_REQ) + NameLen;

    Req = (RM_BLACKBOARD_READ_BBVARI_BY_NAME_REQ*)_alloca (StructSize);
    Req->read_type = read_type;
    Req->OffsetText = sizeof (RM_BLACKBOARD_READ_BBVARI_BY_NAME_REQ);
    MEMCPY (Req + 1, name, NameLen);

    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_BY_NAME_CMD, Req, (int)StructSize, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    *ret_value = Ack.value;
    *ret_byte_width = Ack.ret_byte_width;
    return Ack.Ret;
}

int rm_write_bbvari_by_name(PID pid, const char *name, union FloatOrInt64 value, int type, int bb_type, int add_if_not_exist, int read_type)
{
    RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_REQ *Req;
    RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_ACK Ack;
    size_t NameLen = strlen (name) + 1;
    size_t StructSize = sizeof(RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_REQ) + NameLen;

    Req = (RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_REQ*)_alloca (StructSize);
    Req->Pid = pid;
    Req->type = (uint32_t)type;
    Req->bb_type = (uint32_t)bb_type;
    Req->value = value;
    Req->add_if_not_exist = (uint32_t)add_if_not_exist;
    Req->read_type = read_type;
    Req->OffsetText = sizeof (RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_REQ);
    MEMCPY (Req + 1, name, NameLen);

    TransactRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_CMD, Req, (int)StructSize, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_read_bbvari_frame (VID *Vids, double *RetFrameValues, int Size)
{
    RM_BLACKBOARD_READ_BBVARI_FRAME_REQ *Req;
    RM_BLACKBOARD_READ_BBVARI_FRAME_ACK *Ack;
    size_t ReqStructSize;
    size_t AckStructSize;

    ReqStructSize = sizeof(RM_BLACKBOARD_READ_BBVARI_FRAME_REQ) + (size_t)Size * sizeof (VID);
    AckStructSize = sizeof(RM_BLACKBOARD_READ_BBVARI_FRAME_ACK) + (size_t)Size * sizeof (double);
    Req = (RM_BLACKBOARD_READ_BBVARI_FRAME_REQ*)_alloca (ReqStructSize);
    Ack = (RM_BLACKBOARD_READ_BBVARI_FRAME_ACK*)_alloca (AckStructSize);
    Req->Size = Size;
    Req->OffsetVids = sizeof (RM_BLACKBOARD_READ_BBVARI_FRAME_REQ);
    MEMCPY (Req+1, Vids, (size_t)Size * sizeof(VID));
    TransactRemoteMaster (RM_BLACKBOARD_READ_BBVARI_FRAME_CMD, Req, (int)ReqStructSize, Ack, (int)AckStructSize);
    CHECK_ANSWER(Req, Ack);
    if (Ack->Ret == 0) {
        MEMCPY (RetFrameValues, Ack + 1, (size_t)Size * sizeof(double));
    }
    return Ack->Ret;
}

int rm_write_bbvari_frame (PID Pid, VID *Vids, double *FrameValues, int Size)
{
    RM_BLACKBOARD_WRITE_BBVARI_FRAME_REQ *Req;
    RM_BLACKBOARD_WRITE_BBVARI_FRAME_ACK Ack;
    size_t ReqStructSize;

    ReqStructSize = sizeof(RM_BLACKBOARD_WRITE_BBVARI_FRAME_REQ) + (size_t)Size * sizeof (VID) + (size_t)Size * sizeof(double);
    Req = (RM_BLACKBOARD_WRITE_BBVARI_FRAME_REQ*)_alloca (ReqStructSize);
    Req->Pid = Pid;
    Req->Size = Size;
    Req->OffsetVids = sizeof (RM_BLACKBOARD_WRITE_BBVARI_FRAME_REQ);
    MEMCPY (Req+1, Vids, (size_t)Size * sizeof (VID));
    Req->OffsetValues = (uint32_t)sizeof (RM_BLACKBOARD_WRITE_BBVARI_FRAME_REQ) + (uint32_t)Size * sizeof (VID);
    MEMCPY ((char*)(Req+1) + Req->OffsetValues, FrameValues, (size_t)Size * sizeof(double));
    TransactRemoteMaster (RM_BLACKBOARD_WRITE_BBVARI_FRAME_CMD, Req, (int)ReqStructSize, &Ack, sizeof(Ack));
    CHECK_ANSWER(Req, Ack);
    return Ack.Ret;
}

int rm_req_write_varinfos_to_ini (RM_BLACKBOARD_WRITE_BBVARI_INFOS_REQ *Req)
{
    // nur Debugging!
    int StructSize;
    StructSize = sizeof(RM_BLACKBOARD_WRITE_BBVARI_INFOS_REQ);
    StructSize = sizeof(BB_VARIABLE);
    StructSize = sizeof(BB_VARIABLE_ADDITIONAL_INFOS);
    StructSize = sizeof(enum BB_CONV_TYPES);
    StructSize = sizeof(BB_VARIABLE_CONVERSION);

    if (Req->PackageHeader.Command == RM_BLACKBOARD_WRITE_BBVARI_INFOS_CMD) {
        Req->BaseInfos.pAdditionalInfos = &(Req->AdditionalInfos);
        Req->BaseInfos.pAdditionalInfos->Name = (char*)Req + Req->OffsetName;
        Req->BaseInfos.pAdditionalInfos->Unit = (char*)Req + Req->OffsetUnit;
        switch (Req->BaseInfos.pAdditionalInfos->Conversion.Type) {
        //default:
        case BB_CONV_NONE:
        case BB_CONV_FACTOFF:
            break;
        case BB_CONV_FORMULA:
            Req->BaseInfos.pAdditionalInfos->Conversion.Conv.Formula.FormulaString = (char*)Req + Req->OffsetConversion;
            break;
        case BB_CONV_TEXTREP:
            Req->BaseInfos.pAdditionalInfos->Conversion.Conv.TextReplace.EnumString = (char*)Req + Req->OffsetConversion;
            break;
        case BB_CONV_REF:
            Req->BaseInfos.pAdditionalInfos->Conversion.Conv.Reference.Name = (char*)Req + Req->OffsetConversion;
            break;
        }

        if (Req->OffsetDisplayName) {
            Req->BaseInfos.pAdditionalInfos->DisplayName = (char*)Req + Req->OffsetDisplayName;
        } else {
            Req->BaseInfos.pAdditionalInfos->DisplayName = NULL;
        }
        if (Req->OffsetComment) {
            Req->BaseInfos.pAdditionalInfos->Comment = (char*)Req + Req->OffsetComment;
        } else {
            Req->BaseInfos.pAdditionalInfos->Comment = NULL;
        }
        write_varinfos_to_ini (&(Req->BaseInfos), INI_CACHE_ENTRY_FLAG_ALL_INFOS);
        return 1;
    } else {
        return 0;
    }
}

int rm_req_read_varinfos_from_ini (RM_BLACKBOARD_READ_BBVARI_FROM_INI_REQ *Req)
{
    if (Req->PackageHeader.Command == RM_BLACKBOARD_READ_BBVARI_FROM_INI_CMD) {
        char *Name = (char*)Req + Req->OffsetName;
        int   error;
        char *tmp_var_str;
        BB_VARIABLE s_vari_elem;
        BB_VARIABLE_ADDITIONAL_INFOS AdditionalInfos;

        memset (&s_vari_elem, 0, sizeof (s_vari_elem));
        memset (&AdditionalInfos, 0, sizeof (AdditionalInfos));
        s_vari_elem.pAdditionalInfos = &AdditionalInfos;

        if ((tmp_var_str = IniFileDataBaseReadStringBufferNoDef (VARI_SECTION, Name, GetMainFileDescriptor())) == NULL) {
            set_default_varinfo (&s_vari_elem, s_vari_elem.Type);
            return 0;
        }
        if ((error = set_varinfo_to_inientrys (&s_vari_elem, tmp_var_str)) != 0) {
            set_default_varinfo (&s_vari_elem, s_vari_elem.Type);
            IniFileDataBaseReadStringBufferFree(tmp_var_str);
            return error;
        }
        IniFileDataBaseReadStringBufferFree(tmp_var_str);


        if ((Req->ReadReqMask & READ_UNIT_BBVARI_FROM_INI) == READ_UNIT_BBVARI_FROM_INI) {
            rm_set_bbvari_unit(Req->Vid, s_vari_elem.pAdditionalInfos->Unit);
        }
        if ((Req->ReadReqMask& READ_MIN_BBVARI_FROM_INI) ==  READ_MIN_BBVARI_FROM_INI) {
            rm_set_bbvari_min(Req->Vid, s_vari_elem.pAdditionalInfos->Min);
        }
        if ((Req->ReadReqMask& READ_MAX_BBVARI_FROM_INI) ==  READ_MAX_BBVARI_FROM_INI) {
            rm_set_bbvari_max(Req->Vid, s_vari_elem.pAdditionalInfos->Max);
        }
        if ((Req->ReadReqMask& READ_STEP_BBVARI_FROM_INI) ==  READ_STEP_BBVARI_FROM_INI) {
            rm_set_bbvari_step(Req->Vid, s_vari_elem.pAdditionalInfos->StepType, s_vari_elem.pAdditionalInfos->Step);
        }
        if ((Req->ReadReqMask& READ_WIDTH_PREC_BBVARI_FROM_INI) ==  READ_WIDTH_PREC_BBVARI_FROM_INI) {
            rm_set_bbvari_format(Req->Vid, s_vari_elem.pAdditionalInfos->Width, s_vari_elem.pAdditionalInfos->Prec);
        }
        if ((Req->ReadReqMask& READ_CONVERSION_BBVARI_FROM_INI) ==  READ_CONVERSION_BBVARI_FROM_INI) {
            switch (s_vari_elem.pAdditionalInfos->Conversion.Type) {
            //default:
            case BB_CONV_NONE:
            case BB_CONV_FACTOFF:
                rm_set_bbvari_conversion(Req->Vid, BB_CONV_NONE, "");
                break;
            case BB_CONV_FORMULA:
                rm_set_bbvari_conversion(Req->Vid, BB_CONV_FORMULA, s_vari_elem.pAdditionalInfos->Conversion.Conv.Formula.FormulaString);
                break;
            case BB_CONV_TEXTREP:
                rm_set_bbvari_conversion(Req->Vid, BB_CONV_TEXTREP, s_vari_elem.pAdditionalInfos->Conversion.Conv.TextReplace.EnumString);
                break;
            case BB_CONV_REF:
                rm_set_bbvari_conversion(Req->Vid, BB_CONV_REF, s_vari_elem.pAdditionalInfos->Conversion.Conv.Reference.Name);
                break;
            }
        }
        if ((Req->ReadReqMask& READ_COLOR_BBVARI_FROM_INI) ==  READ_COLOR_BBVARI_FROM_INI) {
            rm_set_bbvari_color(Req->Vid, (int)s_vari_elem.pAdditionalInfos->RgbColor);
        }
        return 1;
    } else {
        return 0;
    }
}


int rm_RegisterEquation (RM_BLACKBOARD_REGISTER_EQUATION_REQ *Req)
{
    if (Req->PackageHeader.Command == RM_BLACKBOARD_REGISTER_EQUATION_CMD) {
        RegisterEquation (Req->Pid,
                          (char*)Req + Req->OffsetEquation,
                          (struct EXEC_STACK_ELEM *)((char*)Req + Req->OffsetExecStack),
                          (char*)Req + Req->OffsetAdditionalInfos,
                          Req->TypeFlags);
        return 1;
    } else {
        return 0;
    }
}


void rm_CallObserationCallbackFunction (RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_REQ *Req)
{
    if (Req->PackageHeader.Command == RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_CMD) {
        if (ObserationCallbackFunction != NULL) {
            ObserationCallbackFunction (Req->Vid, Req->ObservationFlags, Req->ObservationData);
        }
    }
}
