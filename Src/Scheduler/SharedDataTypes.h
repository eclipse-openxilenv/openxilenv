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


#ifndef SHAREDDATATYPES_H
#define SHAREDDATATYPES_H

#if _MSC_VER  == 1310
#include <my_stdint.h>
#else
#include <stdint.h>
#endif

typedef struct {
    unsigned long long UniqueId; 
    unsigned int AttachCounter;
    char Name[1];  // this can be larger than 1.
} NAME_AND_UNIQUE_ID;

typedef struct {
    union UNIQUEID_OR_NAME_AND_UNIQUE_ID {
        unsigned long long UniqueId;
        NAME_AND_UNIQUE_ID *NameAndUniqueId;
    } UniqueIdOrNamneAndUniqueId; 
    unsigned long long Addr;
    int Vid;
    short RangeControlState;
    short RangeControlFlag;
    int Dir;
    short TypeBB;
    short TypePipe;
} PIPE_MESSAGE_REF_COPY_LIST_ELEM;

typedef struct {
    int ElemCount;
    int ElemCountMax;
    int SizeInBytes;
    unsigned long long RefUniqueIdCounter;  // This will be incremented by each reference by one. Size is 64 bit, this should be never overflow
    PIPE_MESSAGE_REF_COPY_LIST_ELEM *pElems;
} PIPE_MESSAGE_REF_COPY_LIST;

typedef struct {
    PIPE_MESSAGE_REF_COPY_LIST List;
    PIPE_MESSAGE_REF_COPY_LIST RdList1;
    PIPE_MESSAGE_REF_COPY_LIST WrList1;
    PIPE_MESSAGE_REF_COPY_LIST RdList2;
    PIPE_MESSAGE_REF_COPY_LIST WrList2;
    PIPE_MESSAGE_REF_COPY_LIST RdList4;
    PIPE_MESSAGE_REF_COPY_LIST WrList4;
    PIPE_MESSAGE_REF_COPY_LIST RdList8;
    PIPE_MESSAGE_REF_COPY_LIST WrList8;
} PIPE_MESSAGE_REF_COPY_LISTS;

union BB_VARI {
    int8_t b;               // BB_BYTE
    uint8_t ub;             // BB_UBYTE
    int16_t w;              // BB_WORD
    uint16_t uw;            // BB_UWORD
    int32_t dw;             // BB_DWORD
    uint32_t udw;           // BB_UDWORD
    int64_t qw;             // BB_QWORD
    uint64_t uqw;           // BB_UQWORD
    float f;                // BB_FLOAT
    double d;               // BB_DOUBLE
    struct {                // BB_BITFIELD
        uint32_t Bits;
        int32_t BitCount;
    } Bit;
    char *str;              // BB_STRING
    union BB_VARI *pExt;    // BB_EXTERNAL_DATA
};

enum BB_DATA_TYPES { BB_INVALID = -1, BB_BYTE, BB_UBYTE, BB_WORD, BB_UWORD, BB_DWORD,
                     BB_UDWORD, BB_FLOAT, BB_DOUBLE, BB_UNKNOWN,
                     BB_UNKNOWN_DOUBLE, BB_UNKNOWN_WAIT,BB_BITFIELD, BB_GET_DATA_TYPE_FOM_DEBUGINFO,
                     BB_UNION, BB_CONVERT_LIMIT_MIN_MAX, BB_CONVERT_TO_DOUBLE, BB_STRING, BB_EXTERNAL_DATA,
                     BB_UNKNOWN_BYTE, BB_UNKNOWN_UBYTE, BB_UNKNOWN_WORD, BB_UNKNOWN_UWORD, BB_UNKNOWN_DWORD,
                     BB_UNKNOWN_UDWORD, BB_UNKNOWN_FLOAT, BB_UNKNOWN_DOUBLE2,
                     BB_UNKNOWN_WAIT_BYTE, BB_UNKNOWN_WAIT_UBYTE, BB_UNKNOWN_WAIT_WORD, BB_UNKNOWN_WAIT_UWORD, BB_UNKNOWN_WAIT_DWORD,
                     BB_UNKNOWN_WAIT_UDWORD, BB_UNKNOWN_WAIT_FLOAT, BB_UNKNOWN_WAIT_DOUBLE,
                     BB_QWORD, BB_UQWORD, BB_UNKNOWN_QWORD, BB_UNKNOWN_UQWORD, BB_UNKNOWN_WAIT_QWORD, BB_UNKNOWN_WAIT_UQWORD
};

enum BB_CONV_TYPES {
    BB_CONV_NONE,
    BB_CONV_FORMULA,
    BB_CONV_TEXTREP,
    BB_CONV_FACTOFF,
    BB_CONV_REF
};

union FloatOrInt64 {
    uint64_t uqw;
    int64_t qw;
    double d;
};
// same as #define EXEC_STACK_I64, EXEC_STACK_U64 and EXEC_STACK_F64 in exec_st.h
#define FLOAT_OR_INT_64_TYPE_INT64      0
#define FLOAT_OR_INT_64_TYPE_UINT64     1
#define FLOAT_OR_INT_64_TYPE_F64        2
#define FLOAT_OR_INT_64_TYPE_INVALID    3


#define NO_FREE_WRFLAG              -801
#define NO_FREE_VID                 -802
#define UNKNOWN_DTYPE               -803
#define UNKNOWN_VARIABLE            -804
#define WRONG_PARAMETER             -805
#define WRONG_INI_ENTRY	            -806
#define NO_INI_ENTRY	            -807
#define UNKNOWN_PROCESS             -808
#define NOT_INITIALIZED             -809
#define PROCESS_RUNNING             -810
#define PROCESS_INIT_ERROR          -811
#define NO_FREE_PID                 -812
#define PROCESS_TIMEOUT			    -813
#define INI_WRITE_ERROR             -814
#define NO_FREE_MEMORY              -815
#define NO_PROCESS_RUNNING          -816
#define EXCEPTION_ERROR             -817
#define INI_READ_ERROR              -818
#define TYPE_CLASH                  -819
#define NO_FREE_ACCESS_MASK         -820
#define CANNOT_EXEC_PROGRAM         -821
#define LABEL_NAME_TOO_LONG         -822
#define ADDRESS_NO_VALID_LABEL      -823
#define LABEL_NAME_NOT_VALID        -824
#define BB_VAR_ADD_INFOS_MEM_ERROR  -825
#define VARIABLE_REF_LIST_FILTERED  -826
#define VARIABLE_REF_ADDRESS_IS_ON_STACK    -827
#define VARIABLE_ALREADY_REFERENCED         -828
#define VARIABLE_ALREADY_EXISTS             -829
#define VARIABLE_REF_LIST_MAX_SIZE_REACHED  -830

#endif //#ifndef SHAREDDATATYPES_H
