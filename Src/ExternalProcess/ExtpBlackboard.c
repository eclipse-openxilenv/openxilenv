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
#include "Platform.h"

#include "StringMaxChar.h"
#include "PipeMessagesShared.h"
#include "XilEnvExtProc.h"
#include "ExtpProcessAndTaskInfos.h"
#include "ExtpBlackboardCopyLists.h"
#include "ExtpBaseMessages.h"
#include "ExtpMemoryAllocation.h"
#include "ExtpBlackboard.h"

static int XilEnvInternal_BbCacheGetOrAllocMemoryForDataAndName(EXTERN_PROCESS_TASK_INFOS_STRUCT *par_TaskInfo, const char *par_Name)
{
    int x;
    int FreeIndex = -1;
    int FoundIndex = -1;
    for (x = 1; x < par_TaskInfo->BbCaches.Count; x++) {  // not using index 0 
        if (par_TaskInfo->BbCaches.Entrys[x].AttachCounter <= 0) {
            if (FreeIndex != -1) {
                FreeIndex = x;
            }
        } else if (!strcmp(par_Name, (char*)(par_TaskInfo->BbCaches.Entrys[x].Buffer + 1))) {
            FoundIndex = x;
            break;
        }
    }
    if (FoundIndex >= 0) {
        par_TaskInfo->BbCaches.Entrys[FoundIndex].AttachCounter++;
        return FoundIndex;
    } else {
        int Len = strlen(par_Name) + 1;
        if (FreeIndex >= 0) {
            par_TaskInfo->BbCaches.Entrys[FreeIndex].AttachCounter = 1;

            par_TaskInfo->BbCaches.Entrys[FreeIndex].Buffer = (union SC_BB_VARI *)XilEnvInternal_realloc(par_TaskInfo->BbCaches.Entrys[FreeIndex].Buffer, sizeof(union SC_BB_VARI) + Len);
            if (par_TaskInfo->BbCaches.Entrys[FreeIndex].Buffer != NULL) {
                if (par_TaskInfo->BbCaches.Entrys[FreeIndex].Buffer == NULL) {
                    ThrowError (1, "out of memory");
                    return -1;
                }
                par_TaskInfo->BbCaches.Entrys[FreeIndex].Buffer->uqw = 0;
                StringCopyMaxCharTruncate((char*)(par_TaskInfo->BbCaches.Entrys[FreeIndex].Buffer + 1), par_Name, Len);
                par_TaskInfo->BbCaches.Entrys[FreeIndex].UniqueId = XilEnvInternal_BuildRefUniqueId (par_TaskInfo);
                return FreeIndex;
            } else {
                return -1;
            }
        } else {
            if (par_TaskInfo->BbCaches.Count == 0) par_TaskInfo->BbCaches.Count++;  // // dont use index 0
            FoundIndex = par_TaskInfo->BbCaches.Count;
            par_TaskInfo->BbCaches.Count++;
            // Add a new one because there are no free element
            if ((par_TaskInfo->BbCaches.Count) >= par_TaskInfo->BbCaches.Size) {
                int x = par_TaskInfo->BbCaches.Size;
                // Change the size
                par_TaskInfo->BbCaches.Size += 100;
                par_TaskInfo->BbCaches.Entrys = (BLACKBOARD_WRITE_READ_CACHE*)XilEnvInternal_realloc(par_TaskInfo->BbCaches.Entrys, sizeof(BLACKBOARD_WRITE_READ_CACHE) * par_TaskInfo->BbCaches.Size);
                if (par_TaskInfo->BbCaches.Entrys == NULL) {
                    ThrowError (1, "out of memory");
                    return -1;
                }
                for (; x < par_TaskInfo->BbCaches.Size; x++) {
                    par_TaskInfo->BbCaches.Entrys[x].Vid = -1;
                    par_TaskInfo->BbCaches.Entrys[x].Type = 0;
                    par_TaskInfo->BbCaches.Entrys[x].Buffer = NULL;
                    par_TaskInfo->BbCaches.Entrys[x].AttachCounter = 0;
                    par_TaskInfo->BbCaches.Entrys[x].DirectFlag =  1;
                }
            }
            par_TaskInfo->BbCaches.Entrys[FoundIndex].AttachCounter = 1;
            par_TaskInfo->BbCaches.Entrys[FoundIndex].Buffer = (union SC_BB_VARI *)XilEnvInternal_malloc(sizeof(union SC_BB_VARI) + Len);
            if (par_TaskInfo->BbCaches.Entrys[FoundIndex].Buffer == NULL) {
                ThrowError (1, "out of memory");
                return -1;
            }
            par_TaskInfo->BbCaches.Entrys[FoundIndex].Buffer->uqw = 0;
            StringCopyMaxCharTruncate((char*)(par_TaskInfo->BbCaches.Entrys[FoundIndex].Buffer + 1), par_Name, Len);
            par_TaskInfo->BbCaches.Entrys[FoundIndex].UniqueId = XilEnvInternal_BuildRefUniqueId (par_TaskInfo);
            return FoundIndex;
        }
    }
    return -1;   // not found
}

static int XilEnvInternal_BbCacheFreeMemoryForDataAndName(EXTERN_PROCESS_TASK_INFOS_STRUCT *par_TaskInfo, int par_Index)
{
    if (par_Index >= par_TaskInfo->BbCaches.Size) {
        ThrowError (1, "par_Index >= par_TaskInfo->BbCaches.Size");
        return -1;
    }
    if (par_TaskInfo->BbCaches.Entrys[par_Index].AttachCounter <= 0) {
        ThrowError (1, "Attach counter should not be <= 0");
        return -1;
    }
    par_TaskInfo->BbCaches.Entrys[par_Index].AttachCounter--;
    if (par_TaskInfo->BbCaches.Entrys[par_Index].AttachCounter <= 0) {
        XilEnvInternal_free (par_TaskInfo->BbCaches.Entrys[par_Index].Buffer);
    }
    return 0;
}

static int XilEnvInternal_BbCacheSetVidAndType(EXTERN_PROCESS_TASK_INFOS_STRUCT *par_TaskInfo, int par_Index, int par_Vid, enum SC_BB_DATA_TYPES par_Type)
{
    if (par_Index >= par_TaskInfo->BbCaches.Size) {
        ThrowError (1, "par_Index >= par_TaskInfo->BbCaches.Size");
        return -1;
    }
    if (par_TaskInfo->BbCaches.Entrys[par_Index].AttachCounter <= 0) {
        ThrowError (1, "Attach counter should not be <= 0");
        return -1;
    }
    par_TaskInfo->BbCaches.Entrys[par_Index].Vid = par_Vid;
    par_TaskInfo->BbCaches.Entrys[par_Index].Type = par_Type;
    par_TaskInfo->BbCaches.Entrys[par_Index].DirectFlag =  1;
    return par_Index;
}

static union SC_BB_VARI *XilEnvInternal_BbCacheGetAddress(EXTERN_PROCESS_TASK_INFOS_STRUCT *par_TaskInfo, int par_Index)
{
    if (par_Index >= par_TaskInfo->BbCaches.Size) {
        ThrowError (1, "par_Index >= par_TaskInfo->BbCaches.Size");
        return NULL;
    }
    if (par_TaskInfo->BbCaches.Entrys[par_Index].AttachCounter <= 0) {
        ThrowError (1, "Attach counter should not be <= 0");
        return NULL;
    }
    return par_TaskInfo->BbCaches.Entrys[par_Index].Buffer;
}

static uint64_t XilEnvInternal_BbCacheGetUniqueId(EXTERN_PROCESS_TASK_INFOS_STRUCT *par_TaskInfo, int par_Index)
{
    if (par_Index >= par_TaskInfo->BbCaches.Size) {
        ThrowError (1, "par_Index >= par_Cache->Size");
        return 0xFFFFFFFFFFFFFFFFULL;
    }
    if (par_TaskInfo->BbCaches.Entrys[par_Index].AttachCounter <= 0) {
        ThrowError (1, "Attach counter should not be <= 0");
        return 0xFFFFFFFFFFFFFFFFULL;
    }
    return par_TaskInfo->BbCaches.Entrys[par_Index].UniqueId;
}


static int XilEnvInternal_GetPosOfCacheEntry(BLACKBOARD_WRITE_READ_CACHES* par_Cache, int par_Vid, int *ret_FreeIndex)
{
    int x;
    *ret_FreeIndex = -1;
    for (x = 1; x < par_Cache->Count; x++) {    // not using index 0
        if (par_Cache->Entrys[x].Vid == -1) {
            if (*ret_FreeIndex != -1) {
                *ret_FreeIndex = x;
            }
        } else if (par_Cache->Entrys[x].Vid == par_Vid) {
            return x;
        }
    }
    return -1;   // nicht gefunden!
}


static int XilEnvInternal_RemoveFromBlackboardCache(EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr, int par_Vid)
{
    int Index;
    int FreeIndex;
    int Ret = -1;

    if (TaskPtr == NULL) {
        Ret = WRONG_PARAMETER;
    } else {
        if ((par_Vid < 0) || (par_Vid >= TaskPtr->BbCaches.Count)) {
            Ret = WRONG_PARAMETER;
        } else {
            Index = XilEnvInternal_GetPosOfCacheEntry(&TaskPtr->BbCaches, par_Vid, &FreeIndex);
            if (Index >= 0) {
                TaskPtr->BbCaches.Entrys[Index].AttachCounter--;
                if (TaskPtr->BbCaches.Entrys[Index].AttachCounter <= 0) {
                    unsigned long long UniqueId;
                    Ret = TaskPtr->BbCaches.Entrys[Index].Vid; // return the blackboard vid
                    TaskPtr->BbCaches.Entrys[Index].Vid = -1;
                    XilEnvInternal_RemoveVariableFromCopyLists (TaskPtr, NULL, Ret, TaskPtr->BbCaches.Entrys[Index].Buffer, 
                                                                 NULL, NULL, NULL, NULL, &UniqueId);
                }
            }
        }
    }
    return Ret;
}


static int XilEnvInternal_GetValueFromBlackboardCache(int par_Vid, int Type, union BB_VARI *ret_Value)
{
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    int DataType;
    int Ret = 0;

    TaskPtr = XilEnvInternal_GetTaskPtr ();
    if (TaskPtr == NULL) {
        ret_Value->uqw = 0;
        return WRONG_PARAMETER;
    } 
    if ((par_Vid < 0) || (par_Vid >= TaskPtr->BbCaches.Count)) {
        ret_Value->uqw = 0;
        Ret = WRONG_PARAMETER;
    } else if (TaskPtr->BbCaches.Entrys[par_Vid].Vid < 0) {
        ret_Value->uqw = 0;
        Ret = WRONG_PARAMETER;
    } else if ((TaskPtr->BbCaches.Entrys[par_Vid].DirectFlag) ||
        (TaskPtr->BbCaches.Entrys[par_Vid].Type == BB_UNKNOWN)) {  // Workaround for attach_bbvari (not cached)
        TaskPtr->BbCaches.Entrys[par_Vid].DirectFlag = 0;
        XilEnvInternal_PipeReadFromBlackboardVariable (TaskPtr, TaskPtr->BbCaches.Entrys[par_Vid].Vid, Type, ret_Value, &DataType);
        TaskPtr->BbCaches.Entrys[par_Vid].Buffer[0] = *(union SC_BB_VARI*)ret_Value;
    } else {
        if (Type == BB_CONVERT_TO_DOUBLE) {
            switch (TaskPtr->BbCaches.Entrys[par_Vid].Type) {
            case BB_BYTE:
                ret_Value->d = (double)TaskPtr->BbCaches.Entrys[par_Vid].Buffer->b;
                break;
            case BB_UBYTE:
                ret_Value->d = (double)TaskPtr->BbCaches.Entrys[par_Vid].Buffer->ub;
                break;
            case BB_WORD:
                ret_Value->d = (double)TaskPtr->BbCaches.Entrys[par_Vid].Buffer->w;
                break;
            case BB_UWORD:
                ret_Value->d = (double)TaskPtr->BbCaches.Entrys[par_Vid].Buffer->uw;
                break;
            case BB_DWORD:
                ret_Value->d = (double)TaskPtr->BbCaches.Entrys[par_Vid].Buffer->dw;
                break;
            case BB_UDWORD:
                ret_Value->d = (double)TaskPtr->BbCaches.Entrys[par_Vid].Buffer->udw;
                break;
            case BB_QWORD:
                ret_Value->d = (double)TaskPtr->BbCaches.Entrys[par_Vid].Buffer->qw;
                break;
		    case BB_UQWORD:
                ret_Value->d = (double)TaskPtr->BbCaches.Entrys[par_Vid].Buffer->uqw;
                break;
            case BB_FLOAT:
                ret_Value->d = (double)TaskPtr->BbCaches.Entrys[par_Vid].Buffer->f;
                break;
            case BB_DOUBLE:
                ret_Value->d = TaskPtr->BbCaches.Entrys[par_Vid].Buffer->b;
                break;
            default:
                ThrowError (1, "internal error (unknown data type %i) cannot covert to double", TaskPtr->BbCaches.Entrys[par_Vid].Type);
                break;
            } 
            TaskPtr->BbCaches.Entrys[par_Vid].Buffer[0] = *(union SC_BB_VARI*)ret_Value;
        } else {
            ret_Value[0].uqw = TaskPtr->BbCaches.Entrys[par_Vid].Buffer[0].uqw;
        }
    }
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);

    return Ret;
}

static int XilEnvInternal_SetValueToBlackboardCache(int par_Vid, int par_Type, union BB_VARI par_Value)
{
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    int Ret = 0;

    TaskPtr = XilEnvInternal_GetTaskPtr ();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }
    if ((par_Vid < 0) || (par_Vid >= TaskPtr->BbCaches.Count)) {
        Ret =  WRONG_PARAMETER;
    } else if (TaskPtr->BbCaches.Entrys[par_Vid].Vid < 0) {
        Ret = WRONG_PARAMETER;
    } else if ((par_Type == BB_CONVERT_LIMIT_MIN_MAX) ||
        (par_Type == BB_UNION) ||
        (TaskPtr->BbCaches.Entrys[par_Vid].Type == BB_UNKNOWN)) {   // Workaround for attach_bbvari (not cached)
        XilEnvInternal_PipeWriteToBlackboardVariable (TaskPtr, TaskPtr->BbCaches.Entrys[par_Vid].Vid, par_Type, par_Value);
        TaskPtr->BbCaches.Entrys[par_Vid].DirectFlag = 1;
    } else {
        TaskPtr->BbCaches.Entrys[par_Vid].Buffer[0].uqw = par_Value.uqw;
    }
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);

    return Ret;
}


int XilEnvInternal_DataTypeConvert (int dir, int type)
{
    if ((dir & DATA_TYPES_ONLY_SUGGESTED_CONVERT) == DATA_TYPES_ONLY_SUGGESTED_CONVERT) {
        if ((dir & DATA_TYPES_WAIT) == DATA_TYPES_WAIT) {
            switch (type) {
            case BB_BYTE:
                type = BB_UNKNOWN_WAIT_BYTE;
                break;
            case BB_UBYTE:
                type = BB_UNKNOWN_WAIT_UBYTE;
                break;
            case BB_WORD:
                type = BB_UNKNOWN_WAIT_WORD;
                break;
            case BB_UWORD:
                type = BB_UNKNOWN_WAIT_UWORD;
                break;
            case BB_DWORD:
                type = BB_UNKNOWN_WAIT_DWORD;
                break;
            case BB_UDWORD:
                type = BB_UNKNOWN_WAIT_UDWORD;
                break;
            case BB_QWORD:
                type = BB_UNKNOWN_WAIT_QWORD;
                break;
            case BB_UQWORD:
                type = BB_UNKNOWN_WAIT_UQWORD;
                break;
            case BB_FLOAT:
                type = BB_UNKNOWN_WAIT_FLOAT;
                break;
            case BB_DOUBLE:
                type = BB_UNKNOWN_WAIT_DOUBLE;
                break;
            // no defaul!:
            }
        } else {
            switch (type) {
            case BB_BYTE:
                type = BB_UNKNOWN_BYTE;
                break;
            case BB_UBYTE:
                type = BB_UNKNOWN_UBYTE;
                break;
            case BB_WORD:
                type = BB_UNKNOWN_WORD;
                break;
            case BB_UWORD:
                type = BB_UNKNOWN_UWORD;
                break;
            case BB_DWORD:
                type = BB_UNKNOWN_DWORD;
                break;
            case BB_UDWORD:
                type = BB_UNKNOWN_UDWORD;
                break;
            case BB_QWORD:
                type = BB_UNKNOWN_QWORD;
                break;
            case BB_UQWORD:
                type = BB_UNKNOWN_UQWORD;
                break;
            case BB_FLOAT:
                type = BB_UNKNOWN_FLOAT;
                break;
            case BB_DOUBLE:
                type = BB_UNKNOWN_DOUBLE;
                break;
            // no defaul!:
            }
        }
    } 
    return type;
}

VID XilEnvInternal_add_bbvari_dir(EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo,
                                  const char *name, int type, const char *unit, int dir,
                                  int ValueIsValid, union BB_VARI Value, int AddressIsValid, uint64_t Address,
                                  uint64_t UniqueId,
                                  int *ret_Type)
{
    type = XilEnvInternal_DataTypeConvert (dir, type);
    return XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo,
                                                              name, type, unit, dir,
                                                              CONVTYPE_UNDEFINED, "",
                                                              SC_NAN, SC_NAN,
                                                              WIDTH_UNDEFINED, PREC_UNDEFINED,
                                                              COLOR_UNDEFINED,
                                                              STEPS_UNDEFINED, 1.0,
                                                              ValueIsValid,
                                                              Value,
                                                              AddressIsValid,
                                                              Address,
                                                              UniqueId,
                                                              ret_Type);
}


VID add_bbvari_dir (const char *name, int type, const char *unit, int dir, int *ret_Type)
{
    union BB_VARI Value;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    VID Ret;

    MEMSET (&Value, 0, sizeof(Value));
    type = XilEnvInternal_DataTypeConvert (dir, type);
    TaskInfo = XilEnvInternal_GetTaskPtr ();

    Ret = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo, name, type, unit, dir,
                                                              CONVTYPE_UNDEFINED, "",
                                                              SC_NAN, SC_NAN,
                                                              WIDTH_UNDEFINED, PREC_UNDEFINED,
                                                              COLOR_UNDEFINED,
                                                              STEPS_UNDEFINED, 1.0,
                                                              FALSE,
                                                              Value,
                                                              FALSE,
                                                              0,
                                                              -1,      // should not include into the copy list
                                                              ret_Type);
    XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    return Ret;
}


// With the following functions it is possible to add/remove/access new none existing blackboard variables without having an address.

EXPORT_OR_IMPORT VID __FUNC_CALL_CONVETION__ add_bbvari (const char *name, int type, const char *unit)
{
    int RealType;
    int VidIndex;
    union BB_VARI Value;
    VID Vid, Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;

    TaskPtr = XilEnvInternal_GetTaskPtr ();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    MEMSET (&Value, 0, sizeof (Value));

    VidIndex = XilEnvInternal_BbCacheGetOrAllocMemoryForDataAndName(TaskPtr, name);

    Vid = XilEnvInternal_add_bbvari_dir (TaskPtr, name, type , unit, PIPE_API_REFERENCE_VARIABLE_DIR_READ_WRITE | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST, 
                                          FALSE, Value, 
                                          TRUE, (uint64_t)XilEnvInternal_BbCacheGetAddress(TaskPtr, VidIndex), 
                                          XilEnvInternal_BbCacheGetUniqueId(TaskPtr, VidIndex), 
                                          &RealType);
    if (Vid > 0) {
        Ret = XilEnvInternal_BbCacheSetVidAndType(TaskPtr, VidIndex, Vid, (enum SC_BB_DATA_TYPES)RealType);  // Ret is than an index into the cache
    } else {
        Ret = Vid;
    }
    if (Ret <= 0) {
        XilEnvInternal_BbCacheFreeMemoryForDataAndName(TaskPtr, VidIndex);
    } else {
        XilEnvInternal_InsertVariableToCopyLists (TaskPtr, name, Vid, 
                                                   (void*)XilEnvInternal_BbCacheGetAddress(TaskPtr, VidIndex), 
                                                   RealType, RealType, PIPE_API_REFERENCE_VARIABLE_DIR_READ_WRITE, 
                                                   XilEnvInternal_BbCacheGetUniqueId(TaskPtr, VidIndex));
    }
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);
    return Ret;
}

VID add_bbvari_all_infos_dir (const char *name, int type, const char *unit, int dir,
                              int convtype, const char *conversion,
                              double min, double max,
                              int width, int prec,
                              unsigned int rgb_color,
                              int steptype, double step, 
                              int *ret_Type)
{
    union BB_VARI Value;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    VID Ret;

    TaskPtr = XilEnvInternal_GetTaskPtr ();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    MEMSET (&Value, 0, sizeof (Value));
    type = XilEnvInternal_DataTypeConvert (dir, type);
    Ret = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskPtr, name, type + 0x1000, unit, dir,    // +0x1000 -> All informations
                                                              convtype, (char*)conversion,
                                                              min, max,
                                                              width, prec,
                                                              rgb_color,
                                                              steptype, step,
                                                              FALSE, Value, FALSE, 0, 
                                                              -1,
                                                              ret_Type);
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);
    return Ret;
}

VID add_bbvari_all_infos (const char *name, int type, const char *unit,
                          int convtype, const char *conversion,
                          double min, double max,
                          int width, int prec,
                          unsigned int rgb_color,
                          int steptype, double step,
                          int *ret_Type)
{
    return add_bbvari_all_infos_dir (name, type, unit, 0,
                                     convtype, conversion,
                                     min, max,
                                     width, prec,
                                     rgb_color,
                                     steptype, step,
                                     ret_Type);
}

int XilEnvInternal_remove_bbvari_not_cached(EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo, VID vid)
{
    // not cached!
    return XilEnvInternal_PipeRemoveBlackboardVariable (TaskInfo, vid, PIPE_API_REFERENCE_VARIABLE_DIR_READ_WRITE, -1, 0, 0); //READ_WRITE_REFERENCE);
}

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ remove_bbvari (VID vid)
{
    // cached
    int Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;

    TaskPtr = XilEnvInternal_GetTaskPtr ();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    XilEnvInternal_RemoveFromBlackboardCache(TaskPtr, vid);
    Ret = XilEnvInternal_PipeRemoveBlackboardVariable (TaskPtr, vid, PIPE_API_REFERENCE_VARIABLE_DIR_READ_WRITE, -1, 0, 0); //READ_WRITE_REFERENCE);
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);
    return Ret;
}


EXPORT_OR_IMPORT double __FUNC_CALL_CONVETION__ __GetNotANumber (void)
{
    double ret;
    union {
        unsigned long u32[2];
        double d;
    } x;

    x.u32[0] = 0;
    x.u32[1] = 0x7FF80000UL;
    ret = x.d;
    return ret;
}


EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ write_bbvari_byte (VID vid, signed char v)
{
    union BB_VARI Value;
    Value.b = v;
    XilEnvInternal_SetValueToBlackboardCache(vid, BB_BYTE, Value);
}

EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ write_bbvari_ubyte (VID vid, unsigned char v)
{
    union BB_VARI Value;
    Value.ub = v;
    XilEnvInternal_SetValueToBlackboardCache(vid, BB_UBYTE, Value);
}

EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ write_bbvari_word (VID vid, short v)
{
    union BB_VARI Value;
    Value.w = v;
    XilEnvInternal_SetValueToBlackboardCache(vid, BB_WORD, Value);
}

EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ write_bbvari_uword (VID vid, unsigned short v)
{
    union BB_VARI Value;
    Value.uw = v;
    XilEnvInternal_SetValueToBlackboardCache(vid, BB_UWORD, Value);
}

EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ write_bbvari_dword (VID vid, WSC_TYPE_INT32 v)
{
    union BB_VARI Value;
    Value.dw = (int32_t)v;
    XilEnvInternal_SetValueToBlackboardCache(vid, BB_DWORD, Value);
}

EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ write_bbvari_udword (VID vid, WSC_TYPE_UINT32 v)
{
    union BB_VARI Value;
    Value.udw = (uint32_t)v;
    XilEnvInternal_SetValueToBlackboardCache(vid, BB_UDWORD, Value);
}

EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ write_bbvari_qword (VID vid, long long v)
{
    union BB_VARI Value;
    Value.qw = v;
    XilEnvInternal_SetValueToBlackboardCache(vid, BB_QWORD, Value);
}

EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ write_bbvari_uqword (VID vid, unsigned long long v)
{
    union BB_VARI Value;
    Value.uqw = v;
    XilEnvInternal_SetValueToBlackboardCache(vid, BB_UQWORD, Value);
}

EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ write_bbvari_float (VID vid, float v)
{
    union BB_VARI Value;
    Value.f = v;
    XilEnvInternal_SetValueToBlackboardCache(vid, BB_FLOAT, Value);
}

EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ write_bbvari_double (VID vid, double v)
{
    union BB_VARI Value;
    Value.d = v;
    XilEnvInternal_SetValueToBlackboardCache(vid, BB_DOUBLE, Value);
}

EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ write_bbvari_union (VID vid, union BB_VARI v)
{
    XilEnvInternal_SetValueToBlackboardCache(vid, BB_UNION, v);
}


EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__  write_bbvari_minmax_check (VID vid, double v)
{
    union BB_VARI Value;
    Value.d = v;
    XilEnvInternal_SetValueToBlackboardCache(vid, BB_CONVERT_LIMIT_MIN_MAX, Value);
}


EXPORT_OR_IMPORT signed char  __FUNC_CALL_CONVETION__ read_bbvari_byte (VID vid)
{
    union BB_VARI Value;
    XilEnvInternal_GetValueFromBlackboardCache(vid, BB_BYTE, &Value);
    return Value.b;
}

EXPORT_OR_IMPORT unsigned char  __FUNC_CALL_CONVETION__ read_bbvari_ubyte (VID vid)
{
    union BB_VARI Value;
    XilEnvInternal_GetValueFromBlackboardCache(vid, BB_UBYTE, &Value);
    return Value.ub;
}

EXPORT_OR_IMPORT short  __FUNC_CALL_CONVETION__ read_bbvari_word (VID vid)
{
    union BB_VARI Value;
    XilEnvInternal_GetValueFromBlackboardCache(vid, BB_WORD, &Value);
    return Value.w;
}

EXPORT_OR_IMPORT unsigned short  __FUNC_CALL_CONVETION__ read_bbvari_uword (VID vid)
{
    union BB_VARI Value;
    XilEnvInternal_GetValueFromBlackboardCache(vid, BB_UWORD, &Value);
    return Value.uw;
}

EXPORT_OR_IMPORT WSC_TYPE_INT32 __FUNC_CALL_CONVETION__ read_bbvari_dword (VID vid)
{
    union BB_VARI Value;
    XilEnvInternal_GetValueFromBlackboardCache(vid, BB_DWORD, &Value);
    return (WSC_TYPE_INT32)Value.dw;
}

EXPORT_OR_IMPORT WSC_TYPE_UINT32  __FUNC_CALL_CONVETION__ read_bbvari_udword (VID vid)
{
    union BB_VARI Value;
    XilEnvInternal_GetValueFromBlackboardCache(vid, BB_UDWORD, &Value);
    return (WSC_TYPE_UINT32)Value.udw;
}

EXPORT_OR_IMPORT long long  __FUNC_CALL_CONVETION__ read_bbvari_qword (VID vid)
{
    union BB_VARI Value;
    XilEnvInternal_GetValueFromBlackboardCache(vid, BB_QWORD, &Value);
    return Value.qw;
}

EXPORT_OR_IMPORT unsigned long long  __FUNC_CALL_CONVETION__ read_bbvari_uqword (VID vid)
{
    union BB_VARI Value;
    XilEnvInternal_GetValueFromBlackboardCache(vid, BB_UQWORD, &Value);
    return Value.uqw;
}

EXPORT_OR_IMPORT float  __FUNC_CALL_CONVETION__ read_bbvari_float (VID vid)
{
    union BB_VARI Value;
    XilEnvInternal_GetValueFromBlackboardCache(vid, BB_FLOAT, &Value);
    return Value.f;
}

EXPORT_OR_IMPORT double  __FUNC_CALL_CONVETION__ read_bbvari_double (VID vid)
{
    union BB_VARI Value;
    XilEnvInternal_GetValueFromBlackboardCache(vid, BB_DOUBLE, &Value);
    return Value.d;
}


EXPORT_OR_IMPORT union BB_VARI  __FUNC_CALL_CONVETION__ read_bbvari_union (VID vid)
{
    union BB_VARI Value;
    XilEnvInternal_GetValueFromBlackboardCache(vid, BB_UNION, &Value);
    return Value;
}

EXPORT_OR_IMPORT double  __FUNC_CALL_CONVETION__ read_bbvari_convert_double (VID vid)
{
    union BB_VARI Value;
    XilEnvInternal_GetValueFromBlackboardCache(vid, BB_CONVERT_TO_DOUBLE, &Value);
    return Value.d;
}

EXPORT_OR_IMPORT VID  __FUNC_CALL_CONVETION__ attach_bbvari (const char *Name)
{
    VID Ret;
    EXTERN_PROCESS_TASK_INFOS_STRUCT* TaskPtr;
    int VidIndex;

    TaskPtr = XilEnvInternal_GetTaskPtr ();
    if (TaskPtr == NULL) {
        return WRONG_PARAMETER;
    }

    VidIndex = XilEnvInternal_BbCacheGetOrAllocMemoryForDataAndName(TaskPtr, Name);
    if (VidIndex <= 0) {
        Ret = WRONG_PARAMETER;
    } else {
        Ret = XilEnvInternal_PipeAttachBlackboardVariable (TaskPtr, Name);
        if (Ret > 0) {
            Ret = XilEnvInternal_BbCacheSetVidAndType(TaskPtr, VidIndex, Ret, (int)BB_UNKNOWN);     // Workaround for attach_bbvari (not cached)
        }
        if (Ret <= 0) {
            XilEnvInternal_BbCacheFreeMemoryForDataAndName(TaskPtr, VidIndex);
        }
    }
    XilEnvInternal_ReleaseTaskPtr(TaskPtr);

    return Ret;
}
