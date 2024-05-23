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


#ifndef EXTPROCESSREFERENCES_H
#define EXTPROCESSREFERENCES_H

#include "SharedDataTypes.h"

int InitProcessRefAddrCriticalSection (void);

int CheckProcessInArray (int Pid);
int DelProcessFromArray (int Pid);
int SearchProcessInArray (int Pid);

int AddVarToProcessRefList (int Pid, const char *Variablename, uint64_t Address, int Type);
int DelVarFromProcessRefList (int Pid, const char *Variablename);

int GetVarAddrFromProcessRefList (int Pid, const char *Variablename, uint64_t *pAddr, int *pType);

int AddNewVariableToArray (int ProcessIdx, const char *Variablename, uint64_t Addr, int Type);

int rereference_all_vari_from_ini (int pid, int IgnoreBasicSettings, int StartIdx);
int look_if_referenced (int pid, char *lname, char *DisplayName, int Maxc);
int remove_referenced_vari_ini (int pid, const char *lname);
int write_referenced_vari_ini (int pid, const char *lname, const char *dname);

/* Export a reference list of an external process
   The process specific section "referenced variables <process name>"
   will be renamed into a process neutral section "referenced variables".
   In case of error the function will return a negative value otherwise 0.*/
int ExportVariableReferenceList (int pid, const char *FileName, int OutputFormat);

/* Importiert a refeence list of an external process
   The process neutral section  "referenced variables" section will be renamed
   to a process specific section "referenced variables <process name>"
   In case of error the function will return a negative value otherwise 0. */
int ImportVariableReferenceList (int pid, const char *FileName);

int AddVariableReferenceList (int pid, char *FileName);

// Compare 2 variable names independed if there are "::" or "._." inside
int CmpVariName (const char *Name1, const char *Name2);

int GetDisplayName (char *Section, int Index, const char *ReferenceName, char *ret_DispayName, int Maxc);
int GetDisplayNameByLabelName (int pid, const char *lname, char *ret_DisplayName, int Maxc);

int ReferenceOneSymbol(const char* Symbol, const char *DisplayName, const char* Process, const char *Unit, int ConversionType, const char *Conversion,
                       double Min, double Max, int Color, int Width, int Precision, int Flags);
int DereferenceOneSymbol(const char* Symbol, const char* Process, int Flags);

enum BB_DATA_TYPES GetRawValueOfOneSymbol(const char* Symbol, const char* Process, int Flags, union BB_VARI *ret_Value);
int SetRawValueOfOneSymbol(const char* Symbol, const char* Process, int Flags, enum BB_DATA_TYPES DataType, union BB_VARI Value);

#endif
