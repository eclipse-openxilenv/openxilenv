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
#include <malloc.h>
#include <stdarg.h>

#include "XilEnvExtProc.h"

#ifndef _WIN32
#define __stdcall
#endif

#if (XILENV_INTERFACE_TYPE != XILENV_LIBRARY_INTERFACE_TYPE)
    #define GET_FCT_PTR_RET(i, n, r) \
    XilEnvFunctionCallPointer Ptr = GetXilEnvFunctionCallPointer(i, n); \
    if (Ptr == NULL) return r;
    #define GET_FCT_PTR_VOID(i, n) \
    XilEnvFunctionCallPointer Ptr = GetXilEnvFunctionCallPointer(i, n); \
    if (Ptr == NULL) return;

    typedef void (*XilEnvFunctionCallPointer)(void *);
    XilEnvFunctionCallPointer GetXilEnvFunctionCallPointer(int par_FunctionNr, const char *par_FunctionName);
#endif

#define EXPORT_OR_IMPORT_OR_INLINE

EXPORT_OR_IMPORT_OR_INLINE int ExternalProcessMain (int par_GuiOrConsole, EXTERN_PROCESS_TASKS_LIST *par_ExternProcessTasksList, int par_ExternProcessTasksListElementCount)
{ 
    GET_FCT_PTR_RET(0x0, "ExternalProcessMain", 0) return ((int (__stdcall *) (int, EXTERN_PROCESS_TASKS_LIST *, int))Ptr)(par_GuiOrConsole, par_ExternProcessTasksList, par_ExternProcessTasksListElementCount);
}

EXPORT_OR_IMPORT_OR_INLINE int CheckIfConnectedTo (EXTERN_PROCESS_TASKS_LIST *ExternProcessTasksList, int ExternProcessTasksListElementCount)
{
    GET_FCT_PTR_RET(0x1, "CheckIfConnectedTo", 0) return ((int (__stdcall *) (EXTERN_PROCESS_TASKS_LIST *, int))Ptr)(ExternProcessTasksList, ExternProcessTasksListElementCount);
}

EXPORT_OR_IMPORT_OR_INLINE int CheckIfConnectedToEx (EXTERN_PROCESS_TASKS_LIST *ExternProcessTasksList, int ExternProcessTasksListElementCount,
                                                 EXTERN_PROCESS_TASK_HANDLE *ret_ProcessHandle)
{
    GET_FCT_PTR_RET(0x2, "CheckIfConnectedToEx", 0) return ((int (__stdcall *) (EXTERN_PROCESS_TASKS_LIST *, int, EXTERN_PROCESS_TASK_HANDLE *))Ptr)(ExternProcessTasksList, ExternProcessTasksListElementCount, ret_ProcessHandle);
}

EXPORT_OR_IMPORT_OR_INLINE void EnterAndExecuteOneProcessLoop (EXTERN_PROCESS_TASK_HANDLE ProcessHandle)
{ 
    GET_FCT_PTR_VOID(0x3, "EnterAndExecuteOneProcessLoop") ((void (__stdcall *) (EXTERN_PROCESS_TASK_HANDLE))Ptr)(ProcessHandle); 
}

EXPORT_OR_IMPORT_OR_INLINE int LoopOutAndWaitForBarriers (const char *par_BarrierBeforeName, const char *par_BarrierBehindName)
{ 
    GET_FCT_PTR_RET(0x4, "LoopOutAndWaitForBarriers", 0) return ((int (__stdcall *) (const char *, const char *))Ptr)(par_BarrierBeforeName, par_BarrierBehindName); 
}

EXPORT_OR_IMPORT_OR_INLINE void DisconnectFrom (EXTERN_PROCESS_TASK_HANDLE Process, int ImmediatelyFlag)
{
    GET_FCT_PTR_VOID(0x5, "DisconnectFrom") ((void (__stdcall *) (EXTERN_PROCESS_TASK_HANDLE, int))Ptr)(Process, ImmediatelyFlag);
}

EXPORT_OR_IMPORT_OR_INLINE EXTERN_PROCESS_TASK_HANDLE BuildNewProcessAndConnectTo (const char *ProcessName, const char *InstanceName, const char *ProcessNamePostfix, const char *ExecutableName, int Timeout_ms)
{
    GET_FCT_PTR_RET(0x6, "BuildNewProcessAndConnectTo", 0) return ((EXTERN_PROCESS_TASK_HANDLE (__stdcall *) (const char *, const char *, const char *, const char *, int))Ptr)(ProcessName, InstanceName, ProcessNamePostfix, ExecutableName, Timeout_ms);
}

EXPORT_OR_IMPORT_OR_INLINE EXTERN_PROCESS_TASK_HANDLE BuildNewProcessAndConnectToEx (const char *ProcessName, const char *InstanceName, const char *ServerName, const char *ProcessNamePostfix, const char *ExecutableName, const char *DllName,
                                                                                     int Priority, int CycleDivider, int Delay, int LoginTimeout,
                                                                                     void (*reference) (void), int (*init) (void), void (*cyclic) (void), void (*terminate) (void))
{ 
    GET_FCT_PTR_RET(0x7, "BuildNewProcessAndConnectToEx", 0) return ((EXTERN_PROCESS_TASK_HANDLE (__stdcall *) (const char *, const char *, const char *, const char *, const char *, const char *,
                                                                                                                int, int, int, int,
                                                                                                                void (*) (void), int (*) (void), void (*) (void), void (*) (void)))Ptr)
                                                                                                                (ProcessName, InstanceName, ServerName, ProcessNamePostfix, ExecutableName, DllName,
                                                                                                                 Priority, CycleDivider, Delay, LoginTimeout,
                                                                                                                reference, init, cyclic, terminate);
}

EXPORT_OR_IMPORT_OR_INLINE int WaitUntilFunctionCallRequest (EXTERN_PROCESS_TASK_HANDLE Process)
{
    GET_FCT_PTR_RET(0x8, "WaitUntilFunctionCallRequest", 0) return ((int (__stdcall *) (EXTERN_PROCESS_TASK_HANDLE))Ptr)(Process);
}

EXPORT_OR_IMPORT_OR_INLINE int AcknowledgmentOfFunctionCallRequest (EXTERN_PROCESS_TASK_HANDLE Process, int Ret)
{
    GET_FCT_PTR_RET(0x9, "AcknowledgmentOfFunctionCallRequest", 0) return ((int (__stdcall *) (EXTERN_PROCESS_TASK_HANDLE, int))Ptr)(Process, Ret);
}

EXPORT_OR_IMPORT_OR_INLINE void SetHwndMainWindow (void *Hwnd)
{
    GET_FCT_PTR_VOID(0xA, "SetHwndMainWindow") ((void (__stdcall *) (void *))Ptr)(Hwnd);
}

EXPORT_OR_IMPORT_OR_INLINE void SetExternalProcessExecutableName(const char *par_ExecutableName)
{
    GET_FCT_PTR_VOID(0xB, "SetExternalProcessExecutableName") ((void (__stdcall *) (const char *))Ptr)(par_ExecutableName);
}


EXPORT_OR_IMPORT_OR_INLINE int ThrowError (int level, char const *format, ...)
{ 
    va_list args;
    int ret;
    XilEnvFunctionCallPointer Ptr = GetXilEnvFunctionCallPointer(0x13, "ThrowErrorVariableArguments");
    if (Ptr == NULL) return 0;
    va_start(args, format);
    ret = ((int (__stdcall *) (int, char const *, va_list))Ptr)(level, format, args);
    va_end(args); 
    return ret;
}

EXPORT_OR_IMPORT_OR_INLINE int ThrowErrorNoVariableArguments(int level, char const *txt)
{
    GET_FCT_PTR_RET(0x14, "ThrowErrorNoVariableArguments", 0) return ((int (__stdcall *) (int, char const *))Ptr)(level, txt);
}

EXPORT_OR_IMPORT_OR_INLINE int ThrowErrorVariableArguments(int level, char const *format, va_list argptr)
{
    GET_FCT_PTR_RET(0x13, "ThrowErrorVariableArguments", 0) return ((int (__stdcall *) (int, char const *, va_list))Ptr)(level, format, argptr);
}

EXPORT_OR_IMPORT_OR_INLINE int GetSchedulingInformation (int *ret_SeparateCyclesForRefAndInitFunction,
                                                        unsigned long long *ret_SchedulerCycleCounter,
                                                        long long *ret_SchedulerPeriod,
                                                        unsigned long long *ret_MainSchedulerCycleCounter,
                                                        unsigned long long *ret_ProcessCycleCounter,
                                                        int *ret_ProcessPerid,
                                                        int *ret_ProcessDelay)
{
    GET_FCT_PTR_RET(0x15, "GetSchedulingInformation", 0) return ((int (__stdcall *) (int*,
                              unsigned long long*,
                              long long*,
                              unsigned long long*,
                              unsigned long long*,
                              int*,
                              int*))Ptr)(ret_SeparateCyclesForRefAndInitFunction,
                                           ret_SchedulerCycleCounter,
                                           ret_SchedulerPeriod,
                                           ret_MainSchedulerCycleCounter,
                                           ret_ProcessCycleCounter,
                                           ret_ProcessPerid,
                                           ret_ProcessDelay);
}

EXPORT_OR_IMPORT_OR_INLINE unsigned long long get_scheduler_cycle_count (void)
{
    GET_FCT_PTR_RET(0x16, "get_scheduler_cycle_count", 0) return ((unsigned long long (__stdcall *) (void))Ptr)();
}

EXPORT_OR_IMPORT_OR_INLINE int get_cycle_period (void)
{
    GET_FCT_PTR_RET(0x17, "get_cycle_period", 0) return ((int (__stdcall *)(void))Ptr)();
}

EXPORT_OR_IMPORT_OR_INLINE const char* get_exe_path (void)
{
    GET_FCT_PTR_RET(0x18, "get_exe_path", "get_exe_path") return ((const char* (__stdcall *) (void))Ptr)();
}

EXPORT_OR_IMPORT_OR_INLINE double __GetNotANumber (void)
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

#define BUILD_REFERENCE_VAR_FUNC_DEF(fnc_nr, dtype, cdtype) \
    EXPORT_OR_IMPORT_OR_INLINE void reference_##dtype##_var (cdtype *ptr, const char *name) \
    { GET_FCT_PTR_VOID(fnc_nr, "reference_" #dtype "_var") ((void (__stdcall *) (cdtype*, const char*))Ptr)(ptr, name); } \
    EXPORT_OR_IMPORT_OR_INLINE void reference_##dtype##_var_unit (cdtype *ptr, const char *name, const char *unit) \
    { GET_FCT_PTR_VOID(fnc_nr+0x10, "reference_" #dtype "_var_unit") ((void (__stdcall *) (cdtype*, const char*, const char*))Ptr)(ptr, name, unit); } \
    EXPORT_OR_IMPORT_OR_INLINE void reference_##dtype##_var_flags (cdtype *ptr, const char *name, const char *unit, unsigned int flags) \
    { GET_FCT_PTR_VOID(fnc_nr+0x20, "reference_" #dtype "_var_flags") ((void (__stdcall *) (cdtype*, const char*, const char*, unsigned int))Ptr)(ptr, name, unit, flags); } \
    EXPORT_OR_IMPORT_OR_INLINE void reference_##dtype##_var_all_infos (cdtype *ptr, const char *name, const char *unit, \
                                                   int convtype, const char *conversion, \
                                                   double min, double max, \
                                                   int width, int prec, \
                                                   unsigned int rgb_color, \
                                                   int steptype, double step) \
    { GET_FCT_PTR_VOID(fnc_nr+0x30, "reference_" #dtype "_var_all_infos") ((void (__stdcall *) (cdtype*, const char*, const char*, int, const char *, double, double, int, int, unsigned int, int, double)) \
          Ptr)(ptr, name, unit, \
                                                   convtype, conversion, \
                                                   min, max, \
                                                   width, prec, \
                                                   rgb_color, \
                                                   steptype, step); } \
    EXPORT_OR_IMPORT_OR_INLINE void reference_##dtype##_var_all_infos_flags (cdtype *ptr, const char *name, const char *unit, \
                                                   int convtype, const char *conversion, \
                                                   double min, double max, \
                                                   int width, int prec, \
                                                   unsigned int rgb_color, \
                                                   int steptype, double step, \
                                                   unsigned int flags) \
    { GET_FCT_PTR_VOID(fnc_nr+0x40, "reference_" #dtype "_var_all_infos_flags") ((void (__stdcall *) (cdtype*, const char*, const char*, int, const char *, double, double, int, int, unsigned int, int, double, unsigned int)) \
          Ptr)(ptr, name, unit, \
                                                   convtype, conversion, \
                                                   min, max, \
                                                   width, prec, \
                                                   rgb_color, \
                                                   steptype, step, \
                                                   flags); }


BUILD_REFERENCE_VAR_FUNC_DEF(0x100, double, double)
BUILD_REFERENCE_VAR_FUNC_DEF(0x101, float, float)
BUILD_REFERENCE_VAR_FUNC_DEF(0x102, qword, long long)
BUILD_REFERENCE_VAR_FUNC_DEF(0x103, uqword, unsigned long long)
BUILD_REFERENCE_VAR_FUNC_DEF(0x104, dword, WSC_TYPE_INT32)
BUILD_REFERENCE_VAR_FUNC_DEF(0x105, udword, WSC_TYPE_UINT32)
BUILD_REFERENCE_VAR_FUNC_DEF(0x106, word, short)
BUILD_REFERENCE_VAR_FUNC_DEF(0x107, uword, unsigned short)
BUILD_REFERENCE_VAR_FUNC_DEF(0x108, byte, signed char)
BUILD_REFERENCE_VAR_FUNC_DEF(0x109, ubyte, unsigned char)
BUILD_REFERENCE_VAR_FUNC_DEF(0x10A, xxx, void)

EXPORT_OR_IMPORT_OR_INLINE VID get_vid_by_reference_address (void *ptr)
{
    GET_FCT_PTR_RET(0x19, "get_vid_by_reference_address", -1) return ((int (__stdcall *) (void *))Ptr) (ptr);
}

EXPORT_OR_IMPORT_OR_INLINE enum SC_GET_LABELNAME_RETURN_CODE get_labelname_by_address (void *address, char *labelname, int maxc)
{
    GET_FCT_PTR_RET(0x1A, "get_labelname_by_address", SC_INVALID_ADDRESS) return ((enum SC_GET_LABELNAME_RETURN_CODE (__stdcall *) (void *, char *, int))Ptr)(address, labelname, maxc);
}

EXPORT_OR_IMPORT_OR_INLINE enum SC_GET_LABELNAME_RETURN_CODE get_not_referenced_labelname_by_address (void *Addr, char *pName, int maxc)
{
    GET_FCT_PTR_RET(0x1B, "get_not_referenced_labelname_by_address", SC_INVALID_ADDRESS) return ((enum SC_GET_LABELNAME_RETURN_CODE (__stdcall *) (void *, char *, int))Ptr)(Addr, pName, maxc);
}

EXPORT_OR_IMPORT_OR_INLINE int get_section_addresse_size (const char *par_section_name, unsigned long long *ret_start_address, unsigned long *ret_size)
{
    GET_FCT_PTR_RET(0x1C, "get_section_addresse_size", -1) return ((int (__stdcall *) (const char *, unsigned long long *, unsigned long *))Ptr)(par_section_name, ret_start_address, ret_size);
}

EXPORT_OR_IMPORT_OR_INLINE int get_next_referenced_variable (int Index, VID *pVid, char *Name)
{
    GET_FCT_PTR_RET(0x1D, "get_next_referenced_variable", -1) return ((int (__stdcall *) (int, int *, char *))Ptr)(Index, pVid, Name);
}

EXPORT_OR_IMPORT_OR_INLINE VID add_bbvari (const char *name, int type, const char *unit)
{
    GET_FCT_PTR_RET(0x20, "add_bbvari", -1) return ((VID (__stdcall *) (const char *, int, const char *))Ptr)(name, type, unit);
}

EXPORT_OR_IMPORT_OR_INLINE VID attach_bbvari (const char *name)
{
    GET_FCT_PTR_RET(0x21, "attach_bbvari", -1) return ((VID (__stdcall *) (const char *))Ptr)(name);
}

EXPORT_OR_IMPORT_OR_INLINE int remove_bbvari (VID vid)
{
    GET_FCT_PTR_RET(0x22, "remove_bbvari", -1) return ((int (__stdcall *) (VID))Ptr)(vid);
}

EXPORT_OR_IMPORT_OR_INLINE void write_bbvari_byte (VID vid, signed char v)
{
    GET_FCT_PTR_VOID(0x23, "write_bbvari_byte") ((void (__stdcall *) (VID, signed char))Ptr)(vid, v);
}
EXPORT_OR_IMPORT_OR_INLINE void write_bbvari_ubyte (VID vid, unsigned char v)
{
    GET_FCT_PTR_VOID(0x24, "write_bbvari_ubyte") ((void (__stdcall *) (VID, unsigned char))Ptr)(vid, v);
}
EXPORT_OR_IMPORT_OR_INLINE void write_bbvari_word (VID vid, short v)
{
    GET_FCT_PTR_VOID(0x25, "write_bbvari_word") ((void (__stdcall *) (VID, short))Ptr)(vid, v);
}
EXPORT_OR_IMPORT_OR_INLINE void write_bbvari_uword (VID vid, unsigned short v)
{
    GET_FCT_PTR_VOID(0x26, "write_bbvari_uword") ((void (__stdcall *) (VID, unsigned short))Ptr)(vid, v);
}
EXPORT_OR_IMPORT_OR_INLINE void write_bbvari_dword (VID vid, WSC_TYPE_INT32 v)
{
    GET_FCT_PTR_VOID(0x27, "write_bbvari_dword") ((void (__stdcall *) (VID, int))Ptr)(vid, v);
}
EXPORT_OR_IMPORT_OR_INLINE void write_bbvari_udword (VID vid, WSC_TYPE_UINT32 v)
{
    GET_FCT_PTR_VOID(0x28, "write_bbvari_udword") ((void (__stdcall *) (VID, unsigned int))Ptr)(vid, v);
}
EXPORT_OR_IMPORT_OR_INLINE void write_bbvari_qword (VID vid, long long v)
{
    GET_FCT_PTR_VOID(0x29, "write_bbvari_qword") ((void (__stdcall *) (VID, long long))Ptr)(vid, v);
}
EXPORT_OR_IMPORT_OR_INLINE void write_bbvari_uqword (VID vid, unsigned long long v)
{
    GET_FCT_PTR_VOID(0x2A, "write_bbvari_uqword") ((void (__stdcall *) (VID, unsigned long long))Ptr)(vid, v);
}
EXPORT_OR_IMPORT_OR_INLINE void write_bbvari_float (VID vid, float v)
{
    GET_FCT_PTR_VOID(0x2B, "write_bbvari_float") ((void (__stdcall *) (VID, float))Ptr)(vid, v);
}
EXPORT_OR_IMPORT_OR_INLINE void write_bbvari_double (VID vid, double v)
{
    GET_FCT_PTR_VOID(0x2C, "write_bbvari_double") ((void (__stdcall *) (VID, double))Ptr)(vid, v);
}
EXPORT_OR_IMPORT_OR_INLINE void write_bbvari_minmax_check (VID vid, double v)
{
    GET_FCT_PTR_VOID(0x2D, "write_bbvari_minmax_check") ((void (__stdcall *) (VID, double))Ptr)(vid, v);
}

EXPORT_OR_IMPORT_OR_INLINE signed char read_bbvari_byte (VID vid)
{
    GET_FCT_PTR_RET(0x2E, "read_bbvari_byte", 0) return ((signed char (__stdcall *) (VID))Ptr)(vid);
}
EXPORT_OR_IMPORT_OR_INLINE unsigned char read_bbvari_ubyte (VID vid)
{
    GET_FCT_PTR_RET(0x2F, "read_bbvari_ubyte", 0) return ((unsigned char (__stdcall *) (VID))Ptr)(vid);
}
EXPORT_OR_IMPORT_OR_INLINE short read_bbvari_word (VID vid)
{
    GET_FCT_PTR_RET(0x30, "read_bbvari_word", 0) return ((short (__stdcall *) (VID))Ptr)(vid);
}
EXPORT_OR_IMPORT_OR_INLINE unsigned short read_bbvari_uword (VID vid)
{
    GET_FCT_PTR_RET(0x31, "read_bbvari_uword", 0) return ((unsigned short (__stdcall *) (VID))Ptr)(vid);
}
EXPORT_OR_IMPORT_OR_INLINE WSC_TYPE_INT32 read_bbvari_dword (VID vid)
{
    GET_FCT_PTR_RET(0x32, "read_bbvari_dword", 0) return ((WSC_TYPE_INT32 (__stdcall *) (VID))Ptr)(vid);
}
EXPORT_OR_IMPORT_OR_INLINE WSC_TYPE_UINT32 read_bbvari_udword (VID vid)
{
    GET_FCT_PTR_RET(0x33, "read_bbvari_udword", 0) return ((WSC_TYPE_UINT32 (__stdcall *) (VID))Ptr)(vid);
}
EXPORT_OR_IMPORT_OR_INLINE long long read_bbvari_qword (VID vid)
{
    GET_FCT_PTR_RET(0x34, "read_bbvari_qword", 0) return ((long long (__stdcall *) (VID))Ptr)(vid);
}
EXPORT_OR_IMPORT_OR_INLINE unsigned long long read_bbvari_uqword (VID vid)
{
    GET_FCT_PTR_RET(0x35, "read_bbvari_uqword", 0) return ((unsigned long long (__stdcall *) (VID))Ptr)(vid);
}
EXPORT_OR_IMPORT_OR_INLINE float read_bbvari_float (VID vid)
{
    GET_FCT_PTR_RET(0x36, "read_bbvari_float", 0) return ((float (__stdcall *) (VID))Ptr)(vid);
}
EXPORT_OR_IMPORT_OR_INLINE double read_bbvari_double (VID vid)
{
    GET_FCT_PTR_RET(0x37, "read_bbvari_double", 0) return ((double (__stdcall *) (VID))Ptr)(vid);
}
EXPORT_OR_IMPORT_OR_INLINE double read_bbvari_convert_double (VID vid)
{
    GET_FCT_PTR_RET(0x38, "read_bbvari_convert_double", 0) return ((double (__stdcall *) (VID))Ptr)(vid);
}

EXPORT_OR_IMPORT_OR_INLINE int init_message_output_stream (const char *para_MessageFilename, int para_Flags, int para_MaxErrorMessageCounter,
                                       const char *para_HeaderText)
{
    GET_FCT_PTR_RET(0x40, "init_message_output_stream", 0) return ((int (__stdcall *) (const char *, int, int, const char *))Ptr)(para_MessageFilename, para_Flags, para_MaxErrorMessageCounter, para_HeaderText);
}

EXPORT_OR_IMPORT_OR_INLINE void message_output_stream (const char *format, ...)
{
    va_list args;
    XilEnvFunctionCallPointer Ptr = GetXilEnvFunctionCallPointer(0x41, "message_output_stream_va_list");
    if (Ptr == NULL) return;
    va_start(args, format);
    ((void (__stdcall *) (char const *, va_list))Ptr)(format, args); 
    va_end(args);
}
EXPORT_OR_IMPORT_OR_INLINE void message_output_stream_no_variable_arguments (const char *string)
{
    GET_FCT_PTR_VOID(0x42, "message_output_stream_no_variable_arguments") ((void (__stdcall *) (char const *))Ptr)(string);
}
EXPORT_OR_IMPORT_OR_INLINE void message_output_stream_va_list (const char *format, va_list argptr)
{
    GET_FCT_PTR_VOID(0x43, "message_output_stream_va_list") ((void (__stdcall *) (char const *, va_list))Ptr)(format, argptr);
}

EXPORT_OR_IMPORT_OR_INLINE int dereference_var (void *ptr, const char *name)
{
    GET_FCT_PTR_RET(0x44, "dereference_var", 0) return ((int (__stdcall *) (void *, const char *))Ptr)(ptr, name);
}

// CAN 
#include "XilEnvExtProcCan.h"

EXPORT_OR_IMPORT_OR_INLINE int sc_open_virt_can (int channel)    
{
    GET_FCT_PTR_RET(0x240, "sc_open_virt_can", -1) return ((int (__stdcall *) (int))Ptr)(channel);
}

EXPORT_OR_IMPORT_OR_INLINE int sc_open_virt_can_fd (int channel, int fd_enable)    
{
    GET_FCT_PTR_RET(0x241, "sc_open_virt_can_fd", -1) return ((int (__stdcall *) (int, int))Ptr)(channel, fd_enable);
}
EXPORT_OR_IMPORT_OR_INLINE int sc_close_virt_can (int channel)    
{
    GET_FCT_PTR_RET(0x242, "sc_close_virt_can", -1) return ((int (__stdcall *) (int))Ptr)(channel);
}
EXPORT_OR_IMPORT_OR_INLINE int sc_fifo_write_virt_can (int channel, WSC_TYPE_UINT32 id, unsigned char ext, unsigned char size, unsigned char *data)
{
    GET_FCT_PTR_RET(0x243, "sc_fifo_write_virt_can", -1) return ((int (__stdcall *) (int, WSC_TYPE_UINT32, unsigned char, unsigned char, unsigned char *))Ptr)(channel, id, ext, size, data);
}
EXPORT_OR_IMPORT_OR_INLINE int sc_fifo_read_virt_can (int channel, WSC_TYPE_UINT32 *pid, unsigned char *pext, unsigned char *psize, unsigned char *data)
{
    GET_FCT_PTR_RET(0x244, "sc_fifo_read_virt_can", -1) return ((int (__stdcall *) (int, WSC_TYPE_UINT32*, unsigned char*, unsigned char*, unsigned char*))Ptr)(channel, pid, pext, psize, data);
}
EXPORT_OR_IMPORT_OR_INLINE int sc_get_fifo_fill_level (int channel)    
{
    GET_FCT_PTR_RET(0x245, "sc_get_fifo_fill_level", -1) return ((int (__stdcall *) (int))Ptr)(channel);
}
EXPORT_OR_IMPORT_OR_INLINE int sc_cfg_virt_can_msg_buff (int channel, int buffer_idx, WSC_TYPE_UINT32 id, unsigned char ext, unsigned char size, unsigned char dir)
{
    GET_FCT_PTR_RET(0x246, "sc_cfg_virt_can_msg_buff", -1) return ((int (__stdcall *) (int, int, WSC_TYPE_UINT32, unsigned char, unsigned char, unsigned char))Ptr)(channel, buffer_idx, id, ext, size, dir);
}
EXPORT_OR_IMPORT_OR_INLINE int sc_read_virt_can_msg_buff (int channel, int buffer_idx, unsigned char *pext, unsigned char *psize, unsigned char *data)
{
    GET_FCT_PTR_RET(0x247, "sc_read_virt_can_msg_buff", -1) return ((int (__stdcall *) (int, int, unsigned char*, unsigned char*, unsigned char*))Ptr)(channel, buffer_idx, pext, psize, data);
}
EXPORT_OR_IMPORT_OR_INLINE int sc_peek_virt_can_msg_buff (int channel, int buffer_idx, unsigned char *pext, unsigned char *psize, unsigned char *data)
{
    GET_FCT_PTR_RET(0x248, "sc_peek_virt_can_msg_buff", -1) return ((int (__stdcall *) (int, int, unsigned char*, unsigned char*, unsigned char*))Ptr)(channel, buffer_idx, pext, psize, data);
}
EXPORT_OR_IMPORT_OR_INLINE int sc_write_virt_can_msg_buff (int channel, int buffer_idx, unsigned char *data)
{
    GET_FCT_PTR_RET(0x249, "sc_write_virt_can_msg_buff", -1) return ((int (__stdcall *) (int, int, unsigned char*))Ptr)(channel, buffer_idx, data);
}







