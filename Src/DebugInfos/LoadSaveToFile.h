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


#ifndef _LOADVALU_H
#define _LOADVALU_H


#define FALSE       0
#define TRUE        1

#define MAX_LINE           2048+64
#define MAX_LABEL              512
#define GP_LABEL_ADDRESS   0xB000L

#define TO_PROCESS      0
#define TO_FILE         1
#define TO_SALFILE      2
#define TO_SATVLFILE    3

#define CHECK_SIZE      0


#define LSB_FIRST_FORMAT    0
#define MSB_FIRST_FORMAT 1

/* Return values fuer WriteLabelToProcess */
#define WLTP_OK         0
#define WLTP_CANCEL     1
#define WLTP_OKALL      2

/* Parameterdefines fuer Funktion WriteProcessToSVLOrSAL */
#define WPTSOS_SVL      1
#define WPTSOS_SAL      2
#define WPTSOS_SATVL    3

/* wird gebraucht wg. Prototyp zu INCLUDE_EXCLUDE_FILTER */
#include "Wildcards.h"
#include "SectionFilter.h"

#include "GlobalDataTypes.h"


int file_exists(char *filename);

// this function will read value from a byte array based on the data type
// the return value are the number of bytes read out of the byte array or 0
// if the data type is invalid
int  bbvari_value_to_str(char *strg,        // return buffer for the value string
                        int maxc,               // size of the buffer
                        unsigned char *src,     // Byte array
                        int bbtype);              // Blackboard data type

// this function returns the size of a blackboard data type
int bbvari_sizeof(int bbtype);

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
                        PROCESS_APPL_DATA *pappldata);

void SwapBytesIfNecessary(unsigned char *MemStream,
                     int Size,
                     int LittleBigEndian);


// This function parse one line: <label> = <value>
// If it was successful it will return 1 otherwise 0
int ParseSvlLine(const char *line,              // SVL file line
                 char *label,                      // Label return buffer
                 size_t max_char_label,
                 char *val,                         // Value return
                 size_t max_char_val);


BOOL WriteProcessToSVLOrSAL(const char *FileName,
                            const char *CurProcess,
                            PROCESS_APPL_DATA *pappldata,
                            const char *filter,  // Label filter
                            INCLUDE_EXCLUDE_FILTER *IncExcludeFilter,
                            SECTION_ADDR_RANGES_FILTER *GlobalSectionFilter,
                            int FileType,        // Flag: SVL or SAL
                            int IncPointer);

int WriteSVLToProcess(const char *SvlFileName,
                      const char *CurProcess);



BOOL ScriptWriteSVLToProcess(const char *SvlFileName,
                             const char *CurProcess);


BOOL ScriptWriteProcessToSVL(const char *SvlFileName,
                             const char *CurProcess,
                             const char *filter);       // Label filter

BOOL ScriptWriteProcessToSAL(const char *SalFileName,
                             const char *CurProcess,
                             const char *filter,       // Label filter
                             int IncPointer);

BOOL ScriptWriteProcessToSATVL(const char *SatvlFileName,
                               const char *CurProcess,
                               const char *filter);        // Label filter

#endif /* #ifndef _LOADVALU_H */

