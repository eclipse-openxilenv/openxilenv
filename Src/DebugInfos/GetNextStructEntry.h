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


#ifndef _GETNEXTSTRUCTENTRY_H
#define _GETNEXTSTRUCTENTRY_H

#include "DebugInfos.h"


#define MAX_STRUCT_DEPTH 64

typedef struct {
    PROCESS_APPL_DATA *pappldata;
     int typenr;
     int what;
     int ConvPointerToIntFlag;
    int32_t typenrs[MAX_STRUCT_DEPTH];
    int32_t idxs[MAX_STRUCT_DEPTH];
} GET_NEXT_STRUCT_ENTRY_BUFFER;

int GetNextStructEntry (PROCESS_APPL_DATA *pappldata, // in  ...
                        char *lname,                  // in  symbol lable name
                        char *buffer,                 // out return the expanded lable name
                        int maxc,
                        uint64_t *paddress,           // io  symbol address
                        int *pbbtype,                 // out data type
                        int flag,                     // in
                        int ConvPointerToIntFlag,     // in
                        GET_NEXT_STRUCT_ENTRY_BUFFER *internal_data); // in/out

int GetNextStructEntryEx (PROCESS_APPL_DATA *pappldata, // in  ...
                          char *lname,                  // in  symbol lable name
                          int32_t parent_typenr,
                          uint64_t parrent_address,
                          char *buffer, int maxc,       // out  return the expanded lable name
                          uint64_t *paddress,           // io  symbol address
                          int *pbbtype,                 // out data type
                          int flag);                    // in
// Return value
#define NO_MORE_ENTRYS  0
#define VALID_ENTRY     1
#define BASIC_TYPE      2
#define ERROR_IN_ENTRY -1


/* Find a lable on base of the address
   Search inside the debug information of the exrenal process */

int DbgInfoTranslatePointerToLabel (uint64_t ptr,     // in  address of the lable to search for
                                    char *label,      // out found lable
                                    int maxc,         // in  max. Lale length
                                    int *pbbtype,     // out data type
                                    int pid);         // in  process Id

#endif
