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


#ifndef DEBUGINFODB_H
#define DEBUGINFODB_H

#include "DebugInfos.h"

// flag ist eine Bitkombination 
#define DEBUG_INFOS_LOADED          0x00000001
#define DEBUG_INFOS_ASSICIATED_EXE  0x00000002

// Um Konflikte mit den Borland-Basis-Datentypen zu vermeiden werden alle VC-Typnummern um 0x1000 erhoeht
#define TYPEID_OFFSET 0x1000

int32_t DeleteDebugInfos (DEBUG_INFOS_DATA *pappldata);

int32_t get_type_by_name (int32_t *ptypenr, char *label, int32_t *what, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata);

char *get_typestr_by_typenr_internal (int32_t *typenr, int32_t *what, DEBUG_INFOS_DATA *pappldata);
char *get_typestr_by_typenr(int32_t *typenr, int32_t *what, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata);


int32_t get_what_is_typenr (int32_t *typenr,           // in out
                        int32_t *ptypenr_idx,      // in out: wenn Inhalt -1 Index ist noch unbekannt deshalb suchen
                                                // wenn != -1 Index keine Suche mehr noetig
                        DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata);


char *get_base_type (int32_t typenr);
int32_t get_base_type_size (int32_t typenr);

int32_t get_number_of_labels (DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata);
char *get_next_label (int32_t *pidx, DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata);
char *get_next_label_ex (int32_t *pidx, uint64_t *paddr, int32_t *ptypenr, PROCESS_APPL_DATA *par_pappldata);
int32_t get_next_sorted_label_index(int32_t *pidx, DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata);

int32_t get_points_to_internal (int32_t *points_to_typenr, int32_t *points_to_what,
                   int32_t parent_typenr, DEBUG_INFOS_DATA *pappldata);
int32_t get_points_to (int32_t *points_to_typenr, int32_t *points_to_what,
                   int32_t parent_typenr, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata);

int32_t get_modify_what (int32_t *modify_typenr, int32_t *modify_what,
                     int32_t parent_typenr, PROCESS_APPL_DATA *pappldata);
int32_t get_modify_what_internal (int32_t *modify_typenr, int32_t *modify_what,
                              int32_t parent_typenr, DEBUG_INFOS_DATA *pappldata);

char *get_next_struct_entry_internal (int32_t *typenr, uint32_t *address_offset, int32_t *pbclass,
                                      int32_t parent_typenr, int32_t flag, int32_t *pos, DEBUG_INFOS_DATA *pappldata);
char *get_next_struct_entry (int32_t *typenr, uint32_t *address_offset, int32_t *pbclass,
                             int32_t parent_typenr, int32_t flag, int32_t *pos, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata);

uint64_t get_label_adress (char *label, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata);

char *get_return_buffer_func_thread (void *func_addr, int32_t needed_size);  // wird auch in GetNextStructEntryEx verwendet!

char *get_struct_entry_typestr (int32_t *ptypenr, int32_t pos, int32_t *what, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata);
char *get_struct_entry_typestr_internal(int32_t *ptypenr, int32_t pos, int32_t *what, DEBUG_INFOS_DATA  *pappldata);

int32_t get_array_elem_type_and_size (int32_t *arrayelem_typenr, int32_t *arrayelem_of_what,
                                  int32_t *array_size, int32_t *array_elements,
                                  int32_t array_typenr, PROCESS_APPL_DATA *pappldata);


int32_t get_array_element_count (int32_t array_typenr, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata);

int32_t get_structure_element_count (int32_t structure_typenr, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata);

int32_t get_array_type_string_internal (int32_t typenr, int32_t elem_or_array_flag, char *ret_type_string, int32_t max_c, DEBUG_INFOS_DATA *pappldata);
int32_t get_array_type_string (int32_t typenr, int32_t elem_or_array_flag, char *ret_type_string, int32_t max_c, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata);

int32_t get_pointer_type_string_internal (int32_t typenr, int32_t points_to_or_pointer_flag, char *ret_type_string, int32_t max_c, DEBUG_INFOS_DATA *pappldata);
int32_t get_pointer_type_string (int32_t typenr, int32_t points_to_or_pointer_flag, char *ret_type_string, int32_t max_c, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata);

int32_t get_base_type_bb_type (int32_t typenr);
int32_t get_base_type_bb_type_ex (int32_t typenr, PROCESS_APPL_DATA *pappldata);
int32_t get_base_type_bb_type_ex_modifyer (int32_t typenr, PROCESS_APPL_DATA *pappldata);

// This should be called if an external process will be terminated
int32_t application_update_terminate_process (char *pname, char *DllName);

// This should be called if an external process will be started
int32_t application_update_start_process (int32_t Pid, char *ProcessName, char *DllName);

char *insert_symbol (const char *symbol, DEBUG_INFOS_DATA *pappldata);  // wird auch in dwarf.c verwendet!

int32_t insert_struct (int16_t what, int32_t typenr, const char * const struct_name,
                   int32_t fields, int32_t fieldsidx, int32_t pointsto,
                   int32_t array_elements, int32_t array_elem_typenr, int32_t array_size,
                   DEBUG_INFOS_DATA *pappldata);

int32_t insert_field (int32_t fieldnr,
                  DEBUG_INFOS_DATA *pappldata);

int32_t insert_label (char *label, int32_t typenr, uint64_t adr, DEBUG_INFOS_DATA *pappldata);
int32_t insert_label_no_address (char *label, int32_t typenr, int32_t own_typenr, DEBUG_INFOS_DATA *pappldata);
int32_t search_label_no_addr_by_typenr (int32_t typenr, char **ret_name, int32_t *ret_typenr, DEBUG_INFOS_DATA *pappldata);

int32_t insert_struct_field (int32_t fieldnr, int32_t typenr,
                         char *field_name, int32_t offset, DEBUG_INFOS_DATA *pappldata);


int32_t calc_all_array_sizes (DEBUG_INFOS_DATA *pappldata, int NoErrMsgFlag);

char *get_label_by_address (uint64_t addr, DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata);
char *get_label_by_address_ex (uint64_t addr,   // in
                               uint64_t *pbase_addr,  // out
                               int32_t *ptypenr,   // out
                               int32_t *ptypenr_idx,  // out
                               int32_t **pptypenr_idx, // out: Adresse des Index falls dieer noch -1 ist
                               DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata);

char *get_struct_elem_by_offset (int32_t parent_typenr,
                                 int32_t *ptypenr_idx,      // in out: wenn Inhalt -1 Index ist noch unbekannt deshalb suchen
                                                         // wenn != -1 Index keine Suche mehr noetig
                                 int32_t offset,   // in: gesuchter Offset
                                 int32_t *poffset, // out: gefundener Offset
                                 int32_t *pbclass,           // out: ist es eine Basis-Klasse
                                 int32_t *ptypenr, // out: Typnummer
                                 int32_t *pfield_idx,       // out: Index im Typnummer-Array oder -1 falls noch unbekannt
                                 DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata);


int32_t look_if_typenr_exist (int32_t typenr, PROCESS_APPL_DATA *pappldata);

int32_t SearchLabelByNameInternal (char *name, DEBUG_INFOS_DATA *pappldata);
int32_t SearchLabelByName (char *name, DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata);

int32_t RemoveConnectFromProcessDebugInfos (int32_t par_UniqueId);
PROCESS_APPL_DATA *ConnectToProcessDebugInfos (int32_t par_UniqueId,
                                               const char *par_ProcessName, int32_t *ret_Pid,
                                               int32_t (*par_LoadUnloadCallBackFunc)(void *par_User, int32_t par_Flags, int32_t par_Action),
                                               int32_t (*par_LoadUnloadCallBackFuncSchedThread)(int32_t par_UniqueId, void *par_User, int32_t par_Flags, int32_t par_Action),
                                               void *par_LoadUnloadCallBackFuncParUsers,
                                               int32_t par_NoErrMsgFlag);

int32_t GetConnectionToProcessDebugInfos (int32_t par_UniqueId,
                                      int32_t (**ret_LoadUnloadCallBackFunc)(void *par_User, int32_t par_Flags, int32_t par_Action),
                                      void** ret_User,
                                      int32_t par_NoErrMsgFlag);

void InitDebugInfosCriticalSections (void);
void TerminateDebugInfos (void);

int32_t GetExternProcessBaseAddressDebugInfos (DEBUG_INFOS_DATA *pappldata, DWORD *ret_BaseAddress);

void SetDebugInfosLoadedFlag (DEBUG_INFOS_DATA *pappldata, int32_t par_DebugInfoType, int32_t par_AbsoluteOrRelativeAddress);

int32_t SortDtypeList (DEBUG_INFOS_DATA *pappldata);

// nur fuer Dwarf
int32_t SetDeclarationToSpecification (int32_t typenr, DEBUG_INFOS_DATA *pappldata);

void GetInternalDebugInfoPointers (DEBUG_INFOS_ASSOCIATED_CONNECTION **ret_DebugInfosAssociatedConnections,
                                   DEBUG_INFOS_ASSOCIATED_PROCESSE **ret_DebugInfosAssociatedProcesses,
                                   DEBUG_INFOS_DATA **ret_DebugInfosDatas);


int32_t GetDbgFilenameForExecutable (char *ProcessName, char *ret_ExeFilename, int32_t par_MaxChars);
int32_t GetPdbFilenameForExecutable (char *ProcessName, char *ret_ExeFilename, int32_t par_MaxChars);


DEBUG_INFOS_ASSOCIATED_CONNECTION *ReadDebufInfosNotConnectingToProcess (const char *par_Type, const char *par_DebugInfosFileName);
void DeleteDebufInfosNotConnectingToProcess(DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata);

#endif
