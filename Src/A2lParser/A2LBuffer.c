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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "A2LParser.h"
#include "A2LTokenizer.h"
#include "A2LBuffer.h"

// Allocate a new memory block with size of STRING_BUFFER_BLOCK_SIZE
// inside the private memory pool
int AddOneStringBuffer (ASAP2_DATA *p)
{
    int Pos = p->StringBufferCounter;
    p->StringBufferCounter++;
    p->StringBuffers = (STRING_BUFFER*)my_realloc (p->StringBuffers, sizeof (STRING_BUFFER) * p->StringBufferCounter);
    if (p->StringBuffers == NULL) return -1;
    p->StringBuffers[Pos].Data = my_malloc (STRING_BUFFER_BLOCK_SIZE);
    if (p->StringBuffers[Pos].Data == NULL) return -1;
    p->StringBuffers[Pos].Pointer = 0;
    p->StringBuffers[Pos].Size = STRING_BUFFER_BLOCK_SIZE;
    return 0;
}

// Allocate a new memory block inside the private memory pool
void *AllocMemFromBuffer (ASAP2_DATA *p, int Size)
{
    int Rest;
    void *Ret = NULL;

    if (p->StringBufferCounter == 0) {
        if (AddOneStringBuffer (p)) return NULL;
    }
    Rest = p->StringBuffers[p->StringBufferCounter-1].Size - p->StringBuffers[p->StringBufferCounter-1].Pointer;
    if (Rest >= Size) {  // passt noch rein
        Ret = p->StringBuffers[p->StringBufferCounter-1].Data + p->StringBuffers[p->StringBufferCounter-1].Pointer;
        p->StringBuffers[p->StringBufferCounter-1].Pointer += Size;
    } else {  // neuer Block
        if (AddOneStringBuffer (p)) return NULL;
        Rest = p->StringBuffers[p->StringBufferCounter-1].Size - p->StringBuffers[p->StringBufferCounter-1].Pointer;
        if (Rest >= Size) {  // Past noch rein
            Ret = p->StringBuffers[p->StringBufferCounter-1].Data + p->StringBuffers[p->StringBufferCounter-1].Pointer;
            p->StringBuffers[p->StringBufferCounter-1].Pointer += Size;
        } 
    }
    return Ret;
}

// Allocate a new memory block with size of the string legth of the parameter String
// inside the private memory pool
char *AddStringToBuffer (ASAP2_DATA *p, const char *String)
{
    int Len = (int)strlen (String) + 1;
    int Rest;
    char *Ret = NULL;

    Rest = p->StringBuffers[p->StringBufferCounter-1].Size - p->StringBuffers[p->StringBufferCounter-1].Pointer;
    if (Rest >= Len) {  // doesn't fit
        Ret = p->StringBuffers[p->StringBufferCounter-1].Data + p->StringBuffers[p->StringBufferCounter-1].Pointer;
        MEMCPY (Ret, String, Len);
        p->StringBuffers[p->StringBufferCounter-1].Pointer += Len;
    } else {  // new block
        if (AddOneStringBuffer (p)) return NULL;
        Rest = p->StringBuffers[p->StringBufferCounter-1].Size - p->StringBuffers[p->StringBufferCounter-1].Pointer;
        if (Rest >= Len) {  // if fit into the current block
            Ret = p->StringBuffers[p->StringBufferCounter-1].Data + p->StringBuffers[p->StringBufferCounter-1].Pointer;
            MEMCPY (Ret, String, Len);
            p->StringBuffers[p->StringBufferCounter-1].Pointer += Len;
        } 
    }
    return Ret;
}

// This will read a keyword from the a2l file cache without buffering
char *ReadKeyWordFromFile (struct ASAP2_PARSER_STRUCT *Parser)
{
    char *Ret;

    if (!CheckParserError(Parser)) {  // if there is set an error message before do not overwrite
        Ret = ParseNextString (Parser, 0);
    } else {
        Ret = NULL;
    }
    return Ret;
}

// This will read a string from the a2l file cache into the private memory pool
char* ReadStringFromFile_Buffered (struct ASAP2_PARSER_STRUCT *Parser)
{
    char *Ret;

    if (!CheckParserError(Parser)) {  // if there is set an error message before do not overwrite
        Ret = ParseNextString (Parser, 1);
    } else {
        Ret = NULL;
    }
    return Ret;
}

// This will read a ident string from the a2l file cache into the private memory pool
char* ReadIdentFromFile_Buffered (struct ASAP2_PARSER_STRUCT *Parser)
{
    char *Ret;

    if (!CheckParserError(Parser)) {  // if there is set an error message before do not overwrite
        Ret = ParseNextString(Parser, 1);
    } else {
        Ret = NULL;
    }
    return Ret;
}

// This will read an enum string from the a2l file cache to the 'EnumDesc' string
// and return the corresponding value back.
// The enum string has the following format:
// EnumString1 Value1 EnumString2 Value2 ... EnumStringN ValueN
// Beispiel: OFF 0 ON 1 ? -1
int ReadEnumFromFile (struct ASAP2_PARSER_STRUCT *Parser, char *EnumDesc)
{
    char *Help, *e, *p;
    int Count1, Count2;
    int Ret = -1;

    if (!CheckParserError(Parser)) {  // if there is set an error message before do not overwrite
        Help = ParseNextString(Parser, 0);
        if (Help != NULL) {
            Count1 = (int)strlen(Help);
            // nun suche in ENUMDesc
            p = e = EnumDesc;
            do {
                Count2 = 0;
                while (*p != ' ') {
                    p++;
                    Count2++;
                }
                if (*p == ' ') p++;
                Ret = strtol(p, &p, 0);   // Value
                if (*p == ' ') p++;
                if (Count1 == Count2) {
                    if (!strncmp(e, Help, Count2)) {
                        return Ret;                     // Match
                    }
                }
                e = p;
            } while (*p);
        }
        ThrowParserError(Parser, __FILE__, __LINE__, "unknown ENUM \"%s\"", (Help == NULL) ? "NULL" : Help);
    }
    return Ret;              // If no match last value is -1
}

// This will read a string from the a2l file cache and convert it to an integer value
int ReadIntFromFile (struct ASAP2_PARSER_STRUCT *Parser)
{
    char *Help, *EndPtr;
    int Ret = 0;
    if (!CheckParserError(Parser)) {  // if there is set an error message before do not overwrite
        Help = ParseNextString(Parser, 0);
        if (Help != NULL) {
            Ret = strtol(Help, &EndPtr, 0);
            if ((int64_t)strlen(Help) != (EndPtr - Help)) {
                ThrowParserError(Parser, __FILE__, __LINE__, "expecting an integer value not \"%s\"", Help);
            }
        }
    }
    return Ret;
}


int TryReadIntFromFile(struct ASAP2_PARSER_STRUCT *Parser, int DefaultValue)
{
    char *Help, *EndPtr;
    int Ret = DefaultValue;
    if (!CheckParserError(Parser)) {  // if there is set an error message before do not overwrite
        int Offset, LineNr;
        Offset = CacheTell(Parser->Cache, &LineNr);
        Help = ParseNextString(Parser, 0);
        if (Help != NULL) {
            Ret = strtol(Help, &EndPtr, 0);
            if ((int64_t)strlen(Help) != (EndPtr - Help)) {
                CacheSeek(Parser->Cache, Offset, SEEK_SET, LineNr);
                return DefaultValue;
            }
        }
    }
    return Ret;
}


// This will read a string from the a2l file cache and convert it to an unsigned integer value
unsigned int ReadUIntFromFile (struct ASAP2_PARSER_STRUCT *Parser)
{
    char *Help, *EndPtr;
    unsigned int Ret = 0;

    if (!CheckParserError(Parser)) {  // if there is set an error message before do not overwrite
        Help = ParseNextString(Parser, 0);
        if (Help != NULL) {
            Ret = strtoul(Help, &EndPtr, 0);
            if ((int64_t)strlen(Help) != (EndPtr - Help)) {
                ThrowParserError(Parser, __FILE__, __LINE__, "expecting an integer value");
            }
        }
    }
    return Ret;
}

// This will read a string from the a2l file cache and convert it to an unsigned 64 bit integer value
uint64_t ReadUInt64FromFile(struct ASAP2_PARSER_STRUCT *Parser)
{
    char *Help, *EndPtr;
    uint64_t Ret = 0;

    if (!CheckParserError(Parser)) {  // if there is set an error message before do not overwrite
        Help = ParseNextString(Parser, 0);
        if (Help != NULL) {
            Ret = strtoull(Help, &EndPtr, 0);
            if ((int64_t)strlen(Help) != (EndPtr - Help)) {
                ThrowParserError(Parser, __FILE__, __LINE__, "expecting an integer value");
            }
        }
    }
    return Ret;
}

// This will read a string from the a2l file cache and convert it to a double integer value
double ReadFloatFromFile (struct ASAP2_PARSER_STRUCT *Parser)
{
    char *Help, *EndPtr;
    double Ret = 0.0;

    if (!CheckParserError(Parser)) {  // if there is set an error message before do not overwrite
        Help = ParseNextString(Parser, 0);
        if (Help != NULL) {
            Ret = strtod(Help, &EndPtr);
            if ((Ret <= 0.0) && (Ret >= -0.0)) Ret = 0.0; // avoid -0.0
            if ((int)strlen(Help) != (EndPtr - Help)) {
                ThrowParserError(Parser, __FILE__, __LINE__, "expecting an float value");
            }
        }
    }
    return Ret;
}

// Alloc a memory block for one MODULE inside the private memory pool
ASAP2_MODULE_DATA* AddEmptyModuleToBuffer (struct ASAP2_PARSER_STRUCT *Parser)
{
    ASAP2_MODULE_DATA* Ret;
    Ret = (ASAP2_MODULE_DATA*)AllocMemFromBuffer (&Parser->Data, sizeof (ASAP2_MODULE_DATA));
    if (Ret != NULL) {
        STRUCT_ZERO_INIT (*Ret, ASAP2_MODULE_DATA);
        Ret->StructType = BUFFER_STRUCT_TYPE_MODULE_DATA;
        Parser->Data.ModuleCounter++;
        if (Parser->Data.ModuleCounter >= Parser->Data.ModuleSize) {
            Parser->Data.ModuleSize += 10;   // 10 more
            Parser->Data.Modules = (ASAP2_MODULE_DATA**) my_realloc (Parser->Data.Modules, Parser->Data.ModuleSize * sizeof (ASAP2_MODULE_DATA*));
            if (Parser->Data.Modules == NULL) {
                ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
                return NULL;
            }
        } 
        Parser->Data.Modules[Parser->Data.ModuleCounter - 1] = Ret;
    }
    return Ret;
}


// Alloc a memory block for a MEASUREMENT inside the private memory pool
ASAP2_MEASUREMENT* AddEmptyMeasurementToBuffer (struct ASAP2_PARSER_STRUCT *Parser)
{
    ASAP2_MEASUREMENT* Ret;
    ASAP2_MODULE_DATA *Module;

    Ret = (ASAP2_MEASUREMENT*)AllocMemFromBuffer (&Parser->Data, sizeof (ASAP2_MEASUREMENT));
    if (Ret != NULL) {
        STRUCT_ZERO_INIT (*Ret, ASAP2_MEASUREMENT);
        Ret->StructType = BUFFER_STRUCT_TYPE_MEASUREMENT;
        Module = Parser->Data.Modules[Parser->Data.ModuleCounter - 1];
        Module->MeasurementCounter++;
        if (Module->MeasurementCounter >= Module->MeasurementSize) {
            Module->MeasurementSize += 100;   // 100 moe
            Module->Measurements = (ASAP2_MEASUREMENT**) my_realloc (Module->Measurements, Module->MeasurementSize * sizeof (ASAP2_MEASUREMENT*));
            if (Module->Measurements == NULL) {
                ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
                return NULL;
            }
        } 
        Module->Measurements[Module->MeasurementCounter - 1] = Ret;
    }
    return Ret;
}

// Alloc a memory block for a CHARACTERISTIC_AXIS_DESCR inside the private memory pool
ASAP2_CHARACTERISTIC_AXIS_DESCR* AddEmptyAxisDescrToBuffer (struct ASAP2_PARSER_STRUCT *Parser)
{
    ASAP2_CHARACTERISTIC_AXIS_DESCR* Ret;
    ASAP2_MODULE_DATA *Module;

    Ret = (ASAP2_CHARACTERISTIC_AXIS_DESCR*)AllocMemFromBuffer (&Parser->Data, sizeof (ASAP2_CHARACTERISTIC_AXIS_DESCR));
    if (Ret != NULL) {
        STRUCT_ZERO_INIT (*Ret, ASAP2_CHARACTERISTIC_AXIS_DESCR);
        Module = Parser->Data.Modules[Parser->Data.ModuleCounter - 1];
        Module->AxisDescrCounter++;
        if (Module->AxisDescrCounter >= Module->AxisDescrSize) {
            Module->AxisDescrSize += 100;   // 100 more
            Module->AxisDescrs = (ASAP2_CHARACTERISTIC_AXIS_DESCR**) my_realloc (Module->AxisDescrs, Module->AxisDescrSize * sizeof (ASAP2_CHARACTERISTIC_AXIS_DESCR*));
            if (Module->AxisDescrs == NULL) {
                return NULL;
            }
        } 
        Module->AxisDescrs[Module->AxisDescrCounter - 1] = Ret;
    }
    return Ret;
}

// Alloc a memory block for a CHARACTERISTIC inside the private memory pool
ASAP2_CHARACTERISTIC* AddEmptyCharacteristicToBuffer (struct ASAP2_PARSER_STRUCT *Parser)
{
    ASAP2_CHARACTERISTIC* Ret;
    ASAP2_MODULE_DATA *Module;

    Ret = (ASAP2_CHARACTERISTIC*)AllocMemFromBuffer (&Parser->Data, sizeof (ASAP2_CHARACTERISTIC));
    if (Ret != NULL) {
        STRUCT_ZERO_INIT (*Ret, ASAP2_CHARACTERISTIC);
        Ret->StructType = BUFFER_STRUCT_TYPE_CHARACTERISTIC;
        Module = Parser->Data.Modules[Parser->Data.ModuleCounter - 1];
        Module->CharacteristicCounter++;
        if (Module->CharacteristicCounter >= Module->CharacteristicSize) {
            Module->CharacteristicSize += 100;   // 100 moe
            Module->Characteristics = (ASAP2_CHARACTERISTIC**) my_realloc (Module->Characteristics, Module->CharacteristicSize * sizeof (ASAP2_CHARACTERISTIC*));
            if (Module->Characteristics == NULL) {
                return NULL;
            }
        } 
        Module->Characteristics[Module->CharacteristicCounter - 1] = Ret;
    }
    return Ret;
}

// Alloc a memory block for a AXIS_PTS inside the private memory pool
ASAP2_AXIS_PTS* AddEmptyAxisPtsToBuffer (struct ASAP2_PARSER_STRUCT *Parser)
{
    ASAP2_AXIS_PTS* Ret;
    ASAP2_MODULE_DATA *Module;

    Ret = (ASAP2_AXIS_PTS*)AllocMemFromBuffer (&Parser->Data, sizeof (ASAP2_AXIS_PTS));
    if (Ret != NULL) {
        STRUCT_ZERO_INIT (*Ret, ASAP2_AXIS_PTS);
        Ret->StructType = BUFFER_STRUCT_TYPE_AXIS_PTS;
        Module = Parser->Data.Modules[Parser->Data.ModuleCounter - 1];
        Module->AxisPtsCounter++;
        if (Module->AxisPtsCounter >= Module->AxisPtsSize) {
            Module->AxisPtsSize += 100;   // 100 moe
            Module->AxisPtss = (ASAP2_AXIS_PTS**) my_realloc (Module->AxisPtss, Module->AxisPtsSize * sizeof (ASAP2_AXIS_PTS*));
            if (Module->AxisPtss == NULL) {
                return NULL;
            }
        } 
        Module->AxisPtss[Module->AxisPtsCounter - 1] = Ret;
    }
    return Ret;
}

// Alloc a memory block for a IF_DATA_CANAPE_EXT inside the private memory pool
ASAP2_IF_DATA_CANAPE_EXT* AddEmptyCanapeExtToBuffer(struct ASAP2_PARSER_STRUCT *Parser)
{
    ASAP2_IF_DATA_CANAPE_EXT* Ret;

    Ret = (ASAP2_IF_DATA_CANAPE_EXT*)AllocMemFromBuffer(&Parser->Data, sizeof(ASAP2_IF_DATA_CANAPE_EXT));
    if (Ret != NULL) {
        STRUCT_ZERO_INIT(*Ret, ASAP2_IF_DATA_CANAPE_EXT);
    }
    return Ret;
}


// Alloc a memory block for a RECORD_LAYOUT inside the private memory pool
ASAP2_RECORD_LAYOUT* AddEmptyRecordLayoutToBuffer (struct ASAP2_PARSER_STRUCT *Parser)
{
    ASAP2_RECORD_LAYOUT* Ret;
    ASAP2_MODULE_DATA *Module;

    Ret = (ASAP2_RECORD_LAYOUT*)AllocMemFromBuffer (&Parser->Data, sizeof (ASAP2_RECORD_LAYOUT));
    if (Ret != NULL) {
        STRUCT_ZERO_INIT (*Ret, ASAP2_RECORD_LAYOUT);
        Ret->StructType = BUFFER_STRUCT_TYPE_RECORD_LAYOUT;
        Module = Parser->Data.Modules[Parser->Data.ModuleCounter - 1];
        Module->RecordLayoutCounter++;
        if (Module->RecordLayoutCounter >= Module->RecordLayoutSize) {
            Module->RecordLayoutSize += 10;   // 10 more
            Module->RecordLayouts = (ASAP2_RECORD_LAYOUT**) my_realloc (Module->RecordLayouts, Module->RecordLayoutSize * sizeof (ASAP2_RECORD_LAYOUT*));
            if (Module->RecordLayouts == NULL) {
                return NULL;
            }
        } 
        Module->RecordLayouts[Module->RecordLayoutCounter - 1] = Ret;
    }
    return Ret;
}

// alloc a memory block for a COMPU_METHOD inside the private memory Pool
ASAP2_COMPU_METHOD* AddEmptyCompuMethodToBuffer (struct ASAP2_PARSER_STRUCT *Parser)
{
    ASAP2_COMPU_METHOD* Ret;
    ASAP2_MODULE_DATA *Module;

    Ret = (ASAP2_COMPU_METHOD*)AllocMemFromBuffer (&Parser->Data, sizeof (ASAP2_COMPU_METHOD));
    if (Ret != NULL) {
        STRUCT_ZERO_INIT (*Ret, ASAP2_COMPU_METHOD);
        Ret->StructType = BUFFER_STRUCT_TYPE_COMPU_METHOD;
        Module = Parser->Data.Modules[Parser->Data.ModuleCounter - 1];
        Module->CompuMethodCounter++;
        if (Module->CompuMethodCounter >= Module->CompuMethodSize) {
            Module->CompuMethodSize += 100;   // 10 more
            Module->CompuMethods = (ASAP2_COMPU_METHOD**) my_realloc (Module->CompuMethods, Module->CompuMethodSize * sizeof (ASAP2_COMPU_METHOD*));
            if (Module->CompuMethods == NULL) {
                return NULL;
            }
        } 
        Module->CompuMethods[Module->CompuMethodCounter - 1] = Ret;
    }
    return Ret;
}

// alloc a memory block for a COMPU_COMPU_TAB inside the private memory Pool
ASAP2_COMPU_TAB* AddEmptyCompuTabToBuffer (struct ASAP2_PARSER_STRUCT *Parser)
{
    ASAP2_COMPU_TAB* Ret;
    ASAP2_MODULE_DATA *Module;

    Ret = (ASAP2_COMPU_TAB*)AllocMemFromBuffer (&Parser->Data, sizeof (ASAP2_COMPU_TAB));
    if (Ret != NULL) {
        STRUCT_ZERO_INIT (*Ret, ASAP2_COMPU_TAB);
        Ret->StructType = BUFFER_STRUCT_TYPE_COMPU_TAB;
        Module = Parser->Data.Modules[Parser->Data.ModuleCounter - 1];
        Module->CompuTabCounter++;
        if (Module->CompuTabCounter >= Module->CompuTabSize) {
            Module->CompuTabSize += 100;   // 10 more
            Module->CompuTabs = (ASAP2_COMPU_TAB**) my_realloc (Module->CompuTabs, Module->CompuTabSize * sizeof (ASAP2_COMPU_TAB*));
            if (Module->CompuTabs == NULL) {
                return NULL;
            }
        } 
        Module->CompuTabs[Module->CompuTabCounter - 1] = Ret;
    }
    return Ret;
}

// alloc a memory block for a FUNCTION inside the private memory Pool
ASAP2_FUNCTION* AddEmptyFunctionToBuffer (struct ASAP2_PARSER_STRUCT *Parser)
{
    ASAP2_FUNCTION* Ret;
    ASAP2_MODULE_DATA *Module;

    Ret = (ASAP2_FUNCTION*)AllocMemFromBuffer (&Parser->Data, sizeof (ASAP2_FUNCTION));
    if (Ret != NULL) {
        STRUCT_ZERO_INIT (*Ret, ASAP2_FUNCTION);
        Ret->StructType = BUFFER_STRUCT_TYPE_FUNCTION;
        Module = Parser->Data.Modules[Parser->Data.ModuleCounter - 1];
        Module->FunctionCounter++;
        if (Module->FunctionCounter >= Module->FunctionSize) {
            Module->FunctionSize += 100;   // 10 more
            Module->Functions = (ASAP2_FUNCTION**) my_realloc (Module->Functions, Module->FunctionSize * sizeof (ASAP2_FUNCTION*));
            if (Module->Functions == NULL) {
                ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
                return NULL;
            }
        } 
        Module->Functions[Module->FunctionCounter - 1] = Ret;
    } else {
        ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
    }
    return Ret;
}

static int QsortCompareFunc (const void *arg1, const void *arg2)
{
    char **aa, **bb;
    char *a, *b;
    aa = *(char***)arg1;
    bb = *(char***)arg2;
    a = *aa;
    b = *bb;
    return strcmp(**(char***)arg1, **(char***)arg2);
}

// Sort all readed a2l objects alphabetical
int SortData (ASAP2_DATA *p)
{
    int x;
    ASAP2_MODULE_DATA *Module;

    for (x = 0; x < p->ModuleCounter; x++) {
        Module = p->Modules[x];
        qsort (Module->AxisPtss, Module->AxisPtsCounter, sizeof(void*), QsortCompareFunc);
        qsort (Module->Characteristics, Module->CharacteristicCounter, sizeof(void*), QsortCompareFunc);
        qsort (Module->CompuMethods, Module->CompuMethodCounter, sizeof(void*), QsortCompareFunc);
        qsort (Module->CompuTabs, Module->CompuTabCounter, sizeof(void*), QsortCompareFunc);
        qsort (Module->Measurements, Module->MeasurementCounter, sizeof(void*), QsortCompareFunc);
        qsort (Module->RecordLayouts, Module->RecordLayoutCounter, sizeof(void*), QsortCompareFunc);
        qsort (Module->Functions, Module->FunctionCounter, sizeof(void*), QsortCompareFunc);
    }
    return 0;
}
