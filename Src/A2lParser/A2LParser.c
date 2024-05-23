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
#include <stdarg.h>
#include <string.h>
//#include <malloc.h>
#include "MyMemory.h"
#include "A2LBuffer.h"
#include "A2LTokenizer.h"
#include "A2LParser.h"

//#define DEBUG_PRINTF

#define UNUSED(x) (void)(x)

#define CHECK_SET_FLAG_BIT_POS(f,b)  if (((f) & (1ULL << (b))) == (1ULL << (b))) {ThrowParserError (Parser, __FILE__, __LINE__, "\"%s\" already defined", KeywordTableEntry->KeywordString);} else (f) |= (1ULL << (b));

// Enum-Strings
// BB_BYTE==0, BB_UBYTE==1, BB_WORD==2, BB_UWORD==3, BB_DWORD==4,
// BB_UDWORD==5, BB_FLOAT==6, BB_DOUBLE==7, BB_UNKNOWN==8,
#define ENUMDEF_DATATYPE "UBYTE 1 SBYTE 0 UWORD 3 SWORD 2 ULONG 5 SLONG 4 FLOAT32_IEEE 6 FLOAT64_IEEE 7 A_INT64 34 A_UINT64 35 FLOAT16_IEEE 36 ? -1"
#define ENUMDEF_DATASIZE "BYTE 1 WORD 2 LONG 4 ? -1"
#define ENUMDEF_ADDRTYPE "PBYTE 1PWORD 2 PLONG 3 DIRECT 4 ? -1"
#define ENUMDEF_BYTEORDER "MSB_FIRST 1 LITTLE_ENDIAN 1 MSB_LAST 0 BIG_ENDIAN 0 ? -1"
#define ENUMDEF_INDEXORDER "INDEX_INCR 1 INDEX_DECR 2 ? -1"
#define ENUMDEF_INDEXMODE "COLUMN_DIR 1 ROW_DIR 2 ALTERNATE_CURVES 3 ALTERNATE_WITH_X 4 ALTERNATE_WITH_Y 5 ? -1"
#define ENUMDEF_CHARACTERISTIC_TYPE "VALUE 1 ASCII 2 VAL_BLK 3 CURVE 4 MAP 5 CUBOID 6 CUBE_4 7 CUBE_5 8 ? -1"
#define ENUMDEF_CONVERSIONTYPE "IDENTICAL 0 LINEAR 1 RAT_FUNC 2 FORM 3 TAB_INTP 4 TAB_NOINTP 5 TAB_VERB 6 ? -1"
#define ENUMDEF_AXIS_ATTRIBUTE "STD_AXIS 1 FIX_AXIS 2 COM_AXIS 3 RES_AXIS 4 CURVE_AXIS 5 ? -1"
#define ENUMDEF_CALIBRATION_TYPE "CALIBRATION 1 NO_CALIBRATION 2 NOT_IN_MCD_SYSTEM 3 OFFLINE_CALIBRATION 4 ? -1"
#define ENUMDEF_MONOTONY "MON_INCREASE 1 MON_DECREASE 2 STRICT_INCREASE 3 STRICT_DECREASE 4 ? -1"
#define ENUMDEF_DEPOSIT_MODE "ABSOLUTE 1 DIFFERENCE 2 ? -1"
#define ENUMDEF_UNIT "EXTENDED_SI 1 DERIVED 2 ? -1"
#define ENUMDEF_LAYOUT "COLUMN_DIR 1 ROW_DIR 2 ? -1"


// This will trigger an error message and terminate the parser
// The ErrorString can be include the same format specifier as printf
int ThrowParserError (struct ASAP2_PARSER_STRUCT *Parser, char *SourceFile, int LineNumber, char *ErrorString, ...)
{
    va_list args;
    int Size;

    Parser->SourceFile = SourceFile;
    Parser->LineNumber = LineNumber;

    sprintf (Parser->ErrorString, "%s [%i]: ", (Parser->Filename == NULL) ? "" : Parser->Filename, (Parser->Cache == NULL) ? -1 : Parser->Cache->LineCounter);

    Size = (int)strlen (Parser->ErrorString);
    va_start (args, ErrorString);
    vsnprintf(Parser->ErrorString + Size, sizeof(Parser->ErrorString) - Size, ErrorString, args);
    va_end (args);
    Parser->ErrorString[sizeof(Parser->ErrorString)-1] = 0;  // ist das noetig?

    printf("%s\n", Parser->ErrorString);

    Parser->State = PARSER_STATE_ERROR;
    return CheckParserError (Parser);
}

// Will give back an error string (if there is an error)
int GetParserErrorString (struct ASAP2_PARSER_STRUCT *Parser, char *ErrString, int MaxSize)
{
    if (ErrString != NULL) {
        if (Parser->State == PARSER_STATE_ERROR) {
            strncpy (ErrString, Parser->ErrorString, MaxSize);
        } else {
            strncpy (ErrString, "no error", MaxSize);
        }
        ErrString[MaxSize-1] = 0;
    }
    return 0;
}

// overlook a block which will begin with /begin XXXX and ends with /end XXXX
int IgnoreBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    UNUSED(HasBeginKeyWord);
    int x;
    char *Help;

    if (KeywordTableEntry->BeginEndToken) {
        x = 1;
        do {
            Help = ParseNextString (Parser, 0);
            if (!strcmp ("/begin", Help)) {
                x++;
            } else if (!strcmp ("/end", Help)) {
                x--;
            }
        } while (x);
        Help = ParseNextString (Parser, 0);
        if (!strcmp (Help, KeywordTableEntry->KeywordString)) {
            return CheckParserError (Parser);
        } else {
            ThrowParserError (Parser, __FILE__, __LINE__, "the name of the /end isn't the same  as /begin");
            return -1;
        }
    } else {
        for (x = 0; x < KeywordTableEntry->ParameterCount; x++) {
            Help = ParseNextString (Parser, 0);
            if (!strcmp ("/begin", Help) || !strcmp ("/end", Help)) {
                ThrowParserError (Parser, __FILE__, __LINE__, "wrong parameter count");
                return -1;
            }
        }
        return CheckParserError (Parser);
    }
}

// Reads an optional parameter on the basis of the KeywordTable
// and will call the  ruft die associated method whith the parameter Data
int GetNextOptionalParameter (struct ASAP2_PARSER_STRUCT *Parser, ASAP2_KEYWORDS* KeywordTable, void* Data)
{
    int x;
    char *Help;
    int BeginFlag = 0;
    //int EndFlag = 0;
    int Offset;
    int LineNr;

    Offset = CacheTell(Parser->Cache, &LineNr);
    Help = ParseNextString (Parser, 0);
    if (CheckParserError (Parser)) return -1;
    if (!strcmp ("/begin", Help)) {
        BeginFlag = 1;
        Help = ParseNextString (Parser, 0);
        if (CheckParserError (Parser)) return -1;
    }

    for (x = 0; KeywordTable[x].KeywordString != NULL; x++) {
        if (!strcmp (KeywordTable[x].KeywordString, Help)) {
            if ((KeywordTable[x].BeginEndToken != -1) &&  // there can be a \begin
                (KeywordTable[x].BeginEndToken != BeginFlag)) {  // there must be a \begin
                ThrowParserError (Parser, __FILE__, __LINE__, "illegal use of /begin block");
            }
            if (KeywordTable[x].ParseBlockFunc == NULL) {
                if (IgnoreBlock (Parser, Data, &KeywordTable[x], BeginFlag)) return -1;
            } else {
                if (KeywordTable[x].ParseBlockFunc(Parser, Data, &KeywordTable[x], BeginFlag)) return -1;
            }
            return 1;
        }
    }
    // it is not a optional parameter therefor jump back
    CacheSeek(Parser->Cache, Offset, SEEK_SET, LineNr);
    return CheckParserError (Parser);
}

int CheckEndKeyWord(struct ASAP2_PARSER_STRUCT *Parser, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    char *Help;
    if (HasBeginKeyWord) {
        Help = ReadKeyWordFromFile(Parser);
        if (Help == NULL) return -1;
        if (strcmp("/end", Help)) {
            ThrowParserError(Parser, __FILE__, __LINE__, "expecting a \"/end\" not a \"%s\"", Help);
            return -1;
        }
        Help = ReadKeyWordFromFile(Parser);
        if (Help == NULL) return -1;
        if (strcmp(KeywordTableEntry->KeywordString, Help)) {
            ThrowParserError(Parser, __FILE__, __LINE__, "expecting a \"%s\" not a \"%s\"", KeywordTableEntry->KeywordString, Help);
            return -1;
        }
    }
    return CheckParserError(Parser);
}

//-------------------------------------------------------------------
// MOD_PAR
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// MOD_COMMON
//-------------------------------------------------------------------

int ParseModCommonSRecLayout (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MOD_COMMON *ModCommon = (ASAP2_MOD_COMMON*)Data;
    CHECK_SET_FLAG_BIT_POS (ModCommon->OptionalParameter.Flags, 
                        OPTPARAM_MOD_COMMON_S_REC_LAYOUT);
    ModCommon->OptionalParameter.StandardRecordLayout = ReadIdentFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseModCommonDeposit (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MOD_COMMON* ModCommon = (ASAP2_MOD_COMMON*)Data;
    CHECK_SET_FLAG_BIT_POS (ModCommon->OptionalParameter.Flags, 
                        OPTPARAM_MOD_COMMON_DEPOSIT);
    ModCommon->OptionalParameter.Deposit = ReadEnumFromFile (Parser, ENUMDEF_DEPOSIT_MODE);
    return CheckParserError (Parser);
}

int ParseModCommonByteOrder (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MOD_COMMON* ModCommon = (ASAP2_MOD_COMMON*)Data;
    CHECK_SET_FLAG_BIT_POS (ModCommon->OptionalParameter.Flags, 
                        OPTPARAM_MOD_COMMON_BYTE_ORDER);
    ModCommon->OptionalParameter.ByteOrder = ReadEnumFromFile (Parser, ENUMDEF_BYTEORDER);
    return CheckParserError (Parser);
}

int ParseModCommonDataSize (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MOD_COMMON* ModCommon = (ASAP2_MOD_COMMON*)Data;
    CHECK_SET_FLAG_BIT_POS (ModCommon->OptionalParameter.Flags, 
                        OPTPARAM_MOD_COMMON_DATA_SIZE);
    ModCommon->OptionalParameter.DataSize = ReadEnumFromFile (Parser, ENUMDEF_DATASIZE);
    return CheckParserError (Parser);
}

int ParseModCommonAlignmentByte (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MOD_COMMON* ModCommon = (ASAP2_MOD_COMMON*)Data;
    CHECK_SET_FLAG_BIT_POS (ModCommon->OptionalParameter.Flags, 
                        OPTPARAM_MOD_COMMON_ALIGNMENT_BYTE);
    ModCommon->OptionalParameter.AlignmentByte = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseModCommonAlignmentWord (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MOD_COMMON* ModCommon = (ASAP2_MOD_COMMON*)Data;
    CHECK_SET_FLAG_BIT_POS (ModCommon->OptionalParameter.Flags, 
                        OPTPARAM_MOD_COMMON_ALIGNMENT_WORD);
    ModCommon->OptionalParameter.AlignmentWord = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseModCommonAlignmentLong (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MOD_COMMON* ModCommon = (ASAP2_MOD_COMMON*)Data;
    CHECK_SET_FLAG_BIT_POS (ModCommon->OptionalParameter.Flags, 
                        OPTPARAM_MOD_COMMON_ALIGNMENT_LONG);
    ModCommon->OptionalParameter.AlignmentLong = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseModCommonAlignmentFloat16 (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MOD_COMMON* RecordLayout = (ASAP2_MOD_COMMON*)Data;
    CHECK_SET_FLAG_BIT_POS (RecordLayout->OptionalParameter.Flags, 
                        OPTPARAM_MOD_COMMON_ALIGNMENT_FLOAT16_IEEE);
    RecordLayout->OptionalParameter.AlignmentFloat16 = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseModCommonAlignmentFloat32(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MOD_COMMON* RecordLayout = (ASAP2_MOD_COMMON*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                       OPTPARAM_MOD_COMMON_ALIGNMENT_FLOAT32_IEEE);
    RecordLayout->OptionalParameter.AlignmentFloat32 = ReadIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseModCommonAlignmentFloat64(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MOD_COMMON* RecordLayout = (ASAP2_MOD_COMMON*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
        OPTPARAM_MOD_COMMON_ALIGNMENT_FLOAT64_IEEE);
    RecordLayout->OptionalParameter.AlignmentFloat64 = ReadIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseModCommonAlignmentInt64(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MOD_COMMON* RecordLayout = (ASAP2_MOD_COMMON*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
        OPTPARAM_MOD_COMMON_ALIGNMENT_INT64);
    RecordLayout->OptionalParameter.AlignmentInt64 = ReadIntFromFile(Parser);
    return CheckParserError(Parser);
}


#define TOKEN_MOD_COMMON_S_REC_LAYOUT             11000001
#define TOKEN_MOD_COMMON_DEPOSIT                  11000002
#define TOKEN_MOD_COMMON_BYTE_ORDER               11000003
#define TOKEN_MOD_COMMON_DATA_SIZE                11000004
#define TOKEN_MOD_COMMON_ALIGNMENT_BYTE           11000005
#define TOKEN_MOD_COMMON_ALIGNMENT_WORD           11000006
#define TOKEN_MOD_COMMON_ALIGNMENT_LONG           11000007
#define TOKEN_MOD_COMMON_ALIGNMENT_INT64          11000008
#define TOKEN_MOD_COMMON_ALIGNMENT_FLOAT16_IEEE   11000009
#define TOKEN_MOD_COMMON_ALIGNMENT_FLOAT32_IEEE   11000010
#define TOKEN_MOD_COMMON_ALIGNMENT_FLOAT64_IEEE   11000011

static ASAP2_KEYWORDS ModCommonKeywordTable[] =
    {{0, "S_REC_LAYOUT",            0, TOKEN_MOD_COMMON_S_REC_LAYOUT,           ParseModCommonSRecLayout, ""},
     {0, "DEPOSIT",                 1, TOKEN_MOD_COMMON_DEPOSIT,                ParseModCommonDeposit, ""},
     {0, "BYTE_ORDER",              1, TOKEN_MOD_COMMON_BYTE_ORDER,             ParseModCommonByteOrder, ""},
     {0, "DATA_SIZE",               1, TOKEN_MOD_COMMON_DATA_SIZE,              ParseModCommonDataSize, ""},
     {0, "ALIGNMENT_BYTE",          1, TOKEN_MOD_COMMON_ALIGNMENT_BYTE,         ParseModCommonAlignmentByte, ""},
     {0, "ALIGNMENT_WORD",          1, TOKEN_MOD_COMMON_ALIGNMENT_WORD,         ParseModCommonAlignmentWord, ""},
     {0, "ALIGNMENT_LONG",          1, TOKEN_MOD_COMMON_ALIGNMENT_LONG,         ParseModCommonAlignmentLong, ""},
     {0, "ALIGNMENT_INT64",         1, TOKEN_MOD_COMMON_ALIGNMENT_INT64,        ParseModCommonAlignmentInt64, ""},
     {0, "ALIGNMENT_FLOAT16_IEEE",  1, TOKEN_MOD_COMMON_ALIGNMENT_FLOAT16_IEEE, ParseModCommonAlignmentFloat16, ""},
     {0, "ALIGNMENT_FLOAT32_IEEE",  1, TOKEN_MOD_COMMON_ALIGNMENT_FLOAT32_IEEE, ParseModCommonAlignmentFloat32, ""},
     {0, "ALIGNMENT_FLOAT64_IEEE",  1, TOKEN_MOD_COMMON_ALIGNMENT_FLOAT64_IEEE, ParseModCommonAlignmentFloat64, ""},
     {0, NULL,                      0, -1,                                 NULL, ""}};

int ParseModCommonBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    UNUSED(HasBeginKeyWord);
    ASAP2_MOD_COMMON *ModCommon;

    ModCommon = &(Parser->Data.Modules[Parser->Data.ModuleCounter - 1]->ModCommon);
    // 1. Parse the fixed paramters
    ModCommon->Comment = ReadStringFromFile_Buffered (Parser);
    if (ModCommon->Comment == NULL) return -1;

    // 2. Parse the optionale parameters
    while (GetNextOptionalParameter (Parser, ModCommonKeywordTable, (void*)ModCommon) == 1);
    if (CheckParserError(Parser)) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}

//-------------------------------------------------------------------
// IF_DATA
//-------------------------------------------------------------------

int IgnoreIfDataBlock(struct ASAP2_PARSER_STRUCT *Parser)
{
    char *Help;
    int x = 1;
    do {
        Help = ReadKeyWordFromFile(Parser);
        if (Help == NULL) return -1;
        if (!strcmp("/begin", Help)) {
            x++;
        }
        else if (!strcmp("/end", Help)) {
            x--;
        }
    } while (x);
    Help = ReadKeyWordFromFile(Parser);
    if (Help == NULL) return -1;
    if (!strcmp(Help, "IF_DATA")) {
        return CheckParserError(Parser);
    }
    else {
        ThrowParserError(Parser, __FILE__, __LINE__, "the name of the /end isn't the same  as /begin");
        return -1;
    }
}

int ParseIfDataCanapeExtLinkMap(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_CANAPE_EXT* CanapeExt = (ASAP2_IF_DATA_CANAPE_EXT*)Data;
    CHECK_SET_FLAG_BIT_POS(CanapeExt->OptionalParameter.Flags,
                       OPTPARAM_IF_DATA_CANAPE_EXT_LINK_MAP);
    CanapeExt->OptionalParameter.Label = ReadIdentFromFile_Buffered(Parser);
    if CheckParserError(Parser) return -1;
    CanapeExt->OptionalParameter.Address = ReadUInt64FromFile(Parser);
    CanapeExt->OptionalParameter.FileOffsetAddress = Parser->CurrentOffsetStartWord;
    if CheckParserError(Parser) return -1;
    CanapeExt->OptionalParameter.AddressExtension = ReadIntFromFile(Parser);
    if CheckParserError(Parser) return -1;
    CanapeExt->OptionalParameter.FlagRelToDS = ReadIntFromFile(Parser);
    if CheckParserError(Parser) return -1;
    CanapeExt->OptionalParameter.AddressOffset = ReadUInt64FromFile(Parser);
    if CheckParserError(Parser) return -1;
    CanapeExt->OptionalParameter.FlagDataTypeValid = ReadIntFromFile(Parser);
    if CheckParserError(Parser) return -1;
    CanapeExt->OptionalParameter.CANapeDataType = ReadIntFromFile(Parser);
    if CheckParserError(Parser) return -1;
    CanapeExt->OptionalParameter.BitOffset = ReadIntFromFile(Parser);
    if CheckParserError(Parser) return -1;
    return CheckParserError(Parser);
}

int ParseIfDataCanapeExtDisplay(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_IF_DATA_CANAPE_EXT* CanapeExt = (ASAP2_IF_DATA_CANAPE_EXT*)Data;
    CHECK_SET_FLAG_BIT_POS(CanapeExt->OptionalParameter.Flags,
        OPTPARAM_IF_DATA_CANAPE_EXT_DISPLAY);
    CanapeExt->OptionalParameter.DisplayColor = ReadUIntFromFile(Parser);
    if CheckParserError(Parser) return -1;
    CanapeExt->OptionalParameter.DisplayPhysMin = ReadFloatFromFile(Parser);
    if CheckParserError(Parser) return -1;
    CanapeExt->OptionalParameter.DisplayPhysMax = ReadFloatFromFile(Parser);
    if CheckParserError(Parser) return -1;
    return CheckParserError(Parser);
}

int ParseIfDataCanapeExtVirtualConversion(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    UNUSED(KeywordTableEntry);
    UNUSED(HasBeginKeyWord);
    char *Help = ReadKeyWordFromFile(Parser);
    if (Help == NULL) return -1;
    return CheckParserError(Parser);
}


#define TOKEN_IF_DATA_CANAPE_EXT_LINK_MAP                10000001
#define TOKEN_IF_DATA_CANAPE_EXT_DISPLAY                 10000002
#define TOKEN_IF_DATA_CANAPE_EXT_VIRTUAL_CONVERSION      10000003
static ASAP2_KEYWORDS IfDataCanapeExtKeywordTable[] =
{ {0, "LINK_MAP",            8, TOKEN_IF_DATA_CANAPE_EXT_LINK_MAP,             ParseIfDataCanapeExtLinkMap, ""},
  {0, "DISPLAY",             3, TOKEN_IF_DATA_CANAPE_EXT_DISPLAY,              ParseIfDataCanapeExtDisplay, ""},
  {1, "VIRTUAL_CONVERSION",  1, TOKEN_IF_DATA_CANAPE_EXT_VIRTUAL_CONVERSION,   ParseIfDataCanapeExtVirtualConversion, ""},
  {0, NULL,                  0, -1,                                            NULL, ""} };


int ParseIfDataBlock(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    struct {
        IDENT Idnet;
        int StructType;
    } *Ptr = Data;
    char *Help = ReadKeyWordFromFile(Parser);
    if (Help == NULL) return -1;
    if (!strcmp(Help, "CANAPE_EXT")) {
        ASAP2_IF_DATA_CANAPE_EXT *CanapeExt;
        switch (Ptr->StructType) {
        case BUFFER_STRUCT_TYPE_CHARACTERISTIC:
        {
            ASAP2_CHARACTERISTIC *Characteristic = (ASAP2_CHARACTERISTIC*)Data;
            if (Characteristic->OptionalParameter.IfDataCanapeExtCount >= 2) {
                ThrowParserError(Parser, __FILE__, __LINE__, "only 2 IF_DATA CANAPE_EXT allowed");
                return -1;
            }
            CanapeExt = AddEmptyCanapeExtToBuffer(Parser);
            if (CanapeExt == NULL) return -1;
            Characteristic->OptionalParameter.IfDataCanapeExt[Characteristic->OptionalParameter.IfDataCanapeExtCount++] = CanapeExt;
            break;
        }
        case BUFFER_STRUCT_TYPE_MEASUREMENT:
        {
            ASAP2_MEASUREMENT *Measurement = (ASAP2_MEASUREMENT*)Data;
            if (Measurement->OptionalParameter.IfDataCanapeExtCount >= 2) {
                ThrowParserError(Parser, __FILE__, __LINE__, "only 2 IF_DATA CANAPE_EXT allowed");
                return -1;
            }
            CanapeExt = AddEmptyCanapeExtToBuffer(Parser);
            if (CanapeExt == NULL) return -1;
            Measurement->OptionalParameter.IfDataCanapeExt[Measurement->OptionalParameter.IfDataCanapeExtCount++] = CanapeExt;
            break;
        }
        case BUFFER_STRUCT_TYPE_AXIS_PTS:
        {
            ASAP2_AXIS_PTS *AxisPts = (ASAP2_AXIS_PTS*)Data;
            if (AxisPts->OptionalParameter.IfDataCanapeExtCount >= 2) {
                ThrowParserError(Parser, __FILE__, __LINE__, "only 2 IF_DATA CANAPE_EXT allowed");
                return -1;
            }
            CanapeExt = AddEmptyCanapeExtToBuffer(Parser);
            if (CanapeExt == NULL) return -1;
            AxisPts->OptionalParameter.IfDataCanapeExt[AxisPts->OptionalParameter.IfDataCanapeExtCount++] = CanapeExt;
            break;
        }
        default:
            return IgnoreIfDataBlock(Parser);
        }
        CanapeExt->Version = ReadIntFromFile(Parser);
        if CheckParserError(Parser) return -1;

        // 2. Parse the optionale Parameters
        while (GetNextOptionalParameter(Parser, IfDataCanapeExtKeywordTable, (void*)CanapeExt) == 1);
        if CheckParserError(Parser) return -1;

        if (CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord)) return -1;
    } else {
        return IgnoreIfDataBlock(Parser);
    }
    return  CheckParserError(Parser);
}


//-------------------------------------------------------------------
// AXIS_DESCR
//-------------------------------------------------------------------

int ParseAxisDescrReadOnly (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisDescr->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_AXIS_DESCR_READ_ONLY);
    AxisDescr->OptionalParameter.ReadOnly = 1;
    return CheckParserError (Parser);
}

int ParseAxisDescrFormat (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisDescr->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_AXIS_DESCR_FORMAT);
    AxisDescr->OptionalParameter.Format = ReadStringFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseAxisDescrAxisPtsRef (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisDescr->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_AXIS_DESCR_AXIS_PTS_REF);
    AxisDescr->OptionalParameter.AxisPoints = ReadIdentFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseAxisDescrMaxGrad (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisDescr->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_AXIS_DESCR_MAX_GRAD);
    AxisDescr->OptionalParameter.MaxGradient = ReadFloatFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseAxisDescrMonotony (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisDescr->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_AXIS_DESCR_MONOTONY);
    AxisDescr->OptionalParameter.Monotony = ReadEnumFromFile (Parser, ENUMDEF_MONOTONY);
    return CheckParserError (Parser);
}

int ParseAxisDescrByteOrder (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisDescr->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_AXIS_DESCR_BYTE_ORDER);
    AxisDescr->OptionalParameter.ByteOrder = ReadEnumFromFile (Parser, ENUMDEF_BYTEORDER);
    return CheckParserError (Parser);
}

int ParseAxisDescrExtendedLimits (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisDescr->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_AXIS_DESCR_EXTENDED_LIMITS);
    AxisDescr->OptionalParameter.ExtendedLimits.LowerLimit = ReadFloatFromFile (Parser);
    AxisDescr->OptionalParameter.ExtendedLimits.UpperLimit = ReadFloatFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseAxisDescrFixAxisPar (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisDescr->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_AXIS_DESCR_FIX_AXIS_PAR);
    AxisDescr->OptionalParameter.FixAxisPar.Offset = ReadIntFromFile (Parser);
    AxisDescr->OptionalParameter.FixAxisPar.Shift = ReadIntFromFile (Parser);
    AxisDescr->OptionalParameter.FixAxisPar.Numberapo = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseAxisDescrFixAxisParDist (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisDescr->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_AXIS_DESCR_FIX_AXIS_PAR_DIST);
    AxisDescr->OptionalParameter.FixAxisParDist.Offset = ReadIntFromFile (Parser);
    AxisDescr->OptionalParameter.FixAxisParDist.Distance = ReadIntFromFile (Parser);
    AxisDescr->OptionalParameter.FixAxisParDist.Numberapo = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseAxisDescrFixAxisParList (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    int x;
    char *Help;

    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisDescr->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_AXIS_DESCR_FIX_AXIS_PAR_LIST);
    AxisDescr->OptionalParameter.FixAxisParList = (double*)AllocMemFromBuffer (&Parser->Data, AxisDescr->MaxAxisPoints * sizeof (double));
    if (AxisDescr->OptionalParameter.FixAxisParList == NULL) {
        ThrowParserError (Parser, __FILE__, __LINE__, "out of memory");
        return -1;
    }
    for (x = 0; x < AxisDescr->MaxAxisPoints; x++) {
        AxisDescr->OptionalParameter.FixAxisParList[x] = ReadFloatFromFile (Parser);
        if (CheckParserError (Parser)) return -1;
    }
    Help = ReadKeyWordFromFile (Parser);
    if (Help == NULL) return -1;
    if (strcmp (Help, "/end")) {
        ThrowParserError (Parser, __FILE__, __LINE__, "less than expected fixed values in axis description");
        return -1;
    }
    Help = ReadKeyWordFromFile (Parser);
    if (Help == NULL) return -1;
    if (strcmp (Help, KeywordTableEntry->KeywordString)) {
        ThrowParserError (Parser, __FILE__, __LINE__, "expecting a \"FIX_AXIS_PAR_LIST\" not a \"%s\"", Help);
        return -1;
    }
    return CheckParserError (Parser);
}

int ParseAxisDescrDeposit (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisDescr->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_AXIS_DESCR_DEPOSIT);
    AxisDescr->OptionalParameter.DepositMode = ReadEnumFromFile (Parser, ENUMDEF_DEPOSIT_MODE);
    return CheckParserError (Parser);
}

int ParseAxisDescrCurveAxisRef (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisDescr->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_AXIS_DESCR_CURVE_AXIS_REF);
    AxisDescr->OptionalParameter.PhysUnit = ReadIdentFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseAxisDescrPhysUnit(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)Data;
    CHECK_SET_FLAG_BIT_POS(AxisDescr->OptionalParameter.Flags,
                       OPTPARAM_CHARACTERISTIC_AXIS_DESCR_PHYS_UNIT);
    AxisDescr->OptionalParameter.CurveAxis = ReadIdentFromFile_Buffered(Parser);
    return CheckParserError(Parser);
}

#define TOKEN_AXIS_DESCR_READ_ONLY                10000001
#define TOKEN_AXIS_DESCR_FORMAT                   10000002
#define TOKEN_AXIS_DESCR_ANNOTATION               10000003
#define TOKEN_AXIS_DESCR_AXIS_PTS_REF             10000004
#define TOKEN_AXIS_DESCR_MAX_GRAD                 10000005
#define TOKEN_AXIS_DESCR_MONOTONY                 10000006
#define TOKEN_AXIS_DESCR_BYTE_ORDER               10000007
#define TOKEN_AXIS_DESCR_EXTENDED_LIMITS          10000008
#define TOKEN_AXIS_DESCR_FIX_AXIS_PAR             10000009
#define TOKEN_AXIS_DESCR_FIX_AXIS_PAR_DIST        10000010
#define TOKEN_AXIS_DESCR_FIX_AXIS_PAR_LIST        10000011 
#define TOKEN_AXIS_DESCR_DEPOSIT                  10000012
#define TOKEN_AXIS_DESCR_CURVE_AXIS_REF           10000013
#define TOKEN_AXIS_DESCR_PHYS_UNIT                10000014
static ASAP2_KEYWORDS AxisDescrKeywordTable[] =
    {{0, "READ_ONLY",          0, TOKEN_AXIS_DESCR_READ_ONLY,         ParseAxisDescrReadOnly, ""},
     {0, "FORMAT",             1, TOKEN_AXIS_DESCR_FORMAT,            ParseAxisDescrFormat, ""},
     {1, "ANNOTATION",        -1, TOKEN_AXIS_DESCR_ANNOTATION,        IgnoreBlock, ""},                  // this will ignore
     {0, "AXIS_PTS_REF",       1, TOKEN_AXIS_DESCR_AXIS_PTS_REF,      ParseAxisDescrAxisPtsRef, ""},
     {0, "MAX_GRAD",           1, TOKEN_AXIS_DESCR_MAX_GRAD,          ParseAxisDescrMaxGrad, ""},
     {0, "MONOTONY",           1, TOKEN_AXIS_DESCR_MONOTONY,          ParseAxisDescrMonotony, ""},
     {0, "BYTE_ORDER",         1, TOKEN_AXIS_DESCR_BYTE_ORDER,        ParseAxisDescrByteOrder, ""},
     {0, "EXTENDED_LIMITS",    2, TOKEN_AXIS_DESCR_EXTENDED_LIMITS,   ParseAxisDescrExtendedLimits, ""},
     {0, "FIX_AXIS_PAR",       3, TOKEN_AXIS_DESCR_FIX_AXIS_PAR,      ParseAxisDescrFixAxisPar, ""},
     {0, "FIX_AXIS_PAR_DIST",  3, TOKEN_AXIS_DESCR_FIX_AXIS_PAR_DIST, ParseAxisDescrFixAxisParDist, ""},
     {1, "FIX_AXIS_PAR_LIST", -1, TOKEN_AXIS_DESCR_FIX_AXIS_PAR_LIST, ParseAxisDescrFixAxisParList, ""},
     {0, "DEPOSIT",            1, TOKEN_AXIS_DESCR_DEPOSIT,           ParseAxisDescrDeposit, ""},
     {0, "CURVE_AXIS_REF",     1, TOKEN_AXIS_DESCR_CURVE_AXIS_REF,    ParseAxisDescrCurveAxisRef, ""},
     {0, "PHYS_UNIT",          1, TOKEN_AXIS_DESCR_PHYS_UNIT,         ParseAxisDescrPhysUnit, ""},   // if this is defined ignore the unit inside the COMPU_METHOD
     {0, NULL,                 0, -1,                                 NULL, ""}};

int ParseAxisDescrBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    ASAP2_CHARACTERISTIC_AXIS_DESCR* AxisDescr;
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;

    if (Characteristic->OptionalParameter.AxisDescrCount >= 2) {
        ThrowParserError (Parser, __FILE__, __LINE__, "not more than 2 \"AXIS_DESCR\" allowed");
        return -1;
    }
    AxisDescr = AddEmptyAxisDescrToBuffer (Parser);
    if (AxisDescr == NULL) return -1;
    // 1. Parse the fixed parameters
    AxisDescr->Attribute = ReadEnumFromFile (Parser, ENUMDEF_AXIS_ATTRIBUTE);
    AxisDescr->InputQuantity = ReadIdentFromFile_Buffered (Parser);  //        "NO_INPUT_QUANTITY"
    AxisDescr->Conversion = ReadIdentFromFile_Buffered (Parser);     //        "NO_COMPU_METHOD"
    AxisDescr->MaxAxisPoints = ReadIntFromFile (Parser);
    AxisDescr->LowerLimit = ReadFloatFromFile (Parser);
    AxisDescr->UpperLimit = ReadFloatFromFile (Parser);
    if CheckParserError(Parser) return -1;

    // 2. Parse the optionale parameters
    while (GetNextOptionalParameter (Parser, AxisDescrKeywordTable, (void*)AxisDescr) == 1);
    if CheckParserError(Parser) return -1;

    if (CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord)) return -1;

    Characteristic->OptionalParameter.AxisDescr[Characteristic->OptionalParameter.AxisDescrCount] = AxisDescr;
    Characteristic->OptionalParameter.AxisDescrCount++;
    return CheckParserError (Parser);
}


//-------------------------------------------------------------------
// CHARACTERISTIC
//-------------------------------------------------------------------

int ParseCharacteristicDisplayIdentifier (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_DISPLAY_IDENTIFIER);
    Characteristic->OptionalParameter.DisplayIdentifier = ReadIdentFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseCharacteristicFormat (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_FORMAT);
    Characteristic->OptionalParameter.Format = ReadStringFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseCharacteristicByteOrder (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_BYTE_ORDER);
    Characteristic->OptionalParameter.ByteOrder = ReadEnumFromFile (Parser, ENUMDEF_BYTEORDER);
    return CheckParserError (Parser);
}

int ParseCharacteristicBitMask (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_BIT_MASK);
    Characteristic->OptionalParameter.BitMask = ReadUInt64FromFile (Parser);
    return CheckParserError (Parser);
}

int ParseCharacteristicNumber (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);

    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_NUMBER);
    Characteristic->OptionalParameter.Number = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseCharacteristicExtendedLimits (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_EXTENDED_LIMITS);
    Characteristic->OptionalParameter.ExtendedLimits.LowerLimit = ReadFloatFromFile (Parser);
    Characteristic->OptionalParameter.ExtendedLimits.UpperLimit = ReadFloatFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseCharacteristicReadOnly (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_READ_ONLY);
    Characteristic->OptionalParameter.ReadOnly = 1;
    return CheckParserError (Parser);
}

int ParseCharacteristicGuardRails (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_GUARD_RAILS);
    Characteristic->OptionalParameter.GuardRails = 1;
    return CheckParserError (Parser);
}

int ParseCharacteristicMaxRefresh (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_MAX_REFRESH);
    Characteristic->OptionalParameter.MaxRefresh.Rate = ReadIntFromFile (Parser);;
    Characteristic->OptionalParameter.MaxRefresh.ScalingUnit = ReadIntFromFile (Parser);;
    return CheckParserError (Parser);
}

int ParseCharacteristicRefMemorySegment (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_REF_MEMORY_SEGMENT);
    Characteristic->OptionalParameter.RefMemorySegment = ReadIdentFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseCharacteristicComparisonQuantity (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_COMPARISON_QUANTITY);
    Characteristic->OptionalParameter.ComparisonQuantity = ReadIdentFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseCharacteristicCalibrationAccess (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_CALIBRATION_ACCESS);
    Characteristic->OptionalParameter.CalibrationAccess = ReadEnumFromFile (Parser, ENUMDEF_CALIBRATION_TYPE);
    return CheckParserError (Parser);
}

int ParseCharacteristicMatrixDim (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_MATRIX_DIM);
    Characteristic->OptionalParameter.MatrixDim.x = ReadIntFromFile (Parser);
    Characteristic->OptionalParameter.MatrixDim.y = TryReadIntFromFile (Parser, 0);  // sometime this is missing
    Characteristic->OptionalParameter.MatrixDim.z = TryReadIntFromFile (Parser, 0);  // sometime this is missing
    return CheckParserError (Parser);
}

int ParseCharacteristicEcuAddressExtension (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS (Characteristic->OptionalParameter.Flags, 
                        OPTPARAM_CHARACTERISTIC_ECU_ADDRESS_EXTENSION);
    Characteristic->OptionalParameter.EcuAddressExtension = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseCharacteristicSymbolLink(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS(Characteristic->OptionalParameter.Flags,
                       OPTPARAM_CHARACTERISTIC_SYMBOL_LINK);
    Characteristic->OptionalParameter.SymbolLink = ReadIdentFromFile_Buffered(Parser);
    Characteristic->OptionalParameter.SymbolLinkX = ReadIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseCharacteristicPhysUnit(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS(Characteristic->OptionalParameter.Flags,
        OPTPARAM_CHARACTERISTIC_PHYS_UNIT);
    Characteristic->OptionalParameter.PhysUnit = ReadIdentFromFile_Buffered(Parser);
    return CheckParserError(Parser);
}

int ParseCharacteristicDiscrete(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS(Characteristic->OptionalParameter.Flags,
                       OPTPARAM_CHARACTERISTIC_DISCRETE);
    Characteristic->OptionalParameter.Discrete = 1;
    return CheckParserError(Parser);
}

int ParseCharacteristicStepSize(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_CHARACTERISTIC* Characteristic = (ASAP2_CHARACTERISTIC*)Data;
    CHECK_SET_FLAG_BIT_POS(Characteristic->OptionalParameter.Flags,
        OPTPARAM_CHARACTERISTIC_STEP_SIZE);
    Characteristic->OptionalParameter.StepSize = ReadFloatFromFile(Parser);
    return CheckParserError(Parser);
}

#define TOKEN_CHARACTERISTIC_DISPLAY_IDENTIFIER       9000001
#define TOKEN_CHARACTERISTIC_FORMAT                   9000002
#define TOKEN_CHARACTERISTIC_BYTE_ORDER               9000003
#define TOKEN_CHARACTERISTIC_BIT_MASK                 9000004
#define TOKEN_CHARACTERISTIC_FUNCTION_LIST            9000005
#define TOKEN_CHARACTERISTIC_NUMBER                   9000006
#define TOKEN_CHARACTERISTIC_EXTENDED_LIMITS          9000007 
#define TOKEN_CHARACTERISTIC_READ_ONLY                9000008
#define TOKEN_CHARACTERISTIC_GUARD_RAILS              9000009
#define TOKEN_CHARACTERISTIC_MAP_LIST                 9000010
#define TOKEN_CHARACTERISTIC_MAX_REFRESH              9000011
#define TOKEN_CHARACTERISTIC_DEPENDENT_CHARACTERISTIC 9000012
#define TOKEN_CHARACTERISTIC_VIRTUAL_CHARACTERISTIC   9000013
#define TOKEN_CHARACTERISTIC_REF_MEMORY_SEGMENT       9000014 
#define TOKEN_CHARACTERISTIC_ANNOTATION               9000015
#define TOKEN_CHARACTERISTIC_COMPARISON_QUANTITY      9000016 
#define TOKEN_CHARACTERISTIC_IF_DATA                  9000017
#define TOKEN_CHARACTERISTIC_AXIS_DESCR               9000018
#define TOKEN_CHARACTERISTIC_CALIBRATION_ACCESS       9000019
#define TOKEN_CHARACTERISTIC_MATRIX_DIM               9000020 
#define TOKEN_CHARACTERISTIC_ECU_ADDRESS_EXTENSION    9000021
#define TOKEN_CHARACTERISTIC_SYMBOL_LINK              9000022
#define TOKEN_CHARACTERISTIC_PHYS_UNIT                9000023
#define TOKEN_CHARACTERISTIC_DISCRETE                 9000024
#define TOKEN_CHARACTERISTIC_STEP_SIZE                9000025

static ASAP2_KEYWORDS CharacteristicKeywordTable[] =
    {{0, "DISPLAY_IDENTIFIER",        1, TOKEN_CHARACTERISTIC_DISPLAY_IDENTIFIER,       ParseCharacteristicDisplayIdentifier, ""},
     {0, "FORMAT",                    1, TOKEN_CHARACTERISTIC_FORMAT,                   ParseCharacteristicFormat, ""},
     {0, "BYTE_ORDER",                1, TOKEN_CHARACTERISTIC_BYTE_ORDER,               ParseCharacteristicByteOrder, ""},
     {0, "BIT_MASK",                  1, TOKEN_CHARACTERISTIC_BIT_MASK,                 ParseCharacteristicBitMask, ""},
     {1, "FUNCTION_LIST",            -1, TOKEN_CHARACTERISTIC_FUNCTION_LIST,            IgnoreBlock, ""}, // wird ignoriert
     {0, "NUMBER",                    1, TOKEN_CHARACTERISTIC_NUMBER,                   ParseCharacteristicNumber, ""},
     {0, "EXTENDED_LIMITS",           2, TOKEN_CHARACTERISTIC_EXTENDED_LIMITS,          ParseCharacteristicExtendedLimits, ""},
     {0, "READ_ONLY",                 0, TOKEN_CHARACTERISTIC_READ_ONLY,                ParseCharacteristicReadOnly, ""},
     {0, "GUARD_RAILS",               0, TOKEN_CHARACTERISTIC_GUARD_RAILS,              ParseCharacteristicGuardRails, ""},
     {1, "MAP_LIST",                 -1, TOKEN_CHARACTERISTIC_MAP_LIST,                 IgnoreBlock, ""},
     {0, "MAX_REFRESH",               2, TOKEN_CHARACTERISTIC_MAX_REFRESH,              ParseCharacteristicMaxRefresh, ""},
     {1, "DEPENDENT_CHARACTERISTIC", -1, TOKEN_CHARACTERISTIC_DEPENDENT_CHARACTERISTIC, IgnoreBlock, ""},  // wird ignoriert
     {1, "VIRTUAL_CHARACTERISTIC",   -1, TOKEN_CHARACTERISTIC_VIRTUAL_CHARACTERISTIC,   IgnoreBlock, ""},  // wird ignoriert
     {0, "REF_MEMORY_SEGMENT",        1, TOKEN_CHARACTERISTIC_REF_MEMORY_SEGMENT,       ParseCharacteristicRefMemorySegment, ""},
     {1, "ANNOTATION",               -1, TOKEN_CHARACTERISTIC_ANNOTATION,               IgnoreBlock, ""},                  // will be ignored
     {0, "COMPARISON_QUANTITY",       1, TOKEN_CHARACTERISTIC_COMPARISON_QUANTITY,      ParseCharacteristicComparisonQuantity, ""},
     {1, "IF_DATA",                  -1, TOKEN_CHARACTERISTIC_IF_DATA,                  ParseIfDataBlock, ""},
     {1, "AXIS_DESCR",               -1, TOKEN_CHARACTERISTIC_AXIS_DESCR,               ParseAxisDescrBlock, ""},
     {0, "CALIBRATION_ACCESS",        1, TOKEN_CHARACTERISTIC_CALIBRATION_ACCESS,       ParseCharacteristicCalibrationAccess, ""},
     {0, "MATRIX_DIM",                3, TOKEN_CHARACTERISTIC_MATRIX_DIM,               ParseCharacteristicMatrixDim, ""},
     {0, "ECU_ADDRESS_EXTENSION",     1, TOKEN_CHARACTERISTIC_ECU_ADDRESS_EXTENSION,    ParseCharacteristicEcuAddressExtension, ""},
     {0, "SYMBOL_LINK",               2, TOKEN_CHARACTERISTIC_SYMBOL_LINK,              ParseCharacteristicSymbolLink, ""},
     {0, "PHYS_UNIT",                 1, TOKEN_CHARACTERISTIC_PHYS_UNIT,                ParseCharacteristicPhysUnit, ""},   // if this is defined ignore the unit inside the COMPU_METHOD
     {0, "DISCRETE",                  0, TOKEN_CHARACTERISTIC_DISCRETE,                 ParseCharacteristicDiscrete, ""},
     {0, "STEP_SIZE",                 1, TOKEN_CHARACTERISTIC_STEP_SIZE,                ParseCharacteristicStepSize, ""},
     {0, NULL,                        0, -1,                                            NULL, ""}};

int ParseCharacteristicBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    ASAP2_CHARACTERISTIC* Characteristic;
    Characteristic = AddEmptyCharacteristicToBuffer (Parser);
    if (Characteristic == NULL) return -1;
    // 1. Parse the fixed parameters
    Characteristic->Name = ReadIdentFromFile_Buffered (Parser);
#ifdef DEBUG_PRINTF
    printf ("CHARACTERISTIC: %s\n", Characteristic->Name);
#endif
    Characteristic->LongIdentifier = ReadStringFromFile_Buffered (Parser);
    if (Characteristic->LongIdentifier == NULL) return -1;
    Characteristic->Type = ReadEnumFromFile (Parser, ENUMDEF_CHARACTERISTIC_TYPE);
    Characteristic->Address = ReadUInt64FromFile (Parser);
    Characteristic->FileOffsetAddress = Parser->CurrentOffsetStartWord;

    Characteristic->Deposit = ReadIdentFromFile_Buffered (Parser);
    Characteristic->MaxDiff = ReadFloatFromFile (Parser);
    Characteristic->Conversion = ReadIdentFromFile_Buffered (Parser);
    Characteristic->LowerLimit = ReadFloatFromFile (Parser);
    Characteristic->UpperLimit = ReadFloatFromFile (Parser);
    if CheckParserError(Parser) return -1;

    // 2. Parse the optionale parameters
    while (GetNextOptionalParameter (Parser, CharacteristicKeywordTable, (void*)Characteristic) == 1);
    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}

//-------------------------------------------------------------------
//  MEASUREMENT
//-------------------------------------------------------------------

int ParseMeasurementDisplayIdentifier (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MEASUREMENT* Measurement = (ASAP2_MEASUREMENT*)Data;
    CHECK_SET_FLAG_BIT_POS (Measurement->OptionalParameter.Flags, 
                        OPTPARAM_MEASUREMENT_DISPLAY_IDENTIFIER);
    Measurement->OptionalParameter.DisplayIdentifier = ReadIdentFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseMeasurementReadWrite (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MEASUREMENT* Measurement = (ASAP2_MEASUREMENT*)Data;
    CHECK_SET_FLAG_BIT_POS (Measurement->OptionalParameter.Flags, 
                        OPTPARAM_MEASUREMENT_WRITABLE_FLAG);
    return CheckParserError (Parser);
}

int ParseMeasurementFormat (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MEASUREMENT* Measurement = (ASAP2_MEASUREMENT*)Data;
    CHECK_SET_FLAG_BIT_POS (Measurement->OptionalParameter.Flags, 
                        OPTPARAM_MEASUREMENT_FORMAT);
    Measurement->OptionalParameter.Format = ReadStringFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseMeasurementArraySize (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MEASUREMENT* Measurement = (ASAP2_MEASUREMENT*)Data;
    CHECK_SET_FLAG_BIT_POS (Measurement->OptionalParameter.Flags, 
                        OPTPARAM_MEASUREMENT_ARRAY_SIZE);
    Measurement->OptionalParameter.Format = ReadStringFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseMeasurementByteOrder (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MEASUREMENT* Measurement = (ASAP2_MEASUREMENT*)Data;
    CHECK_SET_FLAG_BIT_POS (Measurement->OptionalParameter.Flags, 
                        OPTPARAM_MEASUREMENT_BYTE_ORDER);
    Measurement->OptionalParameter.ByteOrder = ReadEnumFromFile (Parser, ENUMDEF_BYTEORDER);
    return CheckParserError (Parser);
}

int ParseMeasurementBitMask (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MEASUREMENT* Measurement = (ASAP2_MEASUREMENT*)Data;
    CHECK_SET_FLAG_BIT_POS (Measurement->OptionalParameter.Flags, 
                        OPTPARAM_MEASUREMENT_BIT_MASK);
    Measurement->OptionalParameter.BitMask = ReadUInt64FromFile (Parser);
    return CheckParserError (Parser);
}

int ParseMeasurementEcuAddress (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MEASUREMENT* Measurement = (ASAP2_MEASUREMENT*)Data;
    CHECK_SET_FLAG_BIT_POS (Measurement->OptionalParameter.Flags, 
                        OPTPARAM_MEASUREMENT_ADDRESS);
    Measurement->OptionalParameter.Address = ReadUInt64FromFile (Parser);
    Measurement->OptionalParameter.FileOffsetAddress = Parser->CurrentOffsetStartWord;

    return CheckParserError (Parser);
}

int ParseMeasurementMatrixDim (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MEASUREMENT* Measurement = (ASAP2_MEASUREMENT*)Data;
    CHECK_SET_FLAG_BIT_POS (Measurement->OptionalParameter.Flags,
                        OPTPARAM_MEASUREMENT_MATRIX_DIM);
    Measurement->OptionalParameter.MatrixDim.x = ReadIntFromFile (Parser);
    Measurement->OptionalParameter.MatrixDim.y = TryReadIntFromFile (Parser, 0);  // sometime this is missing
    Measurement->OptionalParameter.MatrixDim.z = TryReadIntFromFile (Parser, 0);  // sometime this is missing
    return CheckParserError (Parser);
}


int ParseMeasurementSymbolLink(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MEASUREMENT* Measurement = (ASAP2_MEASUREMENT*)Data;
    CHECK_SET_FLAG_BIT_POS(Measurement->OptionalParameter.Flags,
                       OPTPARAM_MEASUREMENT_SYMBOL_LINK);
    Measurement->OptionalParameter.SymbolLink = ReadIdentFromFile_Buffered(Parser);
    Measurement->OptionalParameter.SymbolLinkX = ReadIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseMeasurementPhysUnit(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MEASUREMENT* Measurement = (ASAP2_MEASUREMENT*)Data;
    CHECK_SET_FLAG_BIT_POS(Measurement->OptionalParameter.Flags,
                       OPTPARAM_MEASUREMENT_PHYS_UNIT);
    Measurement->OptionalParameter.PhysUnit = ReadIdentFromFile_Buffered(Parser);
    return CheckParserError(Parser);
}

int ParseMeasurementLayout(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MEASUREMENT* Measurement = (ASAP2_MEASUREMENT*)Data;
    CHECK_SET_FLAG_BIT_POS(Measurement->OptionalParameter.Flags,
                       OPTPARAM_MEASUREMENT_LAYOUT);
    Measurement->OptionalParameter.Layout = ReadEnumFromFile(Parser, ENUMDEF_LAYOUT);
    return CheckParserError(Parser);
}
int ParseMeasurementDiscrete(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_MEASUREMENT* Measurement = (ASAP2_MEASUREMENT*)Data;
    CHECK_SET_FLAG_BIT_POS(Measurement->OptionalParameter.Flags,
                       OPTPARAM_MEASUREMENT_DISCRETE);
    Measurement->OptionalParameter.Discrete = 1;
    return CheckParserError(Parser);
}

#define TOKEN_MEASUREMENT_DISPLAY_IDENTIFIER       8000001
#define TOKEN_MEASUREMENT_READ_WRITE               8000002  
#define TOKEN_MEASUREMENT_FORMAT                   8000003
#define TOKEN_MEASUREMENT_ARRAY_SIZE               8000004
#define TOKEN_MEASUREMENT_BIT_MASK                 8000005 
#define TOKEN_MEASUREMENT_BIT_OPERATION            8000006
#define TOKEN_MEASUREMENT_BYTE_ORDER               8000007
#define TOKEN_MEASUREMENT_MAX_REFRESH              8000008
#define TOKEN_MEASUREMENT_VIRTUAL                  8000009
#define TOKEN_MEASUREMENT_FUNCTION_LIST            8000010  
#define TOKEN_MEASUREMENT_ECU_ADDRESS              8000011
#define TOKEN_MEASUREMENT_ERROR_MASK               8000012
#define TOKEN_MEASUREMENT_REF_MEMORY_SEGMENT       8000013 
#define TOKEN_MEASUREMENT_ANNOTATION               8000014
#define TOKEN_MEASUREMENT_IF_DATA                  8000015
#define TOKEN_MEASUREMENT_MATRIX_DIM               8000015
#define TOKEN_MEASUREMENT_ECU_ADDRESS_EXTENSION    8000017
#define TOKEN_MEASUREMENT_SYMBOL_LINK              8000018
#define TOKEN_MEASUREMENT_PHYS_UNIT                8000019
#define TOKEN_MEASUREMENT_LAYOUT                   8000020
#define TOKEN_MEASUREMENT_DISCRETE                 8000021

static ASAP2_KEYWORDS MeasurementKeywordTable[] =
    {{0, "DISPLAY_IDENTIFIER",     1, TOKEN_MEASUREMENT_DISPLAY_IDENTIFIER,    ParseMeasurementDisplayIdentifier, ""},
     {0, "READ_WRITE",             0, TOKEN_MEASUREMENT_READ_WRITE,            ParseMeasurementReadWrite, ""},
     {0, "FORMAT",                 1, TOKEN_MEASUREMENT_FORMAT,                ParseMeasurementFormat, ""},
     {0, "ARRAY_SIZE",             1, TOKEN_MEASUREMENT_ARRAY_SIZE,            ParseMeasurementArraySize, ""},
     {0, "BIT_MASK",               1, TOKEN_MEASUREMENT_BIT_MASK,              ParseMeasurementBitMask, ""},
     {1, "BIT_OPERATION",         -1, TOKEN_MEASUREMENT_BIT_OPERATION,         IgnoreBlock, ""},  // will be ignored
     {0, "BYTE_ORDER",             1, TOKEN_MEASUREMENT_BYTE_ORDER,            ParseMeasurementByteOrder, ""},
     {0, "MAX_REFRESH",            2, TOKEN_MEASUREMENT_MAX_REFRESH,           IgnoreBlock, ""},  // will be ignored
     {-1, "VIRTUAL",               1, TOKEN_MEASUREMENT_VIRTUAL,               IgnoreBlock, ""},  // will be ignored
     {1, "FUNCTION_LIST",         -1, TOKEN_MEASUREMENT_FUNCTION_LIST,         IgnoreBlock, ""},  // will be ignored
     {0, "ECU_ADDRESS",            1, TOKEN_MEASUREMENT_ECU_ADDRESS,           ParseMeasurementEcuAddress, ""},
     {0, "ERROR_MASK",             1, TOKEN_MEASUREMENT_ERROR_MASK,            IgnoreBlock, ""},  // will be ignored
     {0, "REF_MEMORY_SEGMENT",     1, TOKEN_MEASUREMENT_REF_MEMORY_SEGMENT,    IgnoreBlock, ""},  // will be ignored
     {1, "ANNOTATION",            -1, TOKEN_MEASUREMENT_ANNOTATION,            IgnoreBlock, ""},  // will be ignored
     {1, "IF_DATA",               -1, TOKEN_MEASUREMENT_IF_DATA,               ParseIfDataBlock, ""},
     {0, "MATRIX_DIM",             3, TOKEN_MEASUREMENT_MATRIX_DIM,            ParseMeasurementMatrixDim, ""},
     {0, "ECU_ADDRESS_EXTENSION",  1, TOKEN_MEASUREMENT_ECU_ADDRESS_EXTENSION, IgnoreBlock, ""},  // will be ignored
     {0, "SYMBOL_LINK",            2, TOKEN_MEASUREMENT_SYMBOL_LINK,           ParseMeasurementSymbolLink, ""},
     {0, "PHYS_UNIT",              1, TOKEN_MEASUREMENT_PHYS_UNIT,             ParseMeasurementPhysUnit, ""},   // if this is defined ignore the unit inside the COMPU_METHOD
     {0, "LAYOUT",                 1, TOKEN_MEASUREMENT_LAYOUT,                ParseMeasurementLayout, ""},
     {0, "DISCRETE",               0, TOKEN_MEASUREMENT_DISCRETE,              ParseMeasurementDiscrete, ""},
     {0, NULL,                     0, -1,                                      NULL, ""}};


int ParseMeasurementBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    ASAP2_MEASUREMENT* Measurement;
    Measurement = AddEmptyMeasurementToBuffer (Parser);
    if (Measurement == NULL) return -1;
    // 1. Parse the fixed parameters
    Measurement->Ident = ReadIdentFromFile_Buffered (Parser);
#ifdef DEBUG_PRINTF
    printf ("MEASUREMENT: %s\n", Measurement->Ident);
#endif
    Measurement->LongIdentifier = ReadStringFromFile_Buffered (Parser);
    if (Measurement->LongIdentifier == NULL) return -1;
    Measurement->DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
    Measurement->ConversionIdent = ReadIdentFromFile_Buffered (Parser);
    Measurement->Resolution = ReadIntFromFile (Parser);
    Measurement->Accuracy = ReadFloatFromFile (Parser);
    Measurement->LowerLimit = ReadFloatFromFile (Parser);
    Measurement->UpperLimit = ReadFloatFromFile (Parser);
    if CheckParserError(Parser) return -1;

    // 2. Parse the optionale parameters
    while (GetNextOptionalParameter (Parser, MeasurementKeywordTable, (void*)Measurement) == 1);
    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}

//-------------------------------------------------------------------
//  AXIS_PTS
//-------------------------------------------------------------------

int ParseAxisPtsDisplayIdentifier (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisPts->OptionalParameter.Flags, 
                        OPTPARAM_AXIS_PTS_DISPLAY_IDENTIFIER);
    AxisPts->OptionalParameter.DisplayIdentifier = ReadIdentFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseAxisPtsReadOnly (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisPts->OptionalParameter.Flags, 
                        OPTPARAM_AXIS_PTS_READ_ONLY);
    AxisPts->OptionalParameter.ReadOnly = 1;
    return CheckParserError (Parser);
}

int ParseAxisPtsFormat (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisPts->OptionalParameter.Flags, 
                        OPTPARAM_AXIS_PTS_FORMAT);
    AxisPts->OptionalParameter.Format = ReadStringFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseAxisPtsDeposit (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisPts->OptionalParameter.Flags, 
                        OPTPARAM_AXIS_PTS_DEPOSIT);
    AxisPts->OptionalParameter.DepositMode = ReadEnumFromFile (Parser, ENUMDEF_DEPOSIT_MODE);
    return CheckParserError (Parser);
}

int ParseAxisPtsByteOrder (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisPts->OptionalParameter.Flags, 
                        OPTPARAM_AXIS_PTS_BYTE_ORDER);
    AxisPts->OptionalParameter.ByteOrder = ReadEnumFromFile (Parser, ENUMDEF_BYTEORDER);
    return CheckParserError (Parser);
}

int ParseAxisPtsRefMemorySegment (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisPts->OptionalParameter.Flags, 
                        OPTPARAM_AXIS_PTS_REF_MEMORY_SEGMENT);
    AxisPts->OptionalParameter.RefMemorySegment = ReadIdentFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseAxisPtsGuardRails (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisPts->OptionalParameter.Flags, 
                        OPTPARAM_AXIS_PTS_GUARD_RAILS);
    AxisPts->OptionalParameter.GuardRails = 1;
    return CheckParserError (Parser);
}

int ParseAxisPtsExtendedLimits (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisPts->OptionalParameter.Flags, 
                        OPTPARAM_AXIS_PTS_EXTENDED_LIMITS);
    AxisPts->OptionalParameter.ExtendedLimits.LowerLimit = ReadFloatFromFile (Parser);
    AxisPts->OptionalParameter.ExtendedLimits.UpperLimit = ReadFloatFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseAxisPtsCalibrationAccess (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisPts->OptionalParameter.Flags, 
                        OPTPARAM_AXIS_PTS_CALIBRATION_ACCESS);
    AxisPts->OptionalParameter.CalibrationAccess = ReadEnumFromFile (Parser, ENUMDEF_CALIBRATION_TYPE);
    return CheckParserError (Parser);
}

int ParseAxisPtsEcuAddressExtension (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS (AxisPts->OptionalParameter.Flags, 
                        OPTPARAM_AXIS_PTS_ECU_ADDRESS_EXTENSION);
    AxisPts->OptionalParameter.EcuAddressExtension = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseAxisPtsSymbolLink(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS(AxisPts->OptionalParameter.Flags,
                       OPTPARAM_AXIS_PTS_SYMBOL_LINK);
    AxisPts->OptionalParameter.SymbolLink = ReadIdentFromFile_Buffered(Parser);
    AxisPts->OptionalParameter.SymbolLinkX = ReadIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseAxisPtsPhysUnit(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS(AxisPts->OptionalParameter.Flags,
                       OPTPARAM_AXIS_PTS_PHYS_UNIT);
    AxisPts->OptionalParameter.PhysUnit = ReadIdentFromFile_Buffered(Parser);
    return CheckParserError(Parser);
}

int ParseAxisPtsMonotony(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_AXIS_PTS* AxisPts = (ASAP2_AXIS_PTS*)Data;
    CHECK_SET_FLAG_BIT_POS(AxisPts->OptionalParameter.Flags,
                       OPTPARAM_AXIS_PTS_MONOTONY);
    AxisPts->OptionalParameter.Monotony = ReadEnumFromFile(Parser, ENUMDEF_MONOTONY);
    return CheckParserError(Parser);
}


#define TOKEN_AXIS_PTS_DISPLAY_IDENTIFIER          7000001
#define TOKEN_AXIS_PTS_READ_ONLY                   7000002
#define TOKEN_AXIS_PTS_FORMAT                      7000003
#define TOKEN_AXIS_PTS_DEPOSIT                     7000004
#define TOKEN_AXIS_PTS_BYTE_ORDER                  7000005
#define TOKEN_AXIS_PTS_FUNCTION_LIST               7000006
#define TOKEN_AXIS_PTS_REF_MEMORY_SEGMENT          7000007
#define TOKEN_AXIS_PTS_GUARD_RAILS                 7000008
#define TOKEN_AXIS_PTS_EXTENDED_LIMITS             7000009
#define TOKEN_AXIS_PTS_ANNOTATION                  7000010
#define TOKEN_AXIS_PTS_IF_DATA                     7000011
#define TOKEN_AXIS_PTS_CALIBRATION_ACCESS          7000012
#define TOKEN_AXIS_PTS_ECU_ADDRESS_EXTENSION       7000013
#define TOKEN_AXIS_PTS_SYMBOL_LINK                 7000014
#define TOKEN_AXIS_PTS_PHYS_UNIT                   7000015
#define TOKEN_AXIS_PTS_DESCR_MONOTONY              7000016

static ASAP2_KEYWORDS AxisPtsKeywordTable[] =
    {{0, "DISPLAY_IDENTIFIER",     0, TOKEN_AXIS_PTS_READ_ONLY,             ParseAxisPtsDisplayIdentifier, ""},
     {0, "READ_ONLY",              0, TOKEN_AXIS_PTS_READ_ONLY,             ParseAxisPtsReadOnly, ""},
     {0, "FORMAT",                 1, TOKEN_AXIS_PTS_FORMAT,                ParseAxisPtsFormat, ""},
     {0, "DEPOSIT",                1, TOKEN_AXIS_PTS_DEPOSIT,               ParseAxisPtsDeposit, ""},
     {0, "BYTE_ORDER",             1, TOKEN_AXIS_DESCR_BYTE_ORDER,          ParseAxisPtsByteOrder, ""},
     {1, "FUNCTION_LIST",         -1, TOKEN_AXIS_PTS_FUNCTION_LIST,         IgnoreBlock, ""},   // will be ignored
     {0, "REF_MEMORY_SEGMENT",     1, TOKEN_AXIS_PTS_REF_MEMORY_SEGMENT,    ParseAxisPtsRefMemorySegment, ""},
     {0, "GUARD_RAILS",            0, TOKEN_AXIS_PTS_GUARD_RAILS,           ParseAxisPtsGuardRails, ""},
     {0, "EXTENDED_LIMITS",        2, TOKEN_AXIS_PTS_EXTENDED_LIMITS,       ParseAxisPtsExtendedLimits, ""},
     {1, "ANNOTATION",            -1, TOKEN_AXIS_PTS_ANNOTATION,            IgnoreBlock, ""},                  // will be ignored
     {1, "IF_DATA",               -1, TOKEN_AXIS_PTS_IF_DATA,               ParseIfDataBlock, ""},
     {0, "CALIBRATION_ACCESS",     1, TOKEN_AXIS_PTS_CALIBRATION_ACCESS,    ParseAxisPtsCalibrationAccess, ""},
     {0, "ECU_ADDRESS_EXTENSION",  1, TOKEN_AXIS_PTS_ECU_ADDRESS_EXTENSION, ParseAxisPtsEcuAddressExtension, ""},
     {0, "SYMBOL_LINK",            2, TOKEN_AXIS_PTS_SYMBOL_LINK,           ParseAxisPtsSymbolLink, ""},
     {0, "PHYS_UNIT",              1, TOKEN_AXIS_PTS_PHYS_UNIT,             ParseAxisPtsPhysUnit, ""},   // if this is defined ignore the unit inside the COMPU_METHOD
     {0, "MONOTONY",               1, TOKEN_AXIS_PTS_DESCR_MONOTONY,        ParseAxisPtsMonotony, ""},
     {0, NULL,                     0, -1,                                   NULL, ""}};


int ParseAxisPtsBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    ASAP2_AXIS_PTS* AxisPts;
    AxisPts = AddEmptyAxisPtsToBuffer (Parser);
    if (AxisPts == NULL) return -1;
    // 1. Parse the fixed parameters
    AxisPts->Name = ReadIdentFromFile_Buffered (Parser);
#ifdef DEBUG_PRINTF
    printf ("AXIS_PTS: %s\n", AxisPts->Name);
#endif
    AxisPts->LongIdentifier = ReadStringFromFile_Buffered (Parser);
    if (AxisPts->LongIdentifier == NULL) return -1;
    AxisPts->Address = ReadUInt64FromFile (Parser);
    AxisPts->FileOffsetAddress = Parser->CurrentOffsetStartWord;

    AxisPts->InputQuantity = ReadIdentFromFile_Buffered (Parser);
    AxisPts->Deposit = ReadIdentFromFile_Buffered (Parser);
    AxisPts->MaxDiff = ReadFloatFromFile (Parser);
    AxisPts->Conversion = ReadIdentFromFile_Buffered (Parser);
    AxisPts->MaxAxisPoints = ReadIntFromFile (Parser);
    AxisPts->LowerLimit = ReadFloatFromFile (Parser);
    AxisPts->UpperLimit = ReadFloatFromFile (Parser);
    if CheckParserError(Parser) return -1;

    // 2. Parse the optionale parameters
    while (GetNextOptionalParameter (Parser, AxisPtsKeywordTable, (void*)AxisPts) == 1);
    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}

//-------------------------------------------------------------------
//  COMPU_TAB
//-------------------------------------------------------------------

int ParseCompuTabDefaultValue (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_COMPU_TAB* CompuTab = (ASAP2_COMPU_TAB*)Data;
    CHECK_SET_FLAG_BIT_POS (CompuTab->OptionalParameter.Flags, 
                        OPTPARAM_COMPU_TAB_DEFAULT_VALUE);
    CompuTab->OptionalParameter.DefaultValue = ReadStringFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

#define TOKEN_COMPU_TAB_DEFAULT_VALUE     6000001

static ASAP2_KEYWORDS CompuTabKeywordTable[] =
    {{0, "DEFAULT_VALUE", -1, TOKEN_COMPU_TAB_DEFAULT_VALUE, ParseCompuTabDefaultValue, ""},
     {0, NULL,             0, -1,                            NULL, ""}};

int ParseCompuTabBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    ASAP2_COMPU_TAB* CompuTab;
    int x;

    CompuTab = AddEmptyCompuTabToBuffer (Parser);
    if (CompuTab == NULL) return -1;
    if (!strcmp ("COMPU_VTAB_RANGE", KeywordTableEntry->KeywordString)) {
        CompuTab->TabOrVtabFlag = 2;  // has 2 value and a text replace
    } else if (!strcmp ("COMPU_VTAB", KeywordTableEntry->KeywordString)) {
        CompuTab->TabOrVtabFlag = 1;  // has 1 value and a text replace
    } else {
        CompuTab->TabOrVtabFlag = 0; // COMPU_TAB has only 2 values and no text replaces
    }
    CompuTab->Name = ReadIdentFromFile_Buffered (Parser);
    if (CompuTab->Name == NULL) return -1;
#ifdef DEBUG_PRINTF
    printf ("COMPU_TAB: %s\n", CompuTab->Name);
#endif
    CompuTab->LongIdentifier = ReadStringFromFile_Buffered (Parser);
    if (CompuTab->LongIdentifier == NULL) return -1;
    if (CompuTab->TabOrVtabFlag <= 1) {
        CompuTab->ConversionType = ReadEnumFromFile(Parser, ENUMDEF_CONVERSIONTYPE);
    } else {
        CompuTab->ConversionType = -1;
    }
    CompuTab->NumberValuePairs = ReadIntFromFile (Parser);
    CompuTab->ValuePairs = (ASAP2_COMPU_TAB_VALUE_PAIR*)AllocMemFromBuffer (&Parser->Data, CompuTab->NumberValuePairs * sizeof (ASAP2_COMPU_TAB_VALUE_PAIR));
    if (CompuTab->ValuePairs == NULL) {
        ThrowParserError (Parser, __FILE__, __LINE__, "out of memory");
        return -1;
    }
    for (x = 0; x < CompuTab->NumberValuePairs; x++) {
        CompuTab->ValuePairs[x].InVal_InValMin = ReadFloatFromFile (Parser);
        if ((CompuTab->TabOrVtabFlag == 0) || (CompuTab->TabOrVtabFlag == 2)) CompuTab->ValuePairs[x].OutVal_InValMax = ReadFloatFromFile (Parser);
        if (CompuTab->TabOrVtabFlag != 0) CompuTab->ValuePairs[x].OutString = ReadStringFromFile_Buffered (Parser);
        if (CheckParserError (Parser)) {
            ThrowParserError (Parser, __FILE__, __LINE__, "expecting a value pair");
            return -1;
        }
    }
    if CheckParserError(Parser) return -1;

    // 2. Parse the optionale parameters
    while (GetNextOptionalParameter (Parser, CompuTabKeywordTable, (void*)CompuTab) == 1);

    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}



//-------------------------------------------------------------------
//  COMPU_METHOD
//-------------------------------------------------------------------

int ParseCompuMethodFormula (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    char *Help;

    ASAP2_COMPU_METHOD* CompuMethod = (ASAP2_COMPU_METHOD*)Data;
    CHECK_SET_FLAG_BIT_POS (CompuMethod->OptionalParameter.Flags, 
                        OPTPARAM_COMPU_METHOD_FORMULA);
    CompuMethod->OptionalParameter.Formula = ReadStringFromFile_Buffered (Parser);
    Help = ReadKeyWordFromFile (Parser);
    if (Help == NULL) return -1;
    if (!strcmp (Help, "FORMULA_INV")) {
        CHECK_SET_FLAG_BIT_POS (CompuMethod->OptionalParameter.Flags, 
                            OPTPARAM_COMPU_METHOD_FORMULA_INV);
        CompuMethod->OptionalParameter.FormulaInv = ReadStringFromFile_Buffered (Parser);
        Help = ReadKeyWordFromFile (Parser);
        if (Help == NULL) return -1;
    } 
    if (strcmp (Help, "/end")) {
        ThrowParserError (Parser, __FILE__, __LINE__, "expecting a \"/end FORMULA\" not a \"%s\"", Help);
        return -1;
    }
    Help = ReadKeyWordFromFile (Parser);
    if (Help == NULL) return -1;
    if (strcmp (Help, KeywordTableEntry->KeywordString)) {
        ThrowParserError (Parser, __FILE__, __LINE__, "expecting a \"/end FORMULA\" not a \"%s\"", Help);
        return -1;
    }
    return CheckParserError (Parser);
}

int ParseCompuMethodCoeffs (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_COMPU_METHOD* CompuMethod = (ASAP2_COMPU_METHOD*)Data;
    CHECK_SET_FLAG_BIT_POS (CompuMethod->OptionalParameter.Flags, 
                        OPTPARAM_COMPU_METHOD_COEFFS);
    CompuMethod->OptionalParameter.Coeffs.a = ReadFloatFromFile (Parser);
    CompuMethod->OptionalParameter.Coeffs.b = ReadFloatFromFile (Parser);
    CompuMethod->OptionalParameter.Coeffs.c = ReadFloatFromFile (Parser);
    CompuMethod->OptionalParameter.Coeffs.d = ReadFloatFromFile (Parser);
    CompuMethod->OptionalParameter.Coeffs.e = ReadFloatFromFile (Parser);
    CompuMethod->OptionalParameter.Coeffs.f = ReadFloatFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseCompuMethodCompuTabRef (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_COMPU_METHOD* CompuMethod = (ASAP2_COMPU_METHOD*)Data;
    CHECK_SET_FLAG_BIT_POS (CompuMethod->OptionalParameter.Flags, 
                        OPTPARAM_COMPU_METHOD_COMPU_TAB_REF);
    CompuMethod->OptionalParameter.ConversionTable = ReadIdentFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseCompuMethodRefUnit (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_COMPU_METHOD* CompuMethod = (ASAP2_COMPU_METHOD*)Data;
    CHECK_SET_FLAG_BIT_POS (CompuMethod->OptionalParameter.Flags, 
                        OPTPARAM_COMPU_METHOD_REF_UNIT);
    CompuMethod->OptionalParameter.Unit = ReadIdentFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

int ParseCompuMethodCoeffsLinear(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_COMPU_METHOD* CompuMethod = (ASAP2_COMPU_METHOD*)Data;
    CHECK_SET_FLAG_BIT_POS(CompuMethod->OptionalParameter.Flags,
                       OPTPARAM_COMPU_METHOD_COEFFS_LINEAR);
    CompuMethod->OptionalParameter.Coeffs.a = ReadFloatFromFile(Parser);
    CompuMethod->OptionalParameter.Coeffs.b = ReadFloatFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseCompuMethodStatusStringRef(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_COMPU_METHOD* CompuMethod = (ASAP2_COMPU_METHOD*)Data;
    CHECK_SET_FLAG_BIT_POS(CompuMethod->OptionalParameter.Flags,
                       OPTPARAM_COMPU_METHOD_STATUS_STRING_REFR);
    CompuMethod->OptionalParameter.StatusStringRef = ReadIdentFromFile_Buffered(Parser);
    return CheckParserError(Parser);
}

#define TOKEN_COMPU_METHOD_FORMULA      5000001
// #define TOKEN_COMPU_METHOD_FORMULA_INV  5000002
#define TOKEN_COMPU_COEFFS              5000003
#define TOKEN_COMPU_COMPU_TAB_REF       5000004
#define TOKEN_COMPU_REF_UNIT            5000005
#define TOKEN_COMPU_COEFFS_LINEAR       5000006
#define TOKEN_COMPU_STATUS_STRING_REFR  5000007

static ASAP2_KEYWORDS CompuMethodKeywordTable[] =
    {{1, "FORMULA",          -1, TOKEN_COMPU_METHOD_FORMULA,     ParseCompuMethodFormula, ""},
     {0, "COEFFS",            6, TOKEN_COMPU_COEFFS,             ParseCompuMethodCoeffs, ""},
     {0, "COMPU_TAB_REF",     1, TOKEN_COMPU_COMPU_TAB_REF,      ParseCompuMethodCompuTabRef, ""},
     {0, "REF_UNIT",          1, TOKEN_COMPU_REF_UNIT,           ParseCompuMethodRefUnit, ""},
     {0, "COEFFS_LINEAR",     2, TOKEN_COMPU_COEFFS_LINEAR,      ParseCompuMethodCoeffsLinear, ""},
     {0, "STATUS_STRING_REF", 1, TOKEN_COMPU_STATUS_STRING_REFR, ParseCompuMethodStatusStringRef, ""},
     {0, NULL,             0, -1,                         NULL, ""}};

int ParseCompuMethodBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    ASAP2_COMPU_METHOD* CompuMethod;
    CompuMethod = AddEmptyCompuMethodToBuffer (Parser);
    if (CompuMethod == NULL) return -1;
    // 1. Parse the fixed parameters
    CompuMethod->Name = ReadIdentFromFile_Buffered (Parser);
#ifdef DEBUG_PRINTF
    printf ("COMPU_METHOD: %s\n", CompuMethod->Name);
#endif
    CompuMethod->LongIdentifier = ReadStringFromFile_Buffered (Parser);
    if (CompuMethod->LongIdentifier == NULL) return -1;
    CompuMethod->ConversionType = ReadEnumFromFile (Parser, ENUMDEF_CONVERSIONTYPE);
    CompuMethod->Format = ReadStringFromFile_Buffered (Parser);
    if (CompuMethod->Format == NULL) return -1;
    CompuMethod->Unit = ReadStringFromFile_Buffered (Parser);
    if (CompuMethod->Unit == NULL) return -1;

    // 2. Parse the optionale parameters
    while (GetNextOptionalParameter (Parser, CompuMethodKeywordTable, (void*)CompuMethod) == 1);
    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}

//-------------------------------------------------------------------
//  FUNCTION
//-------------------------------------------------------------------

static IDENT *ReadIdentArray (struct ASAP2_PARSER_STRUCT *Parser, int *ret_Size)
{
    IDENT *Ret;
    char *Help;
    int Count;

    int LineNr;
    int FileOffset = CacheTell(Parser->Cache, &LineNr);
    // First run only count
    for (Count = 0; ; Count++) {
        Help = ReadKeyWordFromFile(Parser);
        if (Help == NULL) return NULL;
        if (!strcmp(Help, "/end")) {
            break; // for (;;)
        }
    }
    CacheSeek(Parser->Cache, FileOffset, SEEK_SET, LineNr);
    Ret = AllocMemFromBuffer (&Parser->Data, Count * sizeof(IDENT));
    if (Ret == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        // secound run insert all
        for (int x = 0; x < Count; x++) {
            Ret[x] = ReadStringFromFile_Buffered(Parser);
            if (Ret[x] == NULL) return NULL;
        }
        Help = ReadKeyWordFromFile(Parser);
        if (Help == NULL) return NULL;
        if (strcmp(Help, "/end")) {
            ThrowParserError(Parser, __FILE__, __LINE__, "expecting a \"/end\" token not a \"%s\"", Help);
        }
    }
    *ret_Size = Count;
    return Ret;
}

int ParseFunctionDefCharacteristic (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    char *Help;
    ASAP2_FUNCTION* Function = (ASAP2_FUNCTION*)Data;
    CHECK_SET_FLAG_BIT_POS (Function->OptionalParameter.Flags, 
                        OPTPARAM_FUNCTION_DEF_CHARACTERISTIC);
    Function->OptionalParameter.DefCharacteristics = ReadIdentArray(Parser, &Function->OptionalParameter.DefCharacteristicSize);
    if (Function->OptionalParameter.DefCharacteristics == NULL) return -1;
    Help = ReadKeyWordFromFile (Parser);
    if (Help == NULL) return -1;
    if (strcmp (Help, KeywordTableEntry->KeywordString)) {
        ThrowParserError (Parser, __FILE__, __LINE__, "expecting a \"/end DEF_CHARACTERISTIC\" token not a \"%s\"", Help);
    }
    return CheckParserError (Parser);
}

int ParseFunctionRefCharacteristic (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    char *Help;
    ASAP2_FUNCTION* Function = (ASAP2_FUNCTION*)Data;
    CHECK_SET_FLAG_BIT_POS (Function->OptionalParameter.Flags, 
                        OPTPARAM_FUNCTION_REF_CHARACTERISTIC);
    Function->OptionalParameter.RefCharacteristics = ReadIdentArray(Parser, &Function->OptionalParameter.RefCharacteristicSize);
    if (Function->OptionalParameter.RefCharacteristics == NULL) return -1;
    Help = ReadKeyWordFromFile (Parser);
    if (Help == NULL) return -1;
    if (strcmp (Help, KeywordTableEntry->KeywordString)) {
        ThrowParserError (Parser, __FILE__, __LINE__, "expecting a \"/end REF_CHARACTERISTIC\" token");
    }
    return CheckParserError (Parser);
}

int ParseFunctionInMeasurement (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    char *Help;
    ASAP2_FUNCTION* Function = (ASAP2_FUNCTION*)Data;
    CHECK_SET_FLAG_BIT_POS (Function->OptionalParameter.Flags, 
                        OPTPARAM_FUNCTION_IN_MEASUREMENT);
    Function->OptionalParameter.InMeasurements = ReadIdentArray(Parser, &Function->OptionalParameter.InMeasurementSize);
    if (Function->OptionalParameter.InMeasurements == NULL) return -1;
    Help = ReadKeyWordFromFile (Parser);
    if (Help == NULL) return -1;
    if (strcmp (Help, KeywordTableEntry->KeywordString)) {
        ThrowParserError (Parser, __FILE__, __LINE__, "expecting a \"/end IN_MEASUREMENT\" token not a \"%s\"", Help);
    }
    return CheckParserError (Parser);
}

int ParseFunctionOutMeasurement (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    char *Help;
    ASAP2_FUNCTION* Function = (ASAP2_FUNCTION*)Data;
    CHECK_SET_FLAG_BIT_POS (Function->OptionalParameter.Flags, 
                        OPTPARAM_FUNCTION_OUT_MEASUREMENT);
    Function->OptionalParameter.OutMeasurements = ReadIdentArray(Parser, &Function->OptionalParameter.OutMeasurementSize);
    if (Function->OptionalParameter.OutMeasurements == NULL) return -1;
    Help = ReadKeyWordFromFile (Parser);
    if (Help == NULL) return -1;
    if (strcmp (Help, KeywordTableEntry->KeywordString)) {
        ThrowParserError (Parser, __FILE__, __LINE__, "expecting a \"/end OUT_MEASUREMENT\" token not a \"%s\"", Help);
    }
    return CheckParserError (Parser);
}

int ParseFunctionLocMeasurement (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    char *Help;
    ASAP2_FUNCTION* Function = (ASAP2_FUNCTION*)Data;
    CHECK_SET_FLAG_BIT_POS (Function->OptionalParameter.Flags, 
                        OPTPARAM_FUNCTION_LOC_MEASUREMENT);
    Function->OptionalParameter.LocMeasurements = ReadIdentArray(Parser, &Function->OptionalParameter.LocMeasurementSize);
    if (Function->OptionalParameter.LocMeasurements == NULL) return -1;
    Help = ReadKeyWordFromFile (Parser);
    if (Help == NULL) return -1;
    if (strcmp (Help, KeywordTableEntry->KeywordString)) {
        ThrowParserError (Parser, __FILE__, __LINE__, "expecting a \"/end LOC_MEASUREMENT\" token not a \"%s\"", Help);
    }
    return CheckParserError (Parser);
}

int ParseFunctionSubFunction (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    char *Help;
    ASAP2_FUNCTION* Function = (ASAP2_FUNCTION*)Data;
    CHECK_SET_FLAG_BIT_POS (Function->OptionalParameter.Flags, 
                        OPTPARAM_FUNCTION_SUB_FUNCTION);
    Function->OptionalParameter.SubFunctions = ReadIdentArray(Parser, &Function->OptionalParameter.SubFunctionSize);
    if (Function->OptionalParameter.SubFunctions == NULL) return -1;
    Help = ReadKeyWordFromFile (Parser);
    if (Help == NULL) return -1;
    if (strcmp (Help, KeywordTableEntry->KeywordString)) {
        ThrowParserError (Parser, __FILE__, __LINE__, "expecting a \"/end SUB_FUNCTION\" token not a \"%s\"", Help);
    }
    return CheckParserError (Parser);
}

int ParseFunctionFunctionVersion (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_FUNCTION* Function = (ASAP2_FUNCTION*)Data;
    CHECK_SET_FLAG_BIT_POS (Function->OptionalParameter.Flags, 
                        OPTPARAM_FUNCTION_FUNCTION_VERSION);
    Function->OptionalParameter.Version = ReadStringFromFile_Buffered (Parser);
    return CheckParserError (Parser);
}

#define TOKEN_FUNCTION_ANNOTATION                 4000001
#define TOKEN_FUNCTION_DEF_CHARACTERISTIC         4000002
#define TOKEN_FUNCTION_REF_CHARACTERISTIC         4000003
#define TOKEN_FUNCTION_IN_MEASUREMENT             4000004
#define TOKEN_FUNCTION_OUT_MEASUREMENT            4000005
#define TOKEN_FUNCTION_LOC_MEASUREMENT            4000006
#define TOKEN_FUNCTION_SUB_FUNCTION               4000007
#define TOKEN_FUNCTION_FUNCTION_VERSION           4000008

static ASAP2_KEYWORDS FunctionKeywordTable[] =
    {{1, "ANNOTATION",         -1, TOKEN_FUNCTION_ANNOTATION,         IgnoreBlock, ""},   // will be ignored
     {1, "DEF_CHARACTERISTIC", -1, TOKEN_FUNCTION_DEF_CHARACTERISTIC, ParseFunctionDefCharacteristic, ""},
     {1, "REF_CHARACTERISTIC", -1, TOKEN_FUNCTION_REF_CHARACTERISTIC, ParseFunctionRefCharacteristic, ""},
     {1, "IN_MEASUREMENT",     -1, TOKEN_FUNCTION_IN_MEASUREMENT,     ParseFunctionInMeasurement, ""},
     {1, "OUT_MEASUREMENT",    -1, TOKEN_FUNCTION_OUT_MEASUREMENT,    ParseFunctionOutMeasurement, ""},
     {1, "LOC_MEASUREMENT",    -1, TOKEN_FUNCTION_LOC_MEASUREMENT,    ParseFunctionLocMeasurement, ""},
     {1, "SUB_FUNCTION",       -1, TOKEN_FUNCTION_SUB_FUNCTION,       ParseFunctionSubFunction, ""},
     {0, "FUNCTION_VERSION",   -1, TOKEN_FUNCTION_FUNCTION_VERSION,   ParseFunctionFunctionVersion, ""},
     {1, NULL,                  0, -1,                                NULL, ""}};


int ParseFunctionBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    ASAP2_FUNCTION* Function;
    Function = AddEmptyFunctionToBuffer (Parser);
    if (Function == NULL) return -1;
    // 1. Parse the fixed parameters
    Function->Name = ReadIdentFromFile_Buffered (Parser);
#ifdef DEBUG_PRINTF
    printf ("FUNCTION: %s\n", Function->Name);
#endif
    Function->LongIdentifier = ReadStringFromFile_Buffered (Parser);
    if (Function->LongIdentifier == NULL) return -1;
    // 2. Parse the optionale parameters
    while (GetNextOptionalParameter (Parser, FunctionKeywordTable, (void*)Function) == 1);

    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}


//-------------------------------------------------------------------
//  RECORD_LAYOUT
//-------------------------------------------------------------------

int AddToRecordLayout(struct ASAP2_PARSER_STRUCT *Parser, ASAP2_RECORD_LAYOUT* RecordLayout, ASAP2_RECORD_LAYOUT_ITEM *Item)
{
    int x, y;
    if (RecordLayout->OptionalParameter.ItemCount >= RecordLayout->OptionalParameter.ItemSize) {
        RecordLayout->OptionalParameter.ItemSize += 16;
        RecordLayout->OptionalParameter.Items = (ASAP2_RECORD_LAYOUT_ITEM**)my_realloc(RecordLayout->OptionalParameter.Items, RecordLayout->OptionalParameter.ItemSize * sizeof(ASAP2_RECORD_LAYOUT_ITEM*));
        if (RecordLayout->OptionalParameter.Items == NULL) {
            ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
            return -1;
        }
    }
    // save it sorted by position
    for (x = 0; x < RecordLayout->OptionalParameter.ItemCount; x++) {
        if (RecordLayout->OptionalParameter.Items[x]->Position > Item->Position) {
            for (y = RecordLayout->OptionalParameter.ItemCount; y > x; y--) {
                RecordLayout->OptionalParameter.Items[y] = RecordLayout->OptionalParameter.Items[y-1];
            }
            break;
        }
    }
    RecordLayout->OptionalParameter.Items[x] = Item;
    RecordLayout->OptionalParameter.ItemCount++;
    return 0;
}

int ParseRecordLayoutFncValues (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS (RecordLayout->OptionalParameter.Flags, 
                            OPTPARAM_RECORDLAYOUT_FNC_VALUES);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_FNC_VALUES;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.FncValues.Datatype = ReadEnumFromFile(Parser, ENUMDEF_DATATYPE);
        New->Item.FncValues.IndexMode = ReadEnumFromFile(Parser, ENUMDEF_INDEXMODE);
        New->Item.FncValues.Addresstype = ReadEnumFromFile(Parser, ENUMDEF_ADDRTYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutAxisPtsX (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_AXIS_PTS_X);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_AXIS_PTS_X;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.AxisPtsX.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        New->Item.AxisPtsX.IndexIncr = ReadEnumFromFile (Parser, ENUMDEF_INDEXORDER);
        New->Item.AxisPtsX.Addressing = ReadEnumFromFile (Parser, ENUMDEF_ADDRTYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutAxisPtsY (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_AXIS_PTS_Y);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_AXIS_PTS_Y;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.AxisPtsY.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        New->Item.AxisPtsY.IndexIncr = ReadEnumFromFile (Parser, ENUMDEF_INDEXORDER);
        New->Item.AxisPtsY.Addressing = ReadEnumFromFile (Parser, ENUMDEF_ADDRTYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutAxisPtsZ (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_AXIS_PTS_Z);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_AXIS_PTS_Z;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.AxisPtsZ.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        New->Item.AxisPtsZ.IndexIncr = ReadEnumFromFile (Parser, ENUMDEF_INDEXORDER);
        New->Item.AxisPtsZ.Addressing = ReadEnumFromFile (Parser, ENUMDEF_ADDRTYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutAxisRescaleX (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_X);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_X;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.AxisRescaleX.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        New->Item.AxisRescaleX.MaxNumberOfRescalePairs = ReadIntFromFile (Parser);
        New->Item.AxisRescaleX.IndexIncr = ReadEnumFromFile (Parser, ENUMDEF_INDEXORDER);
        New->Item.AxisRescaleX.Addressing = ReadEnumFromFile (Parser, ENUMDEF_ADDRTYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutAxisRescaleY (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Y);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Y;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.AxisRescaleY.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        New->Item.AxisRescaleY.MaxNumberOfRescalePairs = ReadIntFromFile (Parser);
        New->Item.AxisRescaleY.IndexIncr = ReadEnumFromFile (Parser, ENUMDEF_INDEXORDER);
        New->Item.AxisRescaleY.Addressing = ReadEnumFromFile (Parser, ENUMDEF_ADDRTYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutAxisRescaleZ (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Z);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_AXIS_RESCALE_Z;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.AxisRescaleZ.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        New->Item.AxisRescaleZ.MaxNumberOfRescalePairs = ReadIntFromFile (Parser);
        New->Item.AxisRescaleZ.IndexIncr = ReadEnumFromFile (Parser, ENUMDEF_INDEXORDER);
        New->Item.AxisRescaleZ.Addressing = ReadEnumFromFile (Parser, ENUMDEF_ADDRTYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutNoAxisPtsX (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_X);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_X;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.NoAxisPtsX.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutNoAxisPtsY (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Y);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Y;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.NoAxisPtsY.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutNoAxisPtsZ (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Z);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_NO_AXIS_PTS_Z;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.NoAxisPtsZ.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutNoRescaleX (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_NO_RESCALE_X);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_NO_RESCALE_X;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.NoRescaleX.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutNoRescaleY (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_NO_RESCALE_Y);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_NO_RESCALE_Y;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.NoRescaleY.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutNoRescaleZ (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_NO_RESCALE_Z);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_NO_RESCALE_Z;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.NoRescaleZ.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}


int ParseRecordLayoutSrcAddrX (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_SRC_ADDR_X);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_SRC_ADDR_X;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.SrcAddrX.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutSrcAddrY (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_SRC_ADDR_Y);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_SRC_ADDR_Y;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.SrcAddrY.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutSrcAddrZ (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_SRC_ADDR_Z);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_SRC_ADDR_Z;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.SrcAddrZ.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutRipAddrX (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_RIP_ADDR_X);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_RIP_ADDR_X;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.RipAddrX.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutRipAddrY (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_RIP_ADDR_Y);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_RIP_ADDR_Y;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.RipAddrY.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutRipAddrZ (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_RIP_ADDR_Z);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_RIP_ADDR_Z;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.RipAddrZ.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutRipAddrW (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_RIP_ADDR_W);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_RIP_ADDR_W;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.RipAddrW.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutShiftOpX (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_SHIFT_OP_X);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_SHIFT_OP_X;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.ShiftOpX.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutShiftOpY (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_SHIFT_OP_Y);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_SHIFT_OP_Y;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.ShiftOpY.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutShiftOpZ (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_SHIFT_OP_Z);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_SHIFT_OP_Z;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.ShiftOpZ.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutOffsetX (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_OFFSET_X);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_OFFSET_X;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.OffsetX.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutOffsetY (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_OFFSET_Y);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_OFFSET_Y; 
        New->Position = ReadUIntFromFile(Parser);
        New->Item.OffsetY.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutOffsetZ (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_OFFSET_Z);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_OFFSET_Z;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.OffsetZ.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutDistOpX (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_DIST_OP_X);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_DIST_OP_X;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.DistOpX.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutDistOpY (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_DIST_OP_Y);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_DIST_OP_Y;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.DistOpY.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutDistOpZ (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_DIST_OP_Z);
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_DIST_OP_Z;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.DistOpZ.DataType = ReadEnumFromFile (Parser, ENUMDEF_DATATYPE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}

int ParseRecordLayoutReserved(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(KeywordTableEntry);
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT_ITEM *New;
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    // da mehrfach vorkommen kann wird hier nicht abgeprueft ob schon gesetzt
    RecordLayout->OptionalParameter.Flags |= 1ULL << OPTPARAM_RECORDLAYOUT_RESERVED;
    New = (ASAP2_RECORD_LAYOUT_ITEM*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_RECORD_LAYOUT_ITEM));
    if (New == NULL) {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    } else {
        New->ItemType = OPTPARAM_RECORDLAYOUT_RESERVED;
        New->Position = ReadUIntFromFile(Parser);
        New->Item.Reserved.DataSize = ReadEnumFromFile(Parser, ENUMDEF_DATASIZE);
        if (CheckParserError(Parser)) return -1;
        if (AddToRecordLayout(Parser, RecordLayout, New)) return -1;
    }
    return 0;
}



int ParseRecordLayoutFixNoAxisPtsX(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_X);
    RecordLayout->OptionalParameter.FixNoAxisPtsX.NumberOfAxisPoints = ReadUIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseRecordLayoutFixNoAxisPtsY(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_Y);
    RecordLayout->OptionalParameter.FixNoAxisPtsY.NumberOfAxisPoints = ReadUIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseRecordLayoutFixNoAxisPtsZ(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_FIX_NO_AXIS_PTS_Z);
    RecordLayout->OptionalParameter.FixNoAxisPtsZ.NumberOfAxisPoints = ReadUIntFromFile(Parser);
    return CheckParserError(Parser);
}


int ParseRecordLayoutAlignmentByte (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_ALIGNMENT_BYTE);
    RecordLayout->OptionalParameter.AlignmentByte.AlignmentBorder = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseRecordLayoutAlignmentWord (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_ALIGNMENT_WORD);
    RecordLayout->OptionalParameter.AlignmentWord.AlignmentBorder = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseRecordLayoutAlignmentLong (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_ALIGNMENT_LONG);
    RecordLayout->OptionalParameter.AlignmentLong.AlignmentBorder = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseRecordLayoutAlignmentInt64(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_ALIGNMENT_INT64);
    RecordLayout->OptionalParameter.AlignmentInt64.AlignmentBorder = ReadIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseRecordLayoutAlignmentFloat16 (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_ALIGNMENT_FLOAT16_IEEE);
    RecordLayout->OptionalParameter.AlignmentFloat16.AlignmentBorder = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseRecordLayoutAlignmentFloat32(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_ALIGNMENT_FLOAT32_IEEE);
    RecordLayout->OptionalParameter.AlignmentFloat32.AlignmentBorder = ReadIntFromFile(Parser);
    return CheckParserError(Parser);
}

int ParseRecordLayoutAlignmentFloat64 (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_ALIGNMENT_FLOAT64_IEEE);
    RecordLayout->OptionalParameter.AlignmentFloat64.AlignmentBorder = ReadIntFromFile (Parser);
    return CheckParserError (Parser);
}

int ParseRecordLayoutStaticRecordLayout(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT* RecordLayout = (ASAP2_RECORD_LAYOUT*)Data;
    CHECK_SET_FLAG_BIT_POS(RecordLayout->OptionalParameter.Flags,
                           OPTPARAM_RECORDLAYOUT_STATIC_RECORD_LAYOUT);
    RecordLayout->OptionalParameter.StaticRecordLayout = 1;
    return CheckParserError(Parser);
}

#define TOKEN_RECORD_LAYOUT_FNC_VALUES               3000001
#define TOKEN_RECORD_LAYOUT_IDENTIFICATION           3000002
#define TOKEN_RECORD_LAYOUT_AXIS_PTS_X               3000003
#define TOKEN_RECORD_LAYOUT_AXIS_PTS_Y               3000004
#define TOKEN_RECORD_LAYOUT_AXIS_PTS_Z               3000005
#define TOKEN_RECORD_LAYOUT_AXIS_RESCALE_X           3000006
#define TOKEN_RECORD_LAYOUT_AXIS_RESCALE_Y           3000007
#define TOKEN_RECORD_LAYOUT_AXIS_RESCALE_Z           3000008
#define TOKEN_RECORD_LAYOUT_NO_AXIS_PTS_X            3000009
#define TOKEN_RECORD_LAYOUT_NO_AXIS_PTS_Y            3000010
#define TOKEN_RECORD_LAYOUT_NO_AXIS_PTS_Z            3000011
#define TOKEN_RECORD_LAYOUT_NO_RESCALE_X             3000012
#define TOKEN_RECORD_LAYOUT_NO_RESCALE_Y             3000013
#define TOKEN_RECORD_LAYOUT_NO_RESCALE_Z             3000014
#define TOKEN_RECORD_LAYOUT_FIX_NO_AXIS_PTS_X        3000015
#define TOKEN_RECORD_LAYOUT_FIX_NO_AXIS_PTS_Y        3000016
#define TOKEN_RECORD_LAYOUT_FIX_NO_AXIS_PTS_Z        3000017
#define TOKEN_RECORD_LAYOUT_SRC_ADDR_X               3000018
#define TOKEN_RECORD_LAYOUT_SRC_ADDR_Y               3000019
#define TOKEN_RECORD_LAYOUT_SRC_ADDR_Z               3000020
#define TOKEN_RECORD_LAYOUT_RIP_ADDR_X               3000021
#define TOKEN_RECORD_LAYOUT_RIP_ADDR_Y               3000022
#define TOKEN_RECORD_LAYOUT_RIP_ADDR_Z               3000023
#define TOKEN_RECORD_LAYOUT_RIP_ADDR_W               3000024
#define TOKEN_RECORD_LAYOUT_SHIFT_OP_X               3000025
#define TOKEN_RECORD_LAYOUT_SHIFT_OP_Y               3000026
#define TOKEN_RECORD_LAYOUT_SHIFT_OP_Z               3000027
#define TOKEN_RECORD_LAYOUT_OFFSET_X                 3000028
#define TOKEN_RECORD_LAYOUT_OFFSET_Y                 3000029
#define TOKEN_RECORD_LAYOUT_OFFSET_Z                 3000030
#define TOKEN_RECORD_LAYOUT_DIST_OP_X                3000031
#define TOKEN_RECORD_LAYOUT_DIST_OP_Y                3000032
#define TOKEN_RECORD_LAYOUT_DIST_OP_Z                3000033
#define TOKEN_RECORD_LAYOUT_ALIGNMENT_BYTE           3000034
#define TOKEN_RECORD_LAYOUT_ALIGNMENT_WORD           3000035
#define TOKEN_RECORD_LAYOUT_ALIGNMENT_LONG           3000036
#define TOKEN_RECORD_LAYOUT_ALIGNMENT_INT64          3000037
#define TOKEN_RECORD_LAYOUT_ALIGNMENT_FLOAT16_IEEE   3000038
#define TOKEN_RECORD_LAYOUT_ALIGNMENT_FLOAT32_IEEE   3000039
#define TOKEN_RECORD_LAYOUT_ALIGNMENT_FLOAT64_IEEE   3000040
#define TOKEN_RECORD_LAYOUT_RESERVED                 3000041
#define TOKEN_RECORD_LAYOUT_STATIC_RECORD_LAYOUT     3000042

static ASAP2_KEYWORDS RecordLayoutKeywordTable[] =
    {{0, "FNC_VALUES",              4, TOKEN_RECORD_LAYOUT_FNC_VALUES,             ParseRecordLayoutFncValues, ""},
     {0, "IDENTIFICATION",          1, TOKEN_RECORD_LAYOUT_IDENTIFICATION,         IgnoreBlock, ""},   // will be ignored
     {0, "AXIS_PTS_X",              3, TOKEN_RECORD_LAYOUT_AXIS_PTS_Y,             ParseRecordLayoutAxisPtsX, ""},
     {0, "AXIS_PTS_Y",              3, TOKEN_RECORD_LAYOUT_AXIS_PTS_X,             ParseRecordLayoutAxisPtsY, ""},
     {0, "AXIS_PTS_Z",              3, TOKEN_RECORD_LAYOUT_AXIS_PTS_Z,             ParseRecordLayoutAxisPtsZ, ""},
     {0, "AXIS_RESCALE_X",          4, TOKEN_RECORD_LAYOUT_AXIS_RESCALE_X,         ParseRecordLayoutAxisRescaleX, ""},
     {0, "AXIS_RESCALE_Y",          4, TOKEN_RECORD_LAYOUT_AXIS_RESCALE_Y,         ParseRecordLayoutAxisRescaleY, ""},
     {0, "AXIS_RESCALE_Z",          4, TOKEN_RECORD_LAYOUT_AXIS_RESCALE_Z,         ParseRecordLayoutAxisRescaleZ, ""},
     {0, "NO_AXIS_PTS_X",           2, TOKEN_RECORD_LAYOUT_NO_AXIS_PTS_X,          ParseRecordLayoutNoAxisPtsX, ""},
     {0, "NO_AXIS_PTS_Y",           2, TOKEN_RECORD_LAYOUT_NO_AXIS_PTS_Y,          ParseRecordLayoutNoAxisPtsY, ""},
     {0, "NO_AXIS_PTS_Z",           2, TOKEN_RECORD_LAYOUT_NO_AXIS_PTS_Z,          ParseRecordLayoutNoAxisPtsZ, ""},
     {0, "NO_RESCALE_X",            2, TOKEN_RECORD_LAYOUT_NO_RESCALE_X,           ParseRecordLayoutNoRescaleX, ""},
     {0, "NO_RESCALE_Y",            2, TOKEN_RECORD_LAYOUT_NO_RESCALE_Y,           ParseRecordLayoutNoRescaleY, ""},
     {0, "NO_RESCALE_Z",            2, TOKEN_RECORD_LAYOUT_NO_RESCALE_Z,           ParseRecordLayoutNoRescaleZ, ""},
     {0, "FIX_NO_AXIS_PTS_X",       1, TOKEN_RECORD_LAYOUT_FIX_NO_AXIS_PTS_X,      ParseRecordLayoutFixNoAxisPtsX, ""},
     {0, "FIX_NO_AXIS_PTS_Y",       1, TOKEN_RECORD_LAYOUT_FIX_NO_AXIS_PTS_Y,      ParseRecordLayoutFixNoAxisPtsY, ""},
     {0, "FIX_NO_AXIS_PTS_Z",       1, TOKEN_RECORD_LAYOUT_FIX_NO_AXIS_PTS_Z,      ParseRecordLayoutFixNoAxisPtsZ, ""},
     {0, "SRC_ADDR_X",              2, TOKEN_RECORD_LAYOUT_SRC_ADDR_X,             ParseRecordLayoutSrcAddrX, ""},
     {0, "SRC_ADDR_Y",              2, TOKEN_RECORD_LAYOUT_SRC_ADDR_Y,             ParseRecordLayoutSrcAddrY, ""},
     {0, "SRC_ADDR_Z",              2, TOKEN_RECORD_LAYOUT_SRC_ADDR_Z,             ParseRecordLayoutSrcAddrZ, ""},
     {0, "RIP_ADDR_X",              2, TOKEN_RECORD_LAYOUT_RIP_ADDR_X,             ParseRecordLayoutRipAddrX, ""},
     {0, "RIP_ADDR_Y",              2, TOKEN_RECORD_LAYOUT_RIP_ADDR_Y,             ParseRecordLayoutRipAddrY, ""},
     {0, "RIP_ADDR_Z",              2, TOKEN_RECORD_LAYOUT_RIP_ADDR_Z,             ParseRecordLayoutRipAddrZ, ""},
     {0, "RIP_ADDR_W",              2, TOKEN_RECORD_LAYOUT_RIP_ADDR_W,             ParseRecordLayoutRipAddrW, ""},
     {0, "SHIFT_OP_X",              2, TOKEN_RECORD_LAYOUT_SHIFT_OP_X,             ParseRecordLayoutShiftOpX, ""},
     {0, "SHIFT_OP_Y",              2, TOKEN_RECORD_LAYOUT_SHIFT_OP_Y,             ParseRecordLayoutShiftOpY, ""},
     {0, "SHIFT_OP_Z",              2, TOKEN_RECORD_LAYOUT_SHIFT_OP_Z,             ParseRecordLayoutShiftOpZ, ""},
     {0, "OFFSET_X",                2, TOKEN_RECORD_LAYOUT_OFFSET_X,               ParseRecordLayoutOffsetX, ""},
     {0, "OFFSET_Y",                2, TOKEN_RECORD_LAYOUT_OFFSET_Y,               ParseRecordLayoutOffsetY, ""},
     {0, "OFFSET_Z",                2, TOKEN_RECORD_LAYOUT_OFFSET_Z,               ParseRecordLayoutOffsetZ, ""},
     {0, "DIST_OP_X",               2, TOKEN_RECORD_LAYOUT_DIST_OP_X,              ParseRecordLayoutDistOpX, ""},
     {0, "DIST_OP_Y",               2, TOKEN_RECORD_LAYOUT_DIST_OP_Y,              ParseRecordLayoutDistOpY, ""},
     {0, "DIST_OP_Z",               2, TOKEN_RECORD_LAYOUT_DIST_OP_Z,              ParseRecordLayoutDistOpZ, ""},
     {0, "ALIGNMENT_BYTE",          1, TOKEN_RECORD_LAYOUT_ALIGNMENT_BYTE,         ParseRecordLayoutAlignmentByte, ""},
     {0, "ALIGNMENT_WORD",          1, TOKEN_RECORD_LAYOUT_ALIGNMENT_WORD,         ParseRecordLayoutAlignmentWord, ""},
     {0, "ALIGNMENT_LONG",          1, TOKEN_RECORD_LAYOUT_ALIGNMENT_LONG,         ParseRecordLayoutAlignmentLong, ""},
     {0, "ALIGNMENT_INT64",         1, TOKEN_RECORD_LAYOUT_ALIGNMENT_INT64,        ParseRecordLayoutAlignmentInt64, ""},
     {0, "ALIGNMENT_FLOAT16_IEEE",  1, TOKEN_RECORD_LAYOUT_ALIGNMENT_FLOAT16_IEEE, ParseRecordLayoutAlignmentFloat16, ""},
     {0, "ALIGNMENT_FLOAT32_IEEE",  1, TOKEN_RECORD_LAYOUT_ALIGNMENT_FLOAT32_IEEE, ParseRecordLayoutAlignmentFloat32, ""},
     {0, "ALIGNMENT_FLOAT64_IEEE",  1, TOKEN_RECORD_LAYOUT_ALIGNMENT_FLOAT64_IEEE, ParseRecordLayoutAlignmentFloat64, ""},
     {0, "RESERVED",                2, TOKEN_RECORD_LAYOUT_RESERVED,               ParseRecordLayoutReserved, ""},
     {0, "STATIC_RECORD_LAYOUT",    0, TOKEN_RECORD_LAYOUT_STATIC_RECORD_LAYOUT,   ParseRecordLayoutStaticRecordLayout, ""},
     {0, NULL,                      0, -1,                                         NULL, ""}};

int ParseRecordLayoutBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    UNUSED(HasBeginKeyWord);
    ASAP2_RECORD_LAYOUT* RecordLayout;
    RecordLayout = AddEmptyRecordLayoutToBuffer (Parser);
    if (RecordLayout == NULL) return -1;
    // 1. Parse the fixed parameters
    RecordLayout->Ident = ReadIdentFromFile_Buffered (Parser);
#ifdef DEBUG_PRINTF
    printf ("RECORD_LAYOUT: %s\n", RecordLayout->Ident);
#endif
    if (CheckParserError (Parser)) return -1; 
    while (GetNextOptionalParameter (Parser, RecordLayoutKeywordTable, (void*)RecordLayout) == 1);
    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}

//-------------------------------------------------------------------
// MODULE
//-------------------------------------------------------------------
#define TOKEN_MODULE_A2ML                2000001
#define TOKEN_MODULE_MOD_PAR             2000002
#define TOKEN_MODULE_MOD_COMMON          2000003 
#define TOKEN_MODULE_IF_DATA             2000004
#define TOKEN_MODULE_CHARACTERISTIC      2000005
#define TOKEN_MODULE_AXIS_PTS            2000006
#define TOKEN_MODULE_MEASUREMENT         2000007  
#define TOKEN_MODULE_COMPU_METHOD        2000008 
#define TOKEN_MODULE_COMPU_TAB           2000009   
#define TOKEN_MODULE_COMPU_VTAB          2000010
#define TOKEN_MODULE_COMPU_VTAB_RANGE    2000011
#define TOKEN_MODULE_FUNCTION            2000012
#define TOKEN_MODULE_GROUP               2000013
#define TOKEN_MODULE_RECORD_LAYOUT       2000014
#define TOKEN_MODULE_VARIANT_CODING      2000015
#define TOKEN_MODULE_FRAME               2000016
#define TOKEN_MODULE_USER_RIGHTS         2000017
#define TOKEN_MODULE_UNIT                2000018

static ASAP2_KEYWORDS ModuleKeywordTable[] =
    {{1, "A2ML",             -1, TOKEN_MODULE_A2ML,             IgnoreBlock, ""},    // /begin A2ML .... /end A2ML will be ignored
     {1, "MOD_PAR",          -1, TOKEN_MODULE_MOD_PAR,          IgnoreBlock, ""},    // /begin MOD_PAR .... /end MOD_PAR will be ignored
     {1, "MOD_COMMON",       -1, TOKEN_MODULE_MOD_COMMON,       ParseModCommonBlock, ""},
     {1, "IF_DATA",          -1, TOKEN_MODULE_IF_DATA,          ParseModuleIfDataBlock, ""},    // /begin IF_DATA .... /end IF_DATA will be ignored (exepct XCP)
     {1, "CHARACTERISTIC",   -1, TOKEN_MODULE_CHARACTERISTIC,   ParseCharacteristicBlock, ""},
     {1, "AXIS_PTS",         -1, TOKEN_MODULE_AXIS_PTS,         ParseAxisPtsBlock, ""},
     {1, "MEASUREMENT",      -1, TOKEN_MODULE_MEASUREMENT,      ParseMeasurementBlock, ""},
     {1, "COMPU_METHOD",     -1, TOKEN_MODULE_COMPU_METHOD,     ParseCompuMethodBlock, ""},
     {1, "COMPU_TAB",        -1, TOKEN_MODULE_COMPU_TAB,        ParseCompuTabBlock, ""},
     {1, "COMPU_VTAB",       -1, TOKEN_MODULE_COMPU_VTAB,       ParseCompuTabBlock, ""},
     {1, "COMPU_VTAB_RANGE", -1, TOKEN_MODULE_COMPU_VTAB_RANGE, ParseCompuTabBlock, ""},
     {1, "FUNCTION",         -1, TOKEN_MODULE_FUNCTION,         ParseFunctionBlock, ""},
     {1, "GROUP",            -1, TOKEN_MODULE_GROUP,            IgnoreBlock, ""},
     {1, "RECORD_LAYOUT",    -1, TOKEN_MODULE_RECORD_LAYOUT,    ParseRecordLayoutBlock, ""},
     {1, "VARIANT_CODING",   -1, TOKEN_MODULE_VARIANT_CODING,   IgnoreBlock, ""},
     {1, "FRAME",            -1, TOKEN_MODULE_FRAME,            IgnoreBlock, ""},
     {1, "USER_RIGHTS",      -1, TOKEN_MODULE_USER_RIGHTS,      IgnoreBlock, ""},
     {1, "UNIT",             -1, TOKEN_MODULE_UNIT,             IgnoreBlock, ""},
     {0, NULL,                0, -1,                            NULL, ""}};

int ParseModuleBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    ASAP2_MODULE_DATA* Module;
    
    Module = AddEmptyModuleToBuffer (Parser);
    if (Module == NULL) return -1;
    Module->Ident = ReadIdentFromFile_Buffered (Parser);
    if (Module->Ident == NULL) return -1;
    Module->String = ReadStringFromFile_Buffered (Parser);
    if (Module->String == NULL) return -1;
    while (GetNextOptionalParameter (Parser, ModuleKeywordTable, (void*)Module) == 1);
    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}

//-------------------------------------------------------------------
// HEADER
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// PROJECT
//-------------------------------------------------------------------
#define TOKEN_PROJECT_HEADER           1000001
#define TOKEN_PROJECT_MODULE           1000002

static ASAP2_KEYWORDS ProjectKeywordTable[] =
    {{1, "HEADER", -1, TOKEN_PROJECT_HEADER, IgnoreBlock, ""},
     {1, "MODULE", -1, TOKEN_PROJECT_MODULE, ParseModuleBlock, ""},
     {0, NULL,      0, -1,                   NULL, ""}};

int ParseProjectBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    char *ProjektIdent;
    char *ProjektString;

    ProjektIdent = ReadIdentFromFile_Buffered (Parser);
    if (ProjektIdent == NULL) return -1;
    ProjektString = ReadStringFromFile_Buffered (Parser);
    if (ProjektString == NULL) return -1;
    while (GetNextOptionalParameter (Parser, ProjectKeywordTable, (void*)NULL) == 1);
    if CheckParserError(Parser) return -1;

    return CheckEndKeyWord(Parser, KeywordTableEntry, HasBeginKeyWord);
}

//-------------------------------------------------------------------
// Root
//-------------------------------------------------------------------
#define TOKEN_ROOT_ASAP2_VERSION    0x10000000
#define TOKEN_ROOT_A2ML_VERSION     0x20000000
#define TOKEN_ROOT_PROJECT          0x30000000

int AsapVersionBlock (struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord)
{
    UNUSED(Data);
    UNUSED(KeywordTableEntry);
    UNUSED(HasBeginKeyWord);
    int Major, Minor;
    Major = ReadIntFromFile (Parser);
    Minor = ReadIntFromFile (Parser);
    UNUSED(Major);
    UNUSED(Minor);
    return CheckParserError (Parser);
}

static ASAP2_KEYWORDS RootKeywordTable[] =
    {{0, "ASAP2_VERSION", 2, TOKEN_ROOT_ASAP2_VERSION, AsapVersionBlock, ""},
     {0, "A2ML_VERSION",  2, TOKEN_ROOT_A2ML_VERSION,  IgnoreBlock, ""},
     {1, "PROJECT",      -1, TOKEN_ROOT_PROJECT,       ParseProjectBlock, ""},
     {0, NULL,            0, -1,                       NULL, ""}};

int ParseRootBlock (struct ASAP2_PARSER_STRUCT *Parser)
{
    while (GetNextOptionalParameter (Parser, RootKeywordTable, (void*)NULL) == 1);
    return CheckParserError (Parser);
}



