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
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <setjmp.h>

#define WITH_EXIT_FUNCTION

#include "MyMemory.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "RealtimeScheduler.h"
#include "ThrowError.h"
#include "StructsRM_Blackboard.h"  // ony for READ_ALL_INFOS_BBVARI_FROM_INI

#ifdef WITH_CPP_CONSTRUCTORS
#include "cpp_startup.h"
#endif

#include "RemoteMasterModelInterface.h"

//#define RUNTIME_MEASUREMENT
// The cache is not a speed improvement!?
//#define PREFETCH_BLACKBOARD_TO_CACHE
#define PREFETCH_BLACKBOARD_TO_CACHE_PRE   2

#define UNUSED(x) (void)(x)

extern int GetLabelAddressFromDbgInfo(void *Address, char *Name, int maxc);

int GetLabelAddressFromDbgInfo(void *Address, char *Name, int maxc)
{
    UNUSED(Address);
    UNUSED(maxc);
    strcpy(Name, "todo");
    return 0;
}

static EGS_BBVARI_REFLIST *egs_bbvari_refarray;
static int egs_bbvari_count;
static int egs_bbvari_size = 64;


#define SIZE_OF_WRITE_BYTES_FIFO 1024

unsigned char FiFoBufferForWriteLen[SIZE_OF_WRITE_BYTES_FIFO];  // Count of the bytes
void *FiFoBufferForWriteAddress[SIZE_OF_WRITE_BYTES_FIFO];
uint64_t FiFoBufferForWriteBytes[SIZE_OF_WRITE_BYTES_FIFO];

volatile int WrPosFiFoBufferForWriteBytes;
volatile int RdPosFiFoBufferForWriteBytes;


static int check_bbvari_ref_collect(void *address, unsigned char *bytes, int len)
{
    int NextWr;

    if (len > 8) {
        ThrowError(1, "cannot write more as 8 bytes at once (%i)", len);
        return -1;
    }
    NextWr = WrPosFiFoBufferForWriteBytes + 1;
    if (NextWr >= SIZE_OF_WRITE_BYTES_FIFO) {
        NextWr = 0;
    }
    while (NextWr == RdPosFiFoBufferForWriteBytes) {
        usleep(1000); // warte mal 1ms
        //error (1, "fifo buffer full"); keine Fehlermeldung da dies bei SVL-Dateien mit mehr als 1024 Zeilen vorkommt!
        //return -2;
    }
    // erst die Daten,
    memcpy(&(FiFoBufferForWriteBytes[WrPosFiFoBufferForWriteBytes]), bytes, len);
    // Dann die Adresse
    FiFoBufferForWriteAddress[WrPosFiFoBufferForWriteBytes] = address;
    // als letztes die Anzahl Bytes (1...8)
    FiFoBufferForWriteLen[WrPosFiFoBufferForWriteBytes] = (unsigned char)len;

    WrPosFiFoBufferForWriteBytes = NextWr;

    return len;
}

void transfer_write_bytes_at_end_of_cycles(void)
{
    while (FiFoBufferForWriteLen[RdPosFiFoBufferForWriteBytes] != 0) {
        //error(1, "write");
        memcpy(FiFoBufferForWriteAddress[RdPosFiFoBufferForWriteBytes], &(FiFoBufferForWriteBytes[RdPosFiFoBufferForWriteBytes]), FiFoBufferForWriteLen[RdPosFiFoBufferForWriteBytes]);
        FiFoBufferForWriteLen[RdPosFiFoBufferForWriteBytes] = 0;
        if (RdPosFiFoBufferForWriteBytes < (SIZE_OF_WRITE_BYTES_FIFO - 1)) {
            RdPosFiFoBufferForWriteBytes++;
        }
        else {
            RdPosFiFoBufferForWriteBytes = 0;
        }
    }
}


int check_bbvari_ref(void *address, unsigned char *bytes, int len)
{
    int x;
    unsigned char *src_from, *src_to;

    src_from = (unsigned char*)address;
    src_to = src_from + len;

    if (egs_bbvari_refarray != NULL) {
        for (x = 0; x < egs_bbvari_size; x++) {
            int type;
            int size;
            unsigned char *ref_from, *ref_to;

            ref_from = (unsigned char*)egs_bbvari_refarray[x].ptr;
            ref_to = ref_from + egs_bbvari_refarray[x].size;

            if ((ref_from < src_to) && (ref_to > src_from)) {
                unsigned char *max_from, *min_to;
                int offset;
                max_from = (ref_from > src_from) ? ref_from : src_from;
                min_to = (ref_to < src_to) ? ref_to : src_to;
                offset = max_from - src_from;
                check_bbvari_ref_collect(max_from, bytes + offset, (int)(min_to - max_from));
            }
        }
    }
    return 0;
}


static void DbgPrintRefList(void)
{
    int x;
    ThrowError(1, "RefList =%i", egs_bbvari_size);
    if (egs_bbvari_refarray != NULL) {
        for (x = 0; x < egs_bbvari_size; x++) {
            if (egs_bbvari_refarray[x].ptr != NULL) {
                ThrowError(1, "%i. %p 0x%X", x, egs_bbvari_refarray[x].ptr, egs_bbvari_refarray[x].vid);
            }
        }
    }
}

void add_vari_ref_list(void *ptr, int vid, unsigned int Flags)
{
    int x;

    //DbgPrintRefList();
    if (egs_bbvari_refarray == NULL) {
        if ((egs_bbvari_refarray = (EGS_BBVARI_REFLIST*)my_calloc(egs_bbvari_size, sizeof(EGS_BBVARI_REFLIST))) == NULL) {
            ThrowError(1, "out of Memmory");
            return;
        }
    }
    while (1) {
        for (x = 0; x < egs_bbvari_size; x++) {
            if (egs_bbvari_refarray[x].ptr == NULL) {
                int type;
                egs_bbvari_refarray[x].vid = vid;
                egs_bbvari_refarray[x].ptr = ptr;
                egs_bbvari_refarray[x].flags = Flags;
                egs_bbvari_refarray[x].overwrite_flag = 0;
                egs_bbvari_refarray[x].overwrite_value.uqw = 0;
                type = get_bbvaritype(vid);
                switch (type) {
                case BB_BYTE:
                case BB_UBYTE:
                    egs_bbvari_refarray[x].size = 1;
                    break;
                case BB_WORD:
                case BB_UWORD:
                    egs_bbvari_refarray[x].size = 2;
                    break;
                case BB_DWORD:
                case BB_UDWORD:
                case BB_FLOAT:
                    egs_bbvari_refarray[x].size = 4;
                    break;
                case BB_DOUBLE:
                case BB_QWORD:
                case BB_UQWORD:
                    egs_bbvari_refarray[x].size = 8;
                    break;
                default:
                    egs_bbvari_refarray[x].size = 1;
                }
                egs_bbvari_count++;
                return;
            }
        }
        // If no free element add 64 new elements
        egs_bbvari_size += 64;
        if ((egs_bbvari_refarray = (EGS_BBVARI_REFLIST*)my_realloc(egs_bbvari_refarray, egs_bbvari_size * sizeof(EGS_BBVARI_REFLIST))) == NULL) {
            ThrowError(1, "out of Memmory");
            return;
        }
        for (x = egs_bbvari_size - 64; x < egs_bbvari_size; x++) {
            egs_bbvari_refarray[x].vid = 0;
            egs_bbvari_refarray[x].ptr = NULL;
            egs_bbvari_refarray[x].flags = 0;
            egs_bbvari_refarray[x].overwrite_flag = 0;
            egs_bbvari_refarray[x].overwrite_value.uqw = 0;
            egs_bbvari_refarray[x].size = 0;
        }
    }
}

int dereference_var(void *ptr, int vid)
{
    int x;

    if (egs_bbvari_refarray != NULL) {
        for (x = 0; x < egs_bbvari_size; x++) {
            if ((egs_bbvari_refarray[x].ptr == ptr) &&
                (egs_bbvari_refarray[x].vid == vid)) {
                remove_bbvari(egs_bbvari_refarray[x].vid);
                egs_bbvari_refarray[x].ptr = NULL;
                egs_bbvari_refarray[x].vid = -1;
                egs_bbvari_refarray[x].flags = 0;
                egs_bbvari_refarray[x].overwrite_flag = 0;
                egs_bbvari_refarray[x].overwrite_value.uqw = 0;
                egs_bbvari_refarray[x].size = 0;
                return 0;  // OK
            }
        }
    }
    ThrowError(1, "dereference unknown variable vid = %li adr = %p", vid, ptr);
    return -1;
}


void referece_error(const char *name, int vid, int bb_dtype, const char *unit, void *ptr, int flag,
                    EGS_BBVARI_PRE_REFLIST_ADDITIONAL_INFOS *ai, unsigned int Flags)
{
    ThrowError(1, "cannot reference variable %s", name);
}

#define CHECK_NAME_NULL_PTR \
    char Help[512]; \
    if (name == NULL) { \
        if (GetLabelAddressFromDbgInfo (ptr, Help, sizeof (Help))) { \
            return; \
        } \
        name = Help; \
    } 

// double
void reference_double_var_flags(double *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;

    CHECK_NAME_NULL_PTR
    if ((vid = add_bbvari_pid_type(name, BB_DOUBLE, unit, -1, flags, 1, (union BB_VARI*)ptr, NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL)) <= 0)
        referece_error(name, vid, BB_DOUBLE, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
    }
}

void reference_double_var(double *ptr, const char *name)
{
    reference_double_var_flags(ptr, name, NULL, REF_READ_WRITE_FLAG);
}

void reference_double_var_unit(double *ptr, const char *name, const char *unit)
{
    reference_double_var_flags(ptr, name, unit, REF_READ_WRITE_FLAG);
}

// float
void reference_float_var_flags(float *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;

    CHECK_NAME_NULL_PTR
    if ((vid = add_bbvari_pid_type(name, BB_FLOAT, unit, -1, flags, 1, (union BB_VARI*)ptr, NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL)) <= 0)
        referece_error(name, vid, BB_FLOAT, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
    }
}

void reference_float_var(float *ptr, const char *name)
{
    reference_float_var_flags(ptr, name, NULL, REF_READ_WRITE_FLAG);
}

void reference_float_var_unit(float *ptr, const char *name, const char *unit)
{
    reference_float_var_flags(ptr, name, unit, REF_READ_WRITE_FLAG);
}

// qword
void reference_qword_var_flags(int64_t *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;

    CHECK_NAME_NULL_PTR
    if ((vid = add_bbvari_pid_type(name, BB_QWORD, unit, -1, flags, 1, (union BB_VARI*)ptr, NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL)) <= 0)
        referece_error(name, vid, BB_QWORD, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
    }
}

void reference_qword_var(int64_t *ptr, const char *name)
{
    reference_qword_var_flags(ptr, name, NULL, REF_READ_WRITE_FLAG);
}

void reference_qword_var_unit(int64_t *ptr, const char *name, const char *unit)
{
    reference_qword_var_flags(ptr, name, unit, REF_READ_WRITE_FLAG);
}

// uqword
void reference_uqword_var_flags(uint64_t *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;

    CHECK_NAME_NULL_PTR
    if ((vid = add_bbvari_pid_type(name, BB_UQWORD, unit, -1, flags, 1, (union BB_VARI*)ptr, NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL)) <= 0)
        referece_error(name, vid, BB_UQWORD, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
    }
}

void reference_uqword_var(uint64_t *ptr, const char *name)
{
    reference_uqword_var_flags(ptr, name, NULL, REF_READ_WRITE_FLAG);
}

void reference_uqword_var_unit(uint64_t *ptr, const char *name, const char *unit)
{
    reference_uqword_var_flags(ptr, name, unit, REF_READ_WRITE_FLAG);
}

// dword
void reference_dword_var_flags(int *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;

    CHECK_NAME_NULL_PTR
    if ((vid = add_bbvari_pid_type(name, BB_DWORD, unit, -1, flags, 1, (union BB_VARI*)ptr, NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL)) <= 0)
        referece_error(name, vid, BB_DWORD, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
    }
}

void reference_dword_var(int *ptr, const char *name)
{
    reference_dword_var_flags(ptr, name, NULL, REF_READ_WRITE_FLAG);
}

void reference_dword_var_unit(int *ptr, const char *name, const char *unit)
{
    reference_dword_var_flags(ptr, name, unit, REF_READ_WRITE_FLAG);
}

// udword
void reference_udword_var_flags(unsigned int *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;

    CHECK_NAME_NULL_PTR
    if ((vid = add_bbvari_pid_type(name, BB_UDWORD, unit, -1, flags, 1, (union BB_VARI*)ptr, NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL)) <= 0)
        referece_error(name, vid, BB_UDWORD, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
    }
}

void reference_udword_var(unsigned int *ptr, const char *name)
{
    reference_udword_var_flags(ptr, name, NULL, REF_READ_WRITE_FLAG);
}

void reference_udword_var_unit(unsigned int *ptr, const char *name, const char *unit)
{
    reference_udword_var_flags(ptr, name, unit, REF_READ_WRITE_FLAG);
}

// word
void reference_word_var_flags(short *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;

    CHECK_NAME_NULL_PTR
    if ((vid = add_bbvari_pid_type(name, BB_WORD, unit, -1, flags, 1, (union BB_VARI*)ptr, NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL)) <= 0)
        referece_error(name, vid, BB_WORD, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
    }
}

void reference_word_var(short *ptr, const char *name)
{
    reference_word_var_flags(ptr, name, NULL, REF_READ_WRITE_FLAG);
}

void reference_word_var_unit(short *ptr, const char *name, const char *unit)
{
    reference_word_var_flags(ptr, name, unit, REF_READ_WRITE_FLAG);
}

// uword
void reference_uword_var_flags(unsigned short *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;

    CHECK_NAME_NULL_PTR
    if ((vid = add_bbvari_pid_type(name, BB_UWORD, unit, -1, flags, 1, (union BB_VARI*)ptr, NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL)) <= 0)
        referece_error(name, vid, BB_UWORD, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
    }
}

void reference_uword_var(unsigned short *ptr, const char *name)
{
    reference_uword_var_flags(ptr, name, NULL, REF_READ_WRITE_FLAG);
}

void reference_uword_var_unit(unsigned short *ptr, const char *name, const char *unit)
{
    reference_uword_var_flags(ptr, name, unit, REF_READ_WRITE_FLAG);
}

// byte
void reference_byte_var_flags(signed char *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;

    CHECK_NAME_NULL_PTR
    if ((vid = add_bbvari_pid_type(name, BB_BYTE, unit, -1, flags, 1, (union BB_VARI*)ptr, NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL)) <= 0)
        referece_error(name, vid, BB_BYTE, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
    }
}

void reference_byte_var(signed char *ptr, const char *name)
{
    reference_byte_var_flags(ptr, name, NULL, REF_READ_WRITE_FLAG);
}

void reference_byte_var_unit(signed char *ptr, const char *name, const char *unit)
{
    reference_byte_var_flags(ptr, name, unit, REF_READ_WRITE_FLAG);
}

// ubyte
void reference_ubyte_var_flags(unsigned char *ptr, const char *name, const char *unit, unsigned int flags)
{
    int vid;

    CHECK_NAME_NULL_PTR
    if ((vid = add_bbvari_pid_type(name, BB_UBYTE, unit, -1, flags, 1, (union BB_VARI*)ptr, NULL, READ_ALL_INFOS_BBVARI_FROM_INI, NULL, NULL, NULL)) <= 0)
        referece_error(name, vid, BB_UBYTE, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
    }
}

void reference_ubyte_var(unsigned char *ptr, const char *name)
{
    reference_ubyte_var_flags(ptr, name, NULL, REF_READ_WRITE_FLAG);
}

void reference_ubyte_var_unit(unsigned char *ptr, const char *name, const char *unit)
{
    reference_ubyte_var_flags(ptr, name, unit, REF_READ_WRITE_FLAG);
}


/* All Infos */
// double
void reference_double_var_all_infos_flags(double *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step,
    unsigned int flags)
{
    int vid;
    union BB_VARI Value;
    int RealType;

    CHECK_NAME_NULL_PTR
    
    Value.d = *ptr;
    if ((vid = add_bbvari_all_infos(GET_PID(), name, 0x1000 | BB_DOUBLE, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step,
        flags, 1, Value,
        (uint64_t)ptr, 1, &RealType)) <= 0)
        referece_error(name, vid, BB_DOUBLE, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
        if ((flags & REF_ONLY_WRITE_FLAG) == REF_ONLY_WRITE_FLAG) write_bbvari_double(vid, *ptr);
    }
}

void reference_double_var_all_infos(double *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step)
{
    reference_double_var_all_infos_flags(ptr, name, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step,
        REF_READ_WRITE_FLAG);
}

// float
void reference_float_var_all_infos_flags(float *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step,
    unsigned int flags)
{
    int vid;
    union BB_VARI Value;
    int RealType;

    CHECK_NAME_NULL_PTR
    
    Value.f = *ptr;
    if ((vid = add_bbvari_all_infos(GET_PID(), name, 0x1000 | BB_FLOAT, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step, flags, 1, Value,
        (uint64_t)ptr, 1, &RealType)) <= 0)
        referece_error(name, vid, BB_FLOAT, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
        if ((flags & REF_ONLY_WRITE_FLAG) == REF_ONLY_WRITE_FLAG) write_bbvari_float(vid, *ptr);
    }
}

void reference_float_var_all_infos(float *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step)
{
    reference_float_var_all_infos_flags(ptr, name, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step,
        REF_READ_WRITE_FLAG);
}

// dword
void  reference_dword_var_all_infos_flags(int32_t *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step,
    unsigned int flags)
{
    int vid;
    union BB_VARI Value;
    int RealType;

    CHECK_NAME_NULL_PTR

    Value.dw = *ptr;
    if ((vid = add_bbvari_all_infos(GET_PID(), name, 0x1000 | BB_DWORD, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step, flags, 1, Value,
        (uint64_t)ptr, 1, &RealType)) <= 0)
        referece_error(name, vid, BB_DWORD, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
        if ((flags & REF_ONLY_WRITE_FLAG) == REF_ONLY_WRITE_FLAG) write_bbvari_dword(vid, *ptr);
    }
}

void reference_dword_var_all_infos(int32_t *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step)
{
    reference_dword_var_all_infos_flags(ptr, name, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step,
        REF_READ_WRITE_FLAG);
}

// udword
void  reference_udword_var_all_infos_flags(uint32_t *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step,
    unsigned int flags)
{
    int vid;
    union BB_VARI Value;
    int RealType;

    CHECK_NAME_NULL_PTR
    
    Value.udw = *ptr;
    if ((vid = add_bbvari_all_infos(GET_PID(), name, 0x1000 | BB_UDWORD, unit,
            convtype, conversion,
            min, max,
            width, prec,
            rgb_color,
            steptype, step, flags, 1, Value,
            (uint64_t)ptr, 1, &RealType)) <= 0)
            referece_error(name, vid, BB_UDWORD, unit, ptr, 0, NULL, flags);
        else {
            add_vari_ref_list((void *)ptr, vid, flags);
            if ((flags & REF_ONLY_WRITE_FLAG) == REF_ONLY_WRITE_FLAG) write_bbvari_udword(vid, *ptr);
        }
}

void reference_udword_var_all_infos(uint32_t *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step)
{
    reference_udword_var_all_infos_flags(ptr, name, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step,
        REF_READ_WRITE_FLAG);
}

// qword
void  reference_qword_var_all_infos_flags(int64_t *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step,
    unsigned int flags)
{
    int vid;
    union BB_VARI Value;
    int RealType;

    CHECK_NAME_NULL_PTR

    Value.qw = *ptr;
    if ((vid = add_bbvari_all_infos(GET_PID(), name, 0x1000 | BB_QWORD, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step, flags, 1, Value,
        (uint64_t)ptr, 1, &RealType)) <= 0)
        referece_error(name, vid, BB_QWORD, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
        if ((flags & REF_ONLY_WRITE_FLAG) == REF_ONLY_WRITE_FLAG) write_bbvari_qword(vid, *ptr);
    }
}

void reference_qword_var_all_infos(int64_t *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step)
{
    reference_qword_var_all_infos_flags(ptr, name, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step,
        REF_READ_WRITE_FLAG);
}

// uqword
void  reference_uqword_var_all_infos_flags(uint64_t *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step,
    unsigned int flags)
{
    int vid;
    union BB_VARI Value;
    int RealType;

    CHECK_NAME_NULL_PTR

    Value.uqw = *ptr;
    if ((vid = add_bbvari_all_infos(GET_PID(), name, 0x1000 | BB_UQWORD, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step, flags, 1, Value,
        (uint64_t)ptr, 1, &RealType)) <= 0)
        referece_error(name, vid, BB_UQWORD, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
        if ((flags & REF_ONLY_WRITE_FLAG) == REF_ONLY_WRITE_FLAG) write_bbvari_uqword(vid, *ptr);
    }
}

void reference_uqword_var_all_infos(uint64_t *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step)
{
    reference_uqword_var_all_infos_flags(ptr, name, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step,
        REF_READ_WRITE_FLAG);
}

// word 
void  reference_word_var_all_infos_flags(short *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step,
    unsigned int flags)
{
    int vid;
    union BB_VARI Value;
    int RealType;

    CHECK_NAME_NULL_PTR

    Value.w = *ptr;

    if ((vid = add_bbvari_all_infos(GET_PID(), name, 0x1000 | BB_WORD, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step, flags, 1, Value,
        (uint64_t)ptr, 1, &RealType)) <= 0)
        referece_error(name, vid, BB_WORD, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
        if ((flags & REF_ONLY_WRITE_FLAG) == REF_ONLY_WRITE_FLAG) write_bbvari_word(vid, *ptr);
    }
}

void reference_word_var_all_infos(short *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step)
{
    reference_word_var_all_infos_flags(ptr, name, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step,
        REF_READ_WRITE_FLAG);
}

// uword
void  reference_uword_var_all_infos_flags(unsigned short *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step,
    unsigned int flags)
{
    int vid;
    union BB_VARI Value;
    int RealType;

    CHECK_NAME_NULL_PTR
    
    Value.uw = *ptr;
    if ((vid = add_bbvari_all_infos(GET_PID(), name, 0x1000 | BB_UWORD, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step, flags, 1, Value,
        (uint64_t)ptr, 1, &RealType)) <= 0)
        referece_error(name, vid, BB_UWORD, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
        if ((flags & REF_ONLY_WRITE_FLAG) == REF_ONLY_WRITE_FLAG) write_bbvari_uword(vid, *ptr);
    }
}

void reference_uword_var_all_infos(unsigned short *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step)
{
    reference_uword_var_all_infos_flags(ptr, name, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step,
        REF_READ_WRITE_FLAG);
}

// byte
void  reference_byte_var_all_infos_flags(signed char *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step,
    unsigned int flags)
{
    int vid;
    union BB_VARI Value;
    int RealType;

    CHECK_NAME_NULL_PTR

    Value.b = *ptr;

    if ((vid = add_bbvari_all_infos(GET_PID(), name, 0x1000 | BB_BYTE, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step, flags, 1, Value,
        (uint64_t)ptr, 1, &RealType)) <= 0)
        referece_error(name, vid, BB_BYTE, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
        if ((flags & REF_ONLY_WRITE_FLAG) == REF_ONLY_WRITE_FLAG) write_bbvari_byte(vid, *ptr);
    }
}

void reference_byte_var_all_infos(signed char *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step)
{
    reference_byte_var_all_infos_flags(ptr, name, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step,
        REF_READ_WRITE_FLAG);
}

void  reference_ubyte_var_all_infos_flags(unsigned char *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step,
    unsigned int flags)
{
    int vid;
    union BB_VARI Value;
    int RealType;

    CHECK_NAME_NULL_PTR

    Value.ub = *ptr;

    if ((vid = add_bbvari_all_infos(GET_PID(), name, 0x1000 | BB_UBYTE, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step, flags, 1, Value,
        (uint64_t)ptr, 1, &RealType)) <= 0)
        referece_error(name, vid, BB_UBYTE, unit, ptr, 0, NULL, flags);
    else {
        add_vari_ref_list((void *)ptr, vid, flags);
        if ((flags & REF_ONLY_WRITE_FLAG) == REF_ONLY_WRITE_FLAG) write_bbvari_ubyte(vid, *ptr);
    }
}

void reference_ubyte_var_all_infos(unsigned char *ptr, const char *name, const char *unit,
    int convtype, const char *conversion,
    double min, double max,
    int width, int prec,
    unsigned int rgb_color,
    int steptype, double step)
{
    reference_ubyte_var_all_infos_flags(ptr, name, unit,
        convtype, conversion,
        min, max,
        width, prec,
        rgb_color,
        steptype, step,
        REF_READ_WRITE_FLAG);
}

// #define RUNTIME_MEASUREMENT
#ifdef RUNTIME_MEASUREMENT
double Runtime_copy_vari_egs_bb;
static double Runtime_copy_vari_egs_bb_x;
double Runtime_copy_vari_bb_egs;
#endif

void copy_vari_egs_bb(void)
{
    int x;
    int vid_index;
    BB_VARIABLE *pvari;
    uint64_t wrflag;
#ifdef RUNTIME_MEASUREMENT
    uint64_t Start;
    Runtime_copy_vari_egs_bb = Runtime_copy_vari_egs_bb_x;
    Start = MyTimeStamp();
#endif
    //if (wrflag == 0) get_free_wrflag (extp_pid, &wrflag);
    if (egs_bbvari_refarray != NULL) {   // es wurden keine Variablen referenziert
        get_free_wrflag(GET_PID(), &wrflag);
        if (1) { // (pid_index = get_process_index(process_identifier)) != -1) {
            EnterBlackboardCriticalSection();
            for (x = 0; x < egs_bbvari_size; x++) {
#ifdef PREFETCH_BLACKBOARD_TO_CACHE
                // prefatch in Cache
                if ((x + PREFETCH_BLACKBOARD_TO_CACHE_PRE) < egs_bbvari_size) {
                    if (egs_bbvari_refarray[x + PREFETCH_BLACKBOARD_TO_CACHE_PRE].ptr != NULL) {
                        vid_index = (int)(egs_bbvari_refarray[x + PREFETCH_BLACKBOARD_TO_CACHE_PRE].vid >> 16);
                        pvari = &blackboard[vid_index].varis;
                        _mm_prefetch(pvari, 1/*_MM_HINT_T0*/);

                    }
                }
#endif
                if (egs_bbvari_refarray[x].ptr != NULL) {
                    vid_index = (int)(egs_bbvari_refarray[x].vid >> 8);
                    pvari = &blackboard[vid_index];
                    if (pvari->Vid == egs_bbvari_refarray[x].vid) {
                        if (pvari->AccessFlags &
                            pvari->WrEnableFlags & wrflag) {
                            // Only copy if an oter process has written
                            if ((pvari->WrFlags & wrflag) == 0) {
                                if ((egs_bbvari_refarray[x].flags & REF_ONLY_WRITE_FLAG) == REF_ONLY_WRITE_FLAG) {
                                    switch (pvari->Type) {
                                    case BB_BYTE:
                                        pvari->Value.b = *(int8_t*)egs_bbvari_refarray[x].ptr;
                                        break;
                                    case BB_UBYTE:
                                        pvari->Value.ub = *(uint8_t*)egs_bbvari_refarray[x].ptr;
                                        break;
                                    case BB_WORD:
                                        pvari->Value.w = *(int16_t*)egs_bbvari_refarray[x].ptr;
                                        break;
                                    case BB_UWORD:
                                        pvari->Value.uw = *(uint16_t*)egs_bbvari_refarray[x].ptr;
                                        break;
                                    case BB_DWORD:
                                        pvari->Value.dw = *(int32_t*)egs_bbvari_refarray[x].ptr;
                                        break;
                                    case BB_UDWORD:
                                        pvari->Value.udw = *(uint32_t*)egs_bbvari_refarray[x].ptr;
                                        break;
                                    case BB_FLOAT:
                                        pvari->Value.f = *(float*)egs_bbvari_refarray[x].ptr;
                                        break;
                                    case BB_DOUBLE:
                                        pvari->Value.d = *(double*)egs_bbvari_refarray[x].ptr;
                                        break;
                                    }
                                    pvari->WrFlags = ALL_WRFLAG_MASK;
                                }
                                else {
                                    pvari->WrFlags = ALL_WRFLAG_MASK;
                                }
                            }
                        }
                    }
                }
            }
            LeaveBlackboardCriticalSection();
        }
    }
#ifdef RUNTIME_MEASUREMENT
    Runtime_copy_vari_egs_bb_x = (uint64_t)(MyTimeStamp() - Start);
#endif
}


void copy_vari_bb_egs(void)
{
    int x;
    int vid_index, pid_index;
    BB_VARIABLE *pvari;
    uint64_t wrflag, wrflag_neg;
#ifdef RUNTIME_MEASUREMENT
    uint64_t Start;
    Start = MyTimeStamp();
#endif
    if (egs_bbvari_refarray != NULL) {   // There are no referenced variables
        get_free_wrflag(GET_PID(), &wrflag);

        if (1) {
            wrflag_neg = ~wrflag;
            EnterBlackboardCriticalSection();
            for (x = 0; x < egs_bbvari_size; x++) {
#ifdef PREFETCH_BLACKBOARD_TO_CACHE
                // prefatch in Cache
                if ((x + PREFETCH_BLACKBOARD_TO_CACHE_PRE) < egs_bbvari_size) {
                    if (egs_bbvari_refarray[x + PREFETCH_BLACKBOARD_TO_CACHE_PRE].ptr != NULL) {
                        vid_index = (int)(egs_bbvari_refarray[x + PREFETCH_BLACKBOARD_TO_CACHE_PRE].vid >> 16);
                        pvari = &blackboard[vid_index].varis;
                        _mm_prefetch(pvari, 1/*_MM_HINT_T0*/);

                    }
                }
#endif
                if (egs_bbvari_refarray[x].ptr != NULL) {
                    vid_index = (int)(egs_bbvari_refarray[x].vid >> 8);
                    pvari = &blackboard[vid_index];
                    // Vid match
                    if (pvari->Vid == egs_bbvari_refarray[x].vid) {
                        if ((egs_bbvari_refarray[x].flags & REF_ONLY_READ_FLAG) == REF_ONLY_READ_FLAG) {
                            // Reset own WR flag to check if an other process has write to the variable meanwhile
                            pvari->WrFlags &= wrflag_neg;
                            switch (pvari->Type) {
                            case BB_BYTE:
                                *(int8_t*)egs_bbvari_refarray[x].ptr = pvari->Value.b;
                                break;
                            case BB_UBYTE:
                                *(uint8_t*)egs_bbvari_refarray[x].ptr = pvari->Value.ub;
                                break;
                            case BB_WORD:
                                *(int16_t*)egs_bbvari_refarray[x].ptr = pvari->Value.w;
                                break;
                            case BB_UWORD:
                                *(uint16_t*)egs_bbvari_refarray[x].ptr = pvari->Value.uw;
                                break;
                            case BB_DWORD:
                                *(int32_t*)egs_bbvari_refarray[x].ptr = pvari->Value.dw;
                                break;
                            case BB_UDWORD:
                                *(uint32_t*)egs_bbvari_refarray[x].ptr = pvari->Value.udw;
                                break;
                            case BB_FLOAT:
                                *(float*)egs_bbvari_refarray[x].ptr = pvari->Value.f;
                                break;
                            case BB_DOUBLE:
                                *(double*)egs_bbvari_refarray[x].ptr = pvari->Value.d;
                                break;
                            }
                        }
                        else {
                            pvari->WrFlags &= wrflag_neg;
                        }
                    }
                }
            }
            LeaveBlackboardCriticalSection();
        }
    }
    transfer_write_bytes_at_end_of_cycles();

#ifdef RUNTIME_MEASUREMENT
    Runtime_copy_vari_bb_egs = (uint64_t)(MyTimeStamp() - Start);
#endif
}



int TerminateConnectionAndRemoveAllVariables(void)
{
    int x;

    if (egs_bbvari_refarray != NULL) {
        for (x = 0; x < egs_bbvari_size; x++) {
            if (egs_bbvari_refarray[x].ptr != NULL) {
                remove_bbvari(egs_bbvari_refarray[x].vid);
            }
        }
        my_free(egs_bbvari_refarray);
        egs_bbvari_refarray = NULL;
    }
    return 0;
}


static MODEL_FUNCTION_POINTERS ModelFunctionPointer;

static int exit_flag = 1;


int __init_test_object(void)
{
    exit_flag = 1;
    ModelFunctionPointer.reference_varis();
    copy_vari_egs_bb();

    copy_vari_bb_egs();
    ModelFunctionPointer.init_test_object();
    copy_vari_egs_bb();
    return 0;
}

void __terminate_test_object(void)
{
    ModelFunctionPointer.terminate_test_object();
#ifdef WITH_CPP_CONSTRUCTORS
    CallAllCppDestrucorsForGlobalObjects();
#endif
    TerminateConnectionAndRemoveAllVariables();
}


static jmp_buf jumper;


void __cyclic_test_object(void)
{
    int value;

    if (exit_flag) {
#ifdef WITH_EXIT_FUNCTION
        value = setjmp (jumper);
#else
        value = 0;
#endif
        if (value != 0) {
            ThrowError(1, "Test process call exit() function with value %d", value);
            exit_flag = 0;
        } else {
            copy_vari_bb_egs ();

            ModelFunctionPointer.cyclic_test_object ();

            copy_vari_egs_bb ();
        }
    } 
}


TASK_CONTROL_BLOCK ModelIntegration_tcb =
        INIT_TASK_COTROL_BLOCK("EmptyTestObject", 1, 100, __cyclic_test_object, __init_test_object, __terminate_test_object, 131072);


void SetModelNameAndFunctionPointers(const char *par_Name, int par_Prio, MODEL_FUNCTION_POINTERS *par_FunctionPointers)
{
    memset(ModelIntegration_tcb.name, 0, sizeof(ModelIntegration_tcb.name));
    strncpy(ModelIntegration_tcb.name, par_Name, sizeof(ModelIntegration_tcb.name) - 1);
    ModelIntegration_tcb.prio = par_Prio;
    ModelFunctionPointer = *par_FunctionPointers;
}

