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
#include "Platform.h"
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#include "StringMaxChar.h"
#include "BlackboardAccess.h"
#include "BlackboardConversion.h"

#include "A2LBuffer.h"
#include "A2LParser.h"
#include "A2LAccess.h"
#include "A2LLink.h"

#include "DebugInfos.h"
#include "ReadWriteValue.h"
#include "EquationParser.h"

#include "A2LAccessData.h"

static int BsearchCompareFunc (const void *arg1, const void *arg2)
{
    return strcmp (**(char***)arg1, **(char***)arg2);
}

// Phys -> Raw

static int ConvertTextReplaceToRaw (ASAP2_MODULE_DATA* Module, const char *Name, A2L_SINGLE_VALUE *par_TextRepace, A2L_SINGLE_VALUE *ret_Raw)
{
    int x;
    ASAP2_COMPU_TAB *CompuTab, **__CompuTab;
    const char **pName = &Name;

    __CompuTab = bsearch (&pName, Module->CompuTabs, Module->CompuTabCounter, sizeof(ASAP2_COMPU_TAB*), BsearchCompareFunc);
    if (__CompuTab != NULL) {
        CompuTab = *__CompuTab;
        switch (CompuTab->TabOrVtabFlag) {
        case 0: // COMPU_TAB will be ignored
        default:
            return -1;
            break;
        case 1: // COMPU_VTAB
        case 2: // COMPU_VTAB_RANGE
            for (x = 0; x < CompuTab->NumberValuePairs; x++) {
                if (!strcmp(par_TextRepace->Value.String, CompuTab->ValuePairs[x].OutString)) {
                    ret_Raw->Type = A2L_ELEM_TYPE_INT;
                    // for a COMPU_VTAB_RANGE also use the min. range value!
                    ret_Raw->Value.Int = (int64_t)CompuTab->ValuePairs[x].InVal_InValMin;
                    // al other will be copied
                    ret_Raw->Flags = par_TextRepace->Flags;
                    ret_Raw->TargetType = par_TextRepace->TargetType;
                    ret_Raw->Address = par_TextRepace->Address;
                    ret_Raw->SizeOfStruct = sizeof(A2L_SINGLE_VALUE);
                    break;   // found
                }
            }
            return 0;
        }
    }
    return -1;
}

static int ConvertValuePairsToRaw (ASAP2_MODULE_DATA* Module, const char *Name, int par_Interpol, A2L_SINGLE_VALUE *par_Phys, A2L_SINGLE_VALUE *ret_Raw)
{
    int x;
    ASAP2_COMPU_TAB *CompuTab, **__CompuTab;
    const char **pName = &Name;

    double NewPhysValue = ConvertRawValueToDouble(par_Phys);
    __CompuTab = bsearch (&pName, Module->CompuTabs, Module->CompuTabCounter, sizeof(ASAP2_COMPU_TAB*), BsearchCompareFunc);
    if (__CompuTab != NULL) {
        CompuTab = *__CompuTab;
        switch (CompuTab->TabOrVtabFlag) {
        case 0: // COMPU_TAB will be ignored
            for (x = 0; x < CompuTab->NumberValuePairs; x++) {
                if (CompuTab->ValuePairs[x].OutVal_InValMax >= NewPhysValue) {
                    if (x == 0) {
                        ret_Raw->Value.Double = CompuTab->ValuePairs[x].InVal_InValMin;
                        ret_Raw->Type = A2L_ELEM_TYPE_DOUBLE;
                    } else {
                        if (par_Interpol) {
                            double PhysDelta = CompuTab->ValuePairs[x].OutVal_InValMax - CompuTab->ValuePairs[x-1].OutVal_InValMax;
                            if ((PhysDelta <= 0.0) && (PhysDelta >= 0.0)) {  // == 0.0
                                ret_Raw->Value.Double = CompuTab->ValuePairs[x-1].InVal_InValMin;
                                ret_Raw->Type = A2L_ELEM_TYPE_DOUBLE;
                            } else {
                                double RawDelta = CompuTab->ValuePairs[x].InVal_InValMin - CompuTab->ValuePairs[x-1].InVal_InValMin;
                                double m = RawDelta / PhysDelta;
                                ret_Raw->Value.Double = CompuTab->ValuePairs[x-1].InVal_InValMin +
                                                        m * (NewPhysValue - CompuTab->ValuePairs[x-1].OutVal_InValMax);
                                ret_Raw->Type = A2L_ELEM_TYPE_DOUBLE;
                            }
                        } else {
                            ret_Raw->Value.Double = CompuTab->ValuePairs[x].InVal_InValMin;
                            ret_Raw->Type = A2L_ELEM_TYPE_DOUBLE;
                        }
                    }
                    break;  // for(;;)
                }
            }
            if (x == CompuTab->NumberValuePairs) {
                ret_Raw->Value.Double = CompuTab->ValuePairs[x-1].InVal_InValMin;
                ret_Raw->Type = A2L_ELEM_TYPE_DOUBLE;
            }
        case 1: // COMPU_VTAB
        case 2: // COMPU_VTAB_RANGE
        default:
            return -1;
            break;
        }
    }
    return -1;
}

static int ConvertWithFormulaToRaw (const char *Formula, A2L_SINGLE_VALUE *par_Phys, A2L_SINGLE_VALUE *ret_Raw)
{
    double RawValue;
    double MatchtPhysValue;
    char *ErrString;
    if (calc_raw_value_for_phys_value (Formula, par_Phys->Value.Double, "", par_Phys->TargetType, &RawValue, &MatchtPhysValue, &ErrString)) {
        FreeErrStringBuffer (&ErrString);
        return -1;
    } else {
        ret_Raw->Type = A2L_ELEM_TYPE_DOUBLE;
        ret_Raw->Value.Double = RawValue;
        // al other will be copied
        ret_Raw->Flags = par_Phys->Flags;
        ret_Raw->TargetType = par_Phys->TargetType;
        ret_Raw->Address = par_Phys->Address;
        ret_Raw->SizeOfStruct = sizeof(A2L_SINGLE_VALUE);
        return 0;
    }
}

int ConvertPhysToRaw(ASAP2_MODULE_DATA* Module, const char *par_ConvertName, A2L_SINGLE_VALUE *par_Phys, A2L_SINGLE_VALUE *ret_Raw)
{
    int Ret = -1;
    ASAP2_COMPU_METHOD *CompuMethod, **__CompuMethod;
    const char **pName = &par_ConvertName;

    __CompuMethod = bsearch (&pName, Module->CompuMethods, Module->CompuMethodCounter, sizeof(ASAP2_COMPU_METHOD*), BsearchCompareFunc);
    if (__CompuMethod != NULL) {
        CompuMethod = *__CompuMethod;
        // "IDENTICAL 0 LINEAR 1 RAT_FUNC 2 FORM 3 TAB_INTP 4 TAB_NOINTP 5 TAB_VERB 6 ? -1
        switch (CompuMethod->ConversionType) {
        case 0:  // IDENTICAL
            *ret_Raw = *par_Phys;
            Ret = 0;
            break;
        case 4:  // TAB_INTP
        case 5:  // TAB_NOINTP
            if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_COMPU_TAB_REF)) {
                if (par_Phys->Type != A2L_ELEM_TYPE_TEXT_REPLACE) {
                    if (!ConvertValuePairsToRaw (Module, CompuMethod->OptionalParameter.ConversionTable,
                                                (CompuMethod->ConversionType == 4), // interpolation?
                                                par_Phys, ret_Raw)) {
                        Ret = 0;
                    }
                }
            }
        default:
            // This will be ignored
            *ret_Raw = *par_Phys;
            break;
        case 6:  // TAB_VERB  -> ENUM (Textersatz)
            if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_COMPU_TAB_REF)) {
                if (par_Phys->Type == A2L_ELEM_TYPE_TEXT_REPLACE) {
                    if (!ConvertTextReplaceToRaw (Module, CompuMethod->OptionalParameter.ConversionTable, par_Phys, ret_Raw)) {
                        Ret = 0;
                    }
                }
            }
            break;
        case 2:  // RAT_FUNC
            if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_COEFFS)) {
                // INT = f(PHYS);
                // f(x) = (axx + bx + c) / (dxx + ex + f)
                double x = ConvertRawValueToDouble(par_Phys);
                double a = CompuMethod->OptionalParameter.Coeffs.a * x * x +
                           CompuMethod->OptionalParameter.Coeffs.b * x +
                           CompuMethod->OptionalParameter.Coeffs.c;
                double b = CompuMethod->OptionalParameter.Coeffs.d * x * x +
                           CompuMethod->OptionalParameter.Coeffs.e * x +
                           CompuMethod->OptionalParameter.Coeffs.f;
                if (b != 0.0) {
                    double Value = a / b;
                    ConvertDoubleToRawValue(par_Phys->TargetType, Value, ret_Raw);
                    Ret = 0;
                }
            }
            break;
        case 1:  // LINEAR slope and offset
            if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_COEFFS_LINEAR)) {
                double Value = (CompuMethod->OptionalParameter.Coeffs.a * ConvertRawValueToDouble(par_Phys)
                                         + CompuMethod->OptionalParameter.Coeffs.b);
                ConvertDoubleToRawValue(par_Phys->TargetType, Value, ret_Raw);
                Ret = 0;
            }
            break;
        case 3:  // FORM -> Formel zB: "x/1000" oder "X1*0.1234"
            if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_FORMULA)) {
                char Formula[1024];
                char *Src, *Dst;
                char LastChar;
                Src = CompuMethod->OptionalParameter.Formula;
                Dst = Formula;
                LastChar = 0;
                while (*Src != 0) {
                    // If ther is a x or X without a letter before or behind will be interpret as an place holder for a variable
                    if (((*Src == 'x') || (*Src == 'X')) && !(isalpha (LastChar) || isalpha(*(Src+1)))) {
                        *Dst = '#';
                        Src++;
                        if (*Src == '1') Src++;   // x1 bzw X1 will also be accept as place holder
                    } else *Dst++ = *Src++;
                }
                *Dst = 0;
                Ret = ConvertWithFormulaToRaw(Formula, par_Phys, ret_Raw);
            }
            break;
        }
    } else if (!strcmp("NO_COMPU_METHOD", par_ConvertName)) {
        ValueCopy(ret_Raw, par_Phys);
        Ret = 0;
    }
    return Ret;
}

// Raw -> Phys

static int ConvertRawToTextReplace (ASAP2_MODULE_DATA* Module, const char *Name, A2L_SINGLE_VALUE *par_Raw, A2L_SINGLE_VALUE *ret_TextRepace)
{
    int x;
    ASAP2_COMPU_TAB *CompuTab, **__CompuTab;
    const char **pName = &Name;

    __CompuTab = bsearch (&pName, Module->CompuTabs, Module->CompuTabCounter, sizeof(ASAP2_COMPU_TAB*), BsearchCompareFunc);
    if (__CompuTab != NULL) {
        ret_TextRepace->Type = A2L_ELEM_TYPE_TEXT_REPLACE;
        CompuTab = *__CompuTab;
        switch (CompuTab->TabOrVtabFlag) {
        case 0: // COMPU_TAB werden ignoriert
        default:
            return -1;
            break;
        case 1: // COMPU_VTAB
            for (x = 0; x < CompuTab->NumberValuePairs; x++) {
                if (ConvertRawValueToInt(par_Raw) == (int)CompuTab->ValuePairs[x].InVal_InValMin) {
                    StringCopyToValue (ret_TextRepace, CompuTab->ValuePairs[x].OutString, 512);
                    return 0;
                }
            }
            if (CheckIfFlagSetPos(CompuTab->OptionalParameter.Flags, OPTPARAM_COMPU_TAB_DEFAULT_VALUE)) {
                StringCopyToValue (ret_TextRepace, CompuTab->OptionalParameter.DefaultValue, 512);
                return 0;
            }
            return -1;
        case 2: // COMPU_VTAB_RANGE
            for (x = 0; x < CompuTab->NumberValuePairs; x++) {
                if ((ConvertRawValueToDouble(par_Raw) >= (int)CompuTab->ValuePairs[x].InVal_InValMin) &&
                    (ConvertRawValueToDouble(par_Raw) >= (int)CompuTab->ValuePairs[x].OutVal_InValMax)) {
                    StringCopyToValue (ret_TextRepace, CompuTab->ValuePairs[x].OutString, 512);
                    return 0;
                }
            }
            if (CheckIfFlagSetPos(CompuTab->OptionalParameter.Flags, OPTPARAM_COMPU_TAB_DEFAULT_VALUE)) {
                StringCopyToValue (ret_TextRepace, CompuTab->OptionalParameter.DefaultValue, 512);
                return 0;
            }
            return -1;
        }
    }
    return -1;
}

static int ConvertRawToPhysByTable (ASAP2_MODULE_DATA* Module, const char *Name, int par_Interpol, A2L_SINGLE_VALUE *par_Raw, A2L_SINGLE_VALUE *ret_Phys)
{
    int x;
    ASAP2_COMPU_TAB *CompuTab, **__CompuTab;
    const char **pName = &Name;
    double RawValue = ConvertRawValueToDouble(par_Raw);

    __CompuTab = bsearch (&pName, Module->CompuTabs, Module->CompuTabCounter, sizeof(ASAP2_COMPU_TAB*), BsearchCompareFunc);
    if (__CompuTab != NULL) {
        CompuTab = *__CompuTab;
        switch (CompuTab->TabOrVtabFlag) {
        case 0: // COMPU_TAB werden ignoriert
            for (x = 0; x < CompuTab->NumberValuePairs; x++) {
                if (CompuTab->ValuePairs[x].InVal_InValMin >= RawValue) {
                    if (x == 0) {
                        ret_Phys->Value.Double = CompuTab->ValuePairs[x].OutVal_InValMax;
                        ret_Phys->Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
                    } else {
                        if (par_Interpol) {
                            double RawDelta = CompuTab->ValuePairs[x].InVal_InValMin - CompuTab->ValuePairs[x-1].InVal_InValMin;
                            if ((RawDelta <= 0.0) && (RawDelta >= 0.0)) {  // == 0.0
                                ret_Phys->Value.Double = CompuTab->ValuePairs[x-1].OutVal_InValMax;
                                ret_Phys->Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
                            } else {
                                double PhysDelta = CompuTab->ValuePairs[x].OutVal_InValMax - CompuTab->ValuePairs[x-1].OutVal_InValMax;
                                double m = PhysDelta / RawDelta;
                                ret_Phys->Value.Double = CompuTab->ValuePairs[x-1].OutVal_InValMax + m * (RawValue - CompuTab->ValuePairs[x-1].InVal_InValMin);
                                ret_Phys->Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
                            }
                        } else {
                            ret_Phys->Value.Double = CompuTab->ValuePairs[x].OutVal_InValMax;
                            ret_Phys->Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
                        }
                    }
                    break;  // for(;;)
                }
            }
            if (x == CompuTab->NumberValuePairs) {
                ret_Phys->Value.Double = CompuTab->ValuePairs[x-1].OutVal_InValMax;
                ret_Phys->Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
            }
        case 1: // COMPU_VTAB
        case 2: // COMPU_VTAB_RANGE
        default:
            return -1;
            break;
        }
    }
    return -1;
}

static int ConvertWithFormulaToPhys (const char *Formula, A2L_SINGLE_VALUE *par_Raw, A2L_SINGLE_VALUE *ret_Phys)
{
    double PhysValue;
    char *ErrString;

    int State = direct_solve_equation_err_string_replace_value((char*)Formula, ConvertRawValueToDouble(par_Raw), &PhysValue, &ErrString);
    if (State) {
        FreeErrStringBuffer (&ErrString);
        return -1;
    } else {
        ret_Phys->Type = A2L_ELEM_TYPE_PHYS_DOUBLE;
        ret_Phys->Value.Double = PhysValue;
        // al other will be copied
        ret_Phys->Flags = ret_Phys->Flags;
        ret_Phys->TargetType = ret_Phys->TargetType;
        ret_Phys->Address = ret_Phys->Address;
        ret_Phys->SizeOfStruct = sizeof(A2L_SINGLE_VALUE);
        return 0;
    }
}

static int AddUnitToValue(A2L_SINGLE_VALUE * par_Value, const char *par_Unit)
{
    if (par_Unit != NULL) {
        int Len = (int)strlen(par_Unit) + 1;
        if (Len > 0) {
            if (par_Value->Type != A2L_ELEM_TYPE_TEXT_REPLACE) {
                if((par_Value->SizeOfStruct + Len) < 512) {
                    char *p = (char*)&par_Value->Value + sizeof(par_Value->Value);
                    StringCopyMaxCharTruncate(p, par_Unit, 512 - par_Value->SizeOfStruct);
                    par_Value->SizeOfStruct += Len;
                    par_Value->Flags |= A2L_VALUE_FLAG_HAS_UNIT;
                    return 0;
                }
            }
        }
    }
    return -1;
}

int ConvertRawToPhys(ASAP2_MODULE_DATA* Module, const char *par_ConvertName, int par_Flags, A2L_SINGLE_VALUE *par_Raw, A2L_SINGLE_VALUE *ret_Phys)
{
    int Ret = -1;
    ASAP2_COMPU_METHOD *CompuMethod, **__CompuMethod;
    const char **pName = &par_ConvertName;

    __CompuMethod = bsearch (&pName, Module->CompuMethods, Module->CompuMethodCounter, sizeof(ASAP2_COMPU_METHOD*), BsearchCompareFunc);
    if (__CompuMethod != NULL) {
        CompuMethod = *__CompuMethod;
        // "IDENTICAL 0 LINEAR 1 RAT_FUNC 2 FORM 3 TAB_INTP 4 TAB_NOINTP 5 TAB_VERB 6 ? -1
        switch (CompuMethod->ConversionType) {
        case 0:  // IDENTICAL
            *ret_Phys = *par_Raw;
            Ret = 0;
            break;
        case 4:  // TAB_INTP
        case 5:  // TAB_NOINTP
            if ((par_Flags & A2L_GET_PHYS_FLAG) == A2L_GET_PHYS_FLAG) {
                if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_COMPU_TAB_REF)) {
                    if (!ConvertRawToPhysByTable (Module, CompuMethod->OptionalParameter.ConversionTable,
                                                 (CompuMethod->ConversionType == 4),
                                                  par_Raw, ret_Phys)) {
                        Ret = 0;
                    }
                }
            } else {
                ValueCopy(ret_Phys, par_Raw);
                Ret = 0;
            }
            break;
        default:
            // will be ignored
            *ret_Phys = *par_Raw;
            break;
        case 6:  // TAB_VERB  -> ENUM (text replace)
            if ((par_Flags & A2L_GET_TEXT_REPLACE_FLAG) == A2L_GET_TEXT_REPLACE_FLAG) {
                if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_COMPU_TAB_REF)) {
                    if (!ConvertRawToTextReplace (Module, CompuMethod->OptionalParameter.ConversionTable, par_Raw, ret_Phys)) {
                        Ret = 0;
                    }
                }
            } else {
                ValueCopy(ret_Phys, par_Raw);
                Ret = 0;
            }
            break;
        case 2:  // RAT_FUNC
            if ((par_Flags & A2L_GET_PHYS_FLAG) == A2L_GET_PHYS_FLAG) {
                if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_COEFFS)) {
                    BB_VARIABLE_CONVERSION Conversion;
                    double Phys;
                    // INT = f(PHYS);
                    // f(x) = (axx + bx + c) / (dxx + ex + f)
                    Conversion.Type = BB_CONV_RAT_FUNC;
                    Conversion.Conv.RatFunc.a = CompuMethod->OptionalParameter.Coeffs.a;
                    Conversion.Conv.RatFunc.b = CompuMethod->OptionalParameter.Coeffs.b;
                    Conversion.Conv.RatFunc.c = CompuMethod->OptionalParameter.Coeffs.c;
                    Conversion.Conv.RatFunc.d = CompuMethod->OptionalParameter.Coeffs.d;
                    Conversion.Conv.RatFunc.e = CompuMethod->OptionalParameter.Coeffs.e;
                    Conversion.Conv.RatFunc.f = CompuMethod->OptionalParameter.Coeffs.f;
                    if (Conv_RationalFunctionRawToPhys(&Conversion, ConvertRawValueToDouble(par_Raw), &Phys) == 0) {
                        ConvertDoubleToPhysValue(par_Raw->TargetType, Phys, ret_Phys);
                        if ((par_Flags & A2L_GET_UNIT_FLAG) == A2L_GET_UNIT_FLAG) {
                            AddUnitToValue(ret_Phys, CompuMethod->Unit);
                        }
                    }
                    Ret = 0;
                }
            } else {
                ValueCopy(ret_Phys, par_Raw);
                Ret = 0;
            }
            break;
        case 1:  // LINEAR slope and offset
            if ((par_Flags & A2L_GET_PHYS_FLAG) == A2L_GET_PHYS_FLAG) {
                if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_COEFFS_LINEAR)) {
                    double Value = (ConvertRawValueToDouble(par_Raw) - CompuMethod->OptionalParameter.Coeffs.b) /
                                    CompuMethod->OptionalParameter.Coeffs.b;
                    ConvertDoubleToPhysValue(par_Raw->TargetType, Value, ret_Phys);
                    if ((par_Flags & A2L_GET_UNIT_FLAG) == A2L_GET_UNIT_FLAG) {
                        AddUnitToValue(ret_Phys, CompuMethod->Unit);
                    }
                    Ret = 0;
                }
            } else {
                ValueCopy(ret_Phys, par_Raw);
                Ret = 0;
            }
            break;
        case 3:  // FORM -> Formel zB: "x/1000" oder "X1*0.1234"
            if ((par_Flags & A2L_GET_PHYS_FLAG) == A2L_GET_PHYS_FLAG) {
                if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_FORMULA)) {
                    char Formula[1024];
                    char *Src, *Dst;
                    char LastChar;
                    Src = CompuMethod->OptionalParameter.Formula;
                    Dst = Formula;
                    LastChar = 0;
                    while (*Src != 0) {
                        // If ther is a x or X without a letter before or behind will be interpret as an place holder for a variable
                        if (((*Src == 'x') || (*Src == 'X')) && !(isalpha (LastChar) || isalpha(*(Src+1)))) {
                            *Dst = '#';
                            Src++;
                            if (*Src == '1') Src++;    // x1 bzw X1 will also be accept as place holder
                        } else *Dst++ = *Src++;
                    }
                    *Dst = 0;
                    Ret = ConvertWithFormulaToPhys(Formula, par_Raw, ret_Phys);
                    if ((Ret == 0) && ((par_Flags & A2L_GET_UNIT_FLAG) == A2L_GET_UNIT_FLAG)) {
                        AddUnitToValue(ret_Phys, CompuMethod->Unit);
                    }
                }
            } else {
                ValueCopy(ret_Phys, par_Raw);
                Ret = 0;
            }
            break;
        }
    } else if (!strcmp("NO_COMPU_METHOD", par_ConvertName)) {
        ValueCopy(ret_Phys, par_Raw);
        Ret = 0;
    }
    return Ret;
}

