/*
 * Copyright 2024 ZF Friedrichshafen AG
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

#ifndef MDF4_STRUCTS_H
#define MDF4_STRUCTS_H

#include <stdint.h>

#define MDF_UNDEFINED_LENGTH 1

#pragma pack(push,1)

typedef struct {
    char FileIdentifier[8];   // always contains "MDF     "
    char FormatTdentifier[8];   // "4.10 " for version 4.1 revision 0
    char ProgramIdentifier[8];
    uint16_t DefaultByteOrder;    // 0 = Little Endian (Intel order)
                                    // Any other value = Big Endian (Motorola order)
    uint16_t DefaultFloatFormat;   // 0 = Floating-point format compliant with IEEE 754 standard
    uint16_t VersionNumber;        // 310 for this version
    uint16_t Reserved1;
    char Reserved2[2];
    char Reserved3[30];
} MDF4_IDBLOCK;

typedef struct {
    char BlockTypeIdentifier[4];   //  always "##xx"
    uint32_t Reserved;
    uint64_t BlockLength;
    uint64_t LinkCount;
} MDF4_BLOCK_HEADER;

typedef struct {
    uint64_t Time;           // Time in nanoseconds since 1970
    int16_t TimeZoneOffset;  // Time zone offsets in minutes. Best practice is to always use UTC.
    int16_t DstOffset;       // DST offset in minutes. Best practice is to always use UTC.
    uint8_t flags;
} MDF4_TIME_STAMP;

typedef struct {
    MDF4_TIME_STAMP TimeStamp;
    uint8_t TimeClass;
    uint8_t Flags;
    uint8_t Reserved;
    double StartAngle;     // in radians
    double StartDistance;  // in meters
} MDF4_HDBLOCK;

typedef struct {
    MDF4_TIME_STAMP TimeStamp;
    uint8_t Reserved[3];
} MDF4_FHBLOCK;

typedef struct {
    uint8_t RecIdSize;
    uint8_t Reserved[7];
} MDF4_DGBLOCK;

typedef struct {
    uint8_t Flags;
    uint8_t Reserved[3];
    uint32_t NoOfBlocks;
    uint64_t EqualLength;
} MDF4_DLBLOCK;

typedef struct {
    uint16_t Flags;
    uint8_t Type; // if zip
    uint8_t Reserved[5];
} MDF4_HLBLOCK;

typedef struct {
    char OrigBlockTypeIdentifier[2];
    uint8_t Type;
    uint8_t Reserved;
    uint32_t Parameter;
    uint64_t OrigDataLength;
    uint64_t DataLength;
} MDF4_DZBLOCK;

typedef struct {
    uint64_t RecordId;
    uint64_t NoOfSamples;
    uint16_t Flags;
    uint16_t PathSeparator;
    uint32_t Reserved;
    uint32_t NoOfDataBytes;
    uint32_t NoOfInvalidBytes;
} MDF4_CGBLOCK;

typedef struct {
    uint8_t Type;
    uint8_t SyncType; ///< Normal channel type
    uint8_t DataType;
    uint8_t BitOffset;
    uint32_t ByteOffset;
    uint32_t BitCount;
    uint32_t Flags;
    uint32_t InvalidBitPos;
    uint8_t Precision;
    uint8_t Reserved;
    uint16_t NoOfAttachments;
    double RangeMin;
    double RangeMax;
    double LimitMin;
    double LimitMax;
    double LimitExtMin;
    double LimitExtMax;
} MDF4_CNBLOCK;

typedef struct {
    uint8_t Type;
    uint8_t Precision;
    uint16_t Flags;
    uint16_t NoOfReferences;
    uint16_t NoOfValues;
    double RangeMin;
    double RangeMax;
} MDF4_CCBLOCK;


#pragma pack(pop)

#endif // MDF4_STRUCTS_H
