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


#include <inttypes.h>
#include <limits.h>
#ifdef REMOTE_MASTER
#include <alloca.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <ctype.h>
#define MAXFLOAT    FLT_MAX
#ifdef REMOTE_MASTER
#include <semaphore.h>
#else
#include "Platform.h"
#endif
#include <malloc.h>

#define BLACKBOARD_C

#include "Config.h"
#include "MemZeroAndCopy.h"
#include "PrintFormatToString.h"
#include "BlackboardHashIndex.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "EquationParser.h"
#include "ExecutionStack.h"
#include "TextReplace.h"
#include "BlackboardIniCache.h"
#ifdef REMOTE_MASTER
#else
#include "IniFileDontExist.h"
#endif
#include "ExecutionStack.h"
#ifdef REMOTE_MASTER
#else
#include "EquationList.h"
#endif
#include "ThrowError.h"
#include "Scheduler.h"
#ifdef REMOTE_MASTER
extern char ini_path[];
#else
#include "Files.h"
#include "IniDataBase.h"
#include "ScBbCopyLists.h"
#endif
#include "MyMemory.h"
#include "StringMaxChar.h"
#ifdef REMOTE_MASTER
#include "RemoteMasterReqToClient.h"
#define PIPE_API_REFERENCE_VARIABLE_DIR_READ               0x00000001
#define PIPE_API_REFERENCE_VARIABLE_DIR_WRITE              0x00000002
#else

#include "MainValues.h"
#include "DebugInfoDB.h"
#include "GetNextStructEntry.h"
#include "ScriptMessageFile.h"
#include "RemoteMasterBlackboard.h"
#endif
#include "StructsRM_Blackboard.h"

// Remark: do not include XilEnvExtProc.h define DATA_TYPES_WAIT here but it have to be the same as inside XilEnvExtProc.h
#define DATA_TYPES_WAIT                        32

#define UNUSED(x) (void)(x)

#ifdef REMOTE_MASTER
#define MAX_PATH 260

#include "RemoteMasterLock.h"

static REMOTE_MASTER_LOCK BlackboardCriticalSection;
#define InitializeCriticalSection(x) RemoteMasterInitLock(x)
#define EnterCriticalSection(x)  RemoteMasterLock(x, __LINE__, __FILE__)
#define LeaveCriticalSection(x)  RemoteMasterUnlock(x)

struct {
    int ReplaceAllNoneAsapCombatibleChars;
    int AsapCombatibleLabelnames;
    int DefaultMinMaxValues;
    double MaxMaxValues;
    double MinMinValues;
} s_main_ini_val;

// Remark: do not include EquationList.h define EQU_TYPE_BLACKBOARD here but it have to be the same as inside EquationList.h
#define EQU_TYPE_BLACKBOARD             0x01

#define RGB(r,g,b)          ((int32_t)(((int8_t)(r)|((int16_t)((int8_t)(g))<<8))|(((int32_t)(int8_t)(b))<<16)))

#else

static CRITICAL_SECTION BlackboardCriticalSection;

#endif

void EnterBlackboardCriticalSection(void)
{
    EnterCriticalSection(&BlackboardCriticalSection);
}

void LeaveBlackboardCriticalSection(void)
{
    LeaveCriticalSection(&BlackboardCriticalSection);
}

static void (*ObserationCallbackFunction) (VID Vid, uint32_t ObservationFlags, uint32_t ObservationData);

#ifdef REMOTE_MASTER
#define CHECK_OBSERVATION(index, ObservationMask) \
if (1) { \
    uint32_t Help = blackboard[index].ObservationFlags & ObservationMask; \
    if (Help != 0) { \
        if (ObserationCallbackFunction != NULL) { \
            rm_ObserationCallbackFunction (ObserationCallbackFunction, blackboard[index].Vid, Help, blackboard[index].pAdditionalInfos->ObservationData); \
        } \
    } \
}

#define CHECK_ADD_REMOVE_OBSERVATION(Vid, ObservationMask) \
if (1) { \
    uint32_t Help = blackboard_infos.ObservationFlags & ObservationMask; \
    if (Help != 0) { \
        if (ObserationCallbackFunction != NULL) { \
            rm_ObserationCallbackFunction (ObserationCallbackFunction, Vid, Help, blackboard_infos.ObservationData); \
        } \
    } \
}

#else
#define CHECK_OBSERVATION(index, ObservationMask) \
if (1) { \
    uint32_t Help = blackboard[index].ObservationFlags & ObservationMask; \
    if (Help != 0) { \
        if (ObserationCallbackFunction != NULL) { \
            ObserationCallbackFunction (blackboard[index].Vid, Help, blackboard[index].pAdditionalInfos->ObservationData); \
        } \
    } \
}


#define CHECK_ADD_REMOVE_OBSERVATION(Vid, ObservationMask) \
if (1) { \
    uint32_t Help = blackboard_infos.ObservationFlags & ObservationMask; \
    if (Help != 0) { \
        if (ObserationCallbackFunction != NULL) { \
            ObserationCallbackFunction (Vid, Help, blackboard_infos.ObservationData); \
        } \
    } \
}
#endif

#ifdef REMOTE_MASTER
void StartTimeMeasurement(void);
void StopTimeMeasurement(void);
#else
#define StartTimeMeasurement()
#define StopTimeMeasurement()
#endif

void SetObserationCallbackFunction (void (*Callback)(VID Vid, uint32_t ObservationFlags, uint32_t ObservationData))
{
    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_SetObserationCallbackFunction (Callback);
            return;
        }
#endif
    }
    ObserationCallbackFunction = Callback;
}

int SetBbvariObservation (VID Vid, uint32_t ObservationFlags, uint32_t ObservationData)
{
    int vid_index;

    // is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_SetBbvariObservation (Vid, ObservationFlags, ObservationData);
        }
#endif
        return NOT_INITIALIZED;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
    // Determine the vid index
    if ((vid_index = get_variable_index(Vid)) == -1) {
        LeaveCriticalSection (&BlackboardCriticalSection);
        return -1;
    }
    blackboard[vid_index].ObservationFlags = ObservationFlags;
    if (blackboard[vid_index].pAdditionalInfos != NULL) {
        if ((ObservationData & OBSERVE_RESET_FLAGS) == OBSERVE_RESET_FLAGS) {
            // Reset
            blackboard[vid_index].pAdditionalInfos->ObservationData &= ~(ObservationData & ~OBSERVE_RESET_FLAGS);
        } else {
            // Set
            blackboard[vid_index].pAdditionalInfos->ObservationData |= (ObservationData & ~OBSERVE_RESET_FLAGS);
        }
    }
    LeaveCriticalSection (&BlackboardCriticalSection);
    return 0;
}

int SetGlobalBlackboradObservation (uint32_t ObservationFlags, uint32_t ObservationData, int32_t **ret_Vids, int32_t *ret_Count)
{
    // is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_SetGlobalBlackboradObservation (ObservationFlags, ObservationData, ret_Vids, ret_Count);
        }
#endif
        return NOT_INITIALIZED;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
    blackboard_infos.ObservationFlags = ObservationFlags & (OBSERVE_ADD_VARIABLE | OBSERVE_REMOVE_VARIABLE | OBSERVE_TYPE_CHANGED | OBSERVE_CONVERSION_TYPE_CHANGED);
    blackboard_infos.ObservationData = ObservationData;
    if ((ret_Vids != NULL) && (ret_Count != NULL)) {
        *ret_Vids = get_all_bbvari_ids_without_lock (GET_ALL_BBVARI_EXISTING_TYPE | GET_ALL_BBVARI_UNKNOWN_WAIT_TYPE, -1, ret_Count);
    }
    LeaveCriticalSection (&BlackboardCriticalSection);
    return 0;
}

int init_blackboard (int blackboard_size, char CopyBB2ProcOnlyIfWrFlagSet, char AllowBBVarsWithSameAddr, char conv_error2message)
{
    int     index;
    int     MaxBBSize = (1 << 23) - 1;

    // This have to be done even if the remote master is active
    InitializeCriticalSection (&BlackboardCriticalSection);

#ifdef REMOTE_MASTER
    InitBlackboardIniCache();
#else
    if (s_main_ini_val.ConnectToRemoteMaster) {
        int Ret;
        Ret = rm_init_blackboard (blackboard_size, CopyBB2ProcOnlyIfWrFlagSet, AllowBBVarsWithSameAddr, conv_error2message);
        if (Ret == 0) rm_InitVariableSectionCache();
        return Ret;
    }
#endif

    // Blackboard already exist
    if (blackboard != NULL) {
        return -1;
    }

    if (blackboard_size > MaxBBSize) {
        ThrowError (ERROR_NO_STOP, "blackboard size %i larger than %i limit the size to %i", blackboard_size, MaxBBSize, MaxBBSize);
        blackboard_size = MaxBBSize;
    }
    // we alloc one element more as neccessary so we can detect write to blackboard with index -1!
    // Zeiger auf Speicherbereich ermitteln
    blackboard = (BB_VARIABLE*)my_calloc ((size_t)(blackboard_size + 1), sizeof (BB_VARIABLE));
    if (blackboard == NULL) {
        return -1;
    }
    MEMSET (blackboard, 0xFF, sizeof (BB_VARIABLE));

    blackboard++; // use not the element 0 we have allocated one more as neccessary

    blackboard_infos.Size = blackboard_size;
    blackboard_infos.NumOfVaris = 0;
    blackboard_infos.ActualCycleNumber = 0;

    // Init all Vids to -1
    for (index = 0; index < get_blackboardsize(); index++) {
        blackboard[index].Vid = -1;
    }

    blackboard_infos.pid_access_masks[0] = 1;
    blackboard_infos.pid_wr_masks[0] = 1;
    blackboard_infos.pid_attachcount[0] = 1;

#ifdef USE_HASH_BB_SEARCH
    if (InitMasterHashTable(blackboard_size)) {
        return -1;
    }
#endif
    return 0;
}

#define MAX_COLOR_STRING_LEN  32

int write_varinfos_to_ini (BB_VARIABLE *sp_vari_elem,
                           uint32_t WriteReqFlags)
{
#ifdef REMOTE_MASTER
    int Ret;
    //Ret = rm_write_varinfos_to_ini(sp_vari_elem);
    Ret = BlackboardIniCache_AddEntry(sp_vari_elem->pAdditionalInfos->Name, sp_vari_elem->pAdditionalInfos->Unit, sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString,
                                      sp_vari_elem->pAdditionalInfos->Min, sp_vari_elem->pAdditionalInfos->Max, sp_vari_elem->pAdditionalInfos->Step,
                                      sp_vari_elem->pAdditionalInfos->Width, sp_vari_elem->pAdditionalInfos->Prec, sp_vari_elem->pAdditionalInfos->StepType, sp_vari_elem->pAdditionalInfos->Conversion.Type,
                                      sp_vari_elem->pAdditionalInfos->RgbColor, WriteReqFlags);
    return Ret;
#else
    // Do not write anything to the INI file
    if (s_main_ini_val.DontWriteBlackbardVariableInfosToIni) return 0;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        return rm_WriteToBlackboardIniCache(sp_vari_elem, WriteReqFlags);
    }

    char stack_buffer[INI_MAX_LINE_LENGTH];   // max. line legth inside an INI file
    char *tmp_var_str;
    size_t len_first_part;
    size_t len_convertion;
    int HeapBufferFlag = 0;
    int Red, Green, Blue;
    int Ret;

    // is a valid typ defined
    if (((sp_vari_elem->Type < BB_BYTE) || (sp_vari_elem->Type > BB_DOUBLE)) &&
         (sp_vari_elem->Type != BB_QWORD) && (sp_vari_elem->Type != BB_UQWORD)) {
        return -1;
    }

    tmp_var_str = stack_buffer;
    // Build a string with all variable informations
    len_first_part = (size_t)PrintFormatToString (stack_buffer, sizeof(stack_buffer),
                              "%d,%s,%.18g,%.18g,%d,%d,%d,%lf,%d,",
                              (int)sp_vari_elem->Type, sp_vari_elem->pAdditionalInfos->Unit,
                              sp_vari_elem->pAdditionalInfos->Min,
                              sp_vari_elem->pAdditionalInfos->Max,
                              (int)sp_vari_elem->pAdditionalInfos->Width, (int)sp_vari_elem->pAdditionalInfos->Prec,
                              (int)sp_vari_elem->pAdditionalInfos->StepType, sp_vari_elem->pAdditionalInfos->Step,
                              (int)sp_vari_elem->pAdditionalInfos->Conversion.Type);
    switch(sp_vari_elem->pAdditionalInfos->Conversion.Type) {
    default:
    case BB_CONV_NONE:
        len_convertion = 0;
        break;
    case BB_CONV_FORMULA:
    case BB_CONV_TEXTREP:
    case BB_CONV_REF:
        if (sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString == NULL) {
            len_convertion = 0;
        } else {
            len_convertion = strlen (sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString);
        }
        if ((len_first_part + len_convertion + MAX_COLOR_STRING_LEN) < INI_MAX_LINE_LENGTH) {   // +32 for color value
            // Fits into the stack buffer
            HeapBufferFlag = 0;
            tmp_var_str = stack_buffer;
        } else {
            // Do not fit into stack buffer setup an heap buffer
            HeapBufferFlag = 1;
            tmp_var_str = my_malloc (len_first_part + len_convertion + MAX_COLOR_STRING_LEN);
            if (tmp_var_str == NULL) {
                return -1;
            }
            MEMCPY (tmp_var_str, stack_buffer, len_first_part);
        }
        if (len_convertion > 0) {
            MEMCPY (tmp_var_str + len_first_part,
                   sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString,
                   len_convertion);
        }
        break;
    case BB_CONV_FACTOFF:
        len_convertion = PrintFormatToString (tmp_var_str + len_first_part, sizeof(stack_buffer) - len_first_part, "%.18g:%.18g",
                                  sp_vari_elem->pAdditionalInfos->Conversion.Conv.FactorOffset.Factor,
                                  sp_vari_elem->pAdditionalInfos->Conversion.Conv.FactorOffset.Offset);
        break;
    case BB_CONV_OFFFACT:
        len_convertion = PrintFormatToString (tmp_var_str + len_first_part, sizeof(stack_buffer) - len_first_part, "%.18g:%.18g",
                                 sp_vari_elem->pAdditionalInfos->Conversion.Conv.FactorOffset.Offset,
                                 sp_vari_elem->pAdditionalInfos->Conversion.Conv.FactorOffset.Factor);
        break;
    case BB_CONV_TAB_INTP:
    case BB_CONV_TAB_NOINTP:
    {
        int x, MaxSize, Pos;
        // calc the max. neede size
        MaxSize = sp_vari_elem->pAdditionalInfos->Conversion.Conv.Table.Size * 64; // 64 chars for to double numbers
        if ((len_first_part + MaxSize + MAX_COLOR_STRING_LEN) < INI_MAX_LINE_LENGTH) {   // +32 for color value
            // Fits into the stack buffer
            HeapBufferFlag = 0;
            tmp_var_str = stack_buffer;
        } else {
            // Do not fit into stack buffer setup an heap buffer
            HeapBufferFlag = 1;
            tmp_var_str = my_malloc (len_first_part + MaxSize + MAX_COLOR_STRING_LEN);
            if (tmp_var_str == NULL) {
                return -1;
            }
            MEMCPY (tmp_var_str, stack_buffer, len_first_part);
        }
        len_convertion = 0;
        for (x = 0; x < sp_vari_elem->pAdditionalInfos->Conversion.Conv.Table.Size; x++) {
            len_convertion += PrintFormatToString (tmp_var_str + len_first_part + len_convertion, 64, (x == 0) ? "%.18g/%.18g" : ":%.18g/%.18g",
                            sp_vari_elem->pAdditionalInfos->Conversion.Conv.Table.Values[x].Phys,
                            sp_vari_elem->pAdditionalInfos->Conversion.Conv.Table.Values[x].Raw);
        }
        break;
    }
    case BB_CONV_RAT_FUNC:
        len_convertion = PrintFormatToString (tmp_var_str + len_first_part, sizeof(stack_buffer) - len_first_part, "%.18g:%.18g:%.18g:%.18g:%.18g:%.18g",
                                 sp_vari_elem->pAdditionalInfos->Conversion.Conv.RatFunc.a,
                                 sp_vari_elem->pAdditionalInfos->Conversion.Conv.RatFunc.b,
                                 sp_vari_elem->pAdditionalInfos->Conversion.Conv.RatFunc.c,
                                 sp_vari_elem->pAdditionalInfos->Conversion.Conv.RatFunc.d,
                                 sp_vari_elem->pAdditionalInfos->Conversion.Conv.RatFunc.e,
                                 sp_vari_elem->pAdditionalInfos->Conversion.Conv.RatFunc.f);
        break;
    }

    if (sp_vari_elem->pAdditionalInfos->RgbColor < 0) {
        Red = Green = Blue = -1;
    } else {
        Red = GetRValue(sp_vari_elem->pAdditionalInfos->RgbColor);
        Green = GetGValue(sp_vari_elem->pAdditionalInfos->RgbColor);
        Blue = GetBValue(sp_vari_elem->pAdditionalInfos->RgbColor);
    }
    PrintFormatToString (tmp_var_str + len_first_part + len_convertion, MAX_COLOR_STRING_LEN, ",(%d,%d,%d)",
             Red, Blue, Green);

    // Write string to INI file
    if (!IniFileDataBaseWriteString(VARI_SECTION,
                                   (char*)sp_vari_elem->pAdditionalInfos->Name,
                                    tmp_var_str,
                                    GetMainFileDescriptor())) {
        Ret = -1;
    } else {
        Ret = 0;
    }

    if (HeapBufferFlag) my_free (tmp_var_str);

    return Ret;
#endif
}

#ifdef REMOTE_MASTER
int read_varinfos_from_ini(const char *sz_varname,
                           BB_VARIABLE *sp_vari_elem,
                           int type,
                           uint32_t ReadFromIniReqMask)

{
    //int Ret;
    //Ret = rm_read_varinfos_from_ini(sp_vari_elem->Vid, sp_vari_elem->pAdditionalInfos->Name, ReadFromIniReqMask);
    BBVARI_INI_CACHE_ENTRY *Entry = BlackboardIniCache_GetEntryRef(sz_varname);
    if (Entry != NULL) {
        if ((ReadFromIniReqMask & READ_UNIT_BBVARI_FROM_INI) == READ_UNIT_BBVARI_FROM_INI) {
            if (BBWriteUnit(Entry->Unit, sp_vari_elem) != 0) {
                // No memory available
                return BB_VAR_ADD_INFOS_MEM_ERROR;
            }
        }
        if ((ReadFromIniReqMask & READ_MIN_BBVARI_FROM_INI) == READ_MIN_BBVARI_FROM_INI) {
            sp_vari_elem->pAdditionalInfos->Min = Entry->Min;
        }
        if ((ReadFromIniReqMask & READ_MAX_BBVARI_FROM_INI) == READ_MAX_BBVARI_FROM_INI) {
            sp_vari_elem->pAdditionalInfos->Min = Entry->Min;
        }
        if ((ReadFromIniReqMask & READ_STEP_BBVARI_FROM_INI) == READ_STEP_BBVARI_FROM_INI) {
            sp_vari_elem->pAdditionalInfos->Step = Entry->Step;
            sp_vari_elem->pAdditionalInfos->StepType = Entry->StepType;
        }
        if ((ReadFromIniReqMask & READ_WIDTH_PREC_BBVARI_FROM_INI) == READ_WIDTH_PREC_BBVARI_FROM_INI) {
            sp_vari_elem->pAdditionalInfos->Prec = Entry->Prec;
            sp_vari_elem->pAdditionalInfos->Width = Entry->Width;
        }
        if ((ReadFromIniReqMask & READ_CONVERSION_BBVARI_FROM_INI) == READ_CONVERSION_BBVARI_FROM_INI) {
            sp_vari_elem->pAdditionalInfos->Conversion.Type = Entry->ConversionType;
            if (Entry->Conversion != NULL) {
                if (sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA) {
                    if (BBWriteFormulaString(Entry->Conversion, sp_vari_elem) != 0) {
                        // No memory available
                        sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
                        return BB_VAR_ADD_INFOS_MEM_ERROR;
                    }
                    // Gleichung umwandeln
                    if (sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA) {
                        if (equ_src_to_bin(sp_vari_elem) == -1) {
                            sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
                            sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode[0] = 0;
                        }
                    }
                }
                else if (sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_TEXTREP) {
                    if (BBWriteEnumString(Entry->Conversion, sp_vari_elem) != 0) {
                        // No memory available
                        sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
                        return BB_VAR_ADD_INFOS_MEM_ERROR;
                    }
                }
                else {
                    sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
                }
            } else {
                sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
            }
        }
        if ((ReadFromIniReqMask & READ_COLOR_BBVARI_FROM_INI) == READ_COLOR_BBVARI_FROM_INI) {
            sp_vari_elem->pAdditionalInfos->RgbColor = Entry->RgbColor;
        }
        return 0;
    }
    set_default_varinfo(sp_vari_elem, type);
    return 0;
}

#else

int read_varinfos_from_ini (const char *sz_varname,
                            BB_VARIABLE *sp_vari_elem,
                            int type,
                            uint32_t ReadFromIniReqMask)
{
    UNUSED(ReadFromIniReqMask);
    int   error;
    char *tmp_var_str;
    BB_VARIABLE s_vari_elem;
    char unit_save[BBVARI_UNIT_SIZE];

    if (strlen ((const char*)sp_vari_elem->pAdditionalInfos->Unit)) {
        StringCopyMaxCharTruncate (unit_save, (const char*)sp_vari_elem->pAdditionalInfos->Unit, BBVARI_UNIT_SIZE);
    } else unit_save[0] = 0;

    if ((tmp_var_str = IniFileDataBaseReadStringBufferNoDef (VARI_SECTION, sz_varname, GetMainFileDescriptor())) == NULL) {
        set_default_varinfo (sp_vari_elem, type);
        return 0;
    }
    if ((error = set_varinfo_to_inientrys (sp_vari_elem, tmp_var_str)) != 0) {
        set_default_varinfo (sp_vari_elem, type);
        IniFileDataBaseReadStringBufferFree(tmp_var_str);
        return error;
    }
    IniFileDataBaseReadStringBufferFree(tmp_var_str);

    // Make a copy of the variable
    MEMCPY (&s_vari_elem, sp_vari_elem, sizeof(BB_VARIABLE));

    if (strlen (unit_save)) {
        if (BBWriteUnit(unit_save, sp_vari_elem) != 0) {
          // There is no memory available
          return BB_VAR_ADD_INFOS_MEM_ERROR;
        }
    }
    // Translate the conversion
    if (sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA) {
        if (equ_src_to_bin(sp_vari_elem) == -1) {
            s_vari_elem.pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
            s_vari_elem.pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode[0] = 0;
        }
    }

    // Write back the variable to the INI file
    write_varinfos_to_ini (&s_vari_elem, INI_CACHE_ENTRY_FLAG_ALL_INFOS);

    return 0;
}
#endif

int get_bb_accessmask (PID pid, uint64_t *mask, char *BBPrefix)
{
    int pid_index;

    // is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bb_accessmask (pid, mask, BBPrefix);
        }
#endif
        return NOT_INITIALIZED;
    }
    pid_index = get_process_index(pid);

    blackboard_infos.pid_access_masks[pid_index] = pid;
    blackboard_infos.pid_wr_masks[pid_index] = pid;
    blackboard_infos.pid_attachcount[pid_index] = pid;
    if (BBPrefix == NULL) {
        blackboard_infos.ProcessBBPrefix[pid_index][0] = 0;
    } else {
        strncpy (blackboard_infos.ProcessBBPrefix[pid_index], BBPrefix, 31);
        blackboard_infos.ProcessBBPrefix[pid_index][31] = 0;
    }
    // The maske is a bit position based of the process index (max. 64 running processes)
    *mask = 1ULL << pid_index;

    return 0;
}

int free_bb_accessmask (PID pid, uint64_t mask)
{
    int pid_index, i;

    // is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
        return NOT_INITIALIZED;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
    // Search the process
    pid_index = get_process_index(pid);

    // Delete Process-Id and accessmask
    blackboard_infos.pid_access_masks[pid_index] = 0;
    blackboard_infos.pid_wr_masks[pid_index] = 0;
    blackboard_infos.pid_attachcount[pid_index] = 0;

    // Reset the process flag of all variables
    for (i = 0; i < get_blackboardsize(); i++) {
        if (blackboard[i].Vid >= 0) {
            blackboard[i].AccessFlags &= (~mask);
            blackboard[i].RangeControlFlag &= (~mask);
            blackboard[i].WrEnableFlags &= (~mask);
            blackboard[i].WrFlags &= (~mask);
        }
    }
    LeaveCriticalSection (&BlackboardCriticalSection);
    return 0;
}


int IfUnknowDataTypeConvert (int UnknwonDataType, int *ret_DataType)
{
    switch (UnknwonDataType & 0xFF) {
    case BB_UNKNOWN_BYTE:
        *ret_DataType = BB_BYTE;
        return 1;
    case BB_UNKNOWN_UBYTE:
        *ret_DataType = BB_UBYTE;
        return 1;
    case BB_UNKNOWN_WORD:
        *ret_DataType = BB_WORD;
        return 1;
    case BB_UNKNOWN_UWORD:
        *ret_DataType = BB_UWORD;
        return 1;
    case BB_UNKNOWN_DWORD:
        *ret_DataType = BB_DWORD;
        return 1;
    case BB_UNKNOWN_UDWORD:
        *ret_DataType = BB_UDWORD;
        return 1;
    case BB_UNKNOWN_QWORD:
        *ret_DataType = BB_QWORD;
        return 1;
    case BB_UNKNOWN_UQWORD:
        *ret_DataType = BB_UQWORD;
        return 1;
    case BB_UNKNOWN_FLOAT:
        *ret_DataType = BB_FLOAT;
        return 1;
    case BB_UNKNOWN_DOUBLE:
        *ret_DataType = BB_DOUBLE;
        return 1;
    default:
        *ret_DataType = UnknwonDataType;
        return 0;
    }
}

int IfUnknowWaitDataTypeConvert (int UnknwonDataType, int *ret_DataType)
{
    switch (UnknwonDataType & 0xFF) {
    case BB_UNKNOWN_WAIT_BYTE:
        *ret_DataType = BB_BYTE;
        return 1;
    case BB_UNKNOWN_WAIT_UBYTE:
        *ret_DataType = BB_UBYTE;
        return 1;
    case BB_UNKNOWN_WAIT_WORD:
        *ret_DataType = BB_WORD;
        return 1;
    case BB_UNKNOWN_WAIT_UWORD:
        *ret_DataType = BB_UWORD;
        return 1;
    case BB_UNKNOWN_WAIT_DWORD:
        *ret_DataType = BB_DWORD;
        return 1;
    case BB_UNKNOWN_WAIT_UDWORD:
        *ret_DataType = BB_UDWORD;
        return 1;
    case BB_UNKNOWN_WAIT_QWORD:
        *ret_DataType = BB_QWORD;
        return 1;
    case BB_UNKNOWN_WAIT_UQWORD:
        *ret_DataType = BB_UDWORD;
        return 1;
    case BB_UNKNOWN_WAIT_FLOAT:
        *ret_DataType = BB_FLOAT;
        return 1;
    case BB_UNKNOWN_WAIT_DOUBLE:
        *ret_DataType = BB_DOUBLE;
        return 1;
    default:
        *ret_DataType = UnknwonDataType;
        return 0;
    }
}

#ifdef USE_HASH_BB_SEARCH
int BinarySearchIndex(const char *Name, uint64_t HashCode, int32_t *ret_p1, int32_t *ret_p2)
{
    int Found = 0;
    int index;
    int32_t P1, P2;

    index = BinaryHashSerchIndex(HashCode, &P1, &P2);
    if (index >= 0) {
        int32_t SaveP1, SaveP2;
        if (strcmp(Name, (char*)blackboard[index].pAdditionalInfos->Name) == 0) {
            Found = 1;
        } else {
            SaveP1 = P1;
            SaveP2 = P2;
            while ((index = GetNextIndexBeforeSameHash(HashCode, &P1, &P2)) >= 0) {
                if (strcmp(Name, (char*)blackboard[index].pAdditionalInfos->Name) == 0) {
                    Found = 1;
                    break;
                }
            }
            if (!Found) {
                P1 = SaveP1;
                P2 = SaveP2;
                while ((index = GetNextIndexBehindSameHash(HashCode, &P1, &P2)) >= 0) {
                    if (strcmp(Name, (char*)blackboard[index].pAdditionalInfos->Name) == 0) {
                        Found = 1;
                        break;
                    }
                }
            }
            if (!Found) {
                P1 = SaveP1;
                P2 = SaveP2;
            }
        }
    }
    *ret_p1 = P1;
    *ret_p2 = P2;
    if (Found) return index;
    else return -1;
}
#endif

VID add_bbvari_pid_type (const char *name, enum BB_DATA_TYPES type, const char *unit, int Pid, int Dir, int ValueValidFlag, union BB_VARI *Value, int *ret_Type,
                         uint32_t ReadFromIniReqMask, uint32_t *ret_WriteToIniFlag , uint32_t *ret_Observation, uint32_t *ret_AddRemoveObservation)
{
    int index, pid_index;
    int index_save1, index_save2;
    VID error;
    int CheckObservationFlag = 0;
    int CheckDataTypeObservationFlag = 0;
    VID Ret = -1;
#ifdef USE_HASH_BB_SEARCH
    uint64_t HashCode;
    int32_t P1, P2;
#endif

    //BEGIN_RUNTIME_MEASSUREMENT ("add_bbvari_pid_type")
    // is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            Ret = rm_add_bbvari_pid_type (name, type, unit, Pid, Dir, ValueValidFlag, Value, ret_Type, ReadFromIniReqMask);
            goto __OUT;
        }
#endif
        Ret = NOT_INITIALIZED;
        goto __OUT;
    }

    if (!IsValidVariableName (name)) {
        Ret = LABEL_NAME_NOT_VALID;
        goto __OUT;
    }
    // Check if variable name not to long
    if ((strlen(name) + 1 > BBVARI_NAME_SIZE) ||
        ((unit != NULL) && (strlen(unit) + 1 > BBVARI_UNIT_SIZE))) {
        Ret =  WRONG_PARAMETER; // Error
        goto __OUT;
    }

    if (Pid <= 0) {
        Pid = GET_PID();
    }
    EnterCriticalSection (&BlackboardCriticalSection);

    // Is process-id valid
    if ((pid_index = get_process_index(Pid)) == -1) {
        Ret = UNKNOWN_PROCESS;   // Error
        goto __OUT_CRITICAL;
    }

    index_save1 = index_save2 = -1;

    // lock if variable already available
#ifdef USE_HASH_BB_SEARCH
    HashCode = BuildHashCode(name);
    index = BinarySearchIndex(name, HashCode, &P1, &P2);
    if (index >= 0) {
        int ReadFromIniFlag = 0;

#else
    for (index = 0; index < get_blackboardsize(); index++) {
        if (blackboard[index].pAdditionalInfos != NULL) {
            // Name schon vorhanden
            if (blackboard[index].Vid != -1) {
                int ReadFromIniFlag = 0;
                if (strcmp(name, (char*)blackboard[index].pAdditionalInfos->Name) == 0) {
#endif
                    if (blackboard[index].Type == BB_UNKNOWN_WAIT) {  // Data type is not defined
                        int RealType;
                        if (IfUnknowDataTypeConvert (type, &RealType)) {
                            blackboard[index].Type = RealType;
                            // If there is already a conversion formula remove it
                            if (blackboard[index].pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA) {
                                remove_exec_stack_cs ((struct EXEC_STACK_ELEM*)blackboard[index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode, 0);
                            }
                            ReadFromIniFlag = 1;
                            // If this is a new Variable that are added with add_bbvari init it with 0
                            double_to_bbvari (blackboard[index].Type, &blackboard[index].Value, 0.0);
                        } else if (type == BB_UNKNOWN) {
                            Ret = WRONG_PARAMETER;   // Error
                            goto __OUT_CRITICAL;
                        } else if (type != BB_UNKNOWN_WAIT) {
                            blackboard[index].Type = type;
                            // If there is already a conversion formula remove it
                            if (blackboard[index].pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA) {
                                remove_exec_stack_cs ((struct EXEC_STACK_ELEM*)blackboard[index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode, 0);
                            }
                            ReadFromIniFlag = 1;

                            // If this is a new Variable that are added with add_bbvari init it with 0
                            if (ValueValidFlag && (Value != NULL)) {
                                blackboard[index].Value = *Value;
                            } else {
                                double_to_bbvari (blackboard[index].Type, &blackboard[index].Value, 0.0);
                            }
                            ValueValidFlag = 0;  // do not overwrite it later
                        }
                        blackboard[index].WrFlags = 0;
                        // If before was added with unknown data type and now have a valid data type than inform all which are observe this variable
                        // here will be set only a flag inside the critical section. the send of the information must be done outside the lock.
                        if (type != BB_UNKNOWN_WAIT) {
                            CheckDataTypeObservationFlag = 1;
                        }
                    } else {
                        // Not same data type
                        if ((type != blackboard[index].Type) && (type < BB_UNKNOWN)) {
                            Ret = TYPE_CLASH;   // Error
                            goto __OUT_CRITICAL;
                        }
                    }

                    if (unit != NULL) {
                        if (BBWriteUnit(unit, &blackboard[index]) != 0) {
                            // No memory available
                            Ret = BB_VAR_ADD_INFOS_MEM_ERROR;
                            goto __OUT_CRITICAL;
                        }
                    }

                    // Increment the access counters
                    if (type == BB_UNKNOWN_WAIT) {
                        blackboard[index].pAdditionalInfos->UnknownWaitAttachCount++;
                        if (blackboard[index].pAdditionalInfos->ProcessUnknownWaitAttachCounts[pid_index] == 0) {
                            blackboard[index].WrFlags &= ~(1ULL << pid_index);
                        }
                        blackboard[index].pAdditionalInfos->ProcessUnknownWaitAttachCounts[pid_index]++;
                    } else {
                        blackboard[index].pAdditionalInfos->AttachCount++;
                        if (blackboard[index].pAdditionalInfos->ProcessAttachCounts[pid_index] == 0) {
                            blackboard[index].WrFlags &= ~(1ULL << pid_index);
                        }
                        blackboard[index].pAdditionalInfos->ProcessAttachCounts[pid_index]++;
                    }

                    if (!(blackboard[index].AccessFlags & (1ULL << pid_index))) {
                        blackboard[index].WrEnableFlags |= (1ULL << pid_index);
                    }
                    blackboard[index].AccessFlags |= (1ULL << pid_index);
                    blackboard[index].RangeControlFlag |= (1ULL << pid_index);


                    // if init value valid (reference lists)
                    if (ValueValidFlag && (Value != NULL) && ((Dir & PIPE_API_REFERENCE_VARIABLE_DIR_WRITE) == PIPE_API_REFERENCE_VARIABLE_DIR_WRITE)) {
                        blackboard[index].Value = *Value;
                    }

                    // Return the variable-id
                    Ret = blackboard[index].Vid;
                    if (ret_Type != NULL) *ret_Type = blackboard[index].Type;
                    LeaveCriticalSection (&BlackboardCriticalSection);

                    if (ReadFromIniFlag) {
                        set_default_varinfo(&blackboard[index], blackboard[index].Type);
                        read_varinfos_from_ini (name,
                                                &blackboard[index], type, ReadFromIniReqMask);
                    }

                    // If before was added with unknown data type and now have a valid data inform all outside the lock
                    if (CheckObservationFlag) {
                        if (ret_AddRemoveObservation == NULL) {
                            CHECK_ADD_REMOVE_OBSERVATION(Ret, OBSERVE_ADD_VARIABLE);
                        } else {
                            *ret_AddRemoveObservation |= OBSERVE_ADD_VARIABLE;
                        }
                    }
                    if (CheckDataTypeObservationFlag) {
                        if (ret_Observation == NULL) {
                            CHECK_OBSERVATION(index, OBSERVE_TYPE_CHANGED);
                        } else {
                            *ret_Observation |= OBSERVE_TYPE_CHANGED;
                        }
                        CHECK_ADD_REMOVE_OBSERVATION(Ret, OBSERVE_TYPE_CHANGED);
                    }
                    goto __OUT;          // not CRITICAL
#ifdef USE_HASH_BB_SEARCH
#else
                }
            } else {
                if(index_save2 < 0) index_save2 = index;  // store this mybe used for next Variable
            }
        } else {
            if (index_save1 < 0) index_save1 = index;  // store this mybe used for next Variable
        }
#endif
    }


    // A variable cannot added if variable don't exit before and type == BB_UNKNOWN
    if (type == BB_UNKNOWN) {
        Ret = WRONG_PARAMETER;
        goto __OUT_CRITICAL;
    }

#ifdef USE_HASH_BB_SEARCH
    index = GetFreeBlackboardIndex();
    if (index >= 0) {
        AddBinaryHashKey(HashCode, index, P1, P2);
#else
    if (index_save2 >= 0) {
        index = index_save2;
    } else if (index_save1 >= 0) {
        index = index_save1;
    } else index = 0;

    // If variable is not there setup new one
    for (/*index = 0*/; index < get_blackboardsize(); index++) {
        // freies Element suchen
#endif
        if (blackboard[index].Vid == -1) {
            int RealType;
            int cc;
            BB_VARIABLE_ADDITIONAL_INFOS *Save = blackboard[index].pAdditionalInfos;

            // Incremet global Vid counter
            blackboard_infos.VarCount++;

            // overwrite old data
            STRUCT_ZERO_INIT (blackboard[index], BB_VARIABLE);
            blackboard[index].pAdditionalInfos = Save;

            cc = ((blackboard_infos.VarCount & 0x7F) << 1) | 1; // Vid should be never 0. Therefor the LSB is always 1
            // Generate Vid and add them
            blackboard[index].Vid = ((VID)index << 8) | cc;

            // Alloc memory for additionalInfos
            if (blackboard[index].pAdditionalInfos == NULL) {
                blackboard[index].pAdditionalInfos = (BB_VARIABLE_ADDITIONAL_INFOS*)my_calloc(1, sizeof(BB_VARIABLE_ADDITIONAL_INFOS));
            }
            if (blackboard[index].pAdditionalInfos == NULL) {
                Ret = BB_VAR_ADD_INFOS_MEM_ERROR;
                RemoveBinaryHashKey(P1, P2);
                goto __OUT_CRITICAL;
            }

            // Insert all variable data
            if (BBWriteName(name, &blackboard[index]) != 0) {
                // No memory available
                Ret = BB_VAR_ADD_INFOS_MEM_ERROR;
                RemoveBinaryHashKey(P1, P2);
                goto __OUT_CRITICAL;
            }

            if (IfUnknowDataTypeConvert (type, &RealType)) {
                blackboard[index].Type = RealType;
            } else {
                blackboard[index].Type = type;
            }
            // Increment the access counters
            if (type == BB_UNKNOWN_WAIT) {
                blackboard[index].pAdditionalInfos->UnknownWaitAttachCount++;
                blackboard[index].pAdditionalInfos->ProcessUnknownWaitAttachCounts[pid_index] = 1;
            } else {
                blackboard[index].pAdditionalInfos->AttachCount++;
                blackboard[index].pAdditionalInfos->ProcessAttachCounts[pid_index] = 1;
            }
            blackboard[index].AccessFlags =
            blackboard[index].WrEnableFlags = 1ULL << pid_index;
            blackboard[index].RangeControlFlag = 1ULL << pid_index;

            blackboard[index].WrFlags = 0;

            if (unit != NULL) {
                if (BBWriteUnit(unit, &blackboard[index]) != 0) {
                    // No memory available
                    Ret = BB_VAR_ADD_INFOS_MEM_ERROR;
                    RemoveBinaryHashKey(P1, P2);
                    goto __OUT_CRITICAL;
                }
            } else {
                if (BBWriteUnit("", &blackboard[index]) != 0) {
                    // No memory available
                    Ret = BB_VAR_ADD_INFOS_MEM_ERROR;
                    RemoveBinaryHashKey(P1, P2);
                    goto __OUT_CRITICAL;
                }
            }

            // Read variable infos from INI file
            set_default_varinfo(&blackboard[index], blackboard[index].Type);
            error = read_varinfos_from_ini (name,
                                            &blackboard[index], type, ReadFromIniReqMask);
            if (error != 0) {
                blackboard_infos.VarCount--;
                blackboard[index].Vid = -1;
                Ret = error;
                RemoveBinaryHashKey(P1, P2);
                goto __OUT_CRITICAL;
            }
            if (blackboard[index].Type == BB_UNKNOWN_DOUBLE) {
                blackboard[index].Type = BB_DOUBLE;
            }

            // If this is a new Variable that are added with add_bbvari init it with 0
            if (ValueValidFlag && (Value != NULL)) {
                blackboard[index].Value = *Value;
            } else {
                double_to_bbvari (blackboard[index].Type, &blackboard[index].Value, 0.0);
            }

            // Adjust variable count inside blackboard
            blackboard_infos.NumOfVaris++;

            // Return the variable-id
            Ret = blackboard[index].Vid;
            if (ret_Type != NULL) *ret_Type = blackboard[index].Type;
            LeaveCriticalSection (&BlackboardCriticalSection);

            // Save entry immediately (if there was set default values)
            // wurden)
            if (ret_WriteToIniFlag == NULL) {
                write_varinfos_to_ini(&blackboard[index], INI_CACHE_ENTRY_FLAG_ALL_INFOS);
            } else {
                *ret_WriteToIniFlag |= INI_CACHE_ENTRY_FLAG_ALL_INFOS;
            }

            CHECK_ADD_REMOVE_OBSERVATION(Ret, OBSERVE_ADD_VARIABLE);

            goto __OUT;  //not CRITICAL!
        } else {
            RemoveBinaryHashKey(P1, P2);
        }
    }
    Ret = NO_FREE_VID;   // Error
__OUT_CRITICAL:
    LeaveCriticalSection (&BlackboardCriticalSection);
__OUT:
    return Ret;
}


VID add_bbvari_pid (const char *name, enum BB_DATA_TYPES type, const char *unit, int Pid)
{
    return add_bbvari_pid_type (name, type, unit, Pid, 0, 0, NULL, NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL);
}

VID add_bbvari (const char *name, enum BB_DATA_TYPES type, const char *unit)
{
    VID Ret;
    Ret = add_bbvari_pid_type (name, type, unit, -1, 0, 0, NULL, NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL);
    return Ret;
}

static int IsNotANumber (double v)
{
#ifdef REMOTE_MASTER
    return isnan(v);
#else
    return _isnan (v);
#endif
#if 0
    unsigned __int64 DoubleBits;
    unsigned __int64 MaskBits;

    DoubleBits = *(unsigned __int64*)&v;
    MaskBits = 0x3FF;
    MaskBits << 53;     /* bit 63       sign         (1)
                           bit 62...53  exponent     (11)
                           bit 52       hidden 1     (1)
                           bit 51...0   significand  (52) */
    return ((DoubleBits & MaskBits) == MaskBits);
#endif
}

int GetBlackboarPrefixForProcess (int Pid, char *BBPrefix, int MaxChar)
{
    int pid_index;

    // Is process-id valid
    if ((pid_index = get_process_index (Pid)) == -1) {
        return UNKNOWN_PROCESS;   // Error
    }
    StringCopyMaxCharTruncate (BBPrefix, blackboard_infos.ProcessBBPrefix[pid_index], MaxChar);
    return 0;
}



static __inline int IsAsapAllowedCharacter (int c)
{
    if (c == '_') return 1;
    if (c == '[') return 1;
    if (c == ']') return 1;
    if (c == '.') return 1;
    if ((c >= '0') && (c <= '9')) return 1;
    if ((c >= 'a') && (c <= 'z')) return 1;
    if ((c >= 'A') && (c <= 'Z')) return 1;
    return 0;
}

static int CountNeededBytesForAsap2CombatibleLabel (const char *In)
{
    const char *s;
    int Ret = 0;

    s = In;
    while (*s != 0) {
        if ((*s == ':') && (*(s+1) == ':')) {
            s += 2;
            Ret += 3;  // "._."
        } else if (!IsAsapAllowedCharacter(*s)) {
            s++;
            if (s_main_ini_val.ReplaceAllNoneAsapCombatibleChars == 2) {
                Ret += 8;  // "._[xx]._"
            } else {
                Ret += 5;  // ".[xx]"
            }
        } else {
            s++;
            Ret++;
        }
    }
    return Ret + 1;
}


static __inline int IsNonAsapCombatibleCharReplacaement (const char *p)
{
    if (s_main_ini_val.ReplaceAllNoneAsapCombatibleChars == 2) {
        return (*p == '.') && (*(p+1) == '_')  && (*(p+2) == '[') &&
               (((*(p+3) >= '0') && (*(p+3) <= '9')) || ((*(p+3) >= 'a') && (*(p+3) <= 'z')) || ((*(p+3) >= 'A') && (*(p+3) <= 'Z')))  &&    // High nibble of byte
               (((*(p+4) >= '0') && (*(p+4) <= '9')) || ((*(p+4) >= 'a') && (*(p+4) <= 'z')) || ((*(p+4) >= 'A') && (*(p+4) <= 'Z')))  &&    // Low nibble of byte
               (*(p+5) == ']') && (*(p+6) == '.') && (*(p+7) == '_');
    } else {
        return (*p == '.') && (*(p+1) == '[') &&
               (((*(p+2) >= '0') && (*(p+2) <= '9')) || ((*(p+2) >= 'a') && (*(p+2) <= 'z')) || ((*(p+2) >= 'A') && (*(p+2) <= 'Z')))  &&    // High nibble of byte
               (((*(p+3) >= '0') && (*(p+3) <= '9')) || ((*(p+3) >= 'a') && (*(p+3) <= 'z')) || ((*(p+3) >= 'A') && (*(p+3) <= 'Z')))  &&    // Low nibble of byte
               (*(p+4) == ']');
    }
}

static __inline char ReconstructAsapCombatibleCharReplacaement (const char *p)
{
    char c = *p;
    if ((*(p+2) >= '0') && (*(p+2) <= '9')) {
        c = (char)((*(p+2) - '0') << 4);
    } else if ((*(p+2) >= 'a') && (*(p+2) <= 'z')) {
        c = (char)((*(p+2) - 'a' + 10) << 4);
    } else if ((*(p+2) >= 'A') && (*(p+2) <= 'Z')) {
        c = (char)((*(p+2) - 'A' + 10) << 4);
    }
    if ((*(p+3) >= '0') && (*(p+3) <= '9')) {
        c += (*(p+3) - '0');
    } else if ((*(p+3) >= 'a') && (*(p+3) <= 'z')) {
        c += (*(p+3) - 'a' + 10);
    } else if ((*(p+3) >= 'A') && (*(p+3) <= 'Z')) {
        c += (*(p+3) - 'A' + 10);
    }
    return c;
}

static void ReplaceAll_DoublePointDoublePoint_With_PointUnderscorePoint (const char *In, char *Out)
{
    const char *s;
    char *d;

    s = In;
    d = Out;
    while (*s != 0) {
        if ((*s == ':') && (*(s+1) == ':')) {
           *d++ = '.';
           *d++ = '_';
           *d++ = '.';
           s += 2;
        } else if ((s_main_ini_val.ReplaceAllNoneAsapCombatibleChars == 0) || IsAsapAllowedCharacter(*s)) {
           *d++ = *s++;
        } else {
            // Replace all none A2L combatible characters (all except _, ., 0-9, a-z, A-Z) through .[HexValue] oder ._[HexValue]._
            *d++ = '.';
            if (s_main_ini_val.ReplaceAllNoneAsapCombatibleChars == 2) {
                *d++ = '_';
            }
            *d++ = '[';
            if ((*s >> 4) > 9) {
                *d++ = ('a' - 10) + (*s >> 4);
            } else {
                *d++ = '0' + (*s >> 4);
            }
            if ((*s &0xF) > 9) {
                *d++ = ('a' - 10) + (*s & 0xF);
            } else {
                *d++ = '0' + (*s & 0xF);
            }
            *d++ = ']';
            if (s_main_ini_val.ReplaceAllNoneAsapCombatibleChars == 2) {
                *d++ = '.';
                *d++ = '_';
            }
            s++;
        }
    }
    *d = 0;
}

static void ReplaceAll_PointUnderscorePoint_With_DoublePointDoublePoint (const char *In, char *Out)
{
    const char *s;
    char *d;

    s = In;
    d = Out;
    while (*s != 0) {
        if ((*s == '.') && (*(s+1) == '_') && (*(s+2) == '.')) {
            *d++ = ':';
            *d++ = ':';
            s += 3;
        } else if (IsNonAsapCombatibleCharReplacaement(s)) {
            char c = ReconstructAsapCombatibleCharReplacaement(s);
            *d = c;
            d++;
            if (s_main_ini_val.ReplaceAllNoneAsapCombatibleChars == 2) {
                s += 8;
            } else {
                s += 5;
            }
        } else {
            *d++ = *s++;
        }
    }
    *d = 0;
}

// Inside an A2L file colon inside a label are not allowed
// If op==0 look inside the basic settings if we should replaced
// If op==1 replace all :: with ._. also replace all characters except _, 0-9,a-z,A-Z with .[Code]
// If op>=2 replace ._. with :: also replaceall .[Code] with the  correspond character
int ConvertLabelAsapCombatibleInOut (const char *In, char *Out, int MaxChars, int op)
{
    if (CountNeededBytesForAsap2CombatibleLabel(In) > MaxChars) {
        if ((int)strlen (In) < MaxChars){
            StringCopyMaxCharTruncate (Out, In, MaxChars);
        }
        return -1;
    } else {
        if ((!op && s_main_ini_val.AsapCombatibleLabelnames) || (op == 1)) {   // Replace all :: with ._.
            ReplaceAll_DoublePointDoublePoint_With_PointUnderscorePoint (In, Out);
        } else {                                         // Replace all ._. with ::
            ReplaceAll_PointUnderscorePoint_With_DoublePointDoublePoint (In, Out);
        }
        return 0;
    }
}

int ConvertLabelAsapCombatible (char *InOut,  int MaxChars, int op)
{
    char *Buffer;
    size_t BufferSize = strlen(InOut) + 1;
    Buffer = (char*)alloca (BufferSize);
    StringCopyMaxCharTruncate (Buffer, InOut, (int)BufferSize);
    return ConvertLabelAsapCombatibleInOut (Buffer, InOut, MaxChars, op);
}

// Compare 2 variable name independed if "::" or "._." used
int CmpVariName (const char *Name1, const char *Name2)
{
    const char *p1, *p2;
    p1 = Name1;
    p2 = Name2;

    while ((*p1 != 0)  && (*p2 != 0)) {
        if ((*p1 == ':') && (*(p1+1) == ':')) {
            if ((*p2 == ':') && (*(p2+1) == ':')) {
                p1 += 2;
                p2 += 2;
            } else if ((*p2 == '.') && (*(p2+1) == '_') && (*(p2+2) == '.')) {
                p1 += 2;
                p2 += 3;
            } else return 1;  // not equal
        } else if ((*p1 == '.') && (*(p1+1) == '_') && (*(p1+2) == '.')) {
            if ((*p2 == ':') && (*(p2+1) == ':')) {
                p1 += 3;
                p2 += 2;
            } else if ((*p2 == '.') && (*(p2+1) == '_') && (*(p2+2) == '.')) {
                p1 += 3;
                p2 += 3;
            } else return 1;  // not equal
        } else if (IsNonAsapCombatibleCharReplacaement(p1)) {
            char c1 = ReconstructAsapCombatibleCharReplacaement(p1);
            if (IsNonAsapCombatibleCharReplacaement(p2)) {
                char c2 = ReconstructAsapCombatibleCharReplacaement(p2);
                if (c1 == c2) {
                    if (s_main_ini_val.ReplaceAllNoneAsapCombatibleChars == 2) {
                        p2+=8;
                    } else {
                        p2+=5;
                    }
                } else {
                    return 1; // not equal
                }
            } else {
                if (c1 == *p2) {
                    p2++;
                } else {
                    return 1; // not equal
                }
            }
            p1+=5;
        } else if (IsNonAsapCombatibleCharReplacaement(p2)) {
            char c2 = ReconstructAsapCombatibleCharReplacaement(p2);
            if (c2 == *p1) {
                p1++;
            } else {
                return 1; // not equal
            }
            if (s_main_ini_val.ReplaceAllNoneAsapCombatibleChars == 2) {
                p2+=8;
            } else {
                p2+=5;
            }
        } else if (*p1 == *p2) {
            p1++;
            p2++;
        } else return 1;  // not equal
    }
    return ((*p1 != 0) || (*p2 != 0));
}

static int set_bbvari_step_x(VID vid, unsigned char steptype, double step, uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation);
static int set_bbvari_min_x(VID vid, double min, uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation);
static int set_bbvari_max_x(VID vid, double min, uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation);
static int set_bbvari_format_x (VID vid, int width, int prec, uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation);
static int set_bbvari_color_x (VID vid, int rgb_color, uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation);

VID add_bbvari_all_infos (int Pid, const char *name, int type, const char *unit,
                          int convtype, const char *conversion,
                          double min, double max,
                          int width, int prec,
                          int rgb_color,
                          int steptype, double step,
                          int Dir, int ValueValidFlag, union BB_VARI Value,
                          uint64_t Address,
                          int AddressValidFlag,
                          int *ret_RealType)
{
    UNUSED(Address);
    UNUSED(AddressValidFlag);
    VID vid;
    int all_infos;
    char labelp[BBVARI_NAME_SIZE+32];
    uint32_t ReadFromIniReqMask;
    uint32_t Observation = 0;
    uint32_t AddRemoveObservation = 0;
    uint32_t WriteToIniFlag = 0;
    int vid_index;

    if ((type & 0x1000) == 0x1000) {
        all_infos = 1;
        type &= ~0x1000;
    } else {
        all_infos = 0;
    }

    if (GetBlackboarPrefixForProcess (Pid, labelp, sizeof(labelp))) {
        return UNKNOWN_PROCESS;
    }
    STRING_APPEND_TO_ARRAY(labelp, name);

    ReadFromIniReqMask = READ_ALL_INFOS_BBVARI_FROM_INI;
#ifdef REMOTE_MASTER
    if (all_infos) {
        if ((unit != NULL) || (strlen(unit) > 0)) ReadFromIniReqMask &= ~READ_UNIT_BBVARI_FROM_INI;
        if (!IsNotANumber(min)) ReadFromIniReqMask &= ~READ_MIN_BBVARI_FROM_INI;
        if (!IsNotANumber(max)) ReadFromIniReqMask &= ~READ_MAX_BBVARI_FROM_INI;
        if (steptype != 255) ReadFromIniReqMask &= ~READ_STEP_BBVARI_FROM_INI;
        if ((width != 255) && (prec != 255)) ReadFromIniReqMask &= ~READ_WIDTH_PREC_BBVARI_FROM_INI;
        if (convtype != 255) ReadFromIniReqMask &= ~READ_CONVERSION_BBVARI_FROM_INI;
        if (rgb_color <= 0xFFFFFF) ReadFromIniReqMask &= ~READ_COLOR_BBVARI_FROM_INI;
    }
#endif

    vid = add_bbvari_pid_type (labelp,
                               type,
                               (strlen(unit)) ? unit : NULL,
                               Pid,
                               Dir,
                               ValueValidFlag,
                               &Value,
                               ret_RealType,
                               ReadFromIniReqMask,
                               &WriteToIniFlag,
                               &Observation,
                               &AddRemoveObservation);
    if (all_infos) {
        if (!IsNotANumber(min)) set_bbvari_min_x(vid, min, &WriteToIniFlag, &Observation);
        if (!IsNotANumber(max)) set_bbvari_max_x(vid, max, &WriteToIniFlag, &Observation);
        if (steptype != 255) set_bbvari_step_x(vid, (unsigned char)steptype, step, &WriteToIniFlag, &Observation);
        if ((width != 255) && (prec != 255)) set_bbvari_format_x(vid, width, prec, &WriteToIniFlag, &Observation);
        if (convtype != 255) set_bbvari_conversion_x(vid, convtype, conversion, 0, NULL, &WriteToIniFlag, &Observation, &AddRemoveObservation);
        if ((rgb_color >= 0) && (rgb_color <= 0xFFFFFF)) set_bbvari_color_x(vid, rgb_color, &WriteToIniFlag, &Observation);
    }

    vid_index = get_variable_index(vid);
    if (vid_index > 0) {
        CHECK_OBSERVATION(vid_index, Observation);
        CHECK_ADD_REMOVE_OBSERVATION(vid, AddRemoveObservation);
        if (WriteToIniFlag != 0) {
            write_varinfos_to_ini(&blackboard[vid_index], WriteToIniFlag);
        }
    }
    return vid;
}

static int __attach_bbvari (VID vid, int unknown_wait_flag, int cs, int pid)
{
    int vid_index;
    int pid_index;
    int Ret;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_attach_bbvari (vid, unknown_wait_flag, pid);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return UNKNOWN_VARIABLE;
    }
    // Determine the process  index
    if ((pid_index = get_process_index(pid)) == -1) {
        return UNKNOWN_PROCESS;
    }

    if (cs) EnterCriticalSection (&BlackboardCriticalSection);
    // Increment the access counters
    if (unknown_wait_flag) {
        blackboard[vid_index].pAdditionalInfos->UnknownWaitAttachCount++;
        blackboard[vid_index].pAdditionalInfos->ProcessUnknownWaitAttachCounts[pid_index]++;
    } else {
        blackboard[vid_index].pAdditionalInfos->AttachCount++;
        blackboard[vid_index].pAdditionalInfos->ProcessAttachCounts[pid_index]++;
    }

    // Set access rights
    blackboard[vid_index].AccessFlags |= 1ULL << pid_index;
    blackboard[vid_index].RangeControlFlag |= 1ULL << pid_index;
    blackboard[vid_index].WrEnableFlags |= 1ULL << pid_index;

    //Returns new attach counter
    Ret = (int)blackboard[vid_index].pAdditionalInfos->AttachCount;
    if (cs) LeaveCriticalSection (&BlackboardCriticalSection);
    return Ret;
}

int attach_bbvari (VID vid)
{
    return __attach_bbvari (vid, 0, 1, GET_PID());
}

int attach_bbvari_pid (VID vid, int pid)
{
    return __attach_bbvari (vid, 0, 1, pid);
}

int attach_bbvari_unknown_wait_pid (VID vid, int pid)
{
    return __attach_bbvari(vid, 1, 1, pid);
}

int attach_bbvari_unknown_wait (VID vid)
{
    return __attach_bbvari (vid, 1, 1, GET_PID());
}


int attach_bbvari_cs (VID vid, int cs)
{
    return __attach_bbvari (vid, 0, cs, GET_PID());
}

int attach_bbvari_unknown_wait_cs (VID vid, int cs)
{
    return __attach_bbvari (vid, 1, cs, GET_PID());
}


VID attach_bbvari_by_name (const char *name, int pid)
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
            return rm_attach_bbvari_by_name (name, pid);
        }
#endif
        return NOT_INITIALIZED;
    }
    if (pid <= 0) {
        pid = GET_PID();
    }
    EnterCriticalSection (&BlackboardCriticalSection);
#ifdef USE_HASH_BB_SEARCH
    HashCode = BuildHashCode(name);
    vid_index = BinarySearchIndex(name, HashCode, &P1, &P2);
    if (vid_index >= 0) {    // Search variable
        __attach_bbvari (blackboard[vid_index].Vid, 0, 0, pid);
        LeaveCriticalSection (&BlackboardCriticalSection);
        return blackboard[vid_index].Vid;
    }
#else
    // Search variable
    for (vid_index = 0; vid_index < get_blackboardsize(); vid_index++) {
        if (blackboard[vid_index].Vid > 0) {
            if (blackboard[vid_index].Type < BB_UNKNOWN) {
                if (blackboard[vid_index].pAdditionalInfos != NULL) {
                    // Variablenname stimmen ueberein
                    if (strcmp((char *)blackboard[vid_index].pAdditionalInfos->Name, name) == 0) {
                        __attach_bbvari (blackboard[vid_index].Vid, 0, 0, pid);
                        LeaveCriticalSection (&BlackboardCriticalSection);
                        return blackboard[vid_index].Vid;
                    }
                }
            }
        }
    }
#endif
    LeaveCriticalSection (&BlackboardCriticalSection);
    return UNKNOWN_VARIABLE;     // Error
}


void __free_all_additionl_info_memorys (BB_VARIABLE_ADDITIONAL_INFOS *pAdditionalInfos, int cs)
{
    if (pAdditionalInfos->Name != NULL) my_free (pAdditionalInfos->Name);
    if (pAdditionalInfos->Unit != NULL) my_free (pAdditionalInfos->Unit);
    if (pAdditionalInfos->DisplayName != NULL) my_free (pAdditionalInfos->DisplayName);
    if (pAdditionalInfos->Comment != NULL) my_free (pAdditionalInfos->Comment);

    if (pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA) {
        remove_exec_stack_cs ((struct EXEC_STACK_ELEM *)pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode, cs);
        my_free (pAdditionalInfos->Conversion.Conv.Formula.FormulaString);
        pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
    } else if (pAdditionalInfos->Conversion.Type == BB_CONV_TEXTREP) {
        my_free (pAdditionalInfos->Conversion.Conv.TextReplace.EnumString);
        pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
    }
    STRUCT_ZERO_INIT (*pAdditionalInfos, BB_VARIABLE_ADDITIONAL_INFOS);
}

void free_all_additionl_info_memorys (BB_VARIABLE_ADDITIONAL_INFOS *pAdditionalInfos)
{
    __free_all_additionl_info_memorys (pAdditionalInfos, 1);
}

static int __remove_bbvari (VID vid, int unknown_wait_flag, int cs, int pid)
{
    int vid_index;
    int pid_index;
    int Ret;
    int Vid_Save = -1;
    int CheckObservationFlag = 0;
    int CheckDataTypeObservationFlag = 0;
    int ProcessMatch;

    if (pid <= 0) {
        pid = GET_PID();
    }

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_remove_bbvari (vid, unknown_wait_flag, pid);
        }
#endif
        return NOT_INITIALIZED;
    }
    if (cs) EnterCriticalSection(&BlackboardCriticalSection);
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        if (cs) LeaveCriticalSection(&BlackboardCriticalSection);
        return UNKNOWN_VARIABLE;
    }

    // Determine the process  index
    if ((pid_index = get_process_index(pid)) == -1) {
        if (cs) LeaveCriticalSection(&BlackboardCriticalSection);
        return UNKNOWN_PROCESS;
    }

    // Decrement process attach counter
    ProcessMatch = 0;
    if (unknown_wait_flag) {
        if (blackboard[vid_index].pAdditionalInfos->ProcessUnknownWaitAttachCounts[pid_index]) {
            blackboard[vid_index].pAdditionalInfos->ProcessUnknownWaitAttachCounts[pid_index]--;
            ProcessMatch = 1;
        }
    } else {
        if (blackboard[vid_index].pAdditionalInfos->ProcessAttachCounts[pid_index]) {
            blackboard[vid_index].pAdditionalInfos->ProcessAttachCounts[pid_index]--;
            ProcessMatch = 1;
        }
    }
    if (ProcessMatch) {
        if ((blackboard[vid_index].pAdditionalInfos->ProcessUnknownWaitAttachCounts[pid_index] == 0) &&
            (blackboard[vid_index].pAdditionalInfos->ProcessAttachCounts[pid_index] == 0)) {
            // Reset access flag
            blackboard[vid_index].AccessFlags &= ~(1ULL << pid_index);
            blackboard[vid_index].RangeControlFlag &= ~(1ULL << pid_index);
            blackboard[vid_index].WrEnableFlags &= ~(1ULL << pid_index);
            blackboard[vid_index].WrFlags &= ~(1ULL << pid_index);
        }
        // decrement overall access counter
        if (unknown_wait_flag) {
            if (blackboard[vid_index].pAdditionalInfos->UnknownWaitAttachCount) {
                blackboard[vid_index].pAdditionalInfos->UnknownWaitAttachCount--;
            }
        } else {
            if (blackboard[vid_index].pAdditionalInfos->AttachCount) {
                blackboard[vid_index].pAdditionalInfos->AttachCount--;
            }
        }

        if (blackboard[vid_index].pAdditionalInfos->AttachCount == 0) {
            Vid_Save = blackboard[vid_index].Vid;
            if (blackboard[vid_index].pAdditionalInfos->UnknownWaitAttachCount == 0) {
    #ifdef USE_HASH_BB_SEARCH
                int32_t P1, P2;
                uint64_t HashCode = BuildHashCode(blackboard[vid_index].pAdditionalInfos->Name);  // Das sollte bein add_bbvari gespeichert werden!
                int index = BinarySearchIndex(blackboard[vid_index].pAdditionalInfos->Name, HashCode, &P1, &P2);
                if (index == vid_index) {
                    RemoveBinaryHashKey(P1, P2);
                    FreeBlackboardIndex(vid_index);
                }
    #endif
                __free_all_additionl_info_memorys(blackboard[vid_index].pAdditionalInfos, cs);
                // Update the count of variable inside blackboard
                blackboard_infos.NumOfVaris--;
                // Reset variable-id
                blackboard[vid_index].Vid = -1;

                CheckObservationFlag = 1;
            } else {
                // Reset data type afterwards only display items can access this elements
                blackboard[vid_index].Type = BB_UNKNOWN_WAIT;
                CheckDataTypeObservationFlag = 1;
            }
        }
    }
    // Return the overall attach counter
    Ret = (int)blackboard[vid_index].pAdditionalInfos->AttachCount;
    if (cs) LeaveCriticalSection (&BlackboardCriticalSection);
    // Wenn zuvor mit unbekanntem Datentyp und jetzt mit bekanntem Typ angemeldet wurde dann alle Informieren
    if (CheckDataTypeObservationFlag) {
        CHECK_OBSERVATION (vid_index, OBSERVE_TYPE_CHANGED);
        CHECK_ADD_REMOVE_OBSERVATION (Vid_Save, OBSERVE_TYPE_CHANGED);
    }
    if (CheckObservationFlag) {
        CHECK_ADD_REMOVE_OBSERVATION (Vid_Save, OBSERVE_REMOVE_VARIABLE);
    }

    // Return the overall attach counter
    return Ret;
}

int remove_bbvari (VID vid)
{
    return __remove_bbvari (vid, 0, 1, -1);
}

int remove_bbvari_pid (VID vid, int pid)
{
    return __remove_bbvari (vid, 0, 1, pid);
}

int remove_bbvari_unknown_wait (VID vid)
{
    return __remove_bbvari (vid, 1, 1, -1);
}

int remove_bbvari_unknown_wait_pid (VID vid, PID pid)
{
    return __remove_bbvari (vid, 1, 1, pid);
}

int remove_bbvari_cs (VID vid, int cs)
{
    return __remove_bbvari (vid, 0, cs, -1);
}

int remove_bbvari_unknown_wait_cs (VID vid, int cs)
{
    return __remove_bbvari (vid, 1, cs, -1);
}

int remove_bbvari_for_process_cs (VID vid, int cs, int pid)
{
    return __remove_bbvari (vid, 0, cs, pid);
}

int remove_bbvari_unknown_wait_for_process_cs (VID vid, int cs, int pid)
{
    return __remove_bbvari (vid, 1, cs, pid);
}

int remove_bbvari_extp (VID vid, PID pid, int Dir, int DataType)
{
    int ret;

    // Is process-id valid
    if (get_process_index (pid) == -1) {
        return UNKNOWN_PROCESS;   // Error
    }

    if ((DataType == BB_UNKNOWN_WAIT) ||
        ((Dir & DATA_TYPES_WAIT) == DATA_TYPES_WAIT)) {
        ret = remove_bbvari_unknown_wait_pid (vid, pid);
    } else {
        ret = remove_bbvari_pid (vid, pid);
    }
    return ret;
}

int remove_all_bbvari (PID pid)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_remove_all_bbvari (pid);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the process  index
    if (((pid_index = get_process_index(pid)) == -1) || (pid == 1)) {
        return UNKNOWN_PROCESS;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
    // Search variable
    for (vid_index = 0; vid_index < get_blackboardsize(); vid_index++) {
        if (blackboard[vid_index].Vid > -1) {
            // all variables was attachted by a process
            if ((blackboard[vid_index].pAdditionalInfos->ProcessAttachCounts[pid_index]) ||
                (blackboard[vid_index].pAdditionalInfos->ProcessUnknownWaitAttachCounts[pid_index])) {
                // Decrement overall attach counter
                if (blackboard[vid_index].pAdditionalInfos->AttachCount >= blackboard[vid_index].pAdditionalInfos->ProcessAttachCounts[pid_index]) {
                    blackboard[vid_index].pAdditionalInfos->AttachCount -= blackboard[vid_index].pAdditionalInfos->ProcessAttachCounts[pid_index];
                } else {
                    blackboard[vid_index].pAdditionalInfos->AttachCount = 0;
                }
                // Reset process attach counter
                blackboard[vid_index].pAdditionalInfos->ProcessAttachCounts[pid_index] = 0;

                // Decrement overall attach counter
                if (blackboard[vid_index].pAdditionalInfos->UnknownWaitAttachCount >= blackboard[vid_index].pAdditionalInfos->ProcessUnknownWaitAttachCounts[pid_index]) {
                    blackboard[vid_index].pAdditionalInfos->UnknownWaitAttachCount -= blackboard[vid_index].pAdditionalInfos->ProcessUnknownWaitAttachCounts[pid_index];
                } else {
                    blackboard[vid_index].pAdditionalInfos->UnknownWaitAttachCount = 0;
                }
                // Reset process attach counter
                blackboard[vid_index].pAdditionalInfos->ProcessUnknownWaitAttachCounts[pid_index] = 0;

                // Reset access flags
                blackboard[vid_index].AccessFlags &= ~(1ULL << pid_index);
                blackboard[vid_index].RangeControlFlag &= ~(1ULL << pid_index);
                blackboard[vid_index].WrEnableFlags &= ~(1ULL << pid_index);
                blackboard[vid_index].WrFlags &= ~(1ULL << pid_index);

                if (blackboard[vid_index].pAdditionalInfos->AttachCount == 0) {
                    int SaveVid = blackboard[vid_index].Vid;
                    if (blackboard[vid_index].pAdditionalInfos->UnknownWaitAttachCount == 0) {
#ifdef USE_HASH_BB_SEARCH
                        int32_t P1, P2;
                        uint64_t HashCode = BuildHashCode(blackboard[vid_index].pAdditionalInfos->Name);  // Das sollte bein add_bbvari gespeichert werden!
                        int index = BinarySearchIndex(blackboard[vid_index].pAdditionalInfos->Name, HashCode, &P1, &P2);
                        if (index == vid_index) {
                            RemoveBinaryHashKey(P1, P2);
                            FreeBlackboardIndex(vid_index);
                        }
#endif
                        __free_all_additionl_info_memorys(blackboard[vid_index].pAdditionalInfos, 0);

                        // Update the number ov variable inside the blackboard
                        blackboard_infos.NumOfVaris--;
                        // Update the variable id
                        blackboard[vid_index].Vid = -1;
                    } else {
                        // Reset the data type only displays can access them
                        blackboard[vid_index].Type = BB_UNKNOWN_WAIT;
                        // Inform all how observe this variable obout the chanfed data taype
                        CHECK_OBSERVATION (vid_index, OBSERVE_TYPE_CHANGED);
                        CHECK_ADD_REMOVE_OBSERVATION (SaveVid, OBSERVE_TYPE_CHANGED);
                    }
                    if (blackboard[vid_index].pAdditionalInfos->UnknownWaitAttachCount == 0) {
                        CHECK_ADD_REMOVE_OBSERVATION (SaveVid, OBSERVE_REMOVE_VARIABLE);
                    }
                }
            }
        }
    }
    LeaveCriticalSection (&BlackboardCriticalSection);
    // Return the overall attach counter
    return 0;
}

unsigned int get_num_of_bbvaris (void)
{
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_num_of_bbvaris ();
        }
#endif
        return 0;
    } else {
        return (unsigned int)blackboard_infos.NumOfVaris;
    }
}

VID get_bbvarivid_by_name (const char *name)
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
            return rm_get_bbvarivid_by_name (name);
        }
#endif
        return 0;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
#ifdef USE_HASH_BB_SEARCH
    HashCode = BuildHashCode(name);
    vid_index = BinarySearchIndex(name, HashCode, &P1, &P2);
    if (vid_index >= 0) {    // Search variable
        LeaveCriticalSection (&BlackboardCriticalSection);
        return blackboard[vid_index].Vid;
    }
#else
    for (vid_index = 0; vid_index < get_blackboardsize(); vid_index++) {
        if (blackboard[vid_index].Vid > 0) {
            if (blackboard[vid_index].pAdditionalInfos != NULL) {
                // Variablenname stimmen ueberein
                if (strcmp((char *)blackboard[vid_index].pAdditionalInfos->Name, name) == 0) {
                    LeaveCriticalSection (&BlackboardCriticalSection);
                    return blackboard[vid_index].Vid;
                }
            }
        }
    }
#endif
    LeaveCriticalSection (&BlackboardCriticalSection);
    return 0;     // Error
}

int get_bbvaritype_by_name (char *name)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvaritype_by_name (name);
        }
#endif
        return 0;
    }
    // Search variable
    for (vid_index = 0; vid_index < get_blackboardsize(); vid_index++) {
        if (blackboard[vid_index].Vid > 0) {
            if (blackboard[vid_index].pAdditionalInfos != NULL) {
                // Variablen-ID's stimmen ueberein
                if (strcmp ((char *)blackboard[vid_index].pAdditionalInfos->Name, name) == 0) {
                    return blackboard[vid_index].Type;
                }
            }
        }
    }
    return 0;     // Error
}

int get_bbvari_attachcount (VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvari_attachcount (vid);
        }
#endif
        return 0;
    }
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    // Return the variable-id
    return (int)blackboard[vid_index].pAdditionalInfos->AttachCount;
}

int get_bbvaritype (VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvaritype (vid);
        }
#endif
        return 0;
    }
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    // Return the variable-id
    return blackboard[vid_index].Type;
}

int GetBlackboardVariableName (VID vid, char *txt, int maxc)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_GetBlackboardVariableName (vid, txt, maxc);
        }
#endif
        return -1;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
    if ((vid_index = get_variable_index(vid)) == -1) {
        LeaveCriticalSection (&BlackboardCriticalSection);
        return -1;
    }
    StringCopyMaxCharTruncate (txt, blackboard[vid_index].pAdditionalInfos->Name, maxc);
    LeaveCriticalSection (&BlackboardCriticalSection);
    return 0;
}

int GetBlackboardVariableNameAndTypes (VID vid, char *txt, int maxc, enum BB_DATA_TYPES *ret_data_type, enum BB_CONV_TYPES *ret_conversion_type)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_GetBlackboardVariableNameAndTypes (vid, txt, maxc, ret_data_type, ret_conversion_type);
        }
#endif
        return -1;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
    if ((vid_index = get_variable_index(vid)) == -1) {
        LeaveCriticalSection (&BlackboardCriticalSection);
        return -1;
    }
    // Return the variable-id
    StringCopyMaxCharTruncate (txt, blackboard[vid_index].pAdditionalInfos->Name, maxc);
    *ret_data_type = blackboard[vid_index].Type;
    *ret_conversion_type = blackboard[vid_index].pAdditionalInfos->Conversion.Type;
    LeaveCriticalSection (&BlackboardCriticalSection);
    return 0;
}


int get_process_bbvari_attach_count_pid(VID vid, PID pid)
{
    int vid_index;
    int pid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_process_bbvari_attach_count_pid (vid, pid);
        }
#endif
        return NOT_INITIALIZED;
    }

    if ((vid_index = get_variable_index(vid)) == -1) {
        return UNKNOWN_VARIABLE;
    }

    // Determine the process  index (for access mask)
    if ((pid_index = get_process_index(pid)) == -1) {
        return UNKNOWN_PROCESS;
    }

    return (int)blackboard[vid_index].pAdditionalInfos->ProcessAttachCounts[pid_index];
}

int get_process_bbvari_attach_count(VID vid)
{
    return get_process_bbvari_attach_count_pid(vid, GET_PID());
}


int set_bbvari_unit (VID vid, const char *unit)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_set_bbvari_unit (vid, unit);
        }
#endif
        return NOT_INITIALIZED;
    }

    // Is the unit string too long
    if (strlen(unit) > BBVARI_UNIT_SIZE - 1) {
        return UNKNOWN_VARIABLE;
    }

    if ((vid_index = get_variable_index(vid)) == -1) {
        return UNKNOWN_VARIABLE;
    }

    EnterCriticalSection(&BlackboardCriticalSection);
    // Copy the unit
    if (BBWriteUnit(unit, &blackboard[vid_index]) != 0) {
      // No memory available
      LeaveCriticalSection (&BlackboardCriticalSection);
      return BB_VAR_ADD_INFOS_MEM_ERROR;
    }
    LeaveCriticalSection(&BlackboardCriticalSection);

    // Now save all to the INI file
    write_varinfos_to_ini(&blackboard[vid_index], INI_CACHE_ENTRY_FLAG_UNIT);

    CHECK_OBSERVATION (vid_index, OBSERVE_UNIT_CHANGED);
    return 0;
}

int get_bbvari_unit (VID vid, char *unit, int maxc)
{
    int vid_index;
    int Len;
    int Ret = -1;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvari_unit (vid, unit, maxc);
        }
#endif
        return NOT_INITIALIZED;
    }

    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        *unit = 0;
        return -1;
    }

    EnterCriticalSection (&BlackboardCriticalSection);
    if (blackboard[vid_index].pAdditionalInfos->Unit == 0) {
        unit[0] = 0;
    } else {
        Len = (int)strlen((char*)blackboard[vid_index].pAdditionalInfos->Unit) + 1;
        // Is the unit string too long
        if (Len > maxc) {
            *unit = 0;
            Ret = -Len;
        } else {
            // Copy the unit
            MEMCPY(unit, (char *)blackboard[vid_index].pAdditionalInfos->Unit, (size_t)Len);
            Ret = 0;
        }
    }
    LeaveCriticalSection (&BlackboardCriticalSection);

    return 0;
}

int set_bbvari_conversion_x (VID vid, int convtype, const char *conversion, int sizeof_exec_stack, struct EXEC_STACK_ELEM *exec_stack, uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation, uint32_t *ret_AddRemoveObservation)
{
#ifndef REMOTE_MASTER
    UNUSED(sizeof_exec_stack);
    UNUSED(exec_stack);
#endif
    int ret = 0;
    int vid_index;
    enum BB_CONV_TYPES convertion_type_save;

    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }
    //Is the conversion string too long
    if (((convtype == BB_CONV_FORMULA) || (convtype == BB_CONV_TEXTREP) ||
         (convtype == BB_CONV_FACTOFF) || (convtype == BB_CONV_OFFFACT) ||
         (convtype == BB_CONV_TAB_INTP) || (convtype == BB_CONV_TAB_NOINTP) ||
         (convtype == BB_CONV_RAT_FUNC) || (convtype == BB_CONV_REF)) &&
        ((strlen(conversion) + 1) > BBVARI_CONVERSION_SIZE)) {
        return -1;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
    convertion_type_save = blackboard[vid_index].pAdditionalInfos->Conversion.Type;
    // If there is already a conversion formula remove it
    if (blackboard[vid_index].pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA) {
        remove_exec_stack_cs ((struct EXEC_STACK_ELEM*)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode, 0);
    }
    LeaveCriticalSection (&BlackboardCriticalSection);

    switch (convtype) {
    case BB_CONV_NONE:      // No conversion
        blackboard[vid_index].pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
        break;

    case BB_CONV_FORMULA:     // Raw to physical
        // for the time of translation set it to none
        blackboard[vid_index].pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
        // transfer the translation to the variable
        if (BBWriteFormulaString(conversion, &blackboard[vid_index]) != 0) {
            // No memory available
            ret_WriteToIniFlag = NULL;
            ret = BB_VAR_ADD_INFOS_MEM_ERROR;
        }
#ifdef REMOTE_MASTER
        // Equation is aready translated
        if ((sizeof_exec_stack > 0) && (exec_stack != NULL)) {
            blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode = my_malloc(sizeof_exec_stack);
            if (blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode != NULL) {
                MEMCPY(blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode, exec_stack, sizeof_exec_stack);
                blackboard[vid_index].pAdditionalInfos->Conversion.Type = BB_CONV_FORMULA;
                break;  // switch()
            }
        } else {
            // Try to compile the equation on the remote master side
            if (equ_src_to_bin(&blackboard[vid_index]) == 0) {
                blackboard[vid_index].pAdditionalInfos->Conversion.Type = BB_CONV_FORMULA;
                break;  // switch()
            }
        }
        blackboard[vid_index].pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
        if (blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode != NULL) my_free(blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode);
        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode = NULL;
        if (blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString != NULL) my_free(blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString);
        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString = NULL;
#else
        // Translate formula
        if (equ_src_to_bin (&blackboard[vid_index]) == -1) {
            blackboard[vid_index].pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
            if (blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode != NULL) my_free(blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode);
            blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode = NULL;
            if (blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString != NULL) my_free(blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString);
            blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString = NULL;
            ret = -1;
    } else {
            // Set this after translation
            blackboard[vid_index].pAdditionalInfos->Conversion.Type = BB_CONV_FORMULA;
        }
#endif
        break;

    case BB_CONV_TEXTREP:  // Raw to text replace
        blackboard[vid_index].pAdditionalInfos->Conversion.Type = BB_CONV_TEXTREP;
        if (BBWriteEnumString(conversion, &blackboard[vid_index]) != 0) {
            ret_WriteToIniFlag = NULL;
            ret = BB_VAR_ADD_INFOS_MEM_ERROR;
        }
        break;

    case BB_CONV_FACTOFF:
    case BB_CONV_OFFFACT:
    {
        char *p;
        ret = -1;
        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Factor = strtod(conversion, &p);
        if (*p == ':') {
            blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Offset = strtod(p + 1, &p);
            if (*p == 0) {
                blackboard[vid_index].pAdditionalInfos->Conversion.Type = convtype;
                ret = 0;
            }
        }
        break;
    }
    case BB_CONV_TAB_INTP:
    case BB_CONV_TAB_NOINTP:
    {
        int x;
        char *p;
        int Size = 1;
        const char *cc;
        // first we count the number of points
        cc = conversion;
        while (*cc != 0) {
            if (*cc == ':') Size++;
            cc++;
        }
        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Size = Size;
        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Values = (struct CONVERSION_TABLE_VALUE_PAIR*)my_malloc(Size * sizeof(struct CONVERSION_TABLE_VALUE_PAIR));
        if (blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Values != NULL) {
            p = (char*)conversion;
            for (x = 0; x < Size; x++) {
                blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Values[x].Phys = strtod(p , &p);
                if (*p == '/') p++;
                else break;
                blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Values[x].Raw = strtod(p , &p);
                if (*p == ':') p++;
                else if (*p != 0) break;
            }
            if (x != Size) {
                // an error occured
                blackboard[vid_index].pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
                my_free(blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Values);
                ret = -1;
            } else {
                blackboard[vid_index].pAdditionalInfos->Conversion.Type = convtype;
                ret = 0;
            }
        }
        break;
    }
    case BB_CONV_RAT_FUNC:
    {
        char *p;
        ret = -1;
        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.a = strtod(conversion, &p);
        if (*p == ':') {
            blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.b = strtod(p + 1, &p);
            if (*p == ':') {
                blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.c = strtod(p + 1, &p);
                if (*p == ':') {
                    blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.d = strtod(p + 1, &p);
                    if (*p == ':') {
                        blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.e = strtod(p + 1, &p);
                        if (*p == ':') {
                            blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.f = strtod(p + 1, &p);
                            if (*p == 0) {
                                blackboard[vid_index].pAdditionalInfos->Conversion.Type = convtype;
                                ret = 0;
                            }
                        }
                    }
                }
            }
        }
        break;
    }
    default:
        ret_WriteToIniFlag = NULL;
        ret = -1;
    }

    // Now save all to the INI file
    if (ret_WriteToIniFlag == NULL) {
        write_varinfos_to_ini(&blackboard[vid_index], INI_CACHE_ENTRY_FLAG_CONVERSION);
    } else {
        *ret_WriteToIniFlag |= INI_CACHE_ENTRY_FLAG_CONVERSION;
    }
    if (ret_Observation == NULL) {
        CHECK_OBSERVATION (vid_index, OBSERVE_CONVERSION_CHANGED);
    } else {
        *ret_Observation |= OBSERVE_CONVERSION_CHANGED;
    }
    if (convertion_type_save != blackboard[vid_index].pAdditionalInfos->Conversion.Type) {
        if (ret_AddRemoveObservation == NULL) {
            CHECK_ADD_REMOVE_OBSERVATION(blackboard[vid_index].Vid, OBSERVE_CONVERSION_TYPE_CHANGED);
        } else {
            *ret_AddRemoveObservation |= OBSERVE_CONVERSION_TYPE_CHANGED;
        }
    }
    return ret;
}


int set_bbvari_conversion(VID vid, int convtype, const char *conversion)
{
    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_set_bbvari_conversion (vid, convtype, conversion);
        }
#endif
        return NOT_INITIALIZED;
    }

    return set_bbvari_conversion_x(vid, convtype, conversion, 0, NULL, NULL, NULL, NULL);
}

int get_bbvari_conversion (VID vid, char *conversion, int maxc)
{
    int vid_index;
    int Ret = -1;
    int Len;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvari_conversion (vid, conversion, maxc);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }

    EnterCriticalSection (&BlackboardCriticalSection);
    Ret = blackboard[vid_index].pAdditionalInfos->Conversion.Type;

    switch(Ret) {
    case BB_CONV_NONE:
        if (conversion != NULL) conversion[0] = 0;
        break;
    case BB_CONV_FORMULA:
        Len = (int)strlen ((const char*)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString) + 1;
        if (Len > maxc) {
            if (conversion != NULL) conversion[0] = 0;
            Ret = -Len;
        } else {
            MEMCPY (conversion, (const char*)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString, (size_t)Len);
        }
        break;
    case BB_CONV_TEXTREP:
        Len = (int)strlen ((const char*)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.TextReplace.EnumString) + 1;
        if (Len > maxc) {
            if (conversion != NULL) conversion[0] = 0;
            Ret = -Len;
        } else {
            MEMCPY (conversion, (const char*)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.TextReplace.EnumString, (size_t)Len);
        }
        break;
    case BB_CONV_FACTOFF:
    case BB_CONV_OFFFACT:
    {
        char Help[64];
        Len = PrintFormatToString (Help, sizeof(Help), "%.18g:%.18g",
                       blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Factor,
                       blackboard[vid_index].pAdditionalInfos->Conversion.Conv.FactorOffset.Offset) + 1;
        if (Len > maxc) {
            if (conversion != NULL) conversion[0] = 0;
            Ret = -Len;
        } else {
            MEMCPY (conversion, (const char*)Help, (size_t)Len);
        }
        break;
    }
    case BB_CONV_TAB_INTP:
    case BB_CONV_TAB_NOINTP:
    {
        int l, x;
        char Help[64];
        Len = 0;
        for (x = 0; x < blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Size; x++) {
            l = PrintFormatToString (Help, sizeof(Help), (x == 0) ? "%.18g/%.18g" : ":%.18g/%.18g",
                         blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Values[x].Phys,
                         blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Table.Values[x].Raw);
            if ((Len + l) < maxc) {
                MEMCPY (conversion + Len, (const char*)Help, (size_t)l);
            }
            Len += l;
        }
        if (Len < maxc) {
            conversion[Len] = 0;
        }
        Len++;
        if (Len > maxc) {
            if (conversion != NULL) conversion[0] = 0;
            Ret = -Len;
        }
        break;
    }
    case BB_CONV_RAT_FUNC:
    {
        char Help[6*32];
        Len = PrintFormatToString (Help, sizeof(Help), "%.18g:%.18g:%.18g:%.18g:%.18g:%.18g",
                      blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.a,
                      blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.b,
                      blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.c,
                      blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.d,
                      blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.e,
                      blackboard[vid_index].pAdditionalInfos->Conversion.Conv.RatFunc.f) + 1;
        if (Len > maxc) {
            if (conversion != NULL) conversion[0] = 0;
            Ret = -Len;
        } else {
            MEMCPY (conversion, (const char*)Help, (size_t)Len);
        }
        break;
    }
    default:
        Ret = -1;
    }
    LeaveCriticalSection (&BlackboardCriticalSection);

    // Return the conversion type
    return Ret;
}

int get_bbvari_conversiontype (VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvari_conversiontype (vid);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }
    // Return the conversion type
    return blackboard[vid_index].pAdditionalInfos->Conversion.Type;
}

int get_bbvari_infos (VID par_Vid, BB_VARIABLE *ret_BaseInfos, BB_VARIABLE_ADDITIONAL_INFOS *ret_AdditionalInfos, char *ret_Buffer, int par_SizeOfBuffer)
{
    int Index;
    int Ret = -1;
    size_t Len;
    size_t LenName;
    size_t LenDisplayName;
    size_t LenUnit;
    size_t LenFormulaString = 0;  // avoid waring
    size_t LenFormulaByteCode = 0;  // avoid waring
    size_t LenEnumString = 0;  // avoid waring
    size_t LenReferenceName = 0;  // avoid waring

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvari_infos (par_Vid, ret_BaseInfos, ret_AdditionalInfos, ret_Buffer, par_SizeOfBuffer);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the vid index
    if ((Index = get_variable_index(par_Vid)) == -1) {
        return -1;
    }
    EnterCriticalSection (&BlackboardCriticalSection);

    Len = LenName = strlen (blackboard[Index].pAdditionalInfos->Name) + 1;
    if (blackboard[Index].pAdditionalInfos->DisplayName != NULL) {
        Len += LenDisplayName = strlen (blackboard[Index].pAdditionalInfos->DisplayName) + 1;
    } else LenDisplayName = 0;
    if (blackboard[Index].pAdditionalInfos->Unit != NULL) {
        Len += LenUnit = strlen (blackboard[Index].pAdditionalInfos->Unit) + 1;
    } else LenUnit = 0;
    switch (blackboard[Index].pAdditionalInfos->Conversion.Type) {
    case BB_CONV_FORMULA:
        Len += LenFormulaString = strlen (blackboard[Index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString) + 1;
        Len += LenFormulaByteCode = (size_t)sizeof_exec_stack ((struct EXEC_STACK_ELEM *)(blackboard[Index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode));
        break;
    case BB_CONV_TEXTREP:
        Len += LenEnumString = strlen (blackboard[Index].pAdditionalInfos->Conversion.Conv.TextReplace.EnumString) + 1;
        break;
    case BB_CONV_REF:
        Len += LenReferenceName = strlen (blackboard[Index].pAdditionalInfos->Conversion.Conv.Reference.Name) + 1;
        break;
    default:
        break;
    }
    if ((int)Len < par_SizeOfBuffer) {
        if (ret_BaseInfos != NULL) {
            *ret_BaseInfos = blackboard[Index];
            ret_BaseInfos->pAdditionalInfos = ret_AdditionalInfos;
        }
        if (ret_AdditionalInfos != NULL) {
            char *p = ret_Buffer;
            *ret_AdditionalInfos = *(blackboard[Index].pAdditionalInfos);
            ret_AdditionalInfos->Name = MEMCPY (p, blackboard[Index].pAdditionalInfos->Name, LenName);
            p += LenName;
            if (LenDisplayName) {
                ret_AdditionalInfos->DisplayName = MEMCPY (p, blackboard[Index].pAdditionalInfos->DisplayName, LenDisplayName);
                p += LenDisplayName;
            }
            if (LenUnit) {
                ret_AdditionalInfos->Unit = MEMCPY (p, blackboard[Index].pAdditionalInfos->Unit, LenUnit);
                p += LenUnit;
            }
            switch (ret_AdditionalInfos->Conversion.Type) {
            case BB_CONV_FORMULA:
                ret_AdditionalInfos->Conversion.Conv.Formula.FormulaString = MEMCPY (p, blackboard[Index].pAdditionalInfos->Conversion.Conv.Formula.FormulaString, LenFormulaString);
                p += LenFormulaString;
                ret_AdditionalInfos->Conversion.Conv.Formula.FormulaByteCode = MEMCPY (p, blackboard[Index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode, LenFormulaByteCode);
                p += LenFormulaByteCode;
                break;
            case BB_CONV_TEXTREP:
                ret_AdditionalInfos->Conversion.Conv.TextReplace.EnumString = MEMCPY (p, blackboard[Index].pAdditionalInfos->Conversion.Conv.TextReplace.EnumString, LenEnumString);
                p += LenEnumString;
                break;
            case BB_CONV_REF:
                ret_AdditionalInfos->Conversion.Conv.Reference.Name = MEMCPY (p, blackboard[Index].pAdditionalInfos->Conversion.Conv.Reference.Name, LenReferenceName);
                p += LenReferenceName;
                break;
            default:
                break;
            }
            Ret = 0;  // all OK
        }
    } else {
        Ret = (int)Len + 1;  // Buffer too small return needed size
    }

    LeaveCriticalSection (&BlackboardCriticalSection);

    return Ret;
}

static int set_bbvari_color_x (VID vid, int rgb_color, uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_set_bbvari_color (vid, rgb_color);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return UNKNOWN_VARIABLE;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
    // Set color value
    blackboard[vid_index].pAdditionalInfos->RgbColor = (uint32_t)rgb_color;

    LeaveCriticalSection(&BlackboardCriticalSection);

    // Now save all to the INI file
    if (ret_WriteToIniFlag == NULL) {
        write_varinfos_to_ini(&blackboard[vid_index], INI_CACHE_ENTRY_FLAG_COLOR);
    } else {
        *ret_WriteToIniFlag |= INI_CACHE_ENTRY_FLAG_COLOR;
    }
    if (ret_Observation == NULL) {
        CHECK_OBSERVATION(vid_index, OBSERVE_COLOR_CHANGED);
    } else {
        *ret_Observation |= OBSERVE_COLOR_CHANGED;
    }

    return 0;
}

int set_bbvari_color(VID vid, int rgb_color)
{
    return set_bbvari_color_x(vid, rgb_color, NULL, NULL);
}

int get_bbvari_color (VID vid)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvari_color (vid);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return UNKNOWN_VARIABLE;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    // Set color value
    return (int)blackboard[vid_index].pAdditionalInfos->RgbColor;
}

static int set_bbvari_step_x (VID vid, unsigned char steptype, double step, uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_set_bbvari_step (vid, steptype, step);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return UNKNOWN_VARIABLE;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
    // Steptype und Step zuweisen
    blackboard[vid_index].pAdditionalInfos->StepType = steptype;
    blackboard[vid_index].pAdditionalInfos->Step = step;

    LeaveCriticalSection(&BlackboardCriticalSection);

    // Now save all to the INI file
    if (ret_WriteToIniFlag == NULL) {
        write_varinfos_to_ini(&blackboard[vid_index], INI_CACHE_ENTRY_FLAG_STEP);
    } else {
        *ret_WriteToIniFlag |= INI_CACHE_ENTRY_FLAG_STEP;
    }
    if (ret_Observation == NULL) {
        CHECK_OBSERVATION(vid_index, OBSERVE_STEP_CHANGED);
    } else {
        *ret_Observation |= OBSERVE_STEP_CHANGED;
    }
    return 0;
}

int set_bbvari_step(VID vid, unsigned char steptype, double step)
{
    return set_bbvari_step_x(vid, steptype, step, NULL, NULL);
}

int get_bbvari_step (VID vid, unsigned char *steptype, double *step)
{
    int vid_index;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvari_step (vid, steptype, step);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index (vid)) == -1) {
        return UNKNOWN_VARIABLE;
    }

    // Steptype und Step zuweisen
    *steptype = blackboard[vid_index].pAdditionalInfos->StepType;
    *step = blackboard[vid_index].pAdditionalInfos->Step;

    // Set color value
    return 0;
}

static int set_bbvari_min_x (VID vid, double min, uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation)
{
    int vid_index;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_set_bbvari_min (vid, min);
        }
#endif
        return NOT_INITIALIZED;
    }

    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
    blackboard[vid_index].pAdditionalInfos->Min = min;
    if (blackboard[vid_index].pAdditionalInfos->Max <= min) {
        blackboard[vid_index].pAdditionalInfos->Max = min + 1.0;
    }

    LeaveCriticalSection(&BlackboardCriticalSection);

    // Now save all to the INI file
    if (ret_WriteToIniFlag == NULL) {
        write_varinfos_to_ini(&blackboard[vid_index], INI_CACHE_ENTRY_FLAG_MIN);
    } else {
        *ret_WriteToIniFlag |= INI_CACHE_ENTRY_FLAG_MIN;
    }
    if (ret_Observation == NULL) {
        CHECK_OBSERVATION(vid_index, OBSERVE_MIN_MAX_CHANGED);
    } else {
        *ret_Observation |= OBSERVE_MIN_MAX_CHANGED;
    }
    return 0;
}

int set_bbvari_min(VID vid, double min)
{
    return set_bbvari_min_x(vid, min, NULL, NULL);
}

static int set_bbvari_max_x (VID vid, double max, uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation)
{
    int vid_index;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_set_bbvari_max (vid, max);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return -1;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
    blackboard[vid_index].pAdditionalInfos->Max = max;
    if (blackboard[vid_index].pAdditionalInfos->Min >= max) {
        blackboard[vid_index].pAdditionalInfos->Min = max - 1.0;
    }
    LeaveCriticalSection(&BlackboardCriticalSection);

    // Now save all to the INI file
    if (ret_WriteToIniFlag == NULL) {
        write_varinfos_to_ini(&blackboard[vid_index], INI_CACHE_ENTRY_FLAG_MAX);
    } else {
        *ret_WriteToIniFlag |= INI_CACHE_ENTRY_FLAG_MAX;
    }
    if (ret_Observation == NULL) {
        CHECK_OBSERVATION(vid_index, OBSERVE_MIN_MAX_CHANGED);
    } else {
        *ret_Observation |= OBSERVE_MIN_MAX_CHANGED;
    }
    return 0;
}

int set_bbvari_max(VID vid, double max)
{
    return set_bbvari_max_x(vid, max, NULL, NULL);
}

double get_bbvari_min (VID vid)
{
    int     vid_index;
    double  min;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvari_min (vid);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    min = blackboard[vid_index].pAdditionalInfos->Min;
    return min;
}

double get_bbvari_max (VID vid)
{
    int     vid_index;
    double  max;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvari_max (vid);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index (vid)) == -1) {
        return 0;
    }
    max = blackboard[vid_index].pAdditionalInfos->Max;

    return max;
}

static int set_bbvari_format_x (VID vid, int width, int prec, uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation)
{
    int vid_index;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_set_bbvari_format (vid, width, prec);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the vid index
    if ((vid_index = get_variable_index (vid)) == -1) {
        return -1;
    }
    EnterCriticalSection (&BlackboardCriticalSection);
    // Takeover format
    blackboard[vid_index].pAdditionalInfos->Width = (uint8_t)width;
    blackboard[vid_index].pAdditionalInfos->Prec = (uint8_t)prec;

    LeaveCriticalSection(&BlackboardCriticalSection);

    // Now save all to the INI file
    if (ret_WriteToIniFlag == NULL) {
        write_varinfos_to_ini(&blackboard[vid_index], INI_CACHE_ENTRY_FLAG_WIDTH_PREC);
    } else {
        *ret_WriteToIniFlag |= INI_CACHE_ENTRY_FLAG_WIDTH_PREC;
    }
    if (ret_Observation == NULL) {
        CHECK_OBSERVATION(vid_index, OBSERVE_FORMAT_CHANGED);
    } else {
        *ret_Observation |= OBSERVE_FORMAT_CHANGED;
    }
    return 0;
}

int set_bbvari_format(VID vid, int width, int prec)
{
    return set_bbvari_format_x(vid, width, prec, NULL, NULL);
}

int get_bbvari_format_width (VID vid)
{
    int vid_index;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvari_format_width (vid);
        }
#endif
        return NOT_INITIALIZED;
    }
    if ((vid_index = get_variable_index (vid)) == -1) {
        return -1;
    }

    // Return the display width
    return blackboard[vid_index].pAdditionalInfos->Width;
}

int get_bbvari_format_prec (VID vid)
{
    int vid_index;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_bbvari_format_prec (vid);
        }
#endif
        return NOT_INITIALIZED;
    }

    // Determine the vid index
    if ((vid_index = get_variable_index (vid)) == -1) {
        return -1;
    }

    // Return the precision
    return blackboard[vid_index].pAdditionalInfos->Prec;
}

VID *attach_frame (VID *vids)
{
    int     index;
    VID     *new_vids;

    // search last element
    for (index = 0; vids[index] != 0; index++);

    // plus the last Element
    index++;

    if ((new_vids = (VID *)my_calloc ((size_t)index, sizeof(VID))) == NULL) {
        return NULL;
    }

    MEMCPY (new_vids, vids, (size_t)index * sizeof(VID));

    // Return a pointer to the array
    return new_vids;
}

void free_frame (VID *frame)
{
    if (frame != NULL) {
        my_free (frame);
    }
}

int get_free_wrflag (PID pid, uint64_t *wrflag)
{
    int i;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_get_free_wrflag (pid, wrflag);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Check process-id
    if ((i = get_process_index(pid)) == -1) {
        return -1;
    }

    // Buid bit maske
    *wrflag = 1ULL << i;

    return 0;
}

void free_wrflag (PID pid, uint64_t wrflag)
{
    UNUSED(pid);
    UNUSED(wrflag);
}

void reset_wrflag (VID vid, uint64_t w)
{
    int vid_index;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            rm_reset_wrflag (vid, w);
            return;
        }
#endif
        return;
    }

    // Determine the variable index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return;
    }

    // Reset the corresponding bit
    blackboard[vid_index].WrFlags &= ~w;
}

int test_wrflag (VID vid, uint64_t w)
{
    int vid_index;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_test_wrflag (vid, w);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the variable index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return 0;
    }
    // Check if the bit is set
    return (blackboard[vid_index].WrFlags & w) == w;
}

int read_next_blackboard_vari (int index, char *ret_NameBuffer, int max_c)
{
    size_t len;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_read_next_blackboard_vari (index, ret_NameBuffer, max_c);
        }
#endif
        return NOT_INITIALIZED;
    }

    EnterCriticalSection (&BlackboardCriticalSection);
    // Increment index till the variable is found or the blackboard end is reached
    for ( ; index < get_blackboardsize(); index++) {
        if ((blackboard[index].Vid != -1) &&
            (blackboard[index].Type < BB_UNKNOWN)) { // only real variable
            int index_save = index;
            index++;
            len = strlen(blackboard[index_save].pAdditionalInfos->Name) + 1;
            if (len <= (size_t)max_c) {
                MEMCPY (ret_NameBuffer, blackboard[index_save].pAdditionalInfos->Name, len);
            } else {
                MEMCPY (ret_NameBuffer, blackboard[index_save].pAdditionalInfos->Name, (size_t)max_c - 1);
                ret_NameBuffer[max_c - 1] = 0;
            }
            LeaveCriticalSection (&BlackboardCriticalSection);
            return index;
        }
    }
    // Nothing more found
    LeaveCriticalSection (&BlackboardCriticalSection);
    return -1;
}

int ReadNextVariableProcessAccess (int index, PID pid, int access, char *name, int maxc)
{
    int vid_index;
    int pid_index;

    if (index < 0) index = 0;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_ReadNextVariableProcessAccess (index, pid, access, name, maxc);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine process index
    if ((pid_index = get_process_index(pid)) == -1) {
        return -1;
    }
    vid_index = index;

    // Increment index till the variable is found or the blackboard end is reached
    for ( ; vid_index < get_blackboardsize(); vid_index++) {
        if ((blackboard[vid_index].Vid != -1) &&
            (blackboard[vid_index].pAdditionalInfos->ProcessAttachCounts[pid_index] ||
             blackboard[vid_index].pAdditionalInfos->ProcessUnknownWaitAttachCounts[pid_index])) {
            switch (access & 0x3) {
            case 2:  // Disabled
                if (~blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
                    // Found variabale, return the name
                    StringCopyMaxCharTruncate (name, (char *)blackboard[vid_index].pAdditionalInfos->Name, maxc);
                    vid_index++;
                    return vid_index;
                }
                break;
            case 1:  // Enabled
                if (blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) {
                    // Found variabale, return the name
                    StringCopyMaxCharTruncate (name, (char *)blackboard[vid_index].pAdditionalInfos->Name, maxc);
                    vid_index++;
                    return vid_index;
                }
                break;
            case 3:  // Disabled or Enabled
                // Found variabale, return the name
                StringCopyMaxCharTruncate (name, (char *)blackboard[vid_index].pAdditionalInfos->Name, maxc);
                vid_index++;
                return vid_index;
            }
        }
    }

    // Nothing found
    return -1;
}

int enable_bbvari_access (PID pid, VID vid)
{
    int vid_index;
    int pid_index;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_enable_bbvari_access (pid, vid);
        }
#endif
        return NOT_INITIALIZED;
    }

    // Determine the arry index
    if ((pid_index = get_process_index(pid)) == -1) {
        return NO_FREE_WRFLAG;   // Error
    }

    // Determine the variable index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return NO_FREE_WRFLAG;   // Error
    }

    // Set the corresponding bit
    blackboard[vid_index].WrEnableFlags |= (1ULL << pid_index);

    return 0;
}

/*************************************************************************
**
**    Function    : disable_bbvari_access
**
**    Description : setzen von wrenable_flags einer Variable
**    Parameter   : PID pid - Prozess-ID
**                  VID vid - Variablen-ID
**    Returnvalues: 0 -> Erfolg, NO_FREE_WRFLAG,NOT_INITIALIZED -> Fehler
**
*************************************************************************/
int disable_bbvari_access (PID pid, VID vid)
{
    int vid_index;
    int pid_index;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_disable_bbvari_access (pid, vid);
        }
#endif
        return NOT_INITIALIZED;
    }

    // Determine the arry index
    if ((pid_index = get_process_index(pid)) == -1) {
        return NO_FREE_WRFLAG;   // Error
    }

    // Determine the variable index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return NO_FREE_WRFLAG;   // Error
    }

    // Set the corresponding bit
    blackboard[vid_index].WrEnableFlags &= ~(1ULL << pid_index);

    return 0;
}

int is_bbvari_access_allowed(PID pid, VID vid)
{
    int vid_index;
    int pid_index;
    int ret;

    if (blackboard == NULL) {
#ifndef REMOTE_MASTER
        if (s_main_ini_val.ConnectToRemoteMaster) {
            return rm_disable_bbvari_access(pid, vid);
        }
#endif
        return NOT_INITIALIZED;
    }
    // Determine the arry index
    if ((pid_index = get_process_index(pid)) == -1) {
        return NO_FREE_WRFLAG;   // Error
    }

    // Determine the variable index
    if ((vid_index = get_variable_index(vid)) == -1) {
        return NO_FREE_WRFLAG;   // Error
    }

    // is the corresponding bit set
    ret = ((blackboard[vid_index].WrEnableFlags & (1ULL << pid_index)) != 0) ? 1 : 0;

    return ret;
}


static void UintToBinaryString(uint64_t value, int bit_size, char *string)
{
    int x;
    uint64_t mask;
    // Ignore leading 0
    for (x = bit_size-1; x > 0; x--) {
        mask = 1LLU << x;
        if ((value & mask) == mask) {
            break;
        }
    }
    for (; x >= 0; x--) {
        mask = 1LLU << x;
        if ((value & mask) == mask) {
            *string = '1';
        } else {
            *string = '0';
        }
        string++;
    }
    *string = 0;
}

static uint64_t RemoveSign(int64_t Input)
{
    uint64_t Ret = (uint64_t)Input;
    Ret = Ret - 1;
    Ret = ~Ret;
    return Ret;
}

int bbvari_to_string (enum BB_DATA_TYPES type,
                      union BB_VARI value,
                      int base,
                      char *target_string,
                      int maxc)
{
    switch (type) {
    case BB_BYTE:
        switch (base) {
        default:
        case 10:
            PrintFormatToString(target_string, maxc, "%d", (int)value.b);
            break;
        case 16:
            if (value.b < 0) PrintFormatToString(target_string, maxc, "-0x%" PRIX64 "", RemoveSign(value.b));
            else PrintFormatToString(target_string, maxc, "0x%X", (int)value.b);
            break;
        case 2:
            if (value.b < 0) {
                StringCopyMaxCharTruncate(target_string, "-0b", maxc);
                UintToBinaryString(RemoveSign(value.b), 8, target_string+3);
            } else {
                StringCopyMaxCharTruncate(target_string, "0b", maxc);
                UintToBinaryString((uint64_t)value.b, 8, target_string+2);
            }
            break;
        }
        break;
    case BB_UBYTE:
        switch (base) {
        default:
        case 10:
            PrintFormatToString(target_string, maxc, "%u", (unsigned int)value.ub);
            break;
        case 16:
            PrintFormatToString(target_string, maxc,  "0x%X", (unsigned int)value.ub);
            break;
        case 2:
            StringCopyMaxCharTruncate(target_string, "0b", maxc);
            UintToBinaryString((uint64_t)value.ub, 8, target_string+2);
            break;
        }
        break;
    case BB_WORD:
        switch (base) {
        default:
        case 10:
            PrintFormatToString(target_string, maxc, "%d", (int)value.w);
            break;
        case 16:
            if (value.w < 0) PrintFormatToString(target_string, maxc, "-0x%" PRIX64 "", RemoveSign(value.w));
            else PrintFormatToString(target_string, maxc, "0x%X", (int)value.w);
            break;
        case 2:
            if (value.w < 0) {
                StringCopyMaxCharTruncate(target_string, "-0b", maxc);
                UintToBinaryString(RemoveSign(value.w), 16, target_string+3);
            } else {
                StringCopyMaxCharTruncate(target_string, "0b", maxc);
                UintToBinaryString((uint64_t)value.w, 16, target_string+2);
            }
            break;
        }
        break;
    case BB_UWORD:
        switch (base) {
        default:
        case 10:
            PrintFormatToString(target_string, maxc, "%u", (unsigned int)value.uw);
            break;
        case 16:
            PrintFormatToString(target_string, maxc, "0x%X", (unsigned int)value.uw);
            break;
        case 2:
            StringCopyMaxCharTruncate(target_string, "0b", maxc);
            UintToBinaryString((uint64_t)value.uw, 16, target_string+2);
            break;
        }
        break;
    case BB_DWORD:
        switch (base) {
        default:
        case 10:
            PrintFormatToString(target_string, maxc, "%d", (int)value.dw);
            break;
        case 16:
            if (value.dw < 0) PrintFormatToString(target_string, maxc, "-0x%" PRIX64 "", RemoveSign(value.dw));
            else PrintFormatToString(target_string, maxc, "0x%X", (int)value.dw);
            break;
        case 2:
            if (value.dw < 0) {
                StringCopyMaxCharTruncate(target_string, "-0b", maxc);
                UintToBinaryString(RemoveSign(value.dw), 32, target_string+3);
            } else {
                StringCopyMaxCharTruncate(target_string, "0b", maxc);
                UintToBinaryString((uint64_t)value.dw, 32, target_string+2);
            }
            break;
        }
        break;
    case BB_UDWORD:
        switch (base) {
        default:
        case 10:
            PrintFormatToString(target_string, maxc, "%u", (unsigned int)value.udw);
            break;
        case 16:
            PrintFormatToString(target_string, maxc, "0x%X", (unsigned int)value.udw);
            break;
        case 2:
            StringCopyMaxCharTruncate(target_string, "0b", maxc);
            UintToBinaryString((uint64_t)value.udw, 32, target_string+2);
            break;
        }
        break;
    case BB_QWORD:
        switch (base) {
        default:
        case 10:
            PrintFormatToString(target_string, maxc, "%" PRId64 "", value.qw);
            break;
        case 16:
            if (value.qw < 0LL) PrintFormatToString(target_string, maxc, "-0x%" PRIX64 "", RemoveSign(value.qw));
            else PrintFormatToString(target_string, maxc, "0x%" PRIX64 "", value.qw);
            break;
        case 2:
            if (value.qw < 0LL) {
                StringCopyMaxCharTruncate(target_string, "-0b", maxc);
                UintToBinaryString(RemoveSign(value.qw), 64, target_string+3);
            } else {
                StringCopyMaxCharTruncate(target_string, "0b", maxc);
                UintToBinaryString((uint64_t)value.qw, 64, target_string+2);
            }
            break;
        }
        break;
    case BB_UQWORD:
        switch (base) {
        default:
        case 10:
            PrintFormatToString(target_string, maxc, "%" PRIu64 "", value.uqw);
            break;
        case 16:
            PrintFormatToString(target_string, maxc, "0x%" PRIX64 "", value.uqw);
            break;
        case 2:
            StringCopyMaxCharTruncate(target_string, "0b", maxc);
            UintToBinaryString((uint64_t)value.uqw, 64, target_string+2);
            break;
        }
        break;
    case BB_FLOAT:
        {
            int Prec = 15;
            while (1) {
                PrintFormatToString (target_string, maxc, "%.*g", Prec, (double)value.f);
                if ((Prec++) == 18 || ((double)value.f == strtod (target_string, NULL))) break;
            }
        }
        break;
    case BB_DOUBLE:
        {
            int Prec = 15;
            while (1) {
                PrintFormatToString (target_string, maxc, "%.*g", Prec, value.d);
                if ((Prec++) == 18 || (value.d == strtod (target_string, NULL))) break;
            }
        }
        break;
    default:
        StringCopyMaxCharTruncate (target_string, "invalid data type", maxc);
        return -1;
    }
    return 0;
}

int string_to_bbvari (enum BB_DATA_TYPES type,
                      union BB_VARI *ret_value,
                      const char *src_string)
{
    double double_value = 0.0;
    uint64_t uint64_value = 0;
    int64_t int64_value = 0;
    int neg;   // If integer and negative than this is 1 otherwise 0
    int integer;
    int base = 10;
    int ret = 0;
    char *End;
    const char *src_string_save;

    while (isascii(*src_string) && isspace(*src_string)) src_string++;
    src_string_save = src_string;

    if (*src_string == '-') {
        neg = 1;
        src_string++;
    } else neg = 0;
    if ((*src_string == '0') && ((src_string[1] == 'x') || (src_string[1] == 'X'))) {
        integer = 1;
        base = 16;
        src_string += 2;
    } else if ((*src_string == '0') && ((src_string[1] == 'b') || (src_string[0] == 'B'))) {
        integer = 1;
        base = 2;
        src_string += 2;
    } else {
        int x;
        integer = 1;
        for (x = 0; src_string[x] != 0; x++) {
            if ((src_string[x] == 'e') || (src_string[x] == 'E') || (src_string[x] == '.')) {
                integer = 0;
                break;
            }
        }
    }
    if (integer) {
        if (neg) {
            uint64_value = strtoull(src_string, &End, base);
            uint64_value = ~uint64_value;
            uint64_value = uint64_value + 1;
            int64_value = *(int64_t*)&uint64_value;
        } else {
            uint64_value = strtoull(src_string, &End, base);
        }
    } else {
        double_value = strtod(src_string_save, &End);
    }
    if (*End != 0) ret = -1;    // it is not a number
    switch (type) {
    case BB_BYTE:
        if (integer) {
            if (neg) {
                if (int64_value < INT8_MIN) ret_value->b = INT8_MIN;
                else if (int64_value > INT8_MAX) ret_value->b = INT8_MAX;
                else ret_value->b = (int8_t)int64_value;
            } else {
                if (uint64_value > INT8_MAX) ret_value->b = INT8_MAX;
                else ret_value->b = (int8_t)uint64_value;
            }
        } else {
            if (double_value < INT8_MIN) ret_value->b = INT8_MIN;
            else if (double_value > INT8_MAX) ret_value->b = INT8_MAX;
            else {
                double v = round(double_value);
                ret_value->b = (int8_t)v;
            }
        }
        break;
    case BB_UBYTE:
        if (integer) {
            if (neg) {
                if (int64_value < 0) ret_value->ub = 0;
                else if (int64_value > UINT8_MAX) ret_value->ub = UINT8_MAX;
                else ret_value->ub = (uint8_t)int64_value;
            } else {
                if (uint64_value > UINT8_MAX) ret_value->ub = UINT8_MAX;
                else ret_value->ub = (uint8_t)uint64_value;
            }
        } else {
            if (double_value < 0.0) ret_value->ub = 0;
            else if (double_value > UINT8_MAX) ret_value->ub = UINT8_MAX;
            else {
                double v = round(double_value);
                ret_value->ub = (uint8_t)v;
            }
        }
        break;
    case BB_WORD:
        if (integer) {
            if (neg) {
                if (int64_value < INT16_MIN) ret_value->w = INT16_MIN;
                else if (int64_value > INT16_MAX) ret_value->w = INT16_MAX;
                else ret_value->w = (int16_t)int64_value;
            } else {
                if (uint64_value > INT16_MAX) ret_value->w = INT16_MAX;
                else ret_value->w = (int16_t)uint64_value;
            }
        } else {
            if (double_value < INT16_MIN) ret_value->w = INT16_MIN;
            else if (double_value > INT16_MAX) ret_value->w = INT16_MAX;
            else {
                double v = round(double_value);
                ret_value->w = (int16_t)v;
            }
        }
        break;
    case BB_UWORD:
        if (integer) {
            if (neg) {
                if (int64_value < 0) ret_value->uw = 0;
                else if (int64_value > UINT16_MAX) ret_value->uw = UINT16_MAX;
                else ret_value->uw = (uint16_t)int64_value;
            } else {
                if (uint64_value > UINT16_MAX) ret_value->uw = UINT16_MAX;
                else ret_value->uw = (uint16_t)uint64_value;
            }
        } else {
            if (double_value < 0.0) ret_value->uw = 0;
            else if (double_value > UINT16_MAX) ret_value->uw = UINT16_MAX;
            else {
                double v = round(double_value);
                ret_value->uw = (uint16_t)v;
            }
        }
        break;
    case BB_DWORD:
        if (integer) {
            if (neg) {
                if (int64_value < INT32_MIN) ret_value->dw = INT32_MIN;
                else if (int64_value > INT32_MAX) ret_value->dw = INT32_MAX;
                else ret_value->dw = (int32_t)int64_value;
            } else {
                if (uint64_value > INT32_MAX) ret_value->dw = INT32_MAX;
                else ret_value->dw = (int32_t)uint64_value;
            }
        } else {
            if (double_value < INT32_MIN) ret_value->dw = INT32_MIN;
            else if (double_value > INT32_MAX) ret_value->dw = INT32_MAX;
            else {
                double v = round(double_value);
                ret_value->dw = (int32_t)v;
            }
        }
        break;
    case BB_UDWORD:
        if (integer) {
            if (neg) {
                if (int64_value < 0) ret_value->udw = 0;
                else if (int64_value > UINT32_MAX) ret_value->udw = UINT32_MAX;
                else ret_value->udw = (uint32_t)int64_value;
            } else {
                if (uint64_value > UINT32_MAX) ret_value->udw = UINT32_MAX;
                else ret_value->udw = (uint32_t)uint64_value;
            }
        } else {
            if (double_value < 0.0) ret_value->udw = 0;
            else if (double_value > UINT32_MAX) ret_value->udw = UINT32_MAX;
            else {
                double v = round(double_value);
                ret_value->udw = (uint32_t)v;
            }
        }
        break;
    case BB_QWORD:
        if (integer) {
            if (neg) {
                ret_value->qw = int64_value;
            } else {
                if (uint64_value > INT64_MAX) ret_value->qw = INT64_MAX;
                else ret_value->qw = (int64_t)uint64_value;
            }
        } else {
            if (double_value < INT64_MIN) ret_value->qw = INT64_MIN;
            else if (double_value > (double)INT64_MAX) ret_value->qw = INT64_MAX;
            else {
                double v = round(double_value);
                ret_value->qw = (int64_t)v;
            }
        }
        break;
    case BB_UQWORD:
        if (integer) {
            if (neg) {
                if (int64_value < 0) ret_value->uqw = 0;
                else ret_value->uqw = (uint64_t)int64_value;
            } else {
                ret_value->uqw = uint64_value;
            }
        } else {
            if (double_value < 0.0) ret_value->uqw = 0;
            else if (double_value > (double)UINT64_MAX) ret_value->uqw = UINT64_MAX;
            else {
                double v = round(double_value);
                ret_value->uqw = (uint64_t)v;
            }
        }
        break;
    case BB_FLOAT:
        if (integer) {
            if (neg) {
                ret_value->f = (float)int64_value;
            } else {
                ret_value->f = (float)uint64_value;
            }
        } else {
            if (double_value < (double)-FLT_MAX) ret_value->f = (double)-FLT_MAX;
            else if (double_value > (double)FLT_MAX) ret_value->f = (double)FLT_MAX;
            else ret_value->f = (float)double_value;
        }
        break;
    case BB_DOUBLE:
        if (integer) {
            if (neg) {
                ret_value->d = (double)int64_value;
            } else {
                ret_value->d = (double)uint64_value;
            }
        } else {
            ret_value->d = (double)double_value;
        }
        break;
    default:
        ret_value->uqw = 0;
        ret = -1;
        break;
    }
    return ret;
}

int string_to_bbvari_without_type (enum BB_DATA_TYPES *ret_type,
                                   union BB_VARI *ret_value,
                                   const char *src_string)
{
    double double_value;
    uint64_t uint64_value;
    int64_t int64_value;
    int neg;   // If integer and negative than this is 1 otherwise 0
    int integer;
    int base = 10;
    int ret = 0;
    char *End;
    const char *src_string_save;

    while (isascii(*src_string) && isspace(*src_string)) src_string++;
    src_string_save = src_string;

    if (*src_string == '-') {
        neg = 1;
        src_string++;
    } else neg = 0;
    if ((*src_string == '0') && ((src_string[1] == 'x') || (src_string[1] == 'X'))) {
        integer = 1;
        base = 16;
        src_string += 2;
    } else if ((*src_string == '0') && ((src_string[1] == 'b') || (src_string[0] == 'B'))) {
        integer = 1;
        base = 2;
        src_string += 2;
    } else {
        int x;
        integer = 1;
        for (x = 0; src_string[x] != 0; x++) {
            if ((src_string[x] == 'e') || (src_string[x] == 'E') || (src_string[x] == '.')) {
                integer = 0;
                break;
            }
        }
    }
    if (integer) {
        if (neg) {
            uint64_value = strtoull(src_string, &End, base);
            uint64_value = ~uint64_value;
            uint64_value = uint64_value + 1;
            int64_value = *(int64_t*)&uint64_value;
            *ret_type = BB_QWORD;
            ret_value->qw = int64_value;
        } else {
            uint64_value = strtoull(src_string, &End, base);
            *ret_type = BB_UQWORD;
            ret_value->uqw = uint64_value;
        }
    } else {
        double_value = strtod(src_string_save, &End);
        *ret_type = BB_DOUBLE;
        ret_value->d = double_value;
    }
    if (*End != 0) ret = -1;    // It is not a number
    return ret;
}

int double_to_bbvari (enum BB_DATA_TYPES type,
                      union BB_VARI *bb_var,
                      double value)
{
    switch (type) {
    case BB_BYTE:
        bb_var->b = (int8_t)value;
        break;
    case BB_UBYTE:
        bb_var->ub = (uint8_t)value;
        break;
    case BB_WORD:
        bb_var->w = (int16_t)value;
        break;
    case BB_UWORD:
        bb_var->uw = (uint16_t)value;
        break;
    case BB_DWORD:
        bb_var->dw = (int32_t)value;
        break;
    case BB_UDWORD:
        bb_var->udw = (uint32_t)value;
        break;
    case BB_QWORD:
        bb_var->qw = (int64_t)value;
        break;
    case BB_UQWORD:
        bb_var->uqw = (uint64_t)value;
        break;
    case BB_FLOAT:
        bb_var->f = (float)value;
        break;
    case BB_DOUBLE:
        bb_var->d = value;
        break;
    default:
        return -1;
    }
    return 0;
}

int bbvari_to_double (enum BB_DATA_TYPES type,
                      union BB_VARI bb_var,
                      double *value)
{
    switch (type) {
    case BB_BYTE:
        *value = (double)bb_var.b;
        break;
    case BB_UBYTE:
        *value = (double)bb_var.ub;
        break;
    case BB_WORD:
        *value = (double)bb_var.w;
        break;
    case BB_UWORD:
        *value = (double)bb_var.uw;
        break;
    case BB_DWORD:
        *value = (double)bb_var.dw;
        break;
    case BB_UDWORD:
        *value = (double)bb_var.udw;
        break;
    case BB_QWORD:
        *value = (double)bb_var.qw;
        break;
    case BB_UQWORD:
        *value = (double)bb_var.uqw;
        break;
    case BB_FLOAT:
        *value = (double)bb_var.f;
        break;
    case BB_DOUBLE:
        *value = bb_var.d;
        break;
    default:
        return -1;
    }
    return 0;
}

int bbvari_to_int64 (enum BB_DATA_TYPES type,
                    union BB_VARI bb_var,
                    int64_t *value)
{
    switch (type) {
    case BB_BYTE:
        *value = (int64_t)bb_var.b;
        break;
    case BB_UBYTE:
        *value = (int64_t)bb_var.ub;
        break;
    case BB_WORD:
        *value = (int64_t)bb_var.w;
        break;
    case BB_UWORD:
        *value = (int64_t)bb_var.uw;
        break;
    case BB_DWORD:
        *value = (int64_t)bb_var.dw;
        break;
    case BB_UDWORD:
        *value = (int64_t)bb_var.udw;
        break;
    case BB_QWORD:
        *value = (int64_t)bb_var.qw;
        break;
    case BB_UQWORD:
        *value = (int64_t)bb_var.uqw;
        break;
    case BB_FLOAT:
        if (bb_var.f < INT64_MIN) *value = INT64_MIN;
        else if (bb_var.f > (double)INT64_MAX) *value = INT64_MAX;
        else if (bb_var.f < 0.0f) *value = (int64_t)(bb_var.f - 0.5f);
        else *value = (int64_t)(bb_var.f + 0.5f);
        break;
    case BB_DOUBLE:
        if (bb_var.d < INT64_MIN) *value = INT64_MIN;
        else if (bb_var.d > (double)INT64_MAX) *value = INT64_MAX;
        else if (bb_var.d < 0.0) *value = (int64_t)(bb_var.d - 0.5);
        else *value = (int64_t)(bb_var.d + 0.5);
        break;
    default:
        return -1;
    }
    return 0;
}

#ifndef get_process_index
int get_process_index (PID pid)
{
    int i;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
        return -1;
    }
    if (pid == 0) {
        return -1;
    }

    for (i = 0; i < MAX_PROCESSES; i++) {
        if (blackboard_infos.pid_access_masks[i] == pid) {
            return i;
        }
    }
    return -1;
}
#endif

int get_variable_index (VID vid)
{
    int i;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
        return -1;
    }

    i = (int)(vid >> 8);

    // Check if array index is in valid range
    if ((i < 0) || (i >= get_blackboardsize())) {
        return -1;
    }

    // Is the variable id valid
    if (blackboard[i].Vid != vid) {
        return -1;
    }

    // Return the index
    return i;
}

void set_default_varinfo (BB_VARIABLE *sp_vari_elem, int type)
{
    switch (s_main_ini_val.DefaultMinMaxValues) {
    case 1:    // Use the max. and min. of the corresponding data type
    case 2:
        switch (type) {
        case BB_BYTE:
            sp_vari_elem->pAdditionalInfos->Max = 127.0;
            sp_vari_elem->pAdditionalInfos->Min = -128.0;
            break;
        case BB_UBYTE:
            sp_vari_elem->pAdditionalInfos->Max = 255.0;
            sp_vari_elem->pAdditionalInfos->Min = 0.0;
            break;
        case BB_WORD:
            sp_vari_elem->pAdditionalInfos->Max = 32767.0;
            sp_vari_elem->pAdditionalInfos->Min = -32768.0;
            break;
        case BB_UWORD:
            sp_vari_elem->pAdditionalInfos->Max = 65535.0;
            sp_vari_elem->pAdditionalInfos->Min = 0.0;
            break;
        case BB_DWORD:
            sp_vari_elem->pAdditionalInfos->Max = 2147483647.0;
            sp_vari_elem->pAdditionalInfos->Min = -2147483648.0;
            break;
        case BB_UDWORD:
            sp_vari_elem->pAdditionalInfos->Max = 4294967295.0;
            sp_vari_elem->pAdditionalInfos->Min = 0.0;
            break;
        case BB_QWORD:
            sp_vari_elem->pAdditionalInfos->Max = (double)INT64_MAX;
            sp_vari_elem->pAdditionalInfos->Min = (double)INT64_MIN;
            break;
        case BB_UQWORD:
            sp_vari_elem->pAdditionalInfos->Max = (double)UINT64_MAX;
            sp_vari_elem->pAdditionalInfos->Min = 0.0;
            break;
        case BB_FLOAT:
            sp_vari_elem->pAdditionalInfos->Max = (double)FLT_MAX;
            sp_vari_elem->pAdditionalInfos->Min = (double)-FLT_MAX;
            break;
        case BB_DOUBLE:
            sp_vari_elem->pAdditionalInfos->Max = DBL_MAX;
            sp_vari_elem->pAdditionalInfos->Min = -DBL_MAX;
            break;
        default:
            sp_vari_elem->pAdditionalInfos->Max = 1.0;
            sp_vari_elem->pAdditionalInfos->Min = 0.0;
            break;
        }
        if (s_main_ini_val.DefaultMinMaxValues == 2) {
            if (sp_vari_elem->pAdditionalInfos->Max > s_main_ini_val.MaxMaxValues)
                sp_vari_elem->pAdditionalInfos->Max = s_main_ini_val.MaxMaxValues;
            if (sp_vari_elem->pAdditionalInfos->Min < s_main_ini_val.MinMinValues)
                sp_vari_elem->pAdditionalInfos->Min = s_main_ini_val.MinMinValues;
        }
        break;
    case 0:     // use min. = 0, max. = 1.0
    default:
        sp_vari_elem->pAdditionalInfos->Max = 1.0;
        sp_vari_elem->pAdditionalInfos->Min = 0.0;
        break;
    }
    sp_vari_elem->pAdditionalInfos->Width = 4;
    sp_vari_elem->pAdditionalInfos->Prec = 3;
    sp_vari_elem->pAdditionalInfos->StepType = 0;
    sp_vari_elem->pAdditionalInfos->Step = 1;
    sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
    sp_vari_elem->pAdditionalInfos->RgbColor = -1;
}


static char *StringToken (char *String, char **HelpPointer, char StopChar, int CountBracketFlag)
{
    char *Ret;
    int BrackeCount = 0;
    if (String != NULL) *HelpPointer = String;

    // Jump over whitespace
    while (isascii (**HelpPointer) && isspace (**HelpPointer)) (*HelpPointer)++;
    Ret = *HelpPointer;

    while ((**HelpPointer != StopChar) || BrackeCount) {
        if (**HelpPointer == 0) return NULL;    // End of string
        if (CountBracketFlag) {
            if (**HelpPointer == '(') BrackeCount++;
            if ((**HelpPointer == ')') && BrackeCount) BrackeCount--;
        }
        (*HelpPointer)++;
    }
    **HelpPointer = 0;
    (*HelpPointer)++;
    return Ret;
}

int set_varinfo_to_inientrys (BB_VARIABLE *sp_vari_elem,
                              char *inientrys_str)
{
    char *tmp_ptr;
    int red, green, blue;
    char *HelpPointer;

    // check the data type of the variable
    if ((tmp_ptr = StringToken (inientrys_str, &HelpPointer, ',', 0)) == NULL) {
        return WRONG_PARAMETER;
    }
    if (tmp_ptr[0] == 0) {
        return WRONG_PARAMETER;
    }
    // Unknown data type takeover from the INI file
    if (sp_vari_elem->Type == BB_UNKNOWN_DOUBLE) {
        sp_vari_elem->Type = atoi(tmp_ptr);
        if ((sp_vari_elem->Type > 7) || (sp_vari_elem->Type < 0)) {
            return WRONG_PARAMETER;
        }
    }

    // Unit
    if ((tmp_ptr = StringToken (NULL, &HelpPointer, ',', 0)) == NULL) {
        return WRONG_PARAMETER;
    }

    if (BBWriteUnit(tmp_ptr, sp_vari_elem) != 0) {
        // No memory available
        LeaveCriticalSection (&BlackboardCriticalSection);
        return BB_VAR_ADD_INFOS_MEM_ERROR;
    }

    // Min. value
    if ((tmp_ptr = StringToken (NULL, &HelpPointer, ',', 0)) == NULL) {
        return WRONG_INI_ENTRY;
    }
    // Is there something defined
    if ((tmp_ptr[0]) && (sp_vari_elem->Type != BB_UNKNOWN)) {
        sp_vari_elem->pAdditionalInfos->Min = strtod (tmp_ptr, NULL);
    }

    // Max. value
    if ((tmp_ptr = StringToken (NULL, &HelpPointer, ',', 0)) == NULL) {
        return WRONG_INI_ENTRY;
    }
    // Is there something defined
    if ((tmp_ptr[0]) && (sp_vari_elem->Type != BB_UNKNOWN)) {
        sp_vari_elem->pAdditionalInfos->Max = strtod (tmp_ptr, NULL);
    }
    // Check both values about (Min. and Max.)
    if (sp_vari_elem->pAdditionalInfos->Min >= sp_vari_elem->pAdditionalInfos->Max) {
        sp_vari_elem->pAdditionalInfos->Max = sp_vari_elem->pAdditionalInfos->Min + 1.0;
    }

    // Character count
    if ((tmp_ptr = StringToken (NULL, &HelpPointer, ',', 0)) == NULL) {
        return WRONG_INI_ENTRY;
    }
    // Is there something defined
    if (tmp_ptr[0]) {
        if ((sp_vari_elem->pAdditionalInfos->Width = (unsigned char)atoi(tmp_ptr)) < 1) {
            sp_vari_elem->pAdditionalInfos->Width = 1;
        }
    } else {
        // If nothing defined use 11
        sp_vari_elem->pAdditionalInfos->Width = 11;
    }

    // Precision
    if ((tmp_ptr = StringToken (NULL, &HelpPointer, ',', 0)) == NULL) {
        return WRONG_INI_ENTRY;
    }
    // Is there something defined
    if (tmp_ptr[0]) {
        if ((sp_vari_elem->pAdditionalInfos->Prec = (unsigned char)atoi (tmp_ptr)) >
            sp_vari_elem->pAdditionalInfos->Width - 1) {
            sp_vari_elem->pAdditionalInfos->Width = sp_vari_elem->pAdditionalInfos->Prec + 1;
        }
    } else {
        // If nothing defined use 0
        sp_vari_elem->pAdditionalInfos->Prec = 0;
    }
    // Type of step
    if ((tmp_ptr = StringToken (NULL, &HelpPointer, ',', 0)) == NULL) {
        return WRONG_INI_ENTRY;
    }
    // Is there something defined
    if (tmp_ptr[0]) {
        if ((sp_vari_elem->pAdditionalInfos->StepType = (unsigned char)atoi (tmp_ptr)) > 1) {
            sp_vari_elem->pAdditionalInfos->StepType = 0;
        }
    } else {
        // If nothing defined use 0
        sp_vari_elem->pAdditionalInfos->StepType = 0;
    }

    // Step width
    if ((tmp_ptr = StringToken (NULL, &HelpPointer, ',', 0)) == NULL) {
        return WRONG_INI_ENTRY;
    }
    // Is there something defined
    if (tmp_ptr[0]) {
        sp_vari_elem->pAdditionalInfos->Step = strtod(tmp_ptr, NULL);
    } else {
        // If nothing defined use 1.0
        sp_vari_elem->pAdditionalInfos->Step = 1.0;
    }

    // Conversion type
    if ((tmp_ptr = StringToken (NULL, &HelpPointer, ',', 0)) == NULL) {
        return WRONG_INI_ENTRY;
    }
    // Is there something defined
    if (tmp_ptr[0]) {
        if ((sp_vari_elem->pAdditionalInfos->Conversion.Type = atoi(tmp_ptr)) > BB_CONV_RAT_FUNC) {
            sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
        }
    } else {
        // If nothing defined use BB_CONV_NONE
        sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
    }

    // Conversion string
    if ((tmp_ptr = StringToken (NULL, &HelpPointer, ',', (sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA) ||
                                                         (sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_TEXTREP))) == NULL) {
        return WRONG_INI_ENTRY;
    }
    // Is there something defined
    // Is there a conversion type defined
    // Is the string not to huge
    if (tmp_ptr[0] != 0) {
        if ((sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA) ||
            (sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_TEXTREP)) {
            // otherwise copy the string
            if (BBWriteFormulaString(tmp_ptr, sp_vari_elem) != 0) {
              // No memory available
              LeaveCriticalSection (&BlackboardCriticalSection);
              return BB_VAR_ADD_INFOS_MEM_ERROR;
            }
        } else if ((sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_FACTOFF) ||
                   (sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_OFFFACT)) {
            char *end;
            sp_vari_elem->pAdditionalInfos->Conversion.Conv.FactorOffset.Factor = strtod(tmp_ptr, &end);
            if (*end == ':') {
                end++;
                sp_vari_elem->pAdditionalInfos->Conversion.Conv.FactorOffset.Offset = strtod(end, &end);
                if (*end != 0) {
                    sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
                }
            } else {
                sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
            }
        } else if ((sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_TAB_INTP) ||
                   (sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_TAB_NOINTP)) {
            int x;
            char *p;
            int Size = 1;
            const char *cc;
            // first we count the number of points
            cc = tmp_ptr;
            while (*cc != 0) {
                if (*cc == ':') Size++;
                cc++;
            }
            sp_vari_elem->pAdditionalInfos->Conversion.Conv.Table.Size = Size;
            sp_vari_elem->pAdditionalInfos->Conversion.Conv.Table.Values = (struct CONVERSION_TABLE_VALUE_PAIR*)my_malloc(Size * sizeof(struct CONVERSION_TABLE_VALUE_PAIR));
            if (sp_vari_elem->pAdditionalInfos->Conversion.Conv.Table.Values != NULL) {
                p = tmp_ptr;
                for (x = 0; x < Size; x++) {
                    sp_vari_elem->pAdditionalInfos->Conversion.Conv.Table.Values[x].Phys = strtod(p , &p);
                    if (*p == '/') p++;
                    else break;
                    sp_vari_elem->pAdditionalInfos->Conversion.Conv.Table.Values[x].Raw = strtod(p , &p);
                    if (*p == ':') p++;
                    else if (*p != 0) break;
                }
                if (x != Size) {
                    // an error occured
                    sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
                    my_free(sp_vari_elem->pAdditionalInfos->Conversion.Conv.Table.Values);
                    sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
                }
            }
        } else if (sp_vari_elem->pAdditionalInfos->Conversion.Type == BB_CONV_RAT_FUNC) {
            char *end;
            sp_vari_elem->pAdditionalInfos->Conversion.Conv.RatFunc.a = strtod(tmp_ptr, &end);
            if (*end == ':') {
                sp_vari_elem->pAdditionalInfos->Conversion.Conv.RatFunc.b = strtod(end + 1, &end);
                if (*end == ':') {
                    sp_vari_elem->pAdditionalInfos->Conversion.Conv.RatFunc.c = strtod(end + 1, &end);
                    if (*end == ':') {
                        sp_vari_elem->pAdditionalInfos->Conversion.Conv.RatFunc.d = strtod(end + 1, &end);
                        if (*end == ':') {
                            sp_vari_elem->pAdditionalInfos->Conversion.Conv.RatFunc.e = strtod(end + 1, &end);
                            if (*end == ':') {
                                sp_vari_elem->pAdditionalInfos->Conversion.Conv.RatFunc.f = strtod(end + 1, &end);
                                if (*end == 0) {
                                    end = NULL;  // complete
                                }
                            }
                        }
                    }
                }
            }
            if (end != NULL) {
                sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
            }
        } else {
            sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
        }
    } else {
        // If the string is empty use default value
        sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
    }

    // Color
    // Extrac string between backets
    if ((tmp_ptr = StringToken (NULL, &HelpPointer, '(', 0)) == NULL) {
        return WRONG_INI_ENTRY;
    }
    if ((tmp_ptr = StringToken (NULL, &HelpPointer, ')', 0)) == NULL) {
        return WRONG_INI_ENTRY;
    }
    // Is there something defined
    // Can read all three colors (r,g,b)
    if ((tmp_ptr[0]) &&
        (sscanf(tmp_ptr, "%d,%d,%d", &red, &green, &blue) == 3)) {
        // Translate color to rgb_color
        if ((red < 0) || (green < 0) || (blue < 0)) {
            sp_vari_elem->pAdditionalInfos->RgbColor = -1;
        } else {
            sp_vari_elem->pAdditionalInfos->RgbColor = RGB(red, green, blue);
        }
    } else {
        sp_vari_elem->pAdditionalInfos->RgbColor = -1; // unknown color
    }

    return 0;
}

uint64_t convert_double2uqword (double value)
{
    return ((value >= (double)UINT64_MAX + 0.5) ? UINT64_MAX :
            (value < 0) ? 0 : (uint64_t)(value + 0.5));
}

int64_t convert_double2qword (double value)
{
    if (value < 0) {
        return (value < (double)INT64_MIN) ? INT64_MIN : (int64_t)(value - 0.5);
    } else {
        return (value > (double)INT64_MAX) ? INT64_MAX : (int64_t)(value + 0.5);
    }
}

uint32_t convert_double2udword (double value)
{
    return ((value >= (double)UINT32_MAX + 0.5) ? UINT32_MAX :
            (value < 0) ? 0 : (uint32_t)(value + 0.5));
}

int32_t convert_double2dword (double value)
{
    if (value < 0) {
        return (value < (double)INT32_MIN) ? INT32_MIN : (int32_t)(value - 0.5);
    } else {
        return (value > (double)INT32_MAX) ? INT32_MAX : (int32_t)(value + 0.5);
    }
}

uint16_t convert_double2uword(double value)
{
    return ((value >= (double)UINT16_MAX + 0.5) ? UINT16_MAX :
            (value < 0) ? 0 : (uint16_t)(value + 0.5));
}

int16_t convert_double2word (double value)
{
    if (value < 0) {
        return (value < (double)INT16_MIN) ? INT16_MIN : (short)(value - 0.5);
    } else {
        return (value > (double)INT16_MAX) ? INT16_MAX : (short)(value + 0.5);
    }
}

unsigned char convert_double2ubyte (double value)
{
    return ((value >= (double)UCHAR_MAX + 0.5) ? UCHAR_MAX :
            (value < 0) ? 0 : (unsigned char)(value + 0.5));
}

char convert_double2byte (double value)
{
    if (value < 0) {
        return (value < (double)CHAR_MIN) ? CHAR_MIN : (char)(value - 0.5);
    } else {
        return (value > (double)CHAR_MAX) ? CHAR_MAX : (char)(value + 0.5);
    }
}

float convert_double2float (double value)
{
    if (value < 0) {
        return (value < -(double)MAXFLOAT) ? -MAXFLOAT : (float)value;
    } else {
        return (value > (double)MAXFLOAT) ? MAXFLOAT : (float)value;
    }
}

int equ_src_to_bin (BB_VARIABLE *sp_vari_elem)
{
    struct EXEC_STACK_ELEM *exec_stack_base;

    if ((exec_stack_base = solve_equation_replace_parameter ((char *)sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString)) == NULL) {
        return -1;
    }

    sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode = (unsigned char*)exec_stack_base;
#ifndef REMOTE_MASTER
    RegisterEquation (0, (char *)sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString, (struct EXEC_STACK_ELEM *)sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode,
                      (char*)sp_vari_elem->pAdditionalInfos->Name, EQU_TYPE_BLACKBOARD);
#endif
    return 0;
}


int IsValidVariableName (const char *Name)
{
    int x;

    if (Name == NULL) return 0;
    // First character must be a 'a'-'z', 'A'-'Z', '0'-'9' or a _
    if (!isascii (Name[0])) return 0;
    if (!isalpha (Name[0]) && (Name[0] != '_')) return 0;

    for (x = 1; Name[x] != 0; x++) {
        if (!isascii (Name[x])) return 0;
        if (isspace (Name[x])) return 0;
    }
    return 1;
}


int enable_bbvari_range_control (char *ProcessFilter, char *VariableFilter)
{
#ifdef REMOTE_MASTER
#else
    char *ProcessName;
    int Pid;
    READ_NEXT_PROCESS_NAME *Buffer;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
        return NOT_INITIALIZED;
    }

    Buffer = init_read_next_process_name (2);
    while ((ProcessName = read_next_process_name(Buffer)) != NULL) {
        if (!IsInternalProcess(ProcessName)) {
            if (!Compare2ProcessNames (ProcessName, ProcessFilter)) {
                Pid = get_pid_by_name (ProcessName);
                ChangeVariableRangeControlFilteredInsideCopyLists (Pid, VariableFilter, 1);
            }
        }
    }
    close_read_next_process_name(Buffer);
#endif
    return 0;
}


int disable_bbvari_range_control (char *ProcessFilter, char *VariableFilter)
{
#ifdef REMOTE_MASTER
#else
    char *ProcessName;
    int Pid;
    READ_NEXT_PROCESS_NAME *Buffer;

    // Is blackboard exists here or inside the remote master or not at all
    if (blackboard == NULL) {
        return NOT_INITIALIZED;
    }

    Buffer = init_read_next_process_name (2);
    while ((ProcessName = read_next_process_name(Buffer)) != NULL) {
        if (!IsInternalProcess(ProcessName)) {
            if (!Compare2ProcessNames (ProcessName, ProcessFilter)) {
                Pid = get_pid_by_name (ProcessName);
                ChangeVariableRangeControlFilteredInsideCopyLists (Pid, VariableFilter, 0);
            }
        }
    }
    close_read_next_process_name(Buffer);
#endif
    return 0;
}


int BBWriteName(const char* name, BB_VARIABLE *sp_vari_elem)
{
    sp_vari_elem->pAdditionalInfos->Name = StringRealloc(sp_vari_elem->pAdditionalInfos->Name, name);
    if (sp_vari_elem->pAdditionalInfos->Name == NULL) {
        return 1;
    }
    return 0;
}

int BBWriteDisplayName(const char* display_name, BB_VARIABLE *sp_vari_elem)
{
    if (display_name == NULL) {
        if (sp_vari_elem->pAdditionalInfos->DisplayName != NULL) my_free(sp_vari_elem->pAdditionalInfos->DisplayName);
        sp_vari_elem->pAdditionalInfos->DisplayName = NULL;
    } else  {
        sp_vari_elem->pAdditionalInfos->DisplayName = StringRealloc(sp_vari_elem->pAdditionalInfos->DisplayName, display_name);
        if (sp_vari_elem->pAdditionalInfos->DisplayName == NULL) {
            return 1;
        }
    }
    return 0;
}

int BBWriteUnit(const char* unit, BB_VARIABLE *sp_vari_elem)
{
    if (unit == NULL) {
        if (sp_vari_elem->pAdditionalInfos->Unit != NULL) my_free(sp_vari_elem->pAdditionalInfos->Unit);
        sp_vari_elem->pAdditionalInfos->Unit = NULL;
    } else {
        sp_vari_elem->pAdditionalInfos->Unit = StringRealloc(sp_vari_elem->pAdditionalInfos->Unit, unit);
        if (sp_vari_elem->pAdditionalInfos->Unit == NULL) {
            return 1;
        }
    }
    return 0;
}

int BBWriteComment(const char* comment, BB_VARIABLE *sp_vari_elem)
{
    if (comment == NULL) {
        if (sp_vari_elem->pAdditionalInfos->Comment != NULL) my_free(sp_vari_elem->pAdditionalInfos->Comment);
        sp_vari_elem->pAdditionalInfos->Comment = NULL;
    } else {
        sp_vari_elem->pAdditionalInfos->Comment = StringRealloc(sp_vari_elem->pAdditionalInfos->Comment, comment);
        if (sp_vari_elem->pAdditionalInfos->Comment == NULL) {
            return 1;
        }
    }
    return 0;
}

int BBWriteFormulaString(const char* formula_string, BB_VARIABLE *sp_vari_elem)
{
    if (formula_string == NULL) {
        if (sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString != NULL) my_free(sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString);
        sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString = NULL;
        sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
    } else {
        sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString = StringRealloc(sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString, formula_string);
        if (sp_vari_elem->pAdditionalInfos->Conversion.Conv.Formula.FormulaString == NULL) {
            sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
            return 1;
        }
    }
    return 0;
}

int BBWriteEnumString(const char* enum_string, BB_VARIABLE *sp_vari_elem)
{
    if (enum_string == NULL) {
        if (sp_vari_elem->pAdditionalInfos->Conversion.Conv.TextReplace.EnumString != NULL) my_free(sp_vari_elem->pAdditionalInfos->Conversion.Conv.TextReplace.EnumString);
        sp_vari_elem->pAdditionalInfos->Conversion.Conv.TextReplace.EnumString = NULL;
        sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
    } else {
        sp_vari_elem->pAdditionalInfos->Conversion.Conv.TextReplace.EnumString = StringRealloc(sp_vari_elem->pAdditionalInfos->Conversion.Conv.TextReplace.EnumString, enum_string);
        if (sp_vari_elem->pAdditionalInfos->Conversion.Conv.TextReplace.EnumString == NULL) {
            sp_vari_elem->pAdditionalInfos->Conversion.Type = BB_CONV_NONE;
            return 1;
        }
    }
    return 0;
}


int GetDataTypeByteSize (int par_DataType)
{
    switch (par_DataType) {
    case BB_BYTE:
    case BB_UBYTE:
        return 1;
    case BB_WORD:
    case BB_UWORD:
        return 2;
    case BB_DWORD:
    case BB_UDWORD:
        return 4;
    case BB_QWORD:
    case BB_UQWORD:
        return 8;
    case BB_FLOAT:
        return 4;
    case BB_DOUBLE:
        return 8;
    default:
        return 0;
    }
}

char *GetDataTypeName (int par_DataType)
{
    switch (par_DataType) {
    case BB_BYTE:
        return "BYTE";
    case BB_UBYTE:
        return "UBYTE";
    case BB_WORD:
        return "WORD";
    case BB_UWORD:
        return "UWORD";
    case BB_DWORD:
        return "DWORD";
    case BB_UDWORD:
        return "UDWORD";
    case BB_QWORD:
        return "QWORD";
    case BB_UQWORD:
        return "UQWORD";
    case BB_FLOAT:
        return "FLOAT";
    case BB_DOUBLE:
        return "DOUBLE";
    case BB_UNKNOWN:
        return "UNKNOWN";
    case BB_UNKNOWN_DOUBLE:
        return "UNKNOWN_DOUBLE";
    case BB_UNKNOWN_WAIT:
        return "UNKNOWN_WAIT";
    case BB_BITFIELD:
        return "BITFIELD";
    case BB_UNION:
        return "UNION";
    case BB_CONVERT_LIMIT_MIN_MAX:
        return "CONVERT_LIMIT_MIN_MAX";
    case BB_CONVERT_TO_DOUBLE:
        return "CONVERT_TO_DOUBLE";
    case BB_STRING:
        return "STRING";
    case BB_EXTERNAL_DATA:
        return "EXTERNAL_DATA";
    default:
        break;
    }
    return "error";
}

enum BB_DATA_TYPES GetDataTypebyName (const char *par_DataTypeName)
{
    // If it start with a "BB_" ignore this
    if ((par_DataTypeName[0] == 'B') &&
        (par_DataTypeName[1] == 'B') &&
        (par_DataTypeName[2] == '_')) {
        par_DataTypeName += 3;
    }
    if (!strcmp ("BYTE", par_DataTypeName)) {
        return BB_BYTE;
    }
    if (!strcmp ("UBYTE", par_DataTypeName)) {
        return BB_UBYTE;
    }
    if (!strcmp ("WORD", par_DataTypeName)) {
        return BB_WORD;
    }
    if (!strcmp ("UWORD", par_DataTypeName)) {
        return BB_UWORD;
    }
    if (!strcmp ("DWORD", par_DataTypeName)) {
        return BB_DWORD;
    }
    if (!strcmp ("UDWORD", par_DataTypeName)) {
        return BB_UDWORD;
    }
    if (!strcmp ("QWORD", par_DataTypeName)) {
        return BB_QWORD;
    }
    if (!strcmp ("UQWORD", par_DataTypeName)) {
        return BB_UQWORD;
    }
    if (!strcmp ("FLOAT", par_DataTypeName)) {
        return BB_FLOAT;
    }
    if (!strcmp ("DOUBLE", par_DataTypeName)) {
        return BB_DOUBLE;
    }
    if (!strcmp ("UNKNOWN", par_DataTypeName)) {
        return BB_UNKNOWN;
    }
    if (!strcmp ("UNKNOWN_DOUBLE", par_DataTypeName)) {
        return BB_UNKNOWN_DOUBLE;
    }
    return -1;
}

int get_datatype_min_max_value (int type, double *ret_min, double *ret_max)
{
    switch (type) {
    case BB_BYTE:
        *ret_max = 127.0;
        *ret_min = -128.0;
        break;
    case BB_UBYTE:
        *ret_max = 255.0;
        *ret_min = 0.0;
        break;
    case BB_WORD:
        *ret_max = 32767.0;
        *ret_min = -32768.0;
        break;
    case BB_UWORD:
        *ret_max = 65535.0;
        *ret_min = 0.0;
        break;
    case BB_DWORD:
        *ret_max = 2147483647.0;
        *ret_min = -2147483648.0;
        break;
    case BB_UDWORD:
        *ret_max = 4294967295.0;
        *ret_min = 0.0;
        break;
    case BB_QWORD:
        *ret_max = (double)INT64_MAX;
        *ret_min = (double)INT64_MIN;
        break;
    case BB_UQWORD:
        *ret_max = (double)UINT64_MAX;
        *ret_min = 0.0;
        break;
    case BB_FLOAT:
        *ret_max = (double)FLT_MAX;
        *ret_min = (double)-FLT_MAX;
        break;
    case BB_DOUBLE:
        *ret_max = DBL_MAX;
        *ret_min = -DBL_MAX;
        break;
    default:
        *ret_max = 1.0;
        *ret_min = 0.0;
        return -1;
    }
    return 0;
}


int get_datatype_min_max_union (int type, union BB_VARI *ret_min, union BB_VARI *ret_max)
{
    switch (type) {
    case BB_BYTE:
        ret_max->b = INT8_MAX;
        ret_min->b = INT8_MIN;
        break;
    case BB_UBYTE:
        ret_max->ub = UINT8_MAX;
        ret_min->ub = 0;
        break;
    case BB_WORD:
        ret_max->w = INT16_MAX;
        ret_min->w = INT16_MIN;
        break;
    case BB_UWORD:
        ret_max->uw = UINT16_MAX;
        ret_min->uw = 0;
        break;
    case BB_DWORD:
        ret_max->dw = INT32_MAX;
        ret_min->dw = INT32_MIN;
        break;
    case BB_UDWORD:
        ret_max->udw = UINT32_MAX;
        ret_min->udw = 0;
        break;
    case BB_QWORD:
        ret_max->qw = INT64_MAX;
        ret_min->qw = INT64_MIN;
        break;
    case BB_UQWORD:
        ret_max->uqw = UINT64_MAX;
        ret_min->uqw = 0;
        break;
    case BB_FLOAT:
        ret_max->f = FLT_MAX;
        ret_min->f = -FLT_MAX;
        break;
    case BB_DOUBLE:
        ret_max->d = DBL_MAX;
        ret_min->d = -DBL_MAX;
        break;
    default:
        ret_max->uqw = 0;
        ret_min->uqw = 0;
        return -1;
    }
    return 0;
}


int *get_all_bbvari_ids_without_lock (uint32_t flag, PID pid, int32_t *ret_ElemCount)
{
    int index;
    int pid_index;
    int *Ret = NULL;
    int RetBuffSize = 0;
    int RetElem = 0;
    uint64_t Mask;

    if ((flag & GET_ALL_BBVARI_ONLY_FOR_PROCESS) != 0) {
        // Is process-id valid
        if ((pid_index = get_process_index(pid)) == -1) {
            return NULL;   // Error
        }
        Mask = 1ULL <<  pid_index;
    } else {
        pid_index = -1;
        Mask = 0ULL;
    }

    // Look if variable already exists
    for (index = 0; index < get_blackboardsize(); index++) {
        if (blackboard[index].Vid > 0) {
            if ((((flag & GET_ALL_BBVARI_UNKNOWN_WAIT_TYPE) == GET_ALL_BBVARI_UNKNOWN_WAIT_TYPE) && (blackboard[index].Type == BB_UNKNOWN_WAIT)) ||
                (((flag & GET_ALL_BBVARI_EXISTING_TYPE) == GET_ALL_BBVARI_EXISTING_TYPE) && (blackboard[index].Type != BB_UNKNOWN_WAIT))) {
                if ((pid_index == -1) ||
                    (((flag & GET_ALL_BBVARI_ONLY_FOR_PROCESS_WITH_READ_ACCESS) == GET_ALL_BBVARI_ONLY_FOR_PROCESS_WITH_READ_ACCESS)
                         && ((blackboard[index].AccessFlags & Mask) != 0)) ||
                    (((flag & GET_ALL_BBVARI_ONLY_FOR_PROCESS_WITH_WRITE_ACCESS) == GET_ALL_BBVARI_ONLY_FOR_PROCESS_WITH_WRITE_ACCESS)
                         && ((blackboard[index].WrEnableFlags & Mask) != 0))) {
                    if (RetElem >= RetBuffSize) {
                        RetBuffSize = RetBuffSize + (RetBuffSize >> 1) + 128;
                        Ret = my_realloc (Ret, sizeof (int32_t*) * (size_t)RetBuffSize);
                        if (Ret == NULL) {
                            return NULL;   // Error
                        }
                    }
                    Ret[RetElem] = blackboard[index].Vid;
                    RetElem++;
                }
            }
        }
    }
    if (ret_ElemCount != NULL) *ret_ElemCount = RetElem;
    return Ret;
}

int *get_all_bbvari_ids (uint32_t flag, PID pid, int32_t *ret_ElemCount)
{
    int *Ret;
    EnterCriticalSection (&BlackboardCriticalSection);
    Ret = get_all_bbvari_ids_without_lock (flag, pid, ret_ElemCount);
    LeaveCriticalSection (&BlackboardCriticalSection);
    return Ret;
}


void free_all_bbvari_ids(int32_t *ids)
{
    my_free (ids);
}

void ConvertTo64BitDataType(union BB_VARI par_In, enum BB_DATA_TYPES par_Type,
                            union BB_VARI *ret_Out, enum BB_DATA_TYPES *ret_Type)
{
    switch (par_Type) {
    case BB_BYTE:
        ret_Out->qw = par_In.b;
        *ret_Type = BB_QWORD;
        break;
    case BB_WORD:
        ret_Out->qw = par_In.w;
        *ret_Type = BB_QWORD;
        break;
    case BB_DWORD:
        ret_Out->qw = par_In.dw;
        *ret_Type = BB_QWORD;
        break;
    default:
    case BB_QWORD:
        ret_Out->qw = par_In.qw;
        *ret_Type = BB_QWORD;
        break;
    case BB_UBYTE:
        ret_Out->uqw = par_In.ub;
        *ret_Type = BB_UQWORD;
        break;
    case BB_UWORD:
        ret_Out->uqw = par_In.uw;
        *ret_Type = BB_UQWORD;
        break;
    case BB_UDWORD:
        ret_Out->uqw = par_In.udw;
        *ret_Type = BB_UQWORD;
        break;
    case BB_UQWORD:
        ret_Out->uqw = par_In.uqw;
        *ret_Type = BB_UQWORD;
        break;
    case BB_FLOAT:
        ret_Out->d = (double)par_In.f;
        *ret_Type = BB_DOUBLE;
        break;
    case BB_DOUBLE:
        ret_Out->d = par_In.d;
        *ret_Type = BB_DOUBLE;
        break;
    }
}


union BB_VARI AddOffsetTo(union BB_VARI par_In, enum BB_DATA_TYPES par_Type, double par_Offset)
{
    double RoundedOffset = round(par_Offset);
    switch (par_Type) {
    case BB_BYTE:
        if ((par_Offset > 0.0) && ((int8_t)(par_In.b + (int8_t)RoundedOffset) < par_In.b)) {
            par_In.b = INT8_MAX;  // Check for overflow
        } else if ((par_Offset < 0.0) && ((int8_t)(par_In.b + (int8_t)RoundedOffset) > par_In.b)) {
            par_In.b = INT8_MIN;  //Check for underflow
        } else  {
            par_In.b += (int8_t)RoundedOffset;
        }
        break;
    case BB_WORD:
        if ((par_Offset > 0.0) && ((int16_t)(par_In.w + (int16_t)RoundedOffset) < par_In.w)) {
            par_In.w = INT16_MAX;  // Check for overflow
        } else if ((par_Offset < 0.0) && ((int16_t)(par_In.w + (int16_t)RoundedOffset) > par_In.w)) {
            par_In.w = INT16_MIN;  //Check for underflow
        } else  {
            par_In.w += (int16_t)RoundedOffset;
        }
        break;
    case BB_DWORD:
        if ((par_Offset > 0.0) && ((int32_t)(par_In.dw + (int32_t)RoundedOffset) < par_In.dw)) {
            par_In.dw = INT32_MAX;  // Check for overflow
        } else if ((par_Offset < 0.0) && ((int32_t)(par_In.dw + (int32_t)RoundedOffset) > par_In.dw)) {
            par_In.dw = INT32_MIN;  //Check for underflow
        } else  {
            par_In.dw += (int32_t)RoundedOffset;
        }
        break;
    case BB_QWORD:
        if ((par_Offset > 0.0) && (par_In.qw + (int64_t)RoundedOffset < par_In.qw)) {
            par_In.qw = INT64_MAX;  // Check for overflow
        } else if ((par_Offset < 0.0) && (par_In.qw + (int64_t)RoundedOffset > par_In.qw)) {
            par_In.qw = INT64_MIN;  //Check for underflow
        } else  {
            par_In.qw += (int64_t)RoundedOffset;
        }
        break;
    case BB_UBYTE:
        if (par_Offset > 0) {
            uint8_t Sum = par_In.ub - (uint8_t)-RoundedOffset;
            if (Sum < par_In.ub) {
                par_In.ub = UINT8_MAX;
            } else {
                par_In.ub = Sum;
            }
        } else {
            uint8_t NegRoundedOffset = (uint8_t)-RoundedOffset;
            uint8_t Diff = par_In.ub - NegRoundedOffset;
            if (Diff > par_In.ub) {
                par_In.ub = 0;
            } else {
                par_In.ub = Diff;
            }
        }
        break;
    case BB_UWORD:
        if (par_Offset > 0) {
            uint16_t Sum = par_In.uw - (uint16_t)-RoundedOffset;
            if (Sum < par_In.uw) {
                par_In.uw = UINT16_MAX;
            } else {
                par_In.uw = Sum;
            }
        } else {
            uint16_t NegRoundedOffset =  (uint16_t)-RoundedOffset;
            uint16_t Diff = par_In.uw - NegRoundedOffset;
            if (Diff > par_In.uw) {
                par_In.uw = 0;
            } else {
                par_In.uw = Diff;
            }
        }
        break;
    case BB_UDWORD:
        if (par_Offset > 0) {
            uint32_t Sum = par_In.udw - (uint32_t)-RoundedOffset;
            if (Sum < par_In.udw) {
                par_In.udw = UINT32_MAX;
            } else {
                par_In.udw = Sum;
            }
        } else {
            uint32_t NegRoundedOffset =  (uint32_t)-RoundedOffset;
            uint32_t Diff = par_In.udw - NegRoundedOffset;
            if (Diff > par_In.udw) {
                par_In.udw = 0;
            } else {
                par_In.udw = Diff;
            }
        }
        break;
    case BB_UQWORD:
        if (par_Offset > 0) {
            uint64_t Sum = par_In.uqw - (uint64_t)-RoundedOffset;
            if (Sum < par_In.uqw) {
                par_In.uqw = UINT64_MAX;
            } else {
                par_In.uqw = Sum;
            }
        } else {
            uint64_t NegRoundedOffset =  (uint64_t)-RoundedOffset;
            uint64_t Diff = par_In.uqw - NegRoundedOffset;
            if (Diff > par_In.uqw) {
                par_In.uqw = 0;
            } else {
                par_In.uqw = Diff;
            }
        }
        break;
    case BB_FLOAT:
        par_In.f += (float)par_Offset;
        break;
    case BB_DOUBLE:
        par_In.d += par_Offset;
        break;
    default:    // Error do nothing
        break;
    }
    return par_In;
}

int LimitToDataTypeRange(union BB_VARI par_In, enum BB_DATA_TYPES par_InType, enum BB_DATA_TYPES par_LimitToType, union BB_VARI *ret_Out)
{
    int ret = 0;
    int64_t value_i64 = 0;
    uint64_t value_ui64 = 0;
    double value_double = 0.0;
    int type;

    // Translate it into the 3 64 bit data types
    switch (par_InType) {
    case BB_BYTE:
        value_i64 = par_In.b;
        type = 0;
        break;
    case BB_WORD:
        value_i64 = par_In.w;
        type = 0;
        break;
    case BB_DWORD:
        value_i64 = par_In.dw;
        type = 0;
        break;
    case BB_QWORD:
        value_i64 = par_In.qw;
        type = 0;
        break;
    case BB_UBYTE:
        value_ui64 = par_In.ub;
        type = 1;
        break;
    case BB_UWORD:
        value_ui64 = par_In.uw;
        type = 1;
        break;
    case BB_UDWORD:
        value_ui64 = par_In.udw;
        type = 1;
        break;
    case BB_UQWORD:
        value_ui64 = par_In.uqw;
        type = 1;
        break;
    case BB_FLOAT:
        value_double = (double)par_In.f;
        type = 2;
        break;
    case BB_DOUBLE:
        value_double = par_In.d;
        type = 2;
        break;
    default:
        type = -1;
    }

    switch (par_LimitToType) {
    case BB_BYTE:
        switch(type) {
        case 0:  // int64
            if (value_i64 < INT8_MIN) { ret_Out->b = INT8_MIN; ret = -1; }
            else if (value_i64 > INT8_MAX) { ret_Out->b = INT8_MAX; ret = 1; }
            else ret_Out->b = (int8_t)value_i64;
            break;
        case 1:  // uint64
            if (value_ui64 > INT8_MAX) { ret_Out->b = INT8_MAX; ret = 1; }
            else ret_Out->b = (int8_t)value_ui64;
            break;
        case 2:  // double
            if (value_double < INT8_MIN) { ret_Out->b = INT8_MIN; ret = -1; }
            else if (value_double > INT8_MAX) { ret_Out->b = INT8_MAX; ret = 1; }
            else ret_Out->b = (int8_t)value_double;
            break;
        }
        break;
    case BB_WORD:
        switch(type) {
        case 0:  // int64
            if (value_i64 < INT16_MIN) { ret_Out->w = INT16_MIN; ret = -1; }
            else if (value_i64 > INT16_MAX) { ret_Out->w = INT16_MAX; ret = 1; }
            else ret_Out->w = (int16_t)value_i64;
            break;
        case 1:  // uint64
            if (value_ui64 > INT16_MAX) { ret_Out->w = INT16_MAX; ret = 1; }
            else ret_Out->w = (int16_t)value_ui64;
            break;
        case 2:  // double
            if (value_double < INT16_MIN) { ret_Out->w = INT16_MIN; ret = -1; }
            else if (value_double > INT16_MAX) { ret_Out->w = INT16_MAX; ret = 1; }
            else ret_Out->w = (int16_t)value_double;
            break;
        }
        break;
    case BB_DWORD:
        switch(type) {
        case 0:  // int64
            if (value_i64 < INT32_MIN) { ret_Out->dw = INT32_MIN; ret = -1; }
            else if (value_i64 > INT32_MAX) { ret_Out->dw = INT32_MAX; ret = 1; }
            else ret_Out->dw = (int32_t)value_i64;
            break;
        case 1:  // uint64
            if (value_ui64 > INT32_MAX) { ret_Out->dw = INT32_MAX; ret = 1; }
            else ret_Out->dw = (int32_t)value_ui64;
            break;
        case 2:  // double
            if (value_double < INT32_MIN) { ret_Out->dw = INT32_MIN; ret = -1; }
            else if (value_double > INT32_MAX) { ret_Out->dw = INT32_MAX; ret = 1; }
            else ret_Out->dw = (int32_t)value_double;
            break;
        }
        break;
    case BB_QWORD:
        switch(type) {
        case 0:  // int64
            ret_Out->qw = value_i64;
            break;
        case 1:  // uint64
            if (value_ui64 > INT64_MAX) { ret_Out->qw = INT64_MAX; ret = 1; }
            else ret_Out->qw = (int64_t)value_ui64;
            break;
        case 2:  // double
            if (value_double < INT64_MIN) { ret_Out->qw = INT64_MIN; ret = -1; }
            else if (value_double > (double)INT64_MAX) { ret_Out->qw = INT64_MAX; ret = 1; }
            else ret_Out->qw = (int64_t)value_double;
            break;
        }
        break;
    case BB_UBYTE:
        switch(type) {
        case 0:  // int64
            if (value_i64 < 0) { ret_Out->ub = 0; ret = -1; }
            else if (value_i64 > UINT8_MAX) { ret_Out->ub = UINT8_MAX; ret = 1; }
            else ret_Out->ub = (uint8_t)value_i64;
            break;
        case 1:  // uint64
            if (value_ui64 > UINT8_MAX) { ret_Out->ub = UINT8_MAX; ret = 1; }
            else ret_Out->ub = (uint8_t)value_ui64;
            break;
        case 2:  // double
            if (value_double < 0.0) { ret_Out->ub = 0; ret = -1; }
            else if (value_double > UINT8_MAX) { ret_Out->ub = UINT8_MAX; ret = 1; }
            else ret_Out->ub = (uint8_t)value_double;
            break;
        }
        break;
    case BB_UWORD:
        switch(type) {
        case 0:  // int64
            if (value_i64 < 0) { ret_Out->uw = 0; ret = -1; }
            else if (value_i64 > UINT16_MAX) { ret_Out->uw = UINT16_MAX; ret = 1; }
            else ret_Out->uw = (uint16_t)value_i64;
            break;
        case 1:  // uint64
            if (value_ui64 > UINT16_MAX) { ret_Out->uw = UINT16_MAX; ret = 1; }
            else ret_Out->uw = (uint16_t)value_ui64;
            break;
        case 2:  // double
            if (value_double < 0.0) { ret_Out->uw = 0; ret = -1; }
            else if (value_double > UINT16_MAX) { ret_Out->uw = UINT16_MAX; ret = 1; }
            else ret_Out->uw = (uint16_t)value_double;
            break;
        }
        break;
    case BB_UDWORD:
        switch(type) {
        case 0:  // int64
            if (value_i64 < 0) { ret_Out->udw = 0; ret = -1; }
            else if (value_i64 > UINT32_MAX) { ret_Out->udw = UINT32_MAX; ret = 1; }
            else ret_Out->udw = (uint32_t)value_i64;
            break;
        case 1:  // uint64
            if (value_ui64 > UINT32_MAX) { ret_Out->udw = UINT32_MAX; ret = 1; }
            else ret_Out->udw = (uint32_t)value_ui64;
            break;
        case 2:  // double
            if (value_double < 0.0) { ret_Out->udw = 0; ret = -1; }
            else if (value_double > UINT32_MAX) { ret_Out->udw = UINT32_MAX; ret = 1; }
            else ret_Out->udw = (uint32_t)value_double;
            break;
        }
        break;
    case BB_UQWORD:
        switch(type) {
        case 0:  // int64
            if (value_i64 < 0) { ret_Out->uqw = 0; ret = -1; }
            else ret_Out->uqw = (uint64_t)value_i64;
            break;
        case 1:  // uint64
            ret_Out->uqw = value_ui64;
            break;
        case 2:  // double
            if (value_double < 0.0) { ret_Out->uqw = 0; ret = -1; }
            else if (value_double > (double)UINT64_MAX) { ret_Out->uqw = UINT64_MAX; ret = 1; }
            else ret_Out->uqw = (uint64_t)value_double;
            break;
        }
        break;
    case BB_FLOAT:
        switch(type) {
        case 0:  // int64
            ret_Out->f = (float)value_i64;
            break;
        case 1:  // uint64
            ret_Out->f = (float)value_ui64;
            break;
        case 2:  // double
            if (value_double < (double)-FLT_MAX) { ret_Out->f = 0.0; ret = -1; }
            else if (value_double > (double)FLT_MAX) { ret_Out->f = FLT_MAX; ret = 1; }
            else ret_Out->f = (float)value_double;
            break;
        }
        break;
    case BB_DOUBLE:
        switch(type) {
        case 0:  // int64
            ret_Out->d = (double)value_i64;
            break;
        case 1:  // uint64
            ret_Out->d = (double)value_ui64;
            break;
        case 2:  // double
            ret_Out->d = value_double;
            break;
        }
        break;
    default:
        type = -1;
        break;
    }
    if (type < 0) ThrowError (1, "internel error %s (%i)", __FILE__, __LINE__);
    return ret;
}

int close_blackboard (void)
{
#ifndef REMOTE_MASTER
     if (s_main_ini_val.ConnectToRemoteMaster) {
         rm_WriteBackVariableSectionCache ();
         return rm_close_blackboard();
     }
#endif
    // is blackboard exists here or inside the remote master or not at all
    if (blackboard != NULL) {
        int index;
        EnterCriticalSection (&BlackboardCriticalSection);
        for (index = 0; index < get_blackboardsize(); index++) {
            if (blackboard[index].Vid > 0) {
                int32_t P1, P2;
                uint64_t HashCode = BuildHashCode(blackboard[index].pAdditionalInfos->Name);
                int index2 = BinarySearchIndex(blackboard[index].pAdditionalInfos->Name, HashCode, &P1, &P2);
                if (index2 == index) {
                    RemoveBinaryHashKey(P1, P2);
                    FreeBlackboardIndex(index);
                }
                __free_all_additionl_info_memorys(blackboard[index].pAdditionalInfos, 0);
            }
            if (blackboard[index].pAdditionalInfos != NULL) {
                BB_VARIABLE_ADDITIONAL_INFOS *Infos = blackboard[index].pAdditionalInfos;
                my_free(Infos);
            }
        }

        BB_VARIABLE *blackboard_base = blackboard - 1;  // we have not used the element 0 we have alloc one more as neccessary
        for (index = 0; index < (int)sizeof (BB_VARIABLE); index++) {
            if (((uint8_t*)blackboard_base)[index] != 0xFF) {
                ThrowError(1, "Internal error, somebody have written to blackboard with index -1");
                break;
            }
        }
        LeaveCriticalSection (&BlackboardCriticalSection);
    }
    return 0;
}
