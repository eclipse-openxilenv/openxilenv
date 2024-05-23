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
#include <stdio.h>

extern "C" {
#include "MyMemory.h"
#include "Blackboard.h"
#include "EquationParser.h"
#include "Scheduler.h"
#include "EquationList.h"
#include "ScriptMessageFile.h"
}
#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cWaitUntilCmd)

 enum TimeoutActionEnum {CONTINUE, STOP, GOTO, GOSUB, CALL_PROC};

int cWaitUntilCmd::SyntaxCheck (cParser *par_Parser)
{
    enum TimeoutActionEnum TimeoutAction = CONTINUE;   // Continue
    int TimeoutMessagePos;

    if (par_Parser->GetParameterCounter () >= 3) {  // STOP, oder CONT bzw CONTINUE
        if (!stricmp (par_Parser->GetParameter (2), "STOP")) {
            TimeoutAction = STOP;
            TimeoutMessagePos = 3;
        } else if (!strnicmp (par_Parser->GetParameter (2), "CONTINUE", 4)) {
            TimeoutAction = CONTINUE;
            TimeoutMessagePos = 3;
        } else if (!stricmp (par_Parser->GetParameter (2), "GOTO")) {
            TimeoutAction = GOTO;
            TimeoutMessagePos = 4;
            // begin nearly the same as in cGotoCmd::SyntaxCheck
            int Idx;
            int FileOffset;
            int LineNr;

            FileOffset = par_Parser->GetCurrentScriptFile ()->Ftell (&LineNr);
            if (par_Parser->GetNumNoneSolvedEnvVars (0) || par_Parser->GetNumSolvedEnvVars (0)) {
                // GOTO with unsolved environment variables will throw a warning
                if (par_Parser->GetNumNoneSolvedEnvVars (0)) {
                    par_Parser->Error (SCRIPT_PARSER_WARNING, "WAIT_UNTIL with GOTO \"%s\" which includes an unknown environment variable at compile time, try to resolve it at run time", par_Parser->GetParameter (0));
                }
                Idx = par_Parser->AddGoto (nullptr, par_Parser->GetCurrentIp (),
                                           static_cast<uint32_t>(LineNr),
                                           static_cast<uint32_t>(FileOffset));
            } else {
                Idx = par_Parser->AddGoto (par_Parser->GetParameter (3), par_Parser->GetCurrentIp (),
                                           static_cast<uint32_t>(LineNr),
                                           static_cast<uint32_t>(FileOffset));
            }
            par_Parser->SetOptParameter (static_cast<uint32_t>(Idx), 0);
            if (Idx < 0) return -1;
            // end nearly the same as in cGotoCmd::SyntaxCheck

        } else if (!stricmp (par_Parser->GetParameter (2), "GOSUB")) {
            TimeoutAction = GOSUB;
            TimeoutMessagePos = 4;
            // begin nearly the same as in cGosubCmd::SyntaxCheck
            int Idx;
            int FileOffset;
            int LineNr;

            FileOffset = par_Parser->GetCurrentScriptFile ()->Ftell (&LineNr);
            // if there are a unsolved environment variables, try to solve it now
            if (par_Parser->GetNumNoneSolvedEnvVars (0) || par_Parser->GetNumSolvedEnvVars (0)) {
                // GOSUB with unsolved environment variables will throw a warning
                if (par_Parser->GetNumNoneSolvedEnvVars (0)) {
                    par_Parser->Error (SCRIPT_PARSER_WARNING, "WAIT_UNTIL with GOSUB \"%s\" which includes an unknown environment variable at compile time, try to resolve it at run time", par_Parser->GetParameter (0));
                }
                Idx = par_Parser->AddGoto (nullptr, par_Parser->GetCurrentIp (),
                                           static_cast<uint32_t>(LineNr),
                                           static_cast<uint32_t>(FileOffset));
            } else {
                Idx = par_Parser->AddGoto (par_Parser->GetParameter (3), par_Parser->GetCurrentIp (),
                                           static_cast<uint32_t>(LineNr),
                                           static_cast<uint32_t>(FileOffset));
            }
            par_Parser->SetOptParameter (static_cast<uint32_t>(Idx), 0);
            if (Idx < 0) return -1;
            // end nearly the same as in cGosubCmd::SyntaxCheck
        } else if (!stricmp (par_Parser->GetParameter (2), "CALL_PROC")) {
            TimeoutAction = CALL_PROC;
            TimeoutMessagePos = 4;
            // begin nearly the same as in cCallCmd::SyntaxCheck
            cProc *Proc = par_Parser->SerchProcByName (par_Parser->GetParameter (3));  // 3 == Procedure name
            if (Proc == nullptr) {
                par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "call unknown Proc \"%s\"", par_Parser->GetParameter (3));
                return -1;
            } else {
                // The additional parameter of the procedure are between procedure name and the timeout message
                int TimeoutMessageFlag;
                if ((par_Parser->GetParameterCounter() - Proc->GetParamCount()) > 4) {
                    TimeoutMessagePos += Proc->GetParamCount ();
                    TimeoutMessageFlag = 1;
                } else {
                    TimeoutMessagePos = -1;  // no timeout message
                    TimeoutMessageFlag = 0;
                }
                if (Proc->GetParamCount () != (par_Parser->GetParameterCounter () - (4 + TimeoutMessageFlag))) {
                    par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "wrong parameter count %i (expecting %i) in proc \"%s\"",
                                       par_Parser->GetParameterCounter () - 5, Proc->GetParamCount (), par_Parser->GetParameter (3));
                    return -1;
                }

                for (int x = 4; x < par_Parser->GetParameterCounter () - 1; x++) {  // 1. Condition / 2. Timeout / 3. What to do, / 4. Procedure name / 5. ... n. Parameter of the calling procedure / n. Timeout message
                    char *Parameter = par_Parser->GetParameter (x);
                    if (*Parameter == '&') {   // it is a reference
                        if (Proc->IsParamRefOrValue (x - 4) != 1 /*REFERENCE_PARAMETER*/) {
                            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "parameter numer %i \"%s\" should be a value and not a reference", x, Parameter);
                        }
                    } else {
                        if (Proc->IsParamRefOrValue (x - 4) != 0 /*VALUE_PARAMETER*/) {
                            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "parameter numer %i \"%s\" should be a reference and not a value", x, Parameter);
                        }
                    }
                }
                // The jump address will be stored inside OptParameter of the Cmd table
                par_Parser->SetOptParameter (static_cast<uint32_t>(Proc->GetProcIdx ()), 0);
            }
            // begin nearly the same as in cCallCmd::SyntaxCheck
        } else {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "the third parameter of the WAIT_UNTIL command must be STOP, CONT, GOTO, GOSUB or CALL and not \"%s\"", par_Parser->GetParameter (3));
            return -1;
        }
        if ((TimeoutMessagePos >= 0) &&
            (par_Parser->GetParameterCounter () >= (TimeoutMessagePos + 1))) {  // Timeout message?
            par_Parser->SytaxCheckMessageOutput (TimeoutMessagePos);
        }
    }

    return 0;
}

int cWaitUntilCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    int WaitUntilCycles;
    enum TimeoutActionEnum TimeoutAction = CONTINUE;   // Continue
    int TimeoutMessagePos;

    int MsgPos = -1;

    if (par_Parser->GetParameterCounter () >= 2) { // Timeout set?
        if (par_Parser->SolveEquationForParameter (1, &WaitUntilCycles , -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Equation \"%s\" contains error! Perhaps a variable does not exist...", par_Parser->GetParameter (1));
            return -1;
        }
        par_Executor->SetData (static_cast<int>(GetCycleCounter ()) + WaitUntilCycles);

        if (par_Parser->GetParameterCounter () >= 3) {  // STOP, or CONT or CONTINUE
            if (!stricmp (par_Parser->GetParameter (2), "STOP")) {
                TimeoutAction = STOP;
                TimeoutMessagePos = 3;
            } else if (!strnicmp (par_Parser->GetParameter (2), "CONTINUE", 4)) {
                TimeoutAction = CONTINUE;
                TimeoutMessagePos = 3;
            } else if (!stricmp (par_Parser->GetParameter (2), "GOTO")) {
                TimeoutAction = GOTO;
                TimeoutMessagePos = 4;
            } else if (!stricmp (par_Parser->GetParameter (2), "GOSUB")) {
                TimeoutAction = GOSUB;
                TimeoutMessagePos = 4;
            } else if (!stricmp (par_Parser->GetParameter (2), "CALL_PROC")) {
                TimeoutAction = CALL_PROC;
                int NewProcIdx = static_cast<int>(par_Executor->GetOptParameterCurrentCmd ());
                cProc *Proc = par_Executor->GetProcByIdx (NewProcIdx);
                if ((par_Parser->GetParameterCounter() - Proc->GetParamCount()) > 4) {
                    TimeoutMessagePos = par_Parser->GetParameterCounter () - 1;
                } else {
                    TimeoutMessagePos = -1;  // no timeout message
                }
            } else {
                TimeoutAction = CONTINUE;
                TimeoutMessagePos = 3;
            }
            if ((TimeoutMessagePos >= 0) &&
                (par_Parser->GetParameterCounter () >= (TimeoutMessagePos + 1))) {  // Timeoutmessage?
                MsgPos = TimeoutMessagePos;
            }
        }
    } else {
        par_Executor->SetData (-1);
    }
    par_Executor->SetData (0, reinterpret_cast<uint64_t>(par_Parser->GetParameter (0)));
    par_Executor->SetData (1, static_cast<uint64_t>(TimeoutAction));
    par_Executor->SetData (2, static_cast<uint64_t>(MsgPos));

    return 0;
}

int cWaitUntilCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(Cycles);
    double Erg;
    char *ErrString;
    if (direct_solve_equation_script_stack_err_string (reinterpret_cast<char*>(par_Executor->GetData (0)), &Erg, &ErrString, -1, par_Parser->get_flag_add_bbvari_automatic ())) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Equation \"%s\" contains error! Perhaps %s", reinterpret_cast<char*>(par_Executor->GetData (0)), ErrString);
        FreeErrStringBuffer (&ErrString);
        return 0;
    }
    if (static_cast<int>(Erg + 0.5) != 0) {
        if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("/* WAIT_UNTIL left after %i cycles */\n", par_Executor->GetWaitCyclesNeeded ());
        return 0;
    } else {
        uint32_t WaitUntilEndCycle = static_cast<uint32_t>(par_Executor->GetData ());
        uint32_t ActualCycle = GetCycleCounter ();
        if (WaitUntilEndCycle <= ActualCycle) {
            if (par_Parser->GenDebugFile ()) par_Parser->PrintDebugFile ("/* WAIT_UNTIL timeout after %i cycles */\n", par_Executor->GetWaitCyclesNeeded ());
            int MsgPos = static_cast<int>(par_Executor->GetData (2));
            if (MsgPos >= 0) {
                char *MessageBuffer = par_Parser->FormatMessageOutputString (MsgPos);
                if (MessageBuffer == nullptr) return -1;
                AddScriptMessage (MessageBuffer);
            }
            switch (static_cast<enum TimeoutActionEnum>(par_Executor->GetData (1))) {
            case CONTINUE:
                break;
            case STOP:
                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "script stopped at timeout during WAIT_UNTIL with STOP");
                return -1;
            case GOTO:
                {
                    // begin nearly the same as in cGotoCmd::Execute
                    int FromPos;
                    int ToPos;
                    int GotoIp;
                    int DiffAtomicDepth;

                    int Idx = static_cast<int>(par_Executor->GetOptParameterCurrentCmd ());
                    if (par_Executor->GetGotoFromToDefLocalsPos (Idx, &FromPos, &ToPos, &GotoIp, &DiffAtomicDepth,
                                                                 par_Parser->GetParameter (0), par_Parser->GetCurrentFileNr ())) {
                        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot goto \"%s\" the label include a environment variable but this or the resulting label doesn't exist", par_Parser->GetParameter (0));
                        return -1;
                    }
                    if (DiffAtomicDepth) {
                        if (par_Parser->GenDebugFile ()) {
                            if (DiffAtomicDepth > 0) {
                                par_Parser->PrintDebugFile ("/* jump inside %i ATOMIC block(s) */", DiffAtomicDepth);
                            } else {
                                par_Parser->PrintDebugFile ("/* jump out of %i ATOMIC block(s) */", -DiffAtomicDepth);
                            }
                        }
                    }
                    par_Executor->GotoFromToDefLocalsPos (FromPos, ToPos, DiffAtomicDepth, par_Parser, par_Executor);
                    par_Executor->SetNextIp (GotoIp - 1);
                    // end nearly the same as in cGotoCmd::Execute
                }
                break;
            case GOSUB:
                {
                    // begin nearly the same as in cGosubCmd::Execute
                    int FromPos;
                    int ToPos;
                    int GotoIp;
                    int DiffAtomicDepth;

                    int Idx = static_cast<int>(par_Executor->GetOptParameterCurrentCmd ());
                    if (par_Executor->GetGotoFromToDefLocalsPos (Idx, &FromPos, &ToPos, &GotoIp, &DiffAtomicDepth,
                                                                 par_Parser->GetParameter (0), par_Parser->GetCurrentFileNr ())) {
                        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot gosub \"%s\" the label include a environment variable but this or the resulting label doesn't exist", par_Parser->GetParameter (0));
                        return -1;
                    }
                    DiffAtomicDepth = 0;
                    par_Executor->Stack.AddGosubToStack (par_Parser->GetParameter (0), par_Executor->GetCurrentIp (), par_Executor->GetCurrentAtomicDepth ());
                    par_Executor->GotoFromToDefLocalsPos (FromPos, ToPos, DiffAtomicDepth, par_Parser, par_Executor);
                    par_Executor->SetNextIp (GotoIp - 1);
                    // end nearly the same as in cGosubCmd::Execute
                }
                break;
            case CALL_PROC:
                {
                    // begin nearly the same as in cCallCmd::Execute
                    int x;

                    int CurrentStackPos = par_Executor->Stack.GetCurrentStackPos ();

                    // first store return address onto stack
                    par_Executor->Stack.AddProcToStack (par_Executor->GetCurrentProcIdx (),
                                                        par_Executor->GetNextIp (), // return address
                                                        par_Executor->GetCurrentProcName ());

                    int NewProcIdx = static_cast<int>(par_Executor->GetOptParameterCurrentCmd ());
                    cProc *Proc = par_Executor->SetCurrentProcByIdx (NewProcIdx);
                    // store the parameter onto the stack
                    for (x = 0; x < Proc->GetParamCount (); x++) {
                        if (Proc->GetParameterType (x) == 0 /*VALUE_PARAMETER*/) {
                            double Value;
                            if (par_Parser->SolveEquationForParameter (x + 4, &Value, CurrentStackPos)) { // +1 because the first parameter is the procedure name
                                return -1;
                            }
                            par_Executor->Stack.AddLocalVariableToStack (Proc->GetParameterName (x), Value);
                        } else {
                            if (par_Parser->GetParameter (x + 4)[0] != '&') {
                                par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "expecting a reference and not a value at %i parameter calling procedure %s", x, Proc->GetName ());
                                return -1;
                            } else {
                                int64_t Ref = par_Executor->Stack.GetRefToLocalVariable (par_Parser->GetParameter (x + 4), CurrentStackPos);
                                if (Ref > 0) {
                                    par_Executor->Stack.AddRefToLocalVariableToStack (Proc->GetParameterName (x), Ref);
                                } else {
                                    Ref = add_bbvari (par_Parser->GetParameter (x + 4) + 1, BB_UNKNOWN, nullptr);
                                    if (Ref < 0) {
                                        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot find variable reference %s", par_Parser->GetParameter (x + 4) + 1);
                                        return -1;
                                    }
                                    Ref |= 1LL << 32;
                                    par_Executor->Stack.AddRefToLocalVariableToStack (Proc->GetParameterName (x), Ref);
                                }
                            }
                        }
                    }
                    // end nearly the same as in cCallCmd::Execute
                }
                break;
            }
            return 0;
        } else {
            return static_cast<int>(WaitUntilEndCycle - ActualCycle);
        }
    }
    //return -1;  // never reached
}

static cWaitUntilCmd WaitUntilCmd ("WAIT_UNTIL",                        
                                   1, 
                                   MAX_PARAMETERS,
                                   nullptr,
                                   FALSE, 
                                   TRUE,  
                                   -1,
                                   0,
                                   CMD_INSIDE_ATOMIC_NOT_ALLOWED);
