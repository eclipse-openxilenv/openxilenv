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


extern "C" {
#include "MyMemory.h"
#include "StringMaxChar.h"
}


extern "C" {
#include "MainValues.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
}

#include "Parser.h"
#include "FormatMessageOutput.h"

int FormatMessageOutput (cParser *par_Parser, int par_FormatStringPos, char *par_OutputBuffer, int par_SizeofOutputBuffer, int *ret_FormatSpezifier, int par_Flags, int par_Verbose)
{
    int FormatSpecifierPos = par_FormatStringPos + 1;
    int FormatSpecifierCounter = 0;
    char *Dst = par_OutputBuffer;
    double Value;
    int Len = 0;
    int h = 0;
    char Help[512+1];  // an enum must fit into it

    if (par_Parser->GetParameterCounter () <= par_FormatStringPos) {
        if (Dst != nullptr) {
            *Dst = 0;
        }
        if (ret_FormatSpezifier != nullptr) *ret_FormatSpezifier = -1;
        return 0;
    }
    switch (par_Verbose) {
    case VERBOSE_OFF:
        if ((par_Flags & ONLY_COUNT_SPEZIFIER_FLAG) != ONLY_COUNT_SPEZIFIER_FLAG) {
            h = sprintf  (Help, "MESSAGE: "); 
            Len += h;
            if (Len >= par_SizeofOutputBuffer) Dst = nullptr;
            if (Dst != nullptr) {
                MEMCPY (Dst, Help, static_cast<size_t>(h));
                Dst += h;
            }
        }
        break;
    case VERBOSE_ON:
        if ((par_Flags & ONLY_COUNT_SPEZIFIER_FLAG) != ONLY_COUNT_SPEZIFIER_FLAG) {
            h = sprintf  (Help, "cycle:%08u:MESSAGE: ", read_bbvari_udword (CycleCounterVid));
            Len += h;
            if (Len >= par_SizeofOutputBuffer) Dst = nullptr;
            if (Dst != nullptr) {
                MEMCPY (Dst, Help, static_cast<size_t>(h));
                Dst += h;
            }
        }
        break;
    case VERBOSE_MESSAGE_PREFIX_CYCLE_COUNTER:
        if ((par_Flags & ONLY_COUNT_SPEZIFIER_FLAG) != ONLY_COUNT_SPEZIFIER_FLAG) {
            h = sprintf  (Help, "%08u: ", read_bbvari_udword (CycleCounterVid));
            Len += h;
            if (Len >= par_SizeofOutputBuffer) Dst = nullptr;
            if (Dst != nullptr) {
                MEMCPY (Dst, Help, static_cast<size_t>(h));
                Dst += h;
            }
        }
        break;
    case VERBOSE_MESSAGE_PREFIX_OFF:
        // tue nix
        break;
    }
    char *Src = par_Parser->GetParameter (par_FormatStringPos);
    while (*Src != 0) {
        if (*Src == '%') {
            Src++;
            char Format[32];
            Format[0] = '%';

            char *fd = &(Format[1]);
            char *fs = Src;

            // Are there a width/precison definition  between % and z/d/u/x
            while (((*fs >= '0') && (*fs <= '9')) || (*fs == '.')) {
                if ((fd - Format) >= static_cast<int>(sizeof (Format)-2)) {
                    *fd = 0;
                    goto __NO_FORMAT_SPECIFIER;
                }
                *fd++ = *fs++;
            }
            *fd = 0;

            switch (*fs) {
            case '%':    // A % character should be printed (%% inside the string)
                if ((fd - Format) != 2) {
                    if (Dst != nullptr) {
                        *Dst++ = '%';
                    }
                    fs++;
                    Src = fs;
                } else {
                    goto __NO_FORMAT_SPECIFIER;
                }
                break;
            case 'z':
            case 'g':
            case 'f':
                if (FormatSpecifierPos < par_Parser->GetParameterCounter ()) {
                    if ((par_Flags & ONLY_COUNT_SPEZIFIER_FLAG) != ONLY_COUNT_SPEZIFIER_FLAG) {
                        if (par_Parser->SolveEquationForParameter (FormatSpecifierPos, &Value, -1)) {
                            return -1;
                        }
                        Help[0] = ' ';
                        switch(*fs) {
                        case 'g':
                        case 'z':
                            *fd++ = 'g';
                            break;
                        case 'f':
                            *fd++ = 'f';
                            break;
                        }
                        *fd = 0;
                        h = 1 + sprintf (Help+1, Format, Value);
                        Len += h;
                        if (Len >= par_SizeofOutputBuffer) Dst = nullptr;
                        if (Dst != nullptr) {
                            MEMCPY (Dst, Help, static_cast<size_t>(h));
                            Dst += h;
                        }
                    }
                }
                FormatSpecifierPos++;
                FormatSpecifierCounter++;
                fs++;
                Src = fs;
                break;
            case 'd':  // Dec output
            case 'u':  // Dec output
            case 'x':  // Hex output
                if (FormatSpecifierPos < par_Parser->GetParameterCounter ()) {
                    if ((par_Flags & ONLY_COUNT_SPEZIFIER_FLAG) != ONLY_COUNT_SPEZIFIER_FLAG) {
                        union FloatOrInt64 ResultValue;
                        int ResultType;
                        if (par_Parser->SolveEquationForParameter (FormatSpecifierPos, &ResultValue, &ResultType, -1)) {
                            return -1;
                        }
                        switch (ResultType) {
                        case FLOAT_OR_INT_64_TYPE_INT64:
                            switch(*fs) {
                            case 'd':  // Dec output
                                *fd++ = 'l';
                                *fd++ = 'l';
                                *fd++ = 'd';
                                *fd = 0;
                                h = sprintf (Help, Format, ResultValue.qw);
                                break;
                            case 'u':  // Dec output
                                *fd++ = 'l';
                                *fd++ = 'l';
                                *fd++ = 'u';
                                *fd = 0;
                                h = sprintf (Help, Format, (uint64_t)ResultValue.qw);
                                break;
                            case 'x':  // Hex output
                                *fd++ = 'l';
                                *fd++ = 'l';
                                *fd++ = 'X';
                                Help[0] = '0';
                                Help[1] = 'x';
                                *fd = 0;
                                h = 2 + sprintf (Help+2, Format, (uint64_t)ResultValue.qw);
                                break;
                            }
                            break;
                        case FLOAT_OR_INT_64_TYPE_UINT64:
                            switch(*fs) {
                            case 'd':  // Dec output
                                *fd++ = 'l';
                                *fd++ = 'l';
                                *fd++ = 'd';
                                *fd = 0;
                                h = sprintf (Help, Format, (int64_t)ResultValue.uqw);
                                break;
                            case 'u':  // Dec output
                                *fd++ = 'l';
                                *fd++ = 'l';
                                *fd++ = 'u';
                                *fd = 0;
                                h = sprintf (Help, Format, ResultValue.uqw);
                                break;
                            case 'x':  // Hex output
                                *fd++ = 'l';
                                *fd++ = 'l';
                                *fd++ = 'X';
                                Help[0] = '0';
                                Help[1] = 'x';
                                *fd = 0;
                                h = 2 + sprintf (Help+2, Format, ResultValue.uqw);
                                break;
                            }
                            break;
                        case FLOAT_OR_INT_64_TYPE_F64:
                            switch(*fs) {
                            case 'd':  // Dec output
                                *fd++ = 'l';
                                *fd++ = 'l';
                                *fd++ = 'd';
                                *fd = 0;
                                h = sprintf (Help, Format, (int64_t)ResultValue.d);
                                break;
                            case 'u':  // Dec output
                                *fd++ = 'l';
                                *fd++ = 'l';
                                *fd++ = 'u';
                                *fd = 0;
                                h = sprintf (Help, Format, ResultValue.d);
                                break;
                            case 'x':  // Hex output
                                *fd++ = 'l';
                                *fd++ = 'l';
                                *fd++ = 'X';
                                Help[0] = '0';
                                Help[1] = 'x';
                                *fd = 0;
                                h = 2 + sprintf (Help+2, Format, ResultValue.d);
                                break;
                            }
                            break;
                        case FLOAT_OR_INT_64_TYPE_INVALID:
                        default:
                            h = sprintf (Help, "error");
                            break;
                        }
                        Len += h;
                        if (Len >= par_SizeofOutputBuffer) Dst = nullptr;
                        if (Dst != nullptr) {
                            MEMCPY (Dst, Help, static_cast<size_t>(h));
                            Dst += h;
                        }
                    }
                }
                FormatSpecifierPos++;
                FormatSpecifierCounter++;
                fs++;
                Src = fs;
                break;
            case 's':
                if (FormatSpecifierPos < par_Parser->GetParameterCounter ()) {
                    if ((par_Flags & ONLY_COUNT_SPEZIFIER_FLAG) != ONLY_COUNT_SPEZIFIER_FLAG) {
                        char *VariableName = par_Parser->GetParameter (FormatSpecifierPos);
                        h = 0;
                        if (VariableName != nullptr) {
                            int Vid = get_bbvarivid_by_name(VariableName);
                            if (Vid > 0) {
                                int Color;
                                Help[0] = ' ';
                                if (read_bbvari_textreplace (Vid, Help + 1, sizeof(Help) - 1, &Color) == 0) {
                                    h = (int)strlen(Help) + 1;
                                }
                            }
                        }
                        if (h == 0) {  // no valid enum print value instead
                            if (par_Parser->SolveEquationForParameter (FormatSpecifierPos, &Value, -1)) {
                                return -1;
                            }
                            Help[0] = ' ';
                            h = 1 + sprintf (Help+1, "%g", Value);
                        }
                        Len += h;
                        if (Len >= par_SizeofOutputBuffer) Dst = nullptr;
                        if (Dst != nullptr) {
                            MEMCPY (Dst, Help, static_cast<size_t>(h));
                            Dst += h;
                        }
                    }
                }
                FormatSpecifierPos++;
                FormatSpecifierCounter++;
                fs++;
                Src = fs;
                break;

            default:   // If there are no valid format spezifier then takeover the %- character
            __NO_FORMAT_SPECIFIER:
                Len += 1;
                if (Len >= par_SizeofOutputBuffer) Dst = nullptr;
                if (Dst != nullptr) {
                    *Dst++ = '%';
                }
                break;
            }
        } else {
            Len++;
            if (Len >= par_SizeofOutputBuffer) Dst = nullptr;
            if (Dst != nullptr) {
                *Dst++ = *Src++;
            } else {
                Src++;
            }
        }
    }
    Len++;
    if (Len >= par_SizeofOutputBuffer) Dst = nullptr;
    if (Dst != nullptr) {
        *Dst = 0;
    }
    if (ret_FormatSpezifier != nullptr) *ret_FormatSpezifier = FormatSpecifierCounter;
    return Len;
}
