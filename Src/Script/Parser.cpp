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
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <limits.h>


extern "C" {
#include "MyMemory.h"
#include "PrintFormatToString.h"
#include "EquationParser.h"
#include "ThrowError.h"
#include "Scheduler.h"
#include "Message.h"
#include "ScriptErrorFile.h"
#include "MainValues.h"
}
#include "FileCache.h"
#include "FormatMessageOutput.h"
#include "Parser.h"
#include <stdlib.h>

int cParser::Error (int Level, const char * const FormatStr, ...)
{
    int Size, Len, Offset;
    char *MessageBuffer = NULL;
    char *s;
    int LineNr;
    const char *Filename;
    va_list Args;

    // First file name than line number
    if (CurrentScript != nullptr) {
        LineNr = GetStartCmdLineNr();
        Filename = CurrentScript->GetFilename();
        Size = (int)strlen(Filename) + 1024;  // first try with 1024 bytes
        MessageBuffer = (char*)(my_malloc((size_t)(Size)));
        s = MessageBuffer + PrintFormatToString (MessageBuffer, Size, "%s(%i) : ", Filename, LineNr);
    } else {
        LineNr = 0;
        Filename = "";
        Size = 1024; // first try with 1024 bytes
        s = MessageBuffer = (char*)(my_malloc((size_t)(Size)));
    }

    // Than the error level
    switch (Level) {
    case SCRIPT_PARSER_FATAL_ERROR:
        s += PrintFormatToString (s, Size - (s - MessageBuffer), "fatal error: ");
        ErrorCounter++;
        State = PARSER_STATE_ERROR;
        break;
    case SCRIPT_PARSER_ERROR_CONTINUE:
        s += PrintFormatToString (s, Size - (s - MessageBuffer), "error: ");
        ErrorCounter++;
        break;
    default:
    case SCRIPT_PARSER_NO_ERROR:
        break;
    case SCRIPT_PARSER_WARNING:
        s += PrintFormatToString (s, Size - (s - MessageBuffer), "warning: ");
        break;
    case SCRIPT_PARSER_MESSAGE:
        s += PrintFormatToString (s, Size - (s - MessageBuffer), "comment: ");
        break;
    }

    // Than the error message
    Offset = (int)(s - MessageBuffer);
    int LoopCount = 0;
    while(1) {
        va_start (Args, FormatStr);
        Len = vsnprintf(s, (size_t)(Size - Offset - 1), FormatStr, Args) + 1;
        va_end (Args);

        if ((Len < 0) || (LoopCount > 1)) {
            StringCopyMaxCharTruncate (s, "internal format error", Size - (s - MessageBuffer));
            break;
        }

        if ((Len + Offset) <= (Size)) {
            break;
        }
        Size = Len + Offset;
        //Size = Size + (Size >> 1);  // add 50% more
        MessageBuffer = (char*)(my_realloc(MessageBuffer, (size_t)(Size)));
        if (MessageBuffer == NULL) {
            return -1;
        }
        s = MessageBuffer + Offset;
        LoopCount++;
    }

    if (SyntaxOrRunningFlag == 0) { // Only if syntax check is active write to script.err file
        AddScriptError (MessageBuffer, ERR_COUNTER);
    }
    cScriptErrorMsgDlg::ScriptErrorMsgDlgAddMsg  (LineNr, Filename, MessageBuffer);
    my_free(MessageBuffer);
    return 0;
}


int cParser::IsFileAlreadyLoadedAndChecked (char *par_Filename, int *ret_StartIp)
{
    cFileCache *FileCached;

    if ((FileCached = cFileCache::IsFileCached (par_Filename)) != nullptr) {
        if (!FileCached->IsOnlyUsing ()) {
            *ret_StartIp = FileCached->GetIpStart ();
            return 1; // File was already translated
        }
    }
    return 0;
}

int cParser::EndOfScriptFileReached (void)
{
    int FileIdx;
    int RunIdx;
    int StartIp;
    int CurrentIpHelp;

    // Check if file are loaded with USING command, so it should be added to the file stack.
    if (!FileStack.IsEmpty ()) {
        FileStack.Pop (&CurrentFileNr, &CurrentScript, &CurrentIpHelp);
        return 0;
    } else {
        CompilerTree.EndOfFile (this);

        CurrentScript->SetIpEnd (CurrentIp);

        // Look if there are something to translate.
        if ((FileIdx = ToParseFileStack.GetNextFile ()) < 0) {
            // No more files -> all scripts was translated
            State = PARSER_STATE_IDLE;
            return 1; 
        } else {
            // There are an additional script file, which was loaded by a RUN command should be parsed
            // Resolve all RUN references to this script file
            for (RunIdx = ToParseFileStack.GetFileFirstRun (FileIdx); RunIdx >= 0; RunIdx = ToParseFileStack.NextRunFile (RunIdx)) {
                int Ip = ToParseFileStack.GetRunIp (RunIdx);
                SetOptParameter (Ip, static_cast<uint32_t>(CurrentIp) + 1, 0);
            }
            // Than open new file
            if (ChangeToFileIfNotLoadedChecked (ToParseFileStack.GetFileName (FileIdx), &StartIp, 0, 0) < 0) {
                return -1;
            }
            return 0;
        }
    }
}


int cParser::ChangeToFileIfNotLoadedChecked (char *par_Filename, int *ret_StartIp, int PushToStackFlag, int OnlyUsingFlag)
{
    cFileCache *FileCached;
    int NewFileNr;

    if ((FileCached = cFileCache::IsFileCached (par_Filename, &NewFileNr)) != nullptr) {
        if (FileCached->IsOnlyUsing ()) {
            // File was only open by USING command
            if (OnlyUsingFlag) {
                // And was loaded again by a USING command
                *ret_StartIp = -1;  // No valid IP it was not translated
                return 0;
            } else {
                // It should be translated now
                FileCached->RestUsingFlag ();
                FileCached->Reset ();
                if (PushToStackFlag) {
                    if ((NewFileNr == CurrentFileNr) || FileStack.IsInStack (NewFileNr)) {
                        Error (SCRIPT_PARSER_FATAL_ERROR, "recursives USING das scheint nie zu passieren?!\n");
                    }
                    FileStack.Push (CurrentFileNr, CurrentScript, CurrentIp);
                }
                CurrentScript = FileCached;
                CurrentFileNr = NewFileNr;
                CurrentScript->SetIpStart (CurrentIp + 1);
                *ret_StartIp = CurrentIp + 1;
                return 1;
            }
        } else {
            // File was already translated
            *ret_StartIp = FileCached->GetIpStart ();
        }
        return 0;
    } else {  
        // Complete new file load and immediately translate
        if (PushToStackFlag) FileStack.Push (CurrentFileNr, CurrentScript, CurrentIp);
        CurrentScript = new cFileCache;
        if (CurrentScript->Load (par_Filename, OnlyUsingFlag)) {
            delete CurrentScript;
            CurrentScript = nullptr;
            Error (SCRIPT_PARSER_FATAL_ERROR, "cannot open file \"%s\"", par_Filename);

            return -1;
        } 
        if ((CurrentFileNr = CurrentScript->AddToCache ()) < 0) {
            Error (SCRIPT_PARSER_FATAL_ERROR, "cannot add file \"%s\" to cache", par_Filename);
            return -1;
        }
        CurrentScript->SetIpStart (CurrentIp + 1);
        *ret_StartIp = CurrentIp + 1;
        return 1;
    }
}


int cParser::SyntaxCheck (char *par_Filename)
{
    int Help;

    CompilerTree.SetStrictAtomic (s_main_ini_val.ScrictAtomicEndAtomicCheck);

    ErrorCounter = 0;
    CurrentIp = -1;

    /* Setup a error file */
    if (GenerateNewErrorFile ()) {
        ThrowError (1, "Cannot generate error file ! Disk full ?  Write protected ?");
    }
    // main-Script oeffnen
    if (ChangeToFileIfNotLoadedChecked (par_Filename, &Help, 0, 0) < 0) {
        return -1;
    }

    CurrentIp = 0;  // Start addresse is always 0
    State = PARSER_STATE_PARSE;
    SyntaxOrRunningFlag = 0;
    while (State > 0) {
        int CommandIdx = Tokenizer.ParseNextCommand (this);
        if (CommandIdx < 0) {
            return -1;
        }
        if (AtomicCounter) {
            if (!Tokenizer.IsCommandInsideAtomicAllowed (CommandIdx)) {
                Error (SCRIPT_PARSER_FATAL_ERROR, "command \"%s\" not allowed inside ATOMIC END_ATOMIC block, corresponding ATOMIC found at %s (%i)", Tokenizer.GetCommandName (CommandIdx),
                       cFileCache::GetFilenameByNr (FileNrCorrespondingAtomic), LineNrCorrespondingAtomic);
            }
        }
        int LineCounter = Tokenizer.GetStartCmdLineNr ();
        int StartCmdFileOffset = Tokenizer.GetStartCmdFileOffset ();
        int EndCmdFileOffset = Tokenizer.GetEndCmdFileOffset ();
        // Save the following because the current script file can be switched
        int CurrentFileNrSave = CurrentFileNr;
        int StartCmdFileOffsetSave = StartCmdFileOffset;   // that is not necessary
        int EndCmdFileOffsetSave = EndCmdFileOffset;       // that is not necessary
        cFileCache *CurrentScriptSave = CurrentScript; 
        if (Tokenizer.CallActualCommandSyntaxCheckFunc (this, CommandIdx)) {
            return -1;
        }
        if (!CurrentScriptSave->IsOnlyUsing ()) {
            CurrentIp = CmdTable->AddCmd (CommandIdx, LineCounter, CurrentFileNrSave, StartCmdFileOffsetSave, static_cast<uint32_t>(EndCmdFileOffsetSave),
                                          CmdParameters.OptParameter, CmdParameters.Reserved);
            if (CmdHasParameters) {
                CmdHasParameters = 0;
                CmdParameters.OptParameter = CmdParameters.Reserved = 0;
            }
        }
    }
    
    return ErrorCounter;
}

int cParser::AddIf (void)
{
    return CompilerTree.AddIf (static_cast<uint32_t>(CurrentIp), GetStartCmdLineNr (), static_cast<uint32_t>(Tokenizer.GetStartCmdFileOffset ()));
}

int cParser::AddElse (int par_CurrentIp, int *ret_CorrespondingIfIp)
{
    int CorrespondingIfLineNr;
    uint32_t CorrespondingFileOffset;

    return CompilerTree.AddElse (static_cast<uint32_t>(par_CurrentIp), GetStartCmdLineNr (), static_cast<uint32_t>(Tokenizer.GetStartCmdFileOffset ()),
                                 reinterpret_cast<uint32_t*>(ret_CorrespondingIfIp), &CorrespondingIfLineNr,
                                 &CorrespondingFileOffset, this);
}

int cParser::AddElseIf (int par_CurrentIp, int *ret_CorrespondingIfIp)
{
    int CorrespondingIfLineNr;
    uint32_t CorrespondingFileOffset;

    return CompilerTree.AddElseIf (static_cast<uint32_t>(par_CurrentIp), GetStartCmdLineNr (), static_cast<uint32_t>(Tokenizer.GetStartCmdFileOffset ()),
                                   reinterpret_cast<uint32_t*>(ret_CorrespondingIfIp), &CorrespondingIfLineNr,
                                   &CorrespondingFileOffset, this);
}


int cParser::RemoveEndIf (int *ret_CorrespondingIfIp)
{
    int CorrespondingIfLineNr;
    uint32_t CorrespondingFileOffset;

    return CompilerTree.RemoveEndIf (reinterpret_cast<uint32_t*>(ret_CorrespondingIfIp), &CorrespondingIfLineNr,
                                     &CorrespondingFileOffset, this);
}


int cParser::AddWhile (void)
{
    return CompilerTree.AddWhile (static_cast<uint32_t>(CurrentIp), GetStartCmdLineNr (), static_cast<uint32_t>(Tokenizer.GetStartCmdFileOffset ()));
}

int cParser::AddBreak (void)
{
    return CompilerTree.AddBreak (static_cast<uint32_t>(CurrentIp), GetStartCmdLineNr (), static_cast<uint32_t>(Tokenizer.GetStartCmdFileOffset ()), this);
}

int cParser::RemoveEndWhile (uint32_t *ret_CorrespondingWhileIp)
{
    int CorrespondingWhileLineNr;
    uint32_t CorrespondingFileOffset;

    return CompilerTree.RemoveEndWhile (ret_CorrespondingWhileIp, &CorrespondingWhileLineNr, 
                                        &CorrespondingFileOffset, this);
}

int cParser::RemoveBreakFromWhile (uint32_t *ret_Ip)
{
    int ret_LineNr;
    uint32_t ret_FileOffset;

    return CompilerTree.RemoveBreakFromWhile (ret_Ip, &ret_LineNr, &ret_FileOffset, this);
}



int cParser::AddAtomic (void)
{
    AtomicCounter++;
    InsideProc->IncAtomicDepth ();
    FileNrCorrespondingAtomic = CurrentFileNr;
    LineNrCorrespondingAtomic = GetStartCmdLineNr ();
    return CompilerTree.AddAtomic (static_cast<uint32_t>(CurrentIp), GetStartCmdLineNr (), static_cast<uint32_t>(Tokenizer.GetStartCmdFileOffset ()));
}

int cParser::RemoveEndAtomic (void)
{
    uint32_t CorrespondingAtomicIp;
    int CorrespondingAtomicLineNr;
    uint32_t CorrespondingFileOffset;

    if (AtomicCounter) AtomicCounter--;
    InsideProc->DecAtomicDepth ();
    return CompilerTree.RemoveEndAtomic (&CorrespondingAtomicIp, &CorrespondingAtomicLineNr, 
                                         &CorrespondingFileOffset, this);
}


int cParser::AddDefProc (void)
{
    return CompilerTree.AddDefProc (static_cast<uint32_t>(CurrentIp), GetStartCmdLineNr (), static_cast<uint32_t>(Tokenizer.GetStartCmdFileOffset ()));
}

int cParser::RemoveEndDefProc (int *ret_CorrespondingDefProcIp)
{
    int CorrespondingDefProcLineNr;
    uint32_t CorrespondingFileOffset;

    return CompilerTree.RemoveEndDefProc (reinterpret_cast<uint32_t*>(ret_CorrespondingDefProcIp), &CorrespondingDefProcLineNr,
                                          &CorrespondingFileOffset, this);
}


int cParser::AddDefLocals (void)
{
    return (InsideProc->AddDefLocals (CurrentIp) < 0) ||
           CompilerTree.AddDefLocals (static_cast<uint32_t>(CurrentIp), GetStartCmdLineNr (), static_cast<uint32_t>(Tokenizer.GetStartCmdFileOffset ()));
}

int cParser::RemoveEndDefLocals (void)
{
    uint32_t CorrespondingDefLocalsIp;
    int CorrespondingDefLocalsLineNr;
    uint32_t CorrespondingFileOffset;

    return CompilerTree.RemoveEndDefLocals (&CorrespondingDefLocalsIp, &CorrespondingDefLocalsLineNr, 
                                            &CorrespondingFileOffset, this) ||
           (InsideProc->EndDefLocals (CurrentIp) < 0);
}

int cParser::AddLabel (char *par_LabelName, int par_Ip, uint32_t par_LineNr, uint32_t par_FileOffset)
{
    if (InsideProc == nullptr) {
        Error (SCRIPT_PARSER_FATAL_ERROR, "cannot add label \"%s\", not inside a procedure", par_LabelName);
        return -1;
    }
    return InsideProc->AddLabel (par_LabelName, CurrentFileNr, par_Ip, par_LineNr, par_FileOffset);
}

int cParser::AddGoto (char *par_LabelName, int par_Ip, uint32_t par_LineNr, uint32_t par_FileOffset)
{
    if (InsideProc == nullptr) {
        Error (SCRIPT_PARSER_FATAL_ERROR, "cannot add label \"%s\", not inside a procedure", par_LabelName);
        return -1;
    }
    return InsideProc->AddGoto (par_LabelName, CurrentFileNr, par_Ip, par_LineNr, par_FileOffset);
}

int cParser::GetLineNrOfLabel (char *par_LabelName)
{
    if (InsideProc == nullptr) {
        Error (SCRIPT_PARSER_FATAL_ERROR, "cannot add label \"%s\", not inside a procedure", par_LabelName);
        return -1;
    }
    return InsideProc->GetLineNrOfLabel (par_LabelName, CurrentFileNr);
}


int cParser::AddParamListToParams (cTokenizer *par_Tokenizer, char *par_ParamListName)
{
    if (SyntaxOrRunningFlag) {
        return ParamList.ScriptInsertParameterList (par_ParamListName, par_Tokenizer, this);
    } else {
        return 0;
    }
}

int cParser::AddNewParamList (void)
{
    if (SyntaxOrRunningFlag) {
        return ParamList.AddNewParamList (this);
    } else {
        return 0;
    }
}

int cParser::DelParamList (char *par_ParamListName)
{
    if (SyntaxOrRunningFlag) {
        return ParamList.ScriptDeleteParamList (par_ParamListName);
    } else {
        return 0;
    }
}

int cParser::AddParamsToParamList (void)
{
    if (SyntaxOrRunningFlag) {
        return ParamList.AddParamsToParamList (this);
    } else {
        return 0;
    }
}

int cParser::DelParamsFromParamList (void)
{
    if (SyntaxOrRunningFlag) {
        return ParamList.DelParamsFromParamList (this);
    } else {
        return 0;
    }
}

int cParser::SolveEquationForParameter (int par_ParamIdx, double *ret_Result, int StackPos)
{
    int Ret;
    char *ErrorString;

    Ret = direct_solve_equation_script_stack_err_string (Tokenizer.GetParameter (par_ParamIdx), 
                                                         ret_Result,
                                                         &ErrorString, StackPos, flag_add_bbvari_automatic);
    if (Ret) {
        Error ((flag_add_bbvari_automatic == ADD_BBVARI_AUTOMATIC_STOP) ? SCRIPT_PARSER_FATAL_ERROR : SCRIPT_PARSER_ERROR_CONTINUE, 
               "in equation: %s", ErrorString);
        FreeErrStringBuffer (&ErrorString);
    }
    return Ret;
}


int cParser::SolveEquationForParameter (int par_ParamIdx, union FloatOrInt64 *ret_ResultValue, int *ret_ResultType, int StackPos)
{
    int Ret;
    char *ErrorString;

    Ret = direct_solve_equation_script_stack_err_string_float_or_int64 (Tokenizer.GetParameter (par_ParamIdx),
                                                                        ret_ResultValue, ret_ResultType,
                                                                        &ErrorString, StackPos, flag_add_bbvari_automatic);
    if (Ret) {
        Error ((flag_add_bbvari_automatic == ADD_BBVARI_AUTOMATIC_STOP) ? SCRIPT_PARSER_FATAL_ERROR : SCRIPT_PARSER_ERROR_CONTINUE,
               "in equation: %s", ErrorString);
        FreeErrStringBuffer (&ErrorString);
    }
    return Ret;
}

int cParser::SolveEquationForParameter (int par_ParamIdx, int *ret_Result, int StackPos)
{
    int Ret;
    double Result;
    char *ErrorString;

    Ret = direct_solve_equation_script_stack_err_string (Tokenizer.GetParameter (par_ParamIdx), 
                                                         &Result,
                                                         &ErrorString, StackPos, flag_add_bbvari_automatic);
    if (Ret) {
        Error ((flag_add_bbvari_automatic == ADD_BBVARI_AUTOMATIC_STOP) ? SCRIPT_PARSER_FATAL_ERROR : SCRIPT_PARSER_ERROR_CONTINUE, 
               "in equation: %s", ErrorString);
        FreeErrStringBuffer (&ErrorString);
    }
    if (Result < 0) {
        *ret_Result = (Result < static_cast<double>(INT32_MIN)) ? INT32_MIN : static_cast<int>(Result - 0.5);
    } else {
        *ret_Result = (Result > static_cast<double>(INT32_MAX)) ? INT32_MAX : static_cast<int>(Result + 0.5);
    }
    return Ret;
}

char *cParser::FormatMessageOutputString (int par_FormatStringPos)
{
    int Size;

    while ((Size = FormatMessageOutput (this, par_FormatStringPos, MessageOutputBuffer, SizeofMessageOutputBuffer, nullptr, 0, VerboseMessage)) >= SizeofMessageOutputBuffer) {
        SizeofMessageOutputBuffer = Size + 1;
        if (MessageOutputBuffer != nullptr) my_free (MessageOutputBuffer);
        MessageOutputBuffer = static_cast<char*>(my_malloc (static_cast<size_t>(SizeofMessageOutputBuffer)));
        if (MessageOutputBuffer == nullptr) return nullptr;
    }
    return MessageOutputBuffer;
}

char *cParser::FormatMessageOutputStringNoPrefix (int par_FormatStringPos)
{
    int Size;

    while ((Size = FormatMessageOutput (this, par_FormatStringPos, MessageOutputBuffer, SizeofMessageOutputBuffer, nullptr, 0, VERBOSE_MESSAGE_PREFIX_OFF)) >= SizeofMessageOutputBuffer) {
        SizeofMessageOutputBuffer = Size + 1;
        if (MessageOutputBuffer != nullptr) my_free (MessageOutputBuffer);
        MessageOutputBuffer = static_cast<char*>(my_malloc (static_cast<size_t>(SizeofMessageOutputBuffer)));
        if (MessageOutputBuffer == nullptr) return nullptr;
    }
    return MessageOutputBuffer;
}

int cParser::SytaxCheckMessageOutput (int par_FormatStringPos)
{
    int FormatSpecifierCount;

    // To check the count of format spezifier is not usefull by parameter lists
    if (Tokenizer.HasCmdParamList ()) return 0;

    FormatMessageOutput (this, par_FormatStringPos, nullptr, 0, &FormatSpecifierCount, ONLY_COUNT_SPEZIFIER_FLAG, VERBOSE_MESSAGE_PREFIX_OFF);
    if (FormatSpecifierCount != GetParameterCounter () - 1 - par_FormatStringPos) {
        Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting %i parameter because there are %i format specifier",
               FormatSpecifierCount + par_FormatStringPos, FormatSpecifierCount);     
        return -1;
    }
    return 0;
}

void cParser::Reset (void)
{
    SyntaxOrRunningFlag = 0;

    State = PARSER_STATE_IDLE;

    CurrentIp = 0;
    CurrentFileNr = 0;
    CurrentScript = nullptr;
    cFileCache::ClearCache ();

    ErrorCounter = 0;
    
    if (CmdTable != nullptr) CmdTable->Reset ();

    InsideProc = nullptr;

    ProcStack.Reset (); 

    if (ProcTable != nullptr) ProcTable->Reset ();

    CompilerTree.Reset (this);

    Tokenizer.Reset ();

    FileStack.Reset ();
    ToParseFileStack.Reset ();

    CmdHasParameters = 0;

    ParamList.Reset ();

    VerboseMessage = VERBOSE_OFF;

    flag_add_bbvari_automatic = TRUE;

    cScriptErrorMsgDlg::ScriptErrorMsgDlgReset ();

    AtomicCounter = 0;
}

void cParser::PrintDebugFile (const char * const Format, ...)
{
    va_list Args;

    if (DebugFile != nullptr) {
        va_start (Args, Format);
        vfprintf (DebugFile, Format, Args);
        va_end (Args);
        fflush (DebugFile);
    }
}


int cParser::SendMessageToProcess (const char *par_Message, int par_Len, const char *par_Processname, int par_MessageId)
{
    int pid;
    if ((pid = get_pid_by_name (par_Processname)) < 0) {
        Error (SCRIPT_PARSER_FATAL_ERROR, "Process '%s' not found", par_Processname);
        return -1;
    } else {
        if (write_message (pid, par_MessageId, par_Len, par_Message) < 0) {
            Error (SCRIPT_PARSER_FATAL_ERROR, "Messagequeue-overflow");
            return -1;
        }
    }
    return 0;
}

int cParser::CheckIfCommandInsideAtomicAllowed (int par_CmdIdx)
{
    int Ret = Tokenizer.IsCommandInsideAtomicAllowed (par_CmdIdx);
    if (!Ret) {
        Error (SCRIPT_PARSER_FATAL_ERROR, "command \"%s\" not allowed inside ATOMIC END_ATOMIC block, corresponding ATOMIC found at %s (%i)", Tokenizer.GetCommandName (par_CmdIdx),
               cFileCache::GetFilenameByNr (FileNrCorrespondingAtomic), LineNrCorrespondingAtomic);
        return -1;

    }
    return 0;
}
