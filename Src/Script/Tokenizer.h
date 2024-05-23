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


#ifndef TOKENIZER_H
#define TOKENIZER_H

#define MAX_PARAMETERS    100
#define PARSER_MAX_ERR_STRING_LEN 4096
#define MAX_LABEL_LEN    256

#include "FileCache.h"
extern "C" {
#include <stdio.h>
}

class cBaseCmd;
class cParser;
class cExecutor;

class cTokenizer {
private:
    int CommandIdx;
    int StartCmdFileOffset;
    int StartCmdLineNr;
    int EndCmdFileOffset;
    int EndCmdLineNr;
    int StartParameters[MAX_PARAMETERS];
    char WhitspacesInsideParameters[MAX_PARAMETERS];
    int SolvedEnvVars[MAX_PARAMETERS];
    int NoneSolvedEnvVars[MAX_PARAMETERS];
    int ParameterCounter;

    char *ParameterBuffer;
    int SizeOfParameterBuffer;
    int PointerToParameterBuffer;

    int CmdHasParamListFlag;

    int EmbeddedFileFlag;
    int EmbeddedFileStartFileOffset;
    int EmbeddedFileStartLineNr;
    int EmbeddedFileStopFileOffset;
    int EmbeddedFileStopLineNr;

    static int SizeOfTokenTable;
    static cBaseCmd **TokenTable;
    static int FirstCharOffsetInTokenTabele[256];

    int GetChar (cFileCache *par_FileCache);
    int UngetChar (cFileCache *par_FileCache, int Char);


    int RemoveWhitespacesAndComments (cParser *par_Parser, cFileCache *par_FileCache);

    int ScanOneDoubleQuoteParameter (cParser *par_Parser, cFileCache *par_FileCache);

    int ScanNextToken (cParser *par_Parser, cFileCache *par_FileCache);

    int CopyWhitespacesToParameterBuffer (cParser *par_Parser, cFileCache *par_FileCache, int par_Char, 
                                          int par_LineCounterSave, int par_Filepos, int par_WhiteCharCounter);

    int ScanOneParameter (cParser *par_Parser, cFileCache *par_FileCache);

    int ScanCommandParameters (cParser *par_Parser, cFileCache *par_FileCache);

    int ScanCommandEmbeddedFile (cParser *par_Parser, cFileCache *par_FileCache, const char *TempFilename);

    int ScanParameterListe (cParser *par_Parser, cFileCache *par_FileCache);

    int GetCommandWaitTimeout (int CommandIdx);

    int ParseLabel (cParser *par_Parser);

public:
    cTokenizer (void)
    {
        ParameterCounter = 0;
        ParameterBuffer = nullptr;
        SizeOfParameterBuffer = 0;
        PointerToParameterBuffer = 0;
        EmbeddedFileFlag = 0;
    }

    void Reset (void)
    {
        ParameterCounter = 0;
        PointerToParameterBuffer = 0;
        EmbeddedFileFlag = 0;
    }

    int GetStartCmdFileOffset (void)
    {
        return StartCmdFileOffset;
    }
    int GetStartCmdLineNr (void)
    {
        return StartCmdLineNr;
    }
    int GetEndCmdFileOffset (void)
    {
        return EndCmdFileOffset;
    }

    int ParseNextCommand (cParser *par_Parser);

    int CallActualCommandSyntaxCheckFunc (cParser *par_Parser, int CommandIdx);

    int IsCommandInsideAtomicAllowed (int CommandIdx);

    const char *GetCommandName(int CommandIdx);

    int CallActualCommandExecuteFunc (cParser *par_Parser, cExecutor *par_Executor, int CommandIdx);

    int CallActualCommandWaitFunc (cParser *par_Parser, cExecutor *par_Executor, int CommandIdx, int Cycles);

    int GetParameterCounter (void)
    {
        return ParameterCounter;
    }

    char *GetParameter (int par_Idx)
    {
        if (par_Idx >= ParameterCounter) return nullptr;
        return ParameterBuffer + StartParameters[par_Idx];
    }

    static int AddCmdToTokenArray (cBaseCmd *NewCmd);

    static const char *GetCmdToTokenName(int par_Idx);

    void DebugPrintParsedCommand (void);
    void DebugPrintParsedCommandToFile (FILE *fh, int par_CurrentIp, int par_AtomicCounter);

    int HasCmdEmbeddedFile (int *ret_StartFileOffset, int *ret_StopFileOffset)
    {
        *ret_StartFileOffset = EmbeddedFileStartFileOffset;
        *ret_StopFileOffset = EmbeddedFileStopFileOffset;
        return EmbeddedFileFlag;
    }

    int HasCmdEmbeddedFile (void)
    {
        return EmbeddedFileFlag;
    }

    int HasCmdParamList (void)
    {
        return CmdHasParamListFlag;
    }

    const char *GetEmbeddedFilename(void);

    int InsertOneCharToParameterBuffer (cParser *par_Parser, int Char);

    int StartParameterInBuffer (cParser *par_Parser);

    int EndOfParameterInBuffer (cParser *par_Parser, int par_UseEnvVarFlag);

    int ParseRawParameter (cParser *par_Parser, cFileCache *par_FileCache, int par_CommandIdx);

    int GetNumSolvedEnvVars (int par_ParameterNr)
    {
        return SolvedEnvVars[par_ParameterNr];
    }

    int GetNumNoneSolvedEnvVars (int par_ParameterNr)
    {
        return NoneSolvedEnvVars[par_ParameterNr];
    }

    int GetNumEnvVars (int par_ParameterNr)
    {
        return NoneSolvedEnvVars[par_ParameterNr] + SolvedEnvVars[par_ParameterNr];
    }

};


#endif

