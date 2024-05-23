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
#include <ctype.h>
#include <malloc.h>  // do not use MyMemory.h
extern "C" {
#include "Files.h"
#include "EnvironmentVariables.h"
#include "MainValues.h"
#include "BlackboardAccess.h"
}

#include "Parser.h"
#include "BaseCmd.h"
#include "Script.h"
#include "Tokenizer.h"

#define UNUSED(x) (void)(x)


int cTokenizer::GetChar (cFileCache *par_FileCache)
{
    int Char;

    Char = par_FileCache->Fgetc ();
    return Char;
}

int cTokenizer::UngetChar (cFileCache *par_FileCache, int Char)
{
    UNUSED(Char);
    par_FileCache->UnGetChar ();
    return 0;
}


int cTokenizer::ParseLabel (cParser *par_Parser)
{
    char LabelName[MAX_LABEL_LEN];
    int Pos;

    int LineNr;
    int FileOffset = par_Parser->GetCurrentScriptFile ()->Ftell (&LineNr);

    int c = GetChar (par_Parser->GetCurrentScriptFile ());
    // For EOF there exist a command which is always at index 0
    if (c == EOF) {
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "end of file reached inside a label");
        return -1;
    }
    Pos = 0;
    for (;;) {
        if (isalnum (c) || (c == '_')) {
            LabelName[Pos] = static_cast<char>(c);
            Pos++;
            if (Pos >= MAX_LABEL_LEN) {
                par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "label exceeds %i chars", MAX_LABEL_LEN - 1);
                return -1;
            }
            c = GetChar (par_Parser->GetCurrentScriptFile ());
        } else {
            // End of a label reached
            LabelName[Pos] = 0;
            if (!par_Parser->IsSyntaxOrRunning () && !par_Parser->GetCurrentScriptFile()->IsOnlyUsing ()) {
                if (par_Parser->AddLabel (LabelName, par_Parser->GetCurrentIp () + 1, static_cast<uint32_t>(LineNr), static_cast<uint32_t>(FileOffset)) < 0) {
                    par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "label %s already exists at line %i", LabelName,  par_Parser->GetLineNrOfLabel (LabelName));
                    return -1;
                }
            }
            return 0;
        }
    }
}


int cTokenizer::ScanNextToken (cParser *par_Parser, cFileCache *par_FileCache)
{
    int TokeTableIdx, CommandStringIdx;
    int c;

    TokeTableIdx = CommandStringIdx = 0;

    c = GetChar (par_FileCache);
    // For EOF exist an own command which are always at index 0
    if (c == EOF) return 0;

    // There can be several labels consecutive
    while (c == ':') {  // A label (jump target for GOTO and GOSUB
        if (ParseLabel (par_Parser)) {
            // ParseLabel has already printed an error message
            return -1;
        } else {
            RemoveWhitespacesAndComments (par_Parser, par_FileCache);
            StartCmdFileOffset = par_Parser->GetCurrentScriptFile ()->Ftell (&StartCmdLineNr);
            c = GetChar (par_FileCache);
            // For EOF exist a command always at index 0
            if (c == EOF) return 0;
        }
    }
    // First char of a command
    if (!isalpha (c) && (c != '_')) {
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "command should start with an alphabetic character or '_'");
        return -1;
    }
    if (FirstCharOffsetInTokenTabele[c] == -1) {
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "unknown command");
        return -1;
    }
    TokeTableIdx = FirstCharOffsetInTokenTabele[c];
    CommandStringIdx = 1;
    c = GetChar (par_FileCache); 
    for (;;) {
        if (!isalnum (c) && (c != '_')) {
            if (TokenTable[TokeTableIdx]->GetName()[CommandStringIdx] == 0) {
                if (c != EOF) UngetChar (par_FileCache, c);
                return TokeTableIdx; 
            } else {
                par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "unknown command");
                return -1;
            }
        }
        while (c > TokenTable[TokeTableIdx]->GetName()[CommandStringIdx]) {
            TokeTableIdx++;
            if (TokeTableIdx >= SizeOfTokenTable) {
                par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "unknown command");
                return -1;  // no valid command
            }
        }
        if (c == TokenTable[TokeTableIdx]->GetName()[CommandStringIdx]) {
            CommandStringIdx++;
        }
        c = GetChar (par_FileCache);
    } 
}


int cTokenizer::RemoveWhitespacesAndComments (cParser *par_Parser, cFileCache *par_FileCache)
{
    int c;
    int NestedCommentCounter = 0;

    while (1) {
__CONTINUE:
        c = GetChar (par_FileCache);
        if (MyIsSpace (c)) continue;
        if (c == '/') {
            switch (par_FileCache->WhatIsNextChar ()) {
            case '*':
                // Begin of a C comment
                NestedCommentCounter++;
                while (1) {
                    c = GetChar (par_FileCache);
                    switch (c) {
                    case EOF:
                        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "end of file inside a /* */ comment");
                        return -1;
                    case '/':
                        if (par_FileCache->WhatIsNextChar () == '*') { 
                            GetChar (par_FileCache);
                            NestedCommentCounter++;
                        }
                        break;
                    case '*':
                        if (par_FileCache->WhatIsNextChar () == '/') { 
                            GetChar (par_FileCache);
                            if (NestedCommentCounter <= 0) {
                                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "end of comment */ without corresponding /*");
                                return -1;
                            }
                            NestedCommentCounter--;
                            if (NestedCommentCounter == 0) {
                                goto __CONTINUE;
                            }
                        }
                        break;
                    default:
                        break;
                    }
                }
                //no break;
            case '/':
                // Begin of a C++ comment
                while (1) {
                    c = GetChar (par_FileCache);
                    if ((c == '\n') || (c == EOF)) goto __CONTINUE;
                }
                // no break;
            default:
                if (c != EOF) UngetChar (par_FileCache, c);
                return 0;
            }
        } else {
            if (c != EOF) UngetChar (par_FileCache, c);
            return 0;
        }
    }
    //return 0;  // should never reached
}

/* A script command have following structure
COMMAND 
COMMAND {
    ....
}
COMMAND (PARAMETER) 
COMMAND (PARAMETER) {
    ....
}
COMMAND (PARAMETER1, PARAMETER2)
COMMAND (PARAMETER1, PARAMETER2) {
    ....
}
COMMAND (PARAMETER1, PARAMETER2, ..., PARAMETERx)
COMMAND (PARAMETER1, PARAMETER2, ..., PARAMETERx) {
    ....
COMMAND (PARAMETER1, \\
         PARAMETER2, ..., PARAMETERx)
COMMAND (PARAMETER1, \\
         PARAMETER2, ..., PARAMETERx) {
    ....
}
COMMAND ($[PARAMETERLISTE]) 
COMMAND (PARAMETER1, $[PARAMETERLISTE]) 
COMMAND (PARAMETER1, ..., PARAMETERx, $[PARAMETERLISTE]) 

PARAMETER-Syntax:

(abc)
( abc )
( abc xyz) -> 1 Parameter if spaces are allowed
(abc,xyz)
( abc , xyz  )
("abc", xyz)
("abc def", xyz)
("abc, def")   -> only 1 parameter
("abc \"xxxx\" def") -> only 1 parameter with "-characters included

*/

int cTokenizer::InsertOneCharToParameterBuffer (cParser *par_Parser, int Char)
{
    // Change the buffer size?
    if (PointerToParameterBuffer >= SizeOfParameterBuffer) {
        SizeOfParameterBuffer += 1024;   
        // Buffer are always  2KByte larger as environment variable
        ParameterBuffer = static_cast<char*>(my_realloc (ParameterBuffer, static_cast<size_t>(SizeOfParameterBuffer) + 2048));
        if (ParameterBuffer == nullptr) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Out of memory");
            return -1;
        }
    }
    ParameterBuffer[PointerToParameterBuffer++] = static_cast<char>(Char);
    return 0;
}

int cTokenizer::StartParameterInBuffer (cParser *par_Parser)
{
    if (ParameterCounter >= MAX_PARAMETERS) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "to many parameter (max. 50 allowed)");
        return -1;
    }
    // Store the start position of the next parameter
    StartParameters[ParameterCounter] = PointerToParameterBuffer;
    ParameterCounter++;  
    return 0;
}

int cTokenizer::EndOfParameterInBuffer (cParser *par_Parser, int par_UseEnvVarFlag)
{
    // first terminate string
    InsertOneCharToParameterBuffer (par_Parser, 0);
    // Now search environment variable inside parameter
    if (par_UseEnvVarFlag) {
        PointerToParameterBuffer = StartParameters[ParameterCounter-1];
        PointerToParameterBuffer +=
            SearchAndReplaceEnvironmentStringsExt (ParameterBuffer + StartParameters[ParameterCounter-1], 
                                                   ParameterBuffer + StartParameters[ParameterCounter-1],
                                                   ((par_Parser->IsSyntaxOrRunning ()) ? 0 : 0x40000000) + (SizeOfParameterBuffer + 2047 - StartParameters[ParameterCounter-1]),
                                                   SolvedEnvVars + ParameterCounter - 1,
                                                   NoneSolvedEnvVars + ParameterCounter - 1) + 1;
    }
    return 0;
}


int cTokenizer::ScanOneDoubleQuoteParameter (cParser *par_Parser, cFileCache *par_FileCache)
{
    int Char;
    
    if (StartParameterInBuffer (par_Parser)) return -1;
    while (1) {
        Char = GetChar (par_FileCache);
        switch (Char) {
        case '\\':
            switch (par_FileCache->WhatIsNextChar ()) {
            case '"':     /* Parameter include a \" char  */
                Char = GetChar (par_FileCache);
                if (InsertOneCharToParameterBuffer (par_Parser, Char)) return -1;
                break;
            default:      /* otherwise it is only a simple \ */
                if (InsertOneCharToParameterBuffer (par_Parser, Char)) return -1;
                break;
            }
            break;
        case '"':
            // End of the parameter
            RemoveWhitespacesAndComments (par_Parser, par_FileCache);
            switch (GetChar (par_FileCache)) {
            case '"':
                // This was only a  war nur ein line folding -> continue
                break;
            case ',':
                // Terminate the parameter
                if (EndOfParameterInBuffer (par_Parser, 1)) return -1;
                return 1;   // Parameter with "" successfully read and there are more
                //break;
            case ')':
                // Terminate the parameter
                if (EndOfParameterInBuffer (par_Parser, 1)) return -1;
                return 0;   // Parameter with "" successfully read and there are no more parameter
                //break;
            default:
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "there are unexpected chars behind parameter");
                return -1;  // there are unexpected characters
            }
            break;
        case EOF:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "unexpected end of file in double quoted parameter");
            return -1;
        default:
            // All other characters are part of the parameter
            if (InsertOneCharToParameterBuffer (par_Parser, Char)) return -1;
            break;
        }
    }
    //return -1; // This should never reached
}


int cTokenizer::ScanParameterListe (cParser *par_Parser, cFileCache *par_FileCache)
{
    char c;
    char ParamListName[MAX_LABEL_LEN];
    int Pos = 0;

    while (1) {
        c = static_cast<char>(GetChar (par_FileCache));
        if (isascii (c) && (isalnum (c) || (c == '%') || (c == '_'))) {
            ParamListName[Pos] = c;
            Pos++;
            if (Pos >= (MAX_LABEL_LEN-1)) return -1;
        } else if (c == ']') {
            ParamListName[Pos] = 0;
            // Parameter list detected
            // Parameter list can also be an environment variable
            SearchAndReplaceEnvironmentStrings (ParamListName, 
                                                ParamListName,
                                                sizeof (ParamListName));
            if (par_Parser->AddParamListToParams (this, ParamListName) < 0) {
                //Parameter list unknown
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "the parameter list \"%s\" are unknown", ParamListName);
                return -1;  // There are unknown characters
            }
            CmdHasParamListFlag = 1;
            RemoveWhitespacesAndComments (par_Parser, par_FileCache);
            switch (GetChar (par_FileCache)) {
            case ',':
                return 1;
            case ')':
                return 0;
            default:
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "there are unexpected chars behind parameter");
                return -1;  // There are unknown characters
            }
        } else {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "there are unexpected chars inside parameter list name");
            return -1;  // There are unknown characters
        }
    }
}

int cTokenizer::CopyWhitespacesToParameterBuffer (cParser *par_Parser, cFileCache *par_FileCache, int par_Char,
                                                  int par_LineCounterSave, int par_Filepos, int par_WhiteCharCounter)
{
    if (InsertOneCharToParameterBuffer (par_Parser, par_Char)) return -1;
    if (par_WhiteCharCounter) {
        int LineNr;
        int Filepos = par_FileCache->Ftell (&LineNr);
        par_FileCache->Fseek (par_Filepos, SEEK_SET, par_LineCounterSave);
        while (par_WhiteCharCounter--) {
            int WhiteChar = GetChar (par_FileCache);
            if (InsertOneCharToParameterBuffer (par_Parser, WhiteChar)) return -1;
        }
        par_FileCache->Fseek (Filepos, SEEK_SET, LineNr);
    }
    return 0;
}

// return -1: error
// return 0: found one parameter
// return 1: found one parameter and there are following additional parameters
int cTokenizer::ScanOneParameter (cParser *par_Parser, cFileCache *par_FileCache)
{
    int Char;
    int NestedBraketCounter = 0;

    RemoveWhitespacesAndComments (par_Parser, par_FileCache);
    switch (par_FileCache->WhatIsNextChar ()) {
    case '"':
        GetChar (par_FileCache);
        return ScanOneDoubleQuoteParameter (par_Parser, par_FileCache);
    case ')':
        GetChar (par_FileCache);
        return 0;   // No parameter existing ()
    case '$':   // Parameter list
        {
            int LineNr;
            int FilePos = par_FileCache->Ftell (&LineNr);
            GetChar (par_FileCache);
            if (GetChar (par_FileCache) == '[') {
                return ScanParameterListe (par_Parser, par_FileCache);
            } else {
                par_FileCache->Fseek (FilePos, SEEK_SET, LineNr);
            }
        }
        // fall through
    default:
        if (StartParameterInBuffer (par_Parser)) return -1;
        while (1) {
            Char = GetChar (par_FileCache);
            switch (Char) {
            case ',':
                if (NestedBraketCounter == 0) {
                    if (EndOfParameterInBuffer (par_Parser, 1)) return -1;
                    return 1;   // parameter with "" successfully read and there are additional
                } else {
                    if (InsertOneCharToParameterBuffer (par_Parser, Char)) return -1;
                }
                break;
            case '(':
                if (InsertOneCharToParameterBuffer (par_Parser, Char)) return -1;
                NestedBraketCounter++;
                break;
            case ')':
                if (NestedBraketCounter == 0) {
                    if (EndOfParameterInBuffer (par_Parser, 1)) return -1;
                    return 0;   // Parameter ssuccessfully read but there are no more parameters
                } else {
                    NestedBraketCounter--;
                    if (InsertOneCharToParameterBuffer (par_Parser, Char)) return -1;
                }
                break;
            case EOF:
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "unexpected end of file in parameter (missing bracket)");
                return -1;
            default:
                if (MyIsSpace (Char)) {
                    // Remember file position
                    int WhiteChar;
                    int WhiteCharCounter = 0;
                    int LineCounterSave;
                    int Filepos = par_FileCache->Ftell (&LineCounterSave);
                    // If more whitespaces following
                    while (1) {
                        WhiteChar = GetChar (par_FileCache);
                        if (!MyIsSpace (WhiteChar)) break;
                    }
                    switch (WhiteChar) {
                    case ',':
                        if (NestedBraketCounter == 0) {
                            if (EndOfParameterInBuffer (par_Parser, 1)) return -1;
                            return 1;   // Parameter successfully read, there are more parameters
                        } else {
                            if (CopyWhitespacesToParameterBuffer (par_Parser, par_FileCache, Char,
                                                                  LineCounterSave, Filepos, WhiteCharCounter)) {
                                return -1;
                            }
                            if (InsertOneCharToParameterBuffer (par_Parser, WhiteChar)) return -1;
                        }
                        break;
                    case '(':
                        if (CopyWhitespacesToParameterBuffer (par_Parser, par_FileCache, Char,
                                                              LineCounterSave, Filepos, WhiteCharCounter)) {
                            return -1;
                        }
                        if (InsertOneCharToParameterBuffer (par_Parser, WhiteChar)) return -1;
                        NestedBraketCounter++;
                        break;
                    case ')':
                        if (NestedBraketCounter == 0) {
                            if (EndOfParameterInBuffer (par_Parser, 1)) return -1;
                            return 0;   // Parameter successfully read, there are no more parameters
                        } else {
                            NestedBraketCounter--;
                            if (CopyWhitespacesToParameterBuffer (par_Parser, par_FileCache, Char,
                                                                  LineCounterSave, Filepos, WhiteCharCounter)) {
                                return -1;
                            }
                            if (InsertOneCharToParameterBuffer (par_Parser, WhiteChar)) return -1;
                        }
                        break;
                    case EOF:
                        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "unexpected end of file in parameter (missing bracket)");
                        return -1;
                    default:
                        // Whitspaces are belong to the parameter!
                        if (CopyWhitespacesToParameterBuffer (par_Parser, par_FileCache, Char,
                                                              LineCounterSave, Filepos, WhiteCharCounter)) {
                            return -1;
                        }
                        if (InsertOneCharToParameterBuffer (par_Parser, WhiteChar)) return -1;
                        break;
                    }
                } else {    
                    if (InsertOneCharToParameterBuffer (par_Parser, Char)) return -1;
                }
            }
        }
        //break;  // default:
    }
    //return -1; // this should never reached
}


int cTokenizer::ScanCommandParameters (cParser *par_Parser, cFileCache *par_FileCache)
{
    int Ret;

    ParameterCounter = 0;
    PointerToParameterBuffer = 0;
    CmdHasParamListFlag = 0;

    EndCmdFileOffset = par_Parser->GetCurrentScriptFile ()->Ftell (&EndCmdLineNr);
    RemoveWhitespacesAndComments (par_Parser, par_FileCache);
    if (par_FileCache->WhatIsNextChar () != '(') {
        return 0;  // there are no parameter
    } else {
        GetChar (par_FileCache);
        while ((Ret = ScanOneParameter (par_Parser, par_FileCache)) == 1);
        EndCmdFileOffset = par_Parser->GetCurrentScriptFile ()->Ftell (&EndCmdLineNr);
        return Ret;
    }
}

int cTokenizer::ScanCommandEmbeddedFile (cParser *par_Parser, cFileCache *par_FileCache, const char *TempFilename)
{
    FILE *fh;
    int c;
    int NestedCommentCounter = 0;

    RemoveWhitespacesAndComments (par_Parser, par_FileCache);
    if (par_FileCache->WhatIsNextChar () != '{') {
        EmbeddedFileFlag = 0;
        return 0;  // There are no embedded file
    } else {
        GetChar (par_FileCache);  // delete '{'-char
        EmbeddedFileStartFileOffset = par_FileCache->Ftell (&EmbeddedFileStartLineNr);
        fh = fopen (TempFilename, "w");
        if (fh == nullptr) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot open file %s", TempFilename);
            return -1;
        }
        while (1) {
__CONTINUE:
            c = GetChar (par_FileCache);
            if (c == '}') {
                EmbeddedFileStopFileOffset = par_FileCache->Ftell (&EmbeddedFileStopLineNr);
                EmbeddedFileFlag = 1;
                fclose (fh);
                EndCmdFileOffset = par_Parser->GetCurrentScriptFile ()->Ftell (&EmbeddedFileStopLineNr);
                return 1;   // There are an embedded file
            }
            if (c == '/') {
                switch (par_FileCache->WhatIsNextChar ()) {
                case '*':
                    // Start of a C comment
                    NestedCommentCounter++;
                    while (1) {
                        c = GetChar (par_FileCache);
                        switch (c) {
                        case EOF:
                            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "end of file inside a /* */ comment");
                            return -1;
                        case '/':
                            if (par_FileCache->WhatIsNextChar () == '*') { 
                                GetChar (par_FileCache);
                                NestedCommentCounter++;
                            }
                            break;
                        case '*':
                            if (par_FileCache->WhatIsNextChar () == '/') { 
                                GetChar (par_FileCache);
                                if (NestedCommentCounter <= 0) {
                                    par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "end of comment */ without corresponding /*");
                                    return -1;
                                }
                                NestedCommentCounter--;
                                if (NestedCommentCounter == 0) {
                                    goto __CONTINUE;
                                }
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    //break;
                case '/':
                    // Start of a C++ comment
                    while (1) {
                        c = GetChar (par_FileCache);
                        if ((c == '\n') || (c == EOF)) goto __CONTINUE;
                    }
                    //break;
                default:
                    if (c == EOF) {
                        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "unexpected end of file in embedded file comment //...");
                        fclose (fh);
                        return -1;
                    } else {
                        fputc (c, fh);
                    }
                    break;
                }
            } else if (c == '%') {
                // Maybe an environment variable
                int LineCounterSave;
                int Filepos = par_FileCache->Ftell (&LineCounterSave);
                int c_old = -1;
                int CharCount = 0;
                while (1) {
                    c = GetChar (par_FileCache);
                    if ((c == '%') ||                      // End of an environment variable
                        (isascii(c) && isspace(c))  ||     // No whitespaces
                        ((c == '/') && (c_old == '/')) ||  // C++ Comment
                        ((c == '*') && (c_old == '/')) ||  // C Comment
                        (c == ';') ||
                        (c == '#') ||
                        (c == EOF) ||
                        (CharCount > 2048)) break;
                    CharCount++;
                }
                par_FileCache->Fseek (Filepos, SEEK_SET, LineCounterSave);
                if (c == '%') {
                    // There was something between two % characters
                    char Buffer[4096];
                    Buffer[0] = '%';
                    for (int x = 1; x < (CharCount+2); x++) {
                        Buffer[x] = static_cast<char>(GetChar (par_FileCache));
                    }
                    Buffer[CharCount+2] = 0;
                    if (SearchAndReplaceEnvironmentStrings (Buffer, Buffer, sizeof(Buffer)) < 0) {
                        // It is not an environment variable
                        par_FileCache->Fseek (Filepos, SEEK_SET, LineCounterSave);
                        c = '%';
                        goto __NO_ENV_VAR;
                    }
                    fputs (Buffer, fh);
                    goto __CONTINUE;

                } else {
                    // It is not an environment variable
                    c = '%';
                    goto __NO_ENV_VAR;
                }
            } else {
__NO_ENV_VAR:
                if (c == EOF) {
                    par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "unexpected end of file in embedded file");
                    fclose (fh);
                    return -1;
                } else {
                    fputc (c, fh);
                }
            }
        }
    }
}


int cTokenizer::ParseRawParameter (cParser *par_Parser, cFileCache *par_FileCache, int par_CommandIdx)
{
    ParameterCounter = 0;
    PointerToParameterBuffer = 0;
    CmdHasParamListFlag = 0;

    RemoveWhitespacesAndComments (par_Parser, par_FileCache);
    if (GetChar (par_FileCache) != '(') {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "expecting a '(' bracket on command \"%s\"",
                           TokenTable[par_CommandIdx]->GetName ());
        return -1; 
    } else {
        StartParameterInBuffer (par_Parser);
        int CharCount = 0;
        int PosOfLastCloseBracket = -1;
        int LineNr;
        int FilePos = par_Parser->GetCurrentScriptFile ()->Ftell (&LineNr);
        int c;
        int PosOfFirstComma = -1;
        while (1) {
            c = GetChar (par_FileCache);
            if ((c == '\n') || (c == EOF)) break;
            if (c == ')') PosOfLastCloseBracket = CharCount;
            if ((PosOfFirstComma < 0) && (c == ',')) PosOfFirstComma = CharCount;
            CharCount++;
        }
        if (PosOfLastCloseBracket < 0) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "expecting a ')' bracket on command \"%s\"",
                               TokenTable[par_CommandIdx]->GetName ());
            return -1; 
        }
        // Jump to the beginning of the Parameters
        par_Parser->GetCurrentScriptFile ()->Fseek (FilePos, SEEK_SET, LineNr);

        if ((TokenTable[CommandIdx]->GetMinParams () == -2) &&
            (PosOfFirstComma >= 0)) { 
            // It is a REPORT_RAW_PAR with more than 1 parameter
            // First parameter are all till the first ','
            for (int x = 0; x < PosOfFirstComma; x++) {
                if (InsertOneCharToParameterBuffer (par_Parser, GetChar (par_FileCache))) return -1;
            }
            EndOfParameterInBuffer (par_Parser, 1);

            // treat the remaining parameters like a normal command
            RemoveWhitespacesAndComments (par_Parser, par_FileCache);
            int Ret;
            GetChar (par_FileCache);
            while ((Ret = ScanOneParameter (par_Parser, par_FileCache)) == 1);
            EndCmdFileOffset = par_Parser->GetCurrentScriptFile ()->Ftell (&EndCmdLineNr);
            return Ret;
        } else {
            // It is a REPORT_RAW_PAR with only 1 parameter or a REPORT_RAW
            for (int x = 0; x < PosOfLastCloseBracket; x++) {
                if (InsertOneCharToParameterBuffer (par_Parser, GetChar (par_FileCache))) return -1;
            }
            if (GetChar (par_FileCache) != ')') {
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error %s (%i)", __FILE__, __LINE__);
                return -1;
            }
            EndCmdFileOffset = par_Parser->GetCurrentScriptFile ()->Ftell (&EndCmdLineNr);
            if (TokenTable[CommandIdx]->GetMinParams () == -2) {
                EndOfParameterInBuffer (par_Parser, 1);
            } else {
                EndOfParameterInBuffer (par_Parser, 0);  // Inside a REPORT_RAW command no environ variables allowed
            }
        }
    }
    return 0;
}


int cTokenizer::ParseNextCommand (cParser *par_Parser)
{
    EmbeddedFileFlag = 0;
    RemoveWhitespacesAndComments (par_Parser, par_Parser->GetCurrentScriptFile ());
    StartCmdFileOffset = par_Parser->GetCurrentScriptFile ()->Ftell (&StartCmdLineNr);
    CommandIdx = ScanNextToken (par_Parser, par_Parser->GetCurrentScriptFile ());
    if (CommandIdx >= 0) { 
        if (TokenTable[CommandIdx]->GetMinParams () < 0) {
            // REPORT_RAW has not realy parameter but all between the () breakets are the first parameter (MinParams == -1)
            // and REPORT_RAW_PAR all till the first ',' are the first parameter (MinParams == -2)
            if (ParseRawParameter (par_Parser, par_Parser->GetCurrentScriptFile (), CommandIdx)) {
                return -1;
            }
        } else {
            if (ScanCommandParameters (par_Parser, par_Parser->GetCurrentScriptFile ())) return -1;
            if (TokenTable[CommandIdx]->GetTempFilename () != nullptr) {
                if (ScanCommandEmbeddedFile (par_Parser, par_Parser->GetCurrentScriptFile (), TokenTable[CommandIdx]->GetTempFilename ()) < 0) return -1;
            }
        }
        // Only check the number of Paramter if:
        if (par_Parser->IsSyntaxOrRunning () || !CmdHasParamListFlag ) {
            if ((ParameterCounter + EmbeddedFileFlag) < TokenTable[CommandIdx]->GetMinParams ()) {
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "command \"%s\" has less parameters (%i) as expected (%i)",
                                   TokenTable[CommandIdx]->GetName (), ParameterCounter + EmbeddedFileFlag, TokenTable[CommandIdx]->GetMinParams ());

            }
            if ((ParameterCounter + EmbeddedFileFlag) > TokenTable[CommandIdx]->GetMaxParams ()) {
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "command \"%s\" has more parameters (%i) as expected (%i)",
                                   TokenTable[CommandIdx]->GetName (), ParameterCounter + EmbeddedFileFlag, TokenTable[CommandIdx]->GetMaxParams ());
            }
        }
    } 
    return CommandIdx;
}


int cTokenizer::CallActualCommandSyntaxCheckFunc (cParser *par_Parser, int CommandIdx)
{
    if (par_Parser->GetCurrentScriptFile ()->IsOnlyUsing ()) {
        if (TokenTable[CommandIdx]->SyntaxCheckInUsingMode ()) {
            int Ret = TokenTable[CommandIdx]->SyntaxCheck (par_Parser);
            if (Ret) {
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "command \"%s\" has errors",
                                   TokenTable[CommandIdx]->GetName ());
            }
            return Ret;
        } else {
            return 0;
        }
    } else {
        return TokenTable[CommandIdx]->SyntaxCheck (par_Parser);
    }
}

int cTokenizer::IsCommandInsideAtomicAllowed (int CommandIdx)
{
    return TokenTable[CommandIdx]->IsInsideAtomicAllowed ();
}


int cTokenizer::CallActualCommandExecuteFunc (cParser *par_Parser, cExecutor *par_Executor, int CommandIdx)
{
    int Ret;

    // During execution of a script command leave critical section
    ScriptLeaveCriticalSection();
    Ret = TokenTable[CommandIdx]->Execute (par_Parser, par_Executor);
    ScriptEnterCriticalSection();
    if (TokenTable[CommandIdx]->GetWaitFlagNeeded ()) {   // Has command a timeout?
        par_Executor->StartTimeoutControl (TokenTable[CommandIdx]->GetWaitFlagTimeout ());
    }
    return Ret;
}

int cTokenizer::CallActualCommandWaitFunc (cParser *par_Parser, cExecutor *par_Executor, int CommandIdx,int Cycles)
{
    return TokenTable[CommandIdx]->Wait (par_Parser, par_Executor, Cycles);
}

const char *cTokenizer::GetCommandName (int CommandIdx)
{
    return TokenTable[CommandIdx]->GetName ();
}

int cTokenizer::GetCommandWaitTimeout (int CommandIdx)
{
    return TokenTable[CommandIdx]->GetWaitFlagTimeout ();
}


int cTokenizer::AddCmdToTokenArray (cBaseCmd *NewCmd)
{
    int x, y;

    cTokenizer::SizeOfTokenTable++;
    // Use realloc NOT my_realloc!!! this will called inside the c++ startup routine.
    // Remark: this memory will never freed.
    cTokenizer::TokenTable = static_cast<cBaseCmd**>(realloc (cTokenizer::TokenTable, sizeof (cBaseCmd*) * static_cast<size_t>(cTokenizer::SizeOfTokenTable)));
    if (cTokenizer::TokenTable == nullptr) return -1;
    for (x = 0; x < (cTokenizer::SizeOfTokenTable - 1); x++) {
        if (strcmp (NewCmd->GetName (), cTokenizer::TokenTable[x]->GetName ()) < 0) break;
    }
    for (y = (cTokenizer::SizeOfTokenTable - 1); y > x; y--) {
        cTokenizer::TokenTable[y] = cTokenizer::TokenTable[y-1];
    }
    cTokenizer::TokenTable[x] = NewCmd;

    for (x = 0; x < 256; x++) {
        FirstCharOffsetInTokenTabele[x] = -1;
    }
    for (x = 0; x < cTokenizer::SizeOfTokenTable; x++) {
        int c = *(cTokenizer::TokenTable[x]->GetName ()); // First character of the command
        if (FirstCharOffsetInTokenTabele[c] == -1) {
            FirstCharOffsetInTokenTabele[c] = x;
        }
    }
    
    return 0;
}

const char *cTokenizer::GetCmdToTokenName (int par_Idx)
{
    if (cTokenizer::TokenTable == nullptr) return nullptr;
    if (par_Idx >= cTokenizer::SizeOfTokenTable) return nullptr;
    return cTokenizer::TokenTable[par_Idx]->GetName ();
}

const char *cTokenizer::GetEmbeddedFilename (void)
{
    return TokenTable[CommandIdx]->GetTempFilename ();
}


void cTokenizer::DebugPrintParsedCommand (void)
{
    int x;

    printf ("%s [%i] (", TokenTable[CommandIdx]->GetName (), ParameterCounter);
    for (x = 0; x < ParameterCounter; x++) {
        if (x) printf (", ");
        printf ("%s", ParameterBuffer + StartParameters[x]);
    }
    printf (")\n");
}

void cTokenizer::DebugPrintParsedCommandToFile (FILE *fh, int par_CurrentIp, int par_AtomicCounter)
{
    int x;

    fprintf (fh, "%10u %i %08u %s", read_bbvari_udword (CycleCounterVid), par_AtomicCounter, par_CurrentIp, TokenTable[CommandIdx]->GetName ());
    if (ParameterCounter) {
        fprintf (fh, "(");
        for (x = 0; x < ParameterCounter; x++) {
            if (x) fprintf (fh, ", ");
            fprintf (fh, "%s", ParameterBuffer + StartParameters[x]);
        }
        fprintf (fh, ")");
    }
    fprintf (fh, "\t");
}


int cTokenizer::SizeOfTokenTable;
cBaseCmd **cTokenizer::TokenTable;
int cTokenizer::FirstCharOffsetInTokenTabele[256] = {
    -1, -1 -1, -1, -1, -1, -1, -1,
    -1, -1 -1, -1, -1, -1, -1, -1,
    -1, -1 -1, -1, -1, -1, -1, -1,
    -1, -1 -1, -1, -1, -1, -1, -1,
    -1, -1 -1, -1, -1, -1, -1, -1,
    -1, -1 -1, -1, -1, -1, -1, -1,
    -1, -1 -1, -1, -1, -1, -1, -1,
    -1, -1 -1, -1, -1, -1, -1, -1};

