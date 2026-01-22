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


#ifndef BLACKBOARDCONVERSION_H
#define BLACKBOARDCONVERSION_H

#include "Blackboard.h"

// Rationl function
int Conv_RationalFunctionRawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, double par_RawValue,  double *ret_PhysValue);
int Conv_ParseRationalFunctionString(const char *par_ConvString, BB_VARIABLE_CONVERSION *ret_Conv);
int Conv_RationalFunctionRawToPhysFromString(const char *par_ConvString, double par_RawValue,  double *ret_PhysValue);

int Conv_RationalFunctionPhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue,  double *ret_RawValue);
int Conv_RationalFunctionPhysToRawFromString(const char *par_ConvString, double par_PhysValue,  double *ret_RawValue);

// Factor offset
int FactorOffsetRawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, double par_RawValue,  double *ret_PhysValue);
int Conv_ParseFactorOffsetString(const char *par_ConvString, BB_VARIABLE_CONVERSION *ret_Conv);
int Conv_FactorOffsetRawToPhysFromString(const char *par_ConvString, double par_RawValue,  double *ret_PhysValue);

int Conv_FactorOffsetPhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue,  double *ret_RawValue);
int Conv_FactorOffsetPhysToRawFromString(const char *par_ConvString, double par_PhysValue,  double *ret_RawValue);

// Offset factor
int Conv_OffsetFactorRawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, double par_RawValue,  double *ret_PhysValue);
int Conv_ParseOffsetFactorString(const char *par_ConvString, BB_VARIABLE_CONVERSION *ret_Conv);
int Conv_OffsetFactorRawToPhysFromString(const char *par_ConvString, double par_RawValue,  double *ret_PhysValue);

int Conv_OffsetFactorPhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue,  double *ret_RawValue);
int Conv_OffsetFactorPhysToRawFromString(const char *par_ConvString, double par_PhysValue,  double *ret_RawValue);

// Table interpol
int Conv_TableInterpolRawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, double par_RawValue,  double *ret_PhysValue);
int Conv_ParseTableInterpolString(const char *par_ConvString, BB_VARIABLE_CONVERSION *ret_Conv);
int Conv_TableInterpolRawToPhysFromString(const char *par_ConvString, double par_RawValue,  double *ret_PhysValue);

int Conv_TableInterpolPhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue,  double *ret_RawValue);
int Conv_TableInterpolPhysToRawFromString(const char *par_ConvString, double par_PhysValue,  double *ret_RawValue);

// Table no interpol
int Conv_TableNoInterpolRawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, double par_RawValue,  double *ret_PhysValue);
int Conv_ParseTableNoInterpolString(const char *par_ConvString, BB_VARIABLE_CONVERSION *ret_Conv);
int Conv_TableNoInterpolRawToPhysFromString(const char *par_ConvString, double par_RawValue,  double *ret_PhysValue);

int Conv_TableNoInterpolPhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue,  double *ret_RawValue);
int Conv_TableNoInterpolPhysToRawFromString(const char *par_ConvString, double par_PhysValue,  double *ret_RawValue);

// Formula
int Conv_FormulaRawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, int par_Vid, double par_RawValue, double *ret_PhysValue);
int Conv_ParseFormulaString(const char *par_ConvString, const char *par_BlackboardVariableName,
                       BB_VARIABLE_CONVERSION *ret_Conv);
int Conv_FormulaPhysToRawFromString(const char *par_ConvString, const char *par_BlackboardVariableName,
                               double par_PhysValue, double *ret_RawValue, double *ret_PhysValue,
                               char **ret_ErrorString);

int Conv_FormulaPhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue,  double *ret_RawValue);
int Conv_FormulaRawToPhysFromString(const char *par_ConvString, const char *par_BlackboardVariableName,
                               double par_RawValue, double *ret_PhysValue,
                               char **ret_ErrorString);

// All type of conversion (except text replace)
int Conv_RawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, int par_Vid, double par_RawValue, double *ret_PhysValue);
int Conv_ParseString(const char *par_ConvString, enum BB_CONV_TYPES par_Type,
                     const char *par_BlackboardVariableName, BB_VARIABLE_CONVERSION *ret_Conv);
int Conv_RawToPhysFromString(const char *par_ConvString, enum BB_CONV_TYPES par_Type, const char *par_BlackboardVariableName,
                             double par_RawValue, double *ret_PhysValue,
                             char **ret_ErrorString);

int Conv_PhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue, double *ret_RawValue);
int Conv_PhysToRawFromString(const char *par_ConvString, enum BB_CONV_TYPES par_Type, const char *par_BlackboardVariableName,
                             double par_PhysValue, double *ret_RawValue, double *ret_PhysValue,
                             char **ret_ErrorString);

enum BB_CONV_TYPES TranslateA2lToBlackboardConvType(int par_Type);
int TranslateBlackboardToA2lConvType(enum BB_CONV_TYPES par_Type);

#endif

