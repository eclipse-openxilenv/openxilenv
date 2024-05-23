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


#ifndef MDF_STRUCTS_H
#define MDF_STRUCTS_H

#include <stdint.h>

#define MDF_UNDEFINED_LENGTH 1

#pragma pack(push,1)
typedef char MDF_CHAR;
typedef unsigned char MDF_UINT8;
typedef uint16_t MDF_UINT16;
typedef uint32_t MDF_UINT32;
typedef uint64_t MDF_UINT64;
typedef uint16_t MDF_BOOL;
typedef double MDF_REAL;
typedef int32_t MDF_LINK;

typedef struct {
    MDF_CHAR FileIdentifier[8];     // "MDF     "
    MDF_CHAR FormatTdentifier[8];   // "3.10 "
    MDF_CHAR ProgramIdentifier[8];
    MDF_UINT16 DefaultByteOrder;    // 0 -> LSB fist, > 0 MSB first
    MDF_UINT16 DefaultFloatFormat;  // 0 = float
    MDF_UINT16 VersionNumber;       // 310
    MDF_UINT16 Reserved1;
    MDF_CHAR Reserved2[2];
    MDF_CHAR Reserved3[30];
} MDF_IDBLOCK;


typedef struct {
    MDF_CHAR BlockTypeIdentifier[2];   // "HD"
    MDF_UINT16 BlockSize;              // Size of entire struct
    MDF_LINK FirstDataGroupBlock;      // File offset to the first DGBLOCK
    MDF_LINK MeasurementFileComment;   // File offset to comment text TXBLOCK or NULL
    MDF_LINK ProgramBlock;             // File offset to program block PRBLOCK or NULL
    MDF_UINT16 NumberOfDataGroups;     // Number of data groups
    MDF_CHAR DateOfRecording[10];      // Date of the recording "DD:MM:YYYY"
    MDF_CHAR TimeOfRecording[8];       // Time of the recording "HH:MM:SS"
    MDF_CHAR AuthorName[32];
    MDF_CHAR OrganizationName[32];
    MDF_CHAR ProjectName[32];
    MDF_CHAR MeasurementObject[32];
//    MDF_UINT64 TimeStamp;          // Time stamp of recording was started (ns).
//                                   // Elapsed time since 00:00:00 01.01.1970 Start with version 3.20. before value is 0
} MDF_HDBLOCK;

typedef struct {
    MDF_CHAR BlockTypeIdentifier[2];   // "TX"
    MDF_UINT16 BlockSize;              // size of TXBLOCK
    // MDF_CHAR Text[MDF_UNDEFINED_LENGTH];
} MDF_TXBLOCK;


typedef struct {
    MDF_CHAR BlockTypeIdentifier[2];   // "DG"
    MDF_UINT16 BlockSize;              // Size of DGBLOCK
    MDF_LINK NextDataGroupBlock;       // File offset to next DGBLOCK or NULL
    MDF_LINK FirstChannelGroupBlock;   // File offset to first CGBLOCK or NULL
    MDF_LINK TriggerBlock;             // File offset to TRBLOCK or NULL
    MDF_LINK DataBlock;                // File offset to the data block
    MDF_UINT16 NumberOfChannelGroups;  // Number of channel groups
    MDF_UINT16 NumberOfRecordIDs;      // Number of record IDs in the data block
                                       // 0 = without record ID
                                       // 1 = record ID before data record
                                       // 2 = record ID before and after data record
    MDF_UINT32 Reserved;
} MDF_DGBLOCK;

typedef struct {
    MDF_CHAR BlockTypeIdentifier[2];   // "CG"
    MDF_UINT16 BlockSize;              // Size of CGBLOCK
    MDF_LINK NextChannelGroupBlock;    // File offset to next CGBLOCK or NULL
    MDF_LINK FirstChannelBlock;        // File offset to first CNBLOCK or NULL
    MDF_LINK ChannelGroupComment;      // File offset to TXBLOCK or NULL
    MDF_UINT16 RecordID;               // Record ID
    MDF_UINT16 NumberOfChannels;       // Number of channels
    MDF_UINT16 SizeOfDataRecord;       // Size of data record in Bytes, without record ID
    MDF_UINT32 NumberOfRecords;        // Number of records
} MDF_CGBLOCK;

typedef struct {
    MDF_CHAR BlockTypeIdentifier[2];   //  "CN"
    MDF_UINT16 BlockSize;              //  Size of CNBLOCK
    MDF_LINK NextChannelBlock;         //  File offset to next CNBLOCK or NULL
    MDF_LINK ConversionFormula;        //  File offset to CCBLOCK or NULL
    MDF_LINK SourceDependingExtensions; // File offset to CEBLOCK or NULL
    MDF_LINK DependencyBlock;          //  File offset to CDBLOCK or NULL
    MDF_LINK ChannelComment;           //  File offset to TXBLOCK or NULL
    MDF_UINT16 ChannelType;            //  Channel type
                                       //    0 = Data
                                       //    1 = Time
    MDF_CHAR ShortSignalName[32];      //  Signal name
    MDF_CHAR SignalDescription[128];   //  Signal description
    MDF_UINT16 StartBitOffset;         //  Start offset
    MDF_UINT16 NumberOfBits;           //  Number of bits
    MDF_UINT16 SignalDataType;         //  Data type
                                       //    0 = unsigned int
                                       //    1 = signed int
                                       //    2 = float
                                       //    3 = double
                                       //    4 = VAX float
                                       //    5 = VAX float
                                       //    6 = VAX float
                                       //    7 = String
                                       //    8 = Byte Array
                                       //    9 = unsigned int (big endian)
                                       //    10 = signed int (big endian)
                                       //    11 = float (big endian)
                                       //    12 = double (big endian)
                                       //    13 = unsigned int (little endian)
                                       //    14 = signed int (little endian)
                                       //    15 = float (little endian)
                                       //    16 = double (little endian)
    MDF_BOOL ValueRangeValidFlag;      //  1 -> min. and max. values are valid
    MDF_REAL MinimumSignalValue;       //  Min. raw value
    MDF_REAL MaximumSignalValue;       //  Max. raw value
    MDF_REAL SamplingRate;             //  Sampling period
    MDF_LINK LongSignalName;           //  File offset to TXBLOCK long signal name or NULL
    MDF_LINK SignalDisplayName;        //  File offset to TXBLOCK display name  or NULL
    MDF_UINT16 AdditionalByteOffset;   //  Additional Byte offset default = 0
} MDF_CNBLOCK;


typedef struct {
    MDF_CHAR BlockTypeIdentifier[2];      //  "CC"
    MDF_UINT16 BlockSize;                 //  Block size CCBLOCK
    MDF_BOOL PhysicalValueRangeValidFlag; //  1 -> min. and max. values are valid
    MDF_REAL MinimumPhysicalSignalValue;  //  Min. raw value
    MDF_REAL MaximumPhysicalSignalValue;  //  Max. raw value
    MDF_CHAR PhysicalUnit[20];            //  Unit
    MDF_UINT16 ConversionType;            //  Conversion type:
                                          //     0 = linear
                                          //     1 = tabular with interpolation
                                          //     2 = tabular
                                          //     6 = polynomial function
                                          //     7 = exponential function
                                          //     8 = logarithmic function
                                          //     9 = rational conversion formula
                                          //     10 = Text formula
                                          //     11 = COMPU_VTAB
                                          //     12 = COMPU_VTAB_RANGE
                                          //     132 = date (Based on 7 Byte Date data structure)
                                          //     133 = time (Based on 6 Byte Time data structure)
                                          //     65535 = 1:1 raw = phys
    MDF_UINT16 ConversionDataSize;       // Size
} MDF_CCBLOCK;


typedef struct {
    MDF_REAL LowerRange;
    MDF_REAL UpperRange;
    MDF_LINK PointerToTx;
}  MDF_COMPU_VTAB_RANGE;

typedef struct {
    MDF_REAL Value;
    MDF_CHAR AssignedText[32];
}  MDF_COMPU_VTAB;

#pragma pack(pop)

#endif // MDF_STRUCTS_H
