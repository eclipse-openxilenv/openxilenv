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

typedef struct {
    char FileIdentifier[8];     // "MDF     "
    char FormatTdentifier[8];   // "3.10 "
    char ProgramIdentifier[8];
    uint16_t DefaultByteOrder;    // 0 -> LSB fist, > 0 MSB first
    uint16_t DefaultFloatFormat;  // 0 = float
    uint16_t VersionNumber;       // 310
    uint16_t Reserved1;
    char Reserved2[2];
    char Reserved3[30];
} MDF_IDBLOCK;


typedef struct {
    char BlockTypeIdentifier[2];   // "HD"
    uint16_t BlockSize;              // Size of entire struct
    int32_t FirstDataGroupBlock;      // File offset to the first DGBLOCK
    int32_t MeasurementFileComment;   // File offset to comment text TXBLOCK or NULL
    int32_t ProgramBlock;             // File offset to program block PRBLOCK or NULL
    uint16_t NumberOfDataGroups;     // Number of data groups
    char DateOfRecording[10];      // Date of the recording "DD:MM:YYYY"
    char TimeOfRecording[8];       // Time of the recording "HH:MM:SS"
    char AuthorName[32];
    char OrganizationName[32];
    char ProjectName[32];
    char MeasurementObject[32];
//    uint64_t TimeStamp;          // Time stamp of recording was started (ns).
//                                   // Elapsed time since 00:00:00 01.01.1970 Start with version 3.20. before value is 0
} MDF_HDBLOCK;

typedef struct {
    char BlockTypeIdentifier[2];   // "TX"
    uint16_t BlockSize;              // size of TXBLOCK
    // char Text[MDF_UNDEFINED_LENGTH];
} MDF_TXBLOCK;


typedef struct {
    char BlockTypeIdentifier[2];   // "DG"
    uint16_t BlockSize;              // Size of DGBLOCK
    int32_t NextDataGroupBlock;       // File offset to next DGBLOCK or NULL
    int32_t FirstChannelGroupBlock;   // File offset to first CGBLOCK or NULL
    int32_t TriggerBlock;             // File offset to TRBLOCK or NULL
    int32_t DataBlock;                // File offset to the data block
    uint16_t NumberOfChannelGroups;  // Number of channel groups
    uint16_t NumberOfRecordIDs;      // Number of record IDs in the data block
                                       // 0 = without record ID
                                       // 1 = record ID before data record
                                       // 2 = record ID before and after data record
    uint32_t Reserved;
} MDF_DGBLOCK;

typedef struct {
    char BlockTypeIdentifier[2];   // "CG"
    uint16_t BlockSize;              // Size of CGBLOCK
    int32_t NextChannelGroupBlock;    // File offset to next CGBLOCK or NULL
    int32_t FirstChannelBlock;        // File offset to first CNBLOCK or NULL
    int32_t ChannelGroupComment;      // File offset to TXBLOCK or NULL
    uint16_t RecordID;               // Record ID
    uint16_t NumberOfChannels;       // Number of channels
    uint16_t SizeOfDataRecord;       // Size of data record in Bytes, without record ID
    uint32_t NumberOfRecords;        // Number of records
} MDF_CGBLOCK;

typedef struct {
    char BlockTypeIdentifier[2];   //  "CN"
    uint16_t BlockSize;              //  Size of CNBLOCK
    int32_t NextChannelBlock;         //  File offset to next CNBLOCK or NULL
    int32_t ConversionFormula;        //  File offset to CCBLOCK or NULL
    int32_t SourceDependingExtensions; // File offset to CEBLOCK or NULL
    int32_t DependencyBlock;          //  File offset to CDBLOCK or NULL
    int32_t ChannelComment;           //  File offset to TXBLOCK or NULL
    uint16_t ChannelType;            //  Channel type
                                       //    0 = Data
                                       //    1 = Time
    char ShortSignalName[32];      //  Signal name
    char SignalDescription[128];   //  Signal description
    uint16_t StartBitOffset;         //  Start offset
    uint16_t NumberOfBits;           //  Number of bits
    uint16_t SignalDataType;         //  Data type
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
    uint16_t ValueRangeValidFlag;      //  1 -> min. and max. values are valid
    double MinimumSignalValue;       //  Min. raw value
    double MaximumSignalValue;       //  Max. raw value
    double SamplingRate;             //  Sampling period
    int32_t LongSignalName;           //  File offset to TXBLOCK long signal name or NULL
    int32_t SignalDisplayName;        //  File offset to TXBLOCK display name  or NULL
    uint16_t AdditionalByteOffset;   //  Additional Byte offset default = 0
} MDF_CNBLOCK;


typedef struct {
    char BlockTypeIdentifier[2];      //  "CC"
    uint16_t BlockSize;                 //  Block size CCBLOCK
    uint16_t PhysicalValueRangeValidFlag; //  1 -> min. and max. values are valid
    double MinimumPhysicalSignalValue;  //  Min. raw value
    double MaximumPhysicalSignalValue;  //  Max. raw value
    char PhysicalUnit[20];            //  Unit
    uint16_t ConversionType;            //  Conversion type:
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
    uint16_t ConversionDataSize;       // Size
} MDF_CCBLOCK;


typedef struct {
    double LowerRange;
    double UpperRange;
    int32_t PointerToTx;
}  MDF_COMPU_VTAB_RANGE;

typedef struct {
    double Value;
    char AssignedText[32];
}  MDF_COMPU_VTAB;

#pragma pack(pop)

#endif // MDF_STRUCTS_H
