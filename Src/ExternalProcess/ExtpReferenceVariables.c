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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Platform.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "ExtpMemoryAllocation.h"
#include "ExtpBlackboardCopyLists.h"
#include "ExtpBaseMessages.h"
#include "ExtpBlackboard.h"
//  the most functions are declarated here
#include "XilEnvExtProc.h"
// the rest here
#include "ExtpReferenceVariables.h"




static void XilEnvInternal_referece_error(EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo, const char *name, int vid, void *ptr)
{
    char txt[512 + 256];
    int i;
    int ret;
    static char *errstr[14] = {/* 0*/ "blackboard not initialized",
                               /* 1*/ "called add_bbvari with wrong parameter",
                               /* 2*/ "unknown process",
                               /* 3*/ "syntax error in INI-file" ,
                               /* 4*/ "type clash already referenced with other type",
                               /* 5*/ "blackboard is full, adjust blackboard size in dialogbox: BasicSettings",
                               /* 6*/ "label name too long (max. 511 character)",
                               /* 7*/ "no valid label found for address",
                               /* 8*/ "no valid label name. the name have to start with an alphabetic or '_' character. afterward are numers, '[', ']' and '.' allowed. The Name should not have whitespaces.",
                               /* 9*/ "unknown variable",
                               /*10*/ "variable was filtered",
                               /*11*/ "variable are on stack. You cannot reference variables on stack",
                               /*12*/ "the reference copy list exceeds the max. size",
                               /*13*/ "no explanation" };
    static char OkToAllFlags[14];

    switch (vid) {
    case NOT_INITIALIZED: i = 0; break;
    case WRONG_PARAMETER: i = 1; break;
    case UNKNOWN_PROCESS: i = 2; break;
    case WRONG_INI_ENTRY: i = 3; break;
    case TYPE_CLASH: i = 4; break;
    case NO_FREE_VID: i = 5; break;
    case LABEL_NAME_TOO_LONG: i = 6; break;
    case ADDRESS_NO_VALID_LABEL: i = 7; break;
    case LABEL_NAME_NOT_VALID: i = 8; break;
    case UNKNOWN_VARIABLE: i = 9; break;
    case VARIABLE_REF_LIST_FILTERED: i = 10; break;
    case VARIABLE_REF_ADDRESS_IS_ON_STACK: i = 11; break;
    case VARIABLE_REF_LIST_MAX_SIZE_REACHED: i = 12; break;
    default: i = 13; break;
    }
    if (!OkToAllFlags[i]) {
        PrintFormatToString (txt, sizeof(txt), "cannot reference variable %s: because (%li)-> %s\n continue?", name, vid, errstr[i]);
        // ERROR_OK_OKALL_CANCEL     8
        ret = XilEnvInternal_PipeErrorPopupMessageAndWait(TaskInfo, 8, txt);
        if (ret == 2) {   // Cancle == 2
            exit(-1);
        } else if (ret == 100) { // OK to all == 100
            OkToAllFlags[i] = 1;
        }
    }
}

static int XilEnvInternal_CheckIfAddressIsValid(EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo, const char *par_Name, void *par_Address)
{
    // Visual 2003 ... 2010 dont know about GetCurrentThreadStackLimits
#if((defined _WIN32) && (_MSC_VER > 1600))
    PULONG HighLimit, LowLimit;
    GetCurrentThreadStackLimits((PULONG_PTR)&LowLimit, (PULONG_PTR)&HighLimit);
    if (((PULONG)par_Address >= LowLimit) && ((PULONG)par_Address < HighLimit)) {
        XilEnvInternal_referece_error(TaskInfo, par_Name, VARIABLE_REF_ADDRESS_IS_ON_STACK, par_Address);
        return 0;
    } else {
        return 1;
    }
#else
    return 1;
#endif
}



#define NO_NAME_USE_PTR \
    char addrstr[32]; \
    if (name == NULL) { \
        PrintFormatToString (addrstr, sizeof(addrstr), "0x%p", ptr); \
        name = addrstr; \
    }



// Here will collect the references happens inside the C starup routine

static int CStartupFinishedFlag;

int IsCallingFromCStartup (void)
{
    return CStartupFinishedFlag;
}

void XilEnvInternal_CStartupIsFinished (void)
{
    CStartupFinishedFlag = 1;
}

typedef struct {
    char *ProcessNamePostfix;
    enum BB_DATA_TYPES type;
    void *ptr; 
    char *name; 
    char *unit;
    int convtype;
    char *conversion;
    double min; 
    double max;
    int width; 
    int prec;
    unsigned int rgb_color;
    int steptype;
    double step;
    unsigned int flags;
} REFERENCE_ELEMENT;

static REFERENCE_ELEMENT *ReferenceArray;
static int ReferenceArraySize;
static int ReferenceArrayElemnts;

static char ProcessNamePostfix[MAX_PATH];
static int ReferenceCriticalSectionInitFlag;
static CRITICAL_SECTION ReferenceCriticalSection;

void InitAddReferenceToBlackboardAtEndOfFunction (void)
{
    if (!ReferenceCriticalSectionInitFlag) {  
        ReferenceCriticalSectionInitFlag = 1;
        InitializeCriticalSection (&ReferenceCriticalSection);
    }
}

static int MallocCopyString (char **DstPtr, char *Src)
{
    if (Src == NULL) {
        *DstPtr = NULL;
    } else {
        *DstPtr = StringMalloc (Src);
        if (*DstPtr == NULL) {
            ThrowError (1, "out of memory");
            return -1;
        }
    }
    return 0;
}


static int AddReferenceToBlackboardAtEndOfFunction (enum BB_DATA_TYPES type,
                                                    void *ptr, char *name, char *unit,
                                                    int convtype, char *conversion,
                                                    double min, double max,
                                                    int width, int prec,
                                                    unsigned int rgb_color,
                                                    int steptype, double step,
                                                    unsigned int flags)
{
    InitAddReferenceToBlackboardAtEndOfFunction();

    EnterCriticalSection(&ReferenceCriticalSection);
    
    if (ReferenceArraySize <= ReferenceArrayElemnts) {
        ReferenceArraySize += 10 + (ReferenceArraySize >> 2);
        ReferenceArray = (REFERENCE_ELEMENT*)XilEnvInternal_realloc (ReferenceArray, sizeof (REFERENCE_ELEMENT) * (size_t)ReferenceArraySize);
        if (ReferenceArray == NULL) {
            ThrowError (1, "out of memory");
            return -1;
        }
    }

    if (MallocCopyString (&ReferenceArray[ReferenceArrayElemnts].ProcessNamePostfix, ProcessNamePostfix)) return -1;
    ReferenceArray[ReferenceArrayElemnts].type = type;
    ReferenceArray[ReferenceArrayElemnts].ptr = ptr; 
    if (MallocCopyString (&ReferenceArray[ReferenceArrayElemnts].name, name)) return -1;
    if (MallocCopyString (&ReferenceArray[ReferenceArrayElemnts].unit, unit)) return -1;
    ReferenceArray[ReferenceArrayElemnts].convtype = convtype;
    if (MallocCopyString (&ReferenceArray[ReferenceArrayElemnts].conversion, conversion)) return -1;
    ReferenceArray[ReferenceArrayElemnts].min = min; 
    ReferenceArray[ReferenceArrayElemnts].max = max;
    ReferenceArray[ReferenceArrayElemnts].width = width; 
    ReferenceArray[ReferenceArrayElemnts].prec = prec;
    ReferenceArray[ReferenceArrayElemnts].rgb_color = rgb_color; 
    ReferenceArray[ReferenceArrayElemnts].steptype = steptype;
    ReferenceArray[ReferenceArrayElemnts].step = step;
    ReferenceArray[ReferenceArrayElemnts].flags = flags;
    
    ReferenceArrayElemnts++;
    
    LeaveCriticalSection(&ReferenceCriticalSection);

    return 0;
}


void SetReferenceForProcessPostfix (char *par_ProcessPostfix)
{
    InitAddReferenceToBlackboardAtEndOfFunction();

    EnterCriticalSection(&ReferenceCriticalSection);
    strncpy (ProcessNamePostfix, par_ProcessPostfix, sizeof (ProcessNamePostfix));
    ProcessNamePostfix[sizeof (ProcessNamePostfix)-1] = 0;
    LeaveCriticalSection(&ReferenceCriticalSection);
}    


void XilEnvInternal_DelayReferenceAllCStartupReferences (char *par_ProcessNamePostfix) 
{
    int vid;
    int x;
    int TypeBB;
    int Type;
    int Dir;
    union BB_VARI Value;
    int ValueIsValid;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;

    MEMSET (&Value, 0, sizeof (Value));

    EnterCriticalSection(&ReferenceCriticalSection);
    TaskInfo = XilEnvInternal_GetTaskPtr ();

    for (x = 0; x < ReferenceArrayElemnts; x++) {
        if (!strcmp (ReferenceArray[x].ProcessNamePostfix, par_ProcessNamePostfix)) {
            if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, ReferenceArray[x].name, ReferenceArray[x].ptr)) {
                if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, ReferenceArray[x].name, (unsigned long long)ReferenceArray[x].ptr, ReferenceArray[x].type)) {
                    Dir = (int)((ReferenceArray[x].flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
                    Type = XilEnvInternal_DataTypeConvert (Dir, ReferenceArray[x].type);
                    UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                    ValueIsValid = (ReferenceArray[x].ptr != NULL) && ((ReferenceArray[x].flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                    if (ValueIsValid) Value.uqw = *(uint64_t*)ReferenceArray[x].ptr;  // copy always 8 byte
                    else Value.uqw = 0;
                    if ((vid = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo,
                                                                                  ReferenceArray[x].name, 
                                                                                  Type + 0x1000,   // +0x1000 -> all Infos 
                                                                                  ReferenceArray[x].unit,
                                                                                  Dir,
                                                                                  ReferenceArray[x].convtype, ReferenceArray[x].conversion,
                                                                                  ReferenceArray[x].min, ReferenceArray[x].max,
                                                                                  ReferenceArray[x].width, ReferenceArray[x].prec,
                                                                                  ReferenceArray[x].rgb_color,
                                                                                  ReferenceArray[x].steptype, ReferenceArray[x].step,
                                                                                  ValueIsValid,
                                                                                  Value, 
                                                                                  TRUE, (unsigned long long)ReferenceArray[x].ptr, 
                                                                                  UniqueId,    // Position inside of the copy list
                                                                                  &TypeBB)) <= 0) {
                        if (((vid == UNKNOWN_VARIABLE) && ((ReferenceArray[x].flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                            (vid == VARIABLE_REF_LIST_FILTERED)) {
                            // no error message
                        } else {
                            XilEnvInternal_referece_error (TaskInfo, ReferenceArray[x].name, vid, ReferenceArray[x].ptr);
                        }
                    } else {
                        XilEnvInternal_InsertVariableToCopyLists (TaskInfo, ReferenceArray[x].name, vid, ReferenceArray[x].ptr, TypeBB, ReferenceArray[x].type, (int)ReferenceArray[x].flags, UniqueId);
                    }
                }
            }
        }
    }
    XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    LeaveCriticalSection(&ReferenceCriticalSection);
}

// double
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_double_var_flags (double *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_DOUBLE, ptr, (char*)name, (char*)unit, CONVTYPE_UNDEFINED, "", SC_NAN, SC_NAN, WIDTH_UNDEFINED, PREC_UNDEFINED,COLOR_UNDEFINED, STEPS_UNDEFINED, 1.0,
                                                 (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_DOUBLE)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.d= *ptr;
                else Value.d = 0.0;
                if ((vid = XilEnvInternal_add_bbvari_dir (TaskInfo, name, BB_DOUBLE, unit, (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST), 
                                                           ValueIsValid, Value, TRUE, (unsigned long long)ptr, UniqueId, NULL)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, BB_DOUBLE, BB_DOUBLE, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_double_var (double *ptr, const char *name)
{
    reference_double_var_flags (ptr, name, NULL, READ_WRITE_REFERENCE);
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_double_var_unit (double *ptr, const char *name, const char *unit)
{
    reference_double_var_flags (ptr, name, unit, READ_WRITE_REFERENCE);
}

// float
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_float_var_flags (float *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_FLOAT, ptr, (char*)name, (char*)unit, CONVTYPE_UNDEFINED, "", SC_NAN, SC_NAN, WIDTH_UNDEFINED, PREC_UNDEFINED,COLOR_UNDEFINED, STEPS_UNDEFINED, 1.0,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_FLOAT)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.f = *ptr;
                else Value.f = 0.0;
                if ((vid = XilEnvInternal_add_bbvari_dir (TaskInfo, name, BB_FLOAT, unit, (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST), 
                                                           ValueIsValid, Value, TRUE, (unsigned long long)ptr, UniqueId, NULL)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, BB_FLOAT, BB_FLOAT, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_float_var (float *ptr, const char *name)
{
    reference_float_var_flags (ptr, name, NULL, READ_WRITE_REFERENCE);
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_float_var_unit (float *ptr, const char *name, const char *unit)
{
    reference_float_var_flags (ptr, name, unit, READ_WRITE_REFERENCE);
}

// qword
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_qword_var_flags (long long *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_QWORD, ptr, (char*)name, (char*)unit, CONVTYPE_UNDEFINED, "", SC_NAN, SC_NAN, WIDTH_UNDEFINED, PREC_UNDEFINED,COLOR_UNDEFINED, STEPS_UNDEFINED, 1.0,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_QWORD)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.qw = *ptr;
                else Value.qw = 0;
                if ((vid = XilEnvInternal_add_bbvari_dir (TaskInfo, name, BB_QWORD, unit, (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST), 
                                                           ValueIsValid, Value, TRUE, (unsigned long long)ptr, UniqueId, NULL)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, BB_QWORD, BB_QWORD, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_qword_var (long long *ptr, const char *name)
{
    reference_qword_var_flags (ptr, name, NULL, READ_WRITE_REFERENCE);
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_qword_var_unit (long long *ptr, const char *name, const char *unit)
{
    reference_qword_var_flags (ptr, name, unit, READ_WRITE_REFERENCE);
}

// uqword
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_uqword_var_flags (unsigned long long *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_UQWORD, ptr, (char*)name, (char*)unit, CONVTYPE_UNDEFINED, "", SC_NAN, SC_NAN, WIDTH_UNDEFINED, PREC_UNDEFINED,COLOR_UNDEFINED, STEPS_UNDEFINED, 1.0,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_UQWORD)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.uqw = *ptr;
                else Value.uqw = 0;
                if ((vid = XilEnvInternal_add_bbvari_dir (TaskInfo, name, BB_UQWORD, unit, (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST), 
                                                           ValueIsValid, Value, TRUE, (unsigned long long)ptr, UniqueId, NULL)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, BB_UQWORD, BB_UQWORD, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_uqword_var (unsigned long long *ptr, const char *name)
{
    reference_uqword_var_flags (ptr, name, NULL, READ_WRITE_REFERENCE);
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_uqword_var_unit (unsigned long long *ptr, const char *name, const char *unit)
{
    reference_uqword_var_flags (ptr, name, unit, READ_WRITE_REFERENCE);
}

// dword
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_dword_var_flags (WSC_TYPE_INT32 *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_DWORD, ptr, (char*)name, (char*)unit, CONVTYPE_UNDEFINED, "", SC_NAN, SC_NAN, WIDTH_UNDEFINED, PREC_UNDEFINED,COLOR_UNDEFINED, STEPS_UNDEFINED, 1.0,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_DWORD)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.dw = *ptr;
                else Value.dw = 0;
                if ((vid = XilEnvInternal_add_bbvari_dir (TaskInfo, name, BB_DWORD, unit, (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST), 
                                                           ValueIsValid, Value, TRUE, (unsigned long long)ptr, UniqueId, NULL)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, BB_DWORD, BB_DWORD, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_dword_var (WSC_TYPE_INT32 *ptr, const char *name)
{
    reference_dword_var_flags (ptr, name, NULL, READ_WRITE_REFERENCE);
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_dword_var_unit (WSC_TYPE_INT32 *ptr, const char *name, const char *unit)
{
    reference_dword_var_flags (ptr, name, unit, READ_WRITE_REFERENCE);
}

// udword
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_udword_var_flags (WSC_TYPE_UINT32 *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_UDWORD, ptr, (char*)name, (char*)unit, CONVTYPE_UNDEFINED, "", SC_NAN, SC_NAN, WIDTH_UNDEFINED, PREC_UNDEFINED,COLOR_UNDEFINED, STEPS_UNDEFINED, 1.0,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_UDWORD)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.udw = *ptr;
                else Value.udw = 0;
                if ((vid = XilEnvInternal_add_bbvari_dir (TaskInfo, name, BB_UDWORD, unit, (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST), 
                                                           ValueIsValid, Value, TRUE, (unsigned long long)ptr, UniqueId, NULL)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, BB_UDWORD, BB_UDWORD, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_udword_var (WSC_TYPE_UINT32 *ptr, const char *name)
{
    reference_udword_var_flags (ptr, name, NULL, READ_WRITE_REFERENCE);
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_udword_var_unit (WSC_TYPE_UINT32 *ptr, const char *name, const char *unit)
{
    reference_udword_var_flags (ptr, name, unit, READ_WRITE_REFERENCE);
}

// word
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_word_var_flags (short *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_WORD, ptr, (char*)name, (char*)unit, CONVTYPE_UNDEFINED, "", SC_NAN, SC_NAN, WIDTH_UNDEFINED, PREC_UNDEFINED,COLOR_UNDEFINED, STEPS_UNDEFINED, 1.0,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_WORD)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.w = *ptr;
                else Value.w = 0;
                if ((vid = XilEnvInternal_add_bbvari_dir (TaskInfo, name, BB_WORD, unit, (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST), 
                                                           ValueIsValid, Value, TRUE, (unsigned long long)ptr, UniqueId, NULL)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, BB_WORD, BB_WORD, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_word_var (short *ptr, const char *name)
{
    reference_word_var_flags (ptr, name, NULL, READ_WRITE_REFERENCE);
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_word_var_unit (short *ptr, const char *name, const char *unit)
{
    reference_word_var_flags (ptr, name, unit, READ_WRITE_REFERENCE);
}

// uword
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_uword_var_flags (unsigned short *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_UWORD, ptr, (char*)name, (char*)unit, CONVTYPE_UNDEFINED, "", SC_NAN, SC_NAN, WIDTH_UNDEFINED, PREC_UNDEFINED,COLOR_UNDEFINED, STEPS_UNDEFINED, 1.0,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_UWORD)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.uw = *ptr;
                else Value.uw = 0;
                if ((vid = XilEnvInternal_add_bbvari_dir (TaskInfo, name, BB_UWORD, unit, (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST), 
                                                           ValueIsValid, Value, TRUE, (unsigned long long)ptr, UniqueId, NULL)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, BB_UWORD, BB_UWORD, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_uword_var (unsigned short *ptr, const char *name)
{
    reference_uword_var_flags (ptr, name, NULL, READ_WRITE_REFERENCE);
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_uword_var_unit (unsigned short *ptr, const char *name, const char *unit)
{
    reference_uword_var_flags (ptr, name, unit, READ_WRITE_REFERENCE);
}

// byte
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_byte_var_flags (signed char *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_BYTE, ptr, (char*)name, (char*)unit, CONVTYPE_UNDEFINED, "", SC_NAN, SC_NAN, WIDTH_UNDEFINED, PREC_UNDEFINED,COLOR_UNDEFINED, STEPS_UNDEFINED, 1.0,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_BYTE)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.b = *ptr;
                else Value.b = 0;
                if ((vid = XilEnvInternal_add_bbvari_dir (TaskInfo, name, BB_BYTE, unit, (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST), 
                                                           ValueIsValid, Value, TRUE, (unsigned long long)ptr, UniqueId, NULL)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, BB_BYTE, BB_BYTE, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_byte_var (signed char *ptr, const char *name)
{
    reference_byte_var_flags (ptr, name, NULL, READ_WRITE_REFERENCE);
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_byte_var_unit (signed char *ptr, const char *name, const char *unit)
{
    reference_byte_var_flags (ptr, name, unit, READ_WRITE_REFERENCE);
}

// ubyte
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_ubyte_var_flags (unsigned char *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_UBYTE, ptr, (char*)name, (char*)unit, CONVTYPE_UNDEFINED, "", SC_NAN, SC_NAN, WIDTH_UNDEFINED, PREC_UNDEFINED,COLOR_UNDEFINED, STEPS_UNDEFINED, 1.0,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_UBYTE)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.ub = *ptr;
                else Value.ub = 0;
                if ((vid = XilEnvInternal_add_bbvari_dir (TaskInfo, name, BB_UBYTE, unit, (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST), 
                                                           ValueIsValid, Value, TRUE, (unsigned long long)ptr, UniqueId, NULL)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, BB_UBYTE, BB_UBYTE, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_ubyte_var (unsigned char *ptr, const char *name)
{
    reference_ubyte_var_flags (ptr, name, NULL, READ_WRITE_REFERENCE);
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__  reference_ubyte_var_unit (unsigned char *ptr, const char *name, const char *unit)
{
    reference_ubyte_var_flags (ptr, name, unit, READ_WRITE_REFERENCE);
}

// unbekannter Datentyp (Daten-Typ aus aus Debug-Infos lesen)
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_xxx_var_flags (void *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;
    int RealType;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_GET_DATA_TYPE_FOM_DEBUGINFO, ptr, (char*)name, (char*)unit, CONVTYPE_UNDEFINED, "", SC_NAN, SC_NAN, WIDTH_UNDEFINED, PREC_UNDEFINED,COLOR_UNDEFINED, STEPS_UNDEFINED, 1.0,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_GET_DATA_TYPE_FOM_DEBUGINFO)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.uqw = *(uint64_t*)ptr;  // copy always 8 byte
                else Value.uqw = 0;
                if ((vid = XilEnvInternal_add_bbvari_dir (TaskInfo, name, BB_GET_DATA_TYPE_FOM_DEBUGINFO, unit, (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST), 
                                                           ValueIsValid, Value, TRUE, (unsigned long long)ptr, UniqueId, &RealType)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, RealType, RealType, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_xxx_var (void *ptr, const char *name)
{
    reference_xxx_var_flags (ptr, name, NULL, READ_WRITE_REFERENCE);
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_xxx_var_unit (void *ptr, const char *name, const char *unit)
{
    reference_xxx_var_flags (ptr, name, unit, READ_WRITE_REFERENCE);
}


// *******************************************************************************************************************
// Referenzierungs-Methoden mit allen Infos
/* neue Referenz-Funktionen denen alle Variableninfos uebergeben werden */
// *******************************************************************************************************************

// double
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_double_var_all_infos_flags (double *ptr, const char *name, const char *unit,
                                           int convtype, const char *conversion,
                                           double min, double max,
                                           int width, int prec,
                                           unsigned int rgb_color,
                                           int steptype, double step,
                                           unsigned int flags)
{
    int vid;
    int TypeBB;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_DOUBLE, ptr, (char*)name, (char*)unit, convtype, (char*)conversion, min, max, width, prec, rgb_color, steptype, step,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_DOUBLE)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.d = *ptr;
                else Value.d = 0.0;
                if ((vid = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo, name, BB_DOUBLE + 0x1000, unit,   // + 0x1000 -> all Infos
                                                                              (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST),
                                                                              convtype, (char*)conversion,
                                                                              min, max,
                                                                              width, prec,
                                                                              rgb_color,
                                                                              steptype, step, 
                                                                              ValueIsValid, Value,
                                                                              TRUE, (unsigned long long)ptr,
                                                                              UniqueId,
                                                                              &TypeBB)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, TypeBB, BB_DOUBLE, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_double_var_all_infos (double *ptr, const char *name, const char *unit,
                                     int convtype, const char *conversion,
                                     double min, double max,
                                     int width, int prec,
                                     unsigned int rgb_color,
                                     int steptype, double step)
{
    reference_double_var_all_infos_flags (ptr, name, unit,
                                          convtype, conversion,
                                          min, max,
                                          width, prec,
                                          rgb_color,
                                          steptype, step,
                                          READ_WRITE_REFERENCE);
}

// float
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_float_var_all_infos_flags (float *ptr, const char *name, const char *unit,
                                          int convtype,const  char *conversion,
                                          double min, double max,
                                          int width, int prec,
                                          unsigned int rgb_color,
                                          int steptype, double step,
                                          unsigned int flags)
{
    int vid;
    int TypeBB;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_FLOAT, ptr, (char*)name, (char*)unit, convtype, (char*)conversion, min, max, width, prec, rgb_color, steptype, step,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_FLOAT)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.f = *ptr;
                else Value.f = 0.0;
                if ((vid = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo, name, BB_FLOAT + 0x1000, unit,   // + 0x1000 -> all Infos
                                                                              (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST),
                                                                              convtype, (char*)conversion,
                                                                              min, max,
                                                                              width, prec,
                                                                              rgb_color,
                                                                              steptype, step, 
                                                                              ValueIsValid, Value,
                                                                              TRUE, (unsigned long long)ptr,
                                                                              UniqueId,
                                                                              &TypeBB)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, TypeBB, BB_FLOAT, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_float_var_all_infos (float *ptr, const char *name, const char *unit,
                                    int convtype, const char *conversion,
                                    double min, double max,
                                    int width, int prec,
                                    unsigned int rgb_color,
                                    int steptype, double step)
{
    reference_float_var_all_infos_flags (ptr, name, unit,
                                         convtype, conversion,
                                         min, max,
                                         width, prec,
                                         rgb_color,
                                         steptype, step,
                                         READ_WRITE_REFERENCE);
}

// qword
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_qword_var_all_infos_flags (long long *ptr, const char *name, const char *unit,
                                          int convtype, const char *conversion,
                                          double min, double max,
                                          int width, int prec,
                                          unsigned int rgb_color,
                                          int steptype, double step,
                                          unsigned int flags)
{
    int vid;
    int TypeBB;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_QWORD, ptr, (char*)name, (char*)unit, convtype, (char*)conversion, min, max, width, prec, rgb_color, steptype, step,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_QWORD)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.qw = *ptr;
                else Value.qw = 0;
                if ((vid = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo, name, BB_QWORD + 0x1000, unit,   // + 0x1000 -> all Infos 
                                                                              (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST),
                                                                              convtype, (char*)conversion,
                                                                              min, max,
                                                                              width, prec,
                                                                              rgb_color,
                                                                              steptype, step, 
                                                                              ValueIsValid, Value,
                                                                              TRUE, (unsigned long long)ptr,
                                                                              UniqueId,
                                                                              &TypeBB)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, TypeBB, BB_QWORD, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_qword_var_all_infos (long long *ptr, const char *name, const char *unit,
                                    int convtype, const char *conversion,
                                    double min, double max,
                                    int width, int prec,
                                    unsigned int rgb_color,
                                    int steptype, double step)
{
    reference_qword_var_all_infos_flags (ptr, name, unit,
                                         convtype, conversion,
                                         min, max,
                                         width, prec,
                                         rgb_color,
                                         steptype, step,
                                         READ_WRITE_REFERENCE);
}

// uqword
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_uqword_var_all_infos_flags (unsigned long long *ptr, const char *name, const char *unit,
                                           int convtype, const char *conversion,
                                           double min, double max,
                                           int width, int prec,
                                           unsigned int rgb_color,
                                           int steptype, double step,
                                           unsigned int flags)
{
    int vid;
    int TypeBB;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_UQWORD, ptr, (char*)name, (char*)unit, convtype, (char*)conversion, min, max, width, prec, rgb_color, steptype, step,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_UQWORD)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.uqw = *ptr;
                else Value.uqw = 0;
                if ((vid = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo, name, BB_UQWORD + 0x1000, unit,   // + 0x1000 -> all Infos
                                                                              (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST),   
                                                                              convtype, (char*)conversion,
                                                                              min, max,
                                                                              width, prec,
                                                                              rgb_color,
                                                                              steptype, step, 
                                                                              ValueIsValid, Value,
                                                                              TRUE, (unsigned long long)ptr,
                                                                              UniqueId,
                                                                              &TypeBB)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, TypeBB, BB_UQWORD, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    } 
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_uqword_var_all_infos (unsigned long long *ptr, const char *name, const char *unit,
                                    int convtype, const char *conversion,
                                    double min, double max,
                                    int width, int prec,
                                    unsigned int rgb_color,
                                    int steptype, double step)
{
    reference_uqword_var_all_infos_flags (ptr, name, unit,
                                         convtype, conversion,
                                         min, max,
                                         width, prec,
                                         rgb_color,
                                         steptype, step,
                                         READ_WRITE_REFERENCE);
}

// dword
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_dword_var_all_infos_flags (WSC_TYPE_INT32 *ptr, const char *name, const char *unit,
                                           int convtype, const char *conversion,
                                           double min, double max,
                                           int width, int prec,
                                           unsigned int rgb_color,
                                           int steptype, double step,
                                           unsigned int flags)
{
    int vid;
    int TypeBB;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_DWORD, ptr, (char*)name, (char*)unit, convtype, (char*)conversion, min, max, width, prec, rgb_color, steptype, step,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_DWORD)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.dw = *ptr;
                else Value.dw = 0;
                if ((vid = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo, name, BB_DWORD + 0x1000, unit,   // + 0x1000 -> all Infos
                                                                              (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST),   
                                                                              convtype, (char*)conversion,
                                                                              min, max,
                                                                              width, prec,
                                                                              rgb_color,
                                                                              steptype, step, 
                                                                              ValueIsValid, Value,
                                                                              TRUE, (unsigned long long)ptr,
                                                                              UniqueId,
                                                                              &TypeBB)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, TypeBB, BB_DWORD, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_dword_var_all_infos (WSC_TYPE_INT32 *ptr, const char *name, const char *unit,
                                     int convtype, const char *conversion,
                                     double min, double max,
                                     int width, int prec,
                                     unsigned int rgb_color,
                                     int steptype, double step)
{
    reference_dword_var_all_infos_flags (ptr, name, unit,
                                          convtype, conversion,
                                          min, max,
                                          width, prec,
                                          rgb_color,
                                          steptype, step,
                                          READ_WRITE_REFERENCE);
}

// udword
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_udword_var_all_infos_flags (WSC_TYPE_UINT32 *ptr, const char *name, const char *unit,
                                           int convtype, const char *conversion,
                                           double min, double max,
                                           int width, int prec,
                                           unsigned int rgb_color,
                                           int steptype, double step,
                                           unsigned int flags)
{
    int vid;
    int TypeBB;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_UDWORD, ptr, (char*)name, (char*)unit, convtype, (char*)conversion, min, max, width, prec, rgb_color, steptype, step,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_UDWORD)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.udw = *ptr;
                else Value.udw = 0;
                if ((vid = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo, name, BB_UDWORD + 0x1000, unit,   // + 0x1000 -> all Infos
                                                                              (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST),   
                                                                              convtype, (char*)conversion,
                                                                              min, max,
                                                                              width, prec,
                                                                              rgb_color,
                                                                              steptype, step, 
                                                                              ValueIsValid, Value,
                                                                              TRUE, (unsigned long long)ptr,
                                                                              UniqueId,
                                                                              &TypeBB)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, TypeBB, BB_UDWORD, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_udword_var_all_infos (WSC_TYPE_UINT32 *ptr, const char *name, const char *unit,
                                     int convtype, const char *conversion,
                                     double min, double max,
                                     int width, int prec,
                                     unsigned int rgb_color,
                                     int steptype, double step)
{
    reference_udword_var_all_infos_flags (ptr, name, unit,
                                          convtype, conversion,
                                          min, max,
                                          width, prec,
                                          rgb_color,
                                          steptype, step,
                                          READ_WRITE_REFERENCE);
}

// word
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_word_var_all_infos_flags (short *ptr, const char *name, const char *unit,
                                         int convtype, const char *conversion,
                                         double min, double max,
                                         int width, int prec,
                                         unsigned int rgb_color,
                                         int steptype, double step,
                                         unsigned int flags)
{
    int vid;
    int TypeBB;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR
 
    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_WORD, ptr, (char*)name, (char*)unit, convtype, (char*)conversion, min, max, width, prec, rgb_color, steptype, step,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_WORD)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.w = *ptr;
                else Value.w = 0;
                if ((vid = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo, name, BB_WORD + 0x1000, unit,   // + 0x1000 -> all Infos
                                                                              (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST),
                                                                              convtype, (char*)conversion,
                                                                              min, max,
                                                                              width, prec,
                                                                              rgb_color,
                                                                              steptype, step, 
                                                                              ValueIsValid, Value,
                                                                              TRUE, (unsigned long long)ptr,
                                                                              UniqueId,
                                                                              &TypeBB)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, TypeBB, BB_WORD, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_word_var_all_infos (short *ptr, const char *name, const char *unit,
                                   int convtype, const char *conversion,
                                   double min, double max,
                                   int width, int prec,
                                   unsigned int rgb_color,
                                   int steptype, double step)
{
    reference_word_var_all_infos_flags (ptr, name, unit,
                                        convtype, conversion,
                                        min, max,
                                        width, prec,
                                        rgb_color,
                                        steptype, step,
                                        READ_WRITE_REFERENCE);
}

// uword
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_uword_var_all_infos_flags (unsigned short *ptr, const char *name, const char *unit,
                                          int convtype, const char *conversion,
                                          double min, double max,
                                          int width, int prec,
                                          unsigned int rgb_color,
                                          int steptype, double step,
                                          unsigned int flags)
{
    int vid;
    int TypeBB;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_UWORD, ptr, (char*)name, (char*)unit, convtype, (char*)conversion, min, max, width, prec, rgb_color, steptype, step,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_UWORD)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.uw = *ptr;
                else Value.uw = 0;
                if ((vid = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo, name, BB_UWORD + 0x1000, unit,   // + 0x1000 -> all Infos 
                                                                              (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST),
                                                                              convtype, (char*)conversion,
                                                                              min, max,
                                                                              width, prec,
                                                                              rgb_color,
                                                                              steptype, step, 
                                                                              ValueIsValid, Value,
                                                                              TRUE, (unsigned long long)ptr,
                                                                              UniqueId,
                                                                              &TypeBB)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, TypeBB, BB_UWORD, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_uword_var_all_infos (unsigned short *ptr, const char *name, const char *unit,
                                    int convtype, const char *conversion,
                                    double min, double max,
                                    int width, int prec,
                                    unsigned int rgb_color,
                                    int steptype, double step)
{
    reference_uword_var_all_infos_flags (ptr, name, unit,
                                         convtype, conversion,
                                         min, max,
                                         width, prec,
                                         rgb_color,
                                         steptype, step,
                                         READ_WRITE_REFERENCE);
}

// byte
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_byte_var_all_infos_flags (signed char *ptr, const char *name, const char *unit,
                                         int convtype, const char *conversion,
                                         double min, double max,
                                         int width, int prec,
                                         unsigned int rgb_color,
                                         int steptype, double step,
                                         unsigned int flags)
{
    int vid;
    int TypeBB;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_BYTE, ptr, (char*)name, (char*)unit, convtype, (char*)conversion, min, max, width, prec, rgb_color, steptype, step,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_BYTE)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.b = *ptr;
                else Value.b = 0;
                if ((vid = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo, name, BB_BYTE + 0x1000, unit,   // + 0x1000 -> all Infos 
                                                                              (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST),
                                                                              convtype, (char*)conversion,
                                                                              min, max,
                                                                              width, prec,
                                                                              rgb_color,
                                                                              steptype, step, 
                                                                              ValueIsValid, Value,
                                                                              TRUE, (unsigned long long)ptr,
                                                                              UniqueId,
                                                                              &TypeBB)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, TypeBB, BB_BYTE, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_byte_var_all_infos (signed char *ptr, const char *name, const char *unit,
                                   int convtype, const char *conversion,
                                   double min, double max,
                                   int width, int prec,
                                   unsigned int rgb_color,
                                   int steptype, double step)
{
    reference_byte_var_all_infos_flags (ptr, name, unit,
                                        convtype, conversion,
                                        min, max,
                                        width, prec,
                                        rgb_color,
                                        steptype, step,
                                        READ_WRITE_REFERENCE);
}

// ubyte
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_ubyte_var_all_infos_flags (unsigned char *ptr, const char *name, const char *unit,
                                          int convtype, const char *conversion,
                                          double min, double max,
                                          int width, int prec,
                                          unsigned int rgb_color,
                                          int steptype, double step,
                                          unsigned int flags)
{
    int vid;
    int TypeBB;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_UBYTE, ptr, (char*)name, (char*)unit, convtype, (char*)conversion, min, max, width, prec, rgb_color, steptype, step,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_UBYTE)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.ub = *ptr;
                else Value.ub = 0;
                if ((vid = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo, name, BB_UBYTE + 0x1000, unit,   // + 0x1000 -> all Infos
                                                                              (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST),
                                                                              convtype, (char*)conversion,
                                                                              min, max,
                                                                              width, prec,
                                                                              rgb_color,
                                                                              steptype, step, 
                                                                              ValueIsValid, Value,
                                                                              TRUE, (unsigned long long)ptr,
                                                                              UniqueId,
                                                                              &TypeBB)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, TypeBB, BB_UBYTE, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_ubyte_var_all_infos (unsigned char *ptr, const char *name, const char *unit,
                                    int convtype, const char *conversion,
                                    double min, double max,
                                    int width, int prec,
                                    unsigned int rgb_color,
                                    int steptype, double step)
{
    reference_ubyte_var_all_infos_flags (ptr, name, unit,
                                         convtype, conversion,
                                         min, max,
                                         width, prec,
                                         rgb_color,
                                         steptype, step,
                                         READ_WRITE_REFERENCE);
}


// unbekannter Datentyp (Daten-Typ aus aus Debug-Infos lesen)
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_xxx_var_all_infos_flags (void *ptr, const char *name, const char *unit,
                                        int convtype, const char *conversion,
                                        double min, double max,
                                        int width, int prec,
                                        unsigned int rgb_color,
                                        int steptype, double step,
                                        unsigned int flags)
{
    int vid;
    int RealType;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    union BB_VARI Value;
    int ValueIsValid;
    NO_NAME_USE_PTR

    if (!CStartupFinishedFlag) {   // If this is called from c startup store this till a connection is established
        AddReferenceToBlackboardAtEndOfFunction (BB_GET_DATA_TYPE_FOM_DEBUGINFO, ptr, (char*)name, (char*)unit, convtype, (char*)conversion, min, max, width, prec, rgb_color, steptype, step,
                                                (flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST);
    } else {
        TaskInfo = XilEnvInternal_GetTaskPtr ();
        if (XilEnvInternal_CheckIfAddressIsValid(TaskInfo, name, ptr)) {
            if (!XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists(TaskInfo, name, (unsigned long long)ptr, BB_GET_DATA_TYPE_FOM_DEBUGINFO)) {
                UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
                ValueIsValid = (ptr != NULL) && ((flags & USE_BLACKBOARD_VALUE_AS_INIT) != USE_BLACKBOARD_VALUE_AS_INIT);
                if (ValueIsValid) Value.uqw = *(uint64_t*)ptr;  // copy always 8 byte 
                else Value.uqw = 0;
                if ((vid = XilEnvInternal_PipeAddBlackboardVariableAllInfos (TaskInfo, name, BB_GET_DATA_TYPE_FOM_DEBUGINFO + 0x1000, unit,   // + 0x1000 -> all Infos
                                                                              (int)((flags & 0xFFFF) | PIPE_API_REFERENCE_VARIABLE_DIR_ADD_COPY_LIST),
                                                                              convtype, (char*)conversion,
                                                                              min, max,
                                                                              width, prec,
                                                                              rgb_color,
                                                                              steptype, step, 
                                                                              ValueIsValid, Value,
                                                                              TRUE, (unsigned long long)ptr,
                                                                              UniqueId,
                                                                              &RealType)) <= 0) {
                    if (((vid == UNKNOWN_VARIABLE) && ((flags & NO_ERROR_IF_NOT_EXITING) == NO_ERROR_IF_NOT_EXITING)) ||
                        (vid == VARIABLE_REF_LIST_FILTERED)) {
                        // no error message
                    } else {
                        XilEnvInternal_referece_error (TaskInfo, name, vid, ptr);
                    }
                } else {
                    XilEnvInternal_InsertVariableToCopyLists (TaskInfo, name, vid, (void*)ptr, RealType, RealType, (int)flags, UniqueId);
                }
            }
        }
        XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    }
}

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_xxx_var_all_infos (void *ptr, const char *name, const char *unit,
                                  int convtype, const char *conversion,
                                  double min, double max,
                                  int width, int prec,
                                  unsigned int rgb_color,
                                  int steptype, double step)
{
    reference_xxx_var_all_infos_flags (ptr, name, unit,
                                       convtype, conversion,
                                       min, max,
                                       width, prec,
                                       rgb_color,
                                       steptype, step,
                                       READ_WRITE_REFERENCE);
}


EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ dereference_var (void *ptr, const char *name)
{
    int Dir, DataType;
    unsigned long long UniqueId;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    int Ret;

    TaskInfo = XilEnvInternal_GetTaskPtr ();
    Ret = XilEnvInternal_RemoveVariableFromCopyLists (TaskInfo, name, -1, ptr, &Dir, &DataType, NULL, NULL, &UniqueId);
    if (Ret < 0) {
        ThrowError (1, "dereference unknown variable \"%s\" address = %p", name, ptr);
    } else if (Ret > 0) {
        Ret = XilEnvInternal_PipeRemoveBlackboardVariable (TaskInfo, Ret, Dir, DataType, (unsigned long long)ptr, UniqueId);
    }
    XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    return Ret;
}

int XilEnvInternal_DereferenceVariable (void *ptr, int vid)
{
    int Dir, DataType;
    unsigned long long UniqueId;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    int Ret;

    TaskInfo = XilEnvInternal_GetTaskPtr ();
    Ret = XilEnvInternal_RemoveVariableFromCopyLists (TaskInfo, NULL, vid, ptr, &Dir, &DataType, NULL, NULL, &UniqueId);
    if (Ret < 0) {
        ThrowError (1, "dereference unknown variable vid = %li address = %p", vid, ptr);
    } else if (Ret > 0) {
        Ret = XilEnvInternal_PipeRemoveBlackboardVariable (TaskInfo, vid, Dir, DataType, (unsigned long long)ptr, UniqueId);
    }
    XilEnvInternal_ReleaseTaskPtr(TaskInfo);
    return Ret;
}

int XilEnvInternal_ReferenceVariable (void *Address, char *Name, int Type, int Dir)
{
    union BB_VARI Value;
    int RealType;
    VID Vid;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo;
    unsigned long long UniqueId;
    int Ret = 0;

    switch (Type) {
    case BB_BYTE:
        Value.b = *(signed char*)Address;
        break;
    case BB_UBYTE:
        Value.ub = *(unsigned char*)Address;
        break;
    case BB_WORD:
        Value.w = *(signed short*)Address;
        break;
    case BB_UWORD:
        Value.uw = *(unsigned short*)Address;
        break;
    case BB_DWORD:
        Value.dw = *(signed int*)Address;
        break;
    case BB_UDWORD:
        Value.udw = *(unsigned int*)Address;
        break;
    case BB_QWORD:
        Value.qw = *(signed long long*)Address;
        break;
    case BB_UQWORD:
        Value.uqw = *(unsigned long long*)Address;
        break;
    case BB_FLOAT:
        Value.f = *(float*)Address;
        break;
    case BB_DOUBLE:
        Value.d = *(double*)Address;
        break;
    default:
        ThrowError (1, "internal error unknown data type %i", Type);
        Value.d = 0.0;
        break;
    }    

    TaskInfo = XilEnvInternal_GetTaskPtr ();
    UniqueId = XilEnvInternal_BuildRefUniqueId (TaskInfo);
    Vid = XilEnvInternal_add_bbvari_dir (TaskInfo, Name, Type, NULL, Dir, 
                                          TRUE, Value, TRUE, (unsigned long long)Address, 
                                          UniqueId,
                                          &RealType);
    if (Vid <= 0) {
        if (Vid == VARIABLE_REF_LIST_FILTERED) {
            // no error message
        } else {
            XilEnvInternal_referece_error (TaskInfo, Name, Vid, Address);
        }
        Ret = Vid;
    } else {
        XilEnvInternal_InsertVariableToCopyLists (TaskInfo, Name, Vid, Address, Type, Type, Dir ,UniqueId);
    }
    XilEnvInternal_ReleaseTaskPtr(TaskInfo);

    return Ret;
}
