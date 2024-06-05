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


#ifndef __A2LACCESS_H
#define __A2LACCESS_H

#include <stdint.h>

typedef struct {
    void *Asap2Ptr;   // This is a pointer of type ASAP2_PARSER
    int SelectedModul;
} ASAP2_DATABASE;

#define CheckIfFlagSet(Flags, Mask)  ((Flags & Mask) == Mask)
#define CheckIfFlagSetPos(Flags, BitPos)  ((Flags & (1ULL << (BitPos))) == (1ULL << (BitPos)))


// This function will load an A2L-File into the internal memory database
// The return value will be NULL if an error occur
// and an error message will be written to 'ErrString'
#define A2L_TRY_TO_LOAD_IF_DATA_XCP      0x1
ASAP2_DATABASE *LoadAsapFile (const char *Filename, int Flags, char *ErrString, int MaxErrSize, int *ret_LineNr);

void FreeAsap2Database(ASAP2_DATABASE *Database);


// Returns the namen of the next module inside the loaded A2L data structure.
// If 'Index' = -1 we will start from the beginning. Otherwise this should be the return value of the
// previous call. There are no more modules available if the return value is -1.
int GetNextModuleName (ASAP2_DATABASE *Database, int Index,  char *Filter, char *RetName, int MaxSize);

// Select an active module
int SelectModule (ASAP2_DATABASE *Database, char *Module);

// Returns the namen of the next function block inside the loaded A2L data structure.
// If 'Index' = -1 we will start from the beginning. Otherwise this should be the return value of the
// previous call. There are no more function blocks available if the return value is -1.
int GetNextFunction (ASAP2_DATABASE *Database, int Index, const char *Filter, char *RetName, int MaxSize);

int GetFunctionIndex (ASAP2_DATABASE *Database, const char *Name);

// Returns the namen of the next function block member inside the loaded A2L data structure.
// If 'Index' = -1 we will start from the beginning. Otherwise this should be the return value of the
// previous call. There are no more function block members available if the return value is -1.
#define FLAG_DEF_CHARACTERISTIC    0x1
#define FLAG_REF_CHARACTERISTIC    0x2
#define FLAG_CHARACTERISTIC        (FLAG_DEF_CHARACTERISTIC | FLAG_REF_CHARACTERISTIC)
#define FLAG_IN_MEASUREMENT        0x4
#define FLAG_OUT_MEASUREMENT       0x8
#define FLAG_LOC_MEASUREMENT      0x10
#define FLAG_MEASUREMENT          (FLAG_IN_MEASUREMENT | FLAG_OUT_MEASUREMENT | FLAG_LOC_MEASUREMENT)
#define FLAG_SUB_FUNCTION         0x20
int GetNextFunctionMember (ASAP2_DATABASE *Database, int FuncIdx, int MemberIdx, const char *Filter, unsigned int Flags, char *RetName, int MaxSize, int *RetType);

// Returns the namen of the next measurement inside the loaded A2L data structure.
// If 'Index' = -1 we will start from the beginning. Otherwise this should be the return value of the
// previous call. There are no more measurement available if the return value is -1.
int GetNextMeasurement (ASAP2_DATABASE *Database, int Index, const char *Filter, unsigned int Flags, char *RetName, int MaxSize);

// Returns the index of a measurement inside the loaded A2L data structure.
// If it is not existing it will return -1.
int GetMeasurementIndex (ASAP2_DATABASE *Database, const char *Name);

// Returns all informations of a measurement from the the loaded A2L data structure
int GetMeasurementInfos (ASAP2_DATABASE *Database, int Index,
                         char *Name, int MaxSizeName,
                         int *pType, uint64_t *pAddress,
                         int *pConvType, char *Conv, int MaxSizeConv,
                         char *Unit, int MaxSizeUnit,
                         int *pFormatLength, int *pFormatLayout,
                         int *pXDim, int *pYDim, int *pZDim,
                         double *pMin, double *pMax, int *pIsWritable);


// Returns the namen of the next characteristic inside the loaded A2L data structure.
// If 'Index' = -1 we will start from the beginning. Otherwise this should be the return value of the
// previous call. There are no more measurement available if the return value is -1.
#define FLAG_VALUE_CHARACTERISTIC      0x2
#define FLAG_CURVE_CHARACTERISTIC      0x4
#define FLAG_MAP_CHARACTERISTIC        0x8
#define FLAG_CUBOID_CHARACTERISTIC    0x10
#define FLAG_VAL_BLK_CHARACTERISTIC   0x20
#define FLAG_ASCII_CHARACTERISTIC     0x40
int GetNextCharacteristic (ASAP2_DATABASE *Database, int Index, const char *Filter, unsigned int Flags, char *RetName, int MaxSize);
int GetNextCharacteristicAxis (ASAP2_DATABASE *Database, int Index, const char *Filter, unsigned int Flags, char *RetName, int MaxSize);

int GetCharacteristicType (ASAP2_DATABASE *Database, int Index);

int GetCharacteristicIndex (ASAP2_DATABASE *Database, const char *Name);

int GetCharacteristicAxisIndex (ASAP2_DATABASE *Database, const char *Name);

int GetValueCharacteristicInfos (ASAP2_DATABASE *Database, int Index,
                                char *Name, int MaxSizeName,
                                int *pType, uint64_t *pAddress,
                                int *pConvType, char *Conv, int MaxSizeConv,
                                char *Unit, int MaxSizeUnit,
                                int *pFormatLength, int *pFormatLayout,
                                int *pXDim, int *pYDim, int *pZDim,
                                double *pMin, double *pMax, int *pIsWritable);

int GetCharacteristicInfoType(ASAP2_DATABASE *Database, int Index);
int GetCharacteristicAddress(ASAP2_DATABASE *Database, int Index, uint64_t *ret_Address);
int GetMeasurementAddress(ASAP2_DATABASE *Database, int Index, uint64_t *ret_Address);
#if 0
int GetCharacteristicInfoMinMax(ASAP2_DATABASE *Database, int Index, int AxisNo, double *ret_Min, double *ret_Max);
int GetCharacteristicInfoInput(ASAP2_DATABASE *Database, int Index, int AxisNo, char *ret_Input, int MaxLen);
int GetCharacteristicInfoUnit(ASAP2_DATABASE *Database, int Index, int AxisNo, char *ret_Unit, int MaxLen);
#endif
int GetCharacteristicAxisInfos(ASAP2_DATABASE *Database, int Index, int AxisNo,
                               double *ret_Min, double *ret_Max, char *ret_Input, int InputMaxLen,
                               int *ret_ConvType, char *ret_Conversion, int ConversionMaxLen,
                               char *ret_Unit, int UnitMaxLen,
                               int *ret_FormatLength, int *ret_FormatLayout);

int GetAxisIndex(ASAP2_DATABASE *Database, char *Name);

int GetRecordLayoutIndex(ASAP2_DATABASE* Database, char *Name);

int SearchMinMaxCalibrationAddress(ASAP2_DATABASE *Database, uint64_t *ret_MinAddress, uint64_t *ret_MaxAddress);

#endif
