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


#ifndef __A2LBUFFER
#define __A2LBUFFER

#include "A2LData.h"

struct ASAP2_PARSER_STRUCT;

// alloziert einen neues STRING_BUFFER_BLOCK_SIZE grossen Speicherblock 
// fuer den privaten Speicher-Pool
int AddOneStringBuffer (ASAP2_DATA *p);

// liest ein Keyword vom A2L-File (ohne dies zu puffern)
char *ReadKeyWordFromFile (struct ASAP2_PARSER_STRUCT *Parser);

// liest einen String vom A2L-File und puffert dieses im privaten Speicher-Pool
char* ReadStringFromFile_Buffered (struct ASAP2_PARSER_STRUCT *Parser);

// liest einen Ident-String vom A2L-File und puffert dieses im privaten Speicher-Pool
char* ReadIdentFromFile_Buffered (struct ASAP2_PARSER_STRUCT *Parser);

// liest einen ENUM-String vom A2L-File und sucht den String im 'EnumDesc'-String
// und liefert den entsprechenden Wert zurueck.
// Der ENUM_String hat fogendes Format:
// EnumString1 Wert1 EnumString2 Wert2 ... EnumStringN WertN
// Beispiel: OFF 0 ON 1 ? -1
int ReadEnumFromFile (struct ASAP2_PARSER_STRUCT *Parser, char *EnumDesc);

// liest einen String vom A2L-File und konvertiert diesen in einen Integer 
int ReadIntFromFile (struct ASAP2_PARSER_STRUCT *Parser);
int TryReadIntFromFile(struct ASAP2_PARSER_STRUCT *Parser, int DefaultValue);

// liest einen String vom A2L-File und konvertiert diesen in einen 'unsigned' Integer 
unsigned int ReadUIntFromFile (struct ASAP2_PARSER_STRUCT *Parser);
uint64_t ReadUInt64FromFile(struct ASAP2_PARSER_STRUCT *Parser);

// liest einen String vom A2L-File und konvertiert diesen in einen Float
double ReadFloatFromFile (struct ASAP2_PARSER_STRUCT *Parser);


// alloziert einen Speicherblock aus dem privaten Speicher-Pool
void *AllocMemFromBuffer (ASAP2_DATA *p, int Size);

// alloziert einen Speicherblock aus dem privaten Speicher-Pool
// mit der Laenge eines Strings und kopiert diesen dann dort hinein
char *AddStringToBuffer (ASAP2_DATA *p, const char *String);

// alloziert einen Speicherblock fuer ein MODULE aus dem privaten Speicher-Pool
ASAP2_MODULE_DATA* AddEmptyModuleToBuffer (struct ASAP2_PARSER_STRUCT *Parser);

// alloziert einen Speicherblock fuer ein MEASUREMENT aus dem privaten Speicher-Pool
ASAP2_MEASUREMENT* AddEmptyMeasurementToBuffer (struct ASAP2_PARSER_STRUCT *Parser);

// alloziert einen Speicherblock fuer ein CHARACTERISTIC_AXIS_DESCR aus dem privaten Speicher-Pool
ASAP2_CHARACTERISTIC_AXIS_DESCR* AddEmptyAxisDescrToBuffer (struct ASAP2_PARSER_STRUCT *Parser);

// alloziert einen Speicherblock fuer ein CHARACTERISTIC aus dem privaten Speicher-Pool
ASAP2_CHARACTERISTIC* AddEmptyCharacteristicToBuffer (struct ASAP2_PARSER_STRUCT *Parser);

// alloziert einen Speicherblock fuer ein AXIS_PTS aus dem privaten Speicher-Pool
ASAP2_AXIS_PTS* AddEmptyAxisPtsToBuffer (struct ASAP2_PARSER_STRUCT *Parser);

ASAP2_IF_DATA_CANAPE_EXT* AddEmptyCanapeExtToBuffer(struct ASAP2_PARSER_STRUCT *Parser);

// alloziert einen Speicherblock fuer ein RECORD_LAYOUT aus dem privaten Speicher-Pool
ASAP2_RECORD_LAYOUT* AddEmptyRecordLayoutToBuffer (struct ASAP2_PARSER_STRUCT *Parser);

// alloziert einen Speicherblock fuer ein COMPU_METHOD aus dem privaten Speicher-Pool
ASAP2_COMPU_METHOD* AddEmptyCompuMethodToBuffer (struct ASAP2_PARSER_STRUCT *Parser);

// alloziert einen Speicherblock fuer ein COMPU_TAB aus dem privaten Speicher-Pool
ASAP2_COMPU_TAB* AddEmptyCompuTabToBuffer (struct ASAP2_PARSER_STRUCT *Parser);

// alloziert einen Speicherblock fuer ein FUNCTION aus dem privaten Speicher-Pool
ASAP2_FUNCTION* AddEmptyFunctionToBuffer (struct ASAP2_PARSER_STRUCT *Parser);

// sortiert alle gelesenen Daten alphabetisch
int SortData (ASAP2_DATA *p);


#endif
