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


#ifndef XILENVEXTPROC_H
#define XILENVEXTPROC_H

#define XILENV_VERSION              0
#define XILENV_MINOR_VERSION        8
#define XILENV_PATCH_VERSION       16

#define SC_MAX_LABEL_SIZE         512
#define SC_MAX_UNIT_SIZE           64
#define SC_MAX_CONV_SIZE         1376
#define SC_MAX_CONV_EXT_SIZE   500000
#define SC_MAX_TEXTREPL_SIZE       64

#define XILENV_LIBRARY_INTERFACE_TYPE   0
#define XILENV_DLL_INTERFACE_TYPE       1
#define XILENV_STUB_INTERFACE_TYPE      2
/* if no interface type are selected use the library (mybe better set to XILENV_DLL_INTERFACE_TYPE) */
#ifndef XILENV_INTERFACE_TYPE
#define XILENV_INTERFACE_TYPE  XILENV_LIBRARY_INTERFACE_TYPE
#endif

#ifndef WSC_TYPE_INT32
#if (defined(_WIN32) && !defined(__GNUC__))
#define WSC_TYPE_INT32   long
#define WSC_TYPE_UINT32  unsigned long
#else
#define WSC_TYPE_INT32   int
#define WSC_TYPE_UINT32  unsigned int
#endif
#endif

#ifndef __FUNC_CALL_CONVETION__
#ifdef _WIN32
    #ifdef EXTPROC_DLL_EXPORTS
        #define __FUNC_CALL_CONVETION__  __stdcall
        #define EXPORT_OR_IMPORT __declspec(dllexport)
    #else
        #if defined USE_DLL
            #define __FUNC_CALL_CONVETION__ __stdcall
            #define EXPORT_OR_IMPORT __declspec(dllimport)
        #else
            #define __FUNC_CALL_CONVETION__ 
            #define EXPORT_OR_IMPORT extern
        #endif
    #endif
#else
    #define __FUNC_CALL_CONVETION__
    #define EXPORT_OR_IMPORT extern
#endif
#endif

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif
/*lint
  -esym(621,BuildNewProcessAndConnectTo*)
*/

#define MAX_PROCESSES_INSIDE_ONE_EXECUTABLE 32

typedef struct {
    const char *TaskName;
    void (*reference) (void);         // Variable reference function
    int (*init) (void);               // Init function
    void (*cyclic) (void);            // Cyclic function
    void (*terminate) (void);         // Terminate function
    char *DllName;
} EXTERN_PROCESS_TASKS_LIST;


#define MESSAGE_STOP 1
#define MAX_ERROR_MESSAGE_SIZE 8192

typedef void* EXTERN_PROCESS_TASK_HANDLE;

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ ExternalProcessMain (int par_GuiOrConsole, EXTERN_PROCESS_TASKS_LIST *par_ExternProcessTasksList, int par_ExternProcessTasksListElementCount);

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ CheckIfConnectedTo (EXTERN_PROCESS_TASKS_LIST *ExternProcessTasksList, int ExternProcessTasksListElementCount);
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ CheckIfConnectedToEx (EXTERN_PROCESS_TASKS_LIST *ExternProcessTasksList, int ExternProcessTasksListElementCount,
                                                                          EXTERN_PROCESS_TASK_HANDLE *ret_ProcessHandle);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ EnterAndExecuteOneProcessLoop (EXTERN_PROCESS_TASK_HANDLE ProcessHandle);
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ LoopOutAndWaitForBarriers (const char *par_BarrierBeforeName, const char *par_BarrierBehindName);

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ DisconnectFrom (EXTERN_PROCESS_TASK_HANDLE Process, int ImmediatelyFlag);
EXPORT_OR_IMPORT EXTERN_PROCESS_TASK_HANDLE __FUNC_CALL_CONVETION__ BuildNewProcessAndConnectTo (const char *ProcessName,
                                                                                                 const char *InstanceName,
                                                                                                 const char *ProcessNamePostfix,
                                                                                                 const char *ExecutableName,
                                                                                                 int Timeout_ms);
EXPORT_OR_IMPORT EXTERN_PROCESS_TASK_HANDLE __FUNC_CALL_CONVETION__ BuildNewProcessAndConnectToEx (const char *ProcessName,
                                                                                                   const char *InstanceName,
                                                                                                   const char *ServerName,
                                                                                                   const char *ProcessNamePostfix,
                                                                                                   const char *ExecutableName,
                                                                                                   const char *DllName,
                                                                                                   int Priority,
                                                                                                   int CycleDivider,
                                                                                                   int Delay,
                                                                                                   int LoginTimeout,
                                                                                                   void (*reference) (void),
                                                                                                   int (*init) (void),
                                                                                                   void (*cyclic) (void),
                                                                                                   void (*terminate) (void));
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ WaitUntilFunctionCallRequest (EXTERN_PROCESS_TASK_HANDLE Process);
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ AcknowledgmentOfFunctionCallRequest (EXTERN_PROCESS_TASK_HANDLE Process, int Ret);

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ SetExternalProcessExecutableName(const char *par_ExecutableName);

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ ThrowError(int level, char const *txt, ...);
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ ThrowErrorNoVariableArguments(int level, char const *txt);
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ ThrowErrorVariableArguments (int level, const char *format, va_list argptr);
#ifdef XILENV_RT
#define exit(a) sc_exit(a)
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ sc_exit(int level);
#endif

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ GetSchedulingInformation (int *ret_SeparateCyclesForRefAndInitFunction,
                                                                       unsigned long long *ret_SchedulerCycleCounter,
                                                                       long long *ret_SchedulerPeriod,
                                                                       unsigned long long *ret_MainSchedulerCycleCounter,
                                                                       unsigned long long *ret_ProcessCycleCounter,
                                                                       int *ret_ProcessPerid,
                                                                       int *ret_ProcessDelay);

EXPORT_OR_IMPORT unsigned long long __FUNC_CALL_CONVETION__ get_scheduler_cycle_count(void);

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ get_cycle_period(void);

EXPORT_OR_IMPORT  const char* __FUNC_CALL_CONVETION__ get_exe_path(void);


// Some definitions for reference variables
// With this macros you can add variables which should be synchronize with the blackboard

#define READ_ONLY_REFERENCE                     1
#define WRITE_ONLY_REFERENCE                    2
#define READ_WRITE_REFERENCE                    3
#define ALLOW_SAME_ADDRESS_DIFFERNCE_NAMES      4 
#define ALLOW_SAME_NAME_DIFFERNCE_ADDRESSES     8 
#define DATA_TYPES_ONLY_SUGGESTED_CONVERT      16
#define DATA_TYPES_WAIT                        32
#define USE_BLACKBOARD_VALUE_AS_INIT           64
#define ONLY_ATTACH_EXITING                   128
#define NO_ERROR_IF_NOT_EXITING               256


// 3 different conversion types
// No conversion defined conversion = ""
#define CONVTYPE_NOTHING         0
// A Equation as conversion = "#*10.0"
#define CONVTYPE_EQUATION        1
// A text enum as conversion = "0 0 \"off\"; 1 1 \"on\";"
#define CONVTYPE_TEXTREPLACE     2
// The conversion should not be set
#define CONVTYPE_UNDEFINED     255

// The display format should not be changed
#define WIDTH_UNDEFINED        255
#define PREC_UNDEFINED         255
// The increment width and type should not be changed
#define STEPS_UNDEFINED        255

// This can be used to define the min. and/or max. to be unchanged (SC_NAN/FP_NAN)
EXPORT_OR_IMPORT double __FUNC_CALL_CONVETION__ __GetNotANumber (void);

#ifndef SC_NAN
  #define SC_NAN __GetNotANumber()
#endif
#ifndef __linux__
  #ifndef FP_NAN
    #define FP_NAN __GetNotANumber()
  #endif
#endif


// The color should be be unchanged
#define COLOR_UNDEFINED        0xFFFFFFFFUL
// Or use RGB(red,green,blue) red = 0...255, green = 0...255, blue = 0...255 */
// macro for building the RGB colors to use in the "AllInfos" ref.-macros
#define SC_RGB(r,g,b) \
    ( (unsigned int)(r) + (((unsigned int)(g))<<8) + (((unsigned int)(b))<<16) )


#define BUILD_REFERENCE_VAR_FUNC_DECL(dtype, cdtype) \
    EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_##dtype##_var (cdtype *ptr, const char *name); \
    EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_##dtype##_var_unit (cdtype *ptr, const char *name, const char *unit); \
    EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_##dtype##_var_flags (cdtype *ptr, const char *name, const char *unit, unsigned int flags); \
    EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_##dtype##_var_all_infos (cdtype *ptr, const char *name, const char *unit, \
                                                                 int convtype, const char *conversion, \
                                                                 double min, double max, \
                                                                 int width, int prec, \
                                                                 unsigned int rgb_color, \
                                                                 int steptype, double step); \
    EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ reference_##dtype##_var_all_infos_flags (cdtype *ptr, const char *name, const char *unit, \
                                                                   int convtype, const char *conversion, \
                                                                   double min, double max, \
                                                                   int width, int prec, \
                                                                   unsigned int rgb_color, \
                                                                   int steptype, double step, \
                                                                   unsigned int flags);

BUILD_REFERENCE_VAR_FUNC_DECL(double, double)
BUILD_REFERENCE_VAR_FUNC_DECL(float, float)
BUILD_REFERENCE_VAR_FUNC_DECL(qword, long long)
BUILD_REFERENCE_VAR_FUNC_DECL(uqword, unsigned long long)
BUILD_REFERENCE_VAR_FUNC_DECL(dword, WSC_TYPE_INT32)
BUILD_REFERENCE_VAR_FUNC_DECL(udword, WSC_TYPE_UINT32)
BUILD_REFERENCE_VAR_FUNC_DECL(word, short)
BUILD_REFERENCE_VAR_FUNC_DECL(uword, unsigned short)
BUILD_REFERENCE_VAR_FUNC_DECL(byte, signed char)
BUILD_REFERENCE_VAR_FUNC_DECL(ubyte, unsigned char)
BUILD_REFERENCE_VAR_FUNC_DECL(xxx, void)

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ dereference_var (void *ptr, const char *name);

typedef int VID;

EXPORT_OR_IMPORT VID __FUNC_CALL_CONVETION__ get_vid_by_reference_address (void *ptr);

enum SC_GET_LABELNAME_RETURN_CODE {
    SC_OK = 0,
    SC_INVALID_ADDRESS = -1,
    SC_VARIABLES_NOT_REFERENCED = -2,
    SC_VARIABLE_DOESNT_EXIST = -3,
    SC_VARIABLE_NOT_REFERENCED = -4,
    SC_VARIABLE_NAME_LONGER_THAN_MAXC = -5,
    SC_FUNC_ISNT_SUPPORTED_BY_THIS_VERSION = -6,
    SC_COUMMUNICATION_ERROR = -7
};

EXPORT_OR_IMPORT enum SC_GET_LABELNAME_RETURN_CODE __FUNC_CALL_CONVETION__ get_labelname_by_address (void *address, char *labelname, int maxc);
EXPORT_OR_IMPORT enum SC_GET_LABELNAME_RETURN_CODE __FUNC_CALL_CONVETION__ get_not_referenced_labelname_by_address (void *Addr, char *pName, int maxc);

// This will return the address and the size of an section inside the EXE file of an external Process
EXPORT_OR_IMPORT int  __FUNC_CALL_CONVETION__ get_section_addresse_size (const char *par_section_name, unsigned long long *ret_start_address, unsigned long *ret_size);

// This function will return the next referenced variable of this external process
// For the first call Index should be 0. The return value is the value for the next call.
// or -1 if no more references are available
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ get_next_referenced_variable (int Index, VID *pVid, char *Name);


// This function you can use if you have no symbol with an address to bable to reference to the blackboard
// to be automatically synchronize with the blackboard.
// you have to read and write with the read_bbvari_xxx() and write_bbvari_xxx().

enum SC_BB_DATA_TYPES {
    SC_BB_INVALID = -1, SC_BB_BYTE, SC_BB_UBYTE, SC_BB_WORD, SC_BB_UWORD, SC_BB_DWORD,
    SC_BB_UDWORD, SC_BB_FLOAT, SC_BB_DOUBLE, SC_BB_UNKNOWN,
    SC_BB_UNKNOWN_DOUBLE, SC_BB_UNKNOWN_WAIT, SC_BB_BITFIELD, SC_BB_GET_DATA_TYPE_FOM_DEBUGINFO,
    SC_BB_UNION, SC_BB_CONVERT_LIMIT_MIN_MAX, SC_BB_CONVERT_TO_DOUBLE, SC_BB_STRING, SC_BB_EXTERNAL_DATA,
    SC_BB_UNKNOWN_BYTE, SC_BB_UNKNOWN_UBYTE, SC_BB_UNKNOWN_WORD, SC_BB_UNKNOWN_UWORD, SC_BB_UNKNOWN_DWORD,
    SC_BB_UNKNOWN_UDWORD, SC_BB_UNKNOWN_FLOAT, SC_BB_UNKNOWN_DOUBLE2,
    SC_BB_UNKNOWN_WAIT_BYTE, SC_BB_UNKNOWN_WAIT_UBYTE, SC_BB_UNKNOWN_WAIT_WORD, SC_BB_UNKNOWN_WAIT_UWORD, SC_BB_UNKNOWN_WAIT_DWORD,
    SC_BB_UNKNOWN_WAIT_UDWORD, SC_BB_UNKNOWN_WAIT_FLOAT, SC_BB_UNKNOWN_WAIT_DOUBLE,
    SC_BB_QWORD, SC_BB_UQWORD, SC_BB_UNKNOWN_QWORD, SC_BB_UNKNOWN_UQWORD, SC_BB_UNKNOWN_WAIT_QWORD, SC_BB_UNKNOWN_WAIT_UQWORD
};

union SC_BB_VARI {
    signed char b;             // BB_BYTE
    unsigned char ub;          // BB_UBYTE
    short w;                   // BB_WORD
    unsigned short uw;         // BB_UWORD
    WSC_TYPE_INT32 dw;         // BB_DWORD
    WSC_TYPE_UINT32 udw;       // BB_UDWORD
    long long qw;              // BB_QWORD
    unsigned long long uqw;    // BB_UQWORD
    float f;                   // BB_FLOAT
    double d;                  // BB_DOUBLE
};

// Add, attach, and remove variable
EXPORT_OR_IMPORT VID __FUNC_CALL_CONVETION__ add_bbvari (const char *name, int type, const char *unit);
EXPORT_OR_IMPORT VID __FUNC_CALL_CONVETION__ attach_bbvari (const char *Name);
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ remove_bbvari(VID vid);

// write to a variable
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_byte (VID vid, signed char v);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_ubyte (VID vid, unsigned char v);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_word (VID vid, short v);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_uword (VID vid, unsigned short v);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_dword (VID vid, WSC_TYPE_INT32 v);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_udword (VID vid, WSC_TYPE_UINT32 v);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_qword (VID vid, long long v);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_uqword (VID vid, unsigned long long v);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_float (VID vid, float v);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_double (VID vid, double v);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_minmax_check (VID vid, double v);

// read from a varible
EXPORT_OR_IMPORT signed char __FUNC_CALL_CONVETION__ read_bbvari_byte (VID vid);
EXPORT_OR_IMPORT unsigned char __FUNC_CALL_CONVETION__ read_bbvari_ubyte (VID vid);
EXPORT_OR_IMPORT short __FUNC_CALL_CONVETION__ read_bbvari_word (VID vid);
EXPORT_OR_IMPORT unsigned short __FUNC_CALL_CONVETION__ read_bbvari_uword (VID vid);
EXPORT_OR_IMPORT WSC_TYPE_INT32 __FUNC_CALL_CONVETION__ read_bbvari_dword (VID vid);
EXPORT_OR_IMPORT WSC_TYPE_UINT32 __FUNC_CALL_CONVETION__ read_bbvari_udword (VID vid);
EXPORT_OR_IMPORT long long __FUNC_CALL_CONVETION__ read_bbvari_qword (VID vid);
EXPORT_OR_IMPORT unsigned long long __FUNC_CALL_CONVETION__ read_bbvari_uqword (VID vid);
EXPORT_OR_IMPORT float __FUNC_CALL_CONVETION__ read_bbvari_float (VID vid);
EXPORT_OR_IMPORT double __FUNC_CALL_CONVETION__ read_bbvari_double (VID vid);
EXPORT_OR_IMPORT double __FUNC_CALL_CONVETION__ read_bbvari_convert_double (VID vid);

// Functions you have to implement

#ifdef EXTERN_PROCESS_AS_DLL
__declspec(dllexport) void __stdcall cyclic_test_object (void);
__declspec(dllexport) int __stdcall init_test_object (void);
__declspec(dllexport) void __stdcall terminate_test_object (void);
__declspec(dllexport) void __stdcall reference_varis (void);
#else
extern void cyclic_test_object (void);
extern int init_test_object (void);
extern void terminate_test_object (void);
extern void reference_varis (void);
#endif

#ifdef __cplusplus
}
#endif // #ifdef __cplusplus


/* Reference macros for data type "DOUBLE" = "double" = "double" */
#define REFERENCE_DOUBLE_VAR(ptr, name) \
    reference_double_var (&(ptr), (name));

#define REFERENCE_DOUBLE_VAR_UNIT(ptr, name, unit) \
    reference_double_var_unit (&(ptr), (name), (unit));

#define REF_DOUBLE_VAR(name) \
    reference_double_var (&(name), (#name));

#define REF_DOUBLE_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_double_var (&(name[idx]), str); \
    }

#define REF_DOUBLE_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_double_var (ptr, str); \
    }

#define REF_DOUBLE_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_double_var_all_infos (&(name), (#name), unit, \
                                    convtype, conversion, \
                                    (double)(pmin), (double)(pmax), \
                                    width, prec, \
                                    rgb_color, \
                                    steptype, (double)(step));

#define REF_DIR_DOUBLE_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_double_var_all_infos_flags (&(name), (#name), unit, \
                                          convtype, conversion, \
                                          (double)(pmin), (double)(pmax), \
                                          width, prec, \
                                          rgb_color, \
                                          steptype, (double)(step), dir);

#define REF_PTR_DOUBLE_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_double_var_all_infos_flags (ptr, name, unit, \
                                          convtype, conversion, \
                                          (double)(pmin), (double)(pmax), \
                                          width, prec, \
                                          rgb_color, \
                                          steptype, (double)(step), dir);

#define REF_DIR_DOUBLE_VAR(dir, ptr, name, unit) \
    reference_double_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "FLOAT" = "float" = "float" */
#define REFERENCE_FLOAT_VAR(ptr, name) \
    reference_float_var (&(ptr), (name));

#define REFERENCE_FLOAT_VAR_UNIT(ptr, name, unit) \
    reference_float_var_unit (&(ptr), (name), (unit));

#define REF_FLOAT_VAR(name) \
    reference_float_var (&(name), (#name));

#define REF_FLOAT_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_float_var (&(name[idx]), str); \
    }

#define REF_FLOAT_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_float_var (ptr, str); \
    }

#define REF_FLOAT_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_float_var_all_infos (&(name), (#name), unit, \
                                   convtype, conversion, \
                                   (double)(pmin), (double)(pmax), \
                                   width, prec, \
                                   rgb_color, \
                                   steptype, (double)(step));

#define REF_DIR_FLOAT_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_float_var_all_infos_flags (&(name), (#name), unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_PTR_FLOAT_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_float_var_all_infos_flags (ptr, name, unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_DIR_FLOAT_VAR(dir, ptr, name, unit) \
    reference_float_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "QWORD" = "qword" = "long long" */
#define REFERENCE_QWORD_VAR(ptr, name) \
    reference_qword_var (&(ptr), (name));

#define REFERENCE_QWORD_VAR_UNIT(ptr, name, unit) \
    reference_qword_var_unit (&(ptr), (name), (unit));

#define REF_QWORD_VAR(name) \
    reference_qword_var (&(name), (#name));

#define REF_QWORD_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_qword_var (&(name[idx]), str); \
    }

#define REF_QWORD_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_qword_var (ptr, str); \
    }

#define REF_QWORD_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_qword_var_all_infos (&(name), (#name), unit, \
                                   convtype, conversion, \
                                   (double)(pmin), (double)(pmax), \
                                   width, prec, \
                                   rgb_color, \
                                   steptype, (double)(step));

#define REF_DIR_QWORD_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_qword_var_all_infos_flags (&(name), (#name), unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_PTR_QWORD_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_qword_var_all_infos_flags (ptr, name, unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_DIR_QWORD_VAR(dir, ptr, name, unit) \
    reference_qword_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "UQWORD" = "uqword" = "unsigned long long" */
#define REFERENCE_UQWORD_VAR(ptr, name) \
    reference_uqword_var (&(ptr), (name));

#define REFERENCE_UQWORD_VAR_UNIT(ptr, name, unit) \
    reference_uqword_var_unit (&(ptr), (name), (unit));

#define REF_UQWORD_VAR(name) \
    reference_uqword_var (&(name), (#name));

#define REF_UQWORD_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_uqword_var (&(name[idx]), str); \
    }

#define REF_UQWORD_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_uqword_var (ptr, str); \
    }

#define REF_UQWORD_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_uqword_var_all_infos (&(name), (#name), unit, \
                                    convtype, conversion, \
                                    (double)(pmin), (double)(pmax), \
                                    width, prec, \
                                    rgb_color, \
                                    steptype, (double)(step));

#define REF_DIR_UQWORD_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_uqword_var_all_infos_flags (&(name), (#name), unit, \
                                          convtype, conversion, \
                                          (double)(pmin), (double)(pmax), \
                                          width, prec, \
                                          rgb_color, \
                                          steptype, (double)(step), dir);

#define REF_PTR_UQWORD_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_uqword_var_all_infos_flags (ptr, name, unit, \
                                          convtype, conversion, \
                                          (double)(pmin), (double)(pmax), \
                                          width, prec, \
                                          rgb_color, \
                                          steptype, (double)(step), dir);

#define REF_DIR_UQWORD_VAR(dir, ptr, name, unit) \
    reference_uqword_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "DWORD" = "dword" = "long" */
#define REFERENCE_DWORD_VAR(ptr, name) \
    reference_dword_var (&(ptr), (name));

#define REFERENCE_DWORD_VAR_UNIT(ptr, name, unit) \
    reference_dword_var_unit (&(ptr), (name), (unit));

#define REF_DWORD_VAR(name) \
    reference_dword_var (&(name), (#name));

#define REF_DWORD_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_dword_var (&(name[idx]), str); \
    }

#define REF_DWORD_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_dword_var (ptr, str); \
    }

#define REF_DWORD_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_dword_var_all_infos (&(name), (#name), unit, \
                                   convtype, conversion, \
                                   (double)(pmin), (double)(pmax), \
                                   width, prec, \
                                   rgb_color, \
                                   steptype, (double)(step));

#define REF_DIR_DWORD_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_dword_var_all_infos_flags (&(name), (#name), unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_PTR_DWORD_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_dword_var_all_infos_flags (ptr, name, unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_DIR_DWORD_VAR(dir, ptr, name, unit) \
    reference_dword_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "UDWORD" = "udword" = "unsigned long" */
#define REFERENCE_UDWORD_VAR(ptr, name) \
    reference_udword_var (&(ptr), (name));

#define REFERENCE_UDWORD_VAR_UNIT(ptr, name, unit) \
    reference_udword_var_unit (&(ptr), (name), (unit));

#define REF_UDWORD_VAR(name) \
    reference_udword_var (&(name), (#name));

#define REF_UDWORD_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_udword_var (&(name[idx]), str); \
    }

#define REF_UDWORD_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_udword_var (ptr, str); \
    }

#define REF_UDWORD_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_udword_var_all_infos (&(name), (#name), unit, \
                                    convtype, conversion, \
                                    (double)(pmin), (double)(pmax), \
                                    width, prec, \
                                    rgb_color, \
                                    steptype, (double)(step));

#define REF_DIR_UDWORD_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_udword_var_all_infos_flags (&(name), (#name), unit, \
                                          convtype, conversion, \
                                          (double)(pmin), (double)(pmax), \
                                          width, prec, \
                                          rgb_color, \
                                          steptype, (double)(step), dir);

#define REF_PTR_UDWORD_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_udword_var_all_infos_flags (ptr, name, unit, \
                                          convtype, conversion, \
                                          (double)(pmin), (double)(pmax), \
                                          width, prec, \
                                          rgb_color, \
                                          steptype, (double)(step), dir);

#define REF_DIR_UDWORD_VAR(dir, ptr, name, unit) \
    reference_udword_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "INT" = "dword" = "long" */
#define REFERENCE_INT_VAR(ptr, name) \
    reference_dword_var (&(ptr), (name));

#define REFERENCE_INT_VAR_UNIT(ptr, name, unit) \
    reference_dword_var_unit (&(ptr), (name), (unit));

#define REF_INT_VAR(name) \
    reference_dword_var (&(name), (#name));

#define REF_INT_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_dword_var (&(name[idx]), str); \
    }

#define REF_INT_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_dword_var (ptr, str); \
    }

#define REF_INT_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_dword_var_all_infos (&(name), (#name), unit, \
                                   convtype, conversion, \
                                   (double)(pmin), (double)(pmax), \
                                   width, prec, \
                                   rgb_color, \
                                   steptype, (double)(step));

#define REF_DIR_INT_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_dword_var_all_infos_flags (&(name), (#name), unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_PTR_INT_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_dword_var_all_infos_flags (ptr, name, unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_DIR_INT_VAR(dir, ptr, name, unit) \
    reference_dword_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "UINT" = "udword" = "unsigned long" */
#define REFERENCE_UINT_VAR(ptr, name) \
    reference_udword_var (&(ptr), (name));

#define REFERENCE_UINT_VAR_UNIT(ptr, name, unit) \
    reference_udword_var_unit (&(ptr), (name), (unit));

#define REF_UINT_VAR(name) \
    reference_udword_var (&(name), (#name));

#define REF_UINT_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_udword_var (&(name[idx]), str); \
    }

#define REF_UINT_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_udword_var (ptr, str); \
    }

#define REF_UINT_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_udword_var_all_infos (&(name), (#name), unit, \
                                    convtype, conversion, \
                                    (double)(pmin), (double)(pmax), \
                                    width, prec, \
                                    rgb_color, \
                                    steptype, (double)(step));

#define REF_DIR_UINT_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_udword_var_all_infos_flags (&(name), (#name), unit, \
                                          convtype, conversion, \
                                          (double)(pmin), (double)(pmax), \
                                          width, prec, \
                                          rgb_color, \
                                          steptype, (double)(step), dir);

#define REF_PTR_UINT_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_udword_var_all_infos_flags (ptr, name, unit, \
                                          convtype, conversion, \
                                          (double)(pmin), (double)(pmax), \
                                          width, prec, \
                                          rgb_color, \
                                          steptype, (double)(step), dir);

#define REF_DIR_UINT_VAR(dir, ptr, name, unit) \
    reference_udword_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "CHAR" = "byte" = "signed char" */
#define REFERENCE_CHAR_VAR(ptr, name) \
    reference_byte_var (&(ptr), (name));

#define REFERENCE_CHAR_VAR_UNIT(ptr, name, unit) \
    reference_byte_var_unit (&(ptr), (name), (unit));

#define REF_CHAR_VAR(name) \
    reference_byte_var (&(name), (#name));

#define REF_CHAR_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_byte_var (&(name[idx]), str); \
    }

#define REF_CHAR_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_byte_var (ptr, str); \
    }

#define REF_CHAR_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_byte_var_all_infos (&(name), (#name), unit, \
                                  convtype, conversion, \
                                  (double)(pmin), (double)(pmax), \
                                  width, prec, \
                                  rgb_color, \
                                  steptype, (double)(step));

#define REF_DIR_CHAR_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_byte_var_all_infos_flags (&(name), (#name), unit, \
                                        convtype, conversion, \
                                        (double)(pmin), (double)(pmax), \
                                        width, prec, \
                                        rgb_color, \
                                        steptype, (double)(step), dir);

#define REF_PTR_CHAR_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_byte_var_all_infos_flags (ptr, name, unit, \
                                        convtype, conversion, \
                                        (double)(pmin), (double)(pmax), \
                                        width, prec, \
                                        rgb_color, \
                                        steptype, (double)(step), dir);

#define REF_DIR_CHAR_VAR(dir, ptr, name, unit) \
    reference_byte_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "UCHAR" = "ubyte" = "unsigned char" */
#define REFERENCE_UCHAR_VAR(ptr, name) \
    reference_ubyte_var (&(ptr), (name));

#define REFERENCE_UCHAR_VAR_UNIT(ptr, name, unit) \
    reference_ubyte_var_unit (&(ptr), (name), (unit));

#define REF_UCHAR_VAR(name) \
    reference_ubyte_var (&(name), (#name));

#define REF_UCHAR_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_ubyte_var (&(name[idx]), str); \
    }

#define REF_UCHAR_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_ubyte_var (ptr, str); \
    }

#define REF_UCHAR_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_ubyte_var_all_infos (&(name), (#name), unit, \
                                   convtype, conversion, \
                                   (double)(pmin), (double)(pmax), \
                                   width, prec, \
                                   rgb_color, \
                                   steptype, (double)(step));

#define REF_DIR_UCHAR_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_ubyte_var_all_infos_flags (&(name), (#name), unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_PTR_UCHAR_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_ubyte_var_all_infos_flags (ptr, name, unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_DIR_UCHAR_VAR(dir, ptr, name, unit) \
    reference_ubyte_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "WORD" = "word" = "short" */
#define REFERENCE_WORD_VAR(ptr, name) \
    reference_word_var (&(ptr), (name));

#define REFERENCE_WORD_VAR_UNIT(ptr, name, unit) \
    reference_word_var_unit (&(ptr), (name), (unit));

#define REF_WORD_VAR(name) \
    reference_word_var (&(name), (#name));

#define REF_WORD_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_word_var (&(name[idx]), str); \
    }

#define REF_WORD_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_word_var (ptr, str); \
    }

#define REF_WORD_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_word_var_all_infos (&(name), (#name), unit, \
                                  convtype, conversion, \
                                  (double)(pmin), (double)(pmax), \
                                  width, prec, \
                                  rgb_color, \
                                  steptype, (double)(step));

#define REF_DIR_WORD_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_word_var_all_infos_flags (&(name), (#name), unit, \
                                        convtype, conversion, \
                                        (double)(pmin), (double)(pmax), \
                                        width, prec, \
                                        rgb_color, \
                                        steptype, (double)(step), dir);

#define REF_PTR_WORD_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_word_var_all_infos_flags (ptr, name, unit, \
                                        convtype, conversion, \
                                        (double)(pmin), (double)(pmax), \
                                        width, prec, \
                                        rgb_color, \
                                        steptype, (double)(step), dir);

#define REF_DIR_WORD_VAR(dir, ptr, name, unit) \
    reference_word_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "UWORD" = "uword" = "unsigned short" */
#define REFERENCE_UWORD_VAR(ptr, name) \
    reference_uword_var (&(ptr), (name));

#define REFERENCE_UWORD_VAR_UNIT(ptr, name, unit) \
    reference_uword_var_unit (&(ptr), (name), (unit));

#define REF_UWORD_VAR(name) \
    reference_uword_var (&(name), (#name));

#define REF_UWORD_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_uword_var (&(name[idx]), str); \
    }

#define REF_UWORD_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_uword_var (ptr, str); \
    }

#define REF_UWORD_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_uword_var_all_infos (&(name), (#name), unit, \
                                   convtype, conversion, \
                                   (double)(pmin), (double)(pmax), \
                                   width, prec, \
                                   rgb_color, \
                                   steptype, (double)(step));

#define REF_DIR_UWORD_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_uword_var_all_infos_flags (&(name), (#name), unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_PTR_UWORD_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_uword_var_all_infos_flags (ptr, name, unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_DIR_UWORD_VAR(dir, ptr, name, unit) \
    reference_uword_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "BYTE" = "byte" = "signed char" */
#define REFERENCE_BYTE_VAR(ptr, name) \
    reference_byte_var (&(ptr), (name));

#define REFERENCE_BYTE_VAR_UNIT(ptr, name, unit) \
    reference_byte_var_unit (&(ptr), (name), (unit));

#define REF_BYTE_VAR(name) \
    reference_byte_var (&(name), (#name));

#define REF_BYTE_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_byte_var (&(name[idx]), str); \
    }

#define REF_BYTE_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_byte_var (ptr, str); \
    }

#define REF_BYTE_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_byte_var_all_infos (&(name), (#name), unit, \
                                  convtype, conversion, \
                                  (double)(pmin), (double)(pmax), \
                                  width, prec, \
                                  rgb_color, \
                                  steptype, (double)(step));

#define REF_DIR_BYTE_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_byte_var_all_infos_flags (&(name), (#name), unit, \
                                        convtype, conversion, \
                                        (double)(pmin), (double)(pmax), \
                                        width, prec, \
                                        rgb_color, \
                                        steptype, (double)(step), dir);

#define REF_PTR_BYTE_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_byte_var_all_infos_flags (ptr, name, unit, \
                                        convtype, conversion, \
                                        (double)(pmin), (double)(pmax), \
                                        width, prec, \
                                        rgb_color, \
                                        steptype, (double)(step), dir);

#define REF_DIR_BYTE_VAR(dir, ptr, name, unit) \
    reference_byte_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "UBYTE" = "ubyte" = "unsigned char" */
#define REFERENCE_UBYTE_VAR(ptr, name) \
    reference_ubyte_var (&(ptr), (name));

#define REFERENCE_UBYTE_VAR_UNIT(ptr, name, unit) \
    reference_ubyte_var_unit (&(ptr), (name), (unit));

#define REF_UBYTE_VAR(name) \
    reference_ubyte_var (&(name), (#name));

#define REF_UBYTE_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_ubyte_var (&(name[idx]), str); \
    }

#define REF_UBYTE_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_ubyte_var (ptr, str); \
    }

#define REF_UBYTE_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_ubyte_var_all_infos (&(name), (#name), unit, \
                                   convtype, conversion, \
                                   (double)(pmin), (double)(pmax), \
                                   width, prec, \
                                   rgb_color, \
                                   steptype, (double)(step));

#define REF_DIR_UBYTE_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_ubyte_var_all_infos_flags (&(name), (#name), unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_PTR_UBYTE_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_ubyte_var_all_infos_flags (ptr, name, unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_DIR_UBYTE_VAR(dir, ptr, name, unit) \
    reference_ubyte_var_flags (&(ptr), (name), (unit), dir);


/* Reference macros for data type "BOOL" = "ubyte" = "unsigned char" */
#define REFERENCE_BOOL_VAR(ptr, name) \
    reference_ubyte_var ((unsigned char*)&(ptr), (name));

#define REFERENCE_BOOL_VAR_UNIT(ptr, name, unit) \
    reference_ubyte_var_unit ((unsigned char*)&(ptr), (name), (unit));

#define REF_BOOL_VAR(name) \
    reference_ubyte_var ((unsigned char*)&(name), (#name));

#define REF_BOOL_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_ubyte_var ((unsigned char*)&(name[idx]), str); \
    }

#define REF_BOOL_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_ubyte_var ((unsigned char*)ptr, str); \
    }

#define REF_BOOL_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_ubyte_var_all_infos ((unsigned char*)&(name), (#name), unit, \
                                   convtype, conversion, \
                                   (double)(pmin), (double)(pmax), \
                                   width, prec, \
                                   rgb_color, \
                                   steptype, (double)(step));

#define REF_DIR_BOOL_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_ubyte_var_all_infos_flags ((unsigned char*)&(name), (#name), unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_PTR_BOOL_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_ubyte_var_all_infos_flags (ptr, name, unit, \
                                         convtype, conversion, \
                                         (double)(pmin), (double)(pmax), \
                                         width, prec, \
                                         rgb_color, \
                                         steptype, (double)(step), dir);

#define REF_DIR_BOOL_VAR(dir, ptr, name, unit) \
    reference_ubyte_var_flags ((unsigned char*)&(ptr), (name), (unit), dir);

/* Reference macros for data type "ENUM" */
#define REFERENCE_ENUM_VAR(ptr, name) \
    switch (sizeof (ptr)) {\
    case 1: reference_byte_var ((signed char*)&(ptr), (name)); break;\
    case 2: reference_word_var ((short*)&(ptr), (name)); break;\
    case 4: reference_dword_var ((WSC_TYPE_INT32*)&(ptr), (name)); break;\
    default: ThrowError (1, "call REFERENCE_ENUM_VAR macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

#define REFERENCE_ENUM_VAR_UNIT(ptr, name, unit)\
    switch (sizeof (ptr)) {\
    case 1: reference_byte_var_unit ((signed char*)&(ptr), (name), (unit)); break;\
    case 2: reference_word_var_unit ((short*)&(ptr), (name), (unit)); break;\
    case 4: reference_dword_var_unit ((WSC_TYPE_INT32*)&(ptr), (name), (unit)); break;\
    default: ThrowError (1, "call REFERENCE_ENUM_VAR_UNIT macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

#define REF_ENUM_VAR(name) \
    switch (sizeof (name)) {\
    case 1: reference_byte_var ((signed char*)&(name), (#name)); break;\
    case 2: reference_word_var ((short*)&(name), (#name)); break;\
    case 4: reference_dword_var ((WSC_TYPE_INT32*)&(name), (#name)); break;\
    default: ThrowError (1, "call REF_ENUM_VAR macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

#define REF_ENUM_VAR_INDEXED(name, idx) \
    {\
        char str[SC_MAX_LABEL_SIZE];\
        sprintf (str,"%s[%d]", #name, idx);\
        switch (sizeof (name[0])) {\
        case 1: reference_byte_var ((signed char*)&(name[idx]), str); break;\
        case 2: reference_word_var ((short*)&(name[idx]), str); break;\
        case 4: reference_dword_var ((WSC_TYPE_INT32*)&(name[idx]), str);  break;\
        default: ThrowError (1, "call REF_ENUM_VAR_INDEXED macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
        }\
    }

#define REF_ENUM_INDEXED(name, ptr, idx) \
    {\
        char str[SC_MAX_LABEL_SIZE];\
        sprintf (str,"%s[%d]", #name, idx);\
        switch (sizeof (*ptr)) {\
        case 1: reference_byte_var ((signed char*)ptr, str); break;\
        case 2: reference_word_var ((short*)ptr, str); break;\
        case 4: reference_dword_var ((WSC_TYPE_INT32*)ptr, str); break;\
        default: ThrowError (1, "call REF_ENUM_INDEXED macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
        }\
    }

#define REF_ENUM_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    switch (sizeof (name)) {\
    case 1: reference_byte_var_all_infos ((signed char*)&(name), (#name), unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step)); break;\
    case 2: reference_word_var_all_infos ((short*)&(name), (#name), unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step)); break;\
    case 4: reference_dword_var_all_infos ((WSC_TYPE_INT32*)&(name), (#name), unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step)); break;\
    default: ThrowError (1, "call REF_ENUM_VAR_AI macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

#define REF_DIR_ENUM_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    switch (sizeof (name)) {\
    case 1: reference_byte_var_all_infos_flags ((signed char*)&(name), (#name), unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step), dir); break;\
    case 2: reference_word_var_all_infos_flags ((short*)&(name), (#name), unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step), dir); break;\
    case 4: reference_dword_var_all_infos_flags ((WSC_TYPE_INT32*)&(name), (#name), unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step), dir); break;\
    default: ThrowError (1, "call REF_DIR_ENUM_VAR_AI macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

#define REF_PTR_ENUM_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    switch (sizeof (name)) {\
    case 1: reference_byte_var_all_infos_flags ((signed char*)ptr, name, unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step), dir); break;\
    case 2: reference_word_var_all_infos_flags ((short*)ptr, name, unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step), dir); break;\
    case 4: reference_dword_var_all_infos_flags ((WSC_TYPE_INT32*)ptr, name, unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step), dir); break;\
    default: error (1, "call REF_PTR_ENUM_VAR_AI macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

#define REF_DIR_ENUM_VAR(dir, ptr, name, unit) \
    switch (sizeof (ptr)) {\
    case 1: reference_byte_var_flags ((signed char*)&(ptr), (name), (unit), dir); break;\
    case 2: reference_word_var_flags ((short*)&(ptr), (name), (unit), dir); break;\
    case 4: reference_dword_var_flags ((WSC_TYPE_INT32*)&(ptr), (name), (unit), dir); break;\
    default: ThrowError (1, "call REF_DIR_ENUM_VAR macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

/* Reference macros for data type "UENUM" */
#define REFERENCE_UENUM_VAR(ptr, name) \
    switch (sizeof (ptr)) {\
    case 1: reference_ubyte_var ((unsigned char*)&(ptr), (name)); break;\
    case 2: reference_uword_var ((unsigned short*)&(ptr), (name)); break;\
    case 4: reference_udword_var ((WSC_TYPE_UINT32*)&(ptr), (name)); break;\
    default: ThrowError (1, "call REFERENCE_UENUM_VAR macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

#define REFERENCE_UENUM_VAR_UNIT(ptr, name, unit) \
    switch (sizeof (ptr)) {\
    case 1: reference_ubyte_var_unit ((unsigned char*)&(ptr), (name), (unit)); break;\
    case 2: reference_uword_var_unit ((unsigned short*)&(ptr), (name), (unit)); break;\
    case 4: reference_udword_var_unit ((WSC_TYPE_UINT32*)&(ptr), (name), (unit)); break;\
    default: ThrowError (1, "call REFERENCE_UENUM_VAR_UNIT macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

#define REF_UENUM_VAR(name) \
    switch (sizeof (name)) {\
    case 1: reference_ubyte_var ((unsigned char*)&(name), (#name)); break;\
    case 2: reference_uword_var ((unsigned short*)&(name), (#name)); break;\
    case 4: reference_udword_var ((WSC_TYPE_UINT32*)&(name), (#name)); break;\
    default: ThrowError (1, "call REF_UENUM_VAR macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

#define REF_UENUM_VAR_INDEXED(name, idx) \
    {\
        char str[SC_MAX_LABEL_SIZE];\
        sprintf (str,"%s[%d]", #name, idx);\
        switch (sizeof (name[0])) {\
        case 1: reference_ubyte_var ((unsigned char*)&(name[idx]), str); break;\
        case 2: reference_uword_var ((unsigned short*)&(name[idx]), str); break;\
        case 4: reference_udword_var ((WSC_TYPE_UINT32*)&(name[idx]), str); break;\
        default: ThrowError (1, "call REF_UENUM_VAR_INDEXED macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
        }\
    }

#define REF_UENUM_INDEXED(name, ptr, idx) \
    {\
        char str[SC_MAX_LABEL_SIZE];\
        sprintf (str,"%s[%d]", #name, idx);\
        switch (sizeof (*ptr)) {\
        case 1: reference_ubyte_var ((unsigned char*)ptr, str); break;\
        case 2: reference_uword_var ((unsigned short*)ptr, str); break;\
        case 4: reference_udword_var ((WSC_TYPE_UINT32*)ptr, str); break;\
        default: ThrowError (1, "call REF_UENUM_INDEXED macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
        }\
    }

#define REF_UENUM_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    switch (sizeof (name)) {\
    case 1: reference_ubyte_var_all_infos ((unsigned char*)&(name), (#name), unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step)); break;\
    case 2: reference_uword_var_all_infos ((unsigned short*)&(name), (#name), unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step)); break;\
    case 4: reference_udword_var_all_infos ((WSC_TYPE_UINT32*)&(name), (#name), unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step)); break;\
    default: ThrowError (1, "call REF_UENUM_VAR_AI macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

#define REF_DIR_UENUM_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    switch (sizeof (name)) {\
    case 1: reference_ubyte_var_all_infos_flags ((unsigned char*)&(name), (#name), unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step), dir); break;\
    case 2: reference_uword_var_all_infos_flags ((unsigned short*)&(name), (#name), unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step), dir); break;\
    case 4: reference_udword_var_all_infos_flags ((WSC_TYPE_UINT32*)&(name), (#name), unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step), dir); break;\
    default: ThrowError (1, "call REF_DIR_UENUM_VAR_AI macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

#define REF_PTR_UENUM_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    switch (sizeof (name)) {\
    case 1: reference_ubyte_var_all_infos_flags ((unsigned char*)ptr, name, unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step), dir); break;\
    case 2: reference_uword_var_all_infos_flags ((unsigned short*)ptr, name, unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step), dir); break;\
    case 4: reference_udword_var_all_infos_flags ((WSC_TYPE_UINT32*)ptr, name, unit, convtype, conversion, (double)(pmin), (double)(pmax), width, prec, rgb_color, steptype, (double)(step), dir); break;\
        default: error (1, "call REF_PTR_UENUM_VAR_AI macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

#define REF_DIR_UENUM_VAR(dir, ptr, name, unit) \
    switch (sizeof (ptr)) {\
    case 1: reference_ubyte_var_flags ((unsigned char*)&(ptr), (name), (unit), dir); break;\
    case 2: reference_uword_var_flags ((unsigned short*)&(ptr), (name), (unit), dir); break;\
    case 4: reference_udword_var_flags ((WSC_TYPE_UINT32*)&(ptr), (name), (unit), dir); break;\
    default: ThrowError (1, "call REF_DIR_UENUM_VAR macro with a reference having a wrong size \"%s\" (%i)", __FILE__, __LINE__); break;\
    }

/* Reference macros for unknown data type (the datatype will be searched in the debug info) */
#define REFERENCE_XXX_VAR(ptr, name) \
    reference_xxx_var (&(ptr), (name));

#define REFERENCE_XXX_VAR_UNIT(ptr, name, unit) \
    reference_xxx_var_unit (&(ptr), (name), (unit));

#define REF_XXX_VAR(name) \
    reference_xxx_var (&(name), (#name));

#define REF_XXX_VAR_INDEXED(name, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_xxx_var (&(name[idx]), str); \
    }

#define REF_XXX_INDEXED(name, ptr, idx) \
    { \
        char str[SC_MAX_LABEL_SIZE]; \
        sprintf (str,"%s[%d]", #name, idx); \
        reference_xxx_var (ptr, str); \
    }

#define REF_XXX_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_xxx_var_all_infos (&(name), (#name), unit, \
                                 convtype, conversion, \
                                 (double)(pmin), (double)(pmax), \
                                 width, prec, \
                                 rgb_color, \
                                 steptype, (double)(step));

#define REF_DIR_XXX_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_xxx_var_all_infos_flags (&(name), (#name), unit, \
                                       convtype, conversion, \
                                       (double)(pmin), (double)(pmax), \
                                       width, prec, \
                                       rgb_color, \
                                       steptype, (double)(step), dir);

#define REF_PTR_XXX_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec, rgb_color, steptype, step) \
    reference_xxx_var_all_infos_flags (&(name), name, unit, \
                                       convtype, conversion, \
                                       (double)(pmin), (double)(pmax), \
                                       width, prec, \
                                       rgb_color, \
                                       steptype, (double)(step), dir);

#define REF_DIR_XXX_VAR(dir, ptr, name, unit) \
    reference_xxx_var_flags (&(ptr), (name), (unit), dir);


// new reference macros for type-independent access -> only usable with C++ !
#ifdef __cplusplus
#define REFERENCE_VAR(ptr, name) \
    cReferenceWrapper::RefVar (&(ptr), (name));

#define REF_VAR(name) \
    cReferenceWrapper::RefVar (&(name), (#name));

#define REFERENCE_VAR_UNIT(ptr, name, unit) \
    cReferenceWrapper::RefVarUnit (&(ptr), (name), (unit));

#define REF_VAR_UNIT(name, unit) \
    cReferenceWrapper::RefVarUnit (&(name), (#name), (unit));

#define REFERENCE_VAR_AI(ptr, name, unit, convtype, conversion, pmin, pmax, width,\
                         prec, rgb_color, steptype, step) \
    cReferenceWrapper::RefVarAllInfos (&(ptr), (name), unit, \
                                       convtype, conversion, \
                                       (double)(pmin), (double)(pmax), \
                                       width, prec, \
                                       rgb_color, \
                                       steptype, (double)(step));

#define REF_VAR_AI(name, unit, convtype, conversion, pmin, pmax, width, prec,\
                   rgb_color, steptype, step) \
    cReferenceWrapper::RefVarAllInfos (&(name), (#name), unit, \
                                       convtype, conversion, \
                                       (double)(pmin), (double)(pmax), \
                                       width, prec, \
                                       rgb_color, \
                                       steptype, (double)(step));

#define REF_DIR_VAR_AI(dir, name, unit, convtype, conversion, pmin, pmax, width, prec,\
                       rgb_color, steptype, step) \
    cReferenceWrapper::RefVarAllInfosFlags (&(name), (#name), unit, \
                                            convtype, conversion, \
                                            (double)(pmin), (double)(pmax), \
                                            width, prec, \
                                            rgb_color, \
                                            steptype, (double)(step));

#define REF_PTR_VAR_AI(dir, ptr, name, unit, convtype, conversion, pmin, pmax, width, prec,\
                       rgb_color, steptype, step) \
    cReferenceWrapper::RefVarAllInfosFlags (ptr, name, unit, \
                                            convtype, conversion, \
                                            (double)(pmin), (double)(pmax), \
                                            width, prec, \
                                            rgb_color, \
                                            steptype, (double)(step));

class cReferenceWrapper
{
  public:

    static __inline void RefVar (double *arg_ptr_Var, const char *arg_ptr_Name)
    {
        reference_double_var (arg_ptr_Var, arg_ptr_Name);
    }

    static __inline void RefVarUnit (double *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
        reference_double_var_unit (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
    }

    static __inline void RefVarAllInfos (double *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
        reference_double_var_all_infos (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                        arg_Convtype, arg_ptr_Conversion,
                                        arg_Min, arg_Max,
                                        arg_Width, arg_Prec,
                                        arg_RGBColor,
                                        arg_StepType, arg_step);
    }

    static __inline void RefVarUnitFlags (double *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
        reference_double_var_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
    }

    static __inline void RefVarAllInfosFlags (double *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
        reference_double_var_all_infos_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                              arg_Convtype, arg_ptr_Conversion,
                                              arg_Min, arg_Max,
                                              arg_Width, arg_Prec,
                                              arg_RGBColor,
                                              arg_StepType, arg_step, arg_Flags);
    }

    static __inline void RefVar (float *arg_ptr_Var, const char *arg_ptr_Name)
    {
        reference_float_var (arg_ptr_Var, arg_ptr_Name);
    }

    static __inline void RefVarUnit (float *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
        reference_float_var_unit (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
    }

    static __inline void RefVarAllInfos (float *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
        reference_float_var_all_infos (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                       arg_Convtype, arg_ptr_Conversion,
                                       arg_Min, arg_Max,
                                       arg_Width, arg_Prec,
                                       arg_RGBColor,
                                       arg_StepType, arg_step);
    }

    static __inline void RefVarUnitFlags (float *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
        reference_float_var_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
    }

    static __inline void RefVarAllInfosFlags (float *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
        reference_float_var_all_infos_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                             arg_Convtype, arg_ptr_Conversion,
                                             arg_Min, arg_Max,
                                             arg_Width, arg_Prec,
                                             arg_RGBColor,
                                             arg_StepType, arg_step, arg_Flags);
    }

    static __inline void RefVar (long long *arg_ptr_Var, const char *arg_ptr_Name)
    {
        reference_qword_var (arg_ptr_Var, arg_ptr_Name);
    }

    static __inline void RefVarUnit (long long *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
        reference_qword_var_unit (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
    }

    static __inline void RefVarAllInfos (long long *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
        reference_qword_var_all_infos (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                       arg_Convtype, arg_ptr_Conversion,
                                       arg_Min, arg_Max,
                                       arg_Width, arg_Prec,
                                       arg_RGBColor,
                                       arg_StepType, arg_step);
    }

    static __inline void RefVarUnitFlags (long long *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
        reference_qword_var_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
    }

    static __inline void RefVarAllInfosFlags (long long *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
        reference_qword_var_all_infos_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                             arg_Convtype, arg_ptr_Conversion,
                                             arg_Min, arg_Max,
                                             arg_Width, arg_Prec,
                                             arg_RGBColor,
                                             arg_StepType, arg_step, arg_Flags);
    }

    static __inline void RefVar (unsigned long long *arg_ptr_Var, const char *arg_ptr_Name)
    {
        reference_uqword_var (arg_ptr_Var, arg_ptr_Name);
    }

    static __inline void RefVarUnit (unsigned long long *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
        reference_uqword_var_unit (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
    }

    static __inline void RefVarAllInfos (unsigned long long *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
        reference_uqword_var_all_infos (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                        arg_Convtype, arg_ptr_Conversion,
                                        arg_Min, arg_Max,
                                        arg_Width, arg_Prec,
                                        arg_RGBColor,
                                        arg_StepType, arg_step);
    }

    static __inline void RefVarUnitFlags (unsigned long long *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
        reference_uqword_var_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
    }

    static __inline void RefVarAllInfosFlags (unsigned long long *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
        reference_uqword_var_all_infos_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                              arg_Convtype, arg_ptr_Conversion,
                                              arg_Min, arg_Max,
                                              arg_Width, arg_Prec,
                                              arg_RGBColor,
                                              arg_StepType, arg_step, arg_Flags);
    }
    
    static __inline void RefVar (int *arg_ptr_Var, const char *arg_ptr_Name)
    {
        reference_dword_var ((WSC_TYPE_INT32*)arg_ptr_Var, arg_ptr_Name);
    }

    static __inline void RefVarUnit (int *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
        reference_dword_var_unit ((WSC_TYPE_INT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
    }

    static __inline void RefVarAllInfos (int *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
        reference_dword_var_all_infos ((WSC_TYPE_INT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                       arg_Convtype, arg_ptr_Conversion,
                                       arg_Min, arg_Max,
                                       arg_Width, arg_Prec,
                                       arg_RGBColor,
                                       arg_StepType, arg_step);
    }

    static __inline void RefVarUnitFlags (int *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
        reference_dword_var_flags ((WSC_TYPE_INT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
    }

    static __inline void RefVarAllInfosFlags (int *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
        reference_dword_var_all_infos_flags ((WSC_TYPE_INT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                             arg_Convtype, arg_ptr_Conversion,
                                             arg_Min, arg_Max,
                                             arg_Width, arg_Prec,
                                             arg_RGBColor,
                                             arg_StepType, arg_step, arg_Flags);
    }

    static __inline void RefVar (unsigned int *arg_ptr_Var, const char *arg_ptr_Name)
    {
        reference_udword_var ((WSC_TYPE_UINT32*)arg_ptr_Var, arg_ptr_Name);
    }

    static __inline void RefVarUnit (unsigned int *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
        reference_udword_var_unit ((WSC_TYPE_UINT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
    }

    static __inline void RefVarAllInfos (unsigned int *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
        reference_udword_var_all_infos ((WSC_TYPE_UINT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                        arg_Convtype, arg_ptr_Conversion,
                                        arg_Min, arg_Max,
                                        arg_Width, arg_Prec,
                                        arg_RGBColor,
                                        arg_StepType, arg_step);
    }

    static __inline void RefVarUnitFlags (unsigned int *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
        reference_udword_var_flags ((WSC_TYPE_UINT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
    }

    static __inline void RefVarAllInfosFlags (unsigned int *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
        reference_udword_var_all_infos_flags ((WSC_TYPE_UINT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                              arg_Convtype, arg_ptr_Conversion,
                                              arg_Min, arg_Max,
                                              arg_Width, arg_Prec,
                                              arg_RGBColor,
                                              arg_StepType, arg_step, arg_Flags);
    }

    static __inline void RefVar (long *arg_ptr_Var, const char *arg_ptr_Name)
    {
#ifdef _WIN32
        reference_dword_var ((WSC_TYPE_INT32*)arg_ptr_Var, arg_ptr_Name);
#else
        reference_qword_var ((long long*)arg_ptr_Var, arg_ptr_Name);
#endif
    }

    static __inline void RefVarUnit (long *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
#ifdef _WIN32
        reference_dword_var_unit ((WSC_TYPE_INT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
#else
        reference_qword_var_unit ((long long*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
#endif
    }

    static __inline void RefVarAllInfos (long *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
#ifdef _WIN32
        reference_dword_var_all_infos ((WSC_TYPE_INT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                       arg_Convtype, arg_ptr_Conversion,
                                       arg_Min, arg_Max,
                                       arg_Width, arg_Prec,
                                       arg_RGBColor,
                                       arg_StepType, arg_step);
#else
        reference_qword_var_all_infos ((long long*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                       arg_Convtype, arg_ptr_Conversion,
                                       arg_Min, arg_Max,
                                       arg_Width, arg_Prec,
                                       arg_RGBColor,
                                       arg_StepType, arg_step);
#endif
    }

    static __inline void RefVarUnitFlags (long *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
#ifdef _WIN32
        reference_dword_var_flags ((WSC_TYPE_INT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
#else
        reference_qword_var_flags ((long long*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
#endif
    }

    static __inline void RefVarAllInfosFlags (long *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
#ifdef _WIN32
        reference_dword_var_all_infos_flags ((WSC_TYPE_INT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                             arg_Convtype, arg_ptr_Conversion,
                                             arg_Min, arg_Max,
                                             arg_Width, arg_Prec,
                                             arg_RGBColor,
                                             arg_StepType, arg_step, arg_Flags);
#else
        reference_qword_var_all_infos_flags ((long long*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                             arg_Convtype, arg_ptr_Conversion,
                                             arg_Min, arg_Max,
                                             arg_Width, arg_Prec,
                                             arg_RGBColor,
                                             arg_StepType, arg_step, arg_Flags);
#endif
    }

    static __inline void RefVar (unsigned long *arg_ptr_Var, const char *arg_ptr_Name)
    {
#ifdef _WIN32
        reference_udword_var ((WSC_TYPE_UINT32*)arg_ptr_Var, arg_ptr_Name);
#else
        reference_uqword_var ((unsigned long long*)arg_ptr_Var, arg_ptr_Name);
#endif
    }

    static __inline void RefVarUnit (unsigned long *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
#ifdef _WIN32
        reference_udword_var_unit ((WSC_TYPE_UINT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
#else
        reference_uqword_var_unit ((unsigned long long*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
#endif
    }

    static __inline void RefVarAllInfos (unsigned long *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
#ifdef _WIN32
        reference_udword_var_all_infos ((WSC_TYPE_UINT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                        arg_Convtype, arg_ptr_Conversion,
                                        arg_Min, arg_Max,
                                        arg_Width, arg_Prec,
                                        arg_RGBColor,
                                        arg_StepType, arg_step);
#else
        reference_uqword_var_all_infos ((unsigned long long*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                        arg_Convtype, arg_ptr_Conversion,
                                        arg_Min, arg_Max,
                                        arg_Width, arg_Prec,
                                        arg_RGBColor,
                                        arg_StepType, arg_step);
#endif
    }

    static __inline void RefVarUnitFlags (unsigned long *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
#ifdef _WIN32
        reference_udword_var_flags ((WSC_TYPE_UINT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
#else
        reference_uqword_var_flags ((unsigned long long*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
#endif
    }

    static __inline void RefVarAllInfosFlags (unsigned long *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
#ifdef _WIN32
        reference_udword_var_all_infos_flags ((WSC_TYPE_UINT32*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                              arg_Convtype, arg_ptr_Conversion,
                                              arg_Min, arg_Max,
                                              arg_Width, arg_Prec,
                                              arg_RGBColor,
                                              arg_StepType, arg_step, arg_Flags);
#else
        reference_uqword_var_all_infos_flags ((unsigned long long*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                              arg_Convtype, arg_ptr_Conversion,
                                              arg_Min, arg_Max,
                                              arg_Width, arg_Prec,
                                              arg_RGBColor,
                                              arg_StepType, arg_step, arg_Flags);
#endif
    }    

    static __inline void RefVar (short *arg_ptr_Var, const char *arg_ptr_Name)
    {
        reference_word_var (arg_ptr_Var, arg_ptr_Name);
    }

    static __inline void RefVarUnit (short *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
        reference_word_var_unit (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
    }

    static __inline void RefVarAllInfos (short *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
        reference_word_var_all_infos (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                      arg_Convtype, arg_ptr_Conversion,
                                      arg_Min, arg_Max,
                                      arg_Width, arg_Prec,
                                      arg_RGBColor,
                                      arg_StepType, arg_step);
    }

    static __inline void RefVarUnitFlags (short *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
        reference_word_var_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
    }

    static __inline void RefVarAllInfosFlags (short *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
        reference_word_var_all_infos_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                            arg_Convtype, arg_ptr_Conversion,
                                            arg_Min, arg_Max,
                                            arg_Width, arg_Prec,
                                            arg_RGBColor,
                                            arg_StepType, arg_step, arg_Flags);
    }

    static __inline void RefVar (unsigned short *arg_ptr_Var, const char *arg_ptr_Name)
    {
        reference_uword_var (arg_ptr_Var, arg_ptr_Name);
    }

    static __inline void RefVarUnit (unsigned short *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
        reference_uword_var_unit (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
    }

    static __inline void RefVarAllInfos (unsigned short *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
        reference_uword_var_all_infos (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                       arg_Convtype, arg_ptr_Conversion,
                                       arg_Min, arg_Max,
                                       arg_Width, arg_Prec,
                                       arg_RGBColor,
                                       arg_StepType, arg_step);
    }

    static __inline void RefVarUnitFlags (unsigned short *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
        reference_uword_var_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
    }

    static __inline void RefVarAllInfosFlags (unsigned short *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
        reference_uword_var_all_infos_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                             arg_Convtype, arg_ptr_Conversion,
                                             arg_Min, arg_Max,
                                             arg_Width, arg_Prec,
                                             arg_RGBColor,
                                             arg_StepType, arg_step, arg_Flags);
    }

    static __inline void RefVar (signed char *arg_ptr_Var, const char *arg_ptr_Name)
    {
        reference_byte_var (arg_ptr_Var, arg_ptr_Name);
    }

    static __inline void RefVarUnit (signed char *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
        reference_byte_var_unit (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
    }

    static __inline void RefVarAllInfos (signed char *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
        reference_byte_var_all_infos (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                      arg_Convtype, arg_ptr_Conversion,
                                      arg_Min, arg_Max,
                                      arg_Width, arg_Prec,
                                      arg_RGBColor,
                                      arg_StepType, arg_step);
    }

    static __inline void RefVarUnitFlags (signed char *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
        reference_byte_var_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
    }

    static __inline void RefVarAllInfosFlags (signed char *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
        reference_byte_var_all_infos_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                            arg_Convtype, arg_ptr_Conversion,
                                            arg_Min, arg_Max,
                                            arg_Width, arg_Prec,
                                            arg_RGBColor,
                                            arg_StepType, arg_step, arg_Flags);
    }

    static __inline void RefVar (unsigned char *arg_ptr_Var, const char *arg_ptr_Name)
    {
        reference_ubyte_var (arg_ptr_Var, arg_ptr_Name);
    }

    static __inline void RefVarUnit (unsigned char *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
        reference_ubyte_var_unit (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
    }

    static __inline void RefVarAllInfos (unsigned char *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
        reference_ubyte_var_all_infos (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                       arg_Convtype, arg_ptr_Conversion,
                                       arg_Min, arg_Max,
                                       arg_Width, arg_Prec,
                                       arg_RGBColor,
                                       arg_StepType, arg_step);
    }

    static __inline void RefVarUnitFlags (unsigned char *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
        reference_ubyte_var_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
    }

    static __inline void RefVarAllInfosFlags (unsigned char *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
        reference_ubyte_var_all_infos_flags (arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                             arg_Convtype, arg_ptr_Conversion,
                                             arg_Min, arg_Max,
                                             arg_Width, arg_Prec,
                                             arg_RGBColor,
                                             arg_StepType, arg_step, arg_Flags);
    }

    static __inline void RefVar (bool *arg_ptr_Var, const char *arg_ptr_Name)
    {
        reference_ubyte_var ((unsigned char*)arg_ptr_Var, arg_ptr_Name);
    }

    static __inline void RefVarUnit (bool *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit)
    {
        reference_ubyte_var_unit ((unsigned char*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit);
    }

    static __inline void RefVarAllInfos (bool *arg_ptr_Var, const char *arg_ptr_Name,
                                const char *arg_ptr_Unit, int arg_Convtype,
                                const char *arg_ptr_Conversion,
                                double arg_Min, double arg_Max,
                                int arg_Width, int arg_Prec,
                                unsigned int arg_RGBColor,
                                int arg_StepType, double arg_step)
    {
        reference_ubyte_var_all_infos ((unsigned char*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                       arg_Convtype, arg_ptr_Conversion,
                                       arg_Min, arg_Max,
                                       arg_Width, arg_Prec,
                                       arg_RGBColor,
                                       arg_StepType, arg_step);
    }

    static __inline void RefVarUnitFlags (bool *arg_ptr_Var, const char *arg_ptr_Name, const char *arg_ptr_Unit, unsigned int arg_Flags)
    {
        reference_ubyte_var_flags ((unsigned char*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit, arg_Flags);
    }

    static __inline void RefVarAllInfosFlags (bool *arg_ptr_Var, const char *arg_ptr_Name,
                                     const char *arg_ptr_Unit, int arg_Convtype,
                                     const char *arg_ptr_Conversion,
                                     double arg_Min, double arg_Max,
                                     int arg_Width, int arg_Prec,
                                     unsigned int arg_RGBColor,
                                     int arg_StepType, double arg_step,
                                     unsigned int arg_Flags)
    {
        reference_ubyte_var_all_infos_flags ((unsigned char*)arg_ptr_Var, arg_ptr_Name, arg_ptr_Unit,
                                             arg_Convtype, arg_ptr_Conversion,
                                             arg_Min, arg_Max,
                                             arg_Width, arg_Prec,
                                             arg_RGBColor,
                                             arg_StepType, arg_step, arg_Flags);
    }


};
#endif // #ifdef __cplusplus


#ifdef __cplusplus
extern "C" {
#endif // #ifdef __cplusplus


//  File logging of messages and/or errors

#ifndef XILENV_RT

// Following bits can be set inside the parameter para_Flags of the function init_message_output_stream()

#define FILENAME_ERR_MSG_FLAG           0x00000001 // A file name will be set as parameter
#define ENVVAR_ERR_MSG_FLAG             0x00000002 // A environ variable will be set as parameter (The environmet variable should be set to a file name
#define NO_POPUP_ERR_MSG_FLAG           0x00000004 // If this is set no popup messages should appear
#define IGNORE_ALL_MSG_ERR_FLAG         0x00000008 // Suppress all popups errors during init function
#define NO_ERR_MSG_IF_INIT_ERR_FLAG     0x00000010 // Suppress the popups errors if the file cannot be open
#define MESSAGE_COUNTER_FLAG            0x00000020 // Add a variable message counter inside the blackboard. This will be incremented inside message_output_stream.
#define APPEND_TO_EXISTING_FILE_FLAG    0x00000040 // If this is set the file will be attached and not be overwritten
#define OPEN_FILE_AT_FIRST_MSG_FLAG     0x00000080 // Message file will be gereated delayed within the first message
#define WRITE_MSG_TO_SCRIPT_MSG_FILE    0x00000100 // Message will be written to in Script.msg


// This function can be calledinside init_test_objects
// para_MessageFilename:        The file name (FILENAME_ERR_MSG_FLAG) or the name of the environmet variable (ENVVAR_ERR_MSG_FLAG) which defines the file name
// para_Flags:                  Bitwise combination der above-mentioned defines
// para_MaxErrorMessageCounter: How many Messages would be allowed till XilEnv automatic terminated (0, if no limitation)
// para_Header:                 A Header text, which should be added to the file
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ init_message_output_stream (const char *para_MessageFilename, int para_Flags, int para_MaxErrorMessageCounter,
                                                                         const char *para_HeaderText);

// This function will add amessage the paramter are the same like printf
EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ message_output_stream (const char *format, ...);
EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ message_output_stream_no_variable_arguments (const char *string);
EXPORT_OR_IMPORT void  __FUNC_CALL_CONVETION__ message_output_stream_va_list (const char *format, va_list argptr);

#endif

#ifdef __cplusplus
}
#endif // #ifdef __cplusplus


#if 0
// XCP over Ethernet

#ifndef XILENV_RT

// Prototype of Event Description
// If you want touse XCP over ethernet you have to define at least one event

// This struct exist also inside XcpOverEthernetCfg.h
typedef struct {
    unsigned char timeCycle;                     // Event occures timecycle * timebase
    unsigned char Prio;                          // Priority of the event
    unsigned char timebase;                      // timebase on which timecycle depends on
    // DAQ timestamp unit
    #define DAQ_TIMESTAMP_UNIT_1NS     0
    #define DAQ_TIMESTAMP_UNIT_10NS    1
    #define DAQ_TIMESTAMP_UNIT_100NS   2
    #define DAQ_TIMESTAMP_UNIT_1US     3
    #define DAQ_TIMESTAMP_UNIT_10US    4  
    #define DAQ_TIMESTAMP_UNIT_100US   5
    #define DAQ_TIMESTAMP_UNIT_1MS     6
    #define DAQ_TIMESTAMP_UNIT_10MS    7
    #define DAQ_TIMESTAMP_UNIT_100MS   8
    #define DAQ_TIMESTAMP_UNIT_1S      9
    char Fill1;
    int Fill2;
    char *EventName;
} XCP_EVENT_DESC;

#ifdef __cplusplus
extern  "C" {
#endif

// this function are used for a single instance XCP connector
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ XCPWrapperCreate (void);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperDelete (void);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperSetPort (unsigned short Port);
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ XCPWrapperInit (unsigned char ucAmEvents, XCP_EVENT_DESC* ptrEvents,
                                                             const char *DataSegment, const char *CodeSegment);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperTerminate (void);
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ XCPWrapperCommunicate (void);

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperSetIdentificationString (const char* cpId);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperSetLogfile (const char* cpLogfile);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperXcpEvent (unsigned char ucEvent);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperTakeCommunicationPort (unsigned short usPort);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperActivateDllAddressTranslation (void);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperDeactivateDllAddressTranslation (void);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperSetDaqTimestampUnitAndTicks(unsigned char DaqTimeStampUnit, unsigned char DaqTimestampTicks, unsigned char TimeStampIncOffset);

// this function will be called from the XCPWrapper class (do not call this directly)
EXPORT_OR_IMPORT void* __FUNC_CALL_CONVETION__ XCPWrapperClassConstructor (void);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperClassDelete (void* ThisPtr);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperClassSetPort (void* ThisPtr, unsigned short Port);
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ XCPWrapperClassInit (void* ThisPtr, unsigned char ucAmEvents, XCP_EVENT_DESC* ptrEvents,
                                                             const char *DataSegment, const char *CodeSegment);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperClassTerminate (void* ThisPtr);
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ XCPWrapperClassCommunicate (void* ThisPtr);

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperClassSetIdentificationString (void* ThisPtr, const char* cpId);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperClassSetLogfile (void* ThisPtr, const char* cpLogfile);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperClassXcpEvent (void* ThisPtr, unsigned char ucEvent);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperClassTakeCommunicationPort (void* ThisPtr, unsigned short usPort);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperClassActivateDllAddressTranslation (void* ThisPtr);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperClassDeactivateDllAddressTranslation (void* ThisPtr);
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ XCPWrapperClassSetDaqTimestampUnitAndTicks(void* ThisPtr, unsigned char DaqTimeStampUnit, unsigned char DaqTimestampTicks, unsigned char TimeStampIncOffset);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class cXCPWrapper {
public:
    cXCPWrapper(void) { XCPWrapper = XCPWrapperClassConstructor(); }
    ~cXCPWrapper(void) { XCPWrapperClassDelete(XCPWrapper); };

    void SetNetPort (unsigned short Port) { XCPWrapperClassSetPort(XCPWrapper, Port); }  
    bool Init (unsigned char ucAmEvents, XCP_EVENT_DESC* ptrEvents,
        const char *DataSegment, const char *CodeSegment) { return (XCPWrapperClassInit(XCPWrapper, ucAmEvents, ptrEvents, DataSegment, CodeSegment) != 0); }
    /* Format of DataSegment and CodeSegment strings:
       Each String can contain one or more comma separated linker segment name to A2L segment numbers assignment(s)
       if no number given it will count up beginning with zero. The segment name can also include a process or DLL name.
       Examples:   "mysec"                                      only one segment with name "mysec" and A2L segment number zero
                   "mysec=0"                                    the same as above
                   "mysec_a=0,mysec_b=1"                        two segments with name "mysec_a" and "mysec_b" and the A2L segment numbers 0 and 1
                   "TCU_0.DLL::my_sec=0,TCU_0.DLL::my_sec=1"    two segments with the same name "mysec" but in different DLLs assign to the A2L segment numbers 0 and 1
    */
    void Terminate (void) { XCPWrapperClassTerminate(XCPWrapper); }
    bool Communicate (void) { return (XCPWrapperClassCommunicate(XCPWrapper) != 0); }
    void SetIdentificationString (const char* cpId) { XCPWrapperClassSetIdentificationString(XCPWrapper, cpId); }
    void SetLogfile (const char* cpLogfile) { XCPWrapperClassSetLogfile(XCPWrapper, cpLogfile); }
    void XcpEvent (unsigned char ucEvent) { XCPWrapperClassXcpEvent(XCPWrapper, ucEvent); }
    void TakeCommunicationPort (unsigned short usPort) { XCPWrapperClassTakeCommunicationPort(XCPWrapper, usPort); }
    void ActivateDllAddressTranslation(void) { XCPWrapperClassActivateDllAddressTranslation(XCPWrapper); }
    void DeactivateDllAddressTranslation(void) { XCPWrapperClassDeactivateDllAddressTranslation(XCPWrapper); }
    void SetDaqTimestampUnitAndTicks(unsigned char DaqTimeStampUnit, unsigned char DaqTimestampTicks, unsigned char TimeStampIncOffset)
        {XCPWrapperClassSetDaqTimestampUnitAndTicks(XCPWrapper, DaqTimeStampUnit, DaqTimestampTicks, TimeStampIncOffset); }
private:
    void *XCPWrapper;
};
#endif

#endif
#endif
#endif // XILENVEXTPROC_H
