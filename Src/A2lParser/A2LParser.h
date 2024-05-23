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


#ifndef __A2LPARSER_H
#define __A2LPARSER_H

#include "A2LData.h"
#include "A2LCache.h"

#define MAX_INCLUDE_DEPTH   10

struct ASAP2_KEYWORDS_STRUCT;
struct ASAP2_PARSER_STRUCT;

// Funktions-Pointer: Zeigt auf eine Funktion zum Parsen eines Blocks
typedef int (*PARSE_BLOCK_FUNC) (struct ASAP2_PARSER_STRUCT *Parser, void *Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord);


typedef struct ASAP2_KEYWORDS_STRUCT {
    int BeginEndToken;
    char *KeywordString;
    int ParameterCount;    // -1 beliebig viele
    int Token;
    PARSE_BLOCK_FUNC ParseBlockFunc;
    char Filler[32-20];
} ASAP2_KEYWORDS;

typedef struct ASAP2_PARSER_STRUCT {
    FILE_CACHE *Cache;
    char *Filename;
    char *LastString;
    int MaxStringSize;
    int State;
#define PARSER_STATE_ERROR  -1
#define PARSER_STATE_IDLE    0
#define PARSER_STATE_PARSE   1
#define PARSER_STATE_EOF     2
    char ErrorString[1024];
    char *SourceFile;
    int LineNumber;

    int CurrentOffsetStartWord;
    ASAP2_DATA Data;
#define A2L_PARSER_TRY_TO_LOAD_IF_DATA_XCP      0x1
    int Flags;
} ASAP2_PARSER;


int ParseRootBlock (struct ASAP2_PARSER_STRUCT *Parser);

int GetNextOptionalParameter(struct ASAP2_PARSER_STRUCT *Parser, ASAP2_KEYWORDS* KeywordTable, void* Data);
int CheckEndKeyWord(struct ASAP2_PARSER_STRUCT *Parser, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord);

int IgnoreIfDataBlock(struct ASAP2_PARSER_STRUCT *Parser);
int IgnoreBlock(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord);

int ThrowParserError(struct ASAP2_PARSER_STRUCT *Parser, char *SourceFile, int LineNumber, char *ErrorString, ...);
int GetParserErrorString (struct ASAP2_PARSER_STRUCT *Parser, char *ErrString, int MaxSize);

#define CheckParserError(Parser) (Parser->State != PARSER_STATE_PARSE)

void FreeParser (struct ASAP2_PARSER_STRUCT *Parser);

//  from A2LParserModuleIfData.c
int ParseModuleIfDataBlock(struct ASAP2_PARSER_STRUCT *Parser, void* Data, struct ASAP2_KEYWORDS_STRUCT *KeywordTableEntry, int HasBeginKeyWord);

#endif
