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


#ifndef EXTPBB_H
#define EXTPBB_H

#include "my_stdint.h"
#include "XilEnvExtProc.h"

#include "ExtpProcessAndTaskInfos.h"

int XilEnvInternal_DataTypeConvert (int dir, int type);

VID XilEnvInternal_add_bbvari_dir(EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo,
                                  const char *name, int type, const char *unit, int dir,
                                  int ValueIsValid, union BB_VARI Value, int AddressIsValid, uint64_t Address,
                                  uint64_t UniqueId,
                                  int *ret_Type);

VID add_bbvari_dir (const char *name, int type, const char *unit, int dir,
                                   int *ret_Type);

EXPORT_OR_IMPORT VID __FUNC_CALL_CONVETION__ add_bbvari (const char *name, int type, const char *unit);

VID add_bbvari_all_infos_dir (const char *name, int type, const char *unit, int dir,
                              int convtype, const char *conversion,
                              double min, double max,
                              int width, int prec,
                              unsigned int rgb_color,
                              int steptype, double step,
                              int *ret_Type);

VID add_bbvari_all_infos (const char *name, int type, const char *unit,
                          int convtype, const char *conversion,
                          double min, double max,
                          int width, int prec,
                          unsigned int rgb_color,
                          int steptype, double step,
                          int *ret_Type);


int XilEnvInternal_remove_bbvari_not_cached(EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo, VID vid);

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ remove_bbvari (VID vid);

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
EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_union (VID vid, union BB_VARI v);

EXPORT_OR_IMPORT void __FUNC_CALL_CONVETION__ write_bbvari_minmax_check (VID vid, double v);

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

EXPORT_OR_IMPORT union BB_VARI __FUNC_CALL_CONVETION__ read_bbvari_union (VID vid);
EXPORT_OR_IMPORT double __FUNC_CALL_CONVETION__ read_bbvari_convert_double (VID vid);

#endif
