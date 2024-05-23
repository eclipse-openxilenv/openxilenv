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


#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "FileCache.h"
#include "CmdTable.h"
#include "ProcTable.h"
#include "ProcStack.h"
#include "CompilerTree.h"
#include "Tokenizer.h"
#include "FileStack.h"
#include "ToParseFileStack.h"
#include "ParamList.h"
#include "ScriptErrorMsgDlg.h"

extern "C" {
#include "MyMemory.h"
}

class cExecutor;

extern "C" {
    extern FILE *DebugFile;
}

class cParser {
private:
    int SyntaxOrRunningFlag;  // 0 -> Syntax-Check, 1 -> Running 

    int State;
#define PARSER_STATE_IDLE    0
#define PARSER_STATE_PARSE   1
#define PARSER_STATE_STOP   -1
#define PARSER_STATE_ERROR  -2

    int CurrentIp;
    int CurrentFileNr;
    cFileCache *CurrentScript;

    //ErrorStack Errors;
    int ErrorCounter;

    cCmdTable *CmdTable;

    cProc *InsideProc;      // during parse current active procedure
    cProcStack ProcStack; 
    cProcTable *ProcTable;

    cCompilerTree CompilerTree;

    cTokenizer Tokenizer;

    cFileStack FileStack;  // The  files included with USING are organized as stack
    cToParseFileStack ToParseFileStack;

    // This will use for transfering of optional parameter to the command call methods
    int CmdHasParameters;
    struct {
        uint32_t OptParameter;
        uint32_t Reserved;
    } CmdParameters;

    cParamList ParamList;

    void PrintErroneousClipping ();

    char *MessageOutputBuffer;
    int SizeofMessageOutputBuffer;

    int VerboseMessage;
#define VERBOSE_MESSAGE_PREFIX_OFF  -1
#define VERBOSE_ON                   1
#define VERBOSE_OFF                  0
#define VERBOSE_MESSAGE_PREFIX_CYCLE_COUNTER   2

    int flag_add_bbvari_automatic;
#define ADD_BBVARI_AUTOMATIC_OFF     0
#define ADD_BBVARI_AUTOMATIC_ON      1
#define ADD_BBVARI_AUTOMATIC_REMOVE  2
#define ADD_BBVARI_AUTOMATIC_STOP    3
#define ADD_BBVARI_AUTOMATIC_OFF_EX  4
#define ADD_BBVARI_AUTOMATIC_STOP_EX 5

    int AtomicCounter;  // If this is 0 then we are not inside an ATOMIC ENDATOMIC block
    int FileNrCorrespondingAtomic;
    int LineNrCorrespondingAtomic;

public:
    cParser (void)
    {
        CmdTable = nullptr;
        InsideProc = nullptr;
        ProcTable = nullptr;
        SizeofMessageOutputBuffer = 0;
        MessageOutputBuffer = nullptr;
    }
    ~cParser (void)
    {
        if (MessageOutputBuffer != nullptr) my_free (MessageOutputBuffer);
        MessageOutputBuffer = nullptr;
    }

    char *FormatMessageOutputString (int par_FormatStringPos);
    char *FormatMessageOutputStringNoPrefix (int par_FormatStringPos);
    int SytaxCheckMessageOutput (int par_FormatStringPos);

    void SetCurrentScript (cFileCache *par_FileCache, int par_CurrentFileNr)
    {
        CurrentScript = par_FileCache;
        CurrentFileNr = par_CurrentFileNr;
    }

    int ParseNextCommand (void)
    {
        return Tokenizer.ParseNextCommand (this);
    }

    int GetParameterCounter (void)
    {
        return Tokenizer.GetParameterCounter ();
    }

    char *GetParameter (int par_Idx)
    {
        return Tokenizer.GetParameter (par_Idx);
    }

    int CallActualCommandExecuteFunc (cExecutor *par_Executor, int CommandIdx)
    {
        return Tokenizer.CallActualCommandExecuteFunc (this, par_Executor, CommandIdx);
    }

    int CallActualCommandWaitFunc (cExecutor *par_Executor, int CommandIdx, uint32_t par_Cycles)
    {
        return Tokenizer.CallActualCommandWaitFunc (this, par_Executor, CommandIdx, par_Cycles);
    }


    int CheckIfCommandInsideAtomicAllowed (int par_CmdIdx);

    int GetCurrentFileNr (void)
    {
        return CurrentFileNr;
    }

    const char *GetCurrentScriptPath (void)
    {
        if (CurrentScript == nullptr) return (char*)"";
        return CurrentScript->GetFilename ();
    }

    int GetCurrentLineNr (void)
    {
        if (CurrentScript == nullptr) return -1;
        return CurrentScript->GetLineCounter ();
    }

    int Error (int Level, const char * const FormatStr, ...);
#define SCRIPT_PARSER_FATAL_ERROR     -1
#define SCRIPT_PARSER_ERROR_CONTINUE  -2
#define SCRIPT_PARSER_NO_ERROR         0
#define SCRIPT_PARSER_WARNING          1
#define SCRIPT_PARSER_MESSAGE          2

    int GetNextCmd ();
    cProc *Init (cCmdTable *par_CmdTable, cProcTable *par_ProcTable)
    {
        Reset ();
        VerboseMessage = VERBOSE_OFF;
        CmdTable = par_CmdTable;
        ProcTable = par_ProcTable;
        CurrentIp = 0;
        CmdHasParameters = 0;
        AtomicCounter = 0;
        CmdParameters.OptParameter = CmdParameters.Reserved = 0;
        // The main function must exist always
        InsideProc = new cProc (this, "main", 0, 0, 1, 0);
        ProcTable->AddProc (InsideProc);
        return InsideProc;
    }

    int SyntaxCheck (char *par_Filename);

    int IsFileAlreadyLoadedAndChecked (char *par_Filename, int *ret_StartIp);
    int ChangeToFileIfNotLoadedChecked (char *par_Filename, int *ret_StartIp, int PushToStackFlag, int OnlyUsingFlag);


    cFileCache *GetCurrentScriptFile (void)
    {
        return CurrentScript;
    }

    void SetParserState (int par_State)
    {
        State = par_State;
    }

    int EndOfScriptFileReached (void);

    int GoBackOneFile (void)
    {
        int RunCmdIp;
        if (FileStack.Pop (&CurrentFileNr, &CurrentScript, &RunCmdIp) < 0) {
            // Reach the EOF of the main scripts
            State = PARSER_STATE_IDLE;
            return -1;
        } else {
            CmdTable->SetCmdReservedParam (RunCmdIp, CurrentIp);
        }
        return CurrentFileNr;
    }

    int AddIf (void);
    int AddElse (int par_CurrentIp, int *ret_CorrespondingIfIp);
    int AddElseIf (int par_CurrentIp, int *ret_CorrespondingIfIp);
    int RemoveEndIf (int *ret_CorrespondingIfIp);

    int AddWhile (void);
    int AddBreak (void);
    int RemoveEndWhile (uint32_t *ret_CorrespondingWhileIp);
    int RemoveBreakFromWhile (uint32_t *ret_Ip);

    int AddAtomic (void);
    int RemoveEndAtomic (void);

    int AddDefProc (void);
    int RemoveEndDefProc (int *ret_CorrespondingDefProcIp);

    int AddDefLocals (void);
    int RemoveEndDefLocals (void);


    int AddProc (cProc *par_Proc) 
    {
        return ProcTable->AddProc (par_Proc);
    }

    cProc *SerchProcByName (char *par_Name)
    {
        return ProcTable->SerchProcByName (par_Name);
    }

    void SetOptParameter (uint32_t par_OptParameter, uint32_t par_Reserved)
    {
        CmdHasParameters = 1;
        CmdParameters.OptParameter = par_OptParameter;
        CmdParameters.Reserved = par_Reserved;
    }

    void SetOptParameter (int Ip, uint32_t par_OptParameter, uint32_t par_Reserved)
    {
        CmdTable->SetCmdParam (Ip, par_OptParameter, par_Reserved);
    }

    int GetCurrentIp (void)
    {
        return CurrentIp;
    }

    int AddLabel (char *par_LabelName, int par_Ip, uint32_t par_LineNr, uint32_t par_FileOffset);
    int AddGoto (char *par_LabelName, int par_Ip, uint32_t par_LineNr, uint32_t par_FileOffset);
    int GetLineNrOfLabel (char *par_LabelName);


    int PushProcToStack (cProc *par_Proc)
    {
        if (InsideProc == nullptr) {
            Error (SCRIPT_PARSER_FATAL_ERROR, "internal error at %s %i", __FILE__, __LINE__);
            return -1;
        } else {
            int Ret = ProcStack.Push (InsideProc);
            InsideProc = par_Proc;
            return Ret;
        }
    }

    int PopProcFromStack (void)
    {
        return ProcStack.Pop (&InsideProc);
    }

    int AddToParseFile (char *par_Name, int par_Ip, uint32_t par_LineNr, int par_UsingFlag)
    {
        return ToParseFileStack.AddRun (par_Name, par_Ip, par_LineNr, par_UsingFlag);
    }

    int CheckIfAllGotosResovedInCurrentProc (void)
    {
        return InsideProc->CheckIfAllGotosResoved (this);
    } 
    
    int HasCmdEmbeddedFile (int *ret_StartFileOffset, int *ret_StopFileOffset)
    {
        return Tokenizer.HasCmdEmbeddedFile (ret_StartFileOffset, ret_StopFileOffset);
    }

    const char *GetEmbeddedFilename (void)
    {
        return Tokenizer.GetEmbeddedFilename ();
    }

    int AddParamListToParams (cTokenizer *par_Tokenizer, char *ParamListName);

    int AddNewParamList (void);
    int DelParamList (char *par_ParamListName);
    int AddParamsToParamList (void);
    int DelParamsFromParamList (void);

    void SetSyntaxCheckFlag (void)
    {
        SyntaxOrRunningFlag = 0;
    }
    void SetRunningFlag (void)
    {
        SyntaxOrRunningFlag = 1;
    }
    int IsSyntaxOrRunning (void)
    {
        return SyntaxOrRunningFlag;
    }

    int SolveEquationForParameter (int par_ParamIdx, double *ret_Result, int StackPos);
    int SolveEquationForParameter (int par_ParamIdx, int *ret_Result, int StackPos);
    int SolveEquationForParameter (int par_ParamIdx, union FloatOrInt64 *ret_ResultValue, int *ret_ResultType, int StackPos);

    void SetVerboseMessage (int par_VerboseMessage)
    {
        VerboseMessage = par_VerboseMessage;
    }
    int GetVerboseMessage (void)
    {
        return VerboseMessage;
    }

    int HasCmdParamList (void)
    {
        return Tokenizer.HasCmdParamList ();
    }

    void Reset (void);

    void PrintDebugFile (const char * const Format, ...);

    int GenDebugFile (void)
    {
        return (DebugFile != nullptr);
    }

    void PrintCurrentCommandToDebugFile (int par_CurrentIp, int par_AtomicCounter)
    {
        Tokenizer.DebugPrintParsedCommandToFile (DebugFile, par_CurrentIp, par_AtomicCounter);
    }

    int SendMessageToProcess (const char *par_Message, int par_Len, const char *par_Processname, int par_MessageId);

    int GetStartCmdLineNr (void)
    {
        return Tokenizer.GetStartCmdLineNr ();
    }

    int GetParamListString (char **Buffer, int *SizeOfBuffer,
                            char **RefBuffer, int *SizeOfRefBuffer)
    {
        return ParamList.GetParamListString (Buffer, SizeOfBuffer,
                                             RefBuffer, SizeOfRefBuffer);
    }

    void set_flag_add_bbvari_automatic (int par_flag_add_bbvari_automatic)
    {
        flag_add_bbvari_automatic = par_flag_add_bbvari_automatic;
    }
    int get_flag_add_bbvari_automatic (void)
    {
        return flag_add_bbvari_automatic;
    }

    int GetNumSolvedEnvVars (int par_ParameterNr)
    {
        return Tokenizer.GetNumSolvedEnvVars (par_ParameterNr);
    }

    int GetNumNoneSolvedEnvVars (int par_ParameterNr)
    {
        return Tokenizer.GetNumNoneSolvedEnvVars (par_ParameterNr);
    }

    int GetNumEnvVars (int par_ParameterNr)
    {
        return Tokenizer.GetNumEnvVars (par_ParameterNr);
    }
};

#endif
