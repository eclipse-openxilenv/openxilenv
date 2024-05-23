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
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "A2LParser.h"
#include "A2LBuffer.h"
#include "A2LCache.h"
#include "A2LTokenizer.h"

#define MAX_INCLUDE_DEPTH   10

#define UNUSED(x) (void)(x)

static int IsSpace (int Char)
{
    return ((Char == ' ') || (Char == '\t') || (Char == '\n') || (Char == '\r'));
}


// Open the a2l file, create a parser object and init. it
ASAP2_PARSER *InitASAP2Parser (const char *Filename, int Flags)
{
    ASAP2_PARSER *Parser = NULL;

    Parser = (ASAP2_PARSER*)my_malloc (sizeof(ASAP2_PARSER));
    if (Parser != NULL) {
        memset (Parser, 0, sizeof(ASAP2_PARSER));
        Parser->Flags = Flags;
        Parser->LineNumber = 1;
        Parser->MaxStringSize = 1024;
        Parser->LastString = my_malloc (Parser->MaxStringSize);
        if (Parser->LastString != NULL) {
            Parser->Cache = LoadFileToNewCache(Filename);
            if (Parser->Cache != NULL) {
                // alles OK
                if (AddOneStringBuffer (&Parser->Data)) return NULL; 
                Parser->Filename = AddStringToBuffer (&Parser->Data, Filename);
                Parser->State = PARSER_STATE_PARSE;
            } else {
                my_free (Parser->LastString);
                my_free (Parser);
                Parser = NULL;
            }
        } else {
            my_free (Parser);
            Parser = NULL;
        }
    }
    return Parser;
}

// Read a char from cached a2l file
static int GetNextCharFromFile (ASAP2_PARSER *Parser)
{
    return CacheGetc(Parser->Cache);
}

// Put back a character to the a2l file cache
static int UngetCharToFile (ASAP2_PARSER *Parser, int Char)
{
    UNUSED(Char);
    CacheUnGetChar(Parser->Cache);
    return 0;
}

// overlook all chars till an end of comment "*/"
int RemoveComment (ASAP2_PARSER *Parser)
{
    int Char, LastChar;

    Char = -1;
    do {
        LastChar = Char;
        Char = GetNextCharFromFile (Parser);
        if (Char == EOF) {
            ThrowParserError (Parser, __FILE__, __LINE__, "File ends inside a commentar \"/*\" the \"*/\" is missing");
            return -1;
        } 
    } while ((Char != '/') || (LastChar != '*'));
    return 0;
}

// overlook all chars till an end of line
void RemoveComment2(ASAP2_PARSER *Parser)
{
    int Char;

    do {
        Char = GetNextCharFromFile(Parser);
    } while ((Char != '\n') && (Char != '\r'));
}

// overlook all all whitespaces
static int RemoveWhitespace (ASAP2_PARSER *Parser)
{
    int Char, Char2;

    do {
        Char = GetNextCharFromFile (Parser);
        if (Char == '/') {
            Char2 = GetNextCharFromFile (Parser);
            if (Char2 == '*') {
                if (RemoveComment(Parser)) {
                    return -1;
                }
                Char = GetNextCharFromFile(Parser);
            } else if(Char2 == '/') {
                RemoveComment2(Parser);
                Char = GetNextCharFromFile(Parser);
            } else {
                UngetCharToFile (Parser, Char2);
            }
        } 
    } while (IsSpace (Char));
    UngetCharToFile (Parser, Char);
    return 0;
}

// Read a string witch ends with a whitespace or an comment
// If the flaf 'BufferedFlag' are set the result will be stored inside a own buffer
// If 'BufferedFlag' is not set the result will be overwritten be the next call
char* ParseNextString (ASAP2_PARSER *Parser, int BufferedFlag)
{
    int Char, LastChar, LastLastChar;
    int CharCounter;
    int QuotedString;
    char *DstPtr;
    STRING_BUFFER *Buffer;
    int Rest;
    int LineNr;

    if (Parser->State != PARSER_STATE_PARSE) {
        return NULL;
    }
    RemoveWhitespace (Parser);
    CharCounter = 0;
    QuotedString = 0;
    Char = LastChar = LastLastChar = -1;
    DstPtr = Parser->LastString;

    Parser->CurrentOffsetStartWord = CacheTell(Parser->Cache, &LineNr);

    // Add a buffer
    if (BufferedFlag) {
        Buffer = &Parser->Data.StringBuffers[Parser->Data.StringBufferCounter - 1];
        Rest = Buffer->Size - Buffer->Pointer;
    }
    do {
        LastLastChar = LastChar;
        LastChar = Char;
        Char = GetNextCharFromFile (Parser);
        switch (Char) {
        case EOF:
            Parser->State = PARSER_STATE_EOF;
            goto END_OF_STRING;
            break;
        case '"':
            if ((CharCounter == 0) && (LastChar != '"')) {
                QuotedString = 1;
                continue;
            } else if (((LastLastChar == '\\') && (LastChar == '\\')) || (LastChar != '\\')) {
                // End of the strings!
                goto END_OF_STRING;
            }
            break;
        case '*':
            if (!QuotedString) {
                if (LastChar == '/') {   // it is a comment "/*" start
                    DstPtr--;
                    if (RemoveComment(Parser)) {
                        return NULL;
                    }
                    if (CharCounter) goto END_OF_STRING;
                }
            }
            break;
        case '/':
            if (!QuotedString) {
                if (LastChar == '/') {   // it is a comment "//" start
                    DstPtr--;
                    RemoveComment2(Parser);
                    if (CharCounter) goto END_OF_STRING;
#if 0
                } else {
                    // because of the speciale case that the strings /begin or /end are without whitespaces before or behind
                    int TempLineNr;
                    int CurrentOffset = CacheTell(Parser->Cache, &TempLineNr);
                    switch (GetNextCharFromFile (Parser)) {
                    case 'b':
                        if (GetNextCharFromFile (Parser) == 'e') {
                            if (GetNextCharFromFile (Parser) == 'g') {
                                if (GetNextCharFromFile (Parser) == 'i') {
                                    if (GetNextCharFromFile (Parser) == 'n') {
                                        if (CharCounter) {
                                            // direct following /begin without whitespaces
                                            CacheSeek(Parser->Cache, CurrentOffset - 1, SEEK_SET, TempLineNr);
                                            goto END_OF_STRING;
                                        } else {
                                            if (BufferedFlag) {
                                                ThrowParserError (Parser, __FILE__, __LINE__, "/begin should not be buffered");
                                                return NULL;
                                            } else {
                                                // we have detected a /begin
                                                strcpy(DstPtr, "/begin");
                                                CharCounter = 6;
                                                DstPtr += 6;
                                                goto END_OF_STRING;
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        case 'e':
                            if (GetNextCharFromFile (Parser) == 'n') {
                                if (GetNextCharFromFile (Parser) == 'd') {
                                    if (CharCounter) {
                                        // direct following /end without whitespaces
                                        CacheSeek(Parser->Cache, CurrentOffset - 1, SEEK_SET, TempLineNr);
                                        goto END_OF_STRING;
                                    } else {
                                        if (BufferedFlag) {
                                            ThrowParserError (Parser, __FILE__, __LINE__, "/end should not be buffered");
                                            return NULL;
                                        } else {
                                            // we have detected a /end
                                            strcpy(DstPtr, "/end");
                                            CharCounter = 4;
                                            DstPtr += 4;
                                            goto END_OF_STRING;
                                        }
                                    }
                                }
                            }
                            break;
                        default:
                            break;
                        }
                        CacheSeek(Parser->Cache, CurrentOffset, SEEK_SET, TempLineNr);
                    }
#endif
                }
            }
            break;
        case '\r':
            continue;
        case '\n':
            if (!QuotedString) {
                goto END_OF_STRING;
            }
            break;
        default:
            if (!QuotedString && IsSpace (Char)) {
                goto END_OF_STRING;
            }
            break;
        }
        // valid character, add it
        CharCounter++;
        if (CharCounter >= Parser->MaxStringSize) {
            int Diff = (int)(DstPtr - Parser->LastString);
            Parser->MaxStringSize += 1024;
            Parser->LastString = my_realloc(Parser->LastString, Parser->MaxStringSize);
            if (Parser->LastString == NULL) {
                ThrowParserError(Parser, __FILE__, __LINE__, "out of memory");
                return NULL;
            }
            DstPtr = Parser->LastString + Diff;
        }
        *DstPtr++ = (char)Char;
        if (BufferedFlag) {
            // If the current buffer to small?
            if (CharCounter > Rest) { 
                char *dst, *src; 
                src = Buffer->Data + Buffer->Pointer; 
                // Add new buffer
                if (AddOneStringBuffer (&Parser->Data)) {
                    ThrowParserError (Parser, __FILE__, __LINE__, "out of memory");
                    return NULL; 
                }
                // select new buffer
                Buffer = &Parser->Data.StringBuffers[Parser->Data.StringBufferCounter - 1]; 
                dst = Buffer->Data + Buffer->Pointer; 
                // and copy before readed characters from old to new buffer
                if (CharCounter) MEMCPY (dst, src, CharCounter - 1); 
                Rest = Buffer->Size - Buffer->Pointer; 
                if (CharCounter >= Buffer->Size) {
                    ThrowParserError (Parser, __FILE__, __LINE__, "out of memory");
                    return NULL;  
                }
            } 
            Buffer->Data[Buffer->Pointer + CharCounter - 1] = Char; 
        }
    } while (1);
END_OF_STRING:
    *DstPtr = '\0';
    if (BufferedFlag) {
        // If the current buffer to small?
        if (CharCounter >= Rest) { 
            char *dst, *src; 
            src = Buffer->Data + Buffer->Pointer; 
            // Add new buffer
            if (AddOneStringBuffer (&Parser->Data)) {
                ThrowParserError (Parser, __FILE__, __LINE__, "out of memory");
                return NULL; 
            }
            // select new buffer
            Buffer = &Parser->Data.StringBuffers[Parser->Data.StringBufferCounter - 1]; 
            dst = Buffer->Data + Buffer->Pointer;
            // and copy before readed characters from old to new buffer
            if (CharCounter) MEMCPY (dst, src, CharCounter);
            Rest = Buffer->Size - Buffer->Pointer; 
            if (CharCounter >= Buffer->Size) {
                ThrowParserError (Parser, __FILE__, __LINE__, "out of memory");
                return NULL;  
            }
        } 
        Buffer->Data[Buffer->Pointer + CharCounter] = '\0'; 
        CharCounter++;
        DstPtr = Buffer->Data + Buffer->Pointer;
        Buffer->Pointer += CharCounter;
        return DstPtr;
    }
    return Parser->LastString; 
}


// This will return TRUE if we are at the file end
int IsEndOfParsing (ASAP2_PARSER *Parser)
{
    return (Parser->State != PARSER_STATE_PARSE);
}

