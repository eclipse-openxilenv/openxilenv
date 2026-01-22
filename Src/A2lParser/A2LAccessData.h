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
#include "Platform.h"
#include <stdint.h>
#include "A2LAccess.h"
#include "S19BinFile.h"

// now inside A2LValue.h
// same as ENUMDEF_CHARACTERISTIC_TYPE "VALUE 1 ASCII 2 VAL_BLK 3 CURVE 4 MAP 5 CUBOID 6 CUBE_4 7 CUBE_5 8 ? -1"
//enum A2L_ITEM_TYPE { A2L_ITEM_TYPE_MEASUREMENT = 0, A2L_ITEM_TYPE_VALUE = 1, A2L_ITEM_TYPE_ASCII = 2, A2L_ITEM_TYPE_VAL_BLK = 3, A2L_ITEM_TYPE_CURVE = 4, A2L_ITEM_TYPE_MAP = 5, A2L_ITEM_TYPE_CUBOID = 6, A2L_ITEM_TYPE_CUBE_4 = 7, A2L_ITEM_TYPE_CUBE_5 = 8, A2L_ITEM_TYPE_ERROR = -1 };

// now inside A2LValue.h
// same as #define ENUMDEF_DATATYPE "UBYTE 1 SBYTE 0 UWORD 3 SWORD 2 ULONG 5 SLONG 4 FLOAT32_IEEE 6 FLOAT64_IEEE 7 A_INT64 34 A_UINT64 35 FLOAT16_IEEE 36 ? -1"
// and same as blackboard data type numbers.
//enum A2L_DATA_TYPE { A2L_DATA_TYPE_UBYTE = 1, A2L_DATA_TYPE_SBYTE = 0, A2L_DATA_TYPE_UWORD = 3, A2L_DATA_TYPE_SWORD = 2, A2L_DATA_TYPE_ULONG = 5, A2L_DATA_TYPE_SLONG = 4, A2L_DATA_TYPE_FLOAT32_IEEE = 6, A2L_DATA_TYPE_FLOAT64_IEEE = 7, A2L_DATA_TYPE_A_INT64 = 34, A2L_DATA_TYPE_A_UINT64 = 35, A2L_DATA_TYPE_FLOAT16_IEEE = 36, A2L_DATA_TYPE_ERROR = -1 };


typedef struct {
    int StructPos;  // must be the first item
    int StructSize;
    enum A2L_DATA_TYPE Type;
    uint32_t Flags;
    int ArrayCount;
} A2L_DATA;

typedef struct {
    uint8_t AlignmentByte;
    uint8_t AlignmentWord;
    uint8_t AlignmentLong;
    uint8_t AlignmentInt64;
    uint8_t AlignmentFloat16;
    uint8_t AlignmentFloat32;
    uint8_t AlignmentFloat64;
    uint8_t Fill;
} A2L_ALIGNMENTS;

typedef struct {
    int Type;
    int Offset;
} A2L_CONVERSION;


typedef struct {
    int OffsetSymbol;    // -1 if no symbol defined
    uint64_t Address;
    int OffsetInput;
    int OffsetUnit;
    int FixedSize;     // -1 -> not fixed SizeAddress and SizeDataType must be set.
    uint64_t SizeAddress;
    enum A2L_ELEM_TARGET_TYPE SizeDataType;

    A2L_CONVERSION Conversion;

} A2L_AXIS;


typedef struct {
    int StructSize;
    int StructPos;
    int OffsetName;
    int OffsetSymbol;    // -1 if no symbol defined
    int OffsetUnit;
    uint64_t Address;
    enum A2L_DATA_TYPE Type;
    A2L_ALIGNMENTS Alignments;

    A2L_CONVERSION Conversion;

    int ArrayCount;     // 0....
    int OffsetDim;

    int AxisCount;
    int OffsetAxis;

} A2L_INFOS;


enum A2L_LINK_TYPE { LINK_TYPE_EMPTY = 0, LINK_TYPE_NONE = 1, LINK_TYPE_BINARY = 2, LINK_TYPE_EXT_PROCESS = 3,
                     LINK_TYPE_CCP = 4, LINK_TYPE_XCP_ON_CAN = 5, LINK_TYPE_XCP_ON_TCP_IP = 6, LINK_TYPE_XCP_ON_UDP_IP = 7};

typedef struct {
    CRITICAL_SECTION CriticalSection;
    int InUseFlag;
    int InUseCounter;
    int ShouldBeRemoved;
    int LoadedFlag;
    int CriticalSectionIsInit;
    int LinkNr;
    int Flags;
    enum A2L_LINK_TYPE Type;
    union {
        BIN_FILE_IMAGE *FileImage;  // Type == LINK_TYPE_BINARY
        int Pid;                    // Type == LINK_TYPE_EXT_PROCESS
        int ConnectionNr;           // Type == LINK_TYPE_CCP
        // Type == LINK_TYPE_XCP_ON_CAN
        // Type == LINK_TYPE_XCP_ON_TCP_IP
        // Type == LINK_TYPE_XCP_ON_UDP_IP
    } To;
    char *FileName;
    ASAP2_DATABASE *Database;
    uint64_t BaseAddress;
} A2L_LINK;

#ifndef SCRPCDLL_EXPORTS 
// this file will include into XilEnv and also into the XilEnvRpc interface but the ffunctions should nod declared during RPC build
#define STATE_READ_DIMENSIONS_REQ           1
#define STATE_READ_DIMENSIONS_ACK_DATA_REQ  2
#define STATE_READ_DATA                     3
#define STATE_SUCCESSFUL                    4
#define STATE_READ_MASK_REQ                 5
#define STATE_READ_MASK_ACK_WRITE_DATA_REQ  6
#define STATE_WRITE_DATA                    7
#define STATE_ERROR                        -1
A2L_DATA *GetAxisData(A2L_LINK *par_Link, ASAP2_DATABASE *Database, A2L_DATA *par_Data, int par_ArrayNr, int par_Dim, int par_Index, int par_Flags,
                      int par_State, int par_DimGroupNo, int par_DataGroupNo, const char **ret_Error, int par_DirectCallFlag);
A2L_DATA *GetCharacteristicData(A2L_LINK *par_Link, ASAP2_DATABASE *Database, A2L_DATA *par_Reuse, int par_Index, int par_Flags,
                                int par_State, int par_DimGroupNo, int par_DataGroupNo, const char **ret_Error);
A2L_DATA *GetMeasurementData(A2L_LINK *par_Link, ASAP2_DATABASE *Database, A2L_DATA *par_Reuse, int par_Index, int par_PhysFlag,
                             int par_State, int par_DataGroupNo, const char **ret_Error);

int SetAxisData(A2L_LINK *par_Link, ASAP2_DATABASE *Database, A2L_DATA *par_Data, int par_ArrayNr, int par_Index,
                int par_State, int par_DataGroupMaskNo, int par_DataGroupNo, const char **ret_Error);
int SetCharacteristicData(A2L_LINK *par_Link, ASAP2_DATABASE *Database, A2L_DATA *par_Data, int par_Index,
                          int par_State, int par_DataGroupMaskNo, int par_DataGroupNo, const char **ret_Error);

int GetLinkDataType(A2L_DATA *par_Data);

int GetLinkDataArrayCount(A2L_DATA *par_Data);

int GetLinkDataArraySize(A2L_DATA *par_Data, int par_ArrayNo);

A2L_SINGLE_VALUE* GetLinkSingleValueData(A2L_DATA *par_Data);

A2L_SINGLE_VALUE* GetLinkArrayValueData(A2L_DATA *par_Data, int par_ArrayNo, int par_ElementNo);

void FreeA2lData(void *par_Data);
A2L_DATA *DupA2lData(void *par_Data);
int CompareIfA2lDataAreEqual(A2L_DATA *par_Data1, A2L_DATA *par_Data2);

#endif
