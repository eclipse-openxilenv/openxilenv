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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "MyMemory.h"
#include "Blackboard.h"
#include "EquationParser.h"
#include "ExecutionStack.h"
#ifndef REMOTE_MASTER
#include "EquationList.h"
#endif
#include "StringMaxChar.h"
#include "ThrowError.h"

#include "BlackboardConversion.h"

#define UNUSED(x) (void)(x)


// Rational function

int Conv_RationalFunctionRawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, double par_RawValue,  double *ret_PhysValue)
{
    if (par_Conversion->Conv.RatFunc.a != 0.0) {  // if a*x*x not zero
        if ((par_Conversion->Conv.RatFunc.b == 0.0) &&
            (par_Conversion->Conv.RatFunc.d == 0.0) &&
            (par_Conversion->Conv.RatFunc.e == 0.0) &&
            (par_Conversion->Conv.RatFunc.f != 0.0)) {
            *ret_PhysValue = sqrt((par_RawValue * par_Conversion->Conv.RatFunc.f - par_Conversion->Conv.RatFunc.c)
                                  / par_Conversion->Conv.RatFunc.a);
        } else {
            return -1;  // cannot calculated
        }
    } else { // if a*x*x == 0.0
        if (par_Conversion->Conv.RatFunc.b != 0.0) {  // if b*x not zero
            if ((par_Conversion->Conv.RatFunc.d == 0.0) &&
                (par_Conversion->Conv.RatFunc.e == 0.0) &&
                (par_Conversion->Conv.RatFunc.f != 0.0)) {
                *ret_PhysValue = (par_RawValue * par_Conversion->Conv.RatFunc.f - par_Conversion->Conv.RatFunc.c)
                                      / par_Conversion->Conv.RatFunc.b;
            } else {
                return -1;  // cannot calculated
            }
        } else { // if a*x*x == 0.0 and b*x == 0.0
            if (par_Conversion->Conv.RatFunc.c != 0.0) {  // if c not zero
                if (par_Conversion->Conv.RatFunc.d != 0.0) {  // if d*x*x not zero
                    if (par_Conversion->Conv.RatFunc.e == 0.0) {
                        *ret_PhysValue =  sqrt(((par_Conversion->Conv.RatFunc.c / par_RawValue) - par_Conversion->Conv.RatFunc.f)
                                              / par_Conversion->Conv.RatFunc.d);
                    } else {
                        return -1;  // cannot calculated
                    }
                } else { // if a*x*x == 0.0 and b*x and d*x == 0.0
                    if (par_Conversion->Conv.RatFunc.e != 0.0) {  // if e*x not zero
                        *ret_PhysValue =  ((par_Conversion->Conv.RatFunc.c / par_RawValue) - par_Conversion->Conv.RatFunc.f)
                                          / par_Conversion->Conv.RatFunc.e;
                    } else {
                        // this will be an fixed value
                        *ret_PhysValue =  (par_Conversion->Conv.RatFunc.f / par_Conversion->Conv.RatFunc.c);
                    }
                }
            } else {
                return -1;  // cannot calculated
            }
        }
    }
    return 0;
}

int Conv_ParseRationalFunctionString(const char *par_ConvString, BB_VARIABLE_CONVERSION *ret_Conv)
{
    char *p = (char*)par_ConvString;
    ret_Conv->Conv.RatFunc.a = strtod(p, &p);
    if(*p == ':') {
        ret_Conv->Conv.RatFunc.b = strtod(p+1, &p);
        if(*p == ':') {
            ret_Conv->Conv.RatFunc.c = strtod(p+1, &p);
            if(*p == ':') {
                ret_Conv->Conv.RatFunc.d = strtod(p+1, &p);
                if(*p == ':') {
                    ret_Conv->Conv.RatFunc.e = strtod(p+1, &p);
                    if(*p == ':') {
                        ret_Conv->Conv.RatFunc.f = strtod(p+1, &p);
                        if(*p == 0) {
                            ret_Conv->Type = BB_CONV_RAT_FUNC;
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return -1;
}

int Conv_RationalFunctionRawToPhysFromString(const char *par_ConvString, double par_RawValue,  double *ret_PhysValue)
{
    BB_VARIABLE_CONVERSION Conv;
    if (Conv_ParseRationalFunctionString(par_ConvString, &Conv)) {
        return -1;
    }
    return Conv_RationalFunctionRawToPhys(&Conv, par_RawValue, ret_PhysValue);
}


int Conv_RationalFunctionPhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue,  double *ret_RawValue)
{
    double a, b;
    // f(x)=(axx + bx + c)/(dxx + ex + f)
    // Raw = f(Phys)
    a = par_Conversion->Conv.RatFunc.a * par_PhysValue * par_PhysValue +
        par_Conversion->Conv.RatFunc.b * par_PhysValue +
        par_Conversion->Conv.RatFunc.c;
    b = par_Conversion->Conv.RatFunc.d * par_PhysValue * par_PhysValue +
        par_Conversion->Conv.RatFunc.e * par_PhysValue +
        par_Conversion->Conv.RatFunc.f;
    if (b != 0.0) {
        *ret_RawValue = a / b;
        return 0;
    } else {
        *ret_RawValue = DBL_MAX;
        return 1;
    }
}

int Conv_RationalFunctionPhysToRawFromString(const char *par_ConvString, double par_RawValue,  double *ret_PhysValue)
{
    BB_VARIABLE_CONVERSION Conv;
    if (Conv_ParseRationalFunctionString(par_ConvString, &Conv)) {
        return -1;
    }
    return Conv_RationalFunctionPhysToRaw(&Conv, par_RawValue, ret_PhysValue);
}

// Factor offset

int Conv_FactorOffsetRawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, double par_RawValue,  double *ret_PhysValue)
{
    *ret_PhysValue = par_Conversion->Conv.FactorOffset.Factor * par_RawValue +
                     par_Conversion->Conv.FactorOffset.Offset;
    return 0;
}

int Conv_ParseFactorOffsetString(const char *par_ConvString, BB_VARIABLE_CONVERSION *ret_Conv)
{
    char *p = (char*)par_ConvString;
    ret_Conv->Conv.FactorOffset.Factor = strtod(p, &p);
    if(*p == ':') {
        ret_Conv->Conv.FactorOffset.Offset = strtod(p+1, &p);
        if(*p == 0) {
            ret_Conv->Type = BB_CONV_FACTOFF;
            return 0;
        }
    }
    return -1;
}

int Conv_FactorOffsetRawToPhysFromString(const char *par_ConvString, double par_RawValue,  double *ret_PhysValue)
{
    BB_VARIABLE_CONVERSION Conv;
    if (Conv_ParseFactorOffsetString(par_ConvString, &Conv)) {
        return -1;
    }
    return Conv_FactorOffsetRawToPhys(&Conv, par_RawValue, ret_PhysValue);
}

int Conv_FactorOffsetPhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue,  double *ret_RawValue)
{
    if (par_Conversion->Conv.FactorOffset.Factor != 0.0) {
        *ret_RawValue = (par_PhysValue - par_Conversion->Conv.FactorOffset.Offset) /
                        par_Conversion->Conv.FactorOffset.Factor;
        return 0;
    } else {
        return -1;
    }
}

int Conv_FactorOffsetPhysToRawFromString(const char *par_ConvString, double par_PhysValue,  double *ret_RawValue)
{
    BB_VARIABLE_CONVERSION Conv;
    if (Conv_ParseFactorOffsetString(par_ConvString, &Conv)) {
        return -1;
    }
    return Conv_FactorOffsetPhysToRaw(&Conv, par_PhysValue, ret_RawValue);
}

// Offset factor

int Conv_OffsetFactorRawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, double par_RawValue,  double *ret_PhysValue)
{
    *ret_PhysValue = par_Conversion->Conv.FactorOffset.Factor * (par_RawValue +
                                                                 par_Conversion->Conv.FactorOffset.Offset);
    return 0;
}

int Conv_ParseOffsetFactorString(const char *par_ConvString, BB_VARIABLE_CONVERSION *ret_Conv)
{
    char *p = (char*)par_ConvString;
    ret_Conv->Conv.FactorOffset.Factor = strtod(p, &p);
    if(*p == ':') {
        ret_Conv->Conv.FactorOffset.Offset = strtod(p+1, &p);
        if(*p == 0) {
            ret_Conv->Type = BB_CONV_FACTOFF;
            return 0;
        }
    }
    return -1;
}

int Conv_OffsetFactorRawToPhysFromString(const char *par_ConvString, double par_RawValue,  double *ret_PhysValue)
{
    BB_VARIABLE_CONVERSION Conv;
    if (Conv_ParseOffsetFactorString(par_ConvString, &Conv)) {
        return -1;
    }
    return Conv_OffsetFactorRawToPhys(&Conv, par_RawValue, ret_PhysValue);
}

int Conv_OffsetFactorPhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue,  double *ret_RawValue)
{
    if (par_Conversion->Conv.FactorOffset.Factor != 0.0) {
        *ret_RawValue = par_PhysValue / par_Conversion->Conv.FactorOffset.Factor -
                        par_Conversion->Conv.FactorOffset.Offset;
        return 0;
    } else {
        return -1;
    }
}

int Conv_OffsetFactorPhysToRawFromString(const char *par_ConvString, double par_PhysValue,  double *ret_RawValue)
{
    BB_VARIABLE_CONVERSION Conv;
    if (Conv_ParseOffsetFactorString(par_ConvString, &Conv)) {
        return -1;
    }
    return Conv_OffsetFactorPhysToRaw(&Conv, par_PhysValue, ret_RawValue);
}

// Table interpol
int Conv_TableInterpolRawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, double par_RawValue,  double *ret_PhysValue)
{
    int x, Size;
    double m, RawDelta, PhysDelta;
    struct CONVERSION_TABLE_VALUE_PAIR *Values;

    Size = par_Conversion->Conv.Table.Size;
    Values = par_Conversion->Conv.Table.Values;
    for (x = 0; x < Size; x++) {
        if (Values[x].Raw >= par_RawValue) {
            if (x == 0) {
                *ret_PhysValue = Values[x].Phys;
            } else {
                RawDelta = Values[x].Raw - Values[x-1].Raw;
                if ((RawDelta <= 0.0) && (RawDelta >= 0.0)) {  // == 0.0
                    *ret_PhysValue = Values[x-1].Phys;  // use the smallest one
                } else {
                    PhysDelta = Values[x].Phys - Values[x-1].Phys;
                    m = PhysDelta / RawDelta;
                    *ret_PhysValue = Values[x-1].Phys + m * (par_RawValue - Values[x-1].Raw);
                }
            }
            break;  // for(;;)
        }
    }
    if (x == Size) {
        *ret_PhysValue = Values[Size - 1].Phys;
    }
    return 0;
}

int Conv_ParseTableInterpolString(const char *par_ConvString, BB_VARIABLE_CONVERSION *ret_Conv)
{
    int Size, x;
    char *p = (char*)par_ConvString;
    Size = 1;
    while (*p != 0) {
        if(*p == ':') Size++;
        p++;
    }
    ret_Conv->Conv.Table.Values = (struct CONVERSION_TABLE_VALUE_PAIR*)my_malloc(Size * sizeof(struct CONVERSION_TABLE_VALUE_PAIR));
    if (ret_Conv->Conv.Table.Values == NULL) {
        return -1;
    }
    p = (char*)par_ConvString;
    for (x = 0; x < Size; x++) {
        ret_Conv->Conv.Table.Values[x].Phys = strtod(p, &p);
        if(*p != '/') break;
        ret_Conv->Conv.Table.Values[x].Raw = strtod(p+1, &p);
        if(*p != ':') {
            if(*p != 0) break;
        } else p++;
    }
    if ((*p == 0) && (x == Size)) {
        ret_Conv->Conv.Table.Size = Size;
        ret_Conv->Type = BB_CONV_TAB_INTP;
        return 0;
    } else {
        ret_Conv->Type = BB_CONV_NONE;
        my_free(ret_Conv->Conv.Table.Values);
        return -1;
    }
}

int Conv_TableInterpolRawToPhysFromString(const char *par_ConvString, double par_RawValue,  double *ret_PhysValue)
{
    int Ret;
    BB_VARIABLE_CONVERSION Conv;
    if (Conv_ParseTableInterpolString(par_ConvString, &Conv)) {
        return -1;
    }
    Ret = Conv_TableInterpolRawToPhys(&Conv, par_RawValue, ret_PhysValue);
    my_free(Conv.Conv.Table.Values);
    return Ret;
}

int Conv_TableInterpolPhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue,  double *ret_RawValue)
{
    int x, Size;
    double m, RawDelta, PhysDelta;
    struct CONVERSION_TABLE_VALUE_PAIR *Values;

    Size = par_Conversion->Conv.Table.Size;
    Values = par_Conversion->Conv.Table.Values;
    for (x = 0; x < Size; x++) {
        if (Values[x].Phys >= par_PhysValue) {
            if (x == 0) {
                *ret_RawValue = Values[x].Raw;
            } else {
                PhysDelta = Values[x].Phys - Values[x-1].Phys;
                if ((PhysDelta <= 0.0) && (PhysDelta >= 0.0)) {  // == 0.0
                    *ret_RawValue = Values[x-1].Raw;  // use the smallest one
                } else {
                    RawDelta = Values[x].Raw - Values[x-1].Raw;
                    m = RawDelta / PhysDelta;
                    *ret_RawValue = Values[x-1].Raw + m * (par_PhysValue - Values[x-1].Phys);
                }
            }
            break;  // for(;;)
        }
    }
    if (x == Size) {
        *ret_RawValue = Values[Size - 1].Raw;
    }
    return 0;
}

int Conv_TableInterpolPhysToRawFromString(const char *par_ConvString, double par_PhysValue,  double *ret_RawValue)
{
    int Ret;
    BB_VARIABLE_CONVERSION Conv;
    if (Conv_ParseTableInterpolString(par_ConvString, &Conv)) {
        return -1;
    }
    Ret = Conv_TableInterpolPhysToRaw(&Conv, par_PhysValue, ret_RawValue);
    my_free(Conv.Conv.Table.Values);
    return Ret;
}

// Table no interpol
int Conv_TableNoInterpolRawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, double par_RawValue,  double *ret_PhysValue)
{
    int x, Size;
    double m, RawDelta, PhysDelta;
    struct CONVERSION_TABLE_VALUE_PAIR *Values;

    Size = par_Conversion->Conv.Table.Size;
    Values = par_Conversion->Conv.Table.Values;
    for (x = 0; x < Size; x++) {
        if (Values[x].Raw >= par_RawValue) {
            *ret_PhysValue = Values[x].Phys;
            break;  // for(;;)
        }
    }
    if (x == Size) {
        *ret_PhysValue = Values[Size - 1].Phys;
    }
    return 0;
}
int Conv_ParseTableNoInterpolString(const char *par_ConvString, BB_VARIABLE_CONVERSION *ret_Conv)
{
    int Ret;
    Ret = Conv_ParseTableInterpolString(par_ConvString, ret_Conv);
    ret_Conv->Type = BB_CONV_TAB_NOINTP;
    return Ret;
}

int Conv_TableNoInterpolRawToPhysFromString(const char *par_ConvString, double par_RawValue,  double *ret_PhysValue)
{
    int Ret;
    BB_VARIABLE_CONVERSION Conv;
    if (Conv_ParseTableNoInterpolString(par_ConvString, &Conv)) {
        return -1;
    }
    Ret = Conv_TableNoInterpolRawToPhys(&Conv, par_RawValue, ret_PhysValue);
    my_free(Conv.Conv.Table.Values);
    return Ret;
}

int Conv_TableNoInterpolPhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue,  double *ret_RawValue)
{
    int x, Size;
    double m, RawDelta, PhysDelta;
    struct CONVERSION_TABLE_VALUE_PAIR *Values;

    Size = par_Conversion->Conv.Table.Size;
    Values = par_Conversion->Conv.Table.Values;
    for (x = 0; x < Size; x++) {
        if (Values[x].Phys >= par_PhysValue) {
            *ret_RawValue = Values[x].Raw;
            break;  // for(;;)
        }
    }
    if (x == Size) {
        *ret_RawValue = Values[Size - 1].Raw;
    }
    return 0;
}

int Conv_TableNoInterpolPhysToRawFromString(const char *par_ConvString, double par_PhysValue,  double *ret_RawValue)
{
    int Ret;
    BB_VARIABLE_CONVERSION Conv;
    if (Conv_ParseTableNoInterpolString(par_ConvString, &Conv)) {
        return -1;
    }
    Ret = Conv_TableNoInterpolPhysToRaw(&Conv, par_PhysValue, ret_RawValue);
    my_free(Conv.Conv.Table.Values);
    return Ret;
}

// Formula
int Conv_FormulaRawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, int par_Vid, double par_RawValue,  double *ret_PhysValue)
{
    if (par_Vid > 0) {
        *ret_PhysValue = execute_stack_replace_variable_with_parameter((struct EXEC_STACK_ELEM*)par_Conversion->Conv.Formula.FormulaByteCode,
                                                    par_Vid,
                                                    par_RawValue);
    } else {
        *ret_PhysValue = execute_stack_whith_parameter ((struct EXEC_STACK_ELEM*)par_Conversion->Conv.Formula.FormulaByteCode,
                                                        par_RawValue);
    }
    return 0;
}

int Conv_ParseFormulaString(const char *par_ConvString, const char *par_BlackboardVariableName,
                            BB_VARIABLE_CONVERSION *ret_Conv)
{
    struct EXEC_STACK_ELEM *ExecStack;

    if ((ExecStack = solve_equation_replace_parameter (par_ConvString)) == NULL) {
        return -1;
    }
    ret_Conv->Conv.Formula.FormulaString = StringMalloc(par_ConvString);
    ret_Conv->Conv.Formula.FormulaByteCode = (unsigned char*)ExecStack;
    ret_Conv->Type = BB_CONV_FORMULA;
#ifndef REMOTE_MASTER
    RegisterEquation (0, par_ConvString, ExecStack,
                     par_BlackboardVariableName, EQU_TYPE_BLACKBOARD);
#endif
    return 0;
}

int Conv_FormulaRawToPhysFromString(const char *par_ConvString, const char *par_BlackboardVariableName,
                               double par_RawValue, double *ret_PhysValue,
                               char **ret_ErrorString)
{
    return direct_solve_equation_err_string_replace_var_value (par_ConvString, par_BlackboardVariableName,
                                                               par_RawValue, ret_PhysValue, ret_ErrorString);
}

int Conv_FormulaPhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue, double *ret_RawValue)
{
    ThrowError(1, "FormulaPhysToRaw() is not implemented");
    return -1;
}

int Conv_FormulaPhysToRawFromString(const char *par_ConvString, const char *par_BlackboardVariableName,
                               double par_PhysValue, double *ret_RawValue, double *ret_PhysValue,
                               char **ret_ErrorString)
{
    if (calc_raw_value_for_phys_value (par_ConvString, par_PhysValue, par_BlackboardVariableName, BB_DOUBLE,
                                      ret_RawValue, ret_PhysValue, ret_ErrorString)) {
        return -1;
    } else {
        return 0;
    }
}

// All type of conversion (except text replace)
int Conv_RawToPhys(BB_VARIABLE_CONVERSION *par_Conversion, int par_Vid, double par_RawValue, double *ret_PhysValue)
{
    switch(par_Conversion->Type) {
    case BB_CONV_FORMULA:
        return Conv_FormulaRawToPhys(par_Conversion, par_Vid, par_RawValue, ret_PhysValue);
    case BB_CONV_FACTOFF:
        return Conv_FactorOffsetRawToPhys(par_Conversion, par_RawValue, ret_PhysValue);
    case BB_CONV_OFFFACT:
        return Conv_OffsetFactorRawToPhys(par_Conversion, par_RawValue, ret_PhysValue);
    case BB_CONV_TAB_INTP:
        return Conv_TableInterpolPhysToRaw(par_Conversion, par_RawValue, ret_PhysValue);
    case BB_CONV_TAB_NOINTP:
        return Conv_TableNoInterpolPhysToRaw(par_Conversion, par_RawValue, ret_PhysValue);
    case BB_CONV_RAT_FUNC:
        return Conv_RationalFunctionPhysToRaw(par_Conversion, par_RawValue, ret_PhysValue);
    case BB_CONV_TEXTREP:
    case BB_CONV_NONE:
    default:
        return -1;
    }
}

int Conv_ParseString(const char *par_ConvString, enum BB_CONV_TYPES par_Type,
                     const char *par_BlackboardVariableName, BB_VARIABLE_CONVERSION *ret_Conv)
{
    switch(par_Type) {
    case BB_CONV_FORMULA:
        return Conv_ParseFormulaString(par_ConvString, par_BlackboardVariableName, ret_Conv);
    case BB_CONV_FACTOFF:
        return Conv_ParseFactorOffsetString(par_ConvString, ret_Conv);
    case BB_CONV_OFFFACT:
        return Conv_ParseOffsetFactorString(par_ConvString, ret_Conv);
    case BB_CONV_TAB_INTP:
        return Conv_ParseTableInterpolString(par_ConvString, ret_Conv);
    case BB_CONV_TAB_NOINTP:
        return Conv_ParseTableNoInterpolString(par_ConvString, ret_Conv);
    case BB_CONV_RAT_FUNC:
        return Conv_ParseRationalFunctionString(par_ConvString, ret_Conv);
    case BB_CONV_TEXTREP:
    case BB_CONV_NONE:
    default:
        return -1;
    }
}

int Conv_RawToPhysFromString(const char *par_ConvString, enum BB_CONV_TYPES par_Type, const char *par_BlackboardVariableName,
                             double par_RawValue, double *ret_PhysValue,
                             char **ret_ErrorString)
{
    int Ret = -1;
    switch(par_Type) {
    case BB_CONV_FORMULA:
        return Conv_FormulaRawToPhysFromString(par_ConvString, par_BlackboardVariableName, par_RawValue, ret_PhysValue, ret_ErrorString);
        // no break!
    case BB_CONV_FACTOFF:
        Ret = Conv_FactorOffsetRawToPhysFromString(par_ConvString, par_RawValue, ret_PhysValue);
        break;
    case BB_CONV_OFFFACT:
        Ret =  Conv_OffsetFactorRawToPhysFromString(par_ConvString, par_RawValue, ret_PhysValue);
        break;
    case BB_CONV_TAB_INTP:
        Ret =  Conv_TableInterpolRawToPhysFromString(par_ConvString, par_RawValue, ret_PhysValue);
        break;
    case BB_CONV_TAB_NOINTP:
        Ret =  Conv_TableNoInterpolRawToPhysFromString(par_ConvString, par_RawValue, ret_PhysValue);
        break;
    case BB_CONV_RAT_FUNC:
        Ret =  Conv_RationalFunctionRawToPhysFromString(par_ConvString, par_RawValue, ret_PhysValue);
        break;
    case BB_CONV_TEXTREP:
        break;
    case BB_CONV_NONE:
    default:
        Ret = -1;
        break;
    }
    if (Ret) {
        if (ret_ErrorString != NULL) {
            *ret_ErrorString = StringMalloc("cannot solve conversion");
        }
    }
    return Ret;
}

int Conv_PhysToRaw(BB_VARIABLE_CONVERSION *par_Conversion, double par_PhysValue, double *ret_RawValue)
{
    switch(par_Conversion->Type) {
    case BB_CONV_FORMULA:
        return Conv_FormulaPhysToRaw(par_Conversion, par_PhysValue, ret_RawValue);
    case BB_CONV_FACTOFF:
        return Conv_FactorOffsetPhysToRaw(par_Conversion, par_PhysValue, ret_RawValue);
    case BB_CONV_OFFFACT:
        return Conv_OffsetFactorPhysToRaw(par_Conversion, par_PhysValue, ret_RawValue);
    case BB_CONV_TAB_INTP:
        return Conv_TableInterpolPhysToRaw(par_Conversion, par_PhysValue, ret_RawValue);
    case BB_CONV_TAB_NOINTP:
        return Conv_TableNoInterpolPhysToRaw(par_Conversion, par_PhysValue, ret_RawValue);
    case BB_CONV_RAT_FUNC:
        return Conv_RationalFunctionPhysToRaw(par_Conversion, par_PhysValue, ret_RawValue);
    case BB_CONV_TEXTREP:
    case BB_CONV_NONE:
    default:
        return -1;
    }
}

int Conv_PhysToRawFromString(const char *par_ConvString, enum BB_CONV_TYPES par_Type, const char *par_BlackboardVariableName,
                             double par_PhysValue, double *ret_RawValue, double *ret_PhysValue,
                             char **ret_ErrorString)
{
    int Ret = -1;
    switch(par_Type) {
    case BB_CONV_FORMULA:
        return Conv_FormulaPhysToRawFromString(par_ConvString, par_BlackboardVariableName, par_PhysValue, ret_RawValue, ret_PhysValue, ret_ErrorString);
        // no break!
    case BB_CONV_FACTOFF:
        Ret = Conv_FactorOffsetPhysToRawFromString(par_ConvString, par_PhysValue, ret_RawValue);
        break;
    case BB_CONV_OFFFACT:
        Ret =  Conv_OffsetFactorPhysToRawFromString(par_ConvString, par_PhysValue, ret_RawValue);
        break;
    case BB_CONV_TAB_INTP:
        Ret =  Conv_TableInterpolPhysToRawFromString(par_ConvString, par_PhysValue, ret_RawValue);
        break;
    case BB_CONV_TAB_NOINTP:
        Ret =  Conv_TableNoInterpolPhysToRawFromString(par_ConvString, par_PhysValue, ret_RawValue);
        break;
    case BB_CONV_RAT_FUNC:
        Ret =  Conv_RationalFunctionRawToPhysFromString(par_ConvString, par_PhysValue, ret_RawValue);
        break;
    case BB_CONV_TEXTREP:
        break;
    case BB_CONV_NONE:
    default:
        Ret = -1;
        break;
    }
    if (Ret) {
        if (ret_ErrorString != NULL) {
            *ret_ErrorString = StringMalloc("cannot solve conversion");
        }
    }
    return Ret;
}

enum BB_CONV_TYPES TranslateA2lToBlackboardConvType(int par_Type)
{
    switch(par_Type) {
    case 0: // IDENTICAL
        return BB_CONV_NONE;
    case 1: // LINEAR
        return BB_CONV_FACTOFF;
    case 2: // RAT_FUNC
        return BB_CONV_RAT_FUNC;
    case 3: // FORM
        return BB_CONV_FORMULA;
    case 4: // TAB_INTP
        return BB_CONV_TAB_INTP;
    case 5: // TAB_NOINTP
        return BB_CONV_TAB_NOINTP;
    case 6: // TAB_VERB
        return BB_CONV_TEXTREP;
    default:
        break;
    }
    return -1;
}

int TranslateBlackboardToA2lConvType(enum BB_CONV_TYPES par_Type)
{
    switch(par_Type) {
    case BB_CONV_NONE: // IDENTICAL
        return 0;
    case BB_CONV_FACTOFF: // LINEAR
        return 1;
    case BB_CONV_RAT_FUNC: // RAT_FUNC
        return 2;
    case BB_CONV_FORMULA: // FORM
        return 3;
    case BB_CONV_TAB_INTP: // TAB_INTP
        return 4;
    case BB_CONV_TAB_NOINTP: // TAB_NOINTP
        return 5;
    case BB_CONV_TEXTREP: // TAB_VERB
        return 6;
    default:
        break;
    }
    return -1;
}
