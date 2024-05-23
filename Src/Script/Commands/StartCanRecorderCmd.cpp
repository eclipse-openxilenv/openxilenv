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


#include "Platform.h"
#include <stdio.h>

extern "C" {
#include "Config.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Message.h"
#include "CanRecorder.h"
#include "InterfaceToScript.h"
}

#include "Parser.h"
#include "BaseCmd.h"

DEFINE_CMD_CLASS(cStarCanRecorderCmd)


int cStarCanRecorderCmd::SyntaxCheck (cParser *par_Parser)
{
    UNUSED(par_Parser);
    return 0;
}

static void StringCopy(char *Dst, const char *Src, int MaxC)
{
    int Len = (int)strlen(Src);
    if (Len >= (MaxC-1)) Len = MaxC-1;
    MEMCPY(Dst, Src, Len);
    Dst[Len] = 0;
}

int cStarCanRecorderCmd::Execute (cParser *par_Parser, cExecutor *par_Executor)
{
    char Path[MAX_PATH];
    CAN_ACCEPT_MASK AcceptanceMasks[MAX_CAN_ACCEPTANCE_MASK_ELEMENTS+1];   // the last one must set cahnnel to -1
    int AcceptanceMaskCount = 0;
    int DisplayColumnCounterFlag = 0;
    int DisplayColumnTimeAbsoluteFlag = 0;
    int DisplayColumnTimeDiffFlag = 0;
    int DisplayColumnTimeDiffMinMaxFlag = 0;
    int x;

    if (par_Executor->GetCanRecorder() != nullptr) {
        par_Parser->Error (SCRIPT_PARSER_WARNING, "CAN recorder already running (stopped)");
        StopCanRecorder(par_Executor->GetCanRecorder());
        par_Executor->SetCanRecorder(nullptr);
    }
    StringCopy (Path, par_Parser->GetParameter(0), MAX_PATH);
    ScriptIdentifyPath (Path);

    for (x = 2; x < par_Parser->GetParameterCounter(); x++) {
        char *p = par_Parser->GetParameter(x);
        if (!stricmp(p, "COUNTER")) {
            DisplayColumnCounterFlag = 1;
        } else if (!stricmp(p, "TIME")) {
            DisplayColumnTimeAbsoluteFlag = 1;
        } else if (!stricmp(p, "DIFF_TIME")) {
            DisplayColumnTimeDiffFlag = 1;
        } else if (!stricmp(p, "DIFF_TIME_MIN_MAX")) {
            DisplayColumnTimeDiffMinMaxFlag = 1;
        } else break;
    }
    // now following the masks
    for ( ; (x + 2) < par_Parser->GetParameterCounter(); x+=3) {
        double Value;
        int Channel;
        uint32_t Start, Stop;
        if (AcceptanceMaskCount > MAX_CAN_ACCEPTANCE_MASK_ELEMENTS) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "only %i acceptance masks allowed", MAX_CAN_ACCEPTANCE_MASK_ELEMENTS);
            return -1;
        }
        if (par_Parser->SolveEquationForParameter (x, &Value, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...",
                               par_Parser->GetParameter(x));
            return -1;
        }
        Channel = (int)Value;
        if ((Channel < 0) || (Channel >= 8)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "Channel must be in the range (0...7) not %i", Channel);
            return -1;
        }
        if (par_Parser->SolveEquationForParameter (x + 1, &Value, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...",
                               par_Parser->GetParameter(x + 1));
            return -1;
        }
        Start = (uint32_t)Value;
        if (par_Parser->SolveEquationForParameter (x + 2, &Value, -1)) {
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "equation '%s' contains error ! Perhaps a variable does not exist...",
                               par_Parser->GetParameter(x + 2));
            return -1;
        }
        Stop = (uint32_t)Value;
        AcceptanceMasks[AcceptanceMaskCount].Channel = Channel;
        AcceptanceMasks[AcceptanceMaskCount].Start = Start;
        AcceptanceMasks[AcceptanceMaskCount].Stop = Stop;
        AcceptanceMasks[AcceptanceMaskCount].Fill1 = 0;
        AcceptanceMaskCount++;
    }
    AcceptanceMasks[AcceptanceMaskCount].Channel = -1;;
    AcceptanceMasks[AcceptanceMaskCount].Start = 0;
    AcceptanceMasks[AcceptanceMaskCount].Stop = 0;
    AcceptanceMasks[AcceptanceMaskCount].Fill1 = 0;
    AcceptanceMaskCount++;


    struct CAN_RECORDER *Rec = SetupCanRecorder(Path,
                                                par_Parser->GetParameter(1),  // Trigger equation
                                                DisplayColumnCounterFlag,
                                                DisplayColumnTimeAbsoluteFlag,
                                                DisplayColumnTimeDiffFlag,
                                                DisplayColumnTimeDiffMinMaxFlag,
                                                AcceptanceMasks,
                                                AcceptanceMaskCount);
    if (Rec == nullptr) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot start CAN recorder \"%s\"", Path);
        return -1;
    }
    par_Executor->SetCanRecorder(Rec);

    return 0;
}

int cStarCanRecorderCmd::Wait (cParser *par_Parser, cExecutor *par_Executor, int Cycles)
{
    UNUSED(par_Parser);
    UNUSED(par_Executor);
    UNUSED(Cycles);
    return 0;
    }

static cStarCanRecorderCmd StartCanRecorderCmd ("START_CAN_RECORDER",
                                                6,
                                                MAX_PARAMETERS,
                                                nullptr,
                                                FALSE,
                                                FALSE,
                                                0,
                                                0,
                                                CMD_INSIDE_ATOMIC_ALLOWED);

