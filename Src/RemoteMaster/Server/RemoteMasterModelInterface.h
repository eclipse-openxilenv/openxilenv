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

typedef struct {
/*08*/  void *ptr;
/*04*/  int vid;
/*04*/  unsigned int flags;
#define REF_ONLY_READ_FLAG   1
#define REF_ONLY_WRITE_FLAG  2
#define REF_READ_WRITE_FLAG  3
/*08*/  union BB_VARI overwrite_value;
/*04*/  unsigned int overwrite_flag;
/*04*/  unsigned int size;   // damit Strukturgrsse 2^n
} EGS_BBVARI_REFLIST;

typedef struct {
    double min;
    double max;
    int width;
    int prec;
    int steptype;
    double step;
    unsigned int rgb_color;
    int convtype;
    unsigned char conversion[2 * BBVARI_CONVERSION_SIZE];
} EGS_BBVARI_PRE_REFLIST_ADDITIONAL_INFOS;

typedef struct {
    char Name[BBVARI_NAME_SIZE];
    char Unit[BBVARI_UNIT_SIZE];
    void *ptr;
    int EnUnit;
    int BBDType;

    int all_infos;
    EGS_BBVARI_PRE_REFLIST_ADDITIONAL_INFOS *additional_infos;
} EGS_BBVARI_PRE_REFLIST;


int check_bbvari_ref(void *address, unsigned char *bytes, int len);

int dereference_var(void *ptr, int vid);

void referece_error(const char *name, int vid, int bb_dtype, const char *unit, void *ptr, int flag,
    EGS_BBVARI_PRE_REFLIST_ADDITIONAL_INFOS *ai, unsigned int Flags);
void add_vari_ref_list(void *ptr, int vid, unsigned int Flags);

void reference_double_var(double *ptr, const char *name);
void reference_float_var(float *ptr, const char *name);
void reference_dword_var(int *ptr, const char *name);
void reference_udword_var(unsigned int *ptr, const char *name);
void reference_word_var(short *ptr, const char *name);
void reference_uword_var(unsigned short *ptr, const char *name);
void reference_byte_var(signed char *ptr, const char *name);
void reference_ubyte_var(unsigned char *ptr, const char *name);
void reference_double_var_unit(double *ptr, const char *name, const char *unit);
void reference_float_var_unit(float *ptr, const char *name, const char *unit);
void reference_dword_var_unit(int *ptr, const char *name, const char *unit);
void reference_udword_var_unit(unsigned int *ptr, const char *name, const char *unit);
void reference_word_var_unit(short *ptr, const char *name, const char *unit);
void reference_uword_var_unit(unsigned short *ptr, const char *name, const char *unit);
void reference_byte_var_unit(signed char *ptr, const char *name, const char *unit);
void reference_ubyte_var_unit(unsigned char *ptr, const char *name, const char *unit);

void copy_vari_egs_bb(void);

void copy_vari_bb_egs(void);

int TerminateConnectionAndRemoveAllVariables(void);

#include "SetModelNameAndFunctionPointers.h"

/* This is now inside it own header
typedef struct {
    void(*reference_varis) (void);              // Reference-Funk.                                  
    int(*init_test_object) (void);              // Init-Funk.                                  
    void(*cyclic_test_object) (void);           // Zyklische Funk.                             
    void(*terminate_test_object) (void);         // Beenden-Funk.                               
} MODEL_FUNCTION_POINTERS;

void SetModelNameAndFunctionPointers(const char *par_Name, int par_Prio, MODEL_FUNCTION_POINTERS *par_FunctionPointers);
*/
