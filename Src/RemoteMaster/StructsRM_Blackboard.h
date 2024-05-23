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
#include <stdint.h>

#include "Blackboard.h"
#include "StructRM.h"

#define RM_BLACKBOARD_OFFSET  16


#define RM_BLACKBOARD_INIT_BLACKBOARD_CMD  (RM_BLACKBOARD_OFFSET+0)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t blackboard_size;
    int32_t CopyBB2ProcOnlyIfWrFlagSet;
    int32_t AllowBBVarsWithSameAddr;
    int32_t conv_error2message;
}  RM_BLACKBOARD_INIT_BLACKBOARD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_INIT_BLACKBOARD_ACK;

#define RM_BLACKBOARD_CLOSE_BLACKBOARD_CMD  (RM_BLACKBOARD_OFFSET+1)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
}  RM_BLACKBOARD_CLOSE_BLACKBOARD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_CLOSE_BLACKBOARD_ACK;


#define RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_CMD  (RM_BLACKBOARD_OFFSET+2)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t NameOffset;
    uint32_t Type;
    uint32_t UnitOffset;
    int32_t Pid;
    uint32_t Dir;
    int32_t ValueValidFlag;
    uint64_t ValueUnion;
    uint32_t ReadReqMask;
#define READ_UNIT_BBVARI_FROM_INI          0x1
#define READ_MIN_BBVARI_FROM_INI           0x2
#define READ_MAX_BBVARI_FROM_INI           0x4
#define READ_STEP_BBVARI_FROM_INI          0x8
#define READ_WIDTH_PREC_BBVARI_FROM_INI    0x10
#define READ_CONVERSION_BBVARI_FROM_INI    0x20
#define READ_COLOR_BBVARI_FROM_INI         0x40
#define READ_ALL_INFOS_BBVARI_FROM_INI     0xFFFFFFFFUL

    // ... danach kommt der Name-String und dann der Unit-String
}  RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t RetType;
    int32_t Ret;
}  RM_BLACKBOARD_ADD_BBVARI_PID_TYPE_ACK;

#define RM_BLACKBOARD_ATTACH_BBVARI_CMD  (RM_BLACKBOARD_OFFSET+3)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t unknown_wait_flag;
    int32_t Pid;
}  RM_BLACKBOARD_ATTACH_BBVARI_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_ATTACH_BBVARI_ACK;

#define RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_CMD  (RM_BLACKBOARD_OFFSET+4)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t NameOffset;
    int32_t Pid;
    // ... danach kommt der Name-String
}  RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_ATTACH_BBVARI_BY_NAME_ACK;

#define RM_BLACKBOARD_REMOVE_BBVARI_CMD  (RM_BLACKBOARD_OFFSET+5)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t unknown_wait_flag;
    int32_t Pid;
}  RM_BLACKBOARD_REMOVE_BBVARI_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_REMOVE_BBVARI_ACK;

#define RM_BLACKBOARD_REMOVE_ALL_BBVARI_CMD  (RM_BLACKBOARD_OFFSET+6)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
}  RM_BLACKBOARD_REMOVE_ALL_BBVARI_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_REMOVE_ALL_BBVARI_ACK;

#define RM_BLACKBOARD_GET_NUM_OF_BBVARIS_CMD  (RM_BLACKBOARD_OFFSET+7)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
}  RM_BLACKBOARD_GET_NUM_OF_BBVARIS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t Ret;
}  RM_BLACKBOARD_GET_NUM_OF_BBVARIS_ACK;

#define RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_CMD  (RM_BLACKBOARD_OFFSET+8)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t NameOffset;
    // ... danach kommt der Name-String
}  RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_GET_BBVARIVID_BY_NAME_ACK;

#define RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_CMD  (RM_BLACKBOARD_OFFSET+9)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t NameOffset;
    // ... danach kommt der Name-String
}  RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_GET_BBVARITYPE_BY_NAME_ACK;

#define RM_BLACKBOARD_GET_BBVARI_ATTACHCOUNT_CMD  (RM_BLACKBOARD_OFFSET+10)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
}  RM_BLACKBOARD_GET_BBVARI_ATTACHCOUNT_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_GET_BBVARI_ATTACHCOUNT_ACK;

#define RM_BLACKBOARD_GET_BBVARITYPE_CMD  (RM_BLACKBOARD_OFFSET+11)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
}  RM_BLACKBOARD_GET_BBVARITYPE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_GET_BBVARITYPE_ACK;

#define RM_BLACKBOARD_GET_BBVARI_NAME_CMD  (RM_BLACKBOARD_OFFSET+12)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t MaxChar;
}  RM_BLACKBOARD_GET_BBVARI_NAME_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t NameOffset;
    int32_t Ret;
    // ... danach kommt der Name-String
}  RM_BLACKBOARD_GET_BBVARI_NAME_ACK;

#define RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_CMD  (RM_BLACKBOARD_OFFSET+13)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t MaxChar;
}  RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t NameOffset;
    uint32_t data_type;
    uint32_t conversion_type;
    int32_t Ret;
    // ... danach kommt der Name-String
}  RM_BLACKBOARD_GET_BBVARI_NAME_AND_TYPES_ACK;

#define RM_BLACKBOARD_GET_PROCESS_BBVARI_ATTACH_COUNT_CMD  (RM_BLACKBOARD_OFFSET+14)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t Pid;
}  RM_BLACKBOARD_GET_PROCESS_BBVARI_ATTACH_COUNT_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_GET_PROCESS_BBVARI_ATTACH_COUNT_ACK;

#define RM_BLACKBOARD_SET_BBVARI_UNIT_CMD  (RM_BLACKBOARD_OFFSET+15)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    uint32_t UnitOffset;
    // ... danach kommt der Unit-String
}  RM_BLACKBOARD_SET_BBVARI_UNIT_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_SET_BBVARI_UNIT_ACK;

#define RM_BLACKBOARD_GET_BBVARI_UNIT_CMD  (RM_BLACKBOARD_OFFSET+16)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t max_c;
}  RM_BLACKBOARD_GET_BBVARI_UNIT_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t UnitOffset;
    int32_t Ret;
    // ... danach kommt der Unit-String
}  RM_BLACKBOARD_GET_BBVARI_UNIT_ACK;

#define RM_BLACKBOARD_SET_BBVARI_CONVERSION_CMD  (RM_BLACKBOARD_OFFSET+17)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t ConversionType;
    uint32_t ConversionOffset;
    int32_t sizeof_bin_conversion;
    uint32_t BinConversionOffset;
    uint32_t Fill1;
    // ... danach kommt der Unit-String
}  RM_BLACKBOARD_SET_BBVARI_CONVERSION_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_SET_BBVARI_CONVERSION_ACK;

#define RM_BLACKBOARD_GET_BBVARI_CONVERSION_CMD  (RM_BLACKBOARD_OFFSET+18)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t maxc;
}  RM_BLACKBOARD_GET_BBVARI_CONVERSION_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t ConversionOffset;
    int32_t Ret;
    // ... danach kommt der Conversion-String
}  RM_BLACKBOARD_GET_BBVARI_CONVERSION_ACK;

#define RM_BLACKBOARD_GET_BBVARI_CONVERSIONTYPE_CMD  (RM_BLACKBOARD_OFFSET+19)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
}  RM_BLACKBOARD_GET_BBVARI_CONVERSIONTYPE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_GET_BBVARI_CONVERSIONTYPE_ACK;

#define RM_BLACKBOARD_GET_BBVARI_INFOS_CMD  (RM_BLACKBOARD_OFFSET+20)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t SizeOfBuffer;
}  RM_BLACKBOARD_GET_BBVARI_INFOS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    BB_VARIABLE BaseInfos;
    BB_VARIABLE_ADDITIONAL_INFOS AdditionalInfos;
    uint32_t OffsetName;
    uint32_t OffsetUnit;
    uint32_t OffsetConversion;
    uint32_t OffsetDisplayName;
    uint32_t OffsetComment;
    int32_t Ret;  // Benoetige Buffer-Groesse
    // ... danach kommt die zusaetzlichen Daten
}  RM_BLACKBOARD_GET_BBVARI_INFOS_ACK;

#define RM_BLACKBOARD_SET_BBVARI_COLOR_CMD  (RM_BLACKBOARD_OFFSET+21)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t rgb_color;
}  RM_BLACKBOARD_SET_BBVARI_COLOR_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_SET_BBVARI_COLOR_ACK;

#define RM_BLACKBOARD_GET_BBVARI_COLOR_CMD  (RM_BLACKBOARD_OFFSET+22)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
}  RM_BLACKBOARD_GET_BBVARI_COLOR_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t rgb_color;
}  RM_BLACKBOARD_GET_BBVARI_COLOR_ACK;

#define RM_BLACKBOARD_SET_BBVARI_STEP_CMD  (RM_BLACKBOARD_OFFSET+23)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    uint8_t steptype;
    double step;
}  RM_BLACKBOARD_SET_BBVARI_STEP_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_SET_BBVARI_STEP_ACK;

#define RM_BLACKBOARD_GET_BBVARI_STEP_CMD  (RM_BLACKBOARD_OFFSET+24)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
}  RM_BLACKBOARD_GET_BBVARI_STEP_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint8_t steptype;
    double step;
    int32_t Ret;
}  RM_BLACKBOARD_GET_BBVARI_STEP_ACK;

#define RM_BLACKBOARD_SET_BBVARI_MIN_CMD  (RM_BLACKBOARD_OFFSET+25)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    double min;
}  RM_BLACKBOARD_SET_BBVARI_MIN_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_SET_BBVARI_MIN_ACK;

#define RM_BLACKBOARD_SET_BBVARI_MAX_CMD  (RM_BLACKBOARD_OFFSET+26)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    double max;
}  RM_BLACKBOARD_SET_BBVARI_MAX_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_SET_BBVARI_MAX_ACK;

#define RM_BLACKBOARD_GET_BBVARI_MIN_CMD  (RM_BLACKBOARD_OFFSET+27)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
}  RM_BLACKBOARD_GET_BBVARI_MIN_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    double min;
}  RM_BLACKBOARD_GET_BBVARI_MIN_ACK;

#define RM_BLACKBOARD_GET_BBVARI_MAX_CMD  (RM_BLACKBOARD_OFFSET+28)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
}  RM_BLACKBOARD_GET_BBVARI_MAX_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    double max;
}  RM_BLACKBOARD_GET_BBVARI_MAX_ACK;

#define RM_BLACKBOARD_SET_BBVARI_FORMAT_CMD  (RM_BLACKBOARD_OFFSET+29)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t width;
    int32_t prec;
}  RM_BLACKBOARD_SET_BBVARI_FORMAT_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_SET_BBVARI_FORMAT_ACK;

#define RM_BLACKBOARD_GET_BBVARI_FORMAT_WIDTH_CMD  (RM_BLACKBOARD_OFFSET+30)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
}  RM_BLACKBOARD_GET_BBVARI_FORMAT_WIDTH_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t width;
}  RM_BLACKBOARD_GET_BBVARI_FORMAT_WIDTH_ACK;

#define RM_BLACKBOARD_GET_BBVARI_FORMAT_PREC_CMD  (RM_BLACKBOARD_OFFSET+31)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
}  RM_BLACKBOARD_GET_BBVARI_FORMAT_PREC_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t prec;
}  RM_BLACKBOARD_GET_BBVARI_FORMAT_PREC_ACK;

#define RM_BLACKBOARD_GET_FREE_WRFLAG_CMD  (RM_BLACKBOARD_OFFSET+32)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
}  RM_BLACKBOARD_GET_FREE_WRFLAG_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint64_t wrflag;
    int32_t Ret;
}  RM_BLACKBOARD_GET_FREE_WRFLAG_ACK;

#define RM_BLACKBOARD_RESET_WRFLAG_CMD  (RM_BLACKBOARD_OFFSET+33)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    uint64_t w;
}  RM_BLACKBOARD_RESET_WRFLAG_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_RESET_WRFLAG_ACK;

#define RM_BLACKBOARD_TEST_WRFLAG_CMD  (RM_BLACKBOARD_OFFSET+34)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    uint64_t w;
}  RM_BLACKBOARD_TEST_WRFLAG_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_TEST_WRFLAG_ACK;

#define RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_CMD  (RM_BLACKBOARD_OFFSET+35)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t index;
    int32_t max_c;
}  RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t NameOffset;
    int32_t Ret;
    // ... danach kommt der Name-String
}  RM_BLACKBOARD_READ_NEXT_BLACKBOARD_VARI_ACK;

#define RM_BLACKBOARD_READ_NEXT_VARI_PROCESS_ACCESS_CMD  (RM_BLACKBOARD_OFFSET+36)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t access;
    int32_t index;
    int32_t maxc;
}  RM_BLACKBOARD_READ_NEXT_VARI_PROCESS_ACCESS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t NameOffset;
    int32_t Ret;
    // ... danach kommt der Name-String
}  RM_BLACKBOARD_READ_NEXT_VARI_PROCESS_ACCESS_ACK;

#define RM_BLACKBOARD_ENABLE_BBVARI_ACCESS_CMD  (RM_BLACKBOARD_OFFSET+37)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
}  RM_BLACKBOARD_ENABLE_BBVARI_ACCESS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_ENABLE_BBVARI_ACCESS_ACK;

#define RM_BLACKBOARD_DISABLE_BBVARI_ACCESS_CMD  (RM_BLACKBOARD_OFFSET+38)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
}  RM_BLACKBOARD_DISABLE_BBVARI_ACCESS_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_DISABLE_BBVARI_ACCESS_ACK;


#define RM_BLACKBOARD_SET_OBSERVATION_CALLBACK_FUNCTION_CMD  (RM_BLACKBOARD_OFFSET+39)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint64_t Callback;
}  RM_BLACKBOARD_SET_OBSERVATION_CALLBACK_FUNCTION_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
}  RM_BLACKBOARD_SET_OBSERVATION_CALLBACK_FUNCTION_ACK;

#define RM_BLACKBOARD_SET_BBVARI_OBSERVATION_CMD  (RM_BLACKBOARD_OFFSET+40)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    uint32_t ObservationFlags;
    uint32_t ObservationData;
}  RM_BLACKBOARD_SET_BBVARI_OBSERVATION_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_SET_BBVARI_OBSERVATION_ACK;

#define RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_CMD  (RM_BLACKBOARD_OFFSET+41)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t ObservationFlags;
    uint32_t ObservationData;
}  RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t OffsetVids;
    int32_t ret_Count;
    uint32_t Ret;
    // ... danach kommen die Vids
}  RM_BLACKBOARD_SET_GLOBAL_BLACKBOARD_OBSERVATION_ACK;

#define RM_BLACKBOARD_GET_BB_ACCESSMASK_CMD  (RM_BLACKBOARD_OFFSET+42)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t pid;
    uint32_t BBPrefixOffset;
    // ... danach kommt der BBPrefix-String
}  RM_BLACKBOARD_GET_BB_ACCESSMASK_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint64_t ret_mask;
    int32_t Ret;
}  RM_BLACKBOARD_GET_BB_ACCESSMASK_ACK;


#define RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_CMD  (RM_BLACKBOARD_OFFSET+43)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t VariableNameOffset;
    uint32_t UnitOffset;
    uint32_t ConversionOffset;
    uint32_t RgbColor;
    double Min;
    double Max;
    double Step;
    uint8_t Width;
    uint8_t Prec;
    uint8_t StepType;
    uint8_t ConversionType;
    uint32_t Flags;
    // ... danach kommt ...
}  RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_ADD_VARIABLE_SECTION_CACHE_ENTRY_ACK;

#define RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_CMD  (RM_BLACKBOARD_OFFSET+44)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Index;
}  RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t VariableNameOffset;
    uint32_t UnitOffset;
    uint32_t ConversionOffset;
    double Min;
    double Max;
    double Step;
    uint8_t Width;
    uint8_t Prec;
    uint8_t StepType;
    uint8_t ConversionType;
    uint32_t RgbColor;
    int32_t Ret;
    // ... danach kommt der Namens-String gefolgt vom Datum 
}  RM_BLACKBOARD_GET_NEXT_VARIABLE_SECTION_CACHE_ENTRY_ACK;


#define RM_RD_WR_BLACKBOARD_OFFSET  100

// Write/Read Funktionen

#define RM_BLACKBOARD_WRITE_BBVARI_BYTE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+0)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    int8_t Value;
} RM_BLACKBOARD_WRITE_BBVARI_BYTE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_BYTE_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_UBYTE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+1)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    uint8_t Value;
} RM_BLACKBOARD_WRITE_BBVARI_UBYTE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_UBYTE_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_WORD_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+2)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    int16_t Value;
} RM_BLACKBOARD_WRITE_BBVARI_WORD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_WORD_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_UWORD_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+3)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    uint16_t Value;
} RM_BLACKBOARD_WRITE_BBVARI_UWORD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_UWORD_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_DWORD_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+4)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    int32_t Value;
} RM_BLACKBOARD_WRITE_BBVARI_DWORD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_DWORD_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_UDWORD_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+5)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    uint32_t Value;
} RM_BLACKBOARD_WRITE_BBVARI_UDWORD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_UDWORD_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_QWORD_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+6)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    int64_t Value;
} RM_BLACKBOARD_WRITE_BBVARI_QWORD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_QWORD_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_UQWORD_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+7)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    uint64_t Value;
} RM_BLACKBOARD_WRITE_BBVARI_UQWORD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_UQWORD_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_FLOAT_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+8)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    float Value;
} RM_BLACKBOARD_WRITE_BBVARI_FLOAT_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_FLOAT_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_DOUBLE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+9)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    double Value;
} RM_BLACKBOARD_WRITE_BBVARI_DOUBLE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_DOUBLE_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_UNION_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+10)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    union BB_VARI Value;
} RM_BLACKBOARD_WRITE_BBVARI_UNION_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_UNION_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_UNION_PID_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+11)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    int32_t DataType;
    union BB_VARI Value;
} RM_BLACKBOARD_WRITE_BBVARI_UNION_PID_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_UNION_PID_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_MIN_MAX_CHECK_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+12)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    double Value;
} RM_BLACKBOARD_WRITE_BBVARI_MIN_MAX_CHECK_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
} RM_BLACKBOARD_WRITE_BBVARI_MIN_MAX_CHECK_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+13)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    int32_t convert_from_type;
    uint64_t Value;
} RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
} RM_BLACKBOARD_WRITE_BBVARI_CONVERT_TO_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_PHYS_MINMAX_CHECK_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+14)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    int32_t Vid;
    double Value;
} RM_BLACKBOARD_WRITE_BBVARI_PHYS_MINMAX_CHECK_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
} RM_BLACKBOARD_WRITE_BBVARI_PHYS_MINMAX_CHECK_ACK;

#define RM_BLACKBOARD_GET_PHYS_VALUE_FOR_RAW_VALUE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+15)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    double raw_value;
} RM_BLACKBOARD_GET_PHYS_VALUE_FOR_RAW_VALUE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    double ret_phys_value;
    int32_t Ret;
} RM_BLACKBOARD_GET_PHYS_VALUE_FOR_RAW_VALUE_ACK;

#define RM_BLACKBOARD_GET_RAW_VALUE_FOR_PHYS_VALUE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+16)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    double phys_value;
} RM_BLACKBOARD_GET_RAW_VALUE_FOR_PHYS_VALUE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    double ret_raw_value;
    double ret_phys_value;
    int32_t Ret;
} RM_BLACKBOARD_GET_RAW_VALUE_FOR_PHYS_VALUE_ACK;



#define RM_BLACKBOARD_READ_BBVARI_BYTE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+17)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_BYTE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int8_t Ret;
} RM_BLACKBOARD_READ_BBVARI_BYTE_ACK;

#define RM_BLACKBOARD_READ_BBVARI_UBYTE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+18)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_UBYTE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint8_t Ret;
} RM_BLACKBOARD_READ_BBVARI_UBYTE_ACK;

#define RM_BLACKBOARD_READ_BBVARI_WORD_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+19)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_WORD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int16_t Ret;
} RM_BLACKBOARD_READ_BBVARI_WORD_ACK;

#define RM_BLACKBOARD_READ_BBVARI_UWORD_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+20)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_UWORD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint16_t Ret;
} RM_BLACKBOARD_READ_BBVARI_UWORD_ACK;

#define RM_BLACKBOARD_READ_BBVARI_DWORD_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+21)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_DWORD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
} RM_BLACKBOARD_READ_BBVARI_DWORD_ACK;

#define RM_BLACKBOARD_READ_BBVARI_UDWORD_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+22)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_UDWORD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t Ret;
} RM_BLACKBOARD_READ_BBVARI_UDWORD_ACK;

#define RM_BLACKBOARD_READ_BBVARI_QWORD_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+23)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_QWORD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int64_t Ret;
} RM_BLACKBOARD_READ_BBVARI_QWORD_ACK;

#define RM_BLACKBOARD_READ_BBVARI_UQWORD_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+24)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_UQWORD_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint64_t Ret;
} RM_BLACKBOARD_READ_BBVARI_UQWORD_ACK;

#define RM_BLACKBOARD_READ_BBVARI_FLOAT_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+25)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_FLOAT_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    float Ret;
} RM_BLACKBOARD_READ_BBVARI_FLOAT_ACK;

#define RM_BLACKBOARD_READ_BBVARI_DOUBLE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+26)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_DOUBLE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    double Ret;
} RM_BLACKBOARD_READ_BBVARI_DOUBLE_ACK;

#define RM_BLACKBOARD_READ_BBVARI_UNION_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+27)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_UNION_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    union BB_VARI Ret;
} RM_BLACKBOARD_READ_BBVARI_UNION_ACK;

#define RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+28)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    union BB_VARI ret_Value;
    enum BB_DATA_TYPES Ret;
} RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_ACK;

#define RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+29)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Number;
    uint32_t OffsetVids;
    // ... danach kommen die Vids
} RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t OffsetTypes;
    uint32_t OffsetValues;
    int32_t Ret;
    // ... danach kommen die Types und die Values
} RM_BLACKBOARD_READ_BBVARI_UNION_TYPE_FRAME_ACK;

#define RM_BLACKBOARD_READ_BBVARI_EQU_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+30)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_EQU_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    double Ret;
} RM_BLACKBOARD_READ_BBVARI_EQU_ACK;

#define RM_BLACKBOARD_READ_BBVARI_CONVERT_DOUBLE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+31)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
} RM_BLACKBOARD_READ_BBVARI_CONVERT_DOUBLE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    double Ret;
} RM_BLACKBOARD_READ_BBVARI_CONVERT_DOUBLE_ACK;

#define RM_BLACKBOARD_READ_BBVARI_CONVERT_TO_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+32)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t convert_to_type;
} RM_BLACKBOARD_READ_BBVARI_CONVERT_TO_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    union BB_VARI ret_Value;
    int32_t Ret;
} RM_BLACKBOARD_READ_BBVARI_CONVERT_TO_ACK;

#define RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+33)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t maxc;
} RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t OffsetText;
    int32_t ret_color;
    int32_t Ret;
    // ... danach kommt der Text-String
} RM_BLACKBOARD_READ_BBVARI_TEXTREPLACE_ACK;

#define RM_BLACKBOARD_CONVERT_TEXTREPLACE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+34)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    int32_t Value;
    int32_t maxc;
} RM_BLACKBOARD_CONVERT_TEXTREPLACE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t OffsetText;
    int32_t ret_color;
    int32_t Ret;
    // ... danach kommt der Text-String
} RM_BLACKBOARD_CONVERT_TEXTREPLACE_ACK;

#define RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+35)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    uint32_t OffsetText;
    // ... danach kommt der Text-String
} RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int64_t ret_from;
    int64_t ret_to;
    int32_t Ret;
} RM_BLACKBOARD_CONVERT_TEXTREPLACE_VALUE_ACK;

#define RM_BLACKBOARD_READ_BBVARI_FRAME_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+36)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t OffsetVids;
    int32_t Size;
    // ... danach kommten die Vids
} RM_BLACKBOARD_READ_BBVARI_FRAME_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t OffsetValues;
    int32_t Ret;
    // ... danach kommten die Werte
} RM_BLACKBOARD_READ_BBVARI_FRAME_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_FRAME_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+37)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint32_t OffsetVids;
    uint32_t OffsetValues;
    int32_t Pid;
    int32_t Size;
    // ... danach kommen die Vids
    // ... dann kommten die Werte
} RM_BLACKBOARD_WRITE_BBVARI_FRAME_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
} RM_BLACKBOARD_WRITE_BBVARI_FRAME_ACK;

// begin new  XXXXXXXXXXXXXXXX
#define RM_READ_BBVARI_CONVERT_TO_FLOATANDINT64_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+38)
typedef struct {
	RM_PACKAGE_HEADER PackageHeader;
        int32_t vid;
} RM_READ_BBVARI_CONVERT_TO_FLOATANDINT64_REQ;

typedef struct {
	RM_PACKAGE_HEADER PackageHeader;
	union FloatOrInt64 RetValue;
	int32_t Ret;
} RM_READ_BBVARI_CONVERT_TO_FLOATANDINT64_ACK;

#define RM_WRITE_BBVARI_CONVERT_WITH_FLOATANDINT64_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+39)
typedef struct {
	RM_PACKAGE_HEADER PackageHeader;
	int32_t pid;
        int32_t vid;
	union FloatOrInt64 value;
    uint32_t type;
} RM_WRITE_BBVARI_CONVERT_WITH_FLOATANDINT64_REQ;

typedef struct {
	RM_PACKAGE_HEADER PackageHeader;
} RM_WRITE_BBVARI_CONVERT_WITH_FLOATANDINT64_ACK;


#define RM_BLACKBOARD_READ_BBVARI_BY_NAME_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+40)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t read_type;
    uint32_t OffsetText;
    // ... danach kommt der Text-String
} RM_BLACKBOARD_READ_BBVARI_BY_NAME_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    union FloatOrInt64 value;
    int32_t ret_byte_width;
    int32_t Ret;
} RM_BLACKBOARD_READ_BBVARI_BY_NAME_ACK;

#define RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+41)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    union FloatOrInt64 value;
    int32_t Pid;
    uint32_t type;
    uint32_t bb_type;
    uint32_t add_if_not_exist;
    int32_t read_type;
    uint32_t OffsetText;
    // ... danach kommt der Text-String
} RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_REQ;

typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
} RM_BLACKBOARD_WRITE_BBVARI_BY_NAME_ACK;

// End new  XXXXXXXXXXXXXXXX

// vom Remote Master zum Client
#define RM_BLACKBOARD_WRITE_BBVARI_INFOS_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+200)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    BB_VARIABLE BaseInfos;
    BB_VARIABLE_ADDITIONAL_INFOS AdditionalInfos;
    uint32_t OffsetIniPath;
    uint32_t OffsetName;
    uint32_t OffsetUnit;
    uint32_t OffsetConversion;
    uint32_t OffsetDisplayName;
    uint32_t OffsetComment;
    // ... danach kommt die zusaetzlichen Daten
}  RM_BLACKBOARD_WRITE_BBVARI_INFOS_REQ;

/* no ACK
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_WRITE_BBVARI_INFOS_ACK;
*/

#define RM_BLACKBOARD_READ_BBVARI_FROM_INI_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+201)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Vid;
    uint32_t OffsetName;
    uint32_t ReadReqMask;
    // ... danach kommt der Variablename
}  RM_BLACKBOARD_READ_BBVARI_FROM_INI_REQ;

#define RM_BLACKBOARD_REGISTER_EQUATION_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+202)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Pid;
    uint32_t OffsetEquation;
    uint32_t OffsetExecStack;
    uint32_t OffsetAdditionalInfos;
    uint32_t TypeFlags;
    // ... danach kommt die zusaetzlichen Daten
}  RM_BLACKBOARD_REGISTER_EQUATION_REQ;

/* no Ack
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    int32_t Ret;
}  RM_BLACKBOARD_REGISTER_EQUATION_ACK;
*/

#define RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_CMD  (RM_RD_WR_BLACKBOARD_OFFSET+203)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint64_t Callback;
    int32_t Vid;
    uint32_t ObservationFlags;
    uint32_t ObservationData;
} RM_BLACKBOARD_CALL_OBSERVATION_CALLBACK_FUNCTION_REQ;

#define RM_BLACKBOARD_ATTACH_REGISTER_EQUATION_CMD   (RM_RD_WR_BLACKBOARD_OFFSET+204)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint64_t UniqueNumber;
    int32_t Pid;
} RM_BLACKBOARD_ATTACH_REGISTER_EQUATION_REQ;

#define RM_BLACKBOARD_DETACH_REGISTER_EQUATION_CMD   (RM_RD_WR_BLACKBOARD_OFFSET+205)
typedef struct {
    RM_PACKAGE_HEADER PackageHeader;
    uint64_t UniqueNumber;
    int32_t Pid;
} RM_BLACKBOARD_DETACH_REGISTER_EQUATION_REQ;

