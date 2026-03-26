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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "EnvironmentVariables.h"
#include "SharedDataTypes.h"
#include "Wildcards.h"
#include "A2LBuffer.h"
#include "A2LParser.h"
#include "A2LTokenizer.h"
#include "A2LLink.h"
#include "A2LAccess.h"

// Helper functions
static int BsearchCompareFunc (const void *arg1, const void *arg2)
{
    return strcmp (**(char***)arg1, **(char***)arg2);
}
// End helper functions


ASAP2_DATABASE *LoadAsapFile (const char *Filename, int Flags, char *ErrString, int MaxErrSize, int *ret_LineNr)
{
    ASAP2_DATABASE *Ret = NULL;
    ASAP2_PARSER* Parser;

    Ret = my_malloc (sizeof (ASAP2_DATABASE));
    if (Ret != NULL) {
        char *HelpFileName = my_malloc(1024);
        SearchAndReplaceEnvironmentStrings(Filename, HelpFileName, 1024);
        Parser = InitASAP2Parser (HelpFileName, Flags);
        my_free(HelpFileName);
        Ret->Asap2Ptr = (void*)Parser;
        if (Parser != NULL) {
            ParseRootBlock (Parser);
            if ((Parser->State == PARSER_STATE_EOF) ||      // end of file reached
                (Parser->State == PARSER_STATE_PARSE))  {   // all following chars will be ignored
                SortData (&Parser->Data);
                Ret->SelectedModul = 0;
            } else {
                GetParserErrorString (Parser, ErrString, MaxErrSize);
                if ((ret_LineNr != NULL) && (Parser->Cache != NULL)) *ret_LineNr = Parser->Cache->LineCounter;
                FreeAsap2Database(Ret);
                Ret = NULL;
            }
        } else {
            StringCopyMaxCharTruncate(ErrString, "cannot load file", MaxErrSize);
            my_free (Ret);
            Ret = NULL;
        }
    }
    return Ret;
}

int GetNextModuleName (ASAP2_DATABASE *Database, int Index,  char *Filter, char *RetName, int MaxSize)
{
    int x;
    ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
    for (x = Index; x < Parser->Data.ModuleCounter; x++) {
        if (Compare2StringsWithWildcardsAlwaysCaseSensitive(Parser->Data.Modules[x]->Ident, Filter) == 0) {   // Filter
            StringCopyMaxCharTruncate (RetName, Parser->Data.Modules[x]->Ident, MaxSize);
            return x;
        }
    }
    return -1;
}

int SelectModule (ASAP2_DATABASE *Database, char *Module)
{
    int x;
    ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
    for (x = 0; x < Parser->Data.ModuleCounter; x++) {
        if (!strcmp (Module, Parser->Data.Modules[x]->Ident)) {
            Database ->SelectedModul = x;
            return x;
        }
    }
    return -1;
}

int GetNextFunction (ASAP2_DATABASE *Database, int Index, const char *Filter, char *RetName, int MaxSize)
{
    int x;
    int Size;
    ASAP2_FUNCTION *Function;
    ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
    Size = Parser->Data.Modules[Database->SelectedModul]->FunctionCounter;
    for (x = Index+1; x < Size; x++) {
        Function = Parser->Data.Modules[Database->SelectedModul]->Functions[x];
        if (Compare2StringsWithWildcardsAlwaysCaseSensitive(Function->Name, Filter) == 0) {   // Filter
            StringCopyMaxCharTruncate (RetName, Function->Name, MaxSize);
            return x;
        }
    }
    return -1;
}

static ASAP2_MODULE_DATA * GetModule(ASAP2_DATABASE *Database)
{
    if (Database == NULL) return NULL;
    ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
    if (Parser == NULL) return NULL;
    if (Parser->Data.Modules == NULL) return NULL;
    return Parser->Data.Modules[Database->SelectedModul];
}

int GetFunctionIndex (ASAP2_DATABASE *Database, const char *Name)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_FUNCTION** Function;
        const char **pName = &Name;

        Function = bsearch (&pName, Module->Functions, Module->FunctionCounter, sizeof(ASAP2_FUNCTION*), BsearchCompareFunc);
        if (Function != NULL) {
            return (int)(Function - Module->Functions);   // Index backwards calculation
        }
    }
    return -1;
}

#define FLAG_DEF_CHARACTERISTIC    0x1
#define FLAG_REF_CHARACTERISTIC    0x2
#define FLAG_CHARACTERISTIC        (FLAG_DEF_CHARACTERISTIC | FLAG_REF_CHARACTERISTIC)
#define FLAG_IN_MEASUREMENT        0x4
#define FLAG_OUT_MEASUREMENT       0x8
#define FLAG_LOC_MEASUREMENT      0x10
#define FLAG_MEASUREMENT          (FLAG_IN_MEASUREMENT | FLAG_OUT_MEASUREMENT | FLAG_LOC_MEASUREMENT)
#define FLAG_SUB_FUNCTION         0x20

int GetNextFunctionMember (ASAP2_DATABASE *Database, int FuncIdx, int MemberIdx, const char *Filter, unsigned int Flags, char *RetName, int MaxSize, int *RetType)
{
    int x;
    ASAP2_FUNCTION *Function;
    int FuncSize, MemberSize;
    ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
    
    FuncSize = Parser->Data.Modules[Database->SelectedModul]->FunctionCounter;
    
    MemberIdx++;  // Start always with -1

    if (FuncIdx < FuncSize) {
        Function = Parser->Data.Modules[Database->SelectedModul]->Functions[FuncIdx];
		if      (MemberIdx < 0x10000000) goto __DEF_CHARACTERISTIC;
        else if (MemberIdx < 0x20000000) goto __REF_CHARACTERISTIC;
        else if (MemberIdx < 0x30000000) goto __IN_MEASUREMENT;
        else if (MemberIdx < 0x40000000) goto __OUT_MEASUREMENT;
        else if (MemberIdx < 0x50000000) goto __LOC_MEASUREMENT;
        else if (MemberIdx < 0x60000000) goto __SUB_FUNCTION;
        else goto __EXIT;

__DEF_CHARACTERISTIC:
        if (CheckIfFlagSet (Flags, FLAG_DEF_CHARACTERISTIC) &&
            CheckIfFlagSetPos (Function->OptionalParameter.Flags, OPTPARAM_FUNCTION_DEF_CHARACTERISTIC)) {
            MemberSize = Function->OptionalParameter.DefCharacteristicSize;
            for (x = MemberIdx & 0xFFFFFFF; x < MemberSize; x++) {
                if (Compare2StringsWithWildcardsAlwaysCaseSensitive(Function->OptionalParameter.DefCharacteristics[x], Filter) == 0) {   // Filter
                    StringCopyMaxCharTruncate (RetName, Function->OptionalParameter.DefCharacteristics[x], MaxSize);
                    *RetType = FLAG_DEF_CHARACTERISTIC;
                    return x;
                }
            }
			MemberIdx = 0x10000000;
        }
__REF_CHARACTERISTIC:
        if (CheckIfFlagSet (Flags, FLAG_REF_CHARACTERISTIC) &&
            CheckIfFlagSetPos (Function->OptionalParameter.Flags, OPTPARAM_FUNCTION_REF_CHARACTERISTIC)) {
            MemberSize = Function->OptionalParameter.RefCharacteristicSize;
            for (x = MemberIdx & 0xFFFFFFF; x < MemberSize; x++) {
                if (Compare2StringsWithWildcardsAlwaysCaseSensitive(Function->OptionalParameter.RefCharacteristics[x], Filter) == 0) {   // Filter
                    StringCopyMaxCharTruncate (RetName, Function->OptionalParameter.RefCharacteristics[x], MaxSize);
                    *RetType = FLAG_REF_CHARACTERISTIC;
                    return x + 0x10000000;
                }
            }
			MemberIdx = 0x20000000;
        }
__IN_MEASUREMENT:
        if (CheckIfFlagSet (Flags, FLAG_IN_MEASUREMENT) &&
            CheckIfFlagSetPos (Function->OptionalParameter.Flags, OPTPARAM_FUNCTION_IN_MEASUREMENT)) {
            MemberSize = Function->OptionalParameter.InMeasurementSize;
            for (x = MemberIdx & 0xFFFFFFF; x < MemberSize; x++) {
                if (Compare2StringsWithWildcardsAlwaysCaseSensitive(Function->OptionalParameter.InMeasurements[x], Filter) == 0) {   // Filter
                    StringCopyMaxCharTruncate (RetName, Function->OptionalParameter.InMeasurements[x], MaxSize);
                    *RetType = FLAG_IN_MEASUREMENT;
                    return x + 0x20000000;
                }
            }
			MemberIdx = 0x30000000;

        }
__OUT_MEASUREMENT:
        if (CheckIfFlagSet (Flags, FLAG_OUT_MEASUREMENT) &&
            CheckIfFlagSetPos (Function->OptionalParameter.Flags, OPTPARAM_FUNCTION_IN_MEASUREMENT)) {
            MemberSize = Function->OptionalParameter.OutMeasurementSize;
            for (x = MemberIdx & 0xFFFFFFF; x < MemberSize; x++) {
                if (Compare2StringsWithWildcardsAlwaysCaseSensitive(Function->OptionalParameter.OutMeasurements[x], Filter) == 0) {   // Filter
                    StringCopyMaxCharTruncate (RetName, Function->OptionalParameter.OutMeasurements[x], MaxSize);
                    *RetType = FLAG_OUT_MEASUREMENT;
                    return x + 0x30000000;
                }
            }
			MemberIdx = 0x40000000;
        }
__LOC_MEASUREMENT:
        if (CheckIfFlagSet (Flags, FLAG_LOC_MEASUREMENT) &&
            CheckIfFlagSetPos (Function->OptionalParameter.Flags, OPTPARAM_FUNCTION_LOC_MEASUREMENT)) {
            MemberSize = Function->OptionalParameter.LocMeasurementSize;
            for (x = MemberIdx & 0xFFFFFFF; x < MemberSize; x++) {
                if (Compare2StringsWithWildcardsAlwaysCaseSensitive(Function->OptionalParameter.LocMeasurements[x], Filter) == 0) {   // Filter
                    StringCopyMaxCharTruncate (RetName, Function->OptionalParameter.LocMeasurements[x], MaxSize);
                    *RetType = FLAG_LOC_MEASUREMENT;
                    return x + 0x40000000;
                }
            }
			MemberIdx = 0x50000000;
        }
__SUB_FUNCTION:
        if (CheckIfFlagSet (Flags, FLAG_SUB_FUNCTION) &&
            CheckIfFlagSetPos (Function->OptionalParameter.Flags, OPTPARAM_FUNCTION_SUB_FUNCTION)) {
            MemberSize = Function->OptionalParameter.SubFunctionSize;
            for (x = MemberIdx & 0xFFFFFFF; x < MemberSize; x++) {
                if (Compare2StringsWithWildcardsAlwaysCaseSensitive(Function->OptionalParameter.SubFunctions[x], Filter) == 0) {   // Filter
                    StringCopyMaxCharTruncate (RetName, Function->OptionalParameter.SubFunctions[x], MaxSize);
                    *RetType = FLAG_SUB_FUNCTION;
                    return x + 0x50000000;
                }
            }
        }
    }
__EXIT:
    return -1;
}

static int GetAndTranslateEnums (ASAP2_MODULE_DATA* Module, char *Name, char *Conv, int MaxSizeConv)
{
	char Help[1024];
	int x, s, Size = 0;
    ASAP2_COMPU_TAB *CompuTab, **__CompuTab;
    char **pName = &Name;

	__CompuTab = bsearch (&pName, Module->CompuTabs, Module->CompuTabCounter, sizeof(ASAP2_COMPU_TAB*), BsearchCompareFunc);
    if (__CompuTab != NULL) {
		CompuTab = *__CompuTab;
		switch (CompuTab->TabOrVtabFlag) {
        case 0: // COMPU_TAB will be ignored
            for (x = 0; x < CompuTab->NumberValuePairs; x++) {
                PrintFormatToString (Help, sizeof(Help), (x == 0) ? "%.18g/%.18g" : ":%.18g/%.18g",
                         CompuTab->ValuePairs[x].InVal_InValMin,
                         CompuTab->ValuePairs[x].OutVal_InValMax);
                Size += s = (int)strlen (Help);
                if (Size < (MaxSizeConv-1)) {
                    MEMCPY (Conv, Help, s+1);
                    Conv += s;
                } else {
                    break;   // doesn't fit inside the string
                }
            }
            return 0;
		case 1: // COMPU_VTAB
			for (x = 0; x < CompuTab->NumberValuePairs; x++) {
                PrintFormatToString (Help, sizeof(Help), "%i %i \"%s\";", (int)CompuTab->ValuePairs[x].InVal_InValMin,
                          (int)CompuTab->ValuePairs[x].InVal_InValMin,
                          CompuTab->ValuePairs[x].OutString);
				Size += s = (int)strlen (Help);
				if (Size < (MaxSizeConv-1)) {
                    MEMCPY (Conv, Help, s+1);
					Conv += s;
				} else {
                    break;   // doesn't fit inside the string
				}
			}
			return 0;
		case 2: // COMPU_VTAB_RANGE
			for (x = 0; x < CompuTab->NumberValuePairs; x++) {
                PrintFormatToString (Help, sizeof(Help), "%i %i \"%s\";", (int)CompuTab->ValuePairs[x].InVal_InValMin,
                           (int)CompuTab->ValuePairs[x].OutVal_InValMax,
                           CompuTab->ValuePairs[x].OutString);
				Size += s = (int)strlen (Help);
				if (Size < (MaxSizeConv-1)) {
                    MEMCPY (Conv, Help, s+1);
					Conv += s;
				} else {
                    break;   // doesn't fit inside the string
				}
			}
			return 0;
        default:
            return -1;
            break;
        }
	}
	return -1;
}

static void TranslateFormat (char *Format, int *pFormatLength, int *pFormatLayout)
{
    *pFormatLength = 4;   // Default values
    *pFormatLayout = 3;
	if (*Format == '%') {
		Format++;
		*pFormatLength = strtol (Format, &Format, 10);
		if (*Format == '.') {
			Format++;
			*pFormatLayout = strtol (Format, &Format, 10);
		}
	}
}

static int GetAndTranslateConversion (ASAP2_MODULE_DATA* Module, char *Name, 
							          int *pConvType, char *Conv, int MaxSizeConv, 
						              char *Unit, int MaxSizeUnit,
						              int *pFormatLength, int *pFormatLayout)
{
    ASAP2_COMPU_METHOD *CompuMethod, **__CompuMethod;
    char **pName = &Name;

    if (!strcmp("NO_COMPU_METHOD", Name)) {
        *pConvType = 0;
        *Conv = 0;
        *Unit = 0;
        *pFormatLength = 8;
        *pFormatLayout = 8;
        return 0;
    } else {
        __CompuMethod = bsearch (&pName, Module->CompuMethods, Module->CompuMethodCounter, sizeof(ASAP2_COMPU_METHOD*), BsearchCompareFunc);
        if (__CompuMethod != NULL) {
            CompuMethod = *__CompuMethod;
            // "IDENTICAL 0 LINEAR 1 RAT_FUNC 2 FORM 3 TAB_INTP 4 TAB_NOINTP 5 TAB_VERB 6 ? -1
            switch (CompuMethod->ConversionType) {
            case 0:  // IDENTICAL
            default:
                // This compute methods will be ignored
                *pConvType = 0;
                *Conv = 0;
                *Unit = 0;
                *pFormatLength = 8;
                *pFormatLayout = 8;
                break;
            case 4:  // TAB_INTP
            case 5:  // TAB_NOINTP
                if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_COMPU_TAB_REF)) {
                    if (!GetAndTranslateEnums (Module, CompuMethod->OptionalParameter.ConversionTable, Conv, MaxSizeConv)) {
                        if (CompuMethod->ConversionType == 4) *pConvType = BB_CONV_TAB_INTP;
                        else *pConvType = BB_CONV_TAB_NOINTP;
                    }
                }
                break;
            case 6:  // TAB_VERB  -> ENUM (text replace)
                if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_COMPU_TAB_REF)) {
                    if (!GetAndTranslateEnums (Module, CompuMethod->OptionalParameter.ConversionTable, Conv, MaxSizeConv)) {
                        *pConvType = BB_CONV_TEXTREP;
                    }
                }
                break;
            case 2:  // RAT_FUNC
                if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_COEFFS)) {
                    // INT = f(PHYS);
                    // f(x) = (axx + bx + c) / (dxx + ex + f)
                    if ((CompuMethod->OptionalParameter.Coeffs.a == 0.0) &&  // a == 0
                        (CompuMethod->OptionalParameter.Coeffs.b != 0.0) &&  // b != 0
                        (CompuMethod->OptionalParameter.Coeffs.d == 0.0) &&  // d == 0
                        (CompuMethod->OptionalParameter.Coeffs.e == 0.0)) {  // e == 0
                        // it is a linear conversion
                        PrintFormatToString (Conv, MaxSizeConv, "%.18g:%.18g",
                                CompuMethod->OptionalParameter.Coeffs.f / CompuMethod->OptionalParameter.Coeffs.b,
                                CompuMethod->OptionalParameter.Coeffs.c / CompuMethod->OptionalParameter.Coeffs.b); // f/b, c/b
                        *pConvType = BB_CONV_FACTOFF;
                    } else {
                        PrintFormatToString (Conv, MaxSizeConv, "%.18g:%.18g:%.18g:%.18g:%.18g:%.18g",
                                  CompuMethod->OptionalParameter.Coeffs.a,
                                  CompuMethod->OptionalParameter.Coeffs.b,
                                  CompuMethod->OptionalParameter.Coeffs.c,
                                  CompuMethod->OptionalParameter.Coeffs.d,
                                  CompuMethod->OptionalParameter.Coeffs.e,
                                  CompuMethod->OptionalParameter.Coeffs.f);
                        *pConvType = BB_CONV_RAT_FUNC;
                    }
                } else {
                    *pConvType = 0;
                }
                break;
            case 1:  // LINEAR slope and offset
                if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_COEFFS_LINEAR)) {
                    PrintFormatToString (Conv, MaxSizeConv, "%.18g:%.18g",
                             CompuMethod->OptionalParameter.Coeffs.a,
                             CompuMethod->OptionalParameter.Coeffs.b);
                    *pConvType = BB_CONV_FACTOFF;
                }
                break;
            case 3:  // FORM -> Formula example "x/1000" oder "X1*0.1234"
                if (CheckIfFlagSetPos(CompuMethod->OptionalParameter.Flags, OPTPARAM_COMPU_METHOD_FORMULA)) {
                    char *Src, *Dst;
                    char LastChar;
                    Src = CompuMethod->OptionalParameter.Formula;
                    Dst = Conv;
                    LastChar = 0;
                    while (*Src != 0) {
                        // A 'x' or 'X' character occur without a letter are before or behind it
                        // That will be interpreted as place holder
                        if (((*Src == 'x') || (*Src == 'X')) && !(isalpha (LastChar) || isalpha(*(Src+1)))) {
                            *Dst = '#';
                            Src++;
                            Dst++;
                            if (*Src == '1') Src++;   // x1 bzw X1 will be also accepted
                        } else *Dst++ = *Src++;
                    }
                    *Dst = 0;
                    *pConvType = BB_CONV_FORMULA;
                }
                break;
            }
            StringCopyMaxCharTruncate (Unit, CompuMethod->Unit, MaxSizeUnit);
            TranslateFormat (CompuMethod->Format, pFormatLength, pFormatLayout);
            return 0;
        } else {
            *pConvType = 0;
            *Conv = 0;
            *Unit = 0;
            *pFormatLength = 8;
            *pFormatLayout = 8;
            return -1;
        }
    }
}

int GetNextMeasurement (ASAP2_DATABASE *Database, int Index, const char *Filter, unsigned int Flags, char *RetName, int MaxSize)
{
    int x;
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        for (x = Index + 1; x < Module->MeasurementCounter; x++) {
            if (Compare2StringsWithWildcardsAlwaysCaseSensitive(Module->Measurements[x]->Ident, Filter) == 0) {   // Filter
                if (((Flags & (A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT | A2L_LABEL_TYPE_REFERENCED_MEASUREMENT)) == 0) ||
                    (((Flags & A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT) == A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT) && (Module->Measurements[x]->ReferenceCounter == 0)) ||
                    (((Flags & A2L_LABEL_TYPE_REFERENCED_MEASUREMENT) == A2L_LABEL_TYPE_REFERENCED_MEASUREMENT) && (Module->Measurements[x]->ReferenceCounter > 0))) {
                    StringCopyMaxCharTruncate (RetName, Module->Measurements[x]->Ident, MaxSize);
                    return x;
                }
            }
        }
    }
    return -1;
}

int GetMeasurementIndex (ASAP2_DATABASE *Database, const char *Name)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_MEASUREMENT** Measurement;
        const char **pName = &Name;
        Measurement = bsearch (&pName, Module->Measurements, Module->MeasurementCounter, sizeof(ASAP2_MEASUREMENT*), BsearchCompareFunc);
        if (Measurement != NULL) {
            return (int)(Measurement - Module->Measurements);   // Index backwards calculation
        }
    }
    return -1;
}

int GetMeasurementInfos (ASAP2_DATABASE *Database, int Index, 
						 char *Name, int MaxSizeName, 
						 int *pType, uint64_t *pAddress, 
						 int *pConvType, char *Conv, int MaxSizeConv, 
						 char *Unit, int MaxSizeUnit,
						 int *pFormatLength, int *pFormatLayout,
						 int *pXDim, int *pYDim, int *pZDim,  
                         double *pMin, double *pMax, int *pIsWritable)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_MEASUREMENT* Measurement;

        if (Index >= Module->MeasurementCounter) return -1;
        Measurement = Module->Measurements[Index];
        // optional address, should be exist
        if (!CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_ADDRESS)) return -1;
        StringCopyMaxCharTruncate (Name, Measurement->Ident, MaxSizeName);
        *pType = Measurement->DataType;
        *pAddress = Measurement->OptionalParameter.Address;
        *pMin = Measurement->LowerLimit;
        *pMax = Measurement->UpperLimit;
        GetAndTranslateConversion (Module, Measurement->ConversionIdent,
                                   pConvType, Conv, MaxSizeConv,
                                   Unit, MaxSizeUnit,
                                   pFormatLength, pFormatLayout);
        // optional format (then overwrite this from conversion)
        if (CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_FORMAT)) {
            TranslateFormat (Measurement->OptionalParameter.Format, pFormatLength, pFormatLayout);
        }
        *pXDim = *pYDim = *pZDim = 0;
        // optionale Array-Dimension
        if (CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_ARRAY_SIZE)) {
            *pXDim = Measurement->OptionalParameter.ArraySize;
        }
        if (CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_MATRIX_DIM)) {
            *pXDim = Measurement->OptionalParameter.MatrixDim.x;
            *pYDim = Measurement->OptionalParameter.MatrixDim.y;
            *pZDim = Measurement->OptionalParameter.MatrixDim.z;
        }
        if (CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_WRITABLE_FLAG)) {
            *pIsWritable = 1;
        } else {
            *pIsWritable = 0;
        }
    }
    return 0;
}

//----------------------------------------------------------------
// CHARACTERISTIC
//----------------------------------------------------------------

int GetNextCharacteristic (ASAP2_DATABASE *Database, int Index, const char *Filter, unsigned int Flags, char *RetName, int MaxSize)
{
    int x;
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        for (x = Index + 1; x < Module->CharacteristicCounter; x++) {
            if (((0x100 << (Module->Characteristics[x]->Type - 1)) & Flags) != 0) {
                if (Compare2StringsWithWildcardsAlwaysCaseSensitive(Module->Characteristics[x]->Name, Filter) == 0) {   // Filter
                    if (((Flags & (A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT | A2L_LABEL_TYPE_REFERENCED_MEASUREMENT)) == 0) ||
                        (((Flags & A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT) == A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT) && (Module->Characteristics[x]->ReferenceCounter == 0)) ||
                        (((Flags & A2L_LABEL_TYPE_REFERENCED_MEASUREMENT) == A2L_LABEL_TYPE_REFERENCED_MEASUREMENT) && (Module->Characteristics[x]->ReferenceCounter > 0))) {
                        StringCopyMaxCharTruncate (RetName, Module->Characteristics[x]->Name, MaxSize);
                        return x;
                    }
                }
            }
        }
    }
    return -1;
}

int GetNextCharacteristicAxis (ASAP2_DATABASE *Database, int Index, const char *Filter, unsigned int Flags, char *RetName, int MaxSize)
{
    int x;
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        for (x = Index + 1; x < Module->AxisPtsCounter; x++) {
            if (Compare2StringsWithWildcardsAlwaysCaseSensitive(Module->AxisPtss[x]->Name, Filter) == 0) {   // Filter
                if (((Flags & (A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT | A2L_LABEL_TYPE_REFERENCED_MEASUREMENT)) == 0) ||
                    (((Flags & A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT) == A2L_LABEL_TYPE_NOT_REFERENCED_MEASUREMENT) && (Module->AxisPtss[x]->ReferenceCounter == 0)) ||
                    (((Flags & A2L_LABEL_TYPE_REFERENCED_MEASUREMENT) == A2L_LABEL_TYPE_REFERENCED_MEASUREMENT) && (Module->AxisPtss[x]->ReferenceCounter > 0))) {
                    StringCopyMaxCharTruncate (RetName, Module->AxisPtss[x]->Name, MaxSize);
                    return x;
                }
            }
        }
    }
    return -1;
}

int GetCharacteristicType (ASAP2_DATABASE *Database, int Index)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        if (Index < Module->CharacteristicCounter) {
            return (1 << Module->Characteristics[Index]->Type);
        }
    }
    return -1;
}

int GetCharacteristicIndex (ASAP2_DATABASE *Database, const char *Name)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_CHARACTERISTIC** Characteristic;
        const char **pName = &Name;

        Characteristic = bsearch (&pName, Module->Characteristics, Module->CharacteristicCounter, sizeof(ASAP2_CHARACTERISTIC*), BsearchCompareFunc);
        if (Characteristic != NULL) {
            return (int)(Characteristic - Module->Characteristics);   // reverse calculate index
        }
    }
    return -1;
}

int GetCharacteristicAxisIndex (ASAP2_DATABASE *Database, const char *Name)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_AXIS_PTS** Axis;
        const char **pName = &Name;

        Axis = bsearch (&pName, Module->AxisPtss, Module->AxisPtsCounter, sizeof(ASAP2_AXIS_PTS*), BsearchCompareFunc);
        if (Axis != NULL) {
            return (int)(Axis - Module->AxisPtss);   // reverse calculate index
        }
    }
    return -1;
}

int GetRecordLayoutIndex (ASAP2_DATABASE* Database, char *Name)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_RECORD_LAYOUT** RecordLayout;
        char **pName = &Name;

        RecordLayout = bsearch (&pName, Module->RecordLayouts, Module->RecordLayoutCounter, sizeof(ASAP2_RECORD_LAYOUT*), BsearchCompareFunc);
        if (RecordLayout != NULL) {
            return (int)(RecordLayout - Module->RecordLayouts);   // reverse calculate index
        }
    }
    return -1;
}

static int GetRecordLayoutFncValues (ASAP2_MODULE_DATA* Module, int Index)
{
    int x;
    if (Index >= Module->RecordLayoutCounter) return -1;
    if (!CheckIfFlagSetPos(Module->RecordLayouts[Index]->OptionalParameter.Flags, OPTPARAM_RECORDLAYOUT_FNC_VALUES)) return -1;
    for (x = 0; x < Module->RecordLayouts[Index]->OptionalParameter.ItemCount; x++) {
        if (Module->RecordLayouts[Index]->OptionalParameter.Items[x]->ItemType == OPTPARAM_RECORDLAYOUT_FNC_VALUES) {
            return Module->RecordLayouts[Index]->OptionalParameter.Items[x]->Item.FncValues.Datatype;
        }
    }
    return -1;
}

int GetValueCharacteristicInfos (ASAP2_DATABASE *Database, int Index,
                                char *Name, int MaxSizeName,
                                int *pType, uint64_t *pAddress,
                                int *pConvType, char *Conv, int MaxSizeConv,
                                char *Unit, int MaxSizeUnit,
                                int *pFormatLength, int *pFormatLayout,
                                int *pXDim, int *pYDim, int *pZDim,
                                double *pMin, double *pMax, int *pIsWritable)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_CHARACTERISTIC* Characteristic;
        int RecLayIdx;

        if (Index >= Module->CharacteristicCounter) return -1;
        Characteristic = Module->Characteristics[Index];

        if (Characteristic->Type != 1/*A2L_ITEM_TYPE_VALUE*/) return -1;    // It is not a single parameter
        StringCopyMaxCharTruncate (Name, Characteristic->Name, MaxSizeName);
        // Read data type from record layout
        RecLayIdx = GetRecordLayoutIndex (Database, Characteristic->Deposit);
        if (RecLayIdx < 0) return -1;
        *pType = GetRecordLayoutFncValues (Module, RecLayIdx);
        if (*pType < 0) return -1;

        GetAndTranslateConversion (Module, Characteristic->Conversion,
                                   pConvType, Conv, MaxSizeConv,
                                   Unit, MaxSizeUnit,
                                   pFormatLength, pFormatLayout);
        // optional format (Then overwrite the conversion)
        if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_FORMAT)) {
            TranslateFormat (Characteristic->OptionalParameter.Format, pFormatLength, pFormatLayout);
        }
        if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_MATRIX_DIM)) {
            *pXDim = Characteristic->OptionalParameter.MatrixDim.x;
            *pYDim = Characteristic->OptionalParameter.MatrixDim.y;
            *pZDim = Characteristic->OptionalParameter.MatrixDim.z;
        } else {
            *pXDim = 0;
            *pYDim = 0;
            *pZDim = 0;
        }
        if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_READ_ONLY)) {
            *pIsWritable = 0;
        } else {
            *pIsWritable = 1;
        }
        *pMin = Characteristic->LowerLimit;
        *pMax = Characteristic->UpperLimit;

        *pAddress = Characteristic->Address;
        return 0;
    }
    return -1;
}


int GetAxisIndex(ASAP2_DATABASE *Database, char *Name)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_AXIS_PTS** Axis;
        char **pName = &Name;

        Axis = bsearch(&pName, Module->AxisPtss, Module->AxisPtsCounter, sizeof(ASAP2_AXIS_PTS*), BsearchCompareFunc);
        if (Axis != NULL) {
            return (int)(Axis - Module->AxisPtss);   // reverse calculate index
        }
    }
    return -1;
}


void FreeAsap2Database(ASAP2_DATABASE *Database)
{
    if (Database != NULL) {
        ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
        if (Parser != NULL) {
            for (int x = 0; x < Parser->Data.ModuleCounter; x++) {
                ASAP2_MODULE_DATA* Module = Parser->Data.Modules[x];
                if (Module != NULL) {
                    if (Module->RecordLayouts != NULL) {
                        for (int r = 0; r < Module->RecordLayoutCounter; r++) {
                            if (Module->RecordLayouts[r]->OptionalParameter.Items != NULL) {
                                my_free(Module->RecordLayouts[r]->OptionalParameter.Items);
                            }
                        }
                        my_free(Module->RecordLayouts);
                    }
                    if (Module->AxisDescrs != NULL) my_free(Module->AxisDescrs);
                    if (Module->Characteristics != NULL) my_free(Module->Characteristics);
                    if (Module->Measurements != NULL) my_free(Module->Measurements);
                    if (Module->AxisPtss != NULL) my_free(Module->AxisPtss);
                    if (Module->CompuMethods != NULL) my_free(Module->CompuMethods);
                    if (Module->CompuTabs != NULL) my_free(Module->CompuTabs);
                    if (Module->Functions != NULL) my_free(Module->Functions);
                }
            }
            if (Parser->Data.Modules != NULL) my_free(Parser->Data.Modules);

            if (Parser->Data.StringBuffers != NULL) {
                for (int x = 0; x < Parser->Data.StringBufferCounter; x++) {
                    if (Parser->Data.StringBuffers[x].Data != NULL) my_free(Parser->Data.StringBuffers[x].Data);
                }
                my_free(Parser->Data.StringBuffers);
            }
            if (Parser->LastString != NULL) my_free(Parser->LastString);
            if (Parser->Cache != NULL) DeleteCache(Parser->Cache);
            my_free(Parser);
        }
        my_free(Database);
    }
}

int SearchMinMaxCalibrationAddress(ASAP2_DATABASE *Database, uint64_t *ret_MinAddress, uint64_t *ret_MaxAddress)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_CHARACTERISTIC* Characteristic;
        uint64_t MinAddress = 0xFFFFFFFFFFFFFFFFULL;
        uint64_t MaxAddress = 0ULL;
        if (Module->CharacteristicCounter == 0) return -1;
        for (int Index = 0; Index < Module->CharacteristicCounter; Index++) {
            Characteristic = Module->Characteristics[Index];
            if (Characteristic->Address > MaxAddress) {
                MaxAddress = Characteristic->Address;
            }
            if (Characteristic->Address < MinAddress) {
                MinAddress = Characteristic->Address;
            }
        }
        *ret_MinAddress = MinAddress;
        *ret_MaxAddress = MaxAddress;
        return 0;
    }
    return -1;
}

int GetCharacteristicInfoType(ASAP2_DATABASE *Database, int Index)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_CHARACTERISTIC* Characteristic;

        if (Index >= Module->CharacteristicCounter) return -1;
        Characteristic = Module->Characteristics[Index];
        return Characteristic->Type;
    }
    return -1;
}

int GetCharacteristicAddress(ASAP2_DATABASE *Database, int Index, uint64_t *ret_Address)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_CHARACTERISTIC* Characteristic;

        if (Index >= Module->CharacteristicCounter) return -1;
        Characteristic = Module->Characteristics[Index];
        *ret_Address = Characteristic->Address;
        return 0;
    }
    return -1;
}

int GetMeasurementAddress(ASAP2_DATABASE *Database, int Index, uint64_t *ret_Address)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_MEASUREMENT* Measurement;

        if (Index >= Module->MeasurementCounter) return -1;
        Measurement = Module->Measurements[Index];
        if (!CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_ADDRESS)) return -1;
        *ret_Address = Measurement->OptionalParameter.Address;
        return 0;
    }
    return -1;
}


int GetCharacteristicAxisInfos(ASAP2_DATABASE *Database, int Index, int AxisNo,
                               double *ret_Min, double *ret_Max,
                               char *ret_Input, int InputMaxLen,
                               int *ret_ConvType, char *ret_Conversion, int ConversionMaxLen,
                               char *ret_Unit, int UnitMaxLen,
                               int *ret_FormatLength, int *ret_FormatLayout)
{
    ASAP2_MODULE_DATA* Module = GetModule(Database);
    if (Module != NULL) {
        ASAP2_CHARACTERISTIC* Characteristic;

        *ret_Min = 0.0;
        *ret_Max = 1.0;
        *ret_Input = 0;
        *ret_ConvType = 0;
        *ret_Conversion = 0;
        *ret_Unit = 0;
        *ret_FormatLength = 8;
        *ret_FormatLayout = 8;

        if (Index >= Module->CharacteristicCounter) return -1;
        Characteristic = Module->Characteristics[Index];

        switch (AxisNo) {
        case 0:
            *ret_Min = Characteristic->LowerLimit;
            *ret_Max = Characteristic->UpperLimit;
            if (GetAndTranslateConversion (Module, Characteristic->Conversion,
                                           ret_ConvType, ret_Conversion, ConversionMaxLen,
                                           ret_Unit, UnitMaxLen,
                                           ret_FormatLength, ret_FormatLayout) == 0) {
                return 0;
            }
            break;
        case 1: // x
            if (Characteristic->OptionalParameter.AxisDescrCount >= 1) {
                *ret_Min = Characteristic->OptionalParameter.AxisDescr[0]->LowerLimit;
                *ret_Max = Characteristic->OptionalParameter.AxisDescr[0]->UpperLimit;
                if (Characteristic->OptionalParameter.AxisDescrCount >= 1) {
                    if (!strcmp(Characteristic->OptionalParameter.AxisDescr[0]->InputQuantity, "NO_INPUT_QUANTITY")) {
                        *ret_Input = 0;
                    } else {
                        StringCopyMaxCharTruncate(ret_Input, Characteristic->OptionalParameter.AxisDescr[0]->InputQuantity, InputMaxLen);
                    }
                }
                if (GetAndTranslateConversion (Module, Characteristic->OptionalParameter.AxisDescr[0]->Conversion,
                                               ret_ConvType, ret_Conversion, ConversionMaxLen,
                                               ret_Unit, UnitMaxLen,
                                               ret_FormatLength, ret_FormatLayout) == 0) {
                    return 0;
                }
            }
            break;
        case 2: // y
            if (Characteristic->OptionalParameter.AxisDescrCount >= 2) {
                *ret_Min = Characteristic->OptionalParameter.AxisDescr[1]->LowerLimit;
                *ret_Max = Characteristic->OptionalParameter.AxisDescr[1]->UpperLimit;
                if (Characteristic->OptionalParameter.AxisDescrCount >= 1) {
                    if (!strcmp(Characteristic->OptionalParameter.AxisDescr[1]->InputQuantity, "NO_INPUT_QUANTITY")) {
                        *ret_Input = 0;
                    } else {
                        StringCopyMaxCharTruncate(ret_Input, Characteristic->OptionalParameter.AxisDescr[1]->InputQuantity, InputMaxLen);
                    }
                }
                if (GetAndTranslateConversion (Module, Characteristic->OptionalParameter.AxisDescr[1]->Conversion,
                                               ret_ConvType, ret_Conversion, ConversionMaxLen,
                                               ret_Unit, UnitMaxLen,
                                               ret_FormatLength, ret_FormatLayout) == 0) {
                    return 0;
                }
            }
            break;
        default:
            break;
        }
    }
    return -1;
}
