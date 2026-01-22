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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#ifdef _WIN32 
#include <intrin.h>
#else
static inline uint64_t _umul128(uint64_t a, uint64_t b, uint64_t *ret_HighPart)
{
    __int128 result = (__int128)a * (__int128)b;
    *ret_HighPart = result >> 64;
    return (uint64_t)result;
}
#endif


#include "Scheduler.h"
#include "Blackboard.h"
#include "BlackboardHashIndex.h"
#include "BlackboardConversion.h"
#include "ExecutionStack.h"
#include "TextReplace.h"
#include "StringMaxChar.h"
#include "ThrowError.h"

#include "BlackboardConvertFromTo.h"

#ifndef REMOTE_MASTER
#include "MainValues.h"
#include "RemoteMasterBlackboard.h"
#endif

#include "BlackboardAccess.h"

#define UNUSED(x) (void)(x)


static int convert_physical_internal(int vid_index, double raw_value, double *ret_phys_value)
{
    // Check if conversion type matches
    switch (blackboard[vid_index].pAdditionalInfos->Conversion.Type) {
    case BB_CONV_FORMULA:
        *ret_phys_value = execute_stack_replace_variable_with_parameter((struct EXEC_STACK_ELEM *)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode,
                                                                        blackboard[vid_index].Vid,
                                                                        raw_value);
        //*ret_phys_value = execute_stack_whith_parameter ((struct EXEC_STACK_ELEM *)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode,
        //                                                 raw_value);
        return 0;
    case BB_CONV_FACTOFF:
        *ret_phys_value = blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Factor * raw_value +
                          blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Offset;
        return 0;
    case BB_CONV_OFFFACT:
        *ret_phys_value = raw_value + (blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Offset) *
                          blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Factor;
        return 0;
    case BB_CONV_TAB_INTP:
    case BB_CONV_TAB_NOINTP:
    {
        int x, Size;
        double m, RawDelta, PhysDelta;
        struct CONVERSION_TABLE_VALUE_PAIR *Values;

        Size = blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Size;
        Values = blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Values;
        for (x = 0; x < Size; x++) {
            if (Values[x].Raw >= raw_value) {
                if (x == 0) {
                    *ret_phys_value = Values[x].Phys;
                } else {
                    if (blackboard[vid_index].pAdditionalInfos->Conversion.Type == BB_CONV_TAB_INTP) {
                        RawDelta = Values[x].Raw - Values[x-1].Raw;
                        if ((RawDelta <= 0.0) && (RawDelta >= 0.0)) {  // == 0.0
                            *ret_phys_value = Values[x-1].Phys;  // use the smallest one
                        } else {
                            PhysDelta = Values[x].Phys - Values[x-1].Phys;
                            m = PhysDelta / RawDelta;
                            *ret_phys_value = Values[x-1].Phys + m * (raw_value - Values[x-1].Raw);
                        }
                    } else {
                        *ret_phys_value = Values[x].Phys;
                    }
                }
                break;  // for(;;)
            }
        }
        if (x == Size) {
            *ret_phys_value = Values[Size - 1].Phys;
        }
        return 0;
        break;
    }
    case BB_CONV_RAT_FUNC:
        if (Conv_RationalFunctionRawToPhys(&blackboard[vid_index].pAdditionalInfos->Conversion, raw_value,ret_phys_value) == 0) {
            return 0;
        }
        break;
    default:
        break;
    }
    return -1;
}

void write_bbvari_byte_x (PID pid, VID vid, int8_t v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_byte (pid, vid, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_BYTE) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        blackboard[vid_index].Value.b = v;
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    LeaveBlackboardCriticalSection();
}

void write_bbvari_byte(VID vid, int8_t v)
{
    write_bbvari_byte_x(GET_PID(), vid, v);
}

void write_bbvari_ubyte_x (PID pid, VID vid, uint8_t v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_ubyte (pid, vid, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_UBYTE) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        blackboard[vid_index].Value.ub = v;
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    LeaveBlackboardCriticalSection();
}

void write_bbvari_ubyte(VID vid, uint8_t v)
{
    write_bbvari_ubyte_x(GET_PID(), vid, v);
}

void write_bbvari_word_x (PID pid, VID vid, int16_t v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_word (pid, vid, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_WORD) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        blackboard[vid_index].Value.w = v;
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    LeaveBlackboardCriticalSection();
}

void write_bbvari_word(VID vid, int16_t v)
{
    write_bbvari_word_x(GET_PID(), vid, v);
}

void write_bbvari_uword_x (PID pid, VID vid, uint16_t v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_uword (pid, vid, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_UWORD) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        blackboard[vid_index].Value.uw = v;
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    LeaveBlackboardCriticalSection();
}

void write_bbvari_uword(VID vid, uint16_t v)
{
    write_bbvari_uword_x(GET_PID(), vid, v);
}

void write_bbvari_dword_x (PID pid, VID vid, int32_t v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_dword (pid, vid, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_DWORD) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        blackboard[vid_index].Value.dw = v;
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    LeaveBlackboardCriticalSection();
}

void write_bbvari_dword(VID vid, int32_t v)
{
    write_bbvari_dword_x(GET_PID(), vid, v);
}

void write_bbvari_udword_x (PID pid, VID vid, uint32_t v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_udword (pid, vid, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_UDWORD) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        blackboard[vid_index].Value.udw = v;
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    LeaveBlackboardCriticalSection();
}

void write_bbvari_udword(VID vid, uint32_t v)
{
    write_bbvari_udword_x(GET_PID(), vid, v);
}

#ifdef REMOTE_MASTER
void write_bbvari_udword_without_check(VID vid, uint32_t v)
{
	int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
	if (blackboard == NULL) {
		return;
	}
    // Determine the vid index
	if ((vid_index = get_variable_index(vid)) == -1) {
		return;
	}
    // Check if data type match
	if (blackboard[vid_index].Type != BB_UDWORD) {
		return;
	}
	blackboard[vid_index].Value.udw = v;
	blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
}
#endif

void write_bbvari_qword_x (PID pid, VID vid, int64_t v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_qword (pid, vid, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_QWORD) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        blackboard[vid_index].Value.qw = v;
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    LeaveBlackboardCriticalSection();
}

void write_bbvari_qword(VID vid, int64_t v)
{
    write_bbvari_qword_x(GET_PID(), vid, v);
}

void write_bbvari_uqword_x (PID pid, VID vid, uint64_t v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_uqword (pid, vid, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_UQWORD) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        blackboard[vid_index].Value.uqw = v;
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    LeaveBlackboardCriticalSection();
}

void write_bbvari_uqword(VID vid, uint64_t v)
{
    write_bbvari_uqword_x(GET_PID(), vid, v);
}

void write_bbvari_float_x (PID pid, VID vid, float v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_float (pid, vid, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_FLOAT) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        blackboard[vid_index].Value.f = v;
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    LeaveBlackboardCriticalSection();
}

void write_bbvari_float(VID vid, float v)
{
    write_bbvari_float_x(GET_PID(), vid, v);
}

void write_bbvari_double_x (PID pid, VID vid, double v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_double (pid, vid, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_DOUBLE) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        blackboard[vid_index].Value.d = v;
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    LeaveBlackboardCriticalSection();
}

void write_bbvari_double(VID vid, double v)
{
    write_bbvari_double_x(GET_PID(), vid, v);
}

#ifdef REMOTE_MASTER
void write_bbvari_double_without_check(VID vid, double v)
{
	int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
	if (blackboard == NULL) {
		return;
	}
    // Determine the vid index
	if ((vid_index = get_variable_index(vid)) == -1) {
		return;
	}
    // Check if data type match
	if (blackboard[vid_index].Type != BB_DOUBLE) {
		return;
	}
	blackboard[vid_index].Value.d = v;
	blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
}
#endif

void write_bbvari_union_x (PID pid, VID vid, union BB_VARI v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_union (pid, vid, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        blackboard[vid_index].Value = v;
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    LeaveBlackboardCriticalSection();
}

void write_bbvari_union(VID vid, union BB_VARI v)
{
    write_bbvari_union_x(GET_PID(), vid, v);
}

void write_bbvari_union_pid (int Pid, VID vid, int DataType, union BB_VARI v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_union_pid (Pid, vid, DataType, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(Pid)) == -1) {
        return;
    }
    if ((DataType != blackboard[vid_index].Type) &&
        (DataType != BB_CONVERT_LIMIT_MIN_MAX)) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        switch (DataType) {
        case BB_BYTE:
            blackboard[vid_index].Value.b = v.b;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        case BB_UBYTE:
            blackboard[vid_index].Value.ub = v.ub;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        case BB_WORD:
            blackboard[vid_index].Value.w = v.w;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        case BB_UWORD:
            blackboard[vid_index].Value.uw = v.uw;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        case BB_DWORD:
            blackboard[vid_index].Value.dw = v.dw;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        case BB_UDWORD:
            blackboard[vid_index].Value.udw = v.udw;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        case BB_FLOAT:
            blackboard[vid_index].Value.f = v.f;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        case BB_DOUBLE:
            blackboard[vid_index].Value.d = v.d;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        case BB_CONVERT_LIMIT_MIN_MAX:
            switch(blackboard[vid_index].Type) {
            case BB_BYTE:
                blackboard[vid_index].Value.b = convert_double2byte(v.d);
                blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
                break;

            case BB_UBYTE:
                blackboard[vid_index].Value.ub = convert_double2ubyte(v.d);
                blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
                break;

            case BB_WORD:
                blackboard[vid_index].Value.w = convert_double2word(v.d);
                blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
                break;

            case BB_UWORD:
                blackboard[vid_index].Value.uw = convert_double2uword(v.d);
                blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
                break;

            case BB_DWORD:
                blackboard[vid_index].Value.dw = convert_double2dword(v.d);
                blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
                break;

            case BB_UDWORD:
                blackboard[vid_index].Value.udw = convert_double2udword(v.d);
                blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
                break;

            case BB_FLOAT:
                blackboard[vid_index].Value.f = convert_double2float(v.d);
                blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
                break;

            case BB_DOUBLE:
                blackboard[vid_index].Value.d = v.d;
                blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
                break;
            default:
                break;
            }
            break;
        }
    }
    LeaveBlackboardCriticalSection();
}

void ConvertDoubleToUnion(enum BB_DATA_TYPES Type,
                          double Value,
                          union BB_VARI *ret_Value)
{
    switch(Type) {
    case BB_BYTE:
        ret_Value->b = convert_double2byte(Value);
        break;
    case BB_UBYTE:
        ret_Value->ub = convert_double2ubyte(Value);
        break;
    case BB_WORD:
        ret_Value->w = convert_double2word(Value);
        break;
    case BB_UWORD:
        ret_Value->uw = convert_double2uword(Value);
        break;
    case BB_DWORD:
        ret_Value->dw = convert_double2dword(Value);
        break;
    case BB_UDWORD:
        ret_Value->udw = convert_double2udword(Value);
        break;
    case BB_QWORD:
        ret_Value->qw = convert_double2qword(Value);
        break;
    case BB_UQWORD:
        ret_Value->uqw = convert_double2uqword(Value);
        break;
    case BB_FLOAT:
        ret_Value->f = convert_double2float(Value);
        break;
    case BB_DOUBLE:
        ret_Value->d = Value;
        break;
    default:
        STRUCT_ZERO_INIT(*ret_Value, union BB_VARI);
        break;
    }
}

void ConvertRawMemoryToUnion(enum BB_DATA_TYPES Type,
                             void *RawMemValue,
                             union BB_VARI *ret_Value)
{
    switch(Type) {
    case BB_BYTE:
        ret_Value->b = *(int8_t*)RawMemValue;
        break;
    case BB_UBYTE:
        ret_Value->ub = *(uint8_t*)RawMemValue;
        break;
    case BB_WORD:
        ret_Value->w = *(int16_t*)RawMemValue;
        break;
    case BB_UWORD:
        ret_Value->uw = *(uint16_t*)RawMemValue;
        break;
    case BB_DWORD:
        ret_Value->dw =  *(int32_t*)RawMemValue;
        break;
    case BB_UDWORD:
        ret_Value->udw =  *(uint32_t*)RawMemValue;
        break;
    case BB_QWORD:
        ret_Value->qw = *(int64_t*)RawMemValue;
        break;
    case BB_UQWORD:
        ret_Value->uqw = *(uint64_t*)RawMemValue;
        break;
    case BB_FLOAT:
        ret_Value->f =  *(float*)RawMemValue;;
        break;
    case BB_DOUBLE:
        ret_Value->d =  *(double*)RawMemValue;
        break;
    default:
        STRUCT_ZERO_INIT(*ret_Value, union BB_VARI);
        break;
    }
}

void write_bbvari_minmax_check_pid (PID pid, VID vid, double v)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_write_bbvari_minmax_check (pid, vid, v);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        // Change the  wr-flags
        switch(blackboard[vid_index].Type) {
        case BB_BYTE:
            blackboard[vid_index].Value.b = sc_convert_double2byte(v);
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;

        case BB_UBYTE:
            blackboard[vid_index].Value.ub = sc_convert_double2ubyte(v);
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;

        case BB_WORD:
            blackboard[vid_index].Value.w = sc_convert_double2word(v);
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;

        case BB_UWORD:
            blackboard[vid_index].Value.uw = sc_convert_double2uword(v);
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;

        case BB_DWORD:
            blackboard[vid_index].Value.dw = sc_convert_double2dword(v);
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;

        case BB_UDWORD:
            blackboard[vid_index].Value.udw = sc_convert_double2udword(v);
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;

        case BB_QWORD:
            blackboard[vid_index].Value.qw = sc_convert_double2qword(v);
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;

        case BB_UQWORD:
            blackboard[vid_index].Value.uqw = sc_convert_double2uqword(v);
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;

        case BB_FLOAT:
            blackboard[vid_index].Value.f = sc_convert_double2float(v);
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;

        case BB_DOUBLE:
            blackboard[vid_index].Value.d = v;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        default:
            break;
        }
    }
    LeaveBlackboardCriticalSection();
}

void write_bbvari_minmax_check (VID vid, double v)
{
    write_bbvari_minmax_check_pid (GET_PID(), vid, v);
}

int write_bbvari_convert_to (PID pid, VID vid, int convert_from_type, void *ret_Ptr)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard != NULL) {
        // Determine the vid index
        if ((vid_index = get_variable_index(vid)) >= 0) {
            // Determine the process  index (for access mask)
            if ((pid_index = get_process_index (pid)) >= 0) {
                // Takeover value only if the process have access rights
                if (blackboard[vid_index].AccessFlags &
                    blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
                    int Ret;
                    blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
                    Ret = sc_convert_from_to (convert_from_type, ret_Ptr, blackboard[vid_index].Type, &blackboard[vid_index].Value);
                    blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
                    return Ret;
                }
            }
        }
    } else {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_write_bbvari_convert_to (pid, vid, convert_from_type, ret_Ptr);
        }
#endif
    }
    return size_of_bbvari (convert_from_type);
}

int IsMinOrMaxLimit (enum BB_DATA_TYPES Type,union BB_VARI Value)
{
    switch (Type) {
    case BB_BYTE:
        return ((Value.b == INT8_MAX) ||
                (Value.b == INT8_MIN));
    case BB_UBYTE:
        return ((Value.ub == UINT8_MAX) ||
                (Value.ub == 0));
    case BB_WORD:
        return ((Value.w == INT16_MAX) ||
                (Value.w == INT16_MIN));
    case BB_UWORD:
        return ((Value.uw == UINT16_MAX) ||
                (Value.uw == 0));
    case BB_DWORD:
        return ((Value.dw == INT32_MAX) ||
                (Value.dw == INT32_MIN));
    case BB_UDWORD:
        return ((Value.udw == UINT32_MAX) ||
                (Value.udw == 0));
    case BB_QWORD:
        return ((Value.qw == INT64_MAX) ||
                (Value.qw == INT64_MIN));
    case BB_UQWORD:
        return ((Value.uqw == UINT64_MAX) ||
                (Value.uqw == 0));
    case BB_FLOAT:
    case BB_DOUBLE:
    default:
        return 0;
    }
}

#define UNUSED(x) (void)(x)
static int write_bbvari_phys_minmax_check_iteration (BB_VARIABLE *pVariable, double new_phys_value, double x1, double y1, double min, double max, double delta,
                                                     double *ret_new_dec_value, double *ret_m)
{
    UNUSED(min);
    double x2, y2;
    union BB_VARI Value;
    double Help;
    double m, b;
    double new_dec_value;

    x2 = x1 + delta;
    if (x2 > max) {
        x2 = x1 - delta;
    }
    ConvertDoubleToUnion(pVariable->Type, x2, &Value);
    if (bbvari_to_double (pVariable->Type,
                          Value,
                          &Help) == -1) {
        return -1;
    }

    double diff = x1 - x2;
    if ((diff <= 0.0) && (diff >= 0.0)) {     // Compare 2 double values if they are equel without compiler warning
        return -1;
    }

    y2 = execute_stack_whith_parameter ((struct EXEC_STACK_ELEM *)pVariable->pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode, Help);

    m = (y2 - y1) / (x2 - x1);
    b = y1 - m * x1;

    if (m == 0.0) {
        return -2;
    }

    *ret_m = m;

    new_dec_value = (new_phys_value - b) / m;

    ConvertDoubleToUnion(pVariable->Type, new_dec_value, &Value);

    if (bbvari_to_double (pVariable->Type,
                          Value,
                          ret_new_dec_value) == -1) {
        return -1;
    }
    if (IsMinOrMaxLimit (pVariable->Type, Value)) {
        return 1; // Value is not over the limit of the data type
    } else {
        return 0; // Only the value is not outside the data type range
    }
}

static double write_bbvari_phys_minmax_check_diff (BB_VARIABLE *pVariable, double new_phys_value, double raw_value, double *ret_phys_value)
{
    double phys;
    phys = execute_stack_whith_parameter_cs ((struct EXEC_STACK_ELEM *)pVariable->pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode, raw_value, 0);
    *ret_phys_value = phys;
    return fabs(phys - new_phys_value);
}

static double mantissa_plus_or_minus_one_digit_double (double value, int plus_or_minus)
{
    if (value == 0.0) {
        if (plus_or_minus) {
            return DBL_MIN;
        } else {
            return -DBL_MIN;
        }
    } else {
        double ret;
        int exp;
        double man = frexp (value, &exp);
        if (plus_or_minus) {
            man += DBL_EPSILON/2.0;
        } else {
            man -= DBL_EPSILON/2.0;
        }
        ret = ldexp(man, exp);
        return ret;
    }
}

static float mantissa_plus_or_minus_one_digit_float (float value, int plus_or_minus)
{
    if (value == 0.0f) {
        if (plus_or_minus) {
            return FLT_MIN;
        } else {
            return -FLT_MIN;
        }
    } else {
        float ret;
        int exp;
        float man = frexpf (value, &exp);
        if (plus_or_minus) {
            man += FLT_EPSILON/2.0f;
        } else {
            man -= FLT_EPSILON/2.0f;
        }
        ret = ldexpf(man, exp);
        return ret;
    }
}

static void calc_min_max_target_value_float (double target_value, double *ret_min, double *ret_max)
{
    if (target_value == 0.0) {
         *ret_min = -16.0*(double)FLT_MIN;
         *ret_max = 16.0*(double)FLT_MIN;
    } else {
        int exp;
        float man2;
        float man = frexpf ((float)target_value, &exp);
        man2 = man - 16.0f * FLT_EPSILON;
        *ret_min = (double)ldexpf(man2, exp);
        man2 = man + 16.0f * FLT_EPSILON;
        *ret_max = (double)ldexpf(man2, exp);
    }
}

static void calc_min_max_target_value_double (double target_value, double *ret_min, double *ret_max)
{
    if (target_value == 0.0) {
         *ret_min = -16.0*DBL_MIN;
         *ret_max = 16.0*DBL_MIN;
    } else {
        int exp;
        double man2;
        double man = frexp (target_value, &exp);
        man2 = man - 16.0 * DBL_EPSILON;
        *ret_min = ldexp(man2, exp);
        man2 = man + 16.0 * DBL_EPSILON;
        *ret_max = ldexp(man2, exp);
    }
}

static int write_bbvari_phys_minmax_check_inner(int vid_index, double new_phys_value, union BB_VARI *ret_Value, double *ret_phys_value)
{
    double old_raw_value;
    double old_phys_value;
    double new_raw_value;
    double new_raw_value_help;

    double min_diff, min_diff2;
    int dir, x;
    double delta;

    double min;
    double max;
    double range_min = 0.0;
    double range_max = 0.0;
    double m;
    double phys;

    if (blackboard[vid_index].Type == BB_FLOAT) {
        calc_min_max_target_value_float (new_phys_value, &range_min, &range_max);
    } else if (blackboard[vid_index].Type == BB_DOUBLE) {
        calc_min_max_target_value_double (new_phys_value, &range_min, &range_max);
    }

    get_datatype_min_max_value (blackboard[vid_index].Type, &min, &max);

    if (bbvari_to_double (blackboard[vid_index].Type, blackboard[vid_index].Value, &old_raw_value) == -1) {
        return -1;
    }
    // First approach
    if ((blackboard[vid_index].Type == BB_FLOAT) ||
        (blackboard[vid_index].Type == BB_DOUBLE)) {
        delta = fabs(old_raw_value * 0.01) + 1.0;
    } else {
        delta = fabs(old_raw_value) + 1.0;
    }
    old_phys_value = execute_stack_whith_parameter_cs ((struct EXEC_STACK_ELEM *)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode, old_raw_value, 0);
    if (write_bbvari_phys_minmax_check_iteration (&(blackboard[vid_index]), new_phys_value, old_raw_value, old_phys_value, min, max, delta, &new_raw_value, &m) < 0) {
        return -1;
    }
    // following approach steps (but max. 100)
    for (x = 0; x < 100; x++) {
        //double phys;
        phys = execute_stack_whith_parameter_cs ((struct EXEC_STACK_ELEM *)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode, new_raw_value, 0);
        if ((blackboard[vid_index].Type == BB_FLOAT) ||
            (blackboard[vid_index].Type == BB_DOUBLE)) {
            if ((phys < range_max) && (phys > range_min)) {
                break;
            }
        } else {
            if (fabs(phys - new_phys_value) <= fabs(m)) {
                break;
            }
        }
        if ((blackboard[vid_index].Type == BB_FLOAT) ||
            (blackboard[vid_index].Type == BB_DOUBLE)) {
            delta = fabs(new_raw_value * 0.01) + 1.0;
        } else {
            delta = fabs(new_raw_value) + 1.0;
        }
        if (write_bbvari_phys_minmax_check_iteration (&(blackboard[vid_index]), new_phys_value, new_raw_value, phys, min, max, delta, &new_raw_value, &m) < 0) {
            return -1;
        }
    }
    if (x == 100) {
        return -2;
    }
    min_diff = write_bbvari_phys_minmax_check_diff (&(blackboard[vid_index]), new_phys_value, new_raw_value, ret_phys_value);
    if (min_diff != 0.0) {
        // And now do some single steps but only 4 in both directions
        new_raw_value_help = new_raw_value;
        for (dir = 0; dir < 2; dir++) {
            for (x = 0; x < 100; x++) {
                if (blackboard[vid_index].Type == BB_FLOAT) {
                    new_raw_value_help = (double)mantissa_plus_or_minus_one_digit_float ((float)new_raw_value_help, dir);
                } else if (blackboard[vid_index].Type == BB_DOUBLE) {
                    new_raw_value_help = mantissa_plus_or_minus_one_digit_double (new_raw_value_help, dir);
                } else {
                    // Integer data type
                    if (dir) {
                        new_raw_value_help = round(new_raw_value_help) + 1.0;
                        if (new_raw_value_help > max) {
                            break;
                        }
                    } else {
                        new_raw_value_help = round(new_raw_value_help) - 1.0;
                        if (new_raw_value_help < min) {
                            break;
                        }
                    }
                }
                min_diff2 = write_bbvari_phys_minmax_check_diff (&(blackboard[vid_index]), new_phys_value, new_raw_value_help, ret_phys_value);
                if (min_diff < min_diff2) {
                    break;   //
                } else {
                    new_raw_value = new_raw_value_help;
                    min_diff = min_diff2;
                }
            }
        }
    }
    // Not outside the data type range
    if (new_raw_value > max) {
        new_raw_value = max;
    }
    if (new_raw_value < min) {
        new_raw_value = min;
    }
    ConvertDoubleToUnion(blackboard[vid_index].Type, new_raw_value, ret_Value);
    return 0;
}

static int write_bbvari_factor_offset_minmax_check_inner(int vid_index, double new_phys_value, union BB_VARI *ret_Value, double *ret_phys_value)
{
    double old_raw_value;
    double new_raw_value;
    double min;
    double max;

    get_datatype_min_max_value (blackboard[vid_index].Type, &min, &max);

    if (bbvari_to_double (blackboard[vid_index].Type, blackboard[vid_index].Value, &old_raw_value) == -1) {
        return -1;
    }
    if (blackboard[vid_index].pAdditionalInfos->Conversion.Type == BB_CONV_FACTOFF) {
        new_raw_value = new_phys_value * blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Factor +
                        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Offset;
    } else {
        new_raw_value = (new_phys_value + blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Factor) *
                        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Factor;
    }
    // Not outside the data type range
    if (new_raw_value > max) {
        new_raw_value = max;
    }
    if (new_raw_value < min) {
        new_raw_value = min;
    }
    ConvertDoubleToUnion(blackboard[vid_index].Type, new_raw_value, ret_Value);
    return 0;
}

static int write_bbvari_table_minmax_check_inner(int vid_index, double new_phys_value, union BB_VARI *ret_Value, double *ret_phys_value)
{
    double old_raw_value;
    double new_raw_value;
    double min;
    double max;
    int x, Size;
    double m, PhysDelta, RawDelta;
    struct CONVERSION_TABLE_VALUE_PAIR *Values;

    get_datatype_min_max_value (blackboard[vid_index].Type, &min, &max);

    if (bbvari_to_double (blackboard[vid_index].Type, blackboard[vid_index].Value, &old_raw_value) == -1) {
        return -1;
    }
    Size = blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Size;
    Values = blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Values;

    for (x = 0; x < Size; x++) {
        if (Values[x].Phys >= new_phys_value) {
            if (x == 0) {
                new_raw_value = Values[x].Raw;
            } else {
                if (blackboard[vid_index].pAdditionalInfos->Conversion.Type == BB_CONV_TAB_INTP) {
                    PhysDelta = Values[x].Phys - Values[x-1].Phys;
                    if ((PhysDelta <= 0.0) && (PhysDelta >= 0.0)) {  // == 0.0
                        new_raw_value = Values[x-1].Raw;  // use the smallest one
                    } else {
                        RawDelta = Values[x].Raw - Values[x-1].Raw;
                        m = RawDelta / PhysDelta;
                        new_raw_value = Values[x-1].Raw + m * (new_phys_value - Values[x-1].Phys);
                    }
                } else {
                    new_raw_value = Values[x].Raw;
                }
            }
            break;  // for(;;)
        }
    }
    if (x == Size) {
        new_raw_value = Values[Size - 1].Raw;
    }
    // Not outside the data type range
    if (new_raw_value > max) {
        new_raw_value = max;
    }
    if (new_raw_value < min) {
        new_raw_value = min;
    }
    ConvertDoubleToUnion(blackboard[vid_index].Type, new_raw_value, ret_Value);
    return 0;
}

static int write_bbvari_rational_function_minmax_check_inner(int vid_index, double new_phys_value, union BB_VARI *ret_Value, double *ret_phys_value)
{
    double old_raw_value;
    double new_raw_value;
    double min;
    double max;
    double a, b;

    get_datatype_min_max_value (blackboard[vid_index].Type, &min, &max);

    if (bbvari_to_double (blackboard[vid_index].Type, blackboard[vid_index].Value, &old_raw_value) == -1) {
        return -1;
    }
    // f(x)=(axx + bx + c)/(dxx + ex + f)
    // Raw = f(Phys)
    a = blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.a * new_phys_value * new_phys_value +
        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.b * new_phys_value +
        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.c;
    b = blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.d * new_phys_value * new_phys_value +
        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.e * new_phys_value +
        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.f;
    if (b != 0.0) {
        new_raw_value = a / b;
    } else {
        new_raw_value = DBL_MAX;
    }
    // Not outside the data type range
    if (new_raw_value > max) {
        new_raw_value = max;
    }
    if (new_raw_value < min) {
        new_raw_value = min;
    }
    ConvertDoubleToUnion(blackboard[vid_index].Type, new_raw_value, ret_Value);
    return 0;
}

int write_bbvari_phys_minmax_check_pid_cs (PID pid, VID vid, double new_phys_value, int cs)
{
    int vid_index;
    int pid_index;
    union BB_VARI Value;
    double phys_value;   // will not be used


    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_write_bbvari_phys_minmax_check (pid, vid, new_phys_value);
        }
#endif
        return -1;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return -1;
    }
    if (cs) EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {

        // Is there a conversion defined
        switch (blackboard[vid_index].pAdditionalInfos->Conversion.Type) {
        case BB_CONV_FORMULA:
        {
            int ret;
            ret = write_bbvari_phys_minmax_check_inner(vid_index, new_phys_value, &Value, &phys_value);
            if (ret != 0) {
                if (cs) LeaveBlackboardCriticalSection();
                return ret;
            }
            // Now write the calculated raw value
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            blackboard[vid_index].Value = Value;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        }
        case BB_CONV_FACTOFF:
        case BB_CONV_OFFFACT:
        {
            int ret;
            ret = write_bbvari_factor_offset_minmax_check_inner(vid_index, new_phys_value, &Value, &phys_value);
            if (ret != 0) {
                if (cs) LeaveBlackboardCriticalSection();
                return ret;
            }
            // Now write the calculated raw value
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            blackboard[vid_index].Value = Value;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        }
        case BB_CONV_TAB_INTP:
        case BB_CONV_TAB_NOINTP:
        {
            int ret;
            ret = write_bbvari_table_minmax_check_inner(vid_index, new_phys_value, &Value, &phys_value);
            if (ret != 0) {
                if (cs) LeaveBlackboardCriticalSection();
                return ret;
            }
            // Now write the calculated raw value
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            blackboard[vid_index].Value = Value;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        }
        case BB_CONV_RAT_FUNC:
        {
            int ret;
            ret = write_bbvari_rational_function_minmax_check_inner(vid_index, new_phys_value, &Value, &phys_value);
            if (ret != 0) {
                if (cs) LeaveBlackboardCriticalSection();
                return ret;
            }
            // Now write the calculated raw value
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            blackboard[vid_index].Value = Value;
            blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
            break;
        }
        default:
            // no formula are an error
            if (cs) LeaveBlackboardCriticalSection();
            return -2;
        }
    }
    if (cs) LeaveBlackboardCriticalSection();
    return 0;
}

int write_bbvari_phys_minmax_check (VID vid, double new_phys_value)
{
    return write_bbvari_phys_minmax_check_pid_cs (GET_PID(), vid, new_phys_value, 1);
}

int write_bbvari_phys_minmax_check_cs (VID vid, double new_phys_value, int cs)
{
    return write_bbvari_phys_minmax_check_pid_cs (GET_PID(), vid, new_phys_value, cs);
}

int write_bbvari_phys_minmax_check_pid (PID pid, VID vid, double new_phys_value)
{
    return write_bbvari_phys_minmax_check_pid_cs (pid, vid, new_phys_value, 1);
}

int get_phys_value_for_raw_value (VID vid, double raw_value, double *ret_phys_value)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_phys_value_for_raw_value (vid, raw_value, ret_phys_value);
        }
#endif
        return -1;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }
    //if (blackboard[vid_index].pAdditionalInfos->Conversion.Type != BB_CONV_FORMULA) {
    //    return -2;
    //}
    if (convert_physical_internal(vid_index, raw_value, ret_phys_value)) {
        return -2;
    }
    //*ret_phys_value = execute_stack_replace_variable_with_parameter ((struct EXEC_STACK_ELEM *)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode, vid, raw_value);
    return 0;
}

int get_raw_value_for_phys_value (VID vid, double phys_value, double *ret_raw_value, double *ret_phys_value)
{
    int ret;
    union BB_VARI Value;
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_raw_value_for_phys_value (vid, phys_value, ret_raw_value, ret_phys_value);
        }
#endif
        return -1;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }
    switch(blackboard[vid_index].pAdditionalInfos->Conversion.Type) {
    case BB_CONV_FORMULA:
        ret = write_bbvari_phys_minmax_check_inner(vid_index, phys_value, &Value, ret_phys_value);
        break;
    case BB_CONV_FACTOFF:
    case BB_CONV_OFFFACT:
        ret = write_bbvari_factor_offset_minmax_check_inner(vid_index, phys_value, &Value, ret_phys_value);
        break;
    case BB_CONV_TAB_INTP:
    case BB_CONV_TAB_NOINTP:
        ret = write_bbvari_table_minmax_check_inner(vid_index, phys_value, &Value, ret_phys_value);
        break;
    case BB_CONV_RAT_FUNC:
        ret = write_bbvari_rational_function_minmax_check_inner(vid_index, phys_value, &Value, ret_phys_value);
        break;
    default:
        ret = -1;
    }
    if (ret == 0) {
        if (bbvari_to_double(blackboard[vid_index].Type,
                             Value,
                             ret_raw_value) == -1) {
            ret = -1;
        } else {
            ret = 0;
        }
    }
    return ret;
}

int8_t read_bbvari_byte(VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_byte (vid);
        }
#endif
        return 0;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_BYTE) {
        return 0;
    }
    // Return the value
    return blackboard[vid_index].Value.b;
}

uint8_t read_bbvari_ubyte(VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_ubyte (vid);
        }
#endif
        return 0;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_UBYTE) {
        return 0;
    }
    // Return the value
    return blackboard[vid_index].Value.ub;
}

int16_t read_bbvari_word(VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_word (vid);
        }
#endif
        return 0;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_WORD) {
        return 0;
    }
    // Return the value
    return blackboard[vid_index].Value.w;
}

uint16_t read_bbvari_uword(VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_uword (vid);
        }
#endif
        return 0;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_UWORD) {
        return 0;
    }
    // Return the value
    return blackboard[vid_index].Value.uw;
}

int32_t read_bbvari_dword(VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_dword (vid);
        }
#endif
        return 0;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_DWORD) {
        return 0L;
    }
    // Return the value
    return blackboard[vid_index].Value.dw;
}

uint32_t read_bbvari_udword(VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_udword (vid);
        }
#endif
        return 0;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_UDWORD) {
        return 0UL;
    }
    // Return the value
    return blackboard[vid_index].Value.udw;
}

int64_t read_bbvari_qword (VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_qword (vid);
        }
#endif
        return 0;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_QWORD) {
        return 0L;
    }
    // Return the value
    return blackboard[vid_index].Value.qw;
}

uint64_t read_bbvari_uqword (VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_uqword (vid);
        }
#endif
        return 0;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_UQWORD) {
        return 0UL;
    }
    // Return the value
    return blackboard[vid_index].Value.uqw;
}

float read_bbvari_float (VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_float (vid);
        }
#endif
        return 0.0;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0.0;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_FLOAT) {
        return 0.0;
    }
    // Return the value
    return blackboard[vid_index].Value.f;
}

double read_bbvari_double (VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_double (vid);
        }
#endif
        return 0.0;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0.0;
    }
    // Check if data type match
    if (blackboard[vid_index].Type != BB_DOUBLE) {
        return 0.0;
    }
    // Return the value
    return blackboard[vid_index].Value.d;
}

union BB_VARI read_bbvari_union (VID vid)
{
    int vid_index;
    union BB_VARI error_ret;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_union (vid);
        }
#endif
        error_ret.d = 0;
        return error_ret;
    }
    if ((vid_index = get_variable_index(vid)) == -1) {
        error_ret.d = 0;
        return error_ret;
    }
    // Return the value
    return blackboard[vid_index].Value;
}

enum BB_DATA_TYPES read_bbvari_union_type (VID vid, union BB_VARI *ret_Value)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_union_type (vid, ret_Value);
        }
#endif
        return -1;
    }
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }
    *ret_Value = blackboard[vid_index].Value;
    // Return the type
    return blackboard[vid_index].Type;
}

int read_bbvari_union_type_frame (int Number, VID *Vids, enum BB_DATA_TYPES *ret_Types, union BB_VARI *ret_Values)
{
    int x, vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_union_type_frame (Number, Vids, ret_Types, ret_Values);
        }
#endif
        return -1;
    }
    for (x = 0; x < Number; x++)
        if ((vid_index = get_variable_index(Vids[x])) == -1) {
            ret_Types[x] = BB_INVALID;
            ret_Values[x].uqw = 0;
        } else {
            ret_Types[x] = blackboard[vid_index].Type;
            ret_Values[x] = blackboard[vid_index].Value;
        }
    // Return the number
    return x;
}

#ifndef EXTERN_PROCESS
double read_bbvari_equ (VID vid)
{
    int vid_index;
    double Value;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_equ (vid);
        }
#endif
        return 0.0;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0.0;
    }
    // Check if conversion type matches
    //if (blackboard[vid_index].pAdditionalInfos->Conversion.Type != BB_CONV_FORMULA) {
    //    return 0.0;
    //}
    if (bbvari_to_double (blackboard[vid_index].Type,
                          blackboard[vid_index].Value, &Value)) {
        return 0.0;
    } else {
        if (convert_physical_internal(vid_index, Value, &Value)) {
            return 0.0;
        }
        return Value;
        //return execute_stack_whith_parameter ((struct EXEC_STACK_ELEM *)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode,
        //                                      Value);
    }
}
#endif

double read_bbvari_convert_double (VID vid)
{
    int vid_index;
    double ret_value;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_convert_double (vid);
        }
#endif
        return 0.0;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0.0;
    }
    if (bbvari_to_double (blackboard[vid_index].Type,
                          blackboard[vid_index].Value,
                          &ret_value) == -1) {
        return 0.0;
    }
    // Return the value
    return ret_value;
}

int read_bbvari_convert_to (VID vid, int convert_to_type, union BB_VARI *ret_Ptr)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_convert_to (vid, convert_to_type, ret_Ptr);
        }
#endif
        return -1;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }
    switch (convert_to_type) {
    case BB_BYTE:
        sc_convert_2byte (blackboard[vid_index].Type, &blackboard[vid_index].Value, ret_Ptr);
        return 1;
    case BB_UBYTE:
        sc_convert_2ubyte (blackboard[vid_index].Type, &blackboard[vid_index].Value, ret_Ptr);
        return 1;
    case BB_WORD:
        sc_convert_2word (blackboard[vid_index].Type, &blackboard[vid_index].Value, ret_Ptr);
        return 2;
    case BB_UWORD:
        sc_convert_2uword (blackboard[vid_index].Type, &blackboard[vid_index].Value, ret_Ptr);
        return 2;
    case BB_DWORD:
        sc_convert_2dword (blackboard[vid_index].Type, &blackboard[vid_index].Value, ret_Ptr);
        return 4;
    case BB_UDWORD:
        sc_convert_2udword (blackboard[vid_index].Type, &blackboard[vid_index].Value, ret_Ptr);
        return 4;
    case BB_QWORD:
        sc_convert_2qword (blackboard[vid_index].Type, &blackboard[vid_index].Value, ret_Ptr);
        return 8;
    case BB_UQWORD:
        sc_convert_2uqword (blackboard[vid_index].Type, &blackboard[vid_index].Value, ret_Ptr);
        return 8;
    case BB_FLOAT:
        sc_convert_2float (blackboard[vid_index].Type, &blackboard[vid_index].Value, ret_Ptr);
        return 4;
    case BB_DOUBLE:
        sc_convert_2double (blackboard[vid_index].Type, &blackboard[vid_index].Value, ret_Ptr);
        return 8;
    default:
        return -1;
    }
}

int read_bbvari_textreplace (VID vid, char *txt, int maxc, int *pcolor)
{
    int vid_index;
    int64_t value;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_textreplace (vid, txt, maxc, pcolor);
        }
#endif
        return -1;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }
    // Check if conversion type matches
    if (blackboard[vid_index].pAdditionalInfos->Conversion.Type != BB_CONV_TEXTREP) {
        return -1;
    }
    // Convert variable to a 64 bit integer
    if (bbvari_to_int64( blackboard[vid_index].Type,
        blackboard[vid_index].Value, &value) == -1) {
        return -1;
    }
    // Convert variable to a text replace
    return convert_value2textreplace (value, (char *)blackboard[vid_index].
                                      pAdditionalInfos->Conversion.Conv.Formula.FormulaString, txt, maxc, pcolor);
}

int read_bbvari_convert_to_FloatAndInt64(VID vid, union FloatOrInt64 *ret_Value)
{
    int vid_index;
    int Ret;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_convert_to_FloatAndInt64 (vid, ret_Value);
        }
#endif
        return -1;
    }

    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }

    switch (blackboard[vid_index].Type) {
    case BB_BYTE:
        ret_Value->qw = blackboard[vid_index].Value.b;
        Ret = FLOAT_OR_INT_64_TYPE_INT64;
        break;
    case BB_UBYTE:
        ret_Value->uqw = blackboard[vid_index].Value.ub;
        Ret = FLOAT_OR_INT_64_TYPE_UINT64;
        break;
    case BB_WORD:
        ret_Value->qw = blackboard[vid_index].Value.w;
        Ret = FLOAT_OR_INT_64_TYPE_INT64;
        break;
    case BB_UWORD:
        ret_Value->uqw = blackboard[vid_index].Value.uw;
        Ret = FLOAT_OR_INT_64_TYPE_UINT64;
        break;
    case BB_DWORD:
        ret_Value->qw = blackboard[vid_index].Value.dw;
        Ret = FLOAT_OR_INT_64_TYPE_INT64;
        break;
    case BB_UDWORD:
        ret_Value->uqw = blackboard[vid_index].Value.udw;
        Ret = FLOAT_OR_INT_64_TYPE_UINT64;
        break;
    case BB_QWORD:
        ret_Value->qw = blackboard[vid_index].Value.qw;
        Ret = FLOAT_OR_INT_64_TYPE_INT64;
        break;
    case BB_UQWORD:
        ret_Value->uqw = blackboard[vid_index].Value.uqw;
        Ret = FLOAT_OR_INT_64_TYPE_UINT64;
        break;
    case BB_FLOAT:
        ret_Value->d = (double)blackboard[vid_index].Value.f;
        Ret = FLOAT_OR_INT_64_TYPE_F64;
        break;
    case BB_DOUBLE:
        ret_Value->d = blackboard[vid_index].Value.d;
        Ret = FLOAT_OR_INT_64_TYPE_F64;
        break;
    default:
        ret_Value->uqw = 0;
        Ret = FLOAT_OR_INT_64_TYPE_INVALID;
        break;
    }
    return Ret;
}

int mul_FloatOrInt64(union FloatOrInt64 value1, int type1, union FloatOrInt64 value2, int type2, union FloatOrInt64 *ret_value)
{
    if ((type1 == FLOAT_OR_INT_64_TYPE_F64) || (type2 == FLOAT_OR_INT_64_TYPE_F64)) {
        ret_value->d = FloatOrInt64_ToDouble(value1, type1) * FloatOrInt64_ToDouble(value2, type2);
        return FLOAT_OR_INT_64_TYPE_F64;
    } else {
        if (type1 == FLOAT_OR_INT_64_TYPE_INT64) {
            if (type2 == FLOAT_OR_INT_64_TYPE_UINT64) {
                // int * uint
                if (value1.qw < 0) {
                    uint64_t HighPart;
                    uint64_t V1 = (uint64_t)(-(value1.qw));
                    uint64_t R;
                    R = _umul128(V1, value2.uqw, &HighPart);
                    if ((HighPart > 0) || (R > 0x8000000000000000ULL)) {
                        ret_value->qw  = -0x8000000000000000LL;            // Limitierung kein berlauf
                    } else {
                        ret_value->qw = -(int64_t)R;
                    }
                    return FLOAT_OR_INT_64_TYPE_INT64;
                } else {
                    uint64_t HighPart;
                    uint64_t V1 = (uint64_t)(value1.qw);
                    uint64_t R;
                    R = _umul128(V1, value2.uqw, &HighPart);
                    if (HighPart > 0) {
                        ret_value->uqw = 0xFFFFFFFFFFFFFFFFULL;            // Limitierung kein berlauf
                    } else {
                        ret_value->uqw = R;
                    }
                    return FLOAT_OR_INT_64_TYPE_UINT64;
                }
            } else {
                // int * int
                if (value1.qw < 0) {
                    if (value2.qw < 0) {
                        // beide int sind negativ
                        uint64_t HighPart;
                        uint64_t V1 = (uint64_t)(-(value1.qw));
                        uint64_t V2 = (uint64_t)(-(value2.qw));
                        uint64_t R;
                        R = _umul128(V1, V2, &HighPart);
                        if (HighPart > 0) {
                            ret_value->uqw  = 0xFFFFFFFFFFFFFFFFULL;            // Limitierung kein berlauf
                        } else {
                            ret_value->uqw = R;
                        }
                        return FLOAT_OR_INT_64_TYPE_UINT64;
                    } else {
                        // erster int ist negativ
                        uint64_t HighPart;
                        uint64_t V1 = (uint64_t)(-(value1.qw));
                        uint64_t V2 = (uint64_t)(value2.qw);
                        uint64_t R;
                        R = _umul128(V1, V2, &HighPart);
                        if ((HighPart > 0) || (R > 0x8000000000000000ULL)) {
                            ret_value->qw  = -0x8000000000000000LL;            // Limitierung kein berlauf
                        } else {
                            ret_value->qw = -(int64_t)R;
                        }
                        return FLOAT_OR_INT_64_TYPE_INT64;
                    }
                } else {  // value1.qw >= 0
                    if (value2.qw < 0) {
                        // erster int ist positiv zweiter int ist negativ
                        uint64_t HighPart;
                        uint64_t V1 = (uint64_t)(value1.qw);
                        uint64_t V2 = (uint64_t)(-(value2.qw));
                        uint64_t R;
                        R = _umul128(V1, V2, &HighPart);
                        if ((HighPart > 0) || (R > 0x8000000000000000ULL)) {
                            ret_value->qw  = -0x8000000000000000LL;            // Limitierung kein berlauf
                        } else {
                            ret_value->qw = -(int64_t)R;
                        }
                        return FLOAT_OR_INT_64_TYPE_INT64;
                    } else {
                        // beider ints sind positiv
                        uint64_t HighPart;
                        uint64_t V1 = (uint64_t)(value1.qw);
                        uint64_t V2 = (uint64_t)(value2.qw);
                        uint64_t R;
                        R = _umul128(V1, V2, &HighPart);
                        if (HighPart > 0) {
                            ret_value->uqw = 0xFFFFFFFFFFFFFFFFULL;            // Limitierung kein berlauf
                        } else {
                            ret_value->uqw = R;
                        }
                        return FLOAT_OR_INT_64_TYPE_UINT64;
                    }
                }
            }
         } else {   // (type1 == FLOAT_OR_INT_64_TYPE_UINT64)
            if (type2 == FLOAT_OR_INT_64_TYPE_UINT64) {
                // uint * uint
                uint64_t HighPart;
                uint64_t R;
                R = _umul128(value1.uqw, value2.uqw, &HighPart);
                if (HighPart > 0) {
                    ret_value->uqw  = 0xFFFFFFFFFFFFFFFFULL;            // Limitierung kein berlauf
                } else {
                    ret_value->uqw = R;
                }
                return FLOAT_OR_INT_64_TYPE_UINT64;
            } else {
                // uint * int
                if (value2.qw < 0) {
                    uint64_t HighPart;
                    uint64_t V2 = (uint64_t)(-(value2.qw));
                    uint64_t R;
                    R = _umul128(value1.uqw, V2, &HighPart);
                    if ((HighPart > 0) || (R > 0x8000000000000000ULL)) {
                        ret_value->qw  = -0x8000000000000000LL;            // Limitierung kein berlauf
                    } else {
                        ret_value->qw = -(int64_t)R;
                    }
                    return FLOAT_OR_INT_64_TYPE_INT64;
                } else {
                    uint64_t HighPart;
                    uint64_t V2 = (uint64_t)(value2.qw);
                    uint64_t R;
                    R = _umul128(value1.uqw, V2, &HighPart);
                    if (HighPart > 0) {
                        ret_value->uqw = 0xFFFFFFFFFFFFFFFFULL;            // Limitierung kein berlauf
                    } else {
                        ret_value->uqw = R;
                    }
                    return FLOAT_OR_INT_64_TYPE_UINT64;
                }
            }
        }
    }
}


typedef struct {
   int64_t High;
   uint64_t Low;
} MY_128_INT;

static inline void init128(union FloatOrInt64 Value, int Type, MY_128_INT *Result)
{
    switch (Type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        if (Value.qw < 0) {
            Result->High = -1;
        } else {
            Result->High = 0;
        }
        Result->Low = (uint64_t)Value.qw;
        break;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        Result->High = 0;
        Result->Low = Value.uqw;
        break;
    case FLOAT_OR_INT_64_TYPE_F64:
    default:
        ThrowError(1, "this should never happen");
        Result->High = 0;
        Result->Low = Value.uqw;
        break;
    }
}

static void add128(MY_128_INT *a, MY_128_INT *b, MY_128_INT *Result)
{
    Result->High = a->High + b->High;
    Result->Low = a->Low + b->Low;
    if (Result->Low < a->Low) {
        Result->High++;
    }
}

static void sub128(MY_128_INT *a, MY_128_INT *b, MY_128_INT *Result)
{
    Result->High = a->High + b->High;
    Result->Low = a->Low + b->Low;
    if (Result->Low > a->Low) {
        Result->High--;
    }
}

/*
static inline void inc128(MY_128_INT *a)
{
    uint64_t h = a->Low;
    h++;
    if (h < a->Low) a->High++;
    a->Low = h;
}


static inline void neg128(MY_128_INT *a)
{
    a->High = (int64_t)~((uint64_t)a->High);
    a->Low = ~a->Low;
    inc128(a);
}
*/

static int limit128(MY_128_INT *In, union FloatOrInt64 *Result)
{
    if (In->High < 0) {
        // negativ
        if (In->High < -1) {
            Result->uqw = 0x8000000000000000ULL;
        } else if (In->Low < 0x8000000000000000ULL) {
            Result->uqw = 0x8000000000000000ULL;
        } else {
            Result->uqw = In->Low;   // das ist absichtlich uqw und nicht qw!
        }
        return FLOAT_OR_INT_64_TYPE_INT64;
    } else {
        // postiv
        if (In->High > 0) {
            Result->uqw = 0xFFFFFFFFFFFFFFFFULL;
        } else {
            Result->uqw = In->Low;
        }
        return FLOAT_OR_INT_64_TYPE_UINT64;
    }
}

int add_FloatOrInt64(union FloatOrInt64 value1, int type1, union FloatOrInt64 value2, int type2, union FloatOrInt64 *ret_value)
{
    if ((type1 == FLOAT_OR_INT_64_TYPE_F64) || (type2 == FLOAT_OR_INT_64_TYPE_F64)) {
        ret_value->d = FloatOrInt64_ToDouble(value1, type1) + FloatOrInt64_ToDouble(value2, type2);
        return FLOAT_OR_INT_64_TYPE_F64;
    } else {
        MY_128_INT a128;
        MY_128_INT b128;
        MY_128_INT c128;

        init128(value1, type1, &a128);
        init128(value2, type2, &b128);

        add128(&a128, &b128, &c128);

        return limit128(&c128, ret_value);
    }
}

int sub_FloatOrInt64(union FloatOrInt64 value1, int type1, union FloatOrInt64 value2, int type2, union FloatOrInt64 *ret_value)
{
    if ((type1 == FLOAT_OR_INT_64_TYPE_F64) || (type2 == FLOAT_OR_INT_64_TYPE_F64)) {
        ret_value->d = FloatOrInt64_ToDouble(value1, type1) + FloatOrInt64_ToDouble(value2, type2);
        return FLOAT_OR_INT_64_TYPE_F64;
    } else {
        MY_128_INT a128;
        MY_128_INT b128;
        MY_128_INT c128;

        init128(value1, type1, &a128);
        init128(value2, type2, &b128);

        sub128(&a128, &b128, &c128);

        return limit128(&c128, ret_value);
    }
}

int string_to_FloatOrInt64(char *src_string, union FloatOrInt64 *ret_value)
{
    int Ret;

    if (string_to_bbvari_without_type ((enum BB_DATA_TYPES*)&Ret,
                                       (union BB_VARI*)ret_value,
                                       src_string)) {
        Ret = FLOAT_OR_INT_64_TYPE_INVALID;
    } else {
        switch(Ret) {
        case BB_QWORD:
            Ret = FLOAT_OR_INT_64_TYPE_INT64;
            break;
        case BB_UQWORD:
            Ret = FLOAT_OR_INT_64_TYPE_UINT64;
            break;
        case BB_DOUBLE:
            Ret = FLOAT_OR_INT_64_TYPE_F64;
            break;
        default:
            Ret = FLOAT_OR_INT_64_TYPE_INVALID;
            break;
        }
    }
    return Ret;
}

int FloatOrInt64_to_string(union FloatOrInt64 value, int type, int base, char *dst_string, int maxc)
{
    enum BB_DATA_TYPES help_type;
    union BB_VARI help_value;

    switch(type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        help_type = BB_QWORD;
        help_value.qw = value.qw;
        break;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        help_type = BB_UQWORD;
        help_value.uqw = value.uqw;
        break;
    case FLOAT_OR_INT_64_TYPE_F64:
        help_type = BB_DOUBLE;
        help_value.d = value.d;
        break;
    default:
        StringCopyMaxCharTruncate(dst_string, "0", maxc);
        return -1;
        //break;
    }
    return bbvari_to_string (help_type, help_value, base, dst_string, maxc);
}

int is_equal_FloatOrInt64(double ref_value, union FloatOrInt64 value, int type)
{
    int Ret;
    double diff;
    double v = FloatOrInt64_ToDouble(value, type);
    // das ist wirklich ein Floatingpoint Vergleich auf exakt gleich ohne Warnung!
    diff = ref_value - v;
    if ((diff <= 0.0) && (diff >= 0.0)) Ret = 1;
    else Ret = 0;
    return Ret;
}

double to_double_FloatOrInt64(union FloatOrInt64 value, int type)
{
    return FloatOrInt64_ToDouble(value, type);
}

int to_bool_FloatOrInt64(union FloatOrInt64 value, int type)
{
    return FloatOrInt64_ToBool(value, type);
}

int to_int_FloatOrInt64(union FloatOrInt64 value, int type)
{
    return (int)FloatOrInt64_ToInt64(value, type);
}

void write_bbvari_convert_with_FloatAndInt64_pid_cs(PID pid, VID vid, union FloatOrInt64 value, int type, int cs)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            //ThrowError (1, "rm_write_bbvari_convert_with_FloatAndInt64() missing");
            rm_write_bbvari_convert_with_FloatAndInt64 (pid, vid, value, type);
        }
#endif
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    if (cs) EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {

        switch (blackboard[vid_index].Type) {
        case BB_BYTE:
            blackboard[vid_index].Value.b  = sc_convert_qword2byte(FloatOrInt64_ToInt64(value, type));
            break;
        case BB_UBYTE:
            blackboard[vid_index].Value.ub  = sc_convert_uqword2ubyte(FloatOrInt64_ToUint64(value, type));
            break;
        case BB_WORD:
            blackboard[vid_index].Value.w  = sc_convert_qword2word(FloatOrInt64_ToInt64(value, type));
            break;
        case BB_UWORD:
            blackboard[vid_index].Value.uw  = sc_convert_uqword2uword(FloatOrInt64_ToUint64(value, type));
            break;
        case BB_DWORD:
            blackboard[vid_index].Value.dw  = sc_convert_qword2dword(FloatOrInt64_ToInt64(value, type));
            break;
        case BB_UDWORD:
            blackboard[vid_index].Value.udw  = sc_convert_uqword2udword(FloatOrInt64_ToUint64(value, type));
            break;
        case BB_QWORD:
            blackboard[vid_index].Value.qw  = FloatOrInt64_ToInt64(value, type);
            break;
        case BB_UQWORD:
            blackboard[vid_index].Value.uqw  = FloatOrInt64_ToUint64(value, type);
            break;
        case BB_FLOAT:
            blackboard[vid_index].Value.f  = sc_convert_double2float(FloatOrInt64_ToDouble(value, type));
            break;
        case BB_DOUBLE:
            blackboard[vid_index].Value.d  = FloatOrInt64_ToDouble(value, type);
            break;
        default:
            blackboard[vid_index].Value.uqw = 0;
            break;
        }
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    if (cs) LeaveBlackboardCriticalSection();
}

void write_bbvari_convert_with_FloatAndInt64 (VID vid, union FloatOrInt64 value, int type)
{
    write_bbvari_convert_with_FloatAndInt64_pid_cs (GET_PID(), vid, value, type, 1);
}

void write_bbvari_convert_with_FloatAndInt64_pid(PID pid, VID vid, union FloatOrInt64 value, int type)
{
    write_bbvari_convert_with_FloatAndInt64_pid_cs (pid, vid, value, type, 1);
}

void write_bbvari_convert_with_FloatAndInt64_cs (VID vid, union FloatOrInt64 value, int type, int cs)
{
    write_bbvari_convert_with_FloatAndInt64_pid_cs (GET_PID(), vid, value, type, cs);
}

void write_bbvari_binary_FloatAndInt64_pid_cs(PID pid, VID vid, union FloatOrInt64 value, int type, int cs)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
        return;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }
    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return;
    }
    if (cs) EnterBlackboardCriticalSection();
    // Takeover value only if the process have access rights
    if (blackboard[vid_index].AccessFlags &
        blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
        uint64_t Value = FloatOrInt64_ToUint64(value, type);
        switch (blackboard[vid_index].Type) {
        case BB_BYTE:
            blackboard[vid_index].Value.b  = (int8_t)Value;
            break;
        case BB_UBYTE:
            blackboard[vid_index].Value.ub  = (uint8_t)Value;
            break;
        case BB_WORD:
            blackboard[vid_index].Value.w  = (int16_t)Value;
            break;
        case BB_UWORD:
            blackboard[vid_index].Value.uw  = (uint16_t)Value;
            break;
        case BB_DWORD:
            blackboard[vid_index].Value.dw  =  (int32_t)Value;
            break;
        case BB_UDWORD:
            blackboard[vid_index].Value.udw  = (uint32_t)Value;
            break;
        case BB_QWORD:
            blackboard[vid_index].Value.qw  = (int64_t)Value;
            break;
        case BB_UQWORD:
            blackboard[vid_index].Value.uqw  = Value;
            break;
        case BB_FLOAT:
            *(uint32_t*)&(blackboard[vid_index].Value.f) = (uint32_t)Value;
            break;
        case BB_DOUBLE:
            *(uint64_t*)&(blackboard[vid_index].Value.d) = (uint64_t)Value;
            break;
        default:
            blackboard[vid_index].Value.uqw = 0;
            break;
        }
        blackboard[vid_index].WrFlags = ALL_WRFLAG_MASK;
    }
    if (cs) LeaveBlackboardCriticalSection();
}

void write_bbvari_binary_FloatAndInt64_pid(PID pid, VID vid, union FloatOrInt64 value, int type)
{
    write_bbvari_binary_FloatAndInt64_pid_cs(pid, vid, value, type, 1);
}

void write_bbvari_binary_FloatAndInt64(VID vid, union FloatOrInt64 value, int type)
{
    write_bbvari_binary_FloatAndInt64_pid_cs(GET_PID(), vid, value, type, 1);
}

static int read_bbvari_by_name_x(const char *name, int vid, union FloatOrInt64 *ret_value, int *ret_byte_width, int read_type)
{
    int vid_index;
#ifdef USE_HASH_BB_SEARCH
    uint64_t HashCode;
    int32_t P1, P2;
#endif

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            //ThrowError (1, "rm_write_bbvari_convert_with_FloatAndInt64() missing");
            return rm_read_bbvari_by_name(name, ret_value, ret_byte_width, read_type);
        }
#endif
        return FLOAT_OR_INT_64_TYPE_INVALID; 
    }

    if (name == NULL) {
        // Determine the vid index
        if ((vid_index = get_variable_index(vid)) == -1) {
            return FLOAT_OR_INT_64_TYPE_INVALID;
        } else goto  __VID;
    }
    // schaue ob Variable schon vorhanden
#ifdef USE_HASH_BB_SEARCH
    HashCode = BuildHashCode(name);
    vid_index = BinarySearchIndex(name, HashCode, &P1, &P2);
    if (vid_index >= 0) {
        if (1) {
            if (1) {
                if (1) {

#else
    for (vid_index = 0; vid_index < get_blackboardsize(); vid_index++) {
        if (blackboard[vid_index].pAdditionalInfos != NULL) {
            // Name schon vorhanden
            if (blackboard[vid_index].Vid != -1) {
                if (strcmp(name, (char*)blackboard[vid_index].pAdditionalInfos->Name) == 0) {
#endif
__VID:
                    if (blackboard[vid_index].Type == BB_UNKNOWN_WAIT) {  // noch kein Datentyp angegeben
                        ret_value->uqw = 0;
                        *ret_byte_width = 8;
                        return FLOAT_OR_INT_64_TYPE_INVALID;
                    }
                    switch (read_type) {
                    case READ_TYPE_RAW_VALUE: // Wert
                        switch (blackboard[vid_index].Type) {
                        case BB_BYTE:
                            ret_value->qw = blackboard[vid_index].Value.b;
                            *ret_byte_width = 1;
                            return FLOAT_OR_INT_64_TYPE_INT64;
                        case BB_UBYTE:
                            ret_value->uqw = blackboard[vid_index].Value.ub;
                            *ret_byte_width = 1;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_WORD:
                            ret_value->qw = blackboard[vid_index].Value.w;
                            *ret_byte_width = 2;
                            return FLOAT_OR_INT_64_TYPE_INT64;
                        case BB_UWORD:
                            ret_value->uqw = blackboard[vid_index].Value.uw;
                            *ret_byte_width = 2;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_DWORD:
                            ret_value->qw = blackboard[vid_index].Value.dw;
                            *ret_byte_width = 4;
                            return FLOAT_OR_INT_64_TYPE_INT64;
                        case BB_UDWORD:
                            ret_value->uqw = blackboard[vid_index].Value.udw;
                            *ret_byte_width = 4;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_QWORD:
                            ret_value->qw = blackboard[vid_index].Value.qw;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_INT64;
                        case BB_UQWORD:
                            ret_value->uqw = blackboard[vid_index].Value.uqw;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_FLOAT:
                            ret_value->d = (double)blackboard[vid_index].Value.f;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_F64;
                        case BB_DOUBLE:
                            ret_value->d = blackboard[vid_index].Value.d;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_F64;
                        default:
                            ret_value->uqw = 0;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_INVALID;
                        }
                        //break;
                    case READ_TYPE_PHYS:  // physikalisch
                        {
                            double Value;
                            *ret_byte_width = 8;
                            // Check if conversion type matches
                            //if (blackboard[vid_index].pAdditionalInfos->Conversion.Type != BB_CONV_FORMULA) {
                            //    ret_value->d = 0.0;
                            //    return FLOAT_OR_INT_64_TYPE_INVALID;
                            //}
                            if (bbvari_to_double (blackboard[vid_index].Type,
                                                  blackboard[vid_index].Value, &Value)) {
                                ret_value->d = 0.0;
                                return FLOAT_OR_INT_64_TYPE_INVALID;
                            } else {
                                if (convert_physical_internal(vid_index, Value, &Value)) {
                                    ret_value->d = 0.0;
                                    return FLOAT_OR_INT_64_TYPE_INVALID;
                                }
                                ret_value->d = Value;
                                //ret_value->d = execute_stack_whith_parameter ((struct EXEC_STACK_ELEM *)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode,
                                //                                              Value);
                                return FLOAT_OR_INT_64_TYPE_F64;
                            }
                        }
                        //break;
                    case READ_TYPE_ONS_COMPLEMENT: // Einer Komplement
                        *ret_byte_width = 8;
                        switch (blackboard[vid_index].Type) {
                        case BB_BYTE:
                        case BB_UBYTE:
                            ret_value->uqw = (uint64_t)(~blackboard[vid_index].Value.ub & 0xFF);
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_WORD:
                        case BB_UWORD:
                            ret_value->uqw = (uint64_t)(~blackboard[vid_index].Value.uw & 0xFFFF);
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_DWORD:
                        case BB_UDWORD:
                            ret_value->uqw = (uint64_t)(~blackboard[vid_index].Value.udw);
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_QWORD:
                        case BB_UQWORD:
                            ret_value->uqw = (uint64_t)(~blackboard[vid_index].Value.uqw);
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_FLOAT:
                            ret_value->uqw = (uint64_t)(~((uint64_t)blackboard[vid_index].Value.f));
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_DOUBLE:
                            ret_value->uqw = (uint64_t)(~((uint64_t)blackboard[vid_index].Value.d));
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        default:
                            ret_value->uqw = 0;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_INVALID;
                        }
                        //break;
                    case READ_TYPE_RAW_BINARY: // Wert
                        switch (blackboard[vid_index].Type) {
                        case BB_BYTE:
                            ret_value->uqw = (uint64_t)(uint8_t)blackboard[vid_index].Value.b;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_UBYTE:
                            ret_value->uqw = (uint64_t)blackboard[vid_index].Value.ub;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_WORD:
                            ret_value->uqw = (uint64_t)(uint16_t)blackboard[vid_index].Value.w;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_UWORD:
                            ret_value->uqw = (uint64_t)blackboard[vid_index].Value.uw;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_DWORD:
                            ret_value->uqw = (uint64_t)(uint32_t)blackboard[vid_index].Value.dw;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_UDWORD:
                            ret_value->uqw = (uint64_t)blackboard[vid_index].Value.udw;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_QWORD:
                            ret_value->uqw = (uint64_t)blackboard[vid_index].Value.qw;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_UQWORD:
                            ret_value->uqw = blackboard[vid_index].Value.uqw;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_FLOAT:
                            {
                                uint32_t *ptr = (uint32_t*)&(blackboard[vid_index].Value.f);
                                ret_value->uqw = (uint64_t)ptr[0];
                            }
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        case BB_DOUBLE:
                            {
                                uint64_t *ptr = (uint64_t*)&(blackboard[vid_index].Value.f);
                                ret_value->uqw = ptr[0];
                            }
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_UINT64;
                        default:
                            ret_value->uqw = 0;
                            *ret_byte_width = 8;
                            return FLOAT_OR_INT_64_TYPE_INVALID;
                        }
                        //break;

                    }
                }
            }
        }
    }
    return FLOAT_OR_INT_64_TYPE_INVALID;
}

int read_bbvari_by_name(const char *name, union FloatOrInt64 *ret_value, int *ret_byte_width, int read_type)
{
    int Ret;
    EnterBlackboardCriticalSection();
    Ret = read_bbvari_by_name_x(name, 0, ret_value, ret_byte_width, read_type);
    LeaveBlackboardCriticalSection();
    return Ret;
}

int read_bbvari_by_id(int vid, union FloatOrInt64 *ret_value, int *ret_byte_width, int read_type)
{
    int Ret;
    EnterBlackboardCriticalSection();
    Ret = read_bbvari_by_name_x(NULL, vid, ret_value, ret_byte_width, read_type);
    LeaveBlackboardCriticalSection();
    return Ret;
}


int write_bbvari_by_name(PID pid, const char *name, union FloatOrInt64 value, int type, int bb_type, int add_if_not_exist, int write_type, int cs)
{
    int vid;
    int ret = -1;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            //ThrowError (1, "rm_write_bbvari_convert_with_FloatAndInt64() missing");
            return rm_write_bbvari_by_name(pid, name, value, type, bb_type, add_if_not_exist, write_type);
        }
#endif
        return -1;
    }
    vid = add_bbvari_pid(name, bb_type, "", pid);
    if (vid >= 0) {
        switch (write_type) {
        case WRITE_TYPE_RAW_VALUE:
            write_bbvari_convert_with_FloatAndInt64_pid_cs(pid, vid, value, type, cs);
            ret = 0;
            break;
        case WRITE_TYPE_PHYS:
            ret = write_bbvari_phys_minmax_check_pid_cs (pid, vid, FloatOrInt64_ToDouble(value, type), cs);
            break;
        case WRITE_TYPE_RAW_BINARY:
            write_bbvari_binary_FloatAndInt64_pid_cs (pid, vid, value, type, cs);
            ret = 0;
            break;
        default:
            break;
        }
        if (!add_if_not_exist || (get_process_bbvari_attach_count_pid(vid, pid) > 1)) {
            remove_bbvari(vid);
        }
    }
    return ret;
}

int write_bbvari_by_id(int vid, union FloatOrInt64 value, int type, int bb_type, int add_if_not_exist, int write_type)
{
    UNUSED(bb_type);
    int Ret = -1;
    if (attach_bbvari(vid) > 0) {
        switch (write_type) {
        case WRITE_TYPE_RAW_VALUE:
            write_bbvari_convert_with_FloatAndInt64(vid, value, type);
            Ret = 0;
            break;
        case WRITE_TYPE_PHYS:
            Ret = write_bbvari_phys_minmax_check (vid, FloatOrInt64_ToDouble(value, type));
            break;
        case WRITE_TYPE_RAW_BINARY:
            write_bbvari_binary_FloatAndInt64 (vid, value, type);
            Ret = 0;
            break;
        default:
            break;
        }
        if (!add_if_not_exist || (get_process_bbvari_attach_count_pid(vid, GET_PID()) > 1)) {
            remove_bbvari(vid);
        }
    }
    return Ret;
}


/*************************************************************************
**
**    Function    : convert_value_textreplace
**
**    Description : Textersatz fuer die entsprechende Variable zurueckgeben
**    Parameter   : VID vid - Variablen-ID
**                  long value - Wert fuer den TExtersatz gelten soll
**                  char *txt - Ziel fuer Textersatz
**                  int maxc - Groesse des Zielstrings
**    Returnvalues:
**
*************************************************************************/
int convert_value_textreplace (VID vid, int32_t value, char *txt, int maxc, int *pcolor)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_convert_value_textreplace (vid, value, txt, maxc, pcolor);
        }
#endif
        return -1;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }
    // Check if conversion type matches
    if (blackboard[vid_index].pAdditionalInfos->Conversion.Type != BB_CONV_TEXTREP) {
        return  -1;
    }
    // Convert variable to a text replace
    return convert_value2textreplace (value, (char *)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString,
                                      txt, maxc, pcolor);
}

int convert_textreplace_value (VID vid, char *txt, int64_t *pfrom, int64_t *pto)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
        return -1;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }
    // Check if conversion type matches
    if (blackboard[vid_index].pAdditionalInfos->Conversion.Type != BB_CONV_TEXTREP) {
        return -1;
    }
    // Textersatz in Wert konvertieren
    return convert_textreplace2value ((char *)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString, 
                                      txt, pfrom, pto);
}

int read_bbvari_frame (VID *Vids, int8_t *PhysOrRaw, double *RetFrameValues, int Size)
{
    int x;
    int VidIdx;
    int Ret = 0;


    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_bbvari_frame (Vids, PhysOrRaw, RetFrameValues, Size);
        }
#endif
        return -1;
    }
    EnterBlackboardCriticalSection();
    for (x = 0; x < Size; x++) {
        // Variablenindex ermitteln
        if ((VidIdx = get_variable_index(Vids[x])) == -1) {
            RetFrameValues[x] = 0.0;
            Ret--;
        } else {
            if ((PhysOrRaw == NULL) ||  // all raw value
                (PhysOrRaw[x] == 0)) {   // this value read as raw
                bbvari_to_double (blackboard[VidIdx].Type,
                                 blackboard[VidIdx].Value, &RetFrameValues[x]);
            } else {
                //if (blackboard[VidIdx].pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA) {
                    double Value;
                    if (bbvari_to_double (blackboard[VidIdx].Type,
                                         blackboard[VidIdx].Value, &Value)) {
                        Ret--;
                    } else {
                        if (convert_physical_internal(VidIdx, Value, &Value)) {
                            RetFrameValues[x] = 0.0;
                            Ret--;
                        } else {
                            RetFrameValues[x] = Value;
                        }
                        //RetFrameValues[x] = execute_stack_whith_parameter ((struct EXEC_STACK_ELEM *)blackboard[VidIdx].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode,
                        //                                                  Value);
                    }
                //} else {
                //    Ret--;
                //}
            }
        }
    }
    LeaveBlackboardCriticalSection();
    return Ret;
}

int write_bbvari_frame_pid (PID pid, VID *Vids, int8_t *PhysOrRaw, double *FrameValues, int Size)
{
    int x;
    int VidIdx;
    int Ret = 0;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_write_bbvari_frame(pid, Vids, PhysOrRaw, FrameValues, Size);
        }
#endif
        return -1;
    }
    EnterBlackboardCriticalSection();
    for (x = 0; x < Size; x++) {
        if ((PhysOrRaw == NULL) ||  // all raw value
            (PhysOrRaw[x] == 0)) {   // this value read as raw
            write_bbvari_minmax_check_pid(pid, Vids[x], FrameValues[x]);
        } else {
            if (write_bbvari_phys_minmax_check_pid_cs (pid, Vids[x], FrameValues[x], 0)) {
                Ret--;
            }
#if 0
            if ((VidIdx = get_variable_index(Vids[x])) == -1) {
                Ret--;
            } else {
                if (blackboard[VidIdx].pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA) {
                    union BB_VARI Value;
                    double DoubleValue;
                    if (write_bbvari_phys_minmax_check_inner(VidIdx, FrameValues[x], &Value, &DoubleValue)) {
                        Ret--;
                    }
                    blackboard[VidIdx].WrFlags = ALL_WRFLAG_MASK;
                    blackboard[VidIdx].Value = Value;
                    blackboard[VidIdx].WrFlags = ALL_WRFLAG_MASK;
                } else {
                    Ret--;
                }
            }
#endif
        }
    }
    LeaveBlackboardCriticalSection();
    return Ret;
}
