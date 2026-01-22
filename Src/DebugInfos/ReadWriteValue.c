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
#include "Platform.h"
#include <stdio.h>
#include "Config.h"
#include "PrintFormatToString.h"
#include "Blackboard.h"
#include "DebugInfoDB.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "Scheduler.h"
#include "MainValues.h"
#include "DebugInfoAccessExpression.h"
#include "ThrowError.h"

#include "ReadWriteValue.h"

static int CheckAccess(int par_Pid)
{
    int Ret = 0;

    // If it is a realtime process no lock possible
    if ((s_main_ini_val.ConnectToRemoteMaster) && ((par_Pid & 0x10000000) == 0x10000000)) {
        Ret = 1;
    } else {
        TASK_CONTROL_BLOCK *pTcb;
        pTcb = GetPointerToTaskControlBlock (par_Pid);
        if (pTcb == NULL) {
            ThrowError (1, "cannot read from extern process (%i) the pid is not valid", par_Pid);
            Ret = 0;
        } else if (pTcb->LockedByThreadId != (unsigned int)GetCurrentThreadId()) {
            ThrowError (1, "cannot read from extern process (%i) the current thread is not valid", par_Pid);
            Ret = 0;
        } else {
            Ret = 1;
        }
    }
    return Ret;
}

int ReadValueFromExternProcessViaAddress (uint64_t address, PROCESS_APPL_DATA *pappldata, int pid,
                                          int32_t typenr, union BB_VARI *ret_Value, int *ret_Type, double *ret_DoubleValue)
{
    union BB_VARI bb_vari;
    int Type = 0;
    double DoubleValue = 0.0;
    int Ret = 0;

    bb_vari.uqw = 0;
    if (CheckAccess(pid)) {
        Type = get_base_type_bb_type_ex (typenr, pappldata);
        switch (Type) {
        case BB_BYTE:
            if (scm_read_bytes (address, pid, (char*)&(bb_vari.b), 1) != 1) return -1;
            else DoubleValue = (double)bb_vari.b;
            break;
        case BB_UBYTE:
            if (scm_read_bytes (address, pid, (char*)&(bb_vari.ub), 1) != 1) return -1;
            else DoubleValue = (double)bb_vari.ub;
            break;
        case BB_WORD:
            if (scm_read_bytes (address, pid, (char*)&(bb_vari.w), 2) != 2) return -1;
            else DoubleValue = (double)bb_vari.w;
            break;
        case BB_UWORD:
            if (scm_read_bytes (address, pid, (char*)&(bb_vari.uw), 2) != 2) return -1;
            else DoubleValue = (double)bb_vari.uw;
            break;
        case BB_DWORD:
            if (scm_read_bytes (address, pid, (char*)&(bb_vari.dw), 4) != 4) return -1;
            else DoubleValue = (double)bb_vari.dw;
            break;
        case BB_UDWORD:
            if (scm_read_bytes (address, pid, (char*)&(bb_vari.udw), 4) != 4) return -1;
            else DoubleValue = (double)bb_vari.udw;
            break;
        case BB_QWORD:
            if (scm_read_bytes (address, pid, (char*)&(bb_vari.qw), 8) != 8) return -1;
            else DoubleValue = (double)bb_vari.qw;   // TODO do not convert to double!
            break;
        case BB_UQWORD:
            if (scm_read_bytes (address, pid, (char*)&(bb_vari.uqw), 8) != 8) return -1;
            else DoubleValue = (double)bb_vari.uqw;  // TODO do not convert to double!
            break;
        case BB_FLOAT:
            if (scm_read_bytes (address, pid, (char*)&(bb_vari.f), 4) != 4) return -1;
            else DoubleValue = (double)bb_vari.f;
            break;
        case BB_DOUBLE:
            if (scm_read_bytes (address, pid, (char*)&(bb_vari.d), 8) != 8) return -1;
            else DoubleValue = bb_vari.d;
            break;
        default:
            DoubleValue = 0.0;
            Ret = -2;
            break;
        }
    }
    if (Ret) {
        if (ret_Value != NULL) ret_Value->uqw = 0;
        if (ret_Type != NULL)  *ret_Type = BB_UNKNOWN;
        if (ret_DoubleValue != NULL) *ret_DoubleValue = 0.0;
    } else {
        if (ret_Value != NULL) *ret_Value = bb_vari;
        if (ret_Type != NULL)  *ret_Type = Type;
        if (ret_DoubleValue != NULL) *ret_DoubleValue = DoubleValue;
    }
    return Ret;
}

int WriteValueToExternProcessViaAddress (uint64_t address, PROCESS_APPL_DATA *pappldata, int pid, int *PidSameExe, int PidSameExeCount,
                                         int32_t typenr, union BB_VARI value)
{
    int Type;

    if (CheckAccess(pid)) {
        Type = get_base_type_bb_type_ex (typenr, pappldata);
        switch (Type) {
        case BB_BYTE:
            if (scm_write_bytes_pid_array (address, PidSameExe, PidSameExeCount, (unsigned char*)&(value.b), 1) != 1) return -1;
            break;
        case BB_UBYTE:
            if (scm_write_bytes_pid_array (address, PidSameExe, PidSameExeCount, &(value.ub), 1) != 1) return -1;
            break;
        case BB_WORD:
            if (scm_write_bytes_pid_array (address, PidSameExe, PidSameExeCount, (unsigned char*)&(value.w), 2) != 2) return -1;
            break;
        case BB_UWORD:
            if (scm_write_bytes_pid_array (address, PidSameExe, PidSameExeCount, (unsigned char*)&(value.uw), 2) != 2) return -1;
            break;
        case BB_DWORD:
            if (scm_write_bytes_pid_array (address, PidSameExe, PidSameExeCount, (unsigned char*)&(value.dw), 4) != 4) return -1;
            break;
        case BB_UDWORD:
            if (scm_write_bytes_pid_array (address, PidSameExe, PidSameExeCount, (unsigned char*)&(value.udw), 4) != 4) return -1;
            break;
        case BB_QWORD:
            if (scm_write_bytes_pid_array (address, PidSameExe, PidSameExeCount, (unsigned char*)&(value.qw), 8) != 8) return -1;
            break;
        case BB_UQWORD:
            if (scm_write_bytes_pid_array (address, PidSameExe, PidSameExeCount, (unsigned char*)&(value.uqw), 8) != 8) return -1;
            break;
        case BB_FLOAT:
            if (scm_write_bytes_pid_array (address, PidSameExe, PidSameExeCount, (unsigned char*)&(value.f), 4) != 4) return -1;
            break;
        case BB_DOUBLE:
            if (scm_write_bytes_pid_array (address, PidSameExe, PidSameExeCount, (unsigned char*)&(value.d), 8) != 8) return -1;
            break;
        default:
            return -2;
        }
        return 0;
    }
    return -1;
}

int ReadStringFromExternProcessViaLabel (PROCESS_APPL_DATA *pappldata, int Pid, char *Dst, int MaxSize, char *Label)
{
    char Help[1024];
    uint64_t Address;
    int32_t TypeNr;
    double Value;
    int x;

    if (CheckAccess(Pid)) {
        for (x = 0; x < (MaxSize-1); x++) {
            PrintFormatToString (Help, sizeof(Help), "%s[%i]", Label, x);
            if (!appl_label (pappldata, Pid, Help, &Address, &TypeNr)) { // is it a label?
                if ((TypeNr == BB_BYTE) || (TypeNr == BB_UBYTE)) {  // String must consist of bytes
                    if (!ReadValueFromExternProcessViaAddress (Address, pappldata, Pid, TypeNr, NULL, NULL,&Value)) {
                        ((unsigned char*)Dst)[x] = (unsigned char)Value;
                        if (Dst[x] == 0) return 0;
                        continue;
                    }
                    Dst[x] = 0;
                    return -1;   // Error (no response from external process)
                }
                Dst[x] = 0;
                return -2;   // Error (label has not the data type char or unsigned char array)
            }
            Dst[x] = 0;
            return -3;   // Error (label doesn't exist)
        }
        Dst[x] = 0;
        return -4;   // Error (string is longer than MaxSize-1)
    }
    return -1;
}


int ReadValueFromExternProcessViaLabel (PROCESS_APPL_DATA *pappldata, int Pid, double *pValue, char *Label)
{
    uint64_t Address;
    int32_t TypeNr;
    double Value;

    if (!appl_label (pappldata, Pid, Label, &Address, &TypeNr)) { // is it a label?
        if (!ReadValueFromExternProcessViaAddress (Address, pappldata, Pid, TypeNr, NULL, NULL, &Value)) {
            *pValue = Value;
            return 0;
        }
        return -1;   // Error (no response from external process)
    }
    return -3;   // Error (label doesn't exist)
}


int WriteValuesToExternProcessViaAddressStopScheduler (PROCESS_APPL_DATA *par_DebugInfos, int par_Pid, int par_Count,
                                                       uint64_t *par_Addresses, int32_t *par_TypeNrs, union BB_VARI *par_Values,
                                                       int par_Timeout, int par_ErrorMessageFlag)
{
    int x;
    int Ret = -1;
    int PidSameExe[16];
    int PidSameExeCount;

    if (WaitUntilProcessIsNotActiveAndThanLockItEx (par_Pid, par_Timeout,
                                                    (par_ErrorMessageFlag) ? ERROR_BEHAVIOR_ERROR_MESSAGE : ERROR_BEHAVIOR_NO_ERROR_MESSAGE,
                                                    "write to external process memory", PidSameExe, &PidSameExeCount, 16,
                                                    __FILE__, __LINE__) == 0) {
        for (x = 0; x < par_Count; x++) {
            Ret = WriteValueToExternProcessViaAddress (par_Addresses[x], par_DebugInfos, par_Pid, PidSameExe, PidSameExeCount,
                                                       par_TypeNrs[x], par_Values[x]);
            if (Ret != 0) break;
        }
        UnLockProcess (par_Pid);
    }
    return Ret;
}


int ReadValuesFromExternProcessViaAddressStopScheduler (PROCESS_APPL_DATA *par_DebugInfos, int par_Pid, int par_Count,
                                                        uint64_t *par_Addresses, int32_t *par_TypeNrs, double **ret_Values,
                                                        int par_Timout, int par_ErrorMessageFlag)
{
    int x;
    int Ret = -1;

    if (WaitUntilProcessIsNotActiveAndThanLockIt (par_Pid, par_Timout,
                                                  (par_ErrorMessageFlag) ? ERROR_BEHAVIOR_ERROR_MESSAGE : ERROR_BEHAVIOR_NO_ERROR_MESSAGE,
                                                  "read from external process memory", __FILE__, __LINE__) == 0) {
        for (x = 0; x < par_Count; x++) {
            Ret = ReadValueFromExternProcessViaAddress (par_Addresses[x], par_DebugInfos, par_Pid,
                                                        par_TypeNrs[x], NULL, NULL, ret_Values[x]);
            if (Ret != 0) break;
        }
        UnLockProcess (par_Pid);
    }
    return Ret;
}


int ReadBytesFromExternProcessStopScheduler (int par_Pid,
                                             uint64_t par_StartAddresse, int par_Count, void *ret_Buffer,
                                             int par_Timeout, int par_ErrorMessageFlag)
{
    int Ret = -1;

    if (WaitUntilProcessIsNotActiveAndThanLockIt (par_Pid, par_Timeout,
                                                  (par_ErrorMessageFlag) ? ERROR_BEHAVIOR_ERROR_MESSAGE : ERROR_BEHAVIOR_NO_ERROR_MESSAGE,
                                                  "read from external process memory", __FILE__, __LINE__) == 0) {
        Ret = scm_read_bytes (par_StartAddresse, par_Pid, (char*)ret_Buffer, par_Count);
        UnLockProcess (par_Pid);
        return Ret;
    }
    return Ret;
}


int WriteBytesToExternProcessStopScheduler (int par_Pid,
                                            uint64_t par_StartAddresse, int par_Count, void *par_Buffer,
                                            int par_Timeout, int par_ErrorMessageFlag)
{
    int Ret = -1;

    if (WaitUntilProcessIsNotActiveAndThanLockIt (par_Pid, par_Timeout,
                                                  (par_ErrorMessageFlag) ? ERROR_BEHAVIOR_ERROR_MESSAGE : ERROR_BEHAVIOR_NO_ERROR_MESSAGE,
                                                  "read from external process memory", __FILE__, __LINE__) == 0) {
        Ret = scm_write_bytes (par_StartAddresse, par_Pid, (unsigned char*)par_Buffer, par_Count);
        UnLockProcess (par_Pid);
        return Ret;
    }
    return Ret;
}

