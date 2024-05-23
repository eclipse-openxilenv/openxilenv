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


#include "Platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "Config.h"
#include "StringMaxChar.h"
#include "CanFifo.h"

#include "RpcFuncCalibration.h"
#include "RpcClientSocket.h"
#include "RpcFuncClientA2lLinks.h"

// XilEnvRpc.h must be included after all RpcFuncXxx.h
#include "XilEnvRpc.h"
// this must folow after ilEnvRpc.h
#include "A2LValue.h"
#include "A2LAccessData.h"

CFUNC SCRPCDLL_API enum A2L_DATA_TYPE __STDCALL__ XilEnv_GetLinkDataType(XILENV_LINK_DATA *Data)
{
    A2L_DATA *d = (A2L_DATA*)Data->Data;
    if (d == NULL) return -1;
    return d->Type;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkDataArrayCount(XILENV_LINK_DATA *Data)
{
    A2L_DATA *d = (A2L_DATA*)Data->Data;
    if (d == NULL) return -1;
    return d->ArrayCount;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkDataArraySize(XILENV_LINK_DATA *Data, int ArrayNo)
{
    int *Offsets;
    A2L_VALUE_ARRAY *ArrayHeader;
    int ArrayHeaderOffset;
    A2L_DATA *d = (A2L_DATA*)Data->Data;
    if (d == NULL) return 0;
    if (d->ArrayCount < ArrayNo) return 0;
    Offsets = (int*)(d + 1);
    ArrayHeaderOffset = Offsets[ArrayNo];
    if (ArrayHeaderOffset < 0) return 0;
    if (ArrayHeaderOffset > d->StructPos) return 0;

    ArrayHeader = (A2L_VALUE_ARRAY*)((char*)d +            // base structure
                                     ArrayHeaderOffset);   // + Offsets
    return ArrayHeader->ElementCount;
}

CFUNC SCRPCDLL_API XILENV_LINK_DATA* __STDCALL__ XilEnv_CopyLinkData(XILENV_LINK_DATA *Data)
{
    XILENV_LINK_DATA *Ret;
    A2L_DATA *d = (A2L_DATA*)Data->Data;
    Ret = (XILENV_LINK_DATA*)malloc(sizeof(XILENV_LINK_DATA));
    if (Ret == NULL) return NULL;
    Ret->Data = malloc(d->StructSize);
    if (Ret->Data == NULL) {
        free(Ret);
        return NULL;
    }
    MEMCPY(Ret, Data, d->StructPos);  // not use StructSize here!
    return Ret;

}

CFUNC SCRPCDLL_API XILENV_LINK_DATA* __STDCALL__ XilEnv_FreeLinkData(XILENV_LINK_DATA *Data)
{
    if (Data != NULL) {
        if (Data->Data != NULL) free(Data->Data);
        free(Data);
    }
    return NULL;
}


static A2L_SINGLE_VALUE* GetLinkSingleValueData(XILENV_LINK_DATA *Data)
{
    A2L_DATA *d = (A2L_DATA*)Data->Data;
    if (d == NULL) return NULL;
    if (d->ArrayCount != 0) return NULL;
    return (A2L_SINGLE_VALUE*)(d + 1);
}

static A2L_VALUE_ARRAY *GetArray(A2L_DATA* Data, int ArrayNo)
{
    int *Offsets;
    int ArrayHeaderOffset;
    A2L_VALUE_ARRAY *ArrayHeader;
    A2L_DATA *d = (A2L_DATA*)Data;
    if (d == NULL) return NULL;
    if (d->ArrayCount < ArrayNo) return NULL;
    Offsets = (int*)(d + 1);
    ArrayHeaderOffset = Offsets[ArrayNo];
    if (ArrayHeaderOffset < 0) return NULL;
    if (ArrayHeaderOffset > d->StructPos) return NULL;

    ArrayHeader = (A2L_VALUE_ARRAY*)((char*)d +            // base structure
                                     ArrayHeaderOffset);   // + Offsets
    return ArrayHeader;
}

static A2L_SINGLE_VALUE* GetLinkArrayValueData(XILENV_LINK_DATA *Data, int ArrayNo, int ElementNo)
{
    int *Offsets;
    A2L_SINGLE_VALUE *Value;
    A2L_VALUE_ARRAY *ArrayHeader;
    int ArrayHeaderOffset;
    int ValueOffset;
    A2L_DATA *d = (A2L_DATA*)Data->Data;
    if (d == NULL) return NULL;
    // if ArrayNo < 0 than it should be a single value
    if (ArrayNo < 0) return GetLinkSingleValueData(Data);
    if (d->ArrayCount < ArrayNo) return NULL;
    Offsets = (int*)(d + 1);
    ArrayHeaderOffset = Offsets[ArrayNo];
    if (ArrayHeaderOffset < 0) return NULL;
    if (ArrayHeaderOffset > d->StructPos) return NULL;

    ArrayHeader = (A2L_VALUE_ARRAY*)((char*)d +            // base structure
                                     ArrayHeaderOffset);   // + Offsets
    if (ElementNo >= ArrayHeader->ElementCount) return NULL;
    ValueOffset = ArrayHeader->DimensionOrOffsets[ArrayHeader->DimensionCount + ElementNo];
    if (ValueOffset < 0) return NULL;
    if (ValueOffset > d->StructPos) return NULL;
    Value = (A2L_SINGLE_VALUE*)((char*)d +       // base structure
                                ValueOffset);    // + Offsets
    return Value;
}

static double ConvertToDouble(A2L_SINGLE_VALUE *Value)
{
    if (Value == NULL) return 0.0;
    switch(Value->Type) {
    case A2L_ELEM_TYPE_INT:
        return (double)Value->Value.Int;
    case A2L_ELEM_TYPE_UINT:
        return (double)Value->Value.Uint;
    case A2L_ELEM_TYPE_DOUBLE:
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        return Value->Value.Double;
    case A2L_ELEM_TYPE_TEXT_REPLACE:
    case A2L_ELEM_TYPE_ERROR:
    default:
        return 0.0;
    }
}

static int SetDouble(A2L_SINGLE_VALUE *Value, double NewValue)
{
    if (Value != NULL) {
        if ((Value->Flags & (A2L_VALUE_FLAG_READ_ONLY | A2L_VALUE_FLAG_ONLY_VIRTUAL)) != 0) return -1;
        switch(Value->Type) {
        case A2L_ELEM_TYPE_INT:
            Value->Value.Int = (int64_t)NewValue;
            Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            return 0;
        case A2L_ELEM_TYPE_UINT:
            Value->Value.Uint = (uint64_t)NewValue;
            Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            return 0;
        case A2L_ELEM_TYPE_DOUBLE:
        case A2L_ELEM_TYPE_PHYS_DOUBLE:
            Value->Value.Double = NewValue;
            Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            return 0;
        case A2L_ELEM_TYPE_TEXT_REPLACE:
        case A2L_ELEM_TYPE_ERROR:
        default:
            Value->Value.Uint = 0;
            return -1;
        }
    }
    return -1;
}

static int64_t ConvertToInt(A2L_SINGLE_VALUE *Value)
{
    if (Value == NULL) return 0;
    switch(Value->Type) {
    case A2L_ELEM_TYPE_INT:
        return Value->Value.Int;
    case A2L_ELEM_TYPE_UINT:
        return (int64_t)Value->Value.Uint;
    case A2L_ELEM_TYPE_DOUBLE:
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        return (int64_t)Value->Value.Double;
    case A2L_ELEM_TYPE_TEXT_REPLACE:
    case A2L_ELEM_TYPE_ERROR:
    default:
        return 0;
    }
}

static int SetInt(A2L_SINGLE_VALUE *Value, int64_t NewValue)
{
    if (Value != NULL) {
        if ((Value->Flags & (A2L_VALUE_FLAG_READ_ONLY | A2L_VALUE_FLAG_ONLY_VIRTUAL)) != 0) return -1;
        switch(Value->Type) {
        case A2L_ELEM_TYPE_INT:
            Value->Value.Int = NewValue;
            Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            return 0;
        case A2L_ELEM_TYPE_UINT:
            Value->Value.Uint = (uint64_t)NewValue;
            Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            return 0;
        case A2L_ELEM_TYPE_DOUBLE:
        case A2L_ELEM_TYPE_PHYS_DOUBLE:
            Value->Value.Double = (double)NewValue;
            Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            return 0;
        case A2L_ELEM_TYPE_TEXT_REPLACE:
        case A2L_ELEM_TYPE_ERROR:
        default:
            Value->Value.Uint = 0;
            return -1;
        }
    }
    return -1;
}


static uint64_t ConvertToUint(A2L_SINGLE_VALUE *Value)
{
    if (Value == NULL) return 0;
    switch(Value->Type) {
    case A2L_ELEM_TYPE_INT:
        return (uint64_t)Value->Value.Int;
    case A2L_ELEM_TYPE_UINT:
        return Value->Value.Uint;
    case A2L_ELEM_TYPE_DOUBLE:
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        return (uint64_t)Value->Value.Double;
    case A2L_ELEM_TYPE_TEXT_REPLACE:
    case A2L_ELEM_TYPE_ERROR:
    default:
        return 0;
    }
}

static int SetUint(A2L_SINGLE_VALUE *Value, uint64_t NewValue)
{
    if (Value != NULL) {
        if ((Value->Flags & (A2L_VALUE_FLAG_READ_ONLY | A2L_VALUE_FLAG_ONLY_VIRTUAL)) != 0) return -1;
        switch(Value->Type) {
        case A2L_ELEM_TYPE_INT:
            Value->Value.Int = (int64_t)NewValue;
            Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            return 0;
        case A2L_ELEM_TYPE_UINT:
            Value->Value.Uint = NewValue;
            Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            return 0;
        case A2L_ELEM_TYPE_DOUBLE:
        case A2L_ELEM_TYPE_PHYS_DOUBLE:
            Value->Value.Double = (double)NewValue;
            Value->Flags |= A2L_VALUE_FLAG_UPDATE;
            return 0;
        case A2L_ELEM_TYPE_TEXT_REPLACE:
        case A2L_ELEM_TYPE_ERROR:
        default:
            Value->Value.Uint = 0;
            return -1;
        }
    }
    return -1;
}

static int StringCopy(char *Dst, const char *Src, int MaxLen)
{
    int Len = strlen(Src) + 1;
    if ((Src != 0) && (MaxLen > 0)) {
        if (Len <= MaxLen) {
            MEMCPY(Dst, Src, Len);
        } else {
            MEMCPY(Dst, Src, MaxLen);
            Dst[MaxLen - 1] = 0;
        }
    }
    return Len;
}

static int ConvertToString(A2L_SINGLE_VALUE *Value, char *ret_Value, int MaxLen)
{
    char Help[64];
    if (Value == NULL) return 0;
    switch(Value->Type) {
    case A2L_ELEM_TYPE_INT:
        sprintf (Help, "%lli", Value->Value.Int);
        return StringCopy(ret_Value, Help, MaxLen);
    case A2L_ELEM_TYPE_UINT:
        sprintf (Help, "%llu", Value->Value.Uint);
        return StringCopy(ret_Value, Help, MaxLen);
    case A2L_ELEM_TYPE_DOUBLE:
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        {
            int Prec = 15;
            while (1) {
                sprintf (Help, "%.*g", Prec, Value->Value.Double);
                if ((Prec++) == 18 || (Value->Value.Double == strtod (Help, NULL))) break;
            }
        }
        return StringCopy(ret_Value, Help, MaxLen);
    case A2L_ELEM_TYPE_TEXT_REPLACE:
        return StringCopy(ret_Value, Value->Value.String, MaxLen);
    case A2L_ELEM_TYPE_ERROR:
    default:
        return -1;
    }
}

static void MoveAllUp(A2L_DATA* Data, int Level, int Offset)
{
    int ArrayNo, ElemNo;
    int *Offsets;
    int ArrayHeaderOffset, ElemOffset;
    A2L_VALUE_ARRAY *ArrayHeader;
    A2L_DATA *d = (A2L_DATA*)Data;
    if (d == NULL) return;
    Offsets = (int*)(d + 1);
    for (ArrayNo = 0; ArrayNo < d->ArrayCount; ArrayNo++) {
        ArrayHeaderOffset = Offsets[ArrayNo];
        if (ArrayHeaderOffset < 0) return;
        if (ArrayHeaderOffset > d->StructPos) return;
        if (Offsets[ArrayNo] > Level) Offsets[ArrayNo] += Offset;
        ArrayHeader = (A2L_VALUE_ARRAY*)((char*)d +            // base structure
                                         ArrayHeaderOffset);   // + old offset 
        for (ElemNo = 0; ElemNo < ArrayHeader->ElementCount; ElemNo++) {
            ElemOffset = ArrayHeader->DimensionOrOffsets[ArrayHeader->DimensionCount + ElemNo];
            if (ElemNo < 0) return;
            if (ElemNo > d->StructPos) return;
            if (ArrayHeader->DimensionOrOffsets[ArrayHeader->DimensionCount + ElemNo] > Level) {
                ArrayHeader->DimensionOrOffsets[ArrayHeader->DimensionCount + ElemNo] += Offset;
            }
        }
    }
}


static A2L_DATA *SetString(A2L_DATA *Data, int AxisNo, int ElementNo, A2L_SINGLE_VALUE *Value, const char *String)
{
    int OldLen = strlen(Value->Value.String) + 1;
    int NewLen = strlen(String) + 1;
    if (OldLen >= NewLen) {
        // fits into the old string (that's easy)
        MEMCPY(Value->Value.String, String, NewLen);
        Value->Flags |= A2L_VALUE_FLAG_UPDATE;
    } else {
        if ((Data->StructSize - Data->StructPos) < (NewLen - OldLen)) {
            // we must enlage the structure
            Data->StructSize += (NewLen - OldLen) - (Data->StructSize - Data->StructPos);
            Data = (A2L_DATA*)realloc(Data, Data->StructSize);
            if (Data == NULL) return NULL;
        }
        // now it fits into the struct
        if (AxisNo >= 0) { 
            // it is a curve or a map
            int Level = (char*)Value - (char*)Data;
            int Offset = NewLen - OldLen;
            MoveAllUp(Data, Level, Offset);
        } else {
            // it is a single value
        }
        MEMCPY(Value->Value.String, String, NewLen);
        Value->Flags |= A2L_VALUE_FLAG_UPDATE;
        Data->StructPos += (NewLen - OldLen);
    }
    return Data;
}


// Single value

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkSingleValueDataType(XILENV_LINK_DATA *Data)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    if (d == 0) return A2L_ELEM_TYPE_ERROR;
    return d->Type;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkSingleValueTargetDataType(XILENV_LINK_DATA *Data)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    if (d == 0) return A2L_DATA_TYPE_ERROR;
    return d->TargetType;
}

CFUNC SCRPCDLL_API uint32_t __STDCALL__ XilEnv_GetLinkSingleValueFlags(XILENV_LINK_DATA *Data)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    if (d == 0) return 0;
    return d->Flags;
}

CFUNC SCRPCDLL_API uint64_t __STDCALL__ XilEnv_GetLinkSingleValueAddress(XILENV_LINK_DATA *Data)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    if (d == 0) return 0;
    return d->Address;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkSingleValueDimensionCount(XILENV_LINK_DATA *Data)
{
    return XilEnv_GetLinkArrayValueDimensionCount(Data, 0);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkSingleValueDimension(XILENV_LINK_DATA *Data, int DimNo)
{
    return XilEnv_GetLinkArrayValueDimension(Data, 0, DimNo);
}


CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_GetLinkSingleValueDataDouble(XILENV_LINK_DATA *Data)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    return ConvertToDouble(d);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkSingleValueDataDouble(XILENV_LINK_DATA *Data, double Value)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    return SetDouble(d, Value);
}

CFUNC SCRPCDLL_API int64_t __STDCALL__ XilEnv_GetLinkSingleValueDataInt(XILENV_LINK_DATA *Data)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    return ConvertToInt(d);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkSingleValueDataInt(XILENV_LINK_DATA *Data, int64_t Value)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    return SetInt(d, Value);
}

CFUNC SCRPCDLL_API uint64_t __STDCALL__ XilEnv_GetLinkSingleValueDataUint(XILENV_LINK_DATA *Data)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    return ConvertToUint(d);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkSingleValueDataUint(XILENV_LINK_DATA *Data, uint64_t Value)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    return SetUint(d, Value);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkSingleValueDataString(XILENV_LINK_DATA *Data, char *ret_Value, int MaxLen)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    return ConvertToString(d, ret_Value, MaxLen);
}

CFUNC SCRPCDLL_API const char* __STDCALL__ XilEnv_GetLinkSingleValueDataStringPtr(XILENV_LINK_DATA *Data)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    if ((d == NULL) || (d->Type != A2L_ELEM_TYPE_TEXT_REPLACE)) {
        return "";
    } else {
        return d->Value.String;
    }
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkSingleValueDataString(XILENV_LINK_DATA *Data, const char *Value)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    if ((d->Flags & (A2L_VALUE_FLAG_READ_ONLY | A2L_VALUE_FLAG_ONLY_VIRTUAL)) != 0) return -1;
    Data->Data = (void*)SetString((A2L_DATA*)Data->Data, -1, 0, d, Value);
    return (Data->Data == NULL) ? -1 : 0;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkSingleValueUnit(XILENV_LINK_DATA *Data, char *ret_Value, int MaxLen)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    if ((d == NULL) || (d->Type == A2L_ELEM_TYPE_TEXT_REPLACE) || 
        (d->SizeOfStruct <= sizeof(A2L_SINGLE_VALUE)) || ((d->Flags & A2L_VALUE_FLAG_HAS_UNIT) != A2L_VALUE_FLAG_HAS_UNIT)) {
        return -1;
    } else {
        return StringCopy(ret_Value, (char*)&d->Value + sizeof(d->Value), MaxLen);
    }
}

CFUNC SCRPCDLL_API const char* __STDCALL__ XilEnv_GetLinkSingleValueUnitPtr(XILENV_LINK_DATA *Data)
{
    A2L_SINGLE_VALUE *d = GetLinkSingleValueData(Data);
    if ((d == NULL) || (d->Type == A2L_ELEM_TYPE_TEXT_REPLACE) || 
        (d->SizeOfStruct <= sizeof(A2L_SINGLE_VALUE)) || ((d->Flags & A2L_VALUE_FLAG_HAS_UNIT) != A2L_VALUE_FLAG_HAS_UNIT)) {
        return "";
    } else {
        return (char*)&d->Value + sizeof(d->Value);
    }
}


// Array of values

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkArrayValueDataType(XILENV_LINK_DATA *Data, int ArrayNo, int Number)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    if (d == 0) return A2L_ELEM_TYPE_ERROR;
    return d->Type;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkArrayValueTargetDataType(XILENV_LINK_DATA *Data, int ArrayNo, int Number)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    if (d == 0) return A2L_DATA_TYPE_ERROR;
    return d->TargetType;
}

CFUNC SCRPCDLL_API uint32_t __STDCALL__ XilEnv_GetLinkArrayValueFlags(XILENV_LINK_DATA *Data, int ArrayNo, int Number)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    if (d == 0) return 0;
    return d->Flags;
}

CFUNC SCRPCDLL_API uint64_t __STDCALL__ XilEnv_GetLinkArrayValueAddress(XILENV_LINK_DATA *Data, int ArrayNo, int Number)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    if (d == 0) return 0;
    return d->Address;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkArrayValueDimensionCount(XILENV_LINK_DATA *Data, int ArrayNo)
{
    A2L_VALUE_ARRAY *Array;
    A2L_DATA *d = (A2L_DATA*)Data->Data;
    if (d == NULL) return -1;
    Array = GetArray(d, ArrayNo);
    if (Array == NULL) return -1;
    return Array->DimensionCount;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkArrayValueDimension(XILENV_LINK_DATA *Data, int ArrayNo, int DimNo)
{
    A2L_VALUE_ARRAY *Array;
    A2L_DATA *d = (A2L_DATA*)Data->Data;
    if (d == NULL) return -1;
    Array = GetArray(d, ArrayNo);
    if (Array == NULL) return -1;
    if ((DimNo < 0) && (DimNo >= Array->DimensionCount)) return -1;
    return Array->DimensionOrOffsets[DimNo];
}

CFUNC SCRPCDLL_API double __STDCALL__ XilEnv_GetLinkArrayValueDataDouble(XILENV_LINK_DATA *Data, int ArrayNo, int Number)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    return ConvertToDouble(d);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkArrayValueDataDouble(XILENV_LINK_DATA *Data, int ArrayNo, int Number, double Value)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    return SetDouble(d, Value);
}

CFUNC SCRPCDLL_API int64_t __STDCALL__ XilEnv_GetLinkArrayValueDataInt(XILENV_LINK_DATA *Data, int ArrayNo, int Number)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    return ConvertToInt(d);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkArrayValueDataInt(XILENV_LINK_DATA *Data, int ArrayNo, int Number, int64_t Value)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    return SetInt(d, Value);
}

CFUNC SCRPCDLL_API uint64_t __STDCALL__ XilEnv_GetLinkArrayValueDataUint(XILENV_LINK_DATA *Data, int ArrayNo, int Number)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    return ConvertToUint(d);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkArrayValueDataUint(XILENV_LINK_DATA *Data, int ArrayNo, int Number, uint64_t Value)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    return SetUint(d, Value);
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkArrayValueDataString(XILENV_LINK_DATA *Data, int ArrayNo, int Number, char *ret_Value, int MaxLen)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    return ConvertToString(d, ret_Value, MaxLen);
}

CFUNC SCRPCDLL_API const char* __STDCALL__ XilEnv_GetLinkArrayValueDataStringPtr(XILENV_LINK_DATA *Data, int ArrayNo, int Number)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    if ((d == NULL) || (d->Type != A2L_ELEM_TYPE_TEXT_REPLACE)) {
        return "";
    } else {
        return d->Value.String;
    }
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_SetLinkArrayValueDataString(XILENV_LINK_DATA *Data, int ArrayNo, int Number, const char *Value)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    if ((d->Flags & (A2L_VALUE_FLAG_READ_ONLY | A2L_VALUE_FLAG_ONLY_VIRTUAL)) != 0) return -1;
    Data->Data = (void*)SetString((A2L_DATA*)Data->Data, -1, 0, d, Value);
    return (Data->Data == NULL) ? -1 : 0;
}

CFUNC SCRPCDLL_API int __STDCALL__ XilEnv_GetLinkArrayValueUnit(XILENV_LINK_DATA *Data, int ArrayNo, int Number, char *ret_Value, int MaxLen)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    if ((d == NULL) || (d->Type == A2L_ELEM_TYPE_TEXT_REPLACE) || 
        (d->SizeOfStruct <= sizeof(A2L_SINGLE_VALUE)) || ((d->Flags & A2L_VALUE_FLAG_HAS_UNIT) != A2L_VALUE_FLAG_HAS_UNIT)) {
        return -1;
    } else {
        return StringCopy(ret_Value, (char*)&d->Value + sizeof(d->Value), MaxLen);
    }
}

CFUNC SCRPCDLL_API const char* __STDCALL__ XilEnv_GetLinkArrayValueUnitPtr(XILENV_LINK_DATA *Data, int ArrayNo, int Number)
{
    A2L_SINGLE_VALUE *d = GetLinkArrayValueData(Data, ArrayNo, Number);
    if ((d == NULL) || (d->Type == A2L_ELEM_TYPE_TEXT_REPLACE) || 
        (d->SizeOfStruct <= sizeof(A2L_SINGLE_VALUE)) || ((d->Flags & A2L_VALUE_FLAG_HAS_UNIT) != A2L_VALUE_FLAG_HAS_UNIT)) {
        return "";
    } else {
        return (char*)&d->Value + sizeof(d->Value);
    }
}


static void PrintSingleValue(A2L_SINGLE_VALUE *par_Value)
{
    switch (par_Value->Type) {
    case A2L_ELEM_TYPE_INT:
        printf("%lli", par_Value->Value.Int);
        break;
    case A2L_ELEM_TYPE_UINT:
        printf("%llu", par_Value->Value.Uint);
        break;
    case A2L_ELEM_TYPE_DOUBLE:
        printf("%f", par_Value->Value.Double);
        break;
    case A2L_ELEM_TYPE_PHYS_DOUBLE:
        printf("%f", par_Value->Value.Double);
        break;
    case A2L_ELEM_TYPE_TEXT_REPLACE:
        printf("\"%s\"", par_Value->Value.String);
        break;
    case A2L_ELEM_TYPE_ERROR:
    default:
        printf("error");
        break;
    }
    if ((par_Value->Type != A2L_ELEM_TYPE_TEXT_REPLACE) &&
        (par_Value->SizeOfStruct > sizeof(A2L_SINGLE_VALUE)) && ((par_Value->Flags & A2L_VALUE_FLAG_HAS_UNIT) == A2L_VALUE_FLAG_HAS_UNIT)) {
        printf (" %s", (char*)&par_Value->Value + sizeof(par_Value->Value));
    }
}


CFUNC SCRPCDLL_API void __STDCALL__ XilEnv_PrintLinkData(XILENV_LINK_DATA *par_Data)
{
    A2L_VALUE_ARRAY *ValueArray;
    A2L_SINGLE_VALUE *Value;
    A2L_DATA *Data = (A2L_DATA*)par_Data->Data;
    // print
    if (Data->ArrayCount == 0) { // single value
        Value = (A2L_SINGLE_VALUE*)(Data + 1);
        PrintSingleValue(Value);
        printf("\n");
    } else {
        int d, a;
        int *Offsets;
        //printf("Dim=%i: ", Data->ArrayCount);
        Offsets = (int*)(Data + 1);
        for (d = 0; d < Data->ArrayCount; d++) {
            if (Offsets[d] > 0) {
                ValueArray = (A2L_VALUE_ARRAY*)((char*)Data + Offsets[d]);
                printf("%c[%i]: ", 'X' + (char)d, ValueArray->ElementCount);
                for (a = 0; a < ValueArray->ElementCount; a++) {
                    int Offset = ValueArray->DimensionOrOffsets[ValueArray->DimensionCount + a];
                    Value = (A2L_SINGLE_VALUE*)((char*)Data + Offset);
                    if (a != 0) printf(", ");
                    /*if ((Value->Type == A2L_ELEM_TYPE_TEXT_REPLACE) && ((Value->Flags & (A2L_VALUE_FLAG_READ_ONLY | A2L_VALUE_FLAG_ONLY_VIRTUAL)) == 0)) {
                        printf ("Stop");
                    }*/
                    PrintSingleValue(Value);
                }
                printf("\n");
            } else {
                printf("%c: not exist\n", 'X' + (char)d);
            }
        }
    }
}


XILENV_LINK_DATA *XilEnvInternal_SetUpNewLinkData(void *par_Data, XILENV_LINK_DATA *par_Reuse)
{
    XILENV_LINK_DATA *Ret;
    A2L_DATA *Data = (A2L_DATA*)par_Data;
    if (par_Reuse != NULL) {
        A2L_DATA *OldData = (A2L_DATA*)(par_Reuse->Data);
        Ret = par_Reuse;
        if (OldData->StructSize >= Data->StructSize) {
            // it fits into the reused structure
            Data->StructSize = OldData->StructSize;
        } else {
            // enlarge structure to the new size
            Ret->Data = realloc(Ret->Data, Data->StructSize);
        }
    } else {
        // setup a new one
        Ret = (XILENV_LINK_DATA*)malloc(sizeof(XILENV_LINK_DATA));
        if (Ret == NULL) return NULL;
        Ret->Data = malloc(Data->StructSize);
        if (Ret->Data == NULL) {
            free(Ret);
            return NULL;
        }
    }
    MEMCPY(Ret->Data, Data, Data->StructPos);  // not use StructSize here!
    return Ret;

}
