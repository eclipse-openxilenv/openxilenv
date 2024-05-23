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


#include <string.h>
#include "StringMaxChar.h"
#include "A2LValue.h"
#include "BlackboardConvertFromTo.h"

int ConvertRawValueToInt(A2L_SINGLE_VALUE *RawValue)
{
    switch (RawValue->Type) {
    case A2L_ELEM_TYPE_INT:
        return (int)RawValue->Value.Int;
    case A2L_ELEM_TYPE_UINT:
        return (int)RawValue->Value.Uint;
    case A2L_ELEM_TYPE_DOUBLE:
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        return (int)RawValue->Value.Double;
    case A2L_ELEM_TYPE_TEXT_REPLACE:
    default:
        return 0;
    }
}

double ConvertRawValueToDouble(A2L_SINGLE_VALUE *RawValue)
{
    switch (RawValue->Type) {
    case A2L_ELEM_TYPE_INT:
        return (double)RawValue->Value.Int;
    case A2L_ELEM_TYPE_UINT:
        return (double)RawValue->Value.Uint;
    case A2L_ELEM_TYPE_DOUBLE:
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        return RawValue->Value.Double;
    case A2L_ELEM_TYPE_TEXT_REPLACE:
    default:
        return 0;
    }
}

void ConvertDoubleToRawValue(enum A2L_ELEM_TARGET_TYPE par_Type, double par_Value, A2L_SINGLE_VALUE *ret_RawValue)
{
    ret_RawValue->Flags = 0;
    ret_RawValue->TargetType = par_Type;
    switch (par_Type) {
    case A2L_ELEM_TARGET_TYPE_UINT8:
        ret_RawValue->Value.Uint = sc_convert_double2ubyte(par_Value);
        ret_RawValue->Type = A2L_ELEM_TYPE_UINT;
        break;
    case A2L_ELEM_TARGET_TYPE_UINT16:
        ret_RawValue->Value.Uint = sc_convert_double2uword(par_Value);
        ret_RawValue->Type = A2L_ELEM_TYPE_UINT;
        break;
    case A2L_ELEM_TARGET_TYPE_UINT32:
        ret_RawValue->Value.Uint = sc_convert_double2udword(par_Value);
        ret_RawValue->Type = A2L_ELEM_TYPE_UINT;
        break;
    case A2L_ELEM_TARGET_TYPE_UINT64:
        ret_RawValue->Value.Uint = sc_convert_double2uqword(par_Value);
        ret_RawValue->Type = A2L_ELEM_TYPE_UINT;
        break;
    case A2L_ELEM_TARGET_TYPE_INT8:
        ret_RawValue->Value.Int = sc_convert_double2byte(par_Value);
        ret_RawValue->Type = A2L_ELEM_TYPE_INT;
        break;
    case A2L_ELEM_TARGET_TYPE_INT16:
        ret_RawValue->Value.Int = sc_convert_double2word(par_Value);
        ret_RawValue->Type = A2L_ELEM_TYPE_INT;
        break;
    case A2L_ELEM_TARGET_TYPE_INT32:
        ret_RawValue->Value.Int = sc_convert_double2dword(par_Value);
        ret_RawValue->Type = A2L_ELEM_TYPE_INT;
        break;
    case A2L_ELEM_TARGET_TYPE_INT64:
        ret_RawValue->Value.Int = sc_convert_double2qword(par_Value);
        ret_RawValue->Type = A2L_ELEM_TYPE_INT;
        break;
    case A2L_ELEM_TARGET_TYPE_FLOAT32_IEEE:
        ret_RawValue->Value.Double = sc_convert_double2float(par_Value);
        ret_RawValue->Type = A2L_ELEM_TYPE_DOUBLE;
        break;
    case A2L_ELEM_TARGET_TYPE_FLOAT64_IEEE:
        ret_RawValue->Value.Double = par_Value;
        ret_RawValue->Type = A2L_ELEM_TYPE_DOUBLE;
        break;
    default:
        ret_RawValue->Type = A2L_ELEM_TYPE_ERROR;
        break;
    }
}

void ConvertDoubleToPhysValue(enum A2L_ELEM_TARGET_TYPE par_Type, double par_Value, A2L_SINGLE_VALUE *ret_PhysValue)
{
    ret_PhysValue->SizeOfStruct = sizeof(A2L_SINGLE_VALUE);
    ret_PhysValue->Flags = 0;
    ret_PhysValue->TargetType = par_Type;
    ret_PhysValue->Value.Double = (double)par_Value;
    ret_PhysValue->Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
}

void ValueCopy(A2L_SINGLE_VALUE *ret_DstValue, A2L_SINGLE_VALUE *par_SrcValue)
{
    MEMCPY(ret_DstValue, par_SrcValue, par_SrcValue->SizeOfStruct);
}

void StringCopyToValue (A2L_SINGLE_VALUE *ret_DstValue, const char *par_Src, int MaxSize)
{
    char *Dst = ret_DstValue->Value.String;
    int Counter = 1;
    while ((*par_Src != 0) && (Counter < MaxSize)) {
        *Dst++ = *par_Src++;
        Counter++;
    }
    *Dst = 0;
    if (Counter < (int)sizeof(union A2L_VALUE)) Counter = sizeof(union A2L_VALUE);
    ret_DstValue->SizeOfStruct = sizeof(A2L_SINGLE_VALUE) - sizeof(union A2L_VALUE) + Counter;
}

