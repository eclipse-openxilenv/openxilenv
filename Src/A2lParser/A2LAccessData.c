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


#include <stdint.h>
#include <inttypes.h>
#include "Platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "A2LValue.h"
#include "A2LBuffer.h"
#include "A2LParser.h"
#include "A2LAccess.h"
#include "A2LConvert.h"
#include "A2LLink.h"

#include "DebugInfos.h"
#include "ReadWriteValue.h"

#include "DelayedFetchData.h"

#include "A2LAccessData.h"

#define UNUSED(x) (void)(x)

#define A2L_DATA_MALLOC(s) my_malloc(s)
#define A2L_DATA_REALLOC(p, s) my_realloc(p, s)
#define A2L_DATA_FREE(p) my_free(p)


static int GetAlignments(A2L_LINK *par_Link, ASAP2_MODULE_DATA* par_Module, ASAP2_RECORD_LAYOUT *par_RecordLayout, A2L_ALIGNMENTS *ret_Alignments)
{
    if ((par_Link->Flags & A2L_LINK_DEFAUT_ALIGNMENT_FLAG) == A2L_LINK_DEFAUT_ALIGNMENT_FLAG) {
        ret_Alignments->AlignmentByte = 1;
        ret_Alignments->AlignmentWord = 2;
        ret_Alignments->AlignmentLong = 4;
        ret_Alignments->AlignmentInt64 = 8;
        ret_Alignments->AlignmentFloat16 = 2;
        ret_Alignments->AlignmentFloat32 = 4;
        ret_Alignments->AlignmentFloat64 = 8;
    } else {
        ret_Alignments->AlignmentByte = 1;
        ret_Alignments->AlignmentWord = 1;
        ret_Alignments->AlignmentLong = 1;
        ret_Alignments->AlignmentInt64 = 1;
        ret_Alignments->AlignmentFloat16 = 1;
        ret_Alignments->AlignmentFloat32 = 1;
        ret_Alignments->AlignmentFloat64 = 1;
    }

    if ((par_Link->Flags & A2L_LINK_IGNORE_MOD_COMMON_ALIGNMENTS_FLAG) == 0) {
        // first check the record layout.
        if (CheckIfFlagSetPos(par_Module->ModCommon.OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_BYTE)) {
            ret_Alignments->AlignmentByte = par_Module->ModCommon.OptionalParameter.AlignmentByte;
        }
        if (CheckIfFlagSetPos(par_Module->ModCommon.OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_WORD)) {
            ret_Alignments->AlignmentWord = par_Module->ModCommon.OptionalParameter.AlignmentWord;
        }
        if (CheckIfFlagSetPos(par_Module->ModCommon.OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_LONG)) {
            ret_Alignments->AlignmentLong = par_Module->ModCommon.OptionalParameter.AlignmentLong;
        }
        if (CheckIfFlagSetPos(par_Module->ModCommon.OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_INT64)) {
            ret_Alignments->AlignmentInt64 = par_Module->ModCommon.OptionalParameter.AlignmentInt64;
        }
        if (CheckIfFlagSetPos(par_Module->ModCommon.OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_FLOAT16_IEEE)) {
            ret_Alignments->AlignmentFloat16 = par_Module->ModCommon.OptionalParameter.AlignmentFloat16;
        }
        if (CheckIfFlagSetPos(par_Module->ModCommon.OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_FLOAT32_IEEE)) {
            ret_Alignments->AlignmentFloat32 = par_Module->ModCommon.OptionalParameter.AlignmentFloat32;
        }
        if (CheckIfFlagSetPos(par_Module->ModCommon.OptionalParameter.Flags, OPTPARAM_MOD_COMMON_ALIGNMENT_FLOAT64_IEEE)) {
            ret_Alignments->AlignmentFloat64 = par_Module->ModCommon.OptionalParameter.AlignmentFloat64;
        }
    }
    if ((par_Link->Flags & A2L_LINK_IGNORE_RECORD_LAYOUT_ALIGNMENTS_FLAG) == 0) {
        // then check the record layout (higher prio).
        if (CheckIfFlagSetPos(par_RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_BYTE)) {
            ret_Alignments->AlignmentByte = par_RecordLayout->OptionalParameter.AlignmentByte.AlignmentBorder;
        }
        if (CheckIfFlagSetPos(par_RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_WORD)) {
            ret_Alignments->AlignmentWord = par_RecordLayout->OptionalParameter.AlignmentWord.AlignmentBorder;
        }
        if (CheckIfFlagSetPos(par_RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_LONG)) {
            ret_Alignments->AlignmentLong = par_RecordLayout->OptionalParameter.AlignmentLong.AlignmentBorder;
        }
        if (CheckIfFlagSetPos(par_RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_INT64)) {
            ret_Alignments->AlignmentInt64 = par_RecordLayout->OptionalParameter.AlignmentInt64.AlignmentBorder;
        }
        if (CheckIfFlagSetPos(par_RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_FLOAT16_IEEE)) {
            ret_Alignments->AlignmentFloat16 = par_RecordLayout->OptionalParameter.AlignmentFloat16.AlignmentBorder;
        }
        if (CheckIfFlagSetPos(par_RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_FLOAT32_IEEE)) {
            ret_Alignments->AlignmentFloat32 = par_RecordLayout->OptionalParameter.AlignmentFloat32.AlignmentBorder;
        }
        if (CheckIfFlagSetPos(par_RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_ALIGNMENT_FLOAT64_IEEE)) {
            ret_Alignments->AlignmentFloat64 = par_RecordLayout->OptionalParameter.AlignmentFloat64.AlignmentBorder;
        }
    }
    return 0;
}

uint64_t CheckAlignment(uint64_t par_Address, int par_Alignment)
{
    uint64_t Ret;
    int Modulo = par_Address % par_Alignment;
    if (Modulo) {
        Ret = par_Address + par_Alignment - Modulo;
    } else {
        Ret = par_Address;
    }
    return Ret;
}

uint64_t CheckAlignmentByType(A2L_ALIGNMENTS *ret_Alignments, uint64_t par_Address, uint64_t par_baseAddress,
                              int par_Type)
{
    // Do not align a base address
    // maybe throw a warning?
    if (par_baseAddress == par_Address) {
        return par_Address;
    }
    switch (par_Type) {
    case A2L_ELEM_TARGET_TYPE_UINT8:
    case A2L_ELEM_TARGET_TYPE_INT8:
        return CheckAlignment(par_Address, ret_Alignments->AlignmentByte);
    case A2L_ELEM_TARGET_TYPE_UINT16:
    case A2L_ELEM_TARGET_TYPE_INT16:
        return CheckAlignment(par_Address, ret_Alignments->AlignmentWord);
    case A2L_ELEM_TARGET_TYPE_UINT32:
    case A2L_ELEM_TARGET_TYPE_INT32:
        return CheckAlignment(par_Address, ret_Alignments->AlignmentLong);
    case A2L_ELEM_TARGET_TYPE_FLOAT32_IEEE:
        return CheckAlignment(par_Address, ret_Alignments->AlignmentFloat32);
    case A2L_ELEM_TARGET_TYPE_FLOAT64_IEEE:
        return CheckAlignment(par_Address, ret_Alignments->AlignmentFloat64);
    case A2L_ELEM_TARGET_TYPE_UINT64:
    case A2L_ELEM_TARGET_TYPE_INT64:
        return CheckAlignment(par_Address, ret_Alignments->AlignmentInt64);
    default:
        return -1;
    }
}

static uint64_t IncAddress(uint64_t par_Address, int par_Type)
{
    switch (par_Type) {
    case A2L_ELEM_TARGET_TYPE_UINT8:
    case A2L_ELEM_TARGET_TYPE_INT8:
        return par_Address + 1;
    case A2L_ELEM_TARGET_TYPE_UINT16:
    case A2L_ELEM_TARGET_TYPE_INT16:
        return par_Address + 2;
    case A2L_ELEM_TARGET_TYPE_UINT32:
    case A2L_ELEM_TARGET_TYPE_INT32:
        return par_Address + 4;
    case A2L_ELEM_TARGET_TYPE_FLOAT32_IEEE:
        return par_Address + 4;
    case A2L_ELEM_TARGET_TYPE_FLOAT64_IEEE:
        return par_Address + 8;
    case A2L_ELEM_TARGET_TYPE_UINT64:
    case A2L_ELEM_TARGET_TYPE_INT64:
        return par_Address + 8;
    default:
        return 0;
    }
}

static uint64_t IncAddressNo(uint64_t par_Address, int par_Type, int par_No)
{
    switch (par_Type) {
    case A2L_ELEM_TARGET_TYPE_UINT8:
    case A2L_ELEM_TARGET_TYPE_INT8:
        return par_Address + par_No;
    case A2L_ELEM_TARGET_TYPE_UINT16:
    case A2L_ELEM_TARGET_TYPE_INT16:
        return par_Address + 2 * par_No;
    case A2L_ELEM_TARGET_TYPE_UINT32:
    case A2L_ELEM_TARGET_TYPE_INT32:
        return par_Address + 4 * par_No;
    case A2L_ELEM_TARGET_TYPE_FLOAT32_IEEE:
        return par_Address + 4 * par_No;
    case A2L_ELEM_TARGET_TYPE_FLOAT64_IEEE:
        return par_Address + 8 * par_No;
    case A2L_ELEM_TARGET_TYPE_UINT64:
    case A2L_ELEM_TARGET_TYPE_INT64:
        return par_Address + 8 * par_No;
    default:
        return 0;
    }
}

static void TypeAndValueConvert(enum A2L_ELEM_TARGET_TYPE par_Type, uint8_t *par_Bytes, A2L_SINGLE_VALUE *ret_RawValue)
{
    ret_RawValue->Flags = 0;
    ret_RawValue->SizeOfStruct = sizeof(A2L_SINGLE_VALUE);
    ret_RawValue->TargetType = par_Type;
    switch (par_Type) {
    case A2L_ELEM_TARGET_TYPE_UINT8:
        ret_RawValue->Value.Uint = *(uint8_t*)par_Bytes;
        ret_RawValue->Type = A2L_ELEM_TYPE_UINT;
        break;
    case A2L_ELEM_TARGET_TYPE_UINT16:
        ret_RawValue->Value.Uint = *(uint16_t*)par_Bytes;
        ret_RawValue->Type = A2L_ELEM_TYPE_UINT;
        break;
    case A2L_ELEM_TARGET_TYPE_UINT32:
        ret_RawValue->Value.Uint = *(uint32_t*)par_Bytes;
        ret_RawValue->Type = A2L_ELEM_TYPE_UINT;
        break;
    case A2L_ELEM_TARGET_TYPE_UINT64:
        ret_RawValue->Value.Uint = *(uint64_t*)par_Bytes;
        ret_RawValue->Type = A2L_ELEM_TYPE_UINT;
        break;
    case A2L_ELEM_TARGET_TYPE_INT8:
        ret_RawValue->Value.Int = *(int8_t*)par_Bytes;
        ret_RawValue->Type = A2L_ELEM_TYPE_INT;
        break;
    case A2L_ELEM_TARGET_TYPE_INT16:
        ret_RawValue->Value.Int = *(int16_t*)par_Bytes;
        ret_RawValue->Type = A2L_ELEM_TYPE_INT;
        break;
    case A2L_ELEM_TARGET_TYPE_INT32:
        ret_RawValue->Value.Int = *(int32_t*)par_Bytes;
        ret_RawValue->Type = A2L_ELEM_TYPE_INT;
        break;
    case A2L_ELEM_TARGET_TYPE_INT64:
        ret_RawValue->Value.Int = *(int64_t*)par_Bytes;
        ret_RawValue->Type = A2L_ELEM_TYPE_INT;
        break;
    case A2L_ELEM_TARGET_TYPE_FLOAT32_IEEE:
        ret_RawValue->Value.Double = *(float*)par_Bytes;
        ret_RawValue->Type = A2L_ELEM_TYPE_DOUBLE;
        break;
    case A2L_ELEM_TARGET_TYPE_FLOAT64_IEEE:
        ret_RawValue->Value.Double = *(double*)par_Bytes;
        ret_RawValue->Type = A2L_ELEM_TYPE_DOUBLE;
        break;
    default:
        ret_RawValue->Type = A2L_ELEM_TYPE_ERROR;
        break;
    }
}

static void SwapBytes(uint8_t *Bytes, int par_Size)
{
    uint8_t *Help = alloca(par_Size);
    int x;
    for (x = 0; x < par_Size; x++) {
        Help[x] = Bytes[x];
    }
    for (x = 0; x < par_Size; x++) {
        Bytes[par_Size - 1 - x] = Help[x];
    }
}

static void MaskBits(uint8_t *Bytes, int par_Size, uint64_t par_Mask)
{
    int x;
    uint64_t Help64 = 0;
    for (x = 0; x < par_Size; x++) {
        Help64 |= (uint64_t)Bytes[x] << (x * 8);
    }
    for (x = 0; x < 64; x++) {
        if (((par_Mask >> x) & 0x1) == 0x1) break;
    }
    Help64 = (Help64 & par_Mask) >> x;
    for (x = 0; x < par_Size; x++) {
        Bytes[x] = (uint8_t)(Help64 >> (x * 8));
    }
}

static uint64_t TranslateAddress(A2L_LINK *par_Link, uint64_t par_Address)
{
    uint64_t Ret;
    if ((par_Address + par_Link->BaseAddress) > par_Link->BaseAddress) {
        Ret = par_Address + par_Link->BaseAddress;
    } else {
        ThrowError (1, "Address = 0x%" PRIX64 " out of range (0x%" PRIX64 "...0x%" PRIX64 ")", par_Address, par_Link->BaseAddress, 0xFFFFFFFFFFFFFFFFLLU);
        Ret = 0;
    }
    return Ret;
}


static int ReqOneRawValue(int par_DataGroupNo, uint64_t par_Address, int par_Type)
{
    int Size = (int)IncAddress(0, par_Type);
    return AddToFetchDataBlockToDataBlockGroup(par_DataGroupNo, par_Address, Size, NULL);
}

static int GetAckOneRawValue(A2L_LINK *par_Link, uint64_t par_Address, uint32_t par_Flags, int par_Type, int par_ByteOrder, uint64_t par_Mask,
                             int par_DataGroupNo, A2L_SINGLE_VALUE *ret_RawValue)
{
    switch (par_Link->Type) {
    case LINK_TYPE_BINARY:
        return ReadValueFromImage(par_Link->To.FileImage, par_Address, par_Flags, par_Type, ret_RawValue);
    //case LINK_TYPE_EXT_PROCESS:
    default:
        {
            int Size = (int)IncAddress(0, par_Type);
            ret_RawValue->Flags = par_Flags;
            ret_RawValue->Value.Uint = 0;
            ret_RawValue->SizeOfStruct = sizeof(A2L_SINGLE_VALUE);
            par_Address = TranslateAddress(par_Link, par_Address);

            DATA_BLOCK *Current = GetDataBlockGroupListStartPointer(par_DataGroupNo);
            for (; Current != NULL; Current = Current->Next) {
                if ((par_Address >= Current->StartAddress) && ((par_Address + Size) <= (Current->StartAddress + Current->BlockSize))) {
                    uint8_t *Bytes = (uint8_t*)Current->Data + (par_Address - Current->StartAddress);
                    // Byte must be copied local because of multiple parameter with same address but different mask.
                    uint8_t Buffer[8];
                    if (Size > 8) Size = 8;
                    MEMCPY(Buffer, Bytes, Size);
                    Bytes = Buffer;
                    if (par_ByteOrder) SwapBytes(Bytes, Size);
                    MaskBits(Bytes, Size, par_Mask);
                    TypeAndValueConvert(par_Type, Bytes, ret_RawValue);
                    ret_RawValue->Address = par_Address;
                    return 0;
                }
            }
            break;
        }
    }
    return -1;
}

static int GetOneRawValue(A2L_LINK *par_Link, int par_DataGroupNo, int par_ReqOrAck,
                          uint64_t par_Address, uint32_t par_Flags, int par_Type, int par_ByteOrder, uint64_t par_Mask,
                          A2L_SINGLE_VALUE *ret_RawValue)
{
    if (par_ReqOrAck) {
        return ReqOneRawValue(par_DataGroupNo, par_Address, par_Type);
    } else {
        return GetAckOneRawValue(par_Link, par_Address, par_Flags, par_Type, par_ByteOrder, par_Mask,
                                 par_DataGroupNo, ret_RawValue);
    }
}


static int GetOneDataValue(A2L_LINK *par_Link, int par_DataGroupNo, int par_ReqOrAck,
                           uint64_t par_Address, uint32_t par_ValueFlags, int par_Type, int par_ByteOrder, uint64_t par_Mask,
                           int par_Flags, const char *par_ConvertName, A2L_SINGLE_VALUE *ret_Value)
{
    if (GetOneRawValue(par_Link, par_DataGroupNo, par_ReqOrAck, par_Address, par_ValueFlags, par_Type, par_ByteOrder, par_Mask, ret_Value)) {
        return -1;
    }
    if (!par_ReqOrAck && (par_DataGroupNo >= 0)) {  // if it is not the request and data existing
        if ((par_Flags & (A2L_GET_PHYS_FLAG | A2L_GET_TEXT_REPLACE_FLAG)) != 0) {
            ASAP2_DATABASE *Database = par_Link->Database;
            ASAP2_PARSER* Parser = Database->Asap2Ptr;
            ASAP2_MODULE_DATA* Module = Parser->Data.Modules[Database->SelectedModul];
            A2L_SINGLE_VALUE RawValue = *ret_Value;
            if (ConvertRawToPhys(Module, par_ConvertName, par_Flags, &RawValue, ret_Value) == 0) {
                return 0;
            } else {
                return -1;
            }
        }
    }
    return 0;
}

static A2L_DATA *AddSingleValueToStruct(A2L_DATA *Struct, A2L_SINGLE_VALUE *par_Value, int par_StructOffset, int par_Index)
{
    // check size of struct
    A2L_SINGLE_VALUE *SingleValue;
    int NeededSpace = par_Value->SizeOfStruct;

    if (Struct->StructSize < Struct->StructPos + NeededSpace) {
        Struct->StructSize = Struct->StructPos + NeededSpace;
        Struct = (A2L_DATA*)A2L_DATA_REALLOC(Struct, Struct->StructSize);
        if (Struct == NULL) return NULL;
    }
    SingleValue = (A2L_SINGLE_VALUE*)((char*)Struct + Struct->StructPos);
    //*ret_Value = SingleValue;
    MEMCPY(SingleValue, par_Value, NeededSpace);
    if (par_StructOffset > 0) *((int*)((char*)Struct + par_StructOffset) + par_Index) = Struct->StructPos;
    Struct->StructPos += NeededSpace;
    return Struct;
}

static A2L_DATA *AddValueArrayHeaderToStruct(A2L_DATA *Struct, int par_AxisType,
                                             int par_DimensionCount, int *par_Dimesions,
                                             int par_ElementCount,
                                             uint32_t par_Flags, int *ret_ValueOffsets)
{
    // check size of struct
    A2L_VALUE_ARRAY *ArrayHeader;
    int NeededSpace = sizeof(A2L_VALUE_ARRAY) + (sizeof(int) * (par_DimensionCount + par_ElementCount - 1));
    if ((par_ElementCount < 1) || (par_ElementCount > 100000)) {
        return NULL;
    }
    if (Struct->StructSize < Struct->StructPos + NeededSpace) {
        Struct->StructSize = Struct->StructPos + NeededSpace;
        Struct = (A2L_DATA*)A2L_DATA_REALLOC(Struct, Struct->StructSize);
        if (Struct == NULL) return NULL;
    }
    ArrayHeader = (A2L_VALUE_ARRAY*)((char*)Struct + Struct->StructPos);
    ArrayHeader->DimensionCount = par_DimensionCount;
    ArrayHeader->ElementCount = par_ElementCount;
    ArrayHeader->Flags = par_Flags;
    for (int x = 0; x < par_DimensionCount; x++) {
        ArrayHeader->DimensionOrOffsets[x] = par_Dimesions[x];
    }

    // store the start offset of the axis
    *((int*)(Struct + 1) + par_AxisType) = Struct->StructPos;

    *ret_ValueOffsets = Struct->StructPos + (sizeof(A2L_VALUE_ARRAY) + ((par_DimensionCount - 1) * (int)sizeof(int)));

    Struct->StructPos += NeededSpace;
    return Struct;
}

static void AddedAxisOffsetsToStruct(A2L_DATA *Struct, int par_Count)
{
    int x;
    for (x = 0; x < par_Count; x++) {
        *(int*)((char*)Struct + Struct->StructPos) = -1;
        Struct->StructPos += sizeof(int);  // Offset of one array
    }
}

A2L_DATA *GetAxisData(A2L_LINK *par_Link, ASAP2_DATABASE *Database, A2L_DATA *par_Data, int par_ArrayNr, int par_Dim, int par_Index, int par_Flags,
                      int par_State, int par_DimGroupNo, int par_DataGroupNo, const char **ret_Error, int par_DirectCallFlag)
{
    A2L_DATA *Ret;
    int XDim, YDim, ZDim;
    int XDimValid, YDimValid, ZDimValid;
    ASAP2_RECORD_LAYOUT *RecordLayout;
    ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
    ASAP2_MODULE_DATA* Module = Parser->Data.Modules[Database->SelectedModul];
    ASAP2_AXIS_PTS* Axis;
    int RecLayIdx;
    A2L_ALIGNMENTS Alignments;
    uint64_t BaseAddress, Address;
    A2L_SINGLE_VALUE *ValuePtr;
    int ByteOrder = 0;  // 0 -> MSB_LAST 1-> MSB_FIRST
    uint64_t BitMask = 0xFFFFFFFFFFFFFFFFLLU;
    int r, x, y;
    int ValueOffsets;
    char ValueStackBuffer[sizeof(A2L_SINGLE_VALUE) + 512];
    uint32_t ValueFlags = A2L_VALUE_FLAG_CALIBRATION;

    ValuePtr = (A2L_SINGLE_VALUE*)ValueStackBuffer;

    Ret = par_Data;

    if (par_Index >= Module->AxisPtsCounter) {
        *ret_Error = "is not a valid axis";
        goto __ERROUT;
    }
    Axis = Module->AxisPtss[par_Index];

    if (Axis->Address == 0) {
        *ret_Error = "no valid address";
        goto __ERROUT;
    }
    if ((par_Link->Flags & A2L_LINK_IGNORE_READ_ONLY_FLAG) != A2L_LINK_IGNORE_READ_ONLY_FLAG) {
        if (CheckIfFlagSetPos(Axis->OptionalParameter.Flags, OPTPARAM_AXIS_PTS_READ_ONLY)) {
            ValueFlags |= A2L_VALUE_FLAG_READ_ONLY;
        }
    }

    if (CheckIfFlagSetPos(Axis->OptionalParameter.Flags, OPTPARAM_AXIS_PTS_BYTE_ORDER)) {
        ByteOrder = Axis->OptionalParameter.ByteOrder;
    }
    BaseAddress = Address = Axis->Address;
    RecLayIdx = GetRecordLayoutIndex(Database, Axis->Deposit);
    if (RecLayIdx < 0) {
        *ret_Error = "no record layout";
        goto __ERROUT;
    }
    RecordLayout = Module->RecordLayouts[RecLayIdx];

    // YDim and ZDim not necessary Axis should have always 1 dimension
    XDim = YDim = ZDim = 1;
    if (par_Dim > 0) {
        if (par_Dim >= Axis->MaxAxisPoints) {
            XDim = par_Dim;
        } else {
            XDim = Axis->MaxAxisPoints;
        }
        XDimValid = 1;
    } else {
        XDim = Axis->MaxAxisPoints;
    }
    XDimValid = YDimValid = ZDimValid = 0;

    if (par_DirectCallFlag) {
        // Axis is not part of something, will be addressed direct
        if (Ret == NULL) {
            Ret = A2L_DATA_MALLOC(sizeof(A2L_DATA) + 1024);
            if (Ret == NULL) return NULL;
            Ret->StructSize = sizeof(A2L_DATA) + 1024;
        }
        Ret->StructPos = sizeof(A2L_DATA);
        Ret->Type = A2L_DATA_TYPE_VAL_BLK;
        Ret->ArrayCount = 1;
        AddedAxisOffsetsToStruct(Ret, 1);  // Offset of 1 array
    }

    GetAlignments(par_Link, Module, RecordLayout, &Alignments);

    if (CheckIfFlagSetPos(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_X)) {
        XDim = RecordLayout->OptionalParameter.FixNoAxisPtsX.NumberOfAxisPoints;
    }
    if (CheckIfFlagSetPos(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_Y)) {
        YDim = RecordLayout->OptionalParameter.FixNoAxisPtsY.NumberOfAxisPoints;
    }

    // walk through the array of entrys to figure out all addresses and dimensions
    for (r = 0; r < RecordLayout->OptionalParameter.ItemCount; r++) {
        ASAP2_RECORD_LAYOUT_ITEM *Item = RecordLayout->OptionalParameter.Items[r];
        switch (Item->ItemType) {
        case OPTPARAM_RECORDLAYOUT_FNC_VALUES:
            *ret_Error = " do not expect a \"FNC_VALUES\" inside a \"RECORD_LAYOUT\" for an \"AXIS_PTS\"";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_IDENTIFICATION:
            *ret_Error = "\"IDENTIFICATION\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_PTS_X:
            if (XDim <= 0) {
                *ret_Error = "the x size is <= 0";
                goto __ERROUT;
            }
            if ((par_State == STATE_READ_DATA) ||
                (par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ)) {
                if (par_DataGroupNo >= 0) {
                    if ((Ret = AddValueArrayHeaderToStruct(Ret, par_ArrayNr, 1, &XDim, XDim, ValueFlags, &ValueOffsets)) == NULL) {
                        *ret_Error = "out of memory";
                        goto __ERROUT;
                    }
                }
                for (x = 0; x < XDim; x++) {
                    Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.AxisPtsX.DataType);
                    if (GetOneDataValue(par_Link, par_DataGroupNo,
                                        (par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ),
                                        Address, ValueFlags, Item->Item.AxisPtsX.DataType, ByteOrder, BitMask,
                                        par_Flags, Axis->Conversion, ValuePtr)) {
                        *ret_Error = "cannot read x axis value";
                        goto __ERROUT;
                    }
                    if ((par_DataGroupNo >= 0) && (par_State == STATE_READ_DATA)) {
                        if ((Ret = AddSingleValueToStruct(Ret, ValuePtr, ValueOffsets, x)) == NULL) {
                            *ret_Error = "out of memory";
                            goto __ERROUT;
                        }
                    }
                    Address = IncAddress(Address, Item->Item.AxisPtsX.DataType);
                }
            } else {
                Address = IncAddressNo(Address, Item->Item.FncValues.Datatype, XDim);
            }
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_PTS_Y:
            if (YDim <= 0) {
                *ret_Error = "the y size is <= 0";
                goto __ERROUT;
            }
            if ((par_State == STATE_READ_DATA) ||
                (par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ)) {
                if (par_DataGroupNo >= 0) {
                    if ((Ret = AddValueArrayHeaderToStruct(Ret, par_ArrayNr, 1, &YDim, YDim, ValueFlags, &ValueOffsets)) == NULL) {
                        *ret_Error = "out of memory";
                        goto __ERROUT;
                    }
                }
                for (y = 0; y < YDim; y++) {
                    Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.AxisPtsY.DataType);
                    if (GetOneDataValue(par_Link, par_DataGroupNo,
                                        (par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ),
                                        Address, ValueFlags, Item->Item.AxisPtsY.DataType, ByteOrder, BitMask,
                                        par_Flags, Axis->Conversion, ValuePtr)) {
                        *ret_Error = "cannot read y axis value";
                        goto __ERROUT;
                    }
                    if ((par_DataGroupNo >= 0) && (par_State == STATE_READ_DATA)) {
                        if ((Ret = AddSingleValueToStruct(Ret, ValuePtr, ValueOffsets, y)) == NULL) {
                            *ret_Error = "out of memory";
                            goto __ERROUT;
                        }
                    }
                    Address = IncAddress(Address, Item->Item.AxisPtsY.DataType);
                }
            } else {
                Address = IncAddressNo(Address, Item->Item.FncValues.Datatype, XDim);
            }
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_PTS_Z:
            //Item->Item.AxisPtsZ;
            *ret_Error = "\"AXIS_PTS_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_X:
            //Item->Item.AxisRescaleX;
            *ret_Error = "\"AXIS_RESCALE_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Y:
            //Item->Item.AxisRescaleY;
            *ret_Error = "\"AXIS_RESCALE_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Z:
            //Item->Item.AxisRescaleZ;
            *ret_Error = "\"AXIS_RESCALE_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_X:
            Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.NoAxisPtsX.DataType);
            if (!XDimValid) {
                if (GetOneRawValue(par_Link, par_DimGroupNo, (par_State == STATE_READ_DIMENSIONS_REQ),
                                   Address, 0, Item->Item.NoAxisPtsX.DataType, ByteOrder, 0xFFFFFFFFFFFFFFFFLLU, ValuePtr)) {
                    *ret_Error = "cannot read number of x axis values";
                    goto __ERROUT;
                }
                if (!XDimValid && ((par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ) || (par_State == STATE_READ_DATA))) {
                    int Dim = ConvertRawValueToInt(ValuePtr);
                    if (Dim <= Axis->MaxAxisPoints) {
                        XDim = Dim;
                    } else {
                        XDim = Axis->MaxAxisPoints;
                    }
                }
            }
            Address = IncAddress(Address, Item->Item.NoAxisPtsX.DataType);
            break;
        case OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Y:
            Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.NoAxisPtsY.DataType);
            if (!YDimValid) {
                if (GetOneRawValue(par_Link, par_DimGroupNo, (par_State == STATE_READ_DIMENSIONS_REQ),
                                   Address, 0, Item->Item.NoAxisPtsY.DataType, ByteOrder, 0xFFFFFFFFFFFFFFFFLLU, ValuePtr)) {
                    *ret_Error = "cannot read number of y axis values";
                    goto __ERROUT;
                }
                if (!YDimValid && ((par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ) || (par_State == STATE_READ_DATA))) {
                    int Dim = ConvertRawValueToInt(ValuePtr);
                    if (Dim <= Axis->MaxAxisPoints) {
                        YDim = Dim;
                    } else {
                        YDim = Axis->MaxAxisPoints;
                    }
                }
            }
            Address = IncAddress(Address, Item->Item.NoAxisPtsY.DataType);
            break;
        case OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Z:
            //Item->Item.NoAxisPtsZ;
            *ret_Error = "\"NO_AXIS_PTS_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_RESCALE_X:
            //Item->Item.NoRescaleX;
            *ret_Error = "\"NO_RESCALE_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_RESCALE_Y:
            //Item->Item.NoRescaleY;
            *ret_Error = "\"NO_RESCALE_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_RESCALE_Z:
            //Item->Item.NoRescaleZ;
            *ret_Error = "\"NO_RESCALE_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SRC_ADDR_X:
            //Item->Item.SrcAddrX;
            *ret_Error = "\"SRC_ADDR_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SRC_ADDR_Y:
            //Item->Item.SrcAddrY;
            *ret_Error = "\"SRC_ADDR_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SRC_ADDR_Z:
            //Item->Item.SrcAddrZ;
            *ret_Error = "\"SRC_ADDR_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_X:
            //Item->Item.RipAddrX;
            *ret_Error = "\"RIP_ADDR_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_Y:
            //Item->Item.RipAddrY;
            *ret_Error = "\"RIP_ADDR_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_Z:
            //Item->Item.RipAddrZ;
            *ret_Error = "\"RIP_ADDR_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_W:
            //Item->Item.RipAddrW;
            *ret_Error = "\"RIP_ADDR_W\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SHIFT_OP_X:
            //Item->Item.ShiftOpX;
            *ret_Error = "\"SHIFT_OP_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SHIFT_OP_Y:
            //Item->Item.ShiftOpY;
            *ret_Error = "\"SHIFT_OP_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SHIFT_OP_Z:
            //Item->Item.ShiftOpZ;
            *ret_Error = "\"SHIFT_OP_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_OFFSET_X:
            //Item->Item.OffsetX;
            *ret_Error = "\"OFFSET_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_OFFSET_Y:
            //Item->Item.OffsetY;
            *ret_Error = "\"OFFSET_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_OFFSET_Z:
            //Item->Item.OffsetZ;
            *ret_Error = "\"OFFSET_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_DIST_OP_X:
            //Item->Item.DistOpX;
            *ret_Error = "\"DIST_OP_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_DIST_OP_Y:
            //Item->Item.DistOpY;
            *ret_Error = "\"DIST_OP_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_DIST_OP_Z:
            //Item->Item.DistOpZ;
            *ret_Error = "\"DIST_OP_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RESERVED:
            Address += Item->Item.Reserved.DataSize;
            break;
        default:
            break;
        }
    }
    return Ret;
__ERROUT:
    if (Ret != NULL) A2L_DATA_FREE(Ret);
    return NULL;
}

static A2L_DATA *GetAxisFixedData(A2L_DATA *par_Data, ASAP2_MODULE_DATA* par_Module, ASAP2_CHARACTERISTIC* par_Characteristic,
                                  int par_AxisNr,int par_Dim, int par_ValueFlags, A2L_SINGLE_VALUE *ValueBuffer, int par_Flags, const char **ret_Error)
{
    int x;
    A2L_DATA *Ret = par_Data;
    int ValueOffsets;

    if (par_Characteristic->OptionalParameter.AxisDescrCount > par_AxisNr) {
        ASAP2_CHARACTERISTIC_AXIS_DESCR *AxisDescr = par_Characteristic->OptionalParameter.AxisDescr[par_AxisNr];
        par_ValueFlags |= A2L_VALUE_FLAG_READ_ONLY;
        par_ValueFlags |= A2L_VALUE_FLAG_ONLY_VIRTUAL;
        if ((Ret = AddValueArrayHeaderToStruct(Ret, par_AxisNr, 1, &par_Dim, par_Dim, par_ValueFlags, &ValueOffsets)) == NULL) {
            *ret_Error = "out of memory";
            goto __ERROUT;
        }

        for (x = 0; x < par_Dim; x++) {
            ValueBuffer->SizeOfStruct = sizeof(A2L_SINGLE_VALUE);
            ValueBuffer->Flags = par_ValueFlags;
            ValueBuffer->Address = 0;
            ValueBuffer->TargetType = A2L_ELEM_TARGET_TYPE_NO_TYPE;
            ValueBuffer->Type = A2L_ELEM_TYPE_DOUBLE;
            ValueBuffer->Value.Double = AxisDescr->OptionalParameter.FixAxisParList[x];
            if ((par_Flags & (A2L_GET_PHYS_FLAG | A2L_GET_TEXT_REPLACE_FLAG)) != 0) {
                A2L_SINGLE_VALUE RawValue = *ValueBuffer;
                if (ConvertRawToPhys(par_Module, AxisDescr->Conversion, par_Flags, &RawValue, ValueBuffer) != 0) {
                    *ret_Error = "no valid compute method";
                    goto __ERROUT;
                }
            }
            if ((Ret = AddSingleValueToStruct(Ret, ValueBuffer, ValueOffsets, x)) == NULL) {
                *ret_Error = "out of memory";
                goto __ERROUT;
            }
        }
        return Ret;
    }
__ERROUT:
    if (Ret != NULL) A2L_DATA_FREE(Ret);
    return NULL;
}

static A2L_DATA *GetAxisParDist(A2L_DATA *par_Data, ASAP2_MODULE_DATA* par_Module, ASAP2_CHARACTERISTIC* par_Characteristic,
                                int par_AxisNr, int par_Dim, int par_ValueFlags, A2L_SINGLE_VALUE *ValueBuffer, int par_Flags, const char **ret_Error)
{
    int x;
    A2L_DATA *Ret = par_Data;
    int ValueOffsets;

    if (par_Characteristic->OptionalParameter.AxisDescrCount > par_AxisNr) {
        ASAP2_CHARACTERISTIC_AXIS_DESCR *AxisDescr = par_Characteristic->OptionalParameter.AxisDescr[par_AxisNr];
        if (AxisDescr->OptionalParameter.FixAxisParDist.Numberapo < par_Dim) {
            par_Dim = AxisDescr->OptionalParameter.FixAxisParDist.Numberapo;
        }
        par_ValueFlags |= A2L_VALUE_FLAG_READ_ONLY;
        par_ValueFlags |= A2L_VALUE_FLAG_ONLY_VIRTUAL;
        if ((Ret = AddValueArrayHeaderToStruct(Ret, par_AxisNr, 1, &par_Dim, par_Dim, par_ValueFlags, &ValueOffsets)) == NULL) {
            *ret_Error = "out of memory";
            goto __ERROUT;
        }

        for (x = 0; x < par_Dim; x++) {
            ValueBuffer->SizeOfStruct = sizeof(A2L_SINGLE_VALUE);
            ValueBuffer->Flags = par_ValueFlags;
            ValueBuffer->Address = 0;
            ValueBuffer->TargetType = A2L_ELEM_TARGET_TYPE_NO_TYPE;
            ValueBuffer->Type = A2L_ELEM_TYPE_DOUBLE;
            ValueBuffer->Value.Double = AxisDescr->OptionalParameter.FixAxisParDist.Offset +
                                 AxisDescr->OptionalParameter.FixAxisParDist.Distance * x;
            if ((par_Flags & (A2L_GET_PHYS_FLAG | A2L_GET_TEXT_REPLACE_FLAG)) != 0) {
                A2L_SINGLE_VALUE RawValue = *ValueBuffer;
                if (ConvertRawToPhys(par_Module, AxisDescr->Conversion, par_Flags, &RawValue, ValueBuffer) != 0) {
                    *ret_Error = "no valid compute method";
                    goto __ERROUT;
                }
            }
            if ((Ret = AddSingleValueToStruct(Ret, ValueBuffer, ValueOffsets, x)) == NULL) {
                *ret_Error = "out of memory";
                goto __ERROUT;
            }
        }
        return Ret;
    }
__ERROUT:
    if (Ret != NULL) A2L_DATA_FREE(Ret);
    return NULL;
}

static int GetAxisDimension(ASAP2_CHARACTERISTIC* par_Characteristic, int par_AxisNr, int *ret_Dim, char **ret_AxisName)
{
    if (par_Characteristic->OptionalParameter.AxisDescrCount > par_AxisNr) {
        ASAP2_CHARACTERISTIC_AXIS_DESCR *AxisDescr = par_Characteristic->OptionalParameter.AxisDescr[par_AxisNr];
        *ret_Dim = AxisDescr->MaxAxisPoints;
        if (CheckIfFlagSetPos(AxisDescr->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_AXIS_DESCR_FIX_AXIS_PAR)) {
            *ret_Dim = AxisDescr->OptionalParameter.FixAxisPar.Numberapo;
            return 0;
        } 
        if (CheckIfFlagSetPos(AxisDescr->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_AXIS_DESCR_FIX_AXIS_PAR_DIST)) {
            *ret_Dim = AxisDescr->OptionalParameter.FixAxisParDist.Numberapo;
            return 3;
        }
        if (CheckIfFlagSetPos(AxisDescr->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_AXIS_DESCR_FIX_AXIS_PAR_LIST)) {
            return 2;
        }
        if (CheckIfFlagSetPos(AxisDescr->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_AXIS_DESCR_AXIS_PTS_REF)) {
            if (ret_AxisName != NULL) *ret_AxisName = AxisDescr->OptionalParameter.AxisPoints;
            return 1;
        }
    }
    return -1;
}

static A2L_DATA *GetCharacteristicAxisData(A2L_LINK *par_Link, ASAP2_DATABASE *Database, A2L_DATA *par_Data,
                                           ASAP2_MODULE_DATA* par_Module,
                                           ASAP2_CHARACTERISTIC* Characteristic,
                                           ASAP2_RECORD_LAYOUT *RecordLayout,
                                           int par_AxisNr,
                                           uint32_t par_Flags,
                                           int par_PhysFlag,
                                           int *ret_Dim, int *ret_DimValid,
                                           A2L_SINGLE_VALUE *ValueBuffer,
                                           int par_State, int par_DimGroupNo, int par_DataGroupNo,
                                           const char **ret_Error)
{
    char *AxisName;
    int AxisIndex;
    A2L_DATA *Ret = par_Data;

    switch (GetAxisDimension(Characteristic, par_AxisNr, ret_Dim, &AxisName)) {
    case -1:
        switch(par_AxisNr) {
        case 0:
            if (CheckIfFlagSetPos(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_X)) {
                *ret_Dim = RecordLayout->OptionalParameter.FixNoAxisPtsX.NumberOfAxisPoints;
            }
            break;
        case 1:
            if (CheckIfFlagSetPos(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_Y)) {
                *ret_Dim = RecordLayout->OptionalParameter.FixNoAxisPtsY.NumberOfAxisPoints;
            }
            break;
        case 2:
            if (CheckIfFlagSetPos(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_Z)) {
                *ret_Dim = RecordLayout->OptionalParameter.FixNoAxisPtsZ.NumberOfAxisPoints;
            }
            break;
        default:
            break;
        }
        break;
    case 1:
        AxisIndex = GetAxisIndex(Database, AxisName);
        if (AxisIndex < 0) {
            *ret_Error = "unknown axis reference";
            goto __ERROUT;
        }
        if ((Ret = GetAxisData(par_Link, Database, Ret, par_AxisNr, *ret_Dim, AxisIndex, par_PhysFlag, par_State, par_DimGroupNo, par_DataGroupNo, ret_Error, 0)) == NULL) {
            goto __ERROUT;
        }
        break;
    case 2:
        if ((par_DataGroupNo >= 0) && (par_State == STATE_READ_DATA)) {
            if ((Ret = GetAxisFixedData(Ret, par_Module, Characteristic, par_AxisNr, *ret_Dim, par_Flags, ValueBuffer, par_PhysFlag, ret_Error)) == NULL) {
                goto __ERROUT;
            }
        }
        break;
    case 3:
        if ((par_DataGroupNo >= 0) && (par_State == STATE_READ_DATA)) {
            if ((Ret = GetAxisParDist(Ret, par_Module, Characteristic, par_AxisNr, *ret_Dim, par_Flags, ValueBuffer, par_PhysFlag, ret_Error)) == NULL) {
                goto __ERROUT;
            }
        }
        break;
    case 0:
        *ret_DimValid = 1;
        break;
    }
    return Ret;
__ERROUT:
    if (Ret != NULL) A2L_DATA_FREE(Ret);
    return NULL;

}

A2L_DATA *GetCharacteristicData(A2L_LINK *par_Link, ASAP2_DATABASE *Database, A2L_DATA *par_Reuse, int par_Index, int par_PhysFlag,
                                int par_State, int par_DimGroupNo, int par_DataGroupNo, const char **ret_Error)
{
    A2L_DATA *Ret;
    int XDim, YDim, ZDim;
    int XDimValid, YDimValid, ZDimValid;
    int DimensionCount = 0;
    int Dimensions[3];
    ASAP2_RECORD_LAYOUT *RecordLayout;
    ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
    ASAP2_MODULE_DATA* Module = Parser->Data.Modules[Database->SelectedModul];
    ASAP2_CHARACTERISTIC* Characteristic;
    int RecLayIdx;
    A2L_ALIGNMENTS Alignments;
    uint64_t BaseAddress, Address;
    A2L_SINGLE_VALUE *ValuePtr;
    int ByteOrder = 0;  // 0 -> MSB_LAST 1-> MSB_FIRST
    uint64_t BitMask = 0xFFFFFFFFFFFFFFFFLLU;
    int r, x, y, z;
    int ValueOffsets;
    char ValueStackBuffer[sizeof(A2L_SINGLE_VALUE) + 512];
    uint32_t ValueFlags = A2L_VALUE_FLAG_CALIBRATION;

    ValuePtr = (A2L_SINGLE_VALUE*)ValueStackBuffer;

    Ret = par_Reuse;

     *ret_Error = "no error";
    if ((par_Index < 0) || (par_Index >= Module->CharacteristicCounter)) {
        *ret_Error = "is not a valid characteristic";
        goto __ERROUT;
    }
    Characteristic = Module->Characteristics[par_Index];

    if (Characteristic->Address == 0) {
        *ret_Error = "no valid address";
        goto __ERROUT;
    }

    if ((par_Link->Flags & A2L_LINK_IGNORE_READ_ONLY_FLAG) != A2L_LINK_IGNORE_READ_ONLY_FLAG) {
        if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_READ_ONLY)) {
            ValueFlags |= A2L_VALUE_FLAG_READ_ONLY;
        }
    }
    if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_BYTE_ORDER)) {
        ByteOrder = Characteristic->OptionalParameter.ByteOrder;
    }
    if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_BIT_MASK)) {
        BitMask = Characteristic->OptionalParameter.BitMask;
    }

    BaseAddress = Address = Characteristic->Address;
    RecLayIdx = GetRecordLayoutIndex(Database, Characteristic->Deposit);
    if (RecLayIdx < 0) {
        *ret_Error = "no record layout";
        goto __ERROUT;
    }
    RecordLayout = Module->RecordLayouts[RecLayIdx];
    XDim = YDim = ZDim = 1;
    XDimValid = YDimValid = ZDimValid = 0;

    if (Ret == NULL) {
        Ret = A2L_DATA_MALLOC(sizeof(A2L_DATA) + 1024);
        if (Ret == NULL) return NULL;
        Ret->StructSize = sizeof(A2L_DATA) + 1024;
    }
    Ret->StructPos = sizeof(A2L_DATA);

    Ret->Type = Characteristic->Type;
 switch(Ret->Type) {
    case A2L_DATA_TYPE_MEASUREMENT:
    case A2L_DATA_TYPE_VALUE:
        Ret->ArrayCount = 0;
        DimensionCount = 0;
        break;
    case A2L_DATA_TYPE_ASCII:
    case A2L_DATA_TYPE_VAL_BLK:
        Ret->ArrayCount = 1;
        DimensionCount = 1;
        break;
    case A2L_DATA_TYPE_CURVE:
        Ret->ArrayCount = 2;
        DimensionCount = 1;
        break;
    case A2L_DATA_TYPE_MAP:
        Ret->ArrayCount = 3;
        DimensionCount = 2;
        break;
    case A2L_DATA_TYPE_CUBOID:
        Ret->ArrayCount = 4;
        DimensionCount = 3;
        break;
    case A2L_DATA_TYPE_CUBE_4:
        Ret->ArrayCount = 5;
        DimensionCount = 4;
        break;
    case A2L_DATA_TYPE_CUBE_5:
        Ret->ArrayCount = 6;
        DimensionCount = 5;
        break;
    default:
    case A2L_DATA_TYPE_ERROR:
        break;
    }

    GetAlignments(par_Link, Module, RecordLayout, &Alignments);

    if (Characteristic->Type == A2L_DATA_TYPE_VALUE) {
        Ret->ArrayCount = 0;
    } else if (Characteristic->Type == A2L_DATA_TYPE_ASCII) {
        Ret->ArrayCount = 0;
    } else if (Characteristic->Type == A2L_DATA_TYPE_VAL_BLK) {
        if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_MATRIX_DIM)) {
            XDim = Dimensions[0] = Characteristic->OptionalParameter.MatrixDim.x;
            YDim = Dimensions[1] = Characteristic->OptionalParameter.MatrixDim.y;
            ZDim = Dimensions[2] = Characteristic->OptionalParameter.MatrixDim.z;
            if (Dimensions[2] > 1) {
                DimensionCount = 3;
                ZDimValid = YDimValid = XDimValid = 1;
            } else if (Dimensions[1] > 1) {
                DimensionCount = 2;
                YDimValid = XDimValid = 1;
            } else if (Dimensions[0] > 1) {
                DimensionCount = 1;
                XDimValid = 1;
            } else {
                DimensionCount = 1; // at last one dimension with one element!
                XDimValid = 1;
            }
        } else if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_NUMBER)) {
            XDim = Dimensions[0] = Characteristic->OptionalParameter.Number;
            Ret->ArrayCount = 1; //2;  // array count!
            DimensionCount = 1;
        } else if (CheckIfFlagSetPos(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_X)) {
            XDim = Dimensions[0] = RecordLayout->OptionalParameter.FixNoAxisPtsX.NumberOfAxisPoints;
            Ret->ArrayCount = 2;  // array count!
            DimensionCount = 1;
        } else {
            *ret_Error = "no dimension defined";
            goto __ERROUT;
        }
        AddedAxisOffsetsToStruct(Ret, Ret->ArrayCount);
    } else if (Characteristic->Type == A2L_DATA_TYPE_CURVE) {
        Ret->ArrayCount = 2;
        AddedAxisOffsetsToStruct(Ret, 2);  // Offset of 2 array
        Ret = GetCharacteristicAxisData(par_Link, Database, Ret, Module, Characteristic, RecordLayout, 0, ValueFlags, par_PhysFlag, &XDim, &XDimValid, ValuePtr,
                                        par_State, par_DimGroupNo, par_DataGroupNo, ret_Error);
        if (Ret == NULL) goto __ERROUT;
        Dimensions[0] = XDim;
        DimensionCount = 1;
    } else if (Characteristic->Type == A2L_DATA_TYPE_MAP) {
        Ret->ArrayCount = 3;
        AddedAxisOffsetsToStruct(Ret, 3);  // Offset of 3 arrays
        Ret = GetCharacteristicAxisData(par_Link, Database, Ret, Module, Characteristic, RecordLayout, 0, ValueFlags, par_PhysFlag, &XDim, &XDimValid, ValuePtr, par_State, par_DimGroupNo, par_DataGroupNo, ret_Error);
        if (Ret == NULL) goto __ERROUT;
        Ret = GetCharacteristicAxisData(par_Link, Database, Ret, Module, Characteristic, RecordLayout, 1, ValueFlags, par_PhysFlag, &YDim, &YDimValid, ValuePtr, par_State, par_DimGroupNo, par_DataGroupNo, ret_Error);
        if (Ret == NULL) goto __ERROUT;
        Dimensions[0] = XDim;
        Dimensions[1] = YDim;
        DimensionCount = 2;
    }

    // walk through the array of entrys to figure out all addresses and dimensions
    for (r = 0;  r < RecordLayout->OptionalParameter.ItemCount; r++) {
        ASAP2_RECORD_LAYOUT_ITEM *Item = RecordLayout->OptionalParameter.Items[r];
        switch (Item->ItemType) {
        case OPTPARAM_RECORDLAYOUT_FNC_VALUES:
            if ((par_State == STATE_READ_DATA) ||
                (par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ)) {
                if ((Ret->ArrayCount >= 2) && (XDim <= 0)) {
                    *ret_Error = "the x size is <= 0";
                    goto __ERROUT;
                }
                if ((Ret->ArrayCount >= 3) && (YDim <= 0)) {
                    *ret_Error = "the y size is <= 0";
                    goto __ERROUT;
                }
                if (par_DataGroupNo >= 0) {
                    if (Ret->ArrayCount >= 1) {
                        Dimensions[0] = XDim;
                        Dimensions[1] = YDim;
                        Dimensions[2] = ZDim;
                        if ((Ret = AddValueArrayHeaderToStruct(Ret, Ret->ArrayCount - 1, DimensionCount, Dimensions, ZDim * YDim * XDim, ValueFlags, &ValueOffsets)) == NULL) {
                            *ret_Error = "out of memory";
                            goto __ERROUT;
                        }
                    } else {
                        ValueOffsets = -1;
                    }
                }
                Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.FncValues.Datatype);
                for (z = 0; z < ZDim; z++) {
                    for (y = 0; y < YDim; y++) {
                        for (x = 0; x < XDim; x++) {
                            uint64_t ElementAddress;
                            switch (Item->Item.FncValues.IndexMode) {
                            case 1: // COLUMN_DIR
                                ElementAddress = Address + (z * YDim * XDim + x * YDim + y) * IncAddress(0, Item->Item.FncValues.Datatype);
                                break;
                            case 2: // ROW_DIR
                                ElementAddress = Address + (z * YDim * XDim + y * XDim + x) * IncAddress(0, Item->Item.FncValues.Datatype);
                                break;
                            case 3: // ALTERNATE_CURVES
                            case 4: // ALTERNATE_WITH_X
                            case 5: // ALTERNATE_WITH_Y
                                // TODO: not implementet yet use same as COLUMN_DIR
                                ElementAddress = Address + (z * YDim * XDim + x * YDim + y) * IncAddress(0, Item->Item.FncValues.Datatype);
                                break;
                            }

                            if (GetOneDataValue(par_Link, par_DataGroupNo,
                                                (par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ),
                                                ElementAddress, ValueFlags, Item->Item.FncValues.Datatype, ByteOrder, BitMask,
                                                par_PhysFlag, Characteristic->Conversion, ValuePtr)) {
                                *ret_Error = "cannot read map value";
                                goto __ERROUT;
                            }
                            if ((par_DataGroupNo >= 0) && (par_State == STATE_READ_DATA)) {
                                if ((Ret = AddSingleValueToStruct(Ret, ValuePtr, ValueOffsets, z * YDim * XDim + y * XDim + x)) == NULL) {
                                    *ret_Error = "out of memory";
                                    goto __ERROUT;
                                }
                            }
                        }
                    }
                }
            } else {
                Address = IncAddressNo(Address, Item->Item.FncValues.Datatype, ZDim * YDim * XDim);
            }
            break;
        case OPTPARAM_RECORDLAYOUT_IDENTIFICATION:
            // Item->Item.Identification;
            *ret_Error = "\"IDENTIFICATION\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_PTS_X:
            if (XDim <= 0) {
                *ret_Error = "the x size is <= 0";
                goto __ERROUT;
            }
            if ((par_State == STATE_READ_DATA) ||
                (par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ)) {
                if (par_DataGroupNo >= 0) {
                    if ((Ret = AddValueArrayHeaderToStruct(Ret, 0, 1, &XDim, XDim, ValueFlags, &ValueOffsets)) == NULL) {
                        *ret_Error = "out of memory";
                        goto __ERROUT;
                    }
                }
                for (x = 0; x < XDim; x++) {
                    char *AxisConvert;
                    if (Characteristic->OptionalParameter.AxisDescrCount >= 1) {
                        AxisConvert = Characteristic->OptionalParameter.AxisDescr[0]->Conversion;
                    } else {
                        AxisConvert = "";
                    }
                    Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.AxisPtsX.DataType);
                    if (GetOneDataValue(par_Link, par_DataGroupNo,
                                        (par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ),
                                        Address, ValueFlags, Item->Item.AxisPtsX.DataType, ByteOrder, BitMask,
                                        par_PhysFlag, AxisConvert, ValuePtr)) {
                        *ret_Error = "cannot read x axis value";
                        goto __ERROUT;
                    }
                    if ((par_DataGroupNo >= 0) && (par_State == STATE_READ_DATA)) {
                        if ((Ret = AddSingleValueToStruct(Ret, ValuePtr, ValueOffsets, x)) == NULL) {
                            *ret_Error = "out of memory";
                            goto __ERROUT;
                        }
                    }
                    Address = IncAddress(Address, Item->Item.AxisPtsX.DataType);
                }
            } else {
                Address = IncAddressNo(Address, Item->Item.FncValues.Datatype, XDim);
            }
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_PTS_Y:
            if (YDim <= 0) {
                *ret_Error = "the y size is <= 0";
                goto __ERROUT;
            }
            if ((par_State == STATE_READ_DATA) ||
                (par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ)) {
                if (par_DataGroupNo >= 0) {
                    if ((Ret = AddValueArrayHeaderToStruct(Ret, 1, 1, &YDim, YDim, ValueFlags, &ValueOffsets)) == NULL) {
                        *ret_Error = "out of memory";
                        goto __ERROUT;
                    }
                }
                for (y = 0; y < YDim; y++) {
                    char *AxisConvert;
                    if (Characteristic->OptionalParameter.AxisDescrCount >= 2) {
                        AxisConvert = Characteristic->OptionalParameter.AxisDescr[1]->Conversion;
                    } else {
                        AxisConvert = "";
                    }
                    Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.AxisPtsY.DataType);
                    if (GetOneDataValue(par_Link, par_DataGroupNo,
                                        (par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ),
                                        Address, ValueFlags, Item->Item.AxisPtsY.DataType, ByteOrder, BitMask,
                                        par_PhysFlag, AxisConvert, ValuePtr)) {
                        *ret_Error = "cannot read y axis value";
                        goto __ERROUT;
                    }
                    if ((par_DataGroupNo >= 0) && (par_State == STATE_READ_DATA)) {
                        if ((Ret = AddSingleValueToStruct(Ret, ValuePtr, ValueOffsets, y)) == NULL) {
                            *ret_Error = "out of memory";
                            goto __ERROUT;
                        }
                    }
                    Address = IncAddress(Address, Item->Item.AxisPtsY.DataType);
                }
            } else {
                Address = IncAddressNo(Address, Item->Item.FncValues.Datatype, YDim);
            }
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_PTS_Z:
            //Item->Item.AxisPtsZ;
            *ret_Error = "\"AXIS_PTS_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_X:
            //Item->Item.AxisRescaleX;
            *ret_Error = "\"AXIS_RESCALE_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Y:
            //Item->Item.AxisRescaleY;
            *ret_Error = "\"AXIS_RESCALE_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Z:
            //Item->Item.AxisRescaleZ;
            *ret_Error = "\"AXIS_RESCALE_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_X:
            Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.NoAxisPtsX.DataType);
            if (GetOneRawValue(par_Link, par_DimGroupNo, (par_State == STATE_READ_DIMENSIONS_REQ),
                               Address, 0, Item->Item.NoAxisPtsX.DataType, ByteOrder, 0xFFFFFFFFFFFFFFFFLLU, ValuePtr)) {
                *ret_Error = "cannot read number of x axis values";
                goto __ERROUT;
            }
            if (!XDimValid && ((par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ) || (par_State == STATE_READ_DATA))) {
                XDim = ConvertRawValueToInt(ValuePtr);
            }
            Address = IncAddress(Address, Item->Item.NoAxisPtsX.DataType);
            break;
        case OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Y:
            Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.NoAxisPtsY.DataType);
            if (GetOneRawValue(par_Link, par_DimGroupNo, (par_State == STATE_READ_DIMENSIONS_REQ),
                               Address, 0, Item->Item.NoAxisPtsY.DataType, ByteOrder, 0xFFFFFFFFFFFFFFFFLLU,  ValuePtr)) {
                *ret_Error = "cannot read number of y axis values";
                goto __ERROUT;
            }
            if (!YDimValid && ((par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ) || (par_State == STATE_READ_DATA))) {
                YDim = ConvertRawValueToInt(ValuePtr);
            }
            Address = IncAddress(Address, Item->Item.NoAxisPtsY.DataType);
            break;
        case OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Z:
            //Item->Item.NoAxisPtsZ;
            *ret_Error = "\"NO_AXIS_PTS_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_RESCALE_X:
            //Item->Item.NoRescaleX;
            *ret_Error = "\"NO_RESCALE_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_RESCALE_Y:
            //Item->Item.NoRescaleY;
            *ret_Error = "\"NO_RESCALE_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_RESCALE_Z:
            //Item->Item.NoRescaleZ;
            *ret_Error = "\"NO_RESCALE_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SRC_ADDR_X:
            //Item->Item.SrcAddrX;
            *ret_Error = "\"SRC_ADDR_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SRC_ADDR_Y:
            //Item->Item.SrcAddrY;
            *ret_Error = "\"SRC_ADDR_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SRC_ADDR_Z:
            //Item->Item.SrcAddrZ;
            *ret_Error = "\"SRC_ADDR_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_X:
            //Item->Item.RipAddrX;
            *ret_Error = "\"RIP_ADDR_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_Y:
            //Item->Item.RipAddrY;
            *ret_Error = "\"RIP_ADDR_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_Z:
            //Item->Item.RipAddrZ;
            *ret_Error = "\"RIP_ADDR_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_W:
            //Item->Item.RipAddrW;
            *ret_Error = "\"RIP_ADDR_W\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SHIFT_OP_X:
            //Item->Item.ShiftOpX;
            *ret_Error = "\"SHIFT_OP_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SHIFT_OP_Y:
            //Item->Item.ShiftOpY;
            *ret_Error = "\"SHIFT_OP_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SHIFT_OP_Z:
            //Item->Item.ShiftOpZ;
            *ret_Error = "\"SHIFT_OP_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_OFFSET_X:
            //Item->Item.OffsetX;
            *ret_Error = "\"OFFSET_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_OFFSET_Y:
            //Item->Item.OffsetY;
            *ret_Error = "\"OFFSET_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_OFFSET_Z:
            //Item->Item.OffsetZ;
            *ret_Error = "\"OFFSET_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_DIST_OP_X:
            //Item->Item.DistOpX;
            *ret_Error = "\"DIST_OP_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_DIST_OP_Y:
            //Item->Item.DistOpY;
            *ret_Error = "\"DIST_OP_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_DIST_OP_Z:
            //Item->Item.DistOpZ;
            *ret_Error = "\"DIST_OP_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RESERVED:
            Address += Item->Item.Reserved.DataSize;
            break;
        default:
            break;
        }
    }
    return Ret;
__ERROUT:
    if (Ret != NULL) A2L_DATA_FREE(Ret);
    return NULL;
}


A2L_DATA *GetMeasurementData(A2L_LINK *par_Link, ASAP2_DATABASE *Database, A2L_DATA *par_Reuse, int par_Index, int par_Flags,
                             int par_State, int par_DataGroupNo, const char **ret_Error)
{
    A2L_DATA *Ret;
    int XDim, YDim, ZDim;
    int XDimValid, YDimValid, ZDimValid;
    ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
    ASAP2_MODULE_DATA* Module = Parser->Data.Modules[Database->SelectedModul];
    ASAP2_MEASUREMENT* Measurement;
    //A2L_ALIGNMENTS Alignments;
    uint64_t Address = 0;
    A2L_SINGLE_VALUE *ValuePtr;
    int ByteOrder = 0;  // 0 -> MSB_LAST 1-> MSB_FIRST
    uint64_t BitMask = 0xFFFFFFFFFFFFFFFFLLU;
    int x, y, z;
    int ValueOffsets;
    int DimensionCount = 0;
    int Dimensions[3];
    char ValueStackBuffer[sizeof(A2L_SINGLE_VALUE) + 512];
    uint32_t ValueFlags = A2L_VALUE_FLAG_MEASUREMENT;

    ValuePtr = (A2L_SINGLE_VALUE*)ValueStackBuffer;

    Ret = par_Reuse;

    *ret_Error = "no error";
    if ((par_Index < 0) || (par_Index >= Module->MeasurementCounter)) {
        *ret_Error = "is not a valid measurement";
        goto __ERROUT;
    }
    Measurement = Module->Measurements[par_Index];

    // figure out the address
    if (CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_ADDRESS)) {
        Address = Measurement->OptionalParameter.Address;
    }
    // normaly read only except ...
    if (!CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_WRITABLE_FLAG)) {
        ValueFlags |= A2L_VALUE_FLAG_READ_ONLY;
    }
    if (CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_BYTE_ORDER)) {
        ByteOrder = Measurement->OptionalParameter.ByteOrder;
    }
    if (CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_BIT_MASK)) {
        BitMask = Measurement->OptionalParameter.BitMask;
    }

    if (Address == 0) {
        *ret_Error = "no valid address";
        goto __ERROUT;
    }
    XDim = YDim = ZDim = 1;
    XDimValid = YDimValid = ZDimValid = 0;

    if (Ret == NULL) {
        Ret = A2L_DATA_MALLOC(sizeof(A2L_DATA) + 1024);
        if (Ret == NULL) return NULL;
        Ret->StructSize = sizeof(A2L_DATA) + 1024;
    }

    Ret->StructPos = sizeof(A2L_DATA);

    Ret->Type = A2L_DATA_TYPE_MEASUREMENT;

    Ret->ArrayCount = 0;
    if (CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_MATRIX_DIM)) {
        Ret->ArrayCount = 1;  // it is the array count!
        XDim = Dimensions[0] = Measurement->OptionalParameter.MatrixDim.x;
        YDim = Dimensions[1] = Measurement->OptionalParameter.MatrixDim.y;
        ZDim = Dimensions[2] = Measurement->OptionalParameter.MatrixDim.z;
        if (Dimensions[2] > 1) {
            DimensionCount = 3;
            ZDimValid = YDimValid = XDimValid = 1;
        } else if (Dimensions[1] > 1) {
            DimensionCount = 2;
            YDimValid = XDimValid = 1;
        } else if (Dimensions[0] > 1) {
            DimensionCount = 1;
            XDimValid = 1;
        } else {
            DimensionCount = 1;  // at last one dimension with one element!
            XDimValid = 1;
        }
        AddedAxisOffsetsToStruct(Ret, Ret->ArrayCount); // add one array offset
    }

    if ((par_State == STATE_READ_DATA) ||
        (par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ)) {
        if (Ret->ArrayCount >= 1) {
            if ((Ret = AddValueArrayHeaderToStruct(Ret, Ret->ArrayCount - 1, DimensionCount, Dimensions, ZDim * YDim * XDim, ValueFlags, &ValueOffsets)) == NULL) {
                *ret_Error = "out of memory";
                goto __ERROUT;
            }
        } else {
            ValueOffsets = -1;
        }
        for (z = 0; z < ZDim; z++) {
            for (y = 0; y < YDim; y++) {
                for (x = 0; x < XDim; x++) {
                    if (GetOneDataValue(par_Link, par_DataGroupNo,
                                        (par_State == STATE_READ_DIMENSIONS_ACK_DATA_REQ),
                                        Address, ValueFlags, Measurement->DataType , ByteOrder, BitMask,
                                        par_Flags, Measurement->ConversionIdent, ValuePtr)) {
                        *ret_Error = "cannot read map value";
                        goto __ERROUT;
                    }
                    if ((par_DataGroupNo >= 0) && (par_State == STATE_READ_DATA)) {
                        if ((Ret = AddSingleValueToStruct(Ret, ValuePtr, ValueOffsets, z * YDim * XDim + y * XDim + x)) == NULL) {
                            *ret_Error = "out of memory";
                            goto __ERROUT;
                        }
                    }
                    Address = IncAddress(Address, Measurement->DataType);
                }
            }
        }
    }
    return Ret;
__ERROUT:
    if (Ret != NULL) A2L_DATA_FREE(Ret);
    return NULL;
}


// SET

static int GetShift(uint64_t par_Mask)
{
    int x;
    for (x = 0; x < 64; x++) {
        if (((par_Mask >> x) & 0x1) == 0x1) break;
    }
    return x;
}

static int SetOneRawValue(A2L_LINK *par_Link, int par_State, int par_DataGroupMaskNo, int par_DataGroupNo, uint64_t par_Address, int par_Type, int par_ByteOrder, uint64_t par_Mask, A2L_SINGLE_VALUE *par_RawValue)
{
    UNUSED(par_ByteOrder);

    switch (par_Link->Type) {
    case LINK_TYPE_BINARY:
        return -1;
    case LINK_TYPE_EXT_PROCESS:
    {
        int Size = (int)IncAddress(0, par_Type);
        par_Address = TranslateAddress(par_Link, par_Address);

        if (par_State == STATE_READ_MASK_REQ) {
            if (par_Mask != 0xFFFFFFFFFFFFFFFFLLU) {   // first read
                AddToFetchDataBlockToDataBlockGroup(par_DataGroupMaskNo, par_Address, Size, NULL);
            }
        } else if (par_State == STATE_READ_MASK_ACK_WRITE_DATA_REQ) {
            union {
                uint64_t uqw;
                uint8_t Bytes[8];
            } Write;
            Write.uqw = 0;
            if ((par_RawValue->Type == A2L_ELEM_TYPE_DOUBLE) && (par_RawValue->TargetType == A2L_ELEM_TARGET_TYPE_FLOAT32_IEEE)) {
                   // float data type need a special copy method
                   float HelpFloat = (float)par_RawValue->Value.Double;
                   MEMCPY(Write.Bytes, (void*)&HelpFloat, Size);
               } else {
                   MEMCPY(Write.Bytes, (void*)&par_RawValue->Value, Size);
               }
            if (par_ByteOrder) SwapBytes((uint8_t*)&Write, Size);
            if (par_Mask != 0xFFFFFFFFFFFFFFFFLLU) {   // have a mask
                uint64_t Help = 0;
                int Shift = GetShift(par_Mask);
                GetDataFromDataBlockGroup(par_DataGroupMaskNo, par_Address, Size, (char*)&Help);
                Help = Help & ~par_Mask;
                Help = Help | ((Write.uqw << Shift) & par_Mask);
                Write.uqw = Help;
            }
            AddToFetchDataBlockToDataBlockGroup(par_DataGroupNo, par_Address, Size, (void*)Write.Bytes);
        }
        return 0;
    }
    default:
        return -1;

    }
}


static int SetOneDataValue(A2L_LINK *par_Link, int par_State, int par_DataGroupMaskNo, int par_DataGroupNo, uint64_t par_Address, int par_Type, int par_ByteOrder, uint64_t par_Mask,
                           const char *par_ConvertName, A2L_SINGLE_VALUE *par_Value)
{
    if ((par_Value->Flags & A2L_VALUE_FLAG_UPDATE) == A2L_VALUE_FLAG_UPDATE) {
        if (par_State != STATE_READ_MASK_REQ) {  // do not need to convert phys -> raw if read masked values
            if ((par_Value->Type == A2L_ELEM_TYPE_PHYS_DOUBLE) ||
                (par_Value->Type == A2L_ELEM_TYPE_TEXT_REPLACE)) {
                ASAP2_DATABASE *Database = par_Link->Database;
                ASAP2_PARSER* Parser = Database->Asap2Ptr;
                ASAP2_MODULE_DATA* Module = Parser->Data.Modules[Database->SelectedModul];
                if (ConvertPhysToRaw(Module, par_ConvertName, par_Value, par_Value) != 0) {
                    return -1;
                }
            }
        }
        if (SetOneRawValue(par_Link, par_State, par_DataGroupMaskNo, par_DataGroupNo, par_Address, par_Type, par_ByteOrder, par_Mask, par_Value)) {
            return -1;
        }
    }
    return 0;
}

static A2L_VALUE_ARRAY *GetValueArrayHeaderInsideStruct(A2L_DATA *Struct, int par_AxisType)
{
    A2L_VALUE_ARRAY *ArrayHeader;
    int Offset;
    int *Offsets;
    Offsets = (int*)(Struct + 1);
    Offset = Offsets[par_AxisType];
    if (Offset > (Struct->StructPos - (int)(sizeof(A2L_DATA) + sizeof(A2L_VALUE_ARRAY)))) return NULL;
    ArrayHeader = (A2L_VALUE_ARRAY*)((char*)Struct + Offset);
    return ArrayHeader;
}

static A2L_SINGLE_VALUE *GetValueInsideStruct(A2L_DATA *Struct, int par_Offset)
{
    A2L_SINGLE_VALUE *Value;
    if (par_Offset > (Struct->StructPos - (int)sizeof(A2L_SINGLE_VALUE))) return NULL;
    Value = (A2L_SINGLE_VALUE*)((char*)Struct + par_Offset);
    return Value;
}

int SetAxisData(A2L_LINK *par_Link, ASAP2_DATABASE *Database, A2L_DATA *par_Data, int par_ArrayNr, int par_Index, int par_State, int par_DataGroupMaskNo, int par_DataGroupNo, const char **ret_Error)
{
    int XDim, YDim, ZDim;
    int XDimValid, YDimValid, ZDimValid;
    ASAP2_RECORD_LAYOUT *RecordLayout;
    ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
    ASAP2_MODULE_DATA* Module = Parser->Data.Modules[Database->SelectedModul];
    ASAP2_AXIS_PTS* Axis;
    int RecLayIdx;
    A2L_ALIGNMENTS Alignments;
    uint64_t BaseAddress , Address;
    A2L_SINGLE_VALUE *ValuePtr;
    int ByteOrder = 0;  // 0 -> MSB_LAST 1-> MSB_FIRST
    uint64_t BitMask = 0xFFFFFFFFFFFFFFFFLLU;
    int r, x, y;

    if (par_Index >= Module->AxisPtsCounter) {
        *ret_Error = "is not a valid axis";
        goto __ERROUT;
    }
    Axis = Module->AxisPtss[par_Index];

    if ((par_Link->Flags & A2L_LINK_IGNORE_READ_ONLY_FLAG) != A2L_LINK_IGNORE_READ_ONLY_FLAG) {
        if (CheckIfFlagSetPos(Axis->OptionalParameter.Flags, OPTPARAM_AXIS_PTS_READ_ONLY)) {
            *ret_Error = "it is read only";
            return -1;
        }
    }
    if (CheckIfFlagSetPos(Axis->OptionalParameter.Flags, OPTPARAM_AXIS_PTS_BYTE_ORDER)) {
        ByteOrder = Axis->OptionalParameter.ByteOrder;
    }

    if (Axis->Address == 0) {
        *ret_Error = "no valid address";
        goto __ERROUT;
    }
    BaseAddress =  Address = Axis->Address;
    RecLayIdx = GetRecordLayoutIndex(Database, Axis->Deposit);
    if (RecLayIdx < 0) {
        *ret_Error = "no record layout";
        goto __ERROUT;
    }
    RecordLayout = Module->RecordLayouts[RecLayIdx];
    XDim = YDim = ZDim = Axis->MaxAxisPoints;
    XDimValid = YDimValid = ZDimValid = 0;

    GetAlignments(par_Link, Module, RecordLayout, &Alignments);

    // walk through the array of entrys to figure out all addresses and dimensions
    for (r = 0; r < RecordLayout->OptionalParameter.ItemCount; r++) {
        A2L_VALUE_ARRAY *ArrayHeader;
        ASAP2_RECORD_LAYOUT_ITEM *Item = RecordLayout->OptionalParameter.Items[r];
        switch (Item->ItemType) {
        case OPTPARAM_RECORDLAYOUT_FNC_VALUES:
            *ret_Error = " do not expect a \"FNC_VALUES\" inside a \"RECORD_LAYOUT\" for an \"AXIS_PTS\"";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_IDENTIFICATION:
            // Item->Item.Identification;
            *ret_Error = "\"IDENTIFICATION\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_PTS_X:
            if (XDim <= 0) {
                *ret_Error = "the x size is <= 0";
                goto __ERROUT;
            }
            if ((ArrayHeader = GetValueArrayHeaderInsideStruct(par_Data, par_ArrayNr)) == NULL) {
                *ret_Error = "struct offset error";
                goto __ERROUT;
            }
            for (x = 0; (x < XDim) && (x < ArrayHeader->ElementCount); x++) {
                if ((ValuePtr = GetValueInsideStruct(par_Data, ArrayHeader->DimensionOrOffsets[ArrayHeader->DimensionCount + x])) == NULL) {
                    *ret_Error = "struct offset error";
                    goto __ERROUT;
                }
                Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.AxisPtsX.DataType);
                if (par_State == STATE_READ_MASK_ACK_WRITE_DATA_REQ) {
                    if (CheckIfFlagSetPos(Axis->OptionalParameter.Flags, OPTPARAM_AXIS_PTS_READ_ONLY)) {
                        *ret_Error = "it is read only";
                        return -1;
                    }
                }
                if (SetOneDataValue(par_Link, par_State, par_DataGroupMaskNo, par_DataGroupNo, Address, Item->Item.AxisPtsX.DataType, ByteOrder, BitMask,
                                    Axis->Conversion, ValuePtr)) {
                    *ret_Error = "cannot write x axis value";
                    goto __ERROUT;
                }
                Address = IncAddress(Address, Item->Item.AxisPtsX.DataType);
            }
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_PTS_Y:
            if (YDim <= 0) {
                *ret_Error = "the y size is <= 0";
                goto __ERROUT;
            }
            if ((ArrayHeader = GetValueArrayHeaderInsideStruct(par_Data, par_ArrayNr)) == NULL) {
                *ret_Error = "struct offset error";
                goto __ERROUT;
            }
            for (y = 0; (y < YDim) && (y < ArrayHeader->ElementCount); y++) {
                if ((ValuePtr = GetValueInsideStruct(par_Data, ArrayHeader->DimensionOrOffsets[ArrayHeader->DimensionCount + y])) == NULL) {
                    *ret_Error = "struct offset error";
                    goto __ERROUT;
                }
                Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.AxisPtsY.DataType);
                if (par_State == STATE_READ_MASK_ACK_WRITE_DATA_REQ) {
                    if (CheckIfFlagSetPos(Axis->OptionalParameter.Flags, OPTPARAM_AXIS_PTS_READ_ONLY)) {
                        *ret_Error = "it is read only";
                        return -1;
                    }
                }
                if (SetOneDataValue(par_Link, par_State, par_DataGroupMaskNo, par_DataGroupNo, Address, Item->Item.AxisPtsY.DataType, ByteOrder, BitMask,
                                    Axis->Conversion, ValuePtr)) {
                    *ret_Error = "cannot write y axis value";
                    goto __ERROUT;
                }
                Address = IncAddress(Address, Item->Item.AxisPtsY.DataType);
            }
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_PTS_Z:
            //Item->Item.AxisPtsZ;
            *ret_Error = "\"AXIS_PTS_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_X:
            //Item->Item.AxisRescaleX;
            *ret_Error = "\"AXIS_RESCALE_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Y:
            //Item->Item.AxisRescaleY;
            *ret_Error = "\"AXIS_RESCALE_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Z:
            //Item->Item.AxisRescaleZ;
            *ret_Error = "\"AXIS_RESCALE_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_X:
            Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.NoAxisPtsX.DataType);
            if (!XDimValid) {
                if ((ArrayHeader = GetValueArrayHeaderInsideStruct(par_Data, par_ArrayNr)) == NULL) {
                    *ret_Error = "struct offset error";
                    goto __ERROUT;
                }
                XDim = ArrayHeader->ElementCount;
            }
            Address = IncAddress(Address, Item->Item.NoAxisPtsX.DataType);
            break;
        case OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Y:
            Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.NoAxisPtsY.DataType);
            if (!YDimValid) {
                if ((ArrayHeader = GetValueArrayHeaderInsideStruct(par_Data, par_ArrayNr)) == NULL) {
                    *ret_Error = "struct offset error";
                    goto __ERROUT;
                }
                YDim = ArrayHeader->ElementCount;
            }
            Address = IncAddress(Address, Item->Item.NoAxisPtsY.DataType);
            break;
        case OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Z:
            //Item->Item.NoAxisPtsZ;
            *ret_Error = "\"NO_AXIS_PTS_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_RESCALE_X:
            //Item->Item.NoRescaleX;
            *ret_Error = "\"NO_RESCALE_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_RESCALE_Y:
            //Item->Item.NoRescaleY;
            *ret_Error = "\"NO_RESCALE_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_RESCALE_Z:
            //Item->Item.NoRescaleZ;
            *ret_Error = "\"NO_RESCALE_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SRC_ADDR_X:
            //Item->Item.SrcAddrX;
            *ret_Error = "\"SRC_ADDR_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SRC_ADDR_Y:
            //Item->Item.SrcAddrY;
            *ret_Error = "\"SRC_ADDR_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SRC_ADDR_Z:
            //Item->Item.SrcAddrZ;
            *ret_Error = "\"SRC_ADDR_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_X:
            //Item->Item.RipAddrX;
            *ret_Error = "\"RIP_ADDR_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_Y:
            //Item->Item.RipAddrY;
            *ret_Error = "\"RIP_ADDR_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_Z:
            //Item->Item.RipAddrZ;
            *ret_Error = "\"RIP_ADDR_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_W:
            //Item->Item.RipAddrW;
            *ret_Error = "\"RIP_ADDR_W\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SHIFT_OP_X:
            //Item->Item.ShiftOpX;
            *ret_Error = "\"SHIFT_OP_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SHIFT_OP_Y:
            //Item->Item.ShiftOpY;
            *ret_Error = "\"SHIFT_OP_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SHIFT_OP_Z:
            //Item->Item.ShiftOpZ;
            *ret_Error = "\"SHIFT_OP_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_OFFSET_X:
            //Item->Item.OffsetX;
            *ret_Error = "\"OFFSET_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_OFFSET_Y:
            //Item->Item.OffsetY;
            *ret_Error = "\"OFFSET_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_OFFSET_Z:
            //Item->Item.OffsetZ;
            *ret_Error = "\"OFFSET_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_DIST_OP_X:
            //Item->Item.DistOpX;
            *ret_Error = "\"DIST_OP_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_DIST_OP_Y:
            //Item->Item.DistOpY;
            *ret_Error = "\"DIST_OP_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_DIST_OP_Z:
            //Item->Item.DistOpZ;
            *ret_Error = "\"DIST_OP_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RESERVED:
            Address += Item->Item.Reserved.DataSize;
            break;
        default:
            break;
        }
    }
    return 0;
__ERROUT:
    return -1;
}


int SetCharacteristicData(A2L_LINK *par_Link, ASAP2_DATABASE *Database, A2L_DATA *par_Data, int par_Index,
                          int par_State, int par_DataGroupMaskNo, int par_DataGroupNo, const char **ret_Error)
{
    int XDim, YDim, ZDim;
    int XDimValid, YDimValid, ZDimValid;
    int DimensionCount = 0;
    int Dimensions[3];
    ASAP2_RECORD_LAYOUT *RecordLayout;
    ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
    ASAP2_MODULE_DATA* Module = Parser->Data.Modules[Database->SelectedModul];
    ASAP2_CHARACTERISTIC* Characteristic;
    int RecLayIdx;
    A2L_ALIGNMENTS Alignments;
    uint64_t BaseAddress, Address;
    A2L_SINGLE_VALUE *ValuePtr;
    int ByteOrder = 0;  // 0 -> MSB_LAST 1-> MSB_FIRST
    uint64_t BitMask = 0xFFFFFFFFFFFFFFFFLLU;
    int r, x, y, z;
    int AxisIndex;
    char *AxisName;

    *ret_Error = "no error";
    if (par_Index >= Module->CharacteristicCounter) {
        *ret_Error = "is not a valid characteristic";
        goto __ERROUT;
    }
    Characteristic = Module->Characteristics[par_Index];

    if ((par_Link->Flags & A2L_LINK_IGNORE_READ_ONLY_FLAG) != A2L_LINK_IGNORE_READ_ONLY_FLAG) {
        if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_READ_ONLY)) {
            *ret_Error = "it is read only";
            return -1;
        }
    }
    if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_BYTE_ORDER)) {
        ByteOrder = Characteristic->OptionalParameter.ByteOrder;
    }
    if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_BIT_MASK)) {
        BitMask = Characteristic->OptionalParameter.BitMask;
    }

    if (Characteristic->Address == 0) {
        *ret_Error = "no valid address";
        goto __ERROUT;
    }
    BaseAddress = Address = Characteristic->Address;

    RecLayIdx = GetRecordLayoutIndex(Database, Characteristic->Deposit);
    if (RecLayIdx < 0) {
        *ret_Error = "no record layout";
        goto __ERROUT;
    }
    RecordLayout = Module->RecordLayouts[RecLayIdx];
    XDim = YDim = ZDim = 1;
    XDimValid = YDimValid = ZDimValid = 0;

    GetAlignments(par_Link, Module, RecordLayout, &Alignments);

    if (Characteristic->Type == A2L_DATA_TYPE_VALUE) {
        // do nothing
    } else if (Characteristic->Type == A2L_DATA_TYPE_ASCII) {
        if (par_Data->ArrayCount != 0) {
            *ret_Error = "\"ASCII\" type should not have a dimension";
            goto __ERROUT;
        }
    } else if (Characteristic->Type == A2L_DATA_TYPE_VAL_BLK) {
        if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_MATRIX_DIM)) {
            XDim = Dimensions[0] = Characteristic->OptionalParameter.MatrixDim.x;
            YDim = Dimensions[1] = Characteristic->OptionalParameter.MatrixDim.y;
            ZDim = Dimensions[2] = Characteristic->OptionalParameter.MatrixDim.z;
            if (Dimensions[2] > 1) {
                DimensionCount = 3;
            } else if (Dimensions[1] > 1) {
                DimensionCount = 2;
            } else if (Dimensions[0] > 1) {
                DimensionCount = 1;
            } else {
                DimensionCount = 0;
            }
        } else if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_NUMBER)) {
            XDim = Dimensions[0] = Characteristic->OptionalParameter.Number;
            DimensionCount = 1;
        } else if (CheckIfFlagSetPos(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_X)) {
            XDim = Dimensions[0] = RecordLayout->OptionalParameter.FixNoAxisPtsX.NumberOfAxisPoints;
            DimensionCount = 1;
        } else {
            *ret_Error = "no dimension defined";
            goto __ERROUT;
        }
    } else if (Characteristic->Type == A2L_DATA_TYPE_CURVE) {
        if (par_Data->ArrayCount != 2) {
            *ret_Error = "must have 2 dimensions";
            goto __ERROUT;
        }
        switch (GetAxisDimension(Characteristic, 0, &XDim, &AxisName)) {
        case -1:
            if (CheckIfFlagSetPos(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_X)) {
                XDim = Dimensions[0] = RecordLayout->OptionalParameter.FixNoAxisPtsX.NumberOfAxisPoints;
                XDimValid = 1;
            }
            break;
        case 1:
            AxisIndex = GetAxisIndex(Database, AxisName);
            if (AxisIndex < 0) {
                *ret_Error = "unknown axis reference";
                goto __ERROUT;
            }
            if (SetAxisData(par_Link, Database, par_Data, 0, AxisIndex, par_State, par_DataGroupMaskNo, par_DataGroupNo, ret_Error)) {
                goto __ERROUT;
            }
            break;
        case 2:
            // cannot write fixed data
            break;
        case 0:
            XDimValid = 1;
            break;
        }
    } else if (Characteristic->Type == A2L_DATA_TYPE_MAP) {
        if (par_Data->ArrayCount != 3) {
            *ret_Error = "must have 3 dimensions";
            goto __ERROUT;
        }
        switch (GetAxisDimension(Characteristic, 0, &XDim, &AxisName)) {
        case -1:
            if (CheckIfFlagSetPos(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_X)) {
                XDim = RecordLayout->OptionalParameter.FixNoAxisPtsX.NumberOfAxisPoints;
            }
            break;
        case 1:
            AxisIndex = GetAxisIndex(Database, AxisName);
            if (AxisIndex < 0) {
                *ret_Error = "unknown axis reference";
                goto __ERROUT;
            }
            if (SetAxisData(par_Link, Database, par_Data, 0, AxisIndex, par_State, par_DataGroupMaskNo, par_DataGroupNo, ret_Error)) {
                goto __ERROUT;
            }
            break;
        case 2:
            // cannot write fixed data
            break;
        case 0:
            XDimValid = 1;
            break;
        }
        switch (GetAxisDimension(Characteristic, 1, &YDim, &AxisName)) {
        case -1:
            if (CheckIfFlagSetPos(RecordLayout->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_Y)) {
                YDim = RecordLayout->OptionalParameter.FixNoAxisPtsY.NumberOfAxisPoints;
            }
            break;
        case 1:
            AxisIndex = GetAxisIndex(Database, AxisName);
            if (AxisIndex < 0) {
                *ret_Error = "unknown axis reference";
                goto __ERROUT;
            }
            if (SetAxisData(par_Link, Database, par_Data, 1, AxisIndex, par_State, par_DataGroupMaskNo, par_DataGroupNo, ret_Error)) {
                goto __ERROUT;
            }
            break;
        case 2:
            // cannot write fixed data
            break;
        case 0:
            YDimValid = 1;
            break;
        }
    }

    // walk through the array of entrys to figure out all addresses and dimensions
    for (r = 0;  r < RecordLayout->OptionalParameter.ItemCount; r++) {
        A2L_VALUE_ARRAY *ArrayHeader;
        ASAP2_RECORD_LAYOUT_ITEM *Item = RecordLayout->OptionalParameter.Items[r];
        switch (Item->ItemType) {
        case OPTPARAM_RECORDLAYOUT_FNC_VALUES:
            if ((par_Data->ArrayCount >= 2) && (XDim <= 0)) {
                *ret_Error = "the x size is <= 0";
                goto __ERROUT;
            }
            if ((par_Data->ArrayCount >= 3) && (YDim <= 0)) {
                *ret_Error = "the y size is <= 0";
                goto __ERROUT;
            }
            if (par_Data->ArrayCount > 0) {
                if ((ArrayHeader = GetValueArrayHeaderInsideStruct(par_Data, par_Data->ArrayCount - 1)) == NULL) {
                    *ret_Error = "struct offset error";
                    goto __ERROUT;
                }
            }
            Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.FncValues.Datatype);
            for (z = 0; z < ZDim; z++) {
                for (y = 0; y < YDim; y++) {
                    for (x = 0; x < XDim; x++) {
                        uint64_t ElementAddress;
                        switch (Item->Item.FncValues.IndexMode) {
                        case 1: // COLUMN_DIR
                            ElementAddress = Address + (z * YDim * XDim + x * YDim + y) * IncAddress(0, Item->Item.FncValues.Datatype);
                            break;
                        case 2: // ROW_DIR
                            ElementAddress = Address + (z * YDim * XDim + y * XDim + x) * IncAddress(0, Item->Item.FncValues.Datatype);
                            break;
                        case 3: // ALTERNATE_CURVES
                        case 4: // ALTERNATE_WITH_X
                        case 5: // ALTERNATE_WITH_Y
                            // TODO: not implementet yet use same as COLUMN_DIR that is wrong
                            ElementAddress = Address + (z * YDim * XDim + x * YDim + y) * IncAddress(0, Item->Item.FncValues.Datatype);
                            break;
                        }
                        if (par_Data->ArrayCount > 0) {
                            if ((ValuePtr = GetValueInsideStruct(par_Data, ArrayHeader->DimensionOrOffsets[ArrayHeader->DimensionCount + (z * YDim * XDim) + (y * XDim) + x])) == NULL) {
                                *ret_Error = "struct offset error";
                                goto __ERROUT;
                            }
                        } else {
                            ValuePtr = (A2L_SINGLE_VALUE*)(par_Data + 1);
                        }
                        if (SetOneDataValue(par_Link, par_State, par_DataGroupMaskNo, par_DataGroupNo, ElementAddress, Item->Item.FncValues.Datatype, ByteOrder, BitMask,
                                            Characteristic->Conversion, ValuePtr)) {
                            *ret_Error = "cannot write map value";
                            goto __ERROUT;
                        }
                    }
                }
            }
            break;
        case OPTPARAM_RECORDLAYOUT_IDENTIFICATION:
            // Item->Item.Identification;
            *ret_Error = "\"IDENTIFICATION\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_PTS_X:
            if (XDim <= 0) {
                *ret_Error = "the x size is <= 0";
                goto __ERROUT;
            }
            if ((ArrayHeader = GetValueArrayHeaderInsideStruct(par_Data, 0)) == NULL) {
                *ret_Error = "struct offset error";
                goto __ERROUT;
            }
            for (x = 0; (x < XDim) && (x < ArrayHeader->ElementCount); x++) {
                char *AxisConvert;
                if (Characteristic->OptionalParameter.AxisDescrCount >= 1) {
                    AxisConvert = Characteristic->OptionalParameter.AxisDescr[0]->Conversion;
                } else {
                    AxisConvert = "";
                }
                if ((ValuePtr = GetValueInsideStruct(par_Data, ArrayHeader->DimensionOrOffsets[ArrayHeader->DimensionCount + x])) == NULL) {
                    *ret_Error = "struct offset error";
                    goto __ERROUT;
                }
                Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.AxisPtsX.DataType);
                if (SetOneDataValue(par_Link, par_State, par_DataGroupMaskNo, par_DataGroupNo, Address, Item->Item.AxisPtsX.DataType, ByteOrder, BitMask,
                                    AxisConvert, ValuePtr)) {
                    *ret_Error = "cannot write x axis value";
                    goto __ERROUT;
                }
                Address = IncAddress(Address, Item->Item.AxisPtsX.DataType);
            }
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_PTS_Y:
            if (YDim <= 0) {
                *ret_Error = "the y size is <= 0";
                goto __ERROUT;
            }
            if ((ArrayHeader = GetValueArrayHeaderInsideStruct(par_Data, 1)) == NULL) {
                *ret_Error = "struct offset error";
                goto __ERROUT;
            }
            for (y = 0; (y < YDim) && (y < ArrayHeader->ElementCount); y++) {
                char *AxisConvert;
                if (Characteristic->OptionalParameter.AxisDescrCount >= 2) {
                    AxisConvert = Characteristic->OptionalParameter.AxisDescr[1]->Conversion;
                } else {
                    AxisConvert = "";
                }
                if ((ValuePtr = GetValueInsideStruct(par_Data, ArrayHeader->DimensionOrOffsets[ArrayHeader->DimensionCount + y])) == NULL) {
                    *ret_Error = "struct offset error";
                    goto __ERROUT;
                }
                Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.AxisPtsY.DataType);
                if (SetOneDataValue(par_Link,par_State, par_DataGroupMaskNo, par_DataGroupNo, Address, Item->Item.AxisPtsY.DataType, ByteOrder, BitMask,
                                    AxisConvert, ValuePtr)) {
                    *ret_Error = "cannot write y axis value";
                    goto __ERROUT;
                }
                Address = IncAddress(Address, Item->Item.AxisPtsY.DataType);
            }
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_PTS_Z:
            //Item->Item.AxisPtsZ;
            *ret_Error = "\"AXIS_PTS_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_X:
            //Item->Item.AxisRescaleX;
            *ret_Error = "\"AXIS_RESCALE_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Y:
            //Item->Item.AxisRescaleY;
            *ret_Error = "\"AXIS_RESCALE_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Z:
            //Item->Item.AxisRescaleZ;
            *ret_Error = "\"AXIS_RESCALE_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_X:
            Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.NoAxisPtsX.DataType);
            if (!XDimValid) {
                if ((ArrayHeader = GetValueArrayHeaderInsideStruct(par_Data, 0)) == NULL) {
                    *ret_Error = "struct offset error";
                    goto __ERROUT;
                }
                XDim = ArrayHeader->ElementCount;
            }
            Address = IncAddress(Address, Item->Item.NoAxisPtsX.DataType);
            break;
        case OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Y:
            Address = CheckAlignmentByType(&Alignments, Address, BaseAddress, Item->Item.NoAxisPtsY.DataType);
            if (!YDimValid) {
                if ((ArrayHeader = GetValueArrayHeaderInsideStruct(par_Data, 1)) == NULL) {
                    *ret_Error = "struct offset error";
                    goto __ERROUT;
                }
                YDim = ArrayHeader->ElementCount;
            }
            Address = IncAddress(Address, Item->Item.NoAxisPtsY.DataType);
            break;
        case OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Z:
            //Item->Item.NoAxisPtsZ;
            *ret_Error = "\"NO_AXIS_PTS_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_RESCALE_X:
            //Item->Item.NoRescaleX;
            *ret_Error = "\"NO_RESCALE_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_RESCALE_Y:
            //Item->Item.NoRescaleY;
            *ret_Error = "\"NO_RESCALE_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_NO_RESCALE_Z:
            //Item->Item.NoRescaleZ;
            *ret_Error = "\"NO_RESCALE_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SRC_ADDR_X:
            //Item->Item.SrcAddrX;
            *ret_Error = "\"SRC_ADDR_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SRC_ADDR_Y:
            //Item->Item.SrcAddrY;
            *ret_Error = "\"SRC_ADDR_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SRC_ADDR_Z:
            //Item->Item.SrcAddrZ;
            *ret_Error = "\"SRC_ADDR_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_X:
            //Item->Item.RipAddrX;
            *ret_Error = "\"RIP_ADDR_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_Y:
            //Item->Item.RipAddrY;
            *ret_Error = "\"RIP_ADDR_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_Z:
            //Item->Item.RipAddrZ;
            *ret_Error = "\"RIP_ADDR_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RIP_ADDR_W:
            //Item->Item.RipAddrW;
            *ret_Error = "\"RIP_ADDR_W\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SHIFT_OP_X:
            //Item->Item.ShiftOpX;
            *ret_Error = "\"SHIFT_OP_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SHIFT_OP_Y:
            //Item->Item.ShiftOpY;
            *ret_Error = "\"SHIFT_OP_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_SHIFT_OP_Z:
            //Item->Item.ShiftOpZ;
            *ret_Error = "\"SHIFT_OP_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_OFFSET_X:
            //Item->Item.OffsetX;
            *ret_Error = "\"OFFSET_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_OFFSET_Y:
            //Item->Item.OffsetY;
            *ret_Error = "\"OFFSET_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_OFFSET_Z:
            //Item->Item.OffsetZ;
            *ret_Error = "\"OFFSET_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_DIST_OP_X:
            //Item->Item.DistOpX;
            *ret_Error = "\"DIST_OP_X\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_DIST_OP_Y:
            //Item->Item.DistOpY;
            *ret_Error = "\"DIST_OP_Y\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_DIST_OP_Z:
            //Item->Item.DistOpZ;
            *ret_Error = "\"DIST_OP_Z\" not implemented yet";
            goto __ERROUT;
            break;
        case OPTPARAM_RECORDLAYOUT_RESERVED:
            Address += Item->Item.Reserved.DataSize;
            break;
        default:
            break;
        }
    }
    return 0;
__ERROUT:
    return -1;
}

// extract the values aut of the A2L_DATA structure
int GetLinkDataType(A2L_DATA *par_Data)
{
    if (par_Data == NULL) return -1;
    return par_Data->Type;
}

int GetLinkDataArrayCount(A2L_DATA *par_Data)
{
    if (par_Data == NULL) return -1;
    return par_Data->ArrayCount;
}

int GetLinkDataArraySize(A2L_DATA *par_Data, int par_ArrayNo)
{
    int *Offsets;
    A2L_VALUE_ARRAY *ArrayHeader;
    int ArrayHeaderOffset;
    if (par_Data == NULL) return -1;
    if (par_Data->ArrayCount < par_ArrayNo) return -1;
    Offsets = (int*)(par_Data + 1);
    ArrayHeaderOffset = Offsets[par_ArrayNo];
    if (ArrayHeaderOffset < 0) return -1;
    if (ArrayHeaderOffset > par_Data->StructPos) return -1;

    ArrayHeader = (A2L_VALUE_ARRAY*)((char*)par_Data +     // base structure
                                     ArrayHeaderOffset);   // + Offsets
    return ArrayHeader->ElementCount;
}

A2L_SINGLE_VALUE* GetLinkSingleValueData(A2L_DATA *par_Data)
{
    if (par_Data == NULL) return NULL;
    if (par_Data->ArrayCount != 0) return NULL;
    return (A2L_SINGLE_VALUE*)(par_Data + 1);
}

A2L_SINGLE_VALUE* GetLinkArrayValueData(A2L_DATA *par_Data, int par_ArrayNo, int par_ElementNo)
{
    int *Offsets;
    A2L_SINGLE_VALUE *Value;
    A2L_VALUE_ARRAY *ArrayHeader;
    int ArrayHeaderOffset;
    int ValueOffset;
    if (par_Data == NULL) return NULL;
    if (par_Data->ArrayCount < par_ArrayNo) return NULL;
    Offsets = (int*)(par_Data + 1);
    ArrayHeaderOffset = Offsets[par_ArrayNo];
    if (ArrayHeaderOffset < 0) return NULL;
    if (ArrayHeaderOffset > par_Data->StructPos) return NULL;

    ArrayHeader = (A2L_VALUE_ARRAY*)((char*)par_Data +            // base structure
                                     ArrayHeaderOffset);          // + Offsets
    if (par_ElementNo >= ArrayHeader->ElementCount) return NULL;
    ValueOffset = ArrayHeader->DimensionOrOffsets[ArrayHeader->DimensionCount + par_ElementNo];
    if (ValueOffset < 0) return NULL;
    if (ValueOffset > par_Data->StructPos) return NULL;
    Value = (A2L_SINGLE_VALUE*)((char*)par_Data +       // base structure
                                ValueOffset);           // + Offsets
    return Value;
}

void FreeA2lData(void *par_Data)
{
    A2L_DATA_FREE(par_Data);
}

A2L_DATA *DupA2lData(void *par_Data)
{
    A2L_DATA* Src = (A2L_DATA*)par_Data;
    A2L_DATA* Ret;
    if (Src != NULL) {
        Ret = (A2L_DATA*)A2L_DATA_MALLOC(Src->StructSize);
        MEMCPY(Ret, par_Data, Src->StructSize);
    } else {
        Ret = NULL;
    }
    return Ret;
}

int CompareIfA2lDataAreEqual(A2L_DATA *par_Data1, A2L_DATA *par_Data2)
{
    if (par_Data1->Type == par_Data2->Type) {
        if (par_Data1->StructPos == par_Data2->StructPos) {
            int x;
            for (x = sizeof(A2L_DATA); x < par_Data1->StructPos; x++) {
                if (((char*)par_Data1)[x] != ((char*)par_Data2)[x]) {
                    return 0;  // not equal
                }
            }
            return 1; // are equal
        }
    }
    return 0;  // not equal
}
