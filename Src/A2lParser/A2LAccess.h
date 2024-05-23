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


// Gibt den Namen des naechsten Modules in der eingelesenen A2L-Datenstruktur zurueck.
// Wenn 'Index' = -1 wird von Begin gesucht. Ansonsten sollte der Rueckgabewert des
// vorherigen Aufrufes uebergeben werden. Ist der Return-Wert -1 ist kein
// weiteres Modul mehr vorhanden (normalerweise ist immer ein Modul in einem A2L-File)
int GetNextModuleName (ASAP2_DATABASE *Database, int Index,  char *Filter, char *RetName, int MaxSize);

// waehlt ein Modul als aktiv aus
int SelectModule (ASAP2_DATABASE *Database, char *Module);

// Gibt den Namen des naechsten Function-Block in der eingelesenen A2L-Datenstruktur zurueck.
// Wenn 'Index' = -1 wird von Begin gesucht. Ansonsten sollte der Rueckgabewert des
// vorherigen Aufrufes uebergeben werden. Ist der Return-Wert -1 ist kein
// weiteres Funnction-Block mehr vorhanden.
int GetNextFunction (ASAP2_DATABASE *Database, int Index, const char *Filter, char *RetName, int MaxSize);

int GetFunctionIndex (ASAP2_DATABASE *Database, const char *Name);

// Gibt den Namen des naechsten Member eines Function-Block in der eingelesenen A2L-Datenstruktur zurueck.
// Wenn 'Index' = -1 wird von Begin gesucht. Ansonsten sollte der Rueckgabewert des
// vorherigen Aufrufes uebergeben werden. Ist der Return-Wert -1 ist kein
// weiterer Member mehr vorhanden.
#define FLAG_DEF_CHARACTERISTIC    0x1
#define FLAG_REF_CHARACTERISTIC    0x2
#define FLAG_CHARACTERISTIC        (FLAG_DEF_CHARACTERISTIC | FLAG_REF_CHARACTERISTIC)
#define FLAG_IN_MEASUREMENT        0x4
#define FLAG_OUT_MEASUREMENT       0x8
#define FLAG_LOC_MEASUREMENT      0x10
#define FLAG_MEASUREMENT          (FLAG_IN_MEASUREMENT | FLAG_OUT_MEASUREMENT | FLAG_LOC_MEASUREMENT)
#define FLAG_SUB_FUNCTION         0x20
int GetNextFunctionMember (ASAP2_DATABASE *Database, int FuncIdx, int MemberIdx, const char *Filter, unsigned int Flags, char *RetName, int MaxSize, int *RetType);

// Gibt den Namen des naechsten Measurement in der eingelesenen A2L-Datenstruktur zurueck.
// Wenn 'Index' = -1 wird von Begin gesucht. Ansonsten sollte der Rueckgabewert des
// vorherigen Aufrufes uebergeben werden. Ist der Return-Wert -1 ist kein
// weiteres Measurement mehr vorhanden.
int GetNextMeasurement (ASAP2_DATABASE *Database, int Index, const char *Filter, unsigned int Flags, char *RetName, int MaxSize);

// Gibt den Index eines Measurement in der A2L-Datenstruktur mit dem 'Namen ' zurueck.
// wenn dierser nicht exisitiert wird -1 zurueckgegeben.
int GetMeasurementIndex (ASAP2_DATABASE *Database, const char *Name);

// Liest alle Infos bezueglich eines Measurement aus der A2L-Datenstruktur
int GetMeasurementInfos (ASAP2_DATABASE *Database, int Index,
                         char *Name, int MaxSizeName,
                         int *pType, uint64_t *pAddress,
                         int *pConvType, char *Conv, int MaxSizeConv,
                         char *Unit, int MaxSizeUnit,
                         int *pFormatLength, int *pFormatLayout,
                         int *pXDim, int *pYDim, int *pZDim,
                         double *pMin, double *pMax, int *pIsWritable);


// Gibt den Namen des naechsten Characteristic in der eingelesenen A2L-Datenstruktur zurueck.
// Wenn 'Index' = -1 wird von Begin gesucht. Ansonsten sollte der Rueckgabewert des
// vorherigen Aufrufes uebergeben werden. Ist der Return-Wert -1 ist kein
// weiterer Characteristic mehr vorhanden. 
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
