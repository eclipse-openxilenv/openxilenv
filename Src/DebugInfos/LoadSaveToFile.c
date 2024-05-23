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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <limits.h>

#include "DebugInfos.h"
#include "DebugInfoDB.h"
#include "GetNextStructEntry.h"
#include "Files.h"
#include "Scheduler.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "MainValues.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "DebugInfoAccessExpression.h"
#include "StartExeAndWait.h"
#include "ReplaceFuncWithProg.h"
#include "SectionFilter.h"
#include "WindowIniHelper.h"
#include "UniqueNumber.h"
#include "ReadWriteValue.h"
#include "EquationParser.h"
#include "ScriptMessageFile.h"
#include "LoadSaveToFile.h"
#include "MainWinowSyncWithOtherThreads.h"


extern int conv_error2message;     // flag to switch error mesages to file


#define CHECK_SIZE      0


int file_exists(char *filename)
{
    return (_access(filename, 0) == 0);
}

static BOOL BinStrToDblAndMask(const char *ptr_WertStr,   // String with binary format
                   uint64_t *ptr_IntVal,                  // return value
                   uint64_t *ptr_Mask,                    // return mask/
                   uint32_t *ptr_Cnt                      // return number of bits
                   )
{
    unsigned int i, j=0;

    ptr_WertStr += 2; // jump over "0b" or "0B"

    *ptr_IntVal = 0;
    *ptr_Mask = 0;
    *ptr_Cnt = (uint32_t)strlen(ptr_WertStr); // Count the number of bits

    for (i=0;i<*ptr_Cnt;i++) {
        j = (*ptr_Cnt - i) - 1;
        switch (ptr_WertStr[i]) {
        case '?':
            *ptr_IntVal &= ~(1ULL << j);   // set bit to 0
            *ptr_Mask |= 1ULL << j;        // set bit to 1
            break;
        case '0':
            *ptr_IntVal &= ~(1ULL << j);   // set bit to 0
            *ptr_Mask &= ~(1ULL << j);     // set bit to 0
            break;
        case '1':
            *ptr_IntVal |= 1ULL << j;      // set bit to 1
            *ptr_Mask &= ~(1ULL << j);     // set bit to 0
            break;
        default:
            return FALSE;
        }
    }
    return TRUE;
}


// this function will read value from a byte array based on the data type
// the return value are the number of bytes read out of the byte array or 0
// if the data type is invalid
int  bbvari_value_to_str(char *strg,        // return buffer for the value string
                    int maxc,               // size of the buffer
                    unsigned char *src,     // Byte array
                    int bbtype              // Blackboard data type
                   )
{
    union BB_VARI Value;

    ConvertRawMemoryToUnion(bbtype, src, &Value);
    bbvari_to_string(bbtype, Value, 10, strg, maxc);
    return bbvari_sizeof (bbtype);

}



// this function returns the size of a blackboard data type
int bbvari_sizeof(int bbtype)
{
    switch (bbtype) {
    case BB_BYTE:
    case BB_UBYTE:
        return 1;
    case BB_WORD:
    case BB_UWORD:
        return 2;
    case BB_DWORD:
    case BB_UDWORD:
    case BB_FLOAT:
        return 4;
    case BB_QWORD:
    case BB_UQWORD:
    case BB_DOUBLE:
        return 8;
    default:
        return 0;
    }
}

static int CheckMinMaxRange (union BB_VARI in_value, int in_bbtype, int limit_to_bbtype, union BB_VARI *ret_value,
                            const char *name, BOOL SuppressErrorFctnFlag)
{
    int rc = IDOK;
    int LimitedFlag;

    LimitedFlag = LimitToDataTypeRange(in_value, in_bbtype, limit_to_bbtype, ret_value);

    if (LimitedFlag) {
        if (SuppressErrorFctnFlag == FALSE) {
            union BB_VARI min, max;
            char string_max[64];
            char string_min[64];
            char string_value[64];
            get_datatype_min_max_union(in_bbtype, &min, &max);
            bbvari_to_string(in_bbtype, min, 10, string_min, sizeof(string_min));
            bbvari_to_string(in_bbtype, max, 10, string_max, sizeof(string_max));
            bbvari_to_string(in_bbtype, in_value, 10, string_value, sizeof(string_value));
            rc = ThrowError (ERROR_OK_OKALL_CANCEL,
                "cannot write %s to \"%s\" because value is out of data type value range (%s ... %s)\n"
                        "limit value to %s",
                        string_value, name, string_min, string_max, (LimitedFlag<0) ? string_min : string_max);
        }
    } else return WLTP_OK;
    switch (rc) {
    default:
    case IDOK:
        return WLTP_OK;
    case IDCANCEL:
        return WLTP_CANCEL;
    case IDOKALL:
        return WLTP_OKALL;
    }
}

// This function will write a value to the process
// The return value is WLTP_OK if successful written or error accepted by user
// or WLTP_CANCEL if user has caneled the operation
// or WLTP_OKALL if an error accepted by user and all following error should also accepted
int WriteLabelToProcess(const char *LabelName,  // Name of the label
                    const PID pid,          // Process Id
                    int *PidsSameExe,       // Array of process id living inside the same executable
                    int PidsSameExeCount,
                    const char *ProcName,   // Process name
                    const char *WertStr,    // Value that should be writen as string
                    BOOL  SuppressErrorFctnFlag, // if true do not throw any error
                    PROCESS_APPL_DATA *pappldata
                   )
{
    union BB_VARI bb_vari;
    uint64_t address;
    int32_t typenr;
    int rc;
    union BB_VARI in_value;
    int in_bbtype = 0;
    int ret = WLTP_OK;
	BOOL IsBin=FALSE;
    uint64_t IntVal = 0;
    uint64_t Mask = 0;
    uint32_t Cnt = 0;
    int bbtype;

    in_value.uqw = 0;  // to avoid warning
    if (appl_label_already_locked (pappldata, pid, LabelName, &address, &typenr, 1, 1)) {
        if (SuppressErrorFctnFlag == FALSE) {
            rc = ThrowError (ERROR_OK_OKALL_CANCEL, "Label %s unknown in extern Process %s ! Continue ?", LabelName, ProcName);
            switch (rc) {
              case IDOK:
                if (s_main_ini_val.StopScriptIfLabelDosNotExistInsideSVL) return IDCANCEL;
                else return WLTP_OK;
              case IDCANCEL:
                return WLTP_CANCEL;
              case IDOKALL:
                if (s_main_ini_val.StopScriptIfLabelDosNotExistInsideSVL) return IDCANCEL;
                else return WLTP_OKALL;
            }
        }
    } else {
        if (WertStr[0] == '(') {
            char *errstring;
            union BB_VARI old_value;
            int old_bbtype;

            if (ReadValueFromExternProcessViaAddress (address, pappldata, pid, typenr, &old_value, &old_bbtype, NULL)) {
                ThrowError (1, "Label %s cannot read from extern Process %s", LabelName, ProcName);
                ret = WLTP_CANCEL;
            }

            if (direct_solve_equation_err_string_replace_union (WertStr, old_value, old_bbtype, &in_value, &in_bbtype, &errstring)) {
                ThrowError (1, "Label %s cannot solve equation \"%s\" (%s))", LabelName,  WertStr, errstring);
                my_free (errstring);
                return(WLTP_CANCEL);
            }
        } else if (((WertStr[0] == '0') && ((WertStr[1] == 'b') || (WertStr[1] == 'B'))) && (strstr(WertStr, "?") != NULL)) {
            // If it is a binary string there can be included '?' to mark bit which should be not changend
            if (!BinStrToDblAndMask(WertStr, &IntVal, &Mask, &Cnt)) {
                ThrowError (1, "Value of label (%s = %s) is wrong!", LabelName, WertStr);
                return(WLTP_CANCEL);
            }
            IsBin=TRUE;
        } else {
            enum BB_DATA_TYPES type;
            if (string_to_bbvari_without_type (&type,
                                               &in_value,
                                               WertStr)) {
                ThrowError (1, "cannot convert \"%s\" to a number", WertStr);
            }
            in_bbtype = (int)type;
        }

        bbtype = get_base_type_bb_type_ex (typenr, pappldata);
        switch (bbtype) {
            case BB_BYTE:
				if (IsBin) {
                    ThrowError (1, "Value of label (%s = %s) cannot be written to a signed variable", LabelName, WertStr);
                    ret = WLTP_CANCEL;
					break;
				}
                if ((ret = CheckMinMaxRange (in_value, in_bbtype,  bbtype, &bb_vari, LabelName, SuppressErrorFctnFlag)) != WLTP_CANCEL) {
                    if (scm_write_bytes_pid_array (address, PidsSameExe, PidsSameExeCount,
                                                   (unsigned char*)&(bb_vari.b), 1) != 1)
                    {
                        ThrowError (1, "Label %s cannot write to extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
                    }
                }
                break;
            case BB_UBYTE:
				if (IsBin) {
					if (Cnt > 8) {
						ThrowError (1, "Value of label %s = %s is to long", LabelName, WertStr);
                        ret = WLTP_CANCEL;
						break;
					}
                    // Read value and mask
                    if (scm_read_bytes (address, pid, (char*)&(in_value.ub), 1) != 1)
                    {
                        ThrowError (1, "Label %s cannot read from extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
						break;
                    }
                    in_value.ub &= (unsigned char)Mask;
                    in_value.ub |= (unsigned char)IntVal;
                    in_bbtype = BB_UBYTE;
				}
                if ((ret = CheckMinMaxRange (in_value, in_bbtype,  bbtype, &bb_vari, LabelName, SuppressErrorFctnFlag)) != WLTP_CANCEL) {
                    if (scm_write_bytes_pid_array (address, PidsSameExe, PidsSameExeCount,
                                                   (unsigned char*)&(bb_vari.ub), 1) != 1)
                    {
                        ThrowError (1, "Label %s cannot write to extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
                    }
                }
                break;
            case BB_WORD:
				if (IsBin) {
                    ThrowError (1, "Value of label (%s = %s) cannot be written to a signed variable", LabelName, WertStr);
                    ret = WLTP_CANCEL;
					break;
				}
                if ((ret = CheckMinMaxRange (in_value, in_bbtype,  bbtype, &bb_vari, LabelName, SuppressErrorFctnFlag)) != WLTP_CANCEL) {
                    if (scm_write_bytes_pid_array (address, PidsSameExe, PidsSameExeCount,
                                                   (unsigned char*)&(bb_vari.w), 2) != 2)
                    {
                        ThrowError (1, "Label %s cannot write to extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
                    }
                }
                break;
            case BB_UWORD:
				if (IsBin) {
					if (Cnt > 16) {
						ThrowError (1, "Value of label %s = %s is to long", LabelName, WertStr);
                        ret = WLTP_CANCEL;
						break;
					}
                    // Read value and mask
                    if (scm_read_bytes (address, pid, (char*)&(in_value.uw), 2) != 2)
                    {
                        ThrowError (1, "Label %s cannot read from extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
						break;
                    }
                    in_value.uw &= (unsigned short)Mask;
                    in_value.uw |= (unsigned short)IntVal;
                    in_bbtype = BB_UWORD;
				}
                if ((ret = CheckMinMaxRange (in_value, in_bbtype,  bbtype, &bb_vari, LabelName, SuppressErrorFctnFlag)) != WLTP_CANCEL) {
                    if (scm_write_bytes_pid_array (address, PidsSameExe, PidsSameExeCount,
                                                   (unsigned char*)&(bb_vari.uw), 2) != 2)
                    {
                        ThrowError (1, "Label %s cannot write to extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
                    }
                }
                break;
            case BB_DWORD:
				if (IsBin) {
                    ThrowError (1, "Value of label (%s = %s) cannot be written to a signed variable", LabelName, WertStr);
                    ret = WLTP_CANCEL;
					break;
				}
                if ((ret = CheckMinMaxRange (in_value, in_bbtype,  bbtype, &bb_vari, LabelName, SuppressErrorFctnFlag)) != WLTP_CANCEL) {
                    if (scm_write_bytes_pid_array (address, PidsSameExe, PidsSameExeCount,
                                                   (unsigned char*)&(bb_vari.dw), 4) != 4)
                    {
                        ThrowError (1, "Label %s cannot write to extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
                    }
                }
                break;
            case BB_UDWORD:
				if (IsBin) {
					if (Cnt > 32) {
						ThrowError (1, "Value of label %s = %s is to long", LabelName, WertStr);
                        ret = WLTP_CANCEL;
						break;
					}
                    // Read value and mask
                    if (scm_write_bytes_pid_array (address, PidsSameExe, PidsSameExeCount,
                                                   (unsigned char*)&(in_value.udw), 4) != 4)
                    {
                        ThrowError (1, "Label %s cannot read from extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
						break;
                    }
                    in_value.udw &= (uint32_t)Mask;
                    in_value.udw |= (uint32_t)IntVal;
                    in_bbtype = BB_UDWORD;
				}
                if ((ret = CheckMinMaxRange (in_value, in_bbtype,  bbtype, &bb_vari, LabelName, SuppressErrorFctnFlag)) != WLTP_CANCEL) {
                    if (scm_write_bytes_pid_array (address,  PidsSameExe, PidsSameExeCount,
                                                   (unsigned char*)&(bb_vari.udw), 4) != 4)
                    {
                        ThrowError (1, "Label %s cannot write to extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
                    }
                }
                break;
            case BB_QWORD:
                if (IsBin) {
                    ThrowError (1, "Value of label (%s = %s) cannot be written to a signed variable", LabelName, WertStr);
                    ret = WLTP_CANCEL;
                    break;
                }
                if ((ret = CheckMinMaxRange (in_value, in_bbtype,  bbtype, &bb_vari, LabelName, SuppressErrorFctnFlag)) != WLTP_CANCEL) {
                    if (scm_write_bytes_pid_array (address,  PidsSameExe, PidsSameExeCount,
                                                   (unsigned char*)&(bb_vari.qw), 8) != 8)
                    {
                        ThrowError (1, "Label %s cannot write to extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
                    }
                }
                break;
            case BB_UQWORD:
                if (IsBin) {
                    if (Cnt > 64) {
                        ThrowError (1, "Value of label %s = %s is to long", LabelName, WertStr);
                        ret = WLTP_CANCEL;
                        break;
                    }
                    // Read value and mask
                    if (scm_write_bytes_pid_array (address,  PidsSameExe, PidsSameExeCount,
                                                   (unsigned char*)&(in_value.uqw), 8) != 8)
                    {
                        ThrowError (1, "Label %s cannot read from extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
                        break;
                    }
                    in_value.uqw &= (uint64_t)Mask;
                    in_value.uqw |= (uint64_t)IntVal;
                    in_bbtype = BB_UQWORD;
                }
                if ((ret = CheckMinMaxRange (in_value, in_bbtype,  bbtype, &bb_vari, LabelName, SuppressErrorFctnFlag)) != WLTP_CANCEL) {
                    if (scm_write_bytes_pid_array (address,  PidsSameExe, PidsSameExeCount,
                                                   (unsigned char*)&(bb_vari.uqw), 8) != 8)
                    {
                        ThrowError (1, "Label %s cannot write to extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
                    }
                }
                break;
            case BB_FLOAT:
				if (IsBin) {
                    ThrowError (1, "Value of label (%s = %s) cannot be written to a float variable", LabelName, WertStr);
                    ret = WLTP_CANCEL;
					break;
				}
                if ((ret = CheckMinMaxRange (in_value, in_bbtype,  bbtype, &bb_vari, LabelName, SuppressErrorFctnFlag)) != WLTP_CANCEL) {
                    if (scm_write_bytes_pid_array (address, PidsSameExe, PidsSameExeCount,
                                                   (unsigned char*)&(bb_vari.f), 4) != 4)
                    {
                        ThrowError (1, "Label %s cannot write to extern Process %s",
                                                            LabelName, ProcName);
                        ret = WLTP_CANCEL;
                    }
                }
                break;
            case BB_DOUBLE:
				if (IsBin) {
                    ThrowError (1, "Value of label (%s = %s) cannot be written to a double variable", LabelName, WertStr);
                    ret = WLTP_CANCEL;
					break;
				}
                if ((ret = CheckMinMaxRange (in_value, in_bbtype,  bbtype, &bb_vari, LabelName, SuppressErrorFctnFlag)) != WLTP_CANCEL) {
                    if (scm_write_bytes_pid_array (address, PidsSameExe, PidsSameExeCount,
                                                   (unsigned char*)&(bb_vari.d), 8) != 8)
                    {
                        ThrowError (1, "Label %s cannot write to extern Process %s",
                                                              LabelName, ProcName);
                        ret = WLTP_CANCEL;
                    }
                }
                break;
            default:
                ThrowError (1, "Label %s cannot write to extern Process %s not a "
                          "base type", LabelName, ProcName);
                ret = WLTP_CANCEL;
				break;
        }
    }
    return ret;
}


void SwapBytesIfNecessary(unsigned char *MemStream,
                          int Size,
                          int LittleBigEndian)
{
    unsigned char help_swap;

    if (LittleBigEndian == MSB_FIRST_FORMAT) {
        switch (Size) {
            case 2:
                help_swap = MemStream[0];
                MemStream[0] = MemStream[1];
                MemStream[1] = help_swap;
                break;
            case 4:
                help_swap = MemStream[0];
                MemStream[0] = MemStream[3];
                MemStream[3] = help_swap;
                help_swap = MemStream[1];
                MemStream[1] = MemStream[2];
                MemStream[2] = help_swap;
                break;
            case 8:
                help_swap = MemStream[0];
                MemStream[0] = MemStream[7];
                MemStream[7] = help_swap;
                help_swap = MemStream[1];
                MemStream[1] = MemStream[6];
                MemStream[6] = help_swap;
                help_swap = MemStream[2];
                MemStream[2] = MemStream[5];
                MemStream[5] = help_swap;
                help_swap = MemStream[3];
                MemStream[3] = MemStream[4];
                MemStream[4] = help_swap;
                break;
            default:
                break;
        }
    }
}


// This function parse one line: <label> = <value>
// If it was successful it will return 1 otherwise 0
int ParseSvlLine(const char *line,              // SVL file line
             char *label,                      // Label return buffer
             size_t max_char_label,
             char *val,                         // Value return
             size_t max_char_val)
{
    const char *s, *s2;
    char *d;
    const char *ws;
    int char_count;

    s = line;
    d = label;
    while (isascii(*s) && isspace(*s)) {
        s++;
    }

    char_count = 0;
    while ((*s != 0) && (*s != '=') && !(isascii(*s) && isspace(*s))) {
        char_count++;
        if (char_count >= (int)(max_char_label-1)) {
            return FALSE;
        }
        *d++ = *s++;
    }
    if (char_count == 0) {
        return FALSE;
    }
    *d = 0;
    while (isascii(*s) && isspace(*s)) {
        s++;
    }
    if (*s != '=') {
        return FALSE;
    }
    s++;
    while (isascii(*s) && isspace(*s)) {
        s++;
    }
    if (*s == 0) {
        return FALSE;
    }

    // Value string
    s2 = s;
    ws = NULL;
    while (*s != 0) {
        if (ws == NULL) {
            if (isascii(*s) && isspace(*s)) {
                ws = s;
            }
        } else {
            if (!(isascii(*s) && isspace(*s))) {
                ws = NULL;
            }
        }
        s++;
    }
    if (ws == NULL) {
        char_count = (int)(s - s2);
    } else {
        char_count = (int)(ws - s2);
    }
    if ((char_count <= 0) || (char_count >= (int)(max_char_val - 1))) {
        return FALSE;
    }
    MEMCPY (val, s2, (size_t)char_count);
    val[char_count] = 0;

    return TRUE;
}


int WriteSVLToProcess(const char *SvlFileName,
                      const char *CurProcess)
{
    FILE *fp_svl;
    int64_t SvlFileSize;
    int Percent, Percent_old=0L;
    char line[MAX_LINE], line_save[MAX_LINE], label[MAX_LABEL],
         val[MAX_LINE];
    int IgnoreAllWrongSVLLines=FALSE,
        IgnoreAllWrongSVLLabels=FALSE;
    int progressBarID;
    int rc;
    PID pid;
    int Pid32;
    PROCESS_APPL_DATA *pappldata;
    int LineCounter = 0;
    int UniqueNumber;
    int Ret = 0;
    char *s, *d;
    int ValidCharInsideLine;
    int InsideCommentCounter = 0;
    int LastStartCommentLineNr = 0;

    UniqueNumber = AquireUniqueNumber();

    pappldata = ConnectToProcessDebugInfos (UniqueNumber,
                                            CurProcess,
                                            &Pid32,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0);
    pid = Pid32;

    if (pappldata == NULL) {
        FreeUniqueNumber (UniqueNumber);
        ThrowError(MESSAGE_STOP, "The process \"%s\" is not available !", CurProcess);
        return -1;
    }

    if ((fp_svl = open_file(SvlFileName, "rt")) == NULL) {
        RemoveConnectFromProcessDebugInfos (UniqueNumber);
        FreeUniqueNumber (UniqueNumber);
        ThrowError(MESSAGE_STOP, "Unable to open Symbol-File: \"%s\" !", SvlFileName);
        return -1;
    }

    _fseeki64(fp_svl, 0L, SEEK_END);
    SvlFileSize = ftell(fp_svl);
    progressBarID = OpenProgressBarFromOtherThread("(Load SVL)");
    _fseeki64(fp_svl, 0L, SEEK_SET);

    int PidsSameExe[16];
    int PidsSameExeCount;

    if (WaitUntilProcessIsNotActiveAndThanLockItEx (Pid32, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "write to external process memory",
                                                    PidsSameExe, &PidsSameExeCount, 16,
                                                     __FILE__, __LINE__) == 0) {
        while (fgets(line, MAX_LINE-1, fp_svl) != NULL) {  // read next line
            StringCopyMaxCharTruncate (line_save, line, sizeof(line_save));  // Make a copy for error message
            ValidCharInsideLine = 0;
            if ((LineCounter & 0xFF) == 0) {  // Only every 256 lines ftell
                int64_t Pos = _ftelli64(fp_svl);
                Percent = (long)((100.0 * (double)Pos / (double)SvlFileSize));
                if (Percent > Percent_old) {
                    Percent_old = Percent;
                    if (progressBarID >= 0) SetProgressBarFromOtherThread(progressBarID, Percent);
                }
            }
            LineCounter++;
            s = d = line;
            if (InsideCommentCounter == 0) {
                while (isascii(*s) && isspace(*s)) {
                    s++;
                }
                if ((*s == ';') ||
                    ((*s == '/') && (s[1] == '/'))) {
                    goto NEXT_LINE;    // comment line
                }
                if ((*s == '/') && (s[1] == '*')) {
                    s += 2;
                    InsideCommentCounter++;
                    LastStartCommentLineNr = LineCounter;
                } else if ((*s == '*') && (*s == '/')) {
                    if (InsideCommentCounter) {
                        s += 2;
                        InsideCommentCounter--;
                    } else {
                        ThrowError (1, "corresponding start comment \"/*\" missing for end comment \"/*\" at line %i inside SVL file \"%s\"", LineCounter, SvlFileName);
                        goto OUT_OF_WHILE_FGETS;  // while (fgets(...
                    }
                }
            }
            while (*s != 0) {
                if (InsideCommentCounter == 0) {
                    if ((*s == ';') ||
                        ((*s == '/') && (s[1] == '/'))) {
                        *d = 0;
                        break;     // Rest dof the line are comment
                    } else  if ((s[0] == '/') && (s[1] == '*')) {
                        s += 2;
                        InsideCommentCounter++;
                        LastStartCommentLineNr = LineCounter;
                    } else {
                        ValidCharInsideLine = 1;
                        *d++ = *s++;
                    }
                } else {
                    if ((s[0] == '/') && (s[1] == '*')) {
                        s += 2;
                        InsideCommentCounter++;
                        LastStartCommentLineNr = LineCounter;
                    } else if ((s[0] == '*') && (s[1] == '/')) {
                        if (InsideCommentCounter) {
                            s += 2;
                            InsideCommentCounter--;
                        } else {
                            ThrowError (1, "corresponding start comment \"/*\" missing for end comment \"/*\" at line %i inside SVL file \"%s\"", LineCounter, SvlFileName);
                            goto OUT_OF_WHILE_FGETS;  // while (fgets(...
                        }
                    } else {
                        s++;
                    }
                }
            }
            while ((d > line) && isascii(*(d-1)) && isspace(*(d-1))) {
                d--;
            }
            if (d == line) goto NEXT_LINE;  // empty line

            *d = 0;  // terminate line
            if (ValidCharInsideLine) {
                // Now have a valid SVL line
                if (!ParseSvlLine(line, label, sizeof (label), val, sizeof (val))) {
                    Ret++;
                    if (IgnoreAllWrongSVLLines != TRUE) {
                        switch (ThrowError(ERROR_OK_OKALL_CANCEL,
                                "Incorrect SVL-File \"%s\" line (%i):\"%s\" ! Continue ?",
                                SvlFileName, LineCounter, line))
                        {
                        case IDOK:
                            break;
                        case IDCANCEL:
                            close_file(fp_svl);
                            if (progressBarID >= 0) CloseProgressBarFromOtherThread(progressBarID);
                            UnLockProcess (pid);
                            RemoveConnectFromProcessDebugInfos (UniqueNumber);
                            FreeUniqueNumber (UniqueNumber);
                            return -1;
                        case IDOKALL:
                            if (!conv_error2message)
                            {
                                IgnoreAllWrongSVLLines = TRUE;
                                continue;
                            }
                        default:
                            break;
                        }
                    }
                    goto NEXT_LINE;
                }
            }
            rc = WriteLabelToProcess(label, pid, PidsSameExe, PidsSameExeCount, CurProcess, val, IgnoreAllWrongSVLLabels, pappldata);
            if (rc != WLTP_OK) {
                Ret++;
                if (IgnoreAllWrongSVLLabels == TRUE) {
                    goto NEXT_LINE;
                }
                switch (rc) {
                    case WLTP_OK:
                        break;
                    case WLTP_CANCEL:
                        close_file(fp_svl);
                        if (progressBarID >= 0) CloseProgressBarFromOtherThread(progressBarID);
                        UnLockProcess (pid);
                        RemoveConnectFromProcessDebugInfos (UniqueNumber);
                        FreeUniqueNumber (UniqueNumber);
                        return -1;
                    case WLTP_OKALL:
                        if (!conv_error2message) {
                            IgnoreAllWrongSVLLabels = TRUE;
                            goto NEXT_LINE;
                        }
                    default:
                        break;
                }
            }
NEXT_LINE:
            ;
        }
OUT_OF_WHILE_FGETS:
        if (InsideCommentCounter) {
            ThrowError (1, "missing end of comment \"*/\" started at line %i inside SVL file \"%s\"", LastStartCommentLineNr, SvlFileName);
        }
        if (progressBarID >= 0) SetProgressBarFromOtherThread(progressBarID, 100);
        if (progressBarID >= 0) CloseProgressBarFromOtherThread(progressBarID);
        close_file(fp_svl);
        UnLockProcess (pid);
    }

    RemoveConnectFromProcessDebugInfos (UniqueNumber);
    FreeUniqueNumber (UniqueNumber);

    return Ret;  // Count of the lines with error or unknown labels
}

#define UNUSED(x) (void)(x)


BOOL WriteProcessToSVLOrSAL(const char *FileName,
                       const char *CurProcess,
                       PROCESS_APPL_DATA *pappldata,
                       const char *filter,  // Label filter
                       INCLUDE_EXCLUDE_FILTER *IncExcludeFilter,
                       SECTION_ADDR_RANGES_FILTER *GlobalSectionFilter,
                       int FileType,        // Flag: SVL or SAL
                       int IncPointer)
{
    FILE *fp_svl_sal;
    long Percent, Percent_old=0L;
    int what;
    int32_t typenr;
    char *label;
    char strg[MAX_LINE];
    char buffer[4096];
    uint64_t ProcAdr, BaseAddr;
    int bbtype;
    int err;
    int flag;
    int32_t idx;
    union BB_VARI bb_vari;
    int progressBarID;
    PID pid;
    char DllName[MAX_PATH];
    int64_t SalAddressOffset;  // If SAL, than this can be an address offset
    GET_NEXT_STRUCT_ENTRY_BUFFER GetNextStructEntryBuffer;

    pid = get_pid_by_name(CurProcess);
    if (!IsExternProcessLinuxExecutable(pid) &&
        lives_process_inside_dll(CurProcess, DllName) > 0) {
        uint64_t ModuleBaseAddress;
        int Index;
        if (GetExternProcessBaseAddress(pid, &ModuleBaseAddress, DllName) != 0) { // besser waere die Basis-Adresse beim LOGIN mit zu uebertragen
            ThrowError (1, "cannot get base address of process \"%s\" useing DLL \"%s\"", CurProcess, DllName);
            return(FALSE);
        }
        Index = GetExternProcessIndexInsideExecutable (pid);
        if (Index < 0) {
            ThrowError (1, "cannot get index of process \"%s\" inside executable", CurProcess);
            return(FALSE);
        }
        SalAddressOffset = (int64_t)0x80000000 + (int64_t)(Index << 28) - (int64_t)ModuleBaseAddress;
    } else {
        SalAddressOffset = 0;
    }

    if (pappldata == NULL) {
        ThrowError(MESSAGE_STOP, "The process \"%s\" is not available !", CurProcess);
        return(0);
    }
    if ((fp_svl_sal = open_file(FileName, "wt")) == NULL) {
        ThrowError(MESSAGE_STOP, "Unable to open Symbol-File: \"%s\" !", FileName);
        return(0);
    }

    if (FileType == WPTSOS_SVL) {
        progressBarID = OpenProgressBarFromOtherThread("(Write SVL)");
    }
    else if (FileType == WPTSOS_SAL) {
        progressBarID = OpenProgressBarFromOtherThread("(Write SAL)");
    }
    else if (FileType == WPTSOS_SATVL) {
        progressBarID = OpenProgressBarFromOtherThread("(Write SATVL)");
    }
    else {
        close_file(fp_svl_sal);
        ThrowError(MESSAGE_STOP, "Wrong Filetype parameter in function \"WriteProcessToSVLOrSAL\"!");
        return(FALSE);
    }

    if (WaitUntilProcessIsNotActiveAndThanLockIt (pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "read from external process memory", __FILE__, __LINE__) == 0) {
        idx = 0L;
        while ((label = get_next_label_ex (&idx, &BaseAddr, &typenr, pappldata)) != NULL) {

            Percent = (long)(100.0 * (double)idx / (double)(get_number_of_labels(pappldata) + 1));  // +1 no div by 0
            if (Percent > Percent_old) {
                Percent_old = Percent;
                if (progressBarID >= 0) SetProgressBarFromOtherThread(progressBarID, Percent);
            }

            if (!CheckAddressRangeFilter (BaseAddr, GlobalSectionFilter)) continue;   // Valid address

            if (!get_type_by_name (&typenr, label, &what, pappldata)) {
                continue; // unknown type
            }

            if (IncExcludeFilter != NULL) {
                if (!CheckIncludeExcludeFilter_OnlyLabel (label, IncExcludeFilter)) {
                    // If label don't match the filter it will be ignored
                    continue;
                }
            }

            flag = 1;
            while ((err = GetNextStructEntry (pappldata, label, buffer, sizeof(buffer), &ProcAdr,
                    &bbtype, flag, IncPointer, &GetNextStructEntryBuffer)) != NO_MORE_ENTRYS) {
                flag = 0;
                if (err == -1) {  // It is not a base data type this will be ignored
                    continue;     // search next structure element
                }
                if (CheckAddressRangeFilter (ProcAdr, GlobalSectionFilter)) {  // Valid addresse
                    int GlobalFilterFlag, IncExcludeFilterFlag;
                    // globale label filter
                    if (filter != NULL) GlobalFilterFlag = !Compare2StringsWithWildcards (buffer, filter);
                    else GlobalFilterFlag = 1;
                    // include exclude label filter
                    if (IncExcludeFilter != NULL) IncExcludeFilterFlag = CheckIncludeExcludeFilter (buffer, IncExcludeFilter);
                    else IncExcludeFilterFlag = 1;
                    if (GlobalFilterFlag && IncExcludeFilterFlag) {
                        if (FileType == WPTSOS_SVL) {
                            if (scm_read_bytes (ProcAdr, pid,
                                                (char*)&bb_vari, bbvari_sizeof(bbtype)) == -1L) {
                                if (progressBarID >= 0) CloseProgressBarFromOtherThread(progressBarID);
                                close_file(fp_svl_sal);
                                UnLockProcess (pid);
                                ThrowError(MESSAGE_STOP, "Unable to read process %s label '%s'!", CurProcess, buffer);
                                return(FALSE);
                            }
                            bbvari_value_to_str(strg, sizeof(strg), (unsigned char*)&(bb_vari), bbtype);

                            fprintf(fp_svl_sal, "%s = %s\n", buffer, strg);
                        }
                        else if (FileType == WPTSOS_SAL) {
                            int64_t Addr64 = (int64_t)ProcAdr;
                            if ((Addr64 + SalAddressOffset) < 0) {
                                ThrowError (1, "address 0x%" PRIX64 " of label \"%s\" are smaller than the offset 0x%" PRIX64 "", Addr64, buffer, SalAddressOffset);
                                fprintf(fp_svl_sal, "error: address 0x%" PRIX64 " of label \"%s\" are smaller than the offset 0x%" PRIX64 "\n", Addr64, buffer, SalAddressOffset);
                            } else {
                                Addr64 = Addr64 + SalAddressOffset;
                                fprintf(fp_svl_sal, "%s 0:%" PRIX64 "h\n", buffer, Addr64);
                            }
                        }
                        else if (FileType == WPTSOS_SATVL) {
                            int64_t Addr64 = (int64_t)ProcAdr;
                            if ((Addr64 + SalAddressOffset) < 0) {
                                ThrowError (1, "address 0x%" PRIX64 " of label \"%s\" are smaller than the offset 0x%" PRIX64 "", Addr64, buffer, SalAddressOffset);
                                fprintf(fp_svl_sal, "error: address 0x%" PRIX64 " of label \"%s\" are smaller than the offset 0x%" PRIX64 "\n", Addr64, buffer, SalAddressOffset);
                            } else {
                                if (scm_read_bytes (ProcAdr, pid,
                                                    (char*)&bb_vari, bbvari_sizeof(bbtype)) == -1L) {
                                    if (progressBarID >= 0) CloseProgressBarFromOtherThread(progressBarID);
                                    close_file(fp_svl_sal);
                                    UnLockProcess (pid);
                                    ThrowError(MESSAGE_STOP, "Unable to read process %s label '%s'!", CurProcess, buffer);
                                    return(FALSE);
                                }
                                Addr64 = Addr64 + SalAddressOffset;
                                bbvari_value_to_str(strg, sizeof(strg), (unsigned char*)&(bb_vari), bbtype);
                                fprintf(fp_svl_sal, "%s 0:%" PRIX64 "h %s = %s\n", buffer, Addr64, GetDataTypeName(bbtype), strg);
                            }
                        } else {
                            if (progressBarID >= 0) CloseProgressBarFromOtherThread(progressBarID);
                            close_file(fp_svl_sal);
                            UnLockProcess (pid);
                            ThrowError(MESSAGE_STOP, "Wrong Filetype parameter in function \"WriteProcessToSVLOrSAL\"!");
                            return(FALSE);
                        }
                    }
                }

                if ((err < 0) || (err == BASIC_TYPE)) {
                    break;
                }
            }
        } /* end while */
        if (progressBarID >= 0) SetProgressBarFromOtherThread(progressBarID, 100);
        UnLockProcess (pid);
    }
    if (progressBarID >= 0) CloseProgressBarFromOtherThread(progressBarID);
    close_file(fp_svl_sal);
    return TRUE;
}


BOOL ScriptWriteSVLToProcess(const char *SvlFileName,
                        const char *CurProcess)
{
    return(WriteSVLToProcess(SvlFileName, CurProcess));
}


BOOL ScriptWriteProcessToSVL(
                  const char *SvlFileName,
                  const char *CurProcess,
                  const char *filter        // Label filter
                 )
{
    PROCESS_APPL_DATA *pappldata;
    SECTION_ADDR_RANGES_FILTER *GlobalSectionFilter; 
    int UniqueNumber;
    int Pid;
    BOOL Ret;

    UniqueNumber = AquireUniqueNumber();
    pappldata = ConnectToProcessDebugInfos (UniqueNumber,
                                            CurProcess,
                                            &Pid,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0);
    if (pappldata == NULL) {
        return(FALSE);
    }
    GlobalSectionFilter = BuildGlobalAddrRangeFilter (pappldata);

    Ret = WriteProcessToSVLOrSAL(SvlFileName, CurProcess, pappldata,
                                 (filter[0] == 0) ? "*" : filter,
                                 NULL, GlobalSectionFilter,
                                 WPTSOS_SVL, 0);

    DeleteAddrRangeFilter (GlobalSectionFilter);
    RemoveConnectFromProcessDebugInfos (UniqueNumber);
    FreeUniqueNumber (UniqueNumber);

    return Ret;
}


BOOL ScriptWriteProcessToSAL(
                  const char *SalFileName,
                  const char *CurProcess,
                  const char *filter,       // Label filter
                  int IncPointer
                 )
{
    PROCESS_APPL_DATA *pappldata;
    SECTION_ADDR_RANGES_FILTER *GlobalSectionFilter; 
    int UniqueNumber;
    int Pid;
    BOOL Ret;

    UniqueNumber = AquireUniqueNumber();
    pappldata = ConnectToProcessDebugInfos (UniqueNumber,
                                            CurProcess,
                                            &Pid,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0);
    if (pappldata == NULL) {
        return(FALSE);
    }
    GlobalSectionFilter = BuildGlobalAddrRangeFilter (pappldata);

    Ret = WriteProcessToSVLOrSAL(SalFileName, CurProcess, pappldata,
                                 (filter[0] == 0) ? "*" : filter,
                                 NULL, GlobalSectionFilter,
                                 WPTSOS_SAL,
                                 IncPointer);
 
    DeleteAddrRangeFilter (GlobalSectionFilter);
    RemoveConnectFromProcessDebugInfos (UniqueNumber);
    FreeUniqueNumber (UniqueNumber);

    return Ret;
}

BOOL ScriptWriteProcessToSATVL(
                  const char *SatvlFileName,
                  const char *CurProcess,
                  const char *filter        // Label filter
                 )
{
    PROCESS_APPL_DATA *pappldata;
    SECTION_ADDR_RANGES_FILTER *GlobalSectionFilter;
    int UniqueNumber;
    int Pid;
    BOOL Ret;

    UniqueNumber = AquireUniqueNumber();
    pappldata = ConnectToProcessDebugInfos (UniqueNumber,
                                            CurProcess,
                                            &Pid,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0);
    if (pappldata == NULL) {
        return(FALSE);
    }
    GlobalSectionFilter = BuildGlobalAddrRangeFilter (pappldata);

    Ret = WriteProcessToSVLOrSAL(SatvlFileName, CurProcess, pappldata,
                                 (filter[0] == 0) ? "*" : filter,
                                 NULL, GlobalSectionFilter,
                                 WPTSOS_SATVL, 0);

    DeleteAddrRangeFilter (GlobalSectionFilter);
    RemoveConnectFromProcessDebugInfos (UniqueNumber);
    FreeUniqueNumber (UniqueNumber);

    return Ret;
}
