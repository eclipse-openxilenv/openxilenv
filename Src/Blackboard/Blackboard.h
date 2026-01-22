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


#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#include <stdint.h>

#include "GlobalDataTypes.h"

#define USE_HASH_BB_SEARCH

/*************************************************************************
**    constants and macros
*************************************************************************/

#define ALL_WRFLAG_MASK         0xFFFFFFFFFFFFFFFFULL

#define BBVARI_NAME_SIZE        512
#define BBVARI_UNIT_SIZE         64
// One INI file line can be max. 131071 char long
#define BBVARI_CONVERSION_SIZE  130000
#define BBVARI_ENUM_NAME_SIZE  512

#define BBVARI_PREFIX_MAX_LEN    32

#define MAX_PROCESSES           64

#define VARI_SECTION            "Variables"

#define BLACKBOARD_ELEM         BB_VARIABLE

#define get_blackboardsize() (blackboard_infos.Size)

#include "SharedDataTypes.h"
#include "BlackboardIniCache.h"

typedef struct _BB_VARIABLE_CONVERSION {
    enum BB_CONV_TYPES Type;
    uint32_t fill1;
    union {
        struct {
            char *EnumString;
        } TextReplace;
        struct {
            char *FormulaString;
            unsigned char *FormulaByteCode;
        } Formula;
        struct {
            double Factor;
            double Offset;
        } FactorOffset;
        struct {
            struct CONVERSION_TABLE_VALUE_PAIR {
                double Raw;
                double Phys;
            } *Values;
            int Size;
        } Table;
        struct {
            double a;
            double b;
            double c;
            double d;
            double e;
            double f;
        } RatFunc;
        struct {
            char *Name;
            struct _BB_VARIABLE_CONVERSION *Reference;
        } Reference;
        /* TODO: ASAP conversion */
    } Conv;
} BB_VARIABLE_CONVERSION;

typedef struct {
/*08*/  char *Name;
/*08*/  char *DisplayName;
/*08*/  char *Unit;
/*08*/  char *Comment;

        BB_VARIABLE_CONVERSION Conversion;

// Attcah counter:
/*04*/  uint32_t UnknownWaitAttachCount;
/*04*/  uint32_t AttachCount;
/*128*/ uint16_t ProcessAttachCounts[MAX_PROCESSES];
/*128*/ uint16_t ProcessUnknownWaitAttachCounts[MAX_PROCESSES];

// Display type:
/*08*/  double Min;
/*08*/  double Max;

/*01*/  uint8_t Width;
/*01*/  uint8_t Prec;
/*01*/  uint8_t StepType;
/*01*/  uint8_t fill1;
/*01*/  uint8_t fill2;
/*01*/  uint8_t fill3;
/*01*/  uint8_t fill4;
/*01*/  uint8_t fill5;
/*08*/  double Step;

/*04*/  int32_t RgbColor;

/*04*/  uint32_t ObservationData;

}  BB_VARIABLE_ADDITIONAL_INFOS;


typedef struct {
/*04*/  int32_t Vid;
/*04*/  enum BB_DATA_TYPES Type;
/*08*/  union BB_VARI Value;
/*08*/  BB_VARIABLE_ADDITIONAL_INFOS *pAdditionalInfos;

// Access flags
/*08*/  volatile uint64_t WrFlags;   // 64Bit -> max. 64 processes
/*08*/  uint64_t AccessFlags;
/*08*/  uint64_t WrEnableFlags;

/*08*/  uint64_t RangeControlFlag;     // For each process one bit

/*04*/  uint32_t ObservationFlags;
#define OBSERVE_CONFIG_ANYTHING_CHANGED  0x7FFFFFFEUL
#define OBSERVE_VALUE_CHANGED            0x1UL
#define OBSERVE_TYPE_CHANGED             0x2UL
#define OBSERVE_CONVERSION_CHANGED       0x4UL
#define OBSERVE_MIN_MAX_CHANGED          0x8UL
#define OBSERVE_UNIT_CHANGED             0x10UL
#define OBSERVE_DISPLAYNAME_CHANGED      0x20UL
#define OBSERVE_COLOR_CHANGED            0x40UL
#define OBSERVE_STEP_CHANGED             0x80UL
#define OBSERVE_FORMAT_CHANGED          0x100UL
#define OBSERVE_CONVERSION_TYPE_CHANGED 0x200UL
#define OBSERVE_REMOVE_VARIABLE         0x20000000UL
#define OBSERVE_ADD_VARIABLE            0x40000000UL
#define OBSERVE_RESET_FLAGS             0x80000000UL

/*04*/  char Fill[4];
}  BB_VARIABLE;  /* 64 bytes */

// Blackboard global infos
typedef struct
{
    int32_t Version;

    int32_t Size;                        // Max. variables inside blackboard
    int32_t VarCount;                    // Variables counter this will only increment each time a variable is added
                                         // The low word will be part of an new VID
    int32_t NumOfVaris;                  // Number of current existing variables
    PID pid_access_masks[MAX_PROCESSES]; // which process own maske
    PID pid_wr_masks[MAX_PROCESSES];     // which process own maske
    PID pid_attachcount[MAX_PROCESSES];  // which process own maske

    uint32_t BarrierHistoryCounter;      // Counts from 0..15
    uint32_t LastBarrierHistoryCounter;

    uint64_t ActualCycleNumber;

    char ProcessBBPrefix[MAX_PROCESSES][BBVARI_PREFIX_MAX_LEN];

    uint32_t ObservationFlags;
    uint32_t ObservationData;
} GLOBAL_BBINFOS;


/*************************************************************************
**    global variables
*************************************************************************/


#ifdef BLACKBOARD_C
BB_VARIABLE *blackboard;
GLOBAL_BBINFOS blackboard_infos;
uint32_t process_bb_access_mask;
#else
extern BB_VARIABLE *blackboard;
extern GLOBAL_BBINFOS blackboard_infos;
extern uint32_t process_bb_access_mask;
#endif


/*************************************************************************
**    function prototypes
*************************************************************************/
void EnterBlackboardCriticalSection(void);
void LeaveBlackboardCriticalSection(void);

int GetDataTypeByteSize (int par_DataType);
char *GetDataTypeName (int par_DataType);
enum BB_DATA_TYPES GetDataTypebyName (const char *par_DataTypeName);

int SetBbvariObservation (VID Vid, uint32_t ObservationFlags, uint32_t ObservationData);
int SetGlobalBlackboradObservation (uint32_t ObservationFlags, uint32_t ObservationData, int32_t **ret_Vids, int32_t *ret_Count);
void SetObserationCallbackFunction (void (*Callback)(VID Vid, uint32_t ObservationFlags, uint32_t ObservationData));

int init_blackboard (int bbvari_count, char CopyBB2ProcOnlyIfWrFlagSet, char AllowBBVarsWithSameAddr, char conv_error2message);
int close_blackboard (void);
int ext_process_close_blackboard (void);
int write_varinfos_to_ini(BB_VARIABLE *sp_vari_elem,
                          uint32_t WriteReqFlags);

int read_varinfos_from_ini (const char *sz_varname,
                            BB_VARIABLE *sp_vari_elem,
                            int type,
                            uint32_t ReadFromIniReqMask);
int get_bb_accessmask(PID pid, uint64_t *mask, char *BBPrefix);
int free_bb_accessmask(PID pid, uint64_t mask);
VID add_bbvari (const char *name, enum BB_DATA_TYPES type, const char *unit);
VID add_bbvari_all_infos (int Pid, const char *name, int type, const char *unit,
                          int convtype, const char *conversion,
                          double min, double max,
                          int width, int prec,
                          int rgb_color,
                          int steptype, double step, int Dir,
                          int ValueValidFlag, union BB_VARI Value,
                          uint64_t Address,
                          int AddressValidFlag,
                          int *ret_RealType);
VID add_bbvari_pid (const char *name, enum BB_DATA_TYPES type, const char *unit, int Pid);
VID add_bbvari_pid_type (const char *name, enum BB_DATA_TYPES type, const char *unit, int Pid, int Dir, int ValueValidFlag, union BB_VARI *Value, int *ret_Type,
                         uint32_t ReadFromIniReqMask, uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation, uint32_t *ret_AddRemoveObservation);

int IfUnknowDataTypeConvert (int UnknwonDataType, int *ret_DataType);
int IfUnknowWaitDataTypeConvert (int UnknwonDataType, int *ret_DataType);

int attach_bbvari (VID vid);
int attach_bbvari_pid (VID vid, int pid);
int attach_bbvari_unknown_wait_pid(VID vid, int pid);
int attach_bbvari_unknown_wait (VID vid);
int attach_bbvari_cs (VID vid, int cs);
int attach_bbvari_unknown_wait_cs (VID vid, int cs);

VID attach_bbvari_by_name (const char *name, int pid);

int remove_bbvari(VID vid);
int remove_bbvari_pid (VID vid, int pid);
int remove_bbvari_unknown_wait (VID vid);
int remove_bbvari_cs (VID vid, int cs);
int remove_bbvari_unknown_wait_cs (VID vid, int cs);
int remove_bbvari_extp (VID vid, PID pid, int Dir, int DataType);
int remove_bbvari_for_process_cs (VID vid, int cs, int pid);
int remove_bbvari_unknown_wait_for_process_cs (VID vid, int cs, int pid);

void free_all_additionl_info_memorys (BB_VARIABLE_ADDITIONAL_INFOS *pAdditionalInfos);

int remove_bbvari_by_name(char *name);
int remove_all_bbvari(PID pid);
unsigned int get_num_of_bbvaris(void);
VID get_bbvarivid_by_name(const char *name);
int get_bbvari_attachcount(VID vid);
int get_bbvaritype_by_name(char *name);
int get_bbvaritype(VID vid);
int GetBlackboardVariableName(VID vid, char *txt, int maxc);
int GetBlackboardVariableNameAndTypes (VID vid, char *txt, int maxc, enum BB_DATA_TYPES *ret_data_type, enum BB_CONV_TYPES *ret_conversion_type);

int get_process_bbvari_attach_count_pid(VID vid, PID pid);
int get_process_bbvari_attach_count(VID vid);

int set_bbvari_unit(VID vid, const char *unit);
int get_bbvari_unit(VID vid, char *unit, int maxc);
int set_bbvari_conversion(VID vid, int convtype, const char *conversion);
struct EXEC_STACK_ELEM;
int set_bbvari_conversion_x(VID vid, int convtype, const char *conversion, int sizeof_exec_stack, struct EXEC_STACK_ELEM *exec_stack,
                            uint32_t *ret_WriteToIniFlag, uint32_t *ret_Observation, uint32_t *ret_AddRemoveObservation);

int get_bbvari_conversion(VID vid, char *conversion, int maxc);
int get_bbvari_conversiontype(VID vid);

int get_bbvari_infos (VID par_Vid, BB_VARIABLE *ret_BaseInfos, BB_VARIABLE_ADDITIONAL_INFOS *ret_AdditionalInfos, char *ret_Buffer, int par_SizeOfBuffer);

int set_bbvari_color(VID vid, int rgb_color);
int get_bbvari_color(VID vid);
int set_bbvari_step(VID vid, unsigned char steptype, double step);
int get_bbvari_step(VID vid, unsigned char *steptype, double *step);
int set_bbvari_min(VID vid, double min);
int set_bbvari_max(VID vid, double max);
double get_bbvari_min(VID vid);
double get_bbvari_max(VID vid);
int set_bbvari_format(VID vid, int width, int prec);
int get_bbvari_format_width(VID vid);
int get_bbvari_format_prec(VID vid);
VID *attach_frame(VID *vids);
void free_frame (VID *frame);
int get_free_wrflag(PID pid, uint64_t *wrflag);
void free_wrflag(PID pid, uint64_t wrflag);
void reset_wrflag(VID vid, uint64_t w);
int test_wrflag(VID vid, uint64_t w);

int read_next_blackboard_vari(int index, char *ret_NameBuffer, int max_c);
char *read_next_blackboard_vari_old (int flag);

int ReadNextVariableProcessAccess(int index, PID pid, int access, char *name, int maxc);
int enable_bbvari_access(PID pid, VID vid);
int disable_bbvari_access(PID pid, VID vid);

int is_bbvari_access_allowed(PID pid, VID vid);

int bbvari_to_string(enum BB_DATA_TYPES type,
                     union BB_VARI value, int base,
                     char *target_string, int maxc);

int string_to_bbvari (enum BB_DATA_TYPES type,
                      union BB_VARI *ret_value,
                      const char *src_string);
int string_to_bbvari_without_type (enum BB_DATA_TYPES *ret_type,
                                   union BB_VARI *ret_value,
                                   const char *src_string);

int double_to_bbvari(  enum BB_DATA_TYPES type,
                              union BB_VARI *bb_var,
                              double value);
int bbvari_to_double( enum BB_DATA_TYPES type,
                      union BB_VARI bb_var,
                      double *value);
int bbvari_to_int64(enum BB_DATA_TYPES type,
                    union BB_VARI bb_var,
                    int64_t *value);

// The index are always the last 6 significant bits of the pid
#define get_process_index(pid) ((pid) & 0x3F)

int get_variable_index(VID vid);
void set_default_varinfo(BB_VARIABLE *sp_vari_elem, int type);
int set_varinfo_to_inientrys( BB_VARIABLE *sp_vari_elem,
                              char *inientrys_str);
uint64_t convert_double2uqword (double value);
int64_t convert_double2qword (double value);
uint32_t convert_double2udword (double value);
int32_t convert_double2dword (double value);
uint16_t convert_double2uword (double value);
short convert_double2word (double value);
unsigned char convert_double2ubyte (double value);
char convert_double2byte (double value);
float convert_double2float (double value);
int equ_src_to_bin( BB_VARIABLE *sp_vari_elem);

int GetBlackboarPrefixForProcess (int Pid, char *BBPrefix, int MaxChar);

int IsValidVariableName (const char *Name);

int enable_bbvari_range_control (char *ProcessFilter, char *VariableFilter);
int disable_bbvari_range_control (char *ProcessFilter, char *VariableFilter);

int BBWriteName(const char* name, BB_VARIABLE *sp_vari_elem);

int BBWriteDisplayName(const char* display_name, BB_VARIABLE *sp_vari_elem);
int BBWriteUnit(const char* unit, BB_VARIABLE *sp_vari_elem);
int BBWriteComment(const char* comment, BB_VARIABLE *sp_vari_elem);
int BBWriteFormulaString(const char* formula_string, BB_VARIABLE *sp_vari_elem);
int BBWriteEnumString(const char* enum_string, BB_VARIABLE *sp_vari_elem);

int get_datatype_min_max_value (int type, double *ret_min, double *ret_max);
int get_datatype_min_max_union (int type, union BB_VARI *ret_min, union BB_VARI *ret_max);


#define GET_ALL_BBVARI_EXISTING_TYPE                                0x1
#define GET_ALL_BBVARI_UNKNOWN_WAIT_TYPE                            0x2
#define GET_ALL_BBVARI_ONLY_FOR_PROCESS_WITH_WRITE_ACCESS           0x4
#define GET_ALL_BBVARI_ONLY_FOR_PROCESS_WITH_READ_ACCESS            0x8
#define GET_ALL_BBVARI_ONLY_FOR_PROCESS                             (GET_ALL_BBVARI_ONLY_FOR_PROCESS_WITH_WRITE_ACCESS | GET_ALL_BBVARI_ONLY_FOR_PROCESS_WITH_READ_ACCESS)

int *get_all_bbvari_ids (uint32_t flag, PID pid, int32_t *ret_ElemCount);
int *get_all_bbvari_ids_without_lock(uint32_t flag, PID pid, int32_t *ret_ElemCount);

void free_all_bbvari_ids (int32_t *ids);

int ConvertLabelAsapCombatibleInOut (const char *In, char *Out, int MaxChars, int op);

int ConvertLabelAsapCombatible (char *InOut,  int MaxChars, int op);

int CmpVariName (const char *Name1, const char *Name2);

void ConvertTo64BitDataType(union BB_VARI par_In, enum BB_DATA_TYPES par_Type,
                            union BB_VARI *ret_Out, enum BB_DATA_TYPES *ret_Type);

union BB_VARI AddOffsetTo(union BB_VARI par_In, enum BB_DATA_TYPES par_Type, double par_Offset);

int LimitToDataTypeRange(union BB_VARI par_In, enum BB_DATA_TYPES par_InType, enum BB_DATA_TYPES par_LimitToType, union BB_VARI *ret_Out);


int BinarySearchIndex(const char *Name, uint64_t HashCode, int32_t *ret_p1, int32_t *ret_p2);

#endif  // BLACKBOARD_H
