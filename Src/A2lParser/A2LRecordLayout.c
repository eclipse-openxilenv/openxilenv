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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "A2LBuffer.h"
#include "A2LParser.h"
#include "A2LTokenizer.h"
#include "A2LAccess.h"

#define UNUSED(x) (void)(x)

int BuildRecordLayoutInfos (ASAP2_RECORD_LAYOUT *RecordLayout, ASAP2_MOD_COMMON *ModCommon, ASAP2_CHARACTERISTIC *Characteristic)
{
    UNUSED(Characteristic);
    int AlignmentByte = 1;   
    int AlignmentWord = 1;
    int AlignmentLong = 1;
    int AlignmentInt64 = 1;
    int AlignmentFloat16 = 1;
    int AlignmentFloat32 = 1;
    int AlignmentFloat64 = 1;

    if (CheckIfFlagSet(ModCommon->OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_BYTE)) {
        AlignmentByte = ModCommon->OptionalParameter.AlignmentByte;
    }
    if (CheckIfFlagSet(ModCommon->OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_WORD)) {
        AlignmentWord = ModCommon->OptionalParameter.AlignmentWord;
    }
    if (CheckIfFlagSet(ModCommon->OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_LONG)) {
        AlignmentLong = ModCommon->OptionalParameter.AlignmentLong;
    }
    if (CheckIfFlagSet(ModCommon->OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_INT64)) {
        AlignmentInt64 = ModCommon->OptionalParameter.AlignmentInt64;
    }
    if (CheckIfFlagSet(ModCommon->OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_FLOAT16_IEEE)) {
        AlignmentFloat16 = ModCommon->OptionalParameter.AlignmentFloat16;
    }
    if (CheckIfFlagSet(ModCommon->OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_FLOAT32_IEEE)) {
        AlignmentFloat32 = ModCommon->OptionalParameter.AlignmentFloat32;
    }
    if (CheckIfFlagSet(ModCommon->OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_FLOAT64_IEEE)) {
        AlignmentFloat64 = ModCommon->OptionalParameter.AlignmentFloat64;
    }

	if (CheckIfFlagSet(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_BYTE)) {
        AlignmentByte = RecordLayout->OptionalParameter.AlignmentByte.AlignmentBorder;
    }
	if (CheckIfFlagSet(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_WORD)) {
        AlignmentWord = RecordLayout->OptionalParameter.AlignmentWord.AlignmentBorder;
    }
	if (CheckIfFlagSet(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_LONG)) {
        AlignmentLong = RecordLayout->OptionalParameter.AlignmentLong.AlignmentBorder;
    }
    if (CheckIfFlagSet(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_INT64)) {
        AlignmentInt64 = RecordLayout->OptionalParameter.AlignmentInt64.AlignmentBorder;
    }
    if (CheckIfFlagSet(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_FLOAT16_IEEE)) {
        AlignmentFloat16 = RecordLayout->OptionalParameter.AlignmentFloat16.AlignmentBorder;
    }
    if (CheckIfFlagSet(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_FLOAT32_IEEE)) {
        AlignmentFloat32 = RecordLayout->OptionalParameter.AlignmentFloat32.AlignmentBorder;
    }
    if (CheckIfFlagSet(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_FLOAT64_IEEE)) {
        AlignmentFloat64 = RecordLayout->OptionalParameter.AlignmentFloat64.AlignmentBorder;
    }

    return 0;
}
