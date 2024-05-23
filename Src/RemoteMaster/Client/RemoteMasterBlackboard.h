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


#pragma once
#include "Blackboard.h"
#include "StructsRM_Blackboard.h"

int rm_init_blackboard (int blackboard_size, char CopyBB2ProcOnlyIfWrFlagSet, char AllowBBVarsWithSameAddr, char conv_error2message);

int rm_close_blackboard (void);

VID rm_add_bbvari_pid_type (const char *name, enum BB_DATA_TYPES type, const char *unit, int Pid, int Dir, int ValueValidFlag, union BB_VARI *Value, int *ret_Type, uint32_t ReadReqMask);

int rm_attach_bbvari (VID vid, int unknown_wait_flag, int pid);

VID rm_attach_bbvari_by_name (const char *name, int pid);

int rm_remove_bbvari (VID vid, int unknown_wait_flag, int pid);

int rm_remove_all_bbvari (PID pid);

unsigned int rm_get_num_of_bbvaris (void);

VID rm_get_bbvarivid_by_name (const char *name);

int rm_get_bbvaritype_by_name (const char *name);

int rm_get_bbvari_attachcount (VID vid);

int rm_get_bbvaritype (VID vid);

int rm_GetBlackboardVariableName (VID vid, char *txt, int maxc);

int rm_GetBlackboardVariableNameAndTypes (VID vid, char *txt, int maxc, enum BB_DATA_TYPES *ret_data_type, enum BB_CONV_TYPES *ret_conversion_type);

int rm_get_process_bbvari_attach_count_pid (VID vid, PID pid);

int rm_set_bbvari_unit (VID vid, const char *unit);

int rm_get_bbvari_unit (VID vid, char *unit, int maxc);

int rm_set_bbvari_conversion (VID vid, int convtype, const char *conversion);

int rm_get_bbvari_conversion (VID vid, char *conversion, int maxc);

int rm_get_bbvari_conversiontype (VID vid);

int rm_get_bbvari_infos (VID par_Vid, BB_VARIABLE *ret_BaseInfos, BB_VARIABLE_ADDITIONAL_INFOS *ret_AdditionalInfos, char *ret_Buffer, int par_SizeOfBuffer);

int rm_set_bbvari_color (VID vid, int rgb_color);

int rm_get_bbvari_color(VID vid);

int rm_set_bbvari_step (VID vid, unsigned char steptype, double step);

int rm_get_bbvari_step (VID vid, unsigned char *steptype, double *step);

int rm_set_bbvari_min (VID vid, double min);

int rm_set_bbvari_max (VID vid, double max);

double rm_get_bbvari_min (VID vid);

double rm_get_bbvari_max (VID vid);

int rm_set_bbvari_format (VID vid, int width, int prec);

int rm_get_bbvari_format_width (VID vid);

int rm_get_bbvari_format_prec (VID vid);

int rm_get_free_wrflag (PID pid, uint64_t *wrflag);

void rm_reset_wrflag (VID vid, uint64_t w);

int rm_test_wrflag (VID vid, uint64_t w);

int rm_read_next_blackboard_vari (int index, char *ret_NameBuffer, int max_c);

char *rm_read_next_blackboard_vari_old (int flag);

int rm_ReadNextVariableProcessAccess (int index, PID pid, int access, char *name, int maxc);

int rm_enable_bbvari_access (PID pid, VID vid);

int rm_disable_bbvari_access (PID pid, VID vid);

void rm_SetObserationCallbackFunction (void (*Callback)(VID Vid, uint32_t ObservationFlags, uint32_t ObservationData));
int rm_SetBbvariObservation (VID Vid, uint32_t ObservationFlags, uint32_t ObservationData);
int rm_SetGlobalBlackboradObservation (uint32_t ObservationFlags, uint32_t ObservationData, int **ret_Vids, int *ret_Count);

int rm_get_bb_accessmask (PID pid, uint64_t *mask, char *BBPrefix);

int rm_WriteToBlackboardIniCache(BB_VARIABLE *sp_vari_elem,
                                 uint32_t WriteReqFlags);

int rm_InitVariableSectionCache (void);

int rm_WriteBackVariableSectionCache (void);


// Write/Read Funktionen

void rm_write_bbvari_byte (PID pid, VID vid, int8_t v);

void rm_write_bbvari_ubyte (PID pid, VID vid, uint8_t v);

void rm_write_bbvari_word (PID pid, VID vid, int16_t v);

void rm_write_bbvari_uword (PID pid, VID vid, uint16_t v);

void rm_write_bbvari_dword (PID pid, VID vid, int32_t v);

void rm_write_bbvari_udword (PID pid, VID vid, uint32_t v);

void rm_write_bbvari_qword (PID pid, VID vid, int64_t v);

void rm_write_bbvari_uqword (PID pid, VID vid, uint64_t v);

void rm_write_bbvari_float (PID pid, VID vid, float v);

void rm_write_bbvari_double (PID pid, VID vid, double v);

void rm_write_bbvari_union (PID pid, VID vid, union BB_VARI v);

void rm_write_bbvari_union_pid (PID Pid, VID vid, int DataType, union BB_VARI v);

void rm_write_bbvari_minmax_check (PID Pid, VID vid, double v);

int rm_write_bbvari_convert_to (PID pid, VID vid, int convert_from_type, uint64_t *ret_Ptr);

int rm_write_bbvari_phys_minmax_check (PID pid, VID vid, double new_phys_value);

int rm_get_phys_value_for_raw_value (VID vid, double raw_value, double *ret_phys_value);

int rm_get_raw_value_for_phys_value (VID vid, double phys_value, double *ret_raw_value, double *ret_phys_value);

int8_t rm_read_bbvari_byte (VID vid);

uint8_t rm_read_bbvari_ubyte (VID vid);

int16_t rm_read_bbvari_word (VID vid);

uint16_t rm_read_bbvari_uword (VID vid);

int32_t rm_read_bbvari_dword (VID vid);

uint32_t rm_read_bbvari_udword (VID vid);

int64_t rm_read_bbvari_qword (VID vid);

uint64_t rm_read_bbvari_uqword (VID vid);

float rm_read_bbvari_float (VID vid);

double rm_read_bbvari_double (VID vid);

union BB_VARI rm_read_bbvari_union (VID vid);

enum BB_DATA_TYPES rm_read_bbvari_union_type (VID vid, union BB_VARI *ret_Value);

int rm_read_bbvari_union_type_frame (int Number, VID *Vids, enum BB_DATA_TYPES *ret_Types, union BB_VARI *ret_Values);

double rm_read_bbvari_equ (VID vid);

double rm_read_bbvari_convert_double (VID vid);

int rm_read_bbvari_convert_to (VID vid, int convert_to_type, union BB_VARI *ret_Ptr);

int rm_read_bbvari_textreplace (VID vid, char *txt, int maxc, int *pcolor);

int rm_convert_value_textreplace (VID vid, int32_t value, char *txt, int maxc, int *pcolor);

int rm_convert_textreplace_value (VID vid, char *txt, long *pfrom, long *pto);

int rm_read_bbvari_convert_to_FloatAndInt64 (VID vid, union FloatOrInt64 *ret_Value);

void rm_write_bbvari_convert_with_FloatAndInt64 (PID pid, VID vid, union FloatOrInt64 value, int type);

int rm_read_bbvari_by_name(const char *name, union FloatOrInt64 *ret_value, int *ret_byte_width, int read_type);
int rm_write_bbvari_by_name(PID pid, const char *name, union FloatOrInt64 value, int type, int bb_type, int add_if_not_exist, int read_type);

int rm_read_bbvari_frame (VID *Vids, double *RetFrameValues, int Size);

int rm_write_bbvari_frame (PID pid, VID *Vids, double *FrameValues, int Size);

int rm_req_write_varinfos_to_ini (RM_BLACKBOARD_WRITE_BBVARI_INFOS_REQ *Req);

int rm_req_read_varinfos_from_ini (RM_BLACKBOARD_READ_BBVARI_FROM_INI_REQ *Req);

int rm_RegisterEquation (RM_BLACKBOARD_REGISTER_EQUATION_REQ *Req);

void rm_CallObserationCallbackFunction (RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_REQ *Req);
