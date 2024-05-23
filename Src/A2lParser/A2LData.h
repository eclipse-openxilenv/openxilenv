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


#ifndef A2LDATA_H
#define A2LDATA_H

#include <stdint.h>

#define STRING_BUFFER_BLOCK_SIZE   (1024*1024)

#define STRING_POINTER char*

#define IDENT char*

#define IDENT_POINTER char**

#define BUFFER_STRUCT_TYPE_MODULE_DATA      1
#define BUFFER_STRUCT_TYPE_RECORD_LAYOUT    2
#define BUFFER_STRUCT_TYPE_CHARACTERISTIC   3
#define BUFFER_STRUCT_TYPE_MEASUREMENT      4
#define BUFFER_STRUCT_TYPE_AXIS_PTS         5
//#define BUFFER_STRUCT_TYPE_AXIS_DESCR     6
#define BUFFER_STRUCT_TYPE_COMPU_METHOD     7
#define BUFFER_STRUCT_TYPE_COMPU_TAB        8
#define BUFFER_STRUCT_TYPE_FUNCTION         9
//#define BUFFER_STRUCT_TYPE_CANAPE_EXT      10


typedef struct {
    STRING_POINTER Comment;
    // Optional Parameter
    struct {
        uint64_t Flags;
#define OPTPARAM_MOD_COMMON_S_REC_LAYOUT              1
#define OPTPARAM_MOD_COMMON_DEPOSIT                   2
#define OPTPARAM_MOD_COMMON_BYTE_ORDER                3
#define OPTPARAM_MOD_COMMON_DATA_SIZE                 4
#define OPTPARAM_MOD_COMMON_ALIGNMENT_BYTE            5
#define OPTPARAM_MOD_COMMON_ALIGNMENT_WORD            6
#define OPTPARAM_MOD_COMMON_ALIGNMENT_LONG            7
#define OPTPARAM_MOD_COMMON_ALIGNMENT_INT64           8
#define OPTPARAM_MOD_COMMON_ALIGNMENT_FLOAT16_IEEE    9
#define OPTPARAM_MOD_COMMON_ALIGNMENT_FLOAT32_IEEE   10
#define OPTPARAM_MOD_COMMON_ALIGNMENT_FLOAT64_IEEE   11
        IDENT StandardRecordLayout;
        int Deposit;
        int ByteOrder;
        int DataSize;
        int AlignmentByte;
        int AlignmentWord;
        int AlignmentLong;
        int AlignmentInt64;
        int AlignmentFloat16;
        int AlignmentFloat32;
        int AlignmentFloat64;
    } OptionalParameter;
} ASAP2_MOD_COMMON;

typedef struct {
    int Position;
    int Datatype;
    int IndexMode;
    int Addresstype;
} ASAP2_RECORD_LAYOUT_FNC_VALUES;

typedef struct {
    int Position;
    int Datatype;
} ASAP2_RECORD_LAYOUT_IDENTIFICATION;

typedef struct {
    int Position;  // Position of the axis point values in the deposit structure (description of sequence of elements in the data record).
    int DataType;  // Data type of the axis point values
    int IndexIncr;  // Decreasing or increasing index with increasing addresses
    int Addressing;  // Addressing of the table values (see enum addrtype).
} ASAP2_RECORD_LAYOUT_AXIS_PTS;

typedef struct {
    int Position;  // position of the rescale axis point value pairs in the deposit structure (description of sequence of elements in the data record).
    int DataType;  // Data type of the rescale axis point values
    int MaxNumberOfRescalePairs;  // maximum number of rescaling axis point pairs (see NO_RESCALE_PTS_X/_Y/_Z)
    int IndexIncr;  //  Decreasing or increasing index with increasing addresses
    int Addressing;  // Addressing of the table values (see enum addrtype).
} ASAP2_RECORD_LAYOUT_AXIS_RESCALE;

typedef struct {
    int Position;   // Position of the number of axis points in the deposit structure
    int DataType;   // Data type of the number of axis points
} ASAP2_RECORD_LAYOUT_NO_AXIS_PTS;

typedef struct {
    int Position;   // position of the actual number of rescale axis point value pairs in the deposit structure (description of sequence of elements in the data record).
    int DataType;   // Data type of the number of rescale axis point value pairs
} ASAP2_RECORD_LAYOUT_NO_RESCALE;

typedef struct {
    int NumberOfAxisPoints;   // Dimensioning of characteristic curves or characteristic maps with a fixed number of axis points
} ASAP2_RECORD_LAYOUT_FIX_NO_AXIS_PTS;

typedef struct {
    int Position;  // Position of the address of the input quantity in the deposit structure.
    int DataType;  // Data type of the address.
} ASAP2_RECORD_LAYOUT_SRC_ADDR;

typedef struct {
    int Position;  // Position of the address to the result of the ECU-internal interpolation (see below) in the deposit structure.
    int DataType;  // Data type of the address.
} ASAP2_RECORD_LAYOUT_RIP_ADDR;

typedef struct {
    int Position;  // Position of the shift operand in the deposit structure.
    int DataType;  // Data type of the shift operand.
} ASAP2_RECORD_LAYOUT_SHIFT_OP;

typedef struct {
    int Position;  // Position of the 'offset' parameter in the deposit structure to compute the X-axis points for fixed characteristic curves and fixed characteristic maps.
    int DataType;  // Data type of the 'offset' parameter.
} ASAP2_RECORD_LAYOUT_OFFSET;

typedef struct {
    int Position;  // Position of the distance operand in the deposit structure.
    int DataType;  // Data type of the distance operand.
} ASAP2_RECORD_LAYOUT_DIST_OP;

typedef struct {
    int AlignmentBorder; // describes the border at which the value is aligned to, i.e. its
                         // memory address must be dividable by the value Alignment-Border
} ASAP2_RECORD_LAYOUT_ALIGNMENT;

typedef struct {
    int Position;    // Position of the reserved parameter in the deposit structure
    int DataSize;    // Word length of the reserved parameter
} ASAP2_RECORD_LAYOUT_RESERVED;

typedef struct {
    int Version;
    struct {
        unsigned int Flags;
#define OPTPARAM_IF_DATA_CANAPE_EXT_LINK_MAP               0x1ULL
        IDENT Label;
        uint64_t Address;
        int FileOffsetAddress;
        int  AddressExtension;  /* address extension of referenced linker map symbol */
        int FlagRelToDS;               /* flag: address is relative to DS */
        uint64_t AddressOffset;  /* address offset to address of referenced linker map symbol */
        int FlagDataTypeValid;  /* flag: datatype is valid */
        int CANapeDataType;  /* CANape internal representation of datatyp */
        int BitOffset;  /* bit offset of referenced linker map symbol */
#define OPTPARAM_IF_DATA_CANAPE_EXT_DISPLAY               0x2ULL
        unsigned int DisplayColor;
        double DisplayPhysMin;
        double DisplayPhysMax;
    } OptionalParameter;
} ASAP2_IF_DATA_CANAPE_EXT;

typedef struct {
    int ItemType;
    int Position;    // Position of the parameter in the deposit structure
    union {
#define OPTPARAM_RECORDLAYOUT_FNC_VALUES               1
        ASAP2_RECORD_LAYOUT_FNC_VALUES FncValues;
#define OPTPARAM_RECORDLAYOUT_IDENTIFICATION           2
        ASAP2_RECORD_LAYOUT_IDENTIFICATION Identification;
#define OPTPARAM_RECORDLAYOUT_AXIS_PTS_X               3
        ASAP2_RECORD_LAYOUT_AXIS_PTS AxisPtsX;
#define OPTPARAM_RECORDLAYOUT_AXIS_PTS_Y               4
        ASAP2_RECORD_LAYOUT_AXIS_PTS AxisPtsY;
#define OPTPARAM_RECORDLAYOUT_AXIS_PTS_Z               5
        ASAP2_RECORD_LAYOUT_AXIS_PTS AxisPtsZ;
#define OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_X           6
        ASAP2_RECORD_LAYOUT_AXIS_RESCALE AxisRescaleX;
#define OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Y           7
        ASAP2_RECORD_LAYOUT_AXIS_RESCALE AxisRescaleY;
#define OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Z           8
        ASAP2_RECORD_LAYOUT_AXIS_RESCALE AxisRescaleZ;
#define OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_X            9
        ASAP2_RECORD_LAYOUT_NO_AXIS_PTS NoAxisPtsX;
#define OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Y            10
        ASAP2_RECORD_LAYOUT_NO_AXIS_PTS NoAxisPtsY;
#define OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Z            11
        ASAP2_RECORD_LAYOUT_NO_AXIS_PTS NoAxisPtsZ;
#define OPTPARAM_RECORDLAYOUT_NO_RESCALE_X             12
        ASAP2_RECORD_LAYOUT_NO_RESCALE NoRescaleX;
#define OPTPARAM_RECORDLAYOUT_NO_RESCALE_Y             13
        ASAP2_RECORD_LAYOUT_NO_RESCALE NoRescaleY;
#define OPTPARAM_RECORDLAYOUT_NO_RESCALE_Z             14
        ASAP2_RECORD_LAYOUT_NO_RESCALE NoRescaleZ;
#define OPTPARAM_RECORDLAYOUT_SRC_ADDR_X               15
        ASAP2_RECORD_LAYOUT_SRC_ADDR SrcAddrX;
#define OPTPARAM_RECORDLAYOUT_SRC_ADDR_Y               16
        ASAP2_RECORD_LAYOUT_SRC_ADDR SrcAddrY;
#define OPTPARAM_RECORDLAYOUT_SRC_ADDR_Z               17
        ASAP2_RECORD_LAYOUT_SRC_ADDR SrcAddrZ;
#define OPTPARAM_RECORDLAYOUT_RIP_ADDR_X               18
        ASAP2_RECORD_LAYOUT_RIP_ADDR RipAddrX;
#define OPTPARAM_RECORDLAYOUT_RIP_ADDR_Y               19
        ASAP2_RECORD_LAYOUT_RIP_ADDR RipAddrY;
#define OPTPARAM_RECORDLAYOUT_RIP_ADDR_Z               20
        ASAP2_RECORD_LAYOUT_RIP_ADDR RipAddrZ;
#define OPTPARAM_RECORDLAYOUT_RIP_ADDR_W               21
        ASAP2_RECORD_LAYOUT_RIP_ADDR RipAddrW;
#define OPTPARAM_RECORDLAYOUT_SHIFT_OP_X               22
        ASAP2_RECORD_LAYOUT_SHIFT_OP ShiftOpX;
#define OPTPARAM_RECORDLAYOUT_SHIFT_OP_Y               23
        ASAP2_RECORD_LAYOUT_SHIFT_OP ShiftOpY;
#define OPTPARAM_RECORDLAYOUT_SHIFT_OP_Z               24
        ASAP2_RECORD_LAYOUT_SHIFT_OP ShiftOpZ;
#define OPTPARAM_RECORDLAYOUT_OFFSET_X                 25
        ASAP2_RECORD_LAYOUT_OFFSET OffsetX;
#define OPTPARAM_RECORDLAYOUT_OFFSET_Y                 26
        ASAP2_RECORD_LAYOUT_OFFSET OffsetY;
#define OPTPARAM_RECORDLAYOUT_OFFSET_Z                 27
        ASAP2_RECORD_LAYOUT_OFFSET OffsetZ;
#define OPTPARAM_RECORDLAYOUT_DIST_OP_X                28
        ASAP2_RECORD_LAYOUT_DIST_OP DistOpX;
#define OPTPARAM_RECORDLAYOUT_DIST_OP_Y                29
        ASAP2_RECORD_LAYOUT_DIST_OP DistOpY;
#define OPTPARAM_RECORDLAYOUT_DIST_OP_Z                30
        ASAP2_RECORD_LAYOUT_DIST_OP DistOpZ;
#define OPTPARAM_RECORDLAYOUT_RESERVED                 31
        ASAP2_RECORD_LAYOUT_RESERVED Reserved;
    } Item;
} ASAP2_RECORD_LAYOUT_ITEM;

typedef struct {
    IDENT Ident;     // Must be first structure member!!!!
    int StructType;  //  Must be second structure member!!!!

    // Optional parameter
    struct {
        uint64_t Flags;
#define OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_X        32
        ASAP2_RECORD_LAYOUT_FIX_NO_AXIS_PTS FixNoAxisPtsX;
#define OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_Y        33
        ASAP2_RECORD_LAYOUT_FIX_NO_AXIS_PTS FixNoAxisPtsY;
#define OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_Z        34
        ASAP2_RECORD_LAYOUT_FIX_NO_AXIS_PTS FixNoAxisPtsZ;
#define OPTPARAM_RECORDLAYOUT_ALIGNMENT_BYTE           35
        ASAP2_RECORD_LAYOUT_ALIGNMENT AlignmentByte;
#define OPTPARAM_RECORDLAYOUT_ALIGNMENT_WORD           36
        ASAP2_RECORD_LAYOUT_ALIGNMENT AlignmentWord;
#define OPTPARAM_RECORDLAYOUT_ALIGNMENT_LONG           37
        ASAP2_RECORD_LAYOUT_ALIGNMENT AlignmentLong;
#define OPTPARAM_RECORDLAYOUT_ALIGNMENT_INT64          38
        ASAP2_RECORD_LAYOUT_ALIGNMENT AlignmentInt64;
#define OPTPARAM_RECORDLAYOUT_ALIGNMENT_FLOAT16_IEEE   39
        ASAP2_RECORD_LAYOUT_ALIGNMENT AlignmentFloat16;
#define OPTPARAM_RECORDLAYOUT_ALIGNMENT_FLOAT32_IEEE   40
        ASAP2_RECORD_LAYOUT_ALIGNMENT AlignmentFloat32;
#define OPTPARAM_RECORDLAYOUT_ALIGNMENT_FLOAT64_IEEE   41
        ASAP2_RECORD_LAYOUT_ALIGNMENT AlignmentFloat64;
#define OPTPARAM_RECORDLAYOUT_STATIC_RECORD_LAYOUT     42
        unsigned char StaticRecordLayout;
        int ItemCount;
        int ItemSize;
        ASAP2_RECORD_LAYOUT_ITEM **Items;
    } OptionalParameter;
} ASAP2_RECORD_LAYOUT;

typedef struct {
    int Attribute;
    IDENT InputQuantity;
    IDENT Conversion;
    int MaxAxisPoints;
    double LowerLimit;
    double UpperLimit;
    // Optional Parameter
    struct {
        uint32_t Flags;
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_READ_ONLY             1
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_FORMAT                2
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_AXIS_PTS_REF          3
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_MAX_GRAD              4
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_MONOTONY              5
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_BYTE_ORDER            6
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_EXTENDED_LIMITS       7
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_FIX_AXIS_PAR          8
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_FIX_AXIS_PAR_DIST     9
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_FIX_AXIS_PAR_LIST    10
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_DEPOSIT              11
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_CURVE_AXIS_REF       12
#define OPTPARAM_CHARACTERISTIC_AXIS_DESCR_PHYS_UNIT            13
        unsigned char ReadOnly;
        STRING_POINTER Format;
        IDENT AxisPoints;
        double MaxGradient;
        int Monotony;
        int ByteOrder;
        struct {
            double LowerLimit;  // extended range of table values, lower limit
            double UpperLimit;  // extended range of table values, upper limit
        } ExtendedLimits;
        struct {
            int Offset;  // 'offset' parameter to calculate the axis points of fixed characteristic
                         // curves or maps (see description).
            int Shift;   // 'shift' parameter to calculate the axis points of fixed characteristic
                         // curves or maps (see description).
            int Numberapo;  // number of axis points
        } FixAxisPar;
        struct {
            int Offset;  // 'offset' parameter to calculate the axis points of fixed characteristic
                         // curves or maps (see description).
            int Distance;   // 'shift' parameter to calculate the axis points of fixed characteristic
                         // curves or maps (see description).
            int Numberapo;  // number of axis points
        } FixAxisParDist;
        double *FixAxisParList;
        int DepositMode;
        IDENT CurveAxis;
        IDENT PhysUnit;
    } OptionalParameter;
} ASAP2_CHARACTERISTIC_AXIS_DESCR;

typedef struct {
    IDENT Name;      // Must be first structure member!!!!
    int StructType;  //  Must be second structure member!!!!
    STRING_POINTER LongIdentifier;
    int Type;
    uint64_t Address;
    int FileOffsetAddress;
    IDENT Deposit;    // RECORD_LAYOUT
    double MaxDiff;
    IDENT Conversion;
    double LowerLimit;
    double UpperLimit;

    uint32_t ReferenceCounter; // Counts the references inside the blackboard
    int DirFlags;
    int Vid;

    // Optional Parameter
    struct {
        uint32_t Flags;
#define OPTPARAM_CHARACTERISTIC_DISPLAY_IDENTIFIER                 1
#define OPTPARAM_CHARACTERISTIC_FORMAT                             2
#define OPTPARAM_CHARACTERISTIC_BIT_MASK                           3
#define OPTPARAM_CHARACTERISTIC_BYTE_ORDER                         4
#define OPTPARAM_CHARACTERISTIC_NUMBER                             5
#define OPTPARAM_CHARACTERISTIC_EXTENDED_LIMITS                    6
#define OPTPARAM_CHARACTERISTIC_READ_ONLY                          7
#define OPTPARAM_CHARACTERISTIC_GUARD_RAILS                        8
#define OPTPARAM_CHARACTERISTIC_MAX_REFRESH                        9
#define OPTPARAM_CHARACTERISTIC_REF_MEMORY_SEGMENT                10
#define OPTPARAM_CHARACTERISTIC_COMPARISON_QUANTITY               11
#define OPTPARAM_CHARACTERISTIC_CALIBRATION_ACCESS                12
#define OPTPARAM_CHARACTERISTIC_MATRIX_DIM                        13
#define OPTPARAM_CHARACTERISTIC_ECU_ADDRESS_EXTENSION             14
#define OPTPARAM_CHARACTERISTIC_SYMBOL_LINK                       15
#define OPTPARAM_CHARACTERISTIC_PHYS_UNIT                         16
#define OPTPARAM_CHARACTERISTIC_DISCRETE                          17
#define OPTPARAM_CHARACTERISTIC_STEP_SIZE                         18
#define OPTPARAM_CHARACTERISTICT_IF_DATA_CANAPE                   19

        STRING_POINTER DisplayIdentifier;
        STRING_POINTER Format;
        unsigned char ByteOrder;
        uint64_t BitMask;
        int Number;
        struct {
            double LowerLimit;  // extended range of table values, lower limit
            double UpperLimit;  // extended range of table values, upper limit
        } ExtendedLimits;
        unsigned char ReadOnly;
        unsigned char GuardRails;
        struct {
            int ScalingUnit; // this parameter defines the basic scaling unit. The following
                             // parameter 'Rate' relates on this scaling unit. The value of
                             // ScalingUnit is coded as shown below in Table 4: Codes for scaling units (CSE).
            uint32_t Rate;   // the maximum refresh rate of the concerning measurement
                             // object in the control unit. The unit is defined with parameter 'ScalingUnit'.
        } MaxRefresh;
        IDENT RefMemorySegment;
        IDENT ComparisonQuantity;
        int AxisDescrCount;
        ASAP2_CHARACTERISTIC_AXIS_DESCR *AxisDescr[2];   // 0-> X-Achse 1 -> Y-Achse
        int CalibrationAccess;

        int ArraySize;
        uint64_t ErrorMask;
        int FunctionListSize;
        IDENT_POINTER FunctionList;
        uint64_t Address;
        int FileOffsetAddress;
        struct {
            int x;
            int y;
            int z;
        }MatrixDim;
        int EcuAddressExtension;
        IDENT SymbolLink;
        int SymbolLinkX;
        IDENT PhysUnit;
        unsigned char Discrete;
        double StepSize;
        int IfDataCanapeExtCount;
        ASAP2_IF_DATA_CANAPE_EXT *IfDataCanapeExt[2];
    } OptionalParameter;
} ASAP2_CHARACTERISTIC;

typedef struct {
    IDENT Ident;     // Must be first structure member!!!!
    int StructType;  //  Must be second structure member!!!!
    STRING_POINTER LongIdentifier;
    int DataType;
    IDENT ConversionIdent;
    int Resolution;
    double Accuracy;
    double LowerLimit;
    double UpperLimit;

    int32_t ReferenceCounter; // Counts the references inside the blackboard
    int DirFlags;
    int Vid;

    // Optional Parameter
    struct {
        uint32_t Flags;
#define OPTPARAM_MEASUREMENT_DISPLAY_IDENTIFIER    1
#define OPTPARAM_MEASUREMENT_WRITABLE_FLAG         2
#define OPTPARAM_MEASUREMENT_FORMAT                3
#define OPTPARAM_MEASUREMENT_ARRAY_SIZE            4
#define OPTPARAM_MEASUREMENT_BIT_MASK              5
#define OPTPARAM_MEASUREMENT_BYTE_ORDER            6
#define OPTPARAM_MEASUREMENT_ERROR_MASK            7
#define OPTPARAM_MEASUREMENT_FUNCTIONLIST          8
#define OPTPARAM_MEASUREMENT_ADDRESS               9
#define OPTPARAM_MEASUREMENT_MATRIX_DIM           10
#define OPTPARAM_MEASUREMENT_SYMBOL_LINK          11
#define OPTPARAM_MEASUREMENT_PHYS_UNIT            12
#define OPTPARAM_MEASUREMENT_LAYOUT               13
#define OPTPARAM_MEASUREMENT_DISCRETE             14
#define OPTPARAM_MEASUREMENT_IF_DATA_CANAPE       15
        STRING_POINTER DisplayIdentifier;
        unsigned char WritableFlag;
        STRING_POINTER Format;
        int ArraySize;
        uint64_t BitMask;
        unsigned char ByteOrder;
        uint64_t ErrorMask;
        int FunctionListSize;
        IDENT_POINTER FunctionList;
        uint64_t Address;
        int FileOffsetAddress;
        struct {
            int x;
            int y;
            int z;
        }MatrixDim;
        IDENT SymbolLink;
        int SymbolLinkX;
        IDENT PhysUnit;
        int Layout;
        unsigned char Discrete;
        int IfDataCanapeExtCount;
        ASAP2_IF_DATA_CANAPE_EXT *IfDataCanapeExt[2];
    } OptionalParameter;
} ASAP2_MEASUREMENT;

typedef struct {
    IDENT Name;      // Must be first structure member!!!!
    int StructType;  //  Must be second structure member!!!!
    STRING_POINTER LongIdentifier;
    uint64_t Address;
    int FileOffsetAddress;
    IDENT InputQuantity;
    IDENT Deposit;
    double MaxDiff;
    IDENT Conversion;
    int MaxAxisPoints;
    double LowerLimit;
    double UpperLimit;
    struct {
        uint32_t Flags;
#define OPTPARAM_AXIS_PTS_DISPLAY_IDENTIFIER          1
#define OPTPARAM_AXIS_PTS_READ_ONLY                   2
#define OPTPARAM_AXIS_PTS_FORMAT                      3
#define OPTPARAM_AXIS_PTS_DEPOSIT                     4
#define OPTPARAM_AXIS_PTS_BYTE_ORDER                  5
#define OPTPARAM_AXIS_PTS_FUNCTION_LIST               6
#define OPTPARAM_AXIS_PTS_REF_MEMORY_SEGMENT          7
#define OPTPARAM_AXIS_PTS_GUARD_RAILS                 8
#define OPTPARAM_AXIS_PTS_EXTENDED_LIMITS             9
#define OPTPARAM_AXIS_PTS_ANNOTATION                 10
#define OPTPARAM_AXIS_PTS_IF_DATA                    11
#define OPTPARAM_AXIS_PTS_CALIBRATION_ACCESS         12
#define OPTPARAM_AXIS_PTS_ECU_ADDRESS_EXTENSION      13
#define OPTPARAM_AXIS_PTS_SYMBOL_LINK                14
#define OPTPARAM_AXIS_PTS_PHYS_UNIT                  15
#define OPTPARAM_AXIS_PTS_MONOTONY                   16
#define OPTPARAM_AXIS_PTS_IF_DATA_CANAPE             17

        STRING_POINTER DisplayIdentifier;
        unsigned char ReadOnly;
        STRING_POINTER Format;
        int DepositMode;
        int Monotony;
        int ByteOrder;
        IDENT RefMemorySegment;
        unsigned char GuardRails;
        struct {
            double LowerLimit;  // extended range of table values, lower limit
            double UpperLimit;  // extended range of table values, upper limit
        } ExtendedLimits;
        int CalibrationAccess;
        int EcuAddressExtension;
        IDENT SymbolLink;
        int SymbolLinkX;
        IDENT PhysUnit;
        int IfDataCanapeExtCount;
        ASAP2_IF_DATA_CANAPE_EXT *IfDataCanapeExt[2];
    } OptionalParameter;
} ASAP2_AXIS_PTS;

typedef struct {
    IDENT Name;      // Must be first structure member!!!!
    int StructType;  //  Must be second structure member!!!!
    STRING_POINTER LongIdentifier;
    int ConversionType;
    STRING_POINTER Format;
    STRING_POINTER Unit;
    struct {
        uint32_t Flags;
#define OPTPARAM_COMPU_METHOD_FORMULA             1
#define OPTPARAM_COMPU_METHOD_FORMULA_INV         2
#define OPTPARAM_COMPU_METHOD_COEFFS              3
#define OPTPARAM_COMPU_METHOD_COMPU_TAB_REF       4
#define OPTPARAM_COMPU_METHOD_REF_UNIT            5
#define OPTPARAM_COMPU_METHOD_COEFFS_LINEAR       6
#define OPTPARAM_COMPU_METHOD_STATUS_STRING_REFR  7
        STRING_POINTER Formula;
        STRING_POINTER FormulaInv;
        struct {
            double a;
            double b;
            double c;
            double d;
            double e;
            double f;
        } Coeffs;
        IDENT ConversionTable;
        IDENT Unit;
        IDENT StatusStringRef;
    } OptionalParameter;
} ASAP2_COMPU_METHOD;

typedef struct {
        double InVal_InValMin;
        double OutVal_InValMax;
        STRING_POINTER OutString;
} ASAP2_COMPU_TAB_VALUE_PAIR;

typedef struct {
    IDENT Name;      // Must be first structure member!!!!
    int StructType;  //  Must be second structure member!!!!
    int TabOrVtabFlag;   // 0 -> COMPU_TAB, 1 -> COMPU_VTAB, 2 -> COMPU_VTAB_RANGE
    STRING_POINTER LongIdentifier;
    int ConversionType;
    int NumberValuePairs;
    ASAP2_COMPU_TAB_VALUE_PAIR *ValuePairs;
    struct {
        uint32_t Flags;
#define OPTPARAM_COMPU_TAB_DEFAULT_VALUE             0x1
        STRING_POINTER DefaultValue;
    } OptionalParameter;
} ASAP2_COMPU_TAB;

typedef struct {
    IDENT Name;      // Must be first structure member!!!!
    int StructType;  //  Must be second structure member!!!!
    STRING_POINTER LongIdentifier;
    struct {
        uint32_t Flags;
#define OPTPARAM_FUNCTION_DEF_CHARACTERISTIC             1
#define OPTPARAM_FUNCTION_REF_CHARACTERISTIC             2
#define OPTPARAM_FUNCTION_IN_MEASUREMENT                 3
#define OPTPARAM_FUNCTION_OUT_MEASUREMENT                4
#define OPTPARAM_FUNCTION_LOC_MEASUREMENT                5
#define OPTPARAM_FUNCTION_SUB_FUNCTION                   6
#define OPTPARAM_FUNCTION_FUNCTION_VERSION               7
        int DefCharacteristicSize;
        IDENT_POINTER DefCharacteristics;
        int RefCharacteristicSize;
        IDENT_POINTER RefCharacteristics;
        int InMeasurementSize;
        IDENT_POINTER InMeasurements;
        int OutMeasurementSize;
        IDENT_POINTER OutMeasurements;
        int LocMeasurementSize;
        IDENT_POINTER LocMeasurements;
        int SubFunctionSize;
        IDENT_POINTER SubFunctions;
        STRING_POINTER Version;
    } OptionalParameter;
} ASAP2_FUNCTION;

struct DAQ_LIST_CAN_ID {
    uint32_t DaqListNr;  /* reference to DAQ_LIST_NUMBER */
    int VarOrFixed;
    uint32_t FixedValue;
};

typedef struct CAN_Parameters {
    uint32_t Version;
    struct {
        uint32_t Flags;
#define OPTPARAM_IF_DATA_XCP_ON_CAN_ID_BROADCAST                       1
#define OPTPARAM_IF_DATA_XCP_ON_CAN_ID_MASTER                          2
#define OPTPARAM_IF_DATA_XCP_ON_CAN_ID_SLAVE                           3
#define OPTPARAM_IF_DATA_XCP_ON_CAN_BAUDRATE                           4
#define OPTPARAM_IF_DATA_XCP_ON_CAN_SAMPLE_POINT                       5
#define OPTPARAM_IF_DATA_XCP_ON_CAN_SAMPLE_RATE                        6
#define OPTPARAM_IF_DATA_XCP_ON_CAN_BTL_CYCLES                         7
#define OPTPARAM_IF_DATA_XCP_ON_CAN_SJW                                8
#define OPTPARAM_IF_DATA_XCP_ON_CAN_SYNC_EDGE                          9
#define OPTPARAM_IF_DATA_XCP_ON_CAN_MAX_DLC_REQUIRED                   10
#define OPTPARAM_IF_DATA_XCP_ON_CAN_MAX_BUS_LOAD                       11
#define OPTPARAM_IF_DATA_XCP_ON_CAN_DAQ_LIST_CAN_ID                    12
        uint32_t CanIdBroadcast;  /* Bit31= 1: extended identifier */
        uint32_t CanIdMaster; /*Bit31= 1: extended identifier */
        uint32_t CanIdSlave;  /*Bit31= 1: extended identifier */
        uint32_t Baudrate;
        uint32_t SamplePoint;
        int SampleRate;
        uint32_t BtlCycles;
        uint32_t Sjw;
        int SyncEdge;
        int MaxDlcRequired;
        uint32_t MaxBusLoad;
    #define DAQ_LIST_MAX_NUMBER  32
        int NumberOfDaqLists;
        struct DAQ_LIST_CAN_ID DaqLists[DAQ_LIST_MAX_NUMBER];
    } OptionalParameter;
} ASAP2_IF_DATA_XCP_ON_CAN;


typedef struct {
    uint32_t ProtocolVersion;
    uint32_t T1; /* [ms] */
    uint32_t T2; /* [ms] */
    uint32_t T3; /* [ms] */
    uint32_t T4; /* [ms] */
    uint32_t T5; /* [ms] */
    uint32_t T6; /* [ms] */
    uint32_t T7; /* [ms] */
    uint32_t MAX_CTO;
    uint32_t MAX_DTO;
    int ByteOrder;
    int AddressGranularity;
    struct {
        uint32_t Flags;
#define OPTPARAM_IF_DATA_XCP_PROTOCOL_LAYER_OPTIONAL_CMD                       1
#define OPTPARAM_IF_DATA_XCP_PROTOCOL_LAYER_SEED_AND_KEY_EXTERNAL_FUNCTION     2
        uint64_t OptinalCmds;
        STRING_POINTER SeedAndKeyDll;
    } OptionalParameter;
} ASAP2_IF_DATA_XCP_PROTOCOL_LAYER;

/*typedef struct {
    struct {
        uint32_t Flags;
#define OPTPARAM_IF_DATA_XCP             0x1
        ASAP2_IF_DATA_XCP_PROTOCOL_LAYER ProtocolLayer;
    } OptionalParameter;
} ASAP2_IF_DATA_XCP_ON_CAN;
*/

typedef struct {
    struct {
        uint32_t Flags;
#define OPTPARAM_IF_DATA_XCP_PROTOCOL_LAYER     1
#define OPTPARAM_IF_DATA_XCP_ON_CAN             2
        ASAP2_IF_DATA_XCP_PROTOCOL_LAYER ProtocolLayer;
        ASAP2_IF_DATA_XCP_ON_CAN OnCan;
    } OptionalParameter;
} ASAP2_IF_DATA_XCP;


typedef struct {
    IDENT Ident;      // Must be first structure member!!!!
    int StructType;  //  Must be second structure member!!!!
    char *String;
    int RecordLayoutCounter;
    int RecordLayoutSize;
    ASAP2_RECORD_LAYOUT **RecordLayouts;
    int AxisDescrCounter;
    int AxisDescrSize;
    ASAP2_CHARACTERISTIC_AXIS_DESCR **AxisDescrs;
    int CharacteristicCounter;
    int CharacteristicSize;
    ASAP2_CHARACTERISTIC **Characteristics;
    int MeasurementCounter;
    int MeasurementSize;
    ASAP2_MEASUREMENT **Measurements;
    int AxisPtsCounter;
    int AxisPtsSize;
    ASAP2_AXIS_PTS **AxisPtss;
    int CompuMethodCounter;
    int CompuMethodSize;
    ASAP2_COMPU_METHOD **CompuMethods;
    int CompuTabCounter;
    int CompuTabSize;
    ASAP2_COMPU_TAB **CompuTabs;
    int FunctionCounter;
    int FunctionSize;
    ASAP2_FUNCTION **Functions;

    ASAP2_MOD_COMMON ModCommon;

    int IfDataXcpCounter;
    ASAP2_IF_DATA_XCP IfDataXcp;

} ASAP2_MODULE_DATA;



int AddRecordLayout (char *Ident, ...);
int AddCharacteristic (char *Ident, ...);
int AddMeasurement (char *Ident, ...);
// ...

typedef struct {
    int Size;
    int Pointer;
    char *Data;
} STRING_BUFFER;

typedef struct {
    int ModuleCounter;
    int ModuleSize;
    ASAP2_MODULE_DATA **Modules;
    int StringBufferCounter;        // Anzahl der 64KByte String-Blcke
    STRING_BUFFER *StringBuffers;   // Enthaelt alle waerend des Parsens dynamisch allozierte Elemente wird dynamisch vergrert
} ASAP2_DATA;

#endif // A2LDATA_H
