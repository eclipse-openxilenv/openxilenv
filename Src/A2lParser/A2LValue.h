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

#ifndef SCRPCDLL_EXPORTS
// same as ENUMDEF_CHARACTERISTIC_TYPE "VALUE 1 ASCII 2 VAL_BLK 3 CURVE 4 MAP 5 CUBOID 6 CUBE_4 7 CUBE_5 8 ? -1"
enum A2L_DATA_TYPE { A2L_DATA_TYPE_MEASUREMENT = 0, A2L_DATA_TYPE_VALUE = 1, A2L_DATA_TYPE_ASCII = 2,
                     A2L_DATA_TYPE_VAL_BLK = 3, A2L_DATA_TYPE_CURVE = 4, A2L_DATA_TYPE_MAP = 5,
                     A2L_DATA_TYPE_CUBOID = 6, A2L_DATA_TYPE_CUBE_4 = 7, A2L_DATA_TYPE_CUBE_5 = 8,
                     A2L_DATA_TYPE_ERROR = -1 };

enum A2L_ELEM_TYPE { A2L_ELEM_TYPE_INT = 0, A2L_ELEM_TYPE_UINT = 1, A2L_ELEM_TYPE_DOUBLE = 2,
                     A2L_ELEM_TYPE_PHYS_DOUBLE = 3, A2L_ELEM_TYPE_TEXT_REPLACE = 4, A2L_ELEM_TYPE_ERROR = -1 };

// same as #define ENUMDEF_DATATYPE "UBYTE 1 SBYTE 0 UWORD 3 SWORD 2 ULONG 5 SLONG 4 FLOAT32_IEEE 6 FLOAT64_IEEE 7 A_INT64 34 A_UINT64 35 FLOAT16_IEEE 36 ? -1"
// and same as blackboard data type numbers.
enum A2L_ELEM_TARGET_TYPE { A2L_ELEM_TARGET_TYPE_INT8 = 0, A2L_ELEM_TARGET_TYPE_UINT8 = 1,
                            A2L_ELEM_TARGET_TYPE_INT16 = 2, A2L_ELEM_TARGET_TYPE_UINT16 = 3,
                            A2L_ELEM_TARGET_TYPE_INT32 = 4, A2L_ELEM_TARGET_TYPE_UINT32 = 5,
                            A2L_ELEM_TARGET_TYPE_FLOAT32_IEEE = 6, A2L_ELEM_TARGET_TYPE_FLOAT64_IEEE = 7,
                            A2L_ELEM_TARGET_TYPE_INT64 = 34, A2L_ELEM_TARGET_TYPE_UINT64 = 35,
                            A2L_ELEM_TARGET_TYPE_FLOAT16_IEEE = 36,
                            A2L_ELEM_TARGET_TYPE_NO_TYPE = 100, A2L_ELEM_TARGET_TYPE_ERROR = -1 };
#endif

union A2L_VALUE {
    uint64_t Uint;
    int64_t Int;
    double Double;
    /*struct {
        int StructOffset;
        int Len;
    } TextReplace;*/
    char String[1];  // longer than 1Byte!
};

typedef struct {
    int32_t SizeOfStruct;   // should be always 32Bytes except it is a string
    enum A2L_ELEM_TYPE Type;
    enum A2L_ELEM_TARGET_TYPE TargetType;
    uint32_t Flags;
#define A2L_VALUE_FLAG_CALIBRATION               0x1
#define A2L_VALUE_FLAG_MEASUREMENT               0x2
#define A2L_VALUE_FLAG_PHYS                      0x4  
#define A2L_VALUE_FLAG_READ_ONLY                 0x8
#define A2L_VALUE_FLAG_ONLY_VIRTUAL             0x10
//#define A2L_VALUE_FLAG_NO_CALIBRATION_ACCESS    0x20
#define A2L_VALUE_FLAG_HAS_UNIT                 0x40
#define A2L_VALUE_FLAG_UPDATE                 0x1000
    uint64_t Address;
    union A2L_VALUE Value;
} A2L_SINGLE_VALUE;

typedef struct {
    int ElementCount;
    int DimensionCount;
    int Flags;
    int DimensionOrOffsets[1];  // can be more than 1! first elemenst are the dimensions (DimensionCount elements) folowed by the offsets
} A2L_VALUE_ARRAY;

int ConvertRawValueToInt(A2L_SINGLE_VALUE *RawValue);

double ConvertRawValueToDouble(A2L_SINGLE_VALUE *RawValue);

void ConvertDoubleToRawValue(enum A2L_ELEM_TARGET_TYPE par_Type, double par_Value, A2L_SINGLE_VALUE *ret_RawValue);

void ConvertDoubleToPhysValue(enum A2L_ELEM_TARGET_TYPE par_Type, double par_Value, A2L_SINGLE_VALUE *ret_PhysValue);

void ValueCopy(A2L_SINGLE_VALUE *ret_DstValue, A2L_SINGLE_VALUE *par_SrcValue);

void StringCopyToValue (A2L_SINGLE_VALUE *ret_DstValue, const char *par_Src, int MaxSize);
