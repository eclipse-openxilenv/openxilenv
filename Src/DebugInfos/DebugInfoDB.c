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


#define USER_DEF_TYPEID_LIMIT 0x1000

#include <stdint.h>
#include <inttypes.h>
#include "Platform.h"
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <fcntl.h>

#include "Config.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Scheduler.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "StringMaxChar.h"
#include "Files.h"
#include "Wildcards.h"
#include "DebugInfos.h"
#include "IniDataBase.h"
#include "ReadExeInfos.h"
#include "RunTimeMeasurement.h"
#include "MainValues.h"
#include "DwarfReader.h"
#ifdef _WIN32
#ifdef BUILD_WITH_PDB_READER_DLL_INTERFACE
#include "CallbacksForDebugReaderDll.h"
#endif
#endif
#include "RemoteMasterScheduler.h"

#include "DebugInfoDB.h"

#define UNUSED(x) (void)(x)


#define USE_BSEARCH

static DEBUG_INFOS_DATA DebugInfosDatas[DEBUG_INFOS_MAX_DEBUG_INFOS];
static DEBUG_INFOS_DATA DebugInfosEmptyData;
static DEBUG_INFOS_ASSOCIATED_CONNECTION DebugInfosAssociatedConnections[DEBUG_INFOS_MAX_CONNECTIONS];
static DEBUG_INFOS_ASSOCIATED_PROCESSE DebugInfosAssociatedProcesses[DEBUG_INFOS_MAX_PROCESSES];
static DEBUG_INFOS_ASSOCIATED_PROCESSE DebugInfosAssociatedEmptyProcess;

void GetInternalDebugInfoPointers (DEBUG_INFOS_ASSOCIATED_CONNECTION **ret_DebugInfosAssociatedConnections,
                                   DEBUG_INFOS_ASSOCIATED_PROCESSE **ret_DebugInfosAssociatedProcesses,
                                   DEBUG_INFOS_DATA **ret_DebugInfosDatas)
{
   *ret_DebugInfosAssociatedConnections = DebugInfosAssociatedConnections;
   *ret_DebugInfosAssociatedProcesses = DebugInfosAssociatedProcesses;
   *ret_DebugInfosDatas = DebugInfosDatas;
}

static CRITICAL_SECTION DebugInfosCriticalSection;
static CRITICAL_SECTION ReturnBufferCriticalSection;

static FILE *DebugFile;

void InitDebugInfosCriticalSections (void)
{
    DebugInfosAssociatedEmptyProcess.AssociatedDebugInfos = &DebugInfosEmptyData;
    InitializeCriticalSection (&DebugInfosCriticalSection);
    InitializeCriticalSection (&ReturnBufferCriticalSection);
    //DebugFile = fopen ("DebugLoadDebugInfos.txt", "wt");
    DebugFile = NULL;
}

#define DEBUG_CRITICAL_SECTIONS

#ifdef DEBUG_CRITICAL_SECTIONS
static int32_t DebugGlobalCriticalSectionLineNr;
static DWORD DebugGlobalCriticalSectionThreadId;
static void EnterGlobalCriticalSectionDebug(CRITICAL_SECTION *cs, int32_t LineNr)
{
    EnterCriticalSection(cs);
    DebugGlobalCriticalSectionLineNr = LineNr;
    DebugGlobalCriticalSectionThreadId = GetCurrentThreadId();
}

static void EnterCriticalSectionDebug(DEBUG_INFOS_DATA *pappldata, int32_t LineNr)
{
    EnterCriticalSection(&(pappldata->CriticalSection));
    pappldata->DebugCriticalSectionLineNr = LineNr;
    pappldata->DebugCriticalSectionThreadId = GetCurrentThreadId();
}

static void LeaveGlobalCriticalSectionDebug(CRITICAL_SECTION *cs)
{
    DebugGlobalCriticalSectionLineNr = 0;
    DebugGlobalCriticalSectionThreadId = 0;
    LeaveCriticalSection(cs);
}

static void LeaveCriticalSectionDebug(DEBUG_INFOS_DATA *pappldata)
{
    pappldata->DebugCriticalSectionLineNr = 0;
    pappldata->DebugCriticalSectionThreadId = 0;
    LeaveCriticalSection(&(pappldata->CriticalSection));
}

#define ENTER_GLOBAL_CRITICAL_SECTION(cs) EnterGlobalCriticalSectionDebug((cs), __LINE__)
#define LEAVE_GLOBAL_CRITICAL_SECTION(cs) LeaveGlobalCriticalSectionDebug((cs))
#define ENTER_CRITICAL_SECTION(p) EnterCriticalSectionDebug((p), __LINE__)
#define LEAVE_CRITICAL_SECTION(p) LeaveCriticalSectionDebug((p))
#else
#define ENTER_GLOBAL_CRITICAL_SECTION(cs) EnterCriticalSection((cs))
#define LEAVE_GLOBAL_CRITICAL_SECTION(cs) LeaveCriticalSection((cs))
#define ENTER_CRITICAL_SECTION(p) EnterCriticalSection(&((p)->CriticalSection))
#define LEAVE_CRITICAL_SECTION(p) LeaveCriticalSectionDebug(&((p)->CriticalSection))
#endif


typedef int32_t (*CMP_FUNC_PTR)(const void*, const void*);

static int32_t comp_typenr_func (const DTYPE_LIST_ELEM **p1, const DTYPE_LIST_ELEM **p2)
{
   return((*p1)->typenr - (*p2)->typenr);
}

static int32_t comp_fieldnr_func (const FIELD_LIST_ELEM **p1, const FIELD_LIST_ELEM **p2)
{
   return((*p1)->fieldnr - (*p2)->fieldnr);
}


// This function will remove debug infos if there are more than 2 with no corresponding process
// dazugehoerigen Prozess sind. It will be removed always the oldest one
static void check_if_somthing_schould_be_removed (void)
{
    int32_t x;
    int32_t counter = 0;
    int32_t save_x = -1;
    uint32_t min = 0xFFFFFFFFUL;

    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) check_if_somthing_schould_be_removed()\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__);

    for (x = 0; x < DEBUG_INFOS_MAX_DEBUG_INFOS; x++) {
        if (DebugInfosDatas[x].UsedFlag && (DebugInfosDatas[x].AttachCounter == 0) && !DebugInfosDatas[x].TryToLoadFlag) {
            counter++;
            if (DebugInfosDatas[x].WhenWasTheAssociatedExeRemoved < min) {
                min = DebugInfosDatas[x].WhenWasTheAssociatedExeRemoved;
                save_x = x;
            }
        }
    }
    if (counter > 2) {
        if (save_x > -1) {
            DeleteDebugInfos (&DebugInfosDatas[save_x]);
        }
    }
}

static int32_t UnloadDebugInfos (DEBUG_INFOS_DATA *pappldata)
{
    uint32_t x;

    pappldata->TryToLoadFlag = 0;
    pappldata->LoadedFlag = 0;
    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) UnloadDebugInfos(%" PRIi64 "):  pappldata->LoadedFlag = 0\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__, pappldata - DebugInfosDatas);

    if (pappldata->symbol_buffer != NULL) {
        for (x = 0; x < pappldata->symbol_buffer_entrys; x++) {
            if (pappldata->symbol_buffer[x].symbol_name_list != NULL) {
                my_free (pappldata->symbol_buffer[x].symbol_name_list);
            }
        }
        my_free (pappldata->symbol_buffer);
    }
    pappldata->symbol_buffer = NULL;
    pappldata->symbol_buffer_entrys = 0;
    pappldata->size_symbol_buffer = 0;

    if (pappldata->label_list != NULL) my_free (pappldata->label_list);
    pappldata->label_list = NULL;
    if (pappldata->addr_list != NULL) my_free (pappldata->addr_list);
    pappldata->addr_list = NULL;
    if (pappldata->sorted_label_list != NULL) my_free (pappldata->sorted_label_list);
    pappldata->sorted_label_list = NULL;
    pappldata->label_list_entrys = 0;
    pappldata->size_label_list = 0;

    if (pappldata->label_list_no_addr != NULL) my_free (pappldata->label_list_no_addr);
    pappldata->label_list_no_addr = NULL;
    pappldata->label_list_no_addr_entrys = 0;
    pappldata->size_label_list_no_addr = 0;

    if (pappldata->dtype_list != NULL) my_free (pappldata->dtype_list);
    pappldata->dtype_list = NULL;
    pappldata->dtype_list_entrys = 0;
    pappldata->size_dtype_list = 0;

    if (pappldata->dtype_list_1k_blocks != NULL) {
        for (x = 0; x < pappldata->dtype_list_1k_blocks_entrys; x++) {
            if (pappldata->dtype_list_1k_blocks[x] != NULL) {
                my_free (pappldata->dtype_list_1k_blocks[x]);
            }
        }
        my_free (pappldata->dtype_list_1k_blocks);
    }
    pappldata->dtype_list_1k_blocks = NULL;
    pappldata->dtype_list_1k_blocks_entrys = 0;
    pappldata->size_dtype_list_1k_blocks = 0;

    // List of additionally automatically generated data types (0x7FFFFFFF downwards)
    if (pappldata->h_dtype_list != NULL) my_free (pappldata->h_dtype_list);
    pappldata->h_dtype_list = NULL;
    pappldata->h_dtype_list_entrys = 0;
    pappldata->h_size_dtype_list = 0;

    if (pappldata->h_dtype_list_1k_blocks != NULL) {
        for (x = 0; x < pappldata->h_dtype_list_1k_blocks_entrys; x++) {
            if (pappldata->h_dtype_list_1k_blocks[x] != NULL) {
                my_free (pappldata->h_dtype_list_1k_blocks[x]);
            }
        }
        my_free (pappldata->h_dtype_list_1k_blocks);
    }
    pappldata->h_dtype_list_1k_blocks = NULL;
    pappldata->h_dtype_list_1k_blocks_entrys = 0;
    pappldata->h_size_dtype_list_1k_blocks = 0;

    if (pappldata->field_list != NULL) my_free (pappldata->field_list);
    pappldata->field_list = NULL;
    pappldata->field_list_entrys = 0;
    pappldata->size_field_list = 0;

    if (pappldata->field_list_1k_blocks != NULL) {
        for (x = 0; x < pappldata->field_list_1k_blocks_entrys; x++) {
            if (pappldata->field_list_1k_blocks[x] != NULL) {
                my_free (pappldata->field_list_1k_blocks[x]);
            }
        }
        my_free (pappldata->field_list_1k_blocks);
    }
    pappldata->field_list_1k_blocks = NULL;
    pappldata->field_list_1k_blocks_entrys = 0;
    pappldata->size_field_list_1k_blocks = 0;

    if (pappldata->fieldidx_list != NULL) my_free (pappldata->fieldidx_list);
    pappldata->fieldidx_list = NULL;
    pappldata->fieldidx_list_entrys = 1;    // Do not use Element 0
    pappldata->size_fieldidx_list = 0;

    pappldata->associated_exe_file = 0;

    if (pappldata->SectionNames != NULL) {
        int32_t i;
        for (i = 0; i < pappldata->NumOfSections; i++) {
            if (pappldata->SectionNames[i] != NULL) {
                my_free (pappldata->SectionNames[i]);
            }
        }
        my_free (pappldata->SectionNames);
    }
    pappldata->SectionNames = NULL;
    if (pappldata->SectionVirtualAddress != NULL) my_free (pappldata->SectionVirtualAddress);
    pappldata->SectionVirtualAddress = NULL;
    if (pappldata->SectionVirtualSize != NULL) my_free (pappldata->SectionVirtualSize);
    pappldata->SectionVirtualSize = NULL;
    if (pappldata->SectionSizeOfRawData != NULL) my_free (pappldata->SectionSizeOfRawData);
    pappldata->SectionSizeOfRawData = NULL;
    if (pappldata->SectionPointerToRawData != NULL) my_free (pappldata->SectionPointerToRawData);
    pappldata->SectionPointerToRawData = NULL;
    pappldata->NumOfSections = 0;

    return 0;
}

int32_t DeleteDebugInfos (DEBUG_INFOS_DATA *pappldata)
{
    //ENTER_CRITICAL_SECTION (pappldata);
    UnloadDebugInfos (pappldata);
    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) DeleteDebugInfos(): Number %" PRIi64 "\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__, pappldata - DebugInfosDatas);
    pappldata->UsedFlag = 0;
    //LEAVE_CRITICAL_SECTION (pappldata);
    DeleteCriticalSection (&(pappldata->CriticalSection));

    memset (pappldata, 0, sizeof (pappldata[0]));
    return 0;
}


// This function must be called if a process will be terminaterd
int32_t application_update_terminate_process (char *pname, char *DllName)
{
    int32_t x;
    int32_t (*LoadUnloadCallBackFuncsSchedThread[DEBUG_INFOS_MAX_CONNECTIONS])(int32_t par_UniqueId, void *par_User, int32_t par_Flags, int32_t par_Action);
    void *LoadUnloadCallBackFuncParUsers[DEBUG_INFOS_MAX_CONNECTIONS];
    int32_t UniqueIds[DEBUG_INFOS_MAX_CONNECTIONS];
    int32_t Count = 0;
    int32_t LoadedFlag = 0;

    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) application_update_terminate_process(%s, %s)\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__, pname, DllName);

    ENTER_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
    for (x = 0; x < DEBUG_INFOS_MAX_PROCESSES; x++) {
        if (DebugInfosAssociatedProcesses[x].UsedFlag) {
            if (!Compare2ProcessNames (DebugInfosAssociatedProcesses[x].ProcessName, pname)) {
                int32_t y;
                for (y = 0; y < DEBUG_INFOS_MAX_CONNECTIONS; y++) {
                    if (DebugInfosAssociatedConnections[y].AssociatedProcess == &DebugInfosAssociatedProcesses[x]) {
                        if (DebugInfosAssociatedConnections[y].LoadUnloadCallBackFuncsSchedThread != NULL) {
                            // Collect calibration windows which must be informed that the process is removed
                            LoadUnloadCallBackFuncsSchedThread[Count] = DebugInfosAssociatedConnections[y].LoadUnloadCallBackFuncsSchedThread;
                            LoadUnloadCallBackFuncParUsers[Count] = DebugInfosAssociatedConnections[y].LoadUnloadCallBackFuncParUsers;
                            UniqueIds[Count] = DebugInfosAssociatedConnections[y].UsedByUniqueId;
                            Count++;
                        }
                    }
                }
                if ((DebugInfosAssociatedProcesses[x].AssociatedDebugInfos != NULL) &&
                    (DebugInfosAssociatedProcesses[x].AssociatedDebugInfos != &DebugInfosEmptyData)) {
                    DEBUG_INFOS_DATA *pappldata = DebugInfosAssociatedProcesses[x].AssociatedDebugInfos;
                    if (pappldata->AttachCounter > 0) {
                        pappldata->AttachCounter--;
                    }
                }
                DebugInfosAssociatedProcesses[x].Pid = -1;  // Process is not valid any more
                if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) application_update_terminate_process(): Prozess ist nicht mehr gueltig\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__);
                if (DebugInfosAssociatedProcesses[x].AttachCounter == 0) {
                    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) application_update_terminate_process(): Wird nicht mehr verwendet\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__);
                    DebugInfosAssociatedProcesses[x].UsedFlag = 0;
                    my_free (DebugInfosAssociatedProcesses[x].ProcessName);
                    if (DebugInfosAssociatedProcesses[x].ExeOrDllName != NULL) my_free (DebugInfosAssociatedProcesses[x].ExeOrDllName);
                    DebugInfosAssociatedProcesses[x].ProcessName = NULL;
                    DebugInfosAssociatedProcesses[x].ExeOrDllName = NULL;
                }
                break;
            }
        }
    }

    LEAVE_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
    // Call notify callbacks outside the locks
    for (x = 0; x < Count; x++) {
        if (LoadUnloadCallBackFuncsSchedThread[x] != NULL)  {
            /*int32_t Ret =*/ LoadUnloadCallBackFuncsSchedThread[x] (UniqueIds[x],
                                                                 LoadUnloadCallBackFuncParUsers[x],
                                                                 (LoadedFlag) ? DEBUG_INFOS_FLAG_LOADED : 0,
                                                                 DEBUG_INFOS_ACTION_PROCESS_TERMINATED);
        }
    }
    return 0;
}

static int32_t init_lists (DEBUG_INFOS_DATA *pappldata)
{
    uint32_t x;

    pappldata->PointerSize = 4; // only a purposal, this should be set by the debug info parser
    if (((pappldata->label_list = (LABEL_LIST_ELEM*)my_calloc (sizeof (LABEL_LIST_ELEM), 1024)) == NULL) ||
        ((pappldata->dtype_list = (DTYPE_LIST_ELEM**)my_calloc (sizeof (DTYPE_LIST_ELEM*), 1024)) == NULL) ||
        ((pappldata->h_dtype_list = (DTYPE_LIST_ELEM**)my_calloc (sizeof (DTYPE_LIST_ELEM*), 1024)) == NULL) ||
        ((pappldata->field_list = (FIELD_LIST_ELEM**)my_calloc (sizeof (FIELD_LIST_ELEM*), 1024)) == NULL) ||
        ((pappldata->dtype_list_1k_blocks = (DTYPE_LIST_ELEM**)my_calloc (sizeof (DTYPE_LIST_ELEM*), 1024)) == NULL) ||
        ((pappldata->h_dtype_list_1k_blocks = (DTYPE_LIST_ELEM**)my_calloc (sizeof (DTYPE_LIST_ELEM*), 1024)) == NULL) ||
        ((pappldata->field_list_1k_blocks = (FIELD_LIST_ELEM**)my_calloc (sizeof (FIELD_LIST_ELEM*), 1024)) == NULL) ||
        ((pappldata->fieldidx_list = (FIELDIDX_LIST_ELEM*)my_calloc (sizeof (FIELDIDX_LIST_ELEM), 1024)) == NULL) ||
        ((pappldata->addr_list = (ADDR_LIST_ELEM*)my_calloc (sizeof (ADDR_LIST_ELEM), 1024)) == NULL) ||
        ((pappldata->sorted_label_list = (uint32_t*)my_calloc (sizeof (uint32_t), 1024)) == NULL) ||
        ((pappldata->symbol_buffer = (struct SYMBOL_BUFFER*)my_calloc (sizeof (struct SYMBOL_BUFFER), 64)) == NULL)) {
        return -1;
    } else {
        pappldata->size_symbol_buffer = 64;
        // Alloc 1M byte symbol buffer
        if ((pappldata->symbol_buffer[0].symbol_name_list = (char*)my_calloc (sizeof (char), 1024*1024)) == NULL) {
            return -1;
        }
        pappldata->symbol_buffer[0].size_symbol_name_list = 1024 * 1024;
        pappldata->symbol_buffer[0].symbol_name_list_entrys = 0;
        pappldata->symbol_buffer_entrys = 1;
        pappldata->symbol_buffer_active_entry = 0;

        pappldata->label_list_entrys = 0;
        pappldata->size_label_list = 1024;

        pappldata->dtype_list_entrys = 0;
        pappldata->size_dtype_list = 1024;
        pappldata->size_dtype_list_1k_blocks = 1024;
        // Pre reserve 1024 elements
        if ((pappldata->dtype_list_1k_blocks[0] = (DTYPE_LIST_ELEM*)my_calloc (sizeof (DTYPE_LIST_ELEM), 1024)) == NULL) {
            return -1;
        }
        pappldata->dtype_list_1k_blocks_entrys = 1;
        pappldata->dtype_list_1k_blocks_active_entry = 0;

        for (x = 0; x < 1024; x++) {
            pappldata->dtype_list[pappldata->size_dtype_list - 1024 + x] = &(pappldata->dtype_list_1k_blocks[pappldata->dtype_list_1k_blocks_active_entry][x]);
        }

        // List of additional automatic generated data type ids (0x7FFFFFFF downwards to 0x40000000)
        pappldata->h_dtype_list_entrys = 0;
        pappldata->h_size_dtype_list = 1024;
        pappldata->h_size_dtype_list_1k_blocks = 1024;
        // Pre reserve 1024 elements
        if ((pappldata->h_dtype_list_1k_blocks[0] = (DTYPE_LIST_ELEM*)my_calloc (sizeof (DTYPE_LIST_ELEM), 1024)) == NULL) {
            return -1;
        }
        pappldata->h_dtype_list_1k_blocks_entrys = 1;
        pappldata->h_dtype_list_1k_blocks_active_entry = 0;

        for (x = 0; x < 1024; x++) {
            pappldata->h_dtype_list[pappldata->h_size_dtype_list - 1024 + x] = &(pappldata->h_dtype_list_1k_blocks[pappldata->h_dtype_list_1k_blocks_active_entry][x]);
        }

        pappldata->unique_fieldnr_generator = 0x10000000;   // This will be start with 0x10000000 and will be incremented
        pappldata->field_list_entrys = 0;
        pappldata->size_field_list = 1024;
        pappldata->size_field_list_1k_blocks = 1024;

        // Pre reserve 1024 elements
        if ((pappldata->field_list_1k_blocks[0] = (FIELD_LIST_ELEM*)my_calloc (sizeof (FIELD_LIST_ELEM), 1024)) == NULL) {
            return -1;
        }
        pappldata->field_list_1k_blocks_entrys = 1;
        pappldata->field_list_1k_blocks_active_entry = 0;

        for (x = 0; x < 1024; x++) {
            pappldata->field_list[pappldata->size_field_list - 1024 + x] = &(pappldata->field_list_1k_blocks[pappldata->field_list_1k_blocks_active_entry][x]);
        }


        pappldata->fieldidx_list_entrys = 1;  // Do not use Element 0
        pappldata->size_fieldidx_list = 1024;
    }
    return 0;
}


static int32_t ReadRenamingListFromIni(DEBUG_INFOS_DATA *pappldata);

static int32_t LoadDebugInfos (DEBUG_INFOS_DATA *par_DebugInfos, char *par_ExecutableFileName, int32_t par_NoErrMsgFlag, int IsRealtimeProcess, int MachineType)
{
    int32_t Linker;
    int32_t PdbFileExists;
    int32_t DbgFileExists;
    unsigned int LinkerVersion;
    char PdbFilename[MAX_PATH];
    char DbgFilename[MAX_PATH];

    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) LoadDebugInfos(%" PRIi64 ", %s)\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__,
                                    par_DebugInfos - DebugInfosDatas, par_ExecutableFileName);
    if (par_DebugInfos->LoadedFlag) {
        ThrowError (1, "Debug infos already loaded");
        return 0;  // They are already loaded
    }
    StringCopyMaxCharTruncate (par_DebugInfos->ExecutableFileName, par_ExecutableFileName, sizeof(par_DebugInfos->ExecutableFileName));
    if (IsRealtimeProcess) {
        StringCopyMaxCharTruncate(par_DebugInfos->ShortExecutableFileName, par_DebugInfos->ExecutableFileName, sizeof(par_DebugInfos->ShortExecutableFileName));
    } else {
        GetProsessShortExeFilename (par_DebugInfos->ExecutableFileName, par_DebugInfos->ShortExecutableFileName, sizeof (par_DebugInfos->ShortExecutableFileName));
        if (GetExeFileTimeAndChecksum (par_DebugInfos->ExecutableFileName,
                                       &(par_DebugInfos->LoadedFileTime),   // Age of the loaded PDB file match the age of the EXE file
                                       &(par_DebugInfos->Checksum),         // Checksumme of the EXE file Remark: this must beactivated by linker option
                                       &Linker,
                                       IsRealtimeProcess,
                                       MachineType)) {
            if (!par_NoErrMsgFlag) ThrowError (1, "cannot open executable \"%s\"", par_DebugInfos->ExecutableFileName);
            DeleteDebugInfos (par_DebugInfos);
            return -1;
        }
    }
    ReadRenamingListFromIni(par_DebugInfos);

    GetPdbFilenameForExecutable (par_DebugInfos->ExecutableFileName, PdbFilename, sizeof (PdbFilename));
    GetDbgFilenameForExecutable (par_DebugInfos->ExecutableFileName, DbgFilename, sizeof (DbgFilename));

    PdbFileExists = _access (PdbFilename, 4) == 0;  // Read permission?
    DbgFileExists = _access (DbgFilename, 4) == 0;  // Read permission?

    par_DebugInfos->DebugInfoFileName[0] = 0;
    switch ((PdbFileExists << 1) | DbgFileExists) {
    case 0:  // No PDB and no DBG file than the debug infos should contain inside the EXE
    case 1:  // There are a DBG but no PDB file
        StringCopyMaxCharTruncate(par_DebugInfos->DebugInfoFileName, DbgFilename, sizeof(par_DebugInfos->DebugInfoFileName));
        if (ReadExeInfos (par_DebugInfos->ExecutableFileName, &(par_DebugInfos->ImageBaseAddr), &LinkerVersion,
                          &(par_DebugInfos->NumOfSections),
                          &(par_DebugInfos->SectionNames),
                          &(par_DebugInfos->SectionVirtualAddress),
                          &(par_DebugInfos->SectionVirtualSize),
                          &(par_DebugInfos->SectionSizeOfRawData),
                          &(par_DebugInfos->SectionPointerToRawData),
                          // to check the signature of debug infos
                          &(par_DebugInfos->SignatureType),
                          &(par_DebugInfos->Signature),
                          &(par_DebugInfos->SignatureGuid),
                          &(par_DebugInfos->Age),

                          &(par_DebugInfos->dynamic_base_address), IsRealtimeProcess, MachineType)) {
            DeleteDebugInfos (par_DebugInfos);
            return -1;
        }
        init_lists (par_DebugInfos);
        if (parse_dwarf_from_exe_file (par_DebugInfos->ExecutableFileName, par_DebugInfos, IsRealtimeProcess, MachineType)) {
            DeleteDebugInfos (par_DebugInfos);
            return -1;
        }
        calc_all_array_sizes (par_DebugInfos, 0);
        break;
    case 2:  // There exist a PDB but no DBG file
#if defined(_WIN32)
//        && !defined(__GNUC__)
        StringCopyMaxCharTruncate(par_DebugInfos->DebugInfoFileName, PdbFilename, sizeof(par_DebugInfos->DebugInfoFileName));
        if (ReadExeInfos (par_DebugInfos->ExecutableFileName, &(par_DebugInfos->ImageBaseAddr), &LinkerVersion,
                          &(par_DebugInfos->NumOfSections),
                          &(par_DebugInfos->SectionNames),
                          &(par_DebugInfos->SectionVirtualAddress),
                          &(par_DebugInfos->SectionVirtualSize),
                          &(par_DebugInfos->SectionSizeOfRawData),
                          &(par_DebugInfos->SectionPointerToRawData),
                          // to check the signature of debug infos
                          &(par_DebugInfos->SignatureType),
                          &(par_DebugInfos->Signature),
                          &(par_DebugInfos->SignatureGuid),
                          &(par_DebugInfos->Age),

                          &(par_DebugInfos->dynamic_base_address), IsRealtimeProcess, MachineType)) {
            DeleteDebugInfos (par_DebugInfos);
            return -1;
        }
        init_lists (par_DebugInfos);
#ifdef BUILD_WITH_PDB_READER_DLL_INTERFACE
        if (ParsePdbFile (PdbFilename, par_DebugInfos,
                          par_DebugInfos->dynamic_base_address, LinkerVersion)) {
            DeleteDebugInfos (par_DebugInfos);
            return -1;
        }
#else
        ThrowError(1, "Visual PDB file not supported");
        //if (parse_vc_pdb_file (PdbFilename, par_DebugInfos, par_DebugInfos->ImageBaseAddr, LinkerVersion)) {
        //    DeleteDebugInfos (par_DebugInfos);
        //    return -1;
        //}
#endif
#else
        ThrowError(1, "microsoft PDB files are not supported");
        return -1;
#endif
        break;
    default:
    case 3:
        if (!par_NoErrMsgFlag) ThrowError (1, "there are both type of debug info file (*.pdb/*.dbg) for executable \"%s\"", par_DebugInfos->ExecutableFileName);
        DeleteDebugInfos (par_DebugInfos);
        return -1;
    }
    return 0;
}

// This function should only called inside DebugInfosCriticalSection
static void reload_debug_infos(int32_t x, char *LongExeFileName, int IsRealtimeProcess, int MachineType)
{
    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) reload_debug_infos(%i, %s)\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__, x, LongExeFileName);
    UnloadDebugInfos (&(DebugInfosDatas[x]));
    DebugInfosDatas[x].TryToLoadFlag = 1;
    LEAVE_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
    ENTER_CRITICAL_SECTION (&(DebugInfosDatas[x]));
    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) reload_debug_infos(): LoadDebugInfos\n", (uint32_t)GetCurrentThreadId(),  __FILE__, __LINE__);
    if (LoadDebugInfos (&DebugInfosDatas[x], LongExeFileName, 0, IsRealtimeProcess, MachineType) == 0) {
        DebugInfosDatas[x].LoadedFlag = 1;
    } else {
        DebugInfosDatas[x].LoadedFlag = 0;
    }
    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) reload_debug_infos(): DebugInfosDatas[%i].LoadedFlag = %i\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__, x, DebugInfosDatas[x].LoadedFlag);
    DebugInfosDatas[x].TryToLoadFlag = 0;
    LEAVE_CRITICAL_SECTION (&(DebugInfosDatas[x]));
    ENTER_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
}

static DEBUG_INFOS_DATA *check_and_load_debug_infos (char *LongExecutableFile, int32_t *ret_LoadFlag, int IsRealtimeProcess, int MachineType)  // can be EXE or DLL file name
{
    int32_t x;
    char ShortExeFileName[MAX_PATH];

    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) check_and_load_debug_infos(%s)\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__, LongExecutableFile);
    if (ret_LoadFlag != NULL) *ret_LoadFlag = 0;
    if (IsRealtimeProcess) {
        StringCopyMaxCharTruncate(ShortExeFileName, LongExecutableFile, sizeof(ShortExeFileName));
    } else {
        if (GetProsessShortExeFilename (LongExecutableFile, ShortExeFileName, sizeof (ShortExeFileName))) return NULL;
    }
    for (x = 0; x < DEBUG_INFOS_MAX_DEBUG_INFOS; x++) {
        if (DebugInfosDatas[x].UsedFlag) {
            if (!_stricmp(ShortExeFileName, DebugInfosDatas[x].ShortExecutableFileName)) {   // TODO: check also same path
                // Check if complete loaded
                int TryToLoad;
                ENTER_CRITICAL_SECTION (&(DebugInfosDatas[x]));
                TryToLoad = DebugInfosDatas[x].TryToLoadFlag;
                LEAVE_CRITICAL_SECTION (&(DebugInfosDatas[x]));

                if (TryToLoad) {
                    ThrowError (1, "Connect to a debug info that will be just load");
                }

                // Check if Executable has changed
                uint64_t LoadedFileTime;
                DWORD Checksum;
                int32_t Linker;
                if (GetExeFileTimeAndChecksum (LongExecutableFile,
                                               &LoadedFileTime,   // Age of the EXE associated to the loaded PDB
                                               &Checksum,         // Checksumme of the EXE-Datei (Remark: must be activated linker optionen)
                                               &Linker,
                                               IsRealtimeProcess,
                                               MachineType)) {
                   reload_debug_infos (x, LongExecutableFile, IsRealtimeProcess, MachineType);
                   if (ret_LoadFlag != NULL) *ret_LoadFlag = 1;
                } else {
                    if ((DebugInfosDatas[x].LoadedFileTime != LoadedFileTime)  ||
                        (DebugInfosDatas[x].Checksum != Checksum)) {
                        reload_debug_infos (x, LongExecutableFile, IsRealtimeProcess, MachineType);
                        if (ret_LoadFlag != NULL) *ret_LoadFlag = 1;
                    }
                }
                // Debug-Infos are up-to-date
                if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) check_and_load_debug_infos():  Debug-Infos sind noch aktuell\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__);
                return &DebugInfosDatas[x];
            }
        }
    }
    // The Debug-Infos are not loaded
    for (x = 0; x < DEBUG_INFOS_MAX_DEBUG_INFOS; x++) {
        if ((!DebugInfosDatas[x].UsedFlag) && (!DebugInfosDatas[x].TryToLoadFlag)) {
            DebugInfosDatas[x].UsedFlag = 1;
            InitializeCriticalSection(&DebugInfosDatas[x].CriticalSection);
            reload_debug_infos (x, LongExecutableFile, IsRealtimeProcess, MachineType);
            if (ret_LoadFlag != NULL) *ret_LoadFlag = 1;
            return &DebugInfosDatas[x];
        }
    }
    return NULL;
}

// This function should be called if an external process is started
int32_t application_update_start_process (int32_t Pid, char *ProcessName, char *DllName)
{
    int32_t x;
    int32_t (*LoadUnloadCallBackFuncsSchedThread[DEBUG_INFOS_MAX_CONNECTIONS])(int32_t par_UniqueId, void *par_User, int32_t par_Flags, int32_t par_Action);
    void *LoadUnloadCallBackFuncParUsers[DEBUG_INFOS_MAX_CONNECTIONS];
    int32_t UniqueIds[DEBUG_INFOS_MAX_CONNECTIONS];
    int32_t Count = 0;
    int32_t LoadedFlag = 0;

    char ShortProcessName[MAX_PATH];
    char ExecutableName[MAX_PATH];
    int32_t LoadFlag;
    int32_t Found = 0;
    int32_t IsRealtimeProcess;
    uint64_t BaseAddress64;
    int32_t MachineType;

    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) application_update_start_process(%i, %s, %s)\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__, Pid, ProcessName, DllName);
    if (s_main_ini_val.ConnectToRemoteMaster && (rm_IsRealtimeProcess(ProcessName))) {
        IsRealtimeProcess = 1;
        StringCopyMaxCharTruncate(ShortProcessName, ProcessName, sizeof(ShortProcessName));
        StringCopyMaxCharTruncate(ExecutableName, GetRemoteMasterExecutable(), sizeof(ExecutableName));
        MachineType = 3; // -> 64Bit Linux;

    } else {
        IsRealtimeProcess = 0;
        if (GetProcessNameWithoutPath (Pid, ShortProcessName)) return -1;
        if ((DllName == NULL) || (strlen(DllName) == 0)) {
            if (GetProcessExecutableName (Pid, ExecutableName)) return -1;
        } else {
            memset (ExecutableName, 0, sizeof (ExecutableName));
            strncpy(ExecutableName, DllName, sizeof (ExecutableName) - 1);
        }
        MachineType = GetExternProcessMachineType(Pid);
    }
    ENTER_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);

    if (GetExternProcessBaseAddress (Pid, &BaseAddress64, ExecutableName)) {
        BaseAddress64 = 0;
    }

    // Lock if there exist old debug infos witcch can be removed
    check_if_somthing_schould_be_removed ();

    for (x = 0; x < DEBUG_INFOS_MAX_PROCESSES; x++) {
        if (DebugInfosAssociatedProcesses[x].UsedFlag) {
            if (!Compare2ProcessNames (DebugInfosAssociatedProcesses[x].ProcessName, ShortProcessName)) {
                if (DebugInfosAssociatedProcesses[x].Pid > 0) {
                    ThrowError (1, "Process \"%s\" already in there %s (%i)", ShortProcessName, __FILE__, __LINE__);
                    return -1;
                } else {
                    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) application_update_start_process() Prozess wurde schon von einer Connection angelegt obwohl er noch nicht lief\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__);
                    // Process was added befor by a connection although the process is not running
                    DebugInfosAssociatedProcesses[x].Pid = Pid;
                    DebugInfosAssociatedProcesses[x].IsRealtimeProcess = IsRealtimeProcess;
                    DebugInfosAssociatedProcesses[x].MachineType = MachineType;
                    if (DebugInfosAssociatedProcesses[x].ExeOrDllName != NULL) {
                        my_free (DebugInfosAssociatedProcesses[x].ExeOrDllName);
                    }
                    DebugInfosAssociatedProcesses[x].ExeOrDllName = StringMalloc (ExecutableName);
                    DebugInfosAssociatedProcesses[x].BaseAddress = BaseAddress64;
                    DebugInfosAssociatedProcesses[x].AssociatedDebugInfos = check_and_load_debug_infos (ExecutableName, &LoadFlag, IsRealtimeProcess, MachineType);
                    if (DebugInfosAssociatedProcesses[x].AssociatedDebugInfos != NULL) {
                        DebugInfosAssociatedProcesses[x].AssociatedDebugInfos->AttachCounter++;
                    }
                    {
                        int32_t i;
                        for (i = 0; i < DEBUG_INFOS_MAX_CONNECTIONS; i++) {
                            if (DebugInfosAssociatedConnections[i].UsedFlag) {
                                if (DebugInfosAssociatedConnections[i].AssociatedProcess == &DebugInfosAssociatedProcesses[x]) {
                                    // Collect calibration windows which must be informed that the process is removed
                                    if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) application_update_start_process() Applikations-Fenster die benachrichtigt werden sollen dass ihr Prozess gestartet wurde sammeln\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__);
                                    LoadUnloadCallBackFuncsSchedThread[Count] = DebugInfosAssociatedConnections[i].LoadUnloadCallBackFuncsSchedThread;
                                    LoadUnloadCallBackFuncParUsers[Count] = DebugInfosAssociatedConnections[i].LoadUnloadCallBackFuncParUsers;
                                    UniqueIds[Count] = DebugInfosAssociatedConnections[i].UsedByUniqueId;
                                    Count++;
                                }
                            }
                        }
                    }
                }
                Found = 1;
                break;
            }
        }
    }
    // If a new process
    if (!Found) {
        // build a new one
        if (DebugFile != NULL) fprintf (DebugFile, "%X: %s (%i) application_update_start_process()  neuer Prozess anlegen\n", (uint32_t)GetCurrentThreadId(), __FILE__, __LINE__);
        for (x = 0; x < DEBUG_INFOS_MAX_PROCESSES; x++) {
            if (!DebugInfosAssociatedProcesses[x].UsedFlag) {
                memset(&(DebugInfosAssociatedProcesses[x]), 0, sizeof (DebugInfosAssociatedProcesses[x]));
                DebugInfosAssociatedProcesses[x].UsedFlag = 1;
                DebugInfosAssociatedProcesses[x].Pid = Pid;
                DebugInfosAssociatedProcesses[x].IsRealtimeProcess = IsRealtimeProcess;
                DebugInfosAssociatedProcesses[x].MachineType = MachineType;
                DebugInfosAssociatedProcesses[x].ProcessName = StringMalloc(ProcessName);
                DebugInfosAssociatedProcesses[x].ExeOrDllName = StringMalloc (ExecutableName);
                DebugInfosAssociatedProcesses[x].BaseAddress = BaseAddress64;
                DebugInfosAssociatedProcesses[x].AssociatedDebugInfos = NULL; // Do not load debug infos because there are no connection exist for it
                DebugInfosAssociatedProcesses[x].AttachCounter = 0;
                Found = 1;
                break;
            }
        }
    }
    LEAVE_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
    if (!Found) {
        ThrowError (1, "To many processes max. %i", DEBUG_INFOS_MAX_PROCESSES);
    }
    // Notify-Callbacks outside of locks
    for (x = 0; x < Count; x++) {
        /*int32_t Ret =*/ LoadUnloadCallBackFuncsSchedThread[x] (UniqueIds[x],
                                                         LoadUnloadCallBackFuncParUsers[x],
                                                         (LoadedFlag) ? DEBUG_INFOS_FLAG_LOADED : 0,
                                                         DEBUG_INFOS_ACTION_PROCESS_STARTED);
    }
    return 0;
}


#define DBG_FILE_NAME  1
#define PDB_FILE_NAME  2
static int32_t GetPdbDbgFilenameForExecutable (char *ExecutableName, char *ret_Filename, int32_t par_MaxChars, int32_t par_FileType)
{
    char *s = ExecutableName;
    char *p = NULL;

    while (*s != 0) {
        if (*s == '.') {
            p = s;
        }
        s++;
    }
    if (p != NULL) {  // Last '.' inside Process name
        if (((p[1] == 'E') &&
             (p[2] == 'X') &&
             (p[3] == 'E')) ||  // followed by "EXE"
            ((p[1] == 'D') &&
             (p[2] == 'L') &&
             (p[3] == 'L'))) {  // or by "DLL"
            int32_t Len = (int32_t)(p - ExecutableName);
            if ((Len + 5) >= par_MaxChars) {
                return -1;   // Error name too long
            }
            MEMCPY (ret_Filename, ExecutableName, (size_t)Len);
            switch (par_FileType) {
            case DBG_FILE_NAME:
                StringCopyMaxCharTruncate (ret_Filename + Len, ".DBG", 5);
                return 0;
            case PDB_FILE_NAME:
                StringCopyMaxCharTruncate (ret_Filename + Len, ".PDB", 5);
                return 0;
            default:
                return -3;   // Unknown file type
            }
        }
    }
    return -2; // I is no external process
}


int32_t GetDbgFilenameForExecutable (char *ProcessName, char *ret_ExeFilename, int32_t par_MaxChars)
{
    return GetPdbDbgFilenameForExecutable (ProcessName, ret_ExeFilename, par_MaxChars, DBG_FILE_NAME);
}

int32_t GetPdbFilenameForExecutable (char *ProcessName, char *ret_ExeFilename, int32_t par_MaxChars)
{
    return GetPdbDbgFilenameForExecutable (ProcessName, ret_ExeFilename, par_MaxChars, PDB_FILE_NAME);
}


char *get_base_type (int32_t typenr)
{
    switch (typenr) {
    case 1: return "int8";
    case 2: return "uint8";
    case 3: return "int16";
    case 4: return "uint16";
    case 5: return "int32";
    case 6: return "uint32";
    case 7: return "int64";
    case 8: return "uint64";
    case 9: return "real32";
    case 10: return "real64";
    case 11: return "rint32";
    case 12: return "ruint32";
    case 33: return "void";
    case 101: return "*int8";
    case 102: return "*uint8";
    case 103: return "*int16";
    case 104: return "*uint16";
    case 105: return "*int32";
    case 106: return "*uint32";
    case 107: return "*int64";
    case 108: return "*uint64";
    case 109: return "*real32";
    case 110: return "*real64";
    case 111: return "*rint32";
    case 112: return "*ruint32";
    case 133: return "*void";
    default: return NULL;
    }
}

int32_t get_base_type_size (int32_t typenr)
{
    switch (typenr) {
    case 1: return 1;
    case 2: return 1;
    case 3: return 2;
    case 4: return 2;
    case 5: return 4;
    case 6: return 4;
    case 7: return 8;
    case 8: return 8;
    case 9: return 4;
    case 10: return 8;
    case 11: return 4;
    case 12: return 4;
    case 33: return 0;
    case 101: return 4;
    case 102: return 4;
    case 103: return 4;
    case 104: return 4;
    case 105: return 4;
    case 106: return 4;
    case 107: return 4;
    case 108: return 4;
    case 109: return 4;
    case 110: return 4;
    case 111: return 4;
    case 112: return 4;
    case 133: return 4;
    case (TYPEID_OFFSET - 2): return 8;  // 64Bit int
    case (TYPEID_OFFSET - 3): return 8;  // 64Bit UInt
    default: return 0;
    }
}

int32_t get_base_type_bb_type (int32_t typenr)
{
    switch (typenr) {
    case 1: return BB_BYTE;
    case 2: return BB_UBYTE;
    case 3: return BB_WORD;
    case 4: return BB_UWORD;
    case 5:
    case 11: return BB_DWORD;
    case 6:
    case 12: return BB_UDWORD;
    case 7: return BB_QWORD;
    case 8: return BB_UQWORD;
    case 9: return BB_FLOAT;
    case 10: return BB_DOUBLE;
    default: return -1;
    }
}

int32_t get_base_type_bb_type_ex_modifyer (int32_t typenr, PROCESS_APPL_DATA *pappldata)
{
    int32_t Ret;

    Ret = get_base_type_bb_type (typenr);
    if (Ret == -1) {
        int32_t modifed_what;
        int32_t modifyed_typenr;
        if (get_modify_what (&modifyed_typenr, &modifed_what, typenr, pappldata)) {
            Ret = get_base_type_bb_type (modifyed_typenr);
        }
    }
    return Ret;
}

int32_t get_base_type_bb_type_ex (int32_t typenr, PROCESS_APPL_DATA *pappldata)
{
    return get_base_type_bb_type_ex_modifyer (typenr, pappldata);
}


char *insert_symbol (const char *symbol, DEBUG_INFOS_DATA *pappldata)
{
    uint32_t sym_size = (uint32_t)strlen (symbol) + 1;
    int32_t i;
    const char *src;
    char *dest;

    // alloc one more symbol buffer
    if ((pappldata->symbol_buffer[pappldata->symbol_buffer_active_entry].symbol_name_list_entrys + sym_size) >= pappldata->symbol_buffer[pappldata->symbol_buffer_active_entry].size_symbol_name_list) {
        if ((pappldata->symbol_buffer_entrys + 1) >= pappldata->size_symbol_buffer) {
            pappldata->size_symbol_buffer += 64;
            if ((pappldata->symbol_buffer =(struct SYMBOL_BUFFER*)my_realloc (pappldata->symbol_buffer, sizeof (struct SYMBOL_BUFFER) * pappldata->size_symbol_buffer)) == NULL) {
                ThrowError (1, "out of memory, cannot load debug infos for process \"%s\" (%s %li)", pappldata->ExecutableFileName, __FILE__, (int32_t)__LINE__);
                return NULL;
            }
        }
        pappldata->symbol_buffer_entrys++;
        pappldata->symbol_buffer_active_entry++; // This must be one less to symbol_buffer_entrys
        if ((pappldata->symbol_buffer[pappldata->symbol_buffer_active_entry].symbol_name_list =(char*)my_malloc (sizeof (char) * 1024 * 1024)) == NULL) {
            ThrowError (1, "out of memory, cannot load debug infos for executable \"%s\" (%s %li)", pappldata->ExecutableFileName, __FILE__, (int32_t)__LINE__);
            return NULL;
        }
        pappldata->symbol_buffer[pappldata->symbol_buffer_active_entry].size_symbol_name_list = 1024 * 1024;
        pappldata->symbol_buffer[pappldata->symbol_buffer_active_entry].symbol_name_list_entrys = 0;
    }
    src = symbol;
    dest = &(pappldata->symbol_buffer[pappldata->symbol_buffer_active_entry].symbol_name_list[pappldata->symbol_buffer[pappldata->symbol_buffer_active_entry].symbol_name_list_entrys]);
    for (i = 0; i < (BBVARI_NAME_SIZE-1); i++) {
        if (*src == '\0') break;
        else if (*src == '<') *dest++ = '{';
        else if (*src == '>') *dest++ = '}';
        else if (*src == ',') *dest++ = '@';
        else if (*src != ' ') *dest++ = *src;
        src++;
    }
    *dest = 0;
    pappldata->symbol_buffer[pappldata->symbol_buffer_active_entry].symbol_name_list_entrys += sym_size;

    return (&(pappldata->symbol_buffer[pappldata->symbol_buffer_active_entry].symbol_name_list[pappldata->symbol_buffer[pappldata->symbol_buffer_active_entry].symbol_name_list_entrys - sym_size]));
}


static int32_t get_pos_in_address_list_by_address (uint64_t addr, DEBUG_INFOS_DATA *pappldata)
{
     int32_t pos;
     int32_t dx, count = 0;

     if (pappldata->label_list_entrys == 0) return -1;
     pos = pappldata->label_list_entrys / 2;
     dx = pos / 2;
     if (!dx) dx = 1;
     while (1) {
         if (pappldata->addr_list[pos].adr < addr) {
             if (pos == (int32_t)(pappldata->label_list_entrys-1)) {
                 // Match or doesn't exist
                 return (int32_t)pos;
             } else if (pappldata->addr_list[pos+1].adr > addr) {
                 // Match
                 return (int32_t)pos;
             } else {
                 // Upwards
                 pos += dx;
                 if (pos > (int32_t)(pappldata->label_list_entrys-1)) pos = (int32_t)pappldata->label_list_entrys-1;
             }
         } else if (pappldata->addr_list[pos].adr > addr) {
             if (pos == 0) {
                 // Addresse gibt es nicht
                 return -1;
             } else if (pappldata->addr_list[pos-1].adr < addr) {
                 // Match
                 return (int32_t)(pos-1);
             } else {
                 // Downwards
                 pos -= dx;
                 if (pos < 0) pos = 0;
             }
         } else {
             // exact match Treffer (first element inside structure or single lable)
            return (int32_t)pos;
         }
         if (dx > 1) {
             dx = dx / 2;
         } else {
             if (count++ > 100) {
                 ThrowError (1, "Internel error %s [%i], addr = 0x%X", __FILE__, __LINE__, addr);
                 break;
             }
         }
     }
     return -1;
}


static int32_t own_strcmp (char *s1, char *s2)
{
    int32_t ret;
    if ((s1 == NULL) || (s2 == NULL)) return 0;
    ret = _stricmp(s2, s1);
    if (ret == 0) ret = strcmp(s2, s1);
    return ret;
}

static int32_t get_pos_in_sorted_list_by_name (char *name, uint32_t *sorted_label_list, uint32_t  label_list_entrys, DEBUG_INFOS_DATA *pappldata)
{
     int32_t dx, pos, count = 0;

     if (label_list_entrys == 0) return -1;
     pos = label_list_entrys / 2;
     dx = pos / 2;
     if (!dx) dx = 1;
     while (1) {
         int32_t cmp;  // -1 -> smaller, 0 -> equal, 1 ->larger
         char *l = pappldata->label_list[sorted_label_list[pos]].name;
         cmp = own_strcmp (name, l);
         if (cmp < 0) {
             if (pos == (int32_t)(label_list_entrys-1)) {
                 // Match or doesn't exist
                 return pos;
             } else {
                 l = pappldata->label_list[sorted_label_list[pos+1]].name;
                 cmp = own_strcmp (name, l);
                 if (cmp > 0) {
                     // Match
                     return pos;
                 } else {
                     // Upwards
                     pos += dx;
                     if (pos > (int32_t)(label_list_entrys-1)) pos = (int32_t)label_list_entrys-1;
                 }
             }
         } else if (cmp > 0) {
             if (pos == 0) {
                 // Hash code doesn't exist
                 return -1;
             } else {
                 l = pappldata->label_list[sorted_label_list[pos-1]].name;
                 cmp = own_strcmp (name, l);
                 if (cmp < 0) {
                     // Match
                     return pos-1;
                 } else {
                     // Downwards
                     pos -= dx;
                     if (pos < 0) pos = 0;
                 }
             }
         } else {
             // exact match Treffer (first element inside structure or single lable)
            return pos;
         }
         if (dx > 1) {
             dx = dx / 2;
         } else {
             if (count++ > 100) {
                 ThrowError (1, "Internel error %s [%i], name = %s", __FILE__, __LINE__, name);
                 break;
             }
         }
     }
     return -1;
}


static int32_t get_pos_in_dtype_list_by_dtype (int32_t typenr, DEBUG_INFOS_DATA *pappldata)
{
     int32_t dx, pos, count = 0;

     if (pappldata->dtype_list_entrys == 0) return -1;
     pos = pappldata->dtype_list_entrys / 2;
     dx = pos / 2;
     if (!dx) dx = 1;
     while (1) {
         if (pappldata->dtype_list[pos]->typenr < typenr) {
             if (pos == (int32_t)(pappldata->dtype_list_entrys-1)) {
                 // Match or doesn't exist
                 return pos;
             } else if (pappldata->dtype_list[pos+1]->typenr > typenr) {
                 // Match
                 return pos;
             } else {
                 // Upwards
                 pos += dx;
                 if (pos > (int32_t)(pappldata->dtype_list_entrys-1)) pos = (int32_t)pappldata->dtype_list_entrys-1;
             }
         } else if (pappldata->dtype_list[pos]->typenr > typenr) {
             if (pos == 0) {
                 // Pos doesn't exist
                 return -1;
             } else if (pappldata->dtype_list[pos-1]->typenr < typenr) {
                 // Match
                 return pos-1;
             } else {
                 // Downwards
                 pos -= dx;
                 if (pos < 0) pos = 0;
             }
         } else {
             // exact match Treffer (first element inside structure or single lable)
            return pos;
         }
         if (dx > 1) {
             dx = dx / 2;
         } else {
             if (count++ > 100) {
                 ThrowError (1, "Internel error %s [%i], typenr = 0x%X", __FILE__, __LINE__, typenr);
                 break;
             }
         }
     }
     return -1;
}

static int32_t get_pos_in_h_dtype_list_by_dtype (int32_t typenr, DEBUG_INFOS_DATA *pappldata)
{
     int32_t dx, pos, count = 0;

     if (pappldata->h_dtype_list_entrys == 0) return -1;
     pos = pappldata->h_dtype_list_entrys / 2;
     dx = pos / 2;
     if (!dx) dx = 1;
     while (1) {
         if (pappldata->h_dtype_list[pos]->typenr < typenr) {
             if (pos == (int32_t)(pappldata->h_dtype_list_entrys-1)) {
                 // Match or doesn't exist
                 return pos;
             } else if (pappldata->h_dtype_list[pos+1]->typenr > typenr) {
                 // Match
                 return pos;
             } else {
                 // Upwards
                 pos += dx;
                 if (pos > (int32_t)(pappldata->h_dtype_list_entrys-1)) pos = (int32_t)pappldata->h_dtype_list_entrys-1;
             }
         } else if (pappldata->h_dtype_list[pos]->typenr > typenr) {
             if (pos == 0) {
                 // Addresse doesn't exist
                 return -1;
             } else if (pappldata->h_dtype_list[pos-1]->typenr < typenr) {
                 // Match
                 return pos-1;
             } else {
                 // Downwards
                 pos -= dx;
                 if (pos < 0) pos = 0;
             }
         } else {
             // exact match Treffer (first element inside structure or single lable)
            return pos;
         }
         if (dx > 1) {
             dx = dx / 2;
         } else {
             if (count++ > 100) {
                 ThrowError (1, "Internel error %s [%i], typenr = 0x%X", __FILE__, __LINE__, typenr);
                 break;
             }
         }
     }
     return -1;
}

static int32_t get_pos_in_field_list_by_fieldnr (int32_t fieldnr, DEBUG_INFOS_DATA *pappldata)
{
     int32_t dx, pos, count = 0;

     if (pappldata->field_list_entrys == 0) return -1;
     pos = pappldata->field_list_entrys / 2;
     dx = pos / 2;
     if (!dx) dx = 1;
     while (1) {
         if (pappldata->field_list[pos]->fieldnr < fieldnr) {
             if (pos == (int32_t)(pappldata->field_list_entrys-1)) {
                 // Match or fieldnr doesn't exist
                 return pos;
             } else if (pappldata->field_list[pos+1]->fieldnr > fieldnr) {
                 // Match
                 return pos;
             } else {
                 // Upwards
                 pos += dx;
                 if (pos > (int32_t)(pappldata->field_list_entrys-1)) pos = (int32_t)pappldata->field_list_entrys-1;
             }
         } else if (pappldata->field_list[pos]->fieldnr > fieldnr) {
             if (pos == 0) {
                 // Addresse doesn't exist
                 return -1;
             } else if (pappldata->field_list[pos-1]->fieldnr < fieldnr) {
                 // Match
                 return pos-1;
             } else {
                 // Downwards
                 pos -= dx;
                 if (pos < 0) pos = 0;
             }
         } else {
             // exact match Treffer (first element inside structure or single lable)
            return pos;
         }
         if (dx > 1) {
             dx = dx / 2;
         } else {
             if (count++ > 100) {
                 ThrowError (1, "Internel error %s [%i], fieldnr = 0x%X", __FILE__, __LINE__, fieldnr);
                 break;
             }
         }
     }
     return -1;
}


int32_t SearchLabelByNameInternal (char *name, DEBUG_INFOS_DATA *pappldata)
{
    int32_t pos;

    pos = get_pos_in_sorted_list_by_name(name, pappldata->sorted_label_list, pappldata->label_list_entrys, pappldata);
    if (pos >= 0) {
         // Remark: there can exist identical names
        int32_t cmp;
        char *l;
        while(1) {
            if ((pos+1) >= (int32_t)pappldata->label_list_entrys) break;
            l = pappldata->label_list[pappldata->sorted_label_list[pos+1]].name;
            cmp = own_strcmp (name, l);
            if (cmp == 0) return (int32_t)pappldata->sorted_label_list[pos + 1];
            if (!((cmp <= 0) && (pos < (int32_t)(pappldata->label_list_entrys-1)))) break;
            pos++;
        }
        while(1) {
            if (pos < 0) break;
            l = pappldata->label_list[pappldata->sorted_label_list[pos]].name;
            cmp = own_strcmp (name, l);
            if (cmp == 0) return (int32_t)pappldata->sorted_label_list[pos];
            if (!((cmp > 0) && (pos > 0))) break;
            pos--;
        }
    }

    return -1;    // not found
}

int32_t SearchLabelByName (char *name, DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata)
{
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    if (pappldata == NULL) {
        return -1;
    }
    AssociatedProcess = pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return -1;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return -1;
    }
    return SearchLabelByNameInternal (name, AssociatedDebugInfos);
}

int32_t RenameLabel(DEBUG_INFOS_DATA *pappldata, char *Label, int32_t MaxChars);


int32_t insert_label (char *label, int32_t typenr, uint64_t adr, DEBUG_INFOS_DATA *pappldata)
{
    char *psym;
    int32_t pos;
    int32_t ret;
    char TempLabel[BBVARI_NAME_SIZE];

    BEGIN_RUNTIME_MEASSUREMENT ("insert_label")

    StringCopyMaxCharTruncate (TempLabel, label, sizeof(TempLabel));
    if (RenameLabel(pappldata, TempLabel, sizeof(TempLabel)) > 0) {
        label = TempLabel;
    }
    // Resize
    if ((pappldata->label_list_entrys + 1) >= pappldata->size_label_list) {
        pappldata->size_label_list += 1024;
        if (((pappldata->label_list = (LABEL_LIST_ELEM*)my_realloc (pappldata->label_list, sizeof (LABEL_LIST_ELEM) * pappldata->size_label_list)) == NULL) ||
            ((pappldata->addr_list = (ADDR_LIST_ELEM*)my_realloc (pappldata->addr_list, sizeof (ADDR_LIST_ELEM) * pappldata->size_label_list)) == NULL) ||
            ((pappldata->sorted_label_list = (uint32_t*)my_realloc (pappldata->sorted_label_list, sizeof (uint32_t) * pappldata->size_label_list)) == NULL)) {
            ThrowError (1, "sanity check %s %li", __FILE__, (int32_t)__LINE__);
            ret = -1;
            goto __OUT;
        }
    }
    psym = label;
    // remove special character
    while (*psym != '\0') {
        if (!isalnum(*psym) && (*psym != '_') && (*psym != ':') && (*psym != '@')) {
            ret = 0;
            goto __OUT;
        }
        psym++;
    }

    // Add to Adressliste
    pos = get_pos_in_address_list_by_address (adr, pappldata);
    if (pappldata->addr_list[pos].adr == adr) {
        if (pappldata->label_list[pappldata->addr_list[pos].label_idx].typenr != typenr) {
            pappldata->label_list[pappldata->addr_list[pos].label_idx].typenr = typenr;
        }
        ret = 0;
        goto __OUT;
    }
    pos++;
    memmove (&pappldata->addr_list[pos+1],
             &pappldata->addr_list[pos],
             (pappldata->label_list_entrys - (uint32_t)pos) * sizeof (ADDR_LIST_ELEM));
    pappldata->addr_list[pos].adr = adr;
    pappldata->addr_list[pos].label_idx = (int32_t)pappldata->label_list_entrys;

    // And add to sorted list
    pos = get_pos_in_sorted_list_by_name (label, pappldata->sorted_label_list, pappldata->label_list_entrys, pappldata);

    if (pos >= 0) {
        // Remark: there can exist identical names
        int32_t cmp;
        char *l;
        while(1) {
            if ((pos+1) >= (int32_t)pappldata->label_list_entrys) break;
            l = pappldata->label_list[pappldata->sorted_label_list[pos+1]].name;
            cmp = own_strcmp (label, l);
            if (!((cmp <= 0) && (pos < (int32_t)(pappldata->label_list_entrys-1)))) break;
            pos++;
        }
        while(1) {
            if (pos < 0) break;
            l = pappldata->label_list[pappldata->sorted_label_list[pos]].name;
            cmp = own_strcmp (label, l);
            if (!((cmp > 0) && (pos > 0))) break;
            pos--;
        }
    }
    pos++;
    memmove (&pappldata->sorted_label_list[pos+1],
             &pappldata->sorted_label_list[pos],
             (pappldata->label_list_entrys - (uint32_t)pos) * sizeof (uint32_t));
    pappldata->sorted_label_list[pos] = pappldata->label_list_entrys;


    // Add it to Labelliste
    if ((psym = insert_symbol (label, pappldata)) == NULL) {
        ret = 0;
        goto __OUT;
    }
    pappldata->label_list[pappldata->label_list_entrys].name = psym;
    pappldata->label_list[pappldata->label_list_entrys].typenr = typenr;
    pappldata->label_list[pappldata->label_list_entrys].adr = adr;
    pappldata->label_list[pappldata->label_list_entrys].typenr_idx = -1;

    pappldata->label_list_entrys++;
    ret = 0;
__OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}

int32_t insert_label_no_address (char *label, int32_t typenr, int32_t own_typenr, DEBUG_INFOS_DATA *pappldata)
{
    char *psym;
    char TempLabel[BBVARI_NAME_SIZE];

    StringCopyMaxCharTruncate (TempLabel, label, sizeof(TempLabel));
    if (RenameLabel(pappldata, TempLabel, sizeof(TempLabel)) > 0) {
        label = TempLabel;
    }
    // Groesse anpassen
    if ((pappldata->label_list_no_addr_entrys + 1) >= pappldata->size_label_list_no_addr) {
        pappldata->size_label_list_no_addr += 1024;
        if (((pappldata->label_list_no_addr = (LABEL_LIST_ELEM*)my_realloc (pappldata->label_list_no_addr, sizeof (LABEL_LIST_ELEM) * pappldata->size_label_list_no_addr)) == NULL)) {
            ThrowError (1, "sanity check %s %li", __FILE__, (int32_t)__LINE__);
            return -1;
        }
    }
    psym = label;
    // remove special character
    while (*psym != '\0') {
        if (!isalnum(*psym) && (*psym != '_') && (*psym != ':') && (*psym != '@')) return 0;
        psym++;
    }

    // Add it to Labellist
    if ((psym = insert_symbol (label, pappldata)) == NULL) {
        return 0;
    }
    pappldata->label_list_no_addr[pappldata->label_list_no_addr_entrys].name = psym;
    pappldata->label_list_no_addr[pappldata->label_list_no_addr_entrys].typenr = typenr;
    pappldata->label_list_no_addr[pappldata->label_list_no_addr_entrys].adr = (uint32_t)own_typenr;
    pappldata->label_list_no_addr[pappldata->label_list_no_addr_entrys].typenr_idx = -1;

    pappldata->label_list_no_addr_entrys++;
    return 0;
}



int32_t search_label_no_addr_by_typenr (int32_t typenr, char **ret_name, int32_t *ret_typenr, DEBUG_INFOS_DATA *pappldata)
{
    int32_t x;

    for (x = 0; x < (int32_t)pappldata->label_list_no_addr_entrys; x++) {
        if ((int32_t)pappldata->label_list_no_addr[x].adr == typenr) {
            *ret_name = pappldata->label_list_no_addr[x].name;
            *ret_typenr = pappldata->label_list_no_addr[x].typenr;
            return 0;
        }
    }
    return -1;
}

int32_t insert_struct (int16_t what, int32_t typenr, const char * const struct_name,
                   int32_t fields, int32_t fieldsidx, int32_t pointsto,
                   int32_t array_elements, int32_t array_elem_typenr, int32_t array_size,
                   DEBUG_INFOS_DATA *pappldata)
{
    UNUSED(fields);
    UNUSED(typenr);
    char *psym;
    int32_t pos;
    int32_t ret;
    uint32_t x;
    DTYPE_LIST_ELEM *save;

    BEGIN_RUNTIME_MEASSUREMENT ("insert_struct")

    if (what == FIELD_ELEM) {
        ThrowError (1, "Internal error: what == FIELD_ELEM");
    }

    if (typenr < USER_DEF_TYPEID_LIMIT) {
        ThrowError (1, "Internal error: %s %li", __FILE__, (int32_t)__LINE__);
        ret = -1;
        goto __OUT;
    }

    if ((psym = insert_symbol (struct_name, pappldata)) == NULL) {
        ret = 0;
        goto __OUT;
    }

    if (typenr < 0x40000000) {
        // resize
        if ((pappldata->dtype_list_entrys + 1) >= pappldata->size_dtype_list) {
            pappldata->size_dtype_list += 1024;
            if ((pappldata->dtype_list =(DTYPE_LIST_ELEM**)my_realloc (pappldata->dtype_list, sizeof (DTYPE_LIST_ELEM*) * pappldata->size_dtype_list)) == NULL) {
                ThrowError (1, "out of memory, cannot load debug infos for process \"%s\" (%s %li)", pappldata->ExecutableFileName, __FILE__, (int32_t)__LINE__);
                ret = -1;
                goto __OUT;
            }
            pappldata->dtype_list_1k_blocks_entrys++;
            pappldata->dtype_list_1k_blocks_active_entry++;
            if (pappldata->dtype_list_1k_blocks_entrys >= pappldata->size_dtype_list_1k_blocks) {
                pappldata->size_dtype_list_1k_blocks += 1024;
                if ((pappldata->dtype_list_1k_blocks =(DTYPE_LIST_ELEM**)my_realloc (pappldata->dtype_list_1k_blocks, sizeof (DTYPE_LIST_ELEM**) * pappldata->size_dtype_list_1k_blocks)) == NULL) {
                    ThrowError (1, "out of memory, cannot load debug infos for process \"%s\" (%s %li)", pappldata->ExecutableFileName, __FILE__, (int32_t)__LINE__);
                    ret = -1;
                    goto __OUT;
                }
            }
            // 1024 new elements
            if ((pappldata->dtype_list_1k_blocks[pappldata->dtype_list_1k_blocks_active_entry] = (DTYPE_LIST_ELEM*)my_calloc (sizeof (DTYPE_LIST_ELEM), 1024)) == NULL) {
                ThrowError (1, "out of memory, cannot load debug infos for process \"%s\" (%s %li)", pappldata->ExecutableFileName, __FILE__, (int32_t)__LINE__);
                ret = -1;
                goto __OUT;
            }
            for (x = 0; x < 1024; x++) {
                pappldata->dtype_list[pappldata->size_dtype_list - 1024 + (uint32_t)x] = &(pappldata->dtype_list_1k_blocks[pappldata->dtype_list_1k_blocks_active_entry][x]);
            }
        }

        // Sort into data type liste (VC are unsorted)
        BEGIN_RUNTIME_MEASSUREMENT ("insert_struct::get_pos_in_dtype_list_by_dtype")
        pos = get_pos_in_dtype_list_by_dtype (typenr, pappldata);
        END_RUNTIME_MEASSUREMENT
        pos++;
        save = pappldata->dtype_list[pappldata->dtype_list_entrys];
        BEGIN_RUNTIME_MEASSUREMENT ("insert_struct::memmove")
        if (pos < (int32_t)pappldata->dtype_list_entrys) {
            memmove (&pappldata->dtype_list[pos+1],
                     &pappldata->dtype_list[pos],
                     (pappldata->dtype_list_entrys - (uint32_t)pos) * sizeof (DTYPE_LIST_ELEM*));
        } else {

        }
        END_RUNTIME_MEASSUREMENT
        pappldata->dtype_list[pos] = save;
        pappldata->dtype_list_entrys++;
    } else {
        // List of additional automatic generated data type ids (0x7FFFFFFF downwards to 0x40000000)
        // resize
        if ((pappldata->h_dtype_list_entrys + 1) >= pappldata->h_size_dtype_list) {
            pappldata->h_size_dtype_list += 1024;
            if ((pappldata->h_dtype_list =(DTYPE_LIST_ELEM**)my_realloc (pappldata->h_dtype_list, sizeof (DTYPE_LIST_ELEM*) * pappldata->h_size_dtype_list)) == NULL) {
                ThrowError (1, "out of memory, cannot load debug infos for process \"%s\" (%s %li)", pappldata->ExecutableFileName, __FILE__, (int32_t)__LINE__);
                ret = -1;
                goto __OUT;
            }
            pappldata->h_dtype_list_1k_blocks_entrys++;
            pappldata->h_dtype_list_1k_blocks_active_entry++;
            if (pappldata->h_dtype_list_1k_blocks_entrys >= pappldata->h_size_dtype_list_1k_blocks) {
                pappldata->h_size_dtype_list_1k_blocks += 1024;
                if ((pappldata->h_dtype_list_1k_blocks =(DTYPE_LIST_ELEM**)my_realloc (pappldata->h_dtype_list_1k_blocks, sizeof (DTYPE_LIST_ELEM**) * pappldata->h_size_dtype_list_1k_blocks)) == NULL) {
                    ThrowError (1, "out of memory, cannot load debug infos for process \"%s\" (%s %li)", pappldata->ExecutableFileName, __FILE__, (int32_t)__LINE__);
                    ret = -1;
                    goto __OUT;
                }
            }
            // 1024 neue allozieren
            if ((pappldata->h_dtype_list_1k_blocks[pappldata->h_dtype_list_1k_blocks_active_entry] = (DTYPE_LIST_ELEM*)my_calloc (sizeof (DTYPE_LIST_ELEM), 1024)) == NULL) {
                ThrowError (1, "out of memory, cannot load debug infos for process \"%s\" (%s %li)", pappldata->ExecutableFileName, __FILE__, (int32_t)__LINE__);
                ret = -1;
                goto __OUT;
            }
            for (x = 0; x < 1024; x++) {
                pappldata->h_dtype_list[pappldata->h_size_dtype_list - 1024 + x] = &(pappldata->h_dtype_list_1k_blocks[pappldata->h_dtype_list_1k_blocks_active_entry][x]);
            }
        }

        // Sort into data type liste (VC are unsorted)
        BEGIN_RUNTIME_MEASSUREMENT ("insert_struct::get_pos_in_h_dtype_list_by_dtype")
        pos = get_pos_in_h_dtype_list_by_dtype (typenr, pappldata);
        END_RUNTIME_MEASSUREMENT
        pos++;
        save = pappldata->h_dtype_list[pappldata->h_dtype_list_entrys];
        BEGIN_RUNTIME_MEASSUREMENT ("insert_struct::memmove(h)")
        if (pos < (int32_t)pappldata->h_dtype_list_entrys) {
            memmove (&pappldata->h_dtype_list[pos+1],
                     &pappldata->h_dtype_list[pos],
                     (pappldata->h_dtype_list_entrys - (uint32_t)pos) * sizeof (DTYPE_LIST_ELEM*));
        } else {

        }
        END_RUNTIME_MEASSUREMENT
        pappldata->h_dtype_list[pos] = save;
        pappldata->h_dtype_list_entrys++;
    }
    save->what = (uint16_t)what;
    save->name = psym;
    save->typenr = typenr;
    save->typenr_fields = fieldsidx;
    save->typenr_fields_idx = -1;
    save->array_elem_typenr = array_elem_typenr;
    save->array_elem_typenr_idx = -1;
    save->array_elements = array_elements;
    save->array_size = array_size;
    save->pointsto = pointsto;
    save->pointsto_idx = -1;
    ret = 0;
__OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}



int32_t insert_field (int32_t fieldnr,
                  DEBUG_INFOS_DATA *pappldata)
{
    int32_t pos;
    int32_t ret;
    int32_t x;
    FIELD_LIST_ELEM *save;

    BEGIN_RUNTIME_MEASSUREMENT ("insert_struct")
    if (fieldnr < USER_DEF_TYPEID_LIMIT) {
        ThrowError (1, "Internal error: %s %li", __FILE__, (int32_t)__LINE__);
        ret = -1;
        goto __OUT;
    }

    // resize
    if ((pappldata->field_list_entrys + 1) >= pappldata->size_field_list) {
        pappldata->size_field_list += 1024;
        if ((pappldata->field_list =(FIELD_LIST_ELEM**)my_realloc (pappldata->field_list, sizeof (FIELD_LIST_ELEM*) * pappldata->size_field_list)) == NULL) {
            ThrowError (1, "out of memory, cannot load debug infos for process \"%s\" (%s %li)", pappldata->ExecutableFileName, __FILE__, (int32_t)__LINE__);
            ret = -1;
            goto __OUT;
        }
        pappldata->field_list_1k_blocks_entrys++;
        pappldata->field_list_1k_blocks_active_entry++;
        if (pappldata->field_list_1k_blocks_entrys >= pappldata->size_field_list_1k_blocks) {
            pappldata->size_field_list_1k_blocks += 1024;
            if ((pappldata->field_list_1k_blocks =(FIELD_LIST_ELEM**)my_realloc (pappldata->field_list_1k_blocks, sizeof (FIELD_LIST_ELEM**) * pappldata->size_field_list_1k_blocks)) == NULL) {
                ThrowError (1, "out of memory, cannot load debug infos for process \"%s\" (%s %li)", pappldata->ExecutableFileName, __FILE__, (int32_t)__LINE__);
                ret = -1;
                goto __OUT;
            }
        }
        // 1024 new elements
        if ((pappldata->field_list_1k_blocks[pappldata->field_list_1k_blocks_active_entry] = (FIELD_LIST_ELEM*)my_calloc (sizeof (FIELD_LIST_ELEM), 1024)) == NULL) {
            ThrowError (1, "out of memory, cannot load debug infos for process \"%s\" (%s %li)", pappldata->ExecutableFileName, __FILE__, (int32_t)__LINE__);
            ret = -1;
            goto __OUT;
        }
        for (x = 0; x < 1024; x++) {
            pappldata->field_list[pappldata->size_field_list - 1024 + (uint32_t)x] = &(pappldata->field_list_1k_blocks[pappldata->field_list_1k_blocks_active_entry][x]);
        }
    }

    // Sort into data type liste (VC are unsorted)
    BEGIN_RUNTIME_MEASSUREMENT ("insert_struct::get_pos_in_field_list_by_dtype")
    pos = get_pos_in_field_list_by_fieldnr (fieldnr, pappldata);
    END_RUNTIME_MEASSUREMENT
    pos++;
    save = pappldata->field_list[pappldata->field_list_entrys];
    BEGIN_RUNTIME_MEASSUREMENT ("insert_struct::memmove")
    if (pos < (int32_t)pappldata->field_list_entrys) {
        memmove (&pappldata->field_list[pos+1],
                    &pappldata->field_list[pos],
                    (pappldata->field_list_entrys - (uint32_t)pos) * sizeof (FIELD_LIST_ELEM*));
    } else {

    }
    END_RUNTIME_MEASSUREMENT
    pappldata->field_list[pos] = save;
    pappldata->field_list_entrys++;

    save->fieldnr = fieldnr;
    save->start_fieldidx = -1;
    ret = 0;
__OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}


static DTYPE_LIST_ELEM *search_structure_definition_with_name_inside_compile_unit (char *name, uint32_t typenr, int32_t compile_unit, DEBUG_INFOS_DATA *pappldata, int32_t *ret_Index)
{
    UNUSED(typenr);
    UNUSED(compile_unit);
    int32_t x;

    for (x = 0; x < (int32_t)pappldata->dtype_list_entrys; x++) {
        if (pappldata->dtype_list[x]->what == STRUCT_ELEM) {         // All structure member
            if (!strcmp (pappldata->dtype_list[x]->name, name)) {    // with same namen
                if (pappldata->dtype_list[x]->typenr >= 0x40000000) {
                    if (ret_Index != NULL) *ret_Index = x + 0x40000000;
                } else {
                    if (ret_Index != NULL) *ret_Index = x;
                }
                return pappldata->dtype_list[x];
            }
        }
    }
    return NULL;
}


static DTYPE_LIST_ELEM *search_dtype (int32_t typenr, DEBUG_INFOS_DATA *pappldata, int32_t *ret_Index)
{
    DTYPE_LIST_ELEM **itemptr_ptr;
    DTYPE_LIST_ELEM key;
    DTYPE_LIST_ELEM *key_ptr;

    if (ret_Index != NULL) {
        if (*ret_Index > 0) {   // Was element searched before?
            if (*ret_Index > 0x40000000) {
                return (pappldata->dtype_list[*ret_Index]);
            } else {
                return (pappldata->h_dtype_list[*ret_Index]);
            }
        }
    }

    /* only debugging {
        int32_t x;
        int32_t last = 0;
        for (x = 0; x < pappldata->dtype_list_entrys; x++) {
            if (last >= pappldata->dtype_list[x]->typenr) {
                printf ("Stop");
            }
            last = pappldata->dtype_list[x]->typenr;
            if (pappldata->dtype_list[x]->typenr == typenr) {
                printf ("Stop");
            }
        }
    }*/

    key.typenr = typenr;
    key_ptr = &key;
    if (typenr < 0x40000000) {
        itemptr_ptr = (DTYPE_LIST_ELEM**)bsearch (&key_ptr, pappldata->dtype_list,
                                                  pappldata->dtype_list_entrys,
                                                  sizeof(DTYPE_LIST_ELEM*), (CMP_FUNC_PTR)comp_typenr_func);
        if (itemptr_ptr != NULL) {
            DTYPE_LIST_ELEM *itemptr = *itemptr_ptr;
            // If a predeclaration
            if (itemptr->what == PRE_DEC_STRUCT) {
                // Than search this inside the same compile unit on the basis of its name
                return search_structure_definition_with_name_inside_compile_unit (itemptr->name, (uint32_t)itemptr->typenr, itemptr->pointsto /* Compile Unit*/, pappldata, ret_Index);
            } else {
                if (ret_Index != NULL) *ret_Index = (int32_t)(itemptr_ptr - pappldata->dtype_list);
                return *itemptr_ptr;
            }
        } else {
            return NULL;
        }
    } else {
        // List of additional automatic generated data type ids (0x7FFFFFFF downwards to 0x40000000)
        itemptr_ptr = (DTYPE_LIST_ELEM**)bsearch (&key_ptr, pappldata->h_dtype_list,
                                                  pappldata->h_dtype_list_entrys,
                                                  sizeof(DTYPE_LIST_ELEM*), (CMP_FUNC_PTR)comp_typenr_func);
        if (itemptr_ptr != NULL) {
            if (ret_Index != NULL) *ret_Index = (int32_t)(itemptr_ptr - pappldata->h_dtype_list) + 0x40000000;
            return *itemptr_ptr;
        } else {
            return NULL;
        }
    }
}

static FIELD_LIST_ELEM *search_field (int32_t fieldnr, DEBUG_INFOS_DATA *pappldata, int32_t *ret_Index)
{
    FIELD_LIST_ELEM **itemptr_ptr;
    FIELD_LIST_ELEM key;
    FIELD_LIST_ELEM *key_ptr;

    key.fieldnr = fieldnr;
    key_ptr = &key;
    itemptr_ptr = (FIELD_LIST_ELEM**)bsearch (&key_ptr, pappldata->field_list,
                                              pappldata->field_list_entrys,
                                              sizeof(FIELD_LIST_ELEM*), (CMP_FUNC_PTR)comp_fieldnr_func);
    if (itemptr_ptr != NULL) {
        if (ret_Index != NULL) *ret_Index = (int32_t)(itemptr_ptr - pappldata->field_list);
        return *itemptr_ptr;
    } else {
        return NULL;
    }
}

// Only for Dwarf

int32_t SetDeclarationToSpecification (int32_t typenr, DEBUG_INFOS_DATA *pappldata)
{
    DTYPE_LIST_ELEM **itemptr_ptr;
    DTYPE_LIST_ELEM key;
    DTYPE_LIST_ELEM *key_ptr;

    key.typenr = typenr;
    key_ptr = &key;
    if (typenr < 0x40000000) {
        itemptr_ptr = (DTYPE_LIST_ELEM**)bsearch (&key_ptr, pappldata->dtype_list,
                                                  pappldata->dtype_list_entrys,
                                                  sizeof(DTYPE_LIST_ELEM*), (CMP_FUNC_PTR)comp_typenr_func);
        if (itemptr_ptr != NULL) {
            DTYPE_LIST_ELEM *itemptr = *itemptr_ptr;
            // If a previous declaration exist
            if (itemptr->what == PRE_DEC_STRUCT) {
                itemptr->what = STRUCT_ELEM;
            } else return -1;
        } else {
            return -1;
        }
    } else {
        // List of additional automatic generated data type ids (0x7FFFFFFF downwards to 0x40000000)
        itemptr_ptr = (DTYPE_LIST_ELEM**)bsearch (&key_ptr, pappldata->h_dtype_list,
                                                  pappldata->h_dtype_list_entrys,
                                                  sizeof(DTYPE_LIST_ELEM*), (CMP_FUNC_PTR)comp_typenr_func);
        if (itemptr_ptr != NULL) {
            DTYPE_LIST_ELEM *itemptr = *itemptr_ptr;
            // If a previous declaration exist
            if (itemptr->what == PRE_DEC_STRUCT) {
                itemptr->what = STRUCT_ELEM;
            } else return -1;
        } else {
            return -1;
        }
    }
    return 0;   // OK
}

// end Dwarf

static int32_t connect_struct_field (int32_t fieldnr, int32_t my_idx, DEBUG_INFOS_DATA *pappldata)
{
    int32_t idx;
    FIELD_LIST_ELEM *itemptr;
    int32_t ret;

    BEGIN_RUNTIME_MEASSUREMENT ("connect_struct_field")

    itemptr = search_field (fieldnr, pappldata, NULL);
    if (itemptr != NULL) {
        if (itemptr->fieldnr == fieldnr) { // parent fields found
            idx = itemptr->start_fieldidx;
            if (idx == -1) {   // erster Eintrag
                itemptr->start_fieldidx = my_idx;
            } else {
                while (pappldata->fieldidx_list[idx].nextidx != -1) {
                    idx = pappldata->fieldidx_list[idx].nextidx;
                }
                pappldata->fieldidx_list[idx].nextidx = my_idx;
            }
            itemptr->fields++;   // Number of structure elements
            ret = 0;
            goto __OUT;
        }
    }
    ret = -1;
__OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}

int32_t insert_struct_field (int32_t fieldnr, int32_t typenr,
                         char *field_name, int32_t offset, DEBUG_INFOS_DATA *pappldata)
{
    char *psym;
    int32_t ret;

    BEGIN_RUNTIME_MEASSUREMENT ("insert_struct_field")
    if (fieldnr < USER_DEF_TYPEID_LIMIT) {
        ThrowError (1, "Internal error: %s %li", __FILE__, (int32_t)__LINE__);
        ret = -1;
        goto __OUT;
    }
    // resize
    if ((pappldata->fieldidx_list_entrys + 1) >= pappldata->size_fieldidx_list) {
        pappldata->size_fieldidx_list += 64*1024;
        if ((pappldata->fieldidx_list =(FIELDIDX_LIST_ELEM*)my_realloc (pappldata->fieldidx_list, sizeof (FIELDIDX_LIST_ELEM) * pappldata->size_fieldidx_list)) == NULL) {
            ThrowError (1, "Internal error: %s %li", __FILE__, (int32_t)__LINE__);
            ret = -1;
            goto __OUT;
        }
    }
    if ((psym = insert_symbol (field_name, pappldata)) == NULL) {
        ret = 0;
        goto __OUT;
    }
    pappldata->fieldidx_list[pappldata->fieldidx_list_entrys].name = psym;
    pappldata->fieldidx_list[pappldata->fieldidx_list_entrys].typenr = typenr;
    pappldata->fieldidx_list[pappldata->fieldidx_list_entrys].offset = offset;
    pappldata->fieldidx_list[pappldata->fieldidx_list_entrys].typenr_idx = -1; // Index not known
    pappldata->fieldidx_list[pappldata->fieldidx_list_entrys].nextidx = -1;

    if (connect_struct_field (fieldnr, (int32_t)pappldata->fieldidx_list_entrys, pappldata)) {
        ThrowError (1, "Internal error: %s %li", __FILE__, (int32_t)__LINE__);
        ret = -1;
        goto __OUT;
    }

    pappldata->fieldidx_list_entrys++;
    ret = 0;
__OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}


#define CHECK_DLIST_RANGE(idx) if ((idx < 0) || ((idx) >= (int32_t)AssociatedDebugInfos->dtype_list_entrys)) ThrowError (1, "Internel error %s(%i)", __FILE__, __LINE__)

#define CHECK_H_DLIST_RANGE(idx) if (((idx) < 0x40000000) || (((idx) - 0x40000000) >= (int32_t)AssociatedDebugInfos->h_dtype_list_entrys)) ThrowError (1, "Internel error %s(%i)", __FILE__, __LINE__)

int32_t get_what_is_typenr (int32_t *typenr,           // in out
                            int32_t *ptypenr_idx,      // in out: wenn Inhalt -1 Index ist noch unbekannt deshalb suchen
                                                       // wenn != -1 Index keine Suche mehr noetig
                            DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata)
{
    DTYPE_LIST_ELEM *itemptr;
    //int32_t idx = -1;
    int32_t ret;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
       // (AssociatedProcess == &DebugInfosAssociatedEmptyProcess)) {
        return -1;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return -1;
    }
    if (*typenr < USER_DEF_TYPEID_LIMIT) return 1;  // Basis-Datentyp (int32_t, char, float, ...)
    if (*ptypenr_idx < 0) {   // Index ist noch ungueltig -> Suchen
        itemptr = search_dtype (*typenr, AssociatedDebugInfos, NULL); //&idx);
        if (itemptr != NULL) {
            //*ptypenr_idx = idx
            ret = itemptr->what;
            if ((ret == MODIFIER_ELEM) || (ret == TYPEDEF_ELEM)) {
                int32_t modify_typenr;
                int32_t modify_what;
                if (!get_modify_what_internal (&modify_typenr, &modify_what, *typenr, AssociatedDebugInfos)) {
                    ThrowError (1, "internel error %s (%i)", __FILE__, __LINE__);
                    ret = -1;
                } else {
                    ret = modify_what;
                    *typenr = modify_typenr;
                }
            } else {
                //*ptypenr_idx = idx;   // index nur eintragen falls kein Modifier! sonst gehts 7 Zeilen weiter unten schief!
            }
        } else {
            ThrowError (1, "internel error %s (%i)", __FILE__, __LINE__);
            ret = -1;
        }
    } else {
        if (*ptypenr_idx < 0x40000000) {
            CHECK_DLIST_RANGE (*ptypenr_idx);
            ret = AssociatedDebugInfos->dtype_list[*ptypenr_idx]->what;
        } else {
            // List of additional automatic generated data type ids (0x7FFFFFFF downwards to 0x40000000)
            CHECK_H_DLIST_RANGE (*ptypenr_idx);
            ret = AssociatedDebugInfos->h_dtype_list[*ptypenr_idx - 0x40000000]->what;
        }
    }
    return ret;
}

char *get_struct_elem_by_offset (int32_t parent_typenr,
                                 int32_t *ptypenr_idx,      // in out: wenn Inhalt -1 Index ist noch unbekannt deshalb suchen
                                                         // wenn != -1 Index keine Suche mehr noetig
                                 int32_t offset,   // in: gesuchter Offset
                                 int32_t *poffset, // out: gefundener Offset
                                 int32_t *pbclass,           // out: ist es eine Basis-Klasse
                                 int32_t *ptypenr, // out: Typnummer
                                 int32_t *pfield_idx,       // out: Index im Typnummer-Array oder -1 falls noch unbekannt
                                 DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    UNUSED(pfield_idx);
    DTYPE_LIST_ELEM *itemptr;
    FIELD_LIST_ELEM *itemptr_y;
    int32_t idx;
    int32_t y;
    char *ret = NULL;
    int32_t best_fit_offset = -1;
    int32_t best_fit_idx = 0;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        //(AssociatedProcess == &DebugInfosAssociatedEmptyProcess)) {
        return NULL;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return NULL;
    }

    BEGIN_RUNTIME_MEASSUREMENT ("get_struct_elem_by_offset")
    if (parent_typenr < USER_DEF_TYPEID_LIMIT) {
        ret = NULL;  // Basis-Datentyp (int32_t, char, float, ...)
        goto __OUT;
    }
    if (*ptypenr_idx < 0) {   // Index ist noch ungueltig -> Suchen
        itemptr = search_dtype (parent_typenr, AssociatedDebugInfos, NULL); //&idx);
        //*ptypenr_idx = idx;
    } else {
        if (*ptypenr_idx < 0x40000000) {
            CHECK_DLIST_RANGE (*ptypenr_idx);
            itemptr = AssociatedDebugInfos->dtype_list[*ptypenr_idx];
        } else {
            // List of additional automatic generated data type ids (0x7FFFFFFF downwards to 0x40000000)
            CHECK_H_DLIST_RANGE (*ptypenr_idx);
            itemptr = AssociatedDebugInfos->h_dtype_list[*ptypenr_idx - 0x40000000];
        }
    }
    if (itemptr != NULL) {
        // Ignorieren modifier (ELF-File typedef)
        if ((itemptr->what == MODIFIER_ELEM) || (itemptr->what == TYPEDEF_ELEM)) {
            // ?
        }
        if (itemptr->what != STRUCT_ELEM) {
             ThrowError (1, "sanity check %s %li", __FILE__, (int32_t)__LINE__);
             ret = NULL;
             goto __OUT;
        }
        if (itemptr->typenr_fields_idx >= 0) {  // Fieldlist was searched before
             y = itemptr->typenr_fields_idx;
             itemptr_y = AssociatedDebugInfos->field_list[y];
             goto __FOUND;
        }
        // Search Fieldlist
        itemptr_y = search_field (itemptr->typenr_fields, AssociatedDebugInfos, &y);
        if (itemptr_y != NULL) {
            if (itemptr->typenr_fields == itemptr_y->fieldnr) {
                itemptr->typenr_fields_idx = y;
                __FOUND:
                for (idx = itemptr_y->start_fieldidx;
                        idx != -1;            // -1 is the end of Fieldlist
                        idx = AssociatedDebugInfos->fieldidx_list[idx].nextidx) {
                    // Offset of the element must be smaller or equal
                    if ((AssociatedDebugInfos->fieldidx_list[idx].offset <= offset) &&
                        // Take always the last matching element
                        // There can exist more elementes with same offset
                        // if an element have the size  0 byte
                        (AssociatedDebugInfos->fieldidx_list[idx].offset >= best_fit_offset)) {

                        best_fit_offset = AssociatedDebugInfos->fieldidx_list[idx].offset;
                        best_fit_idx = idx;
                    }
                }
                if (best_fit_offset >= 0) {
                    *ptypenr = AssociatedDebugInfos->fieldidx_list[best_fit_idx].typenr;
                    *poffset = best_fit_offset;
                    // Structure member with namen "public" are  base classes
                    // this will be handle like member objects with the class name as member name
                    if (!strcmp ("public", AssociatedDebugInfos->fieldidx_list[best_fit_idx].name)) {
                        char *typestr;
                        if ((typestr = get_struct_entry_typestr_internal (NULL, best_fit_idx, NULL, AssociatedDebugInfos)) != NULL) {
                            *pbclass = 1;
                            ret = typestr;
                            goto __OUT;
                        }
                    }
                    *pbclass = 0;
                    ret = AssociatedDebugInfos->fieldidx_list[best_fit_idx].name;
                } else {
                    ret = NULL;    // Addresse do not match any element of the structure (object)
                }
            }
        }
    }
  __OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}



char *get_label_by_address_ex (uint64_t addr,   // in
                               uint64_t *pbase_addr,  // out
                               int32_t *ptypenr,   // out
                               int32_t *ptypenr_idx,  // out
                               int32_t **pptypenr_idx, // out: Adresse of the index if it is not -1
                               DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata)
{
    int32_t idx, pos;
    char *ret;
    uint64_t BaseAddress;
    uint64_t Address;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return NULL;
    }
    BaseAddress = AssociatedProcess->BaseAddress;
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return NULL;
    }

    BEGIN_RUNTIME_MEASSUREMENT ("get_label_by_address_ex")
    switch (AssociatedDebugInfos->AbsoluteOrRelativeAddress) {
    default:
    case DEBUGINFO_ADDRESS_ABSOLUTE:
        Address = addr;
        break;
    case DEBUGINFO_ADDRESS_RELATIVE:
        Address = addr - BaseAddress;
        break;
    }
    pos = get_pos_in_address_list_by_address (Address, AssociatedDebugInfos);
    if (pos >= 0) {
        idx = AssociatedDebugInfos->addr_list[pos].label_idx;
        ret = AssociatedDebugInfos->label_list[idx].name;
        switch (AssociatedDebugInfos->AbsoluteOrRelativeAddress) {
        default:
        case DEBUGINFO_ADDRESS_ABSOLUTE:
            *pbase_addr = AssociatedDebugInfos->label_list[idx].adr;
            break;
        case DEBUGINFO_ADDRESS_RELATIVE:
            *pbase_addr = BaseAddress + AssociatedDebugInfos->label_list[idx].adr;
            break;
        }
        *ptypenr = AssociatedDebugInfos->label_list[idx].typenr;
        *ptypenr_idx = AssociatedDebugInfos->label_list[idx].typenr_idx;
        *pptypenr_idx = &(AssociatedDebugInfos->label_list[idx].typenr_idx);
    } else {
        ret = NULL;
    }
    END_RUNTIME_MEASSUREMENT
    return ret;
}

char *get_label_by_address (uint64_t addr,
                            DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata)
{
    int32_t idx, pos;
    char *ret;
    uint64_t BaseAddress;
    uint64_t Address;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return NULL;
    }
    BaseAddress = AssociatedProcess->BaseAddress;
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return NULL;
    }

    BEGIN_RUNTIME_MEASSUREMENT ("get_label_by_address")
    switch (AssociatedDebugInfos->AbsoluteOrRelativeAddress) {
    default:
    case DEBUGINFO_ADDRESS_ABSOLUTE:
        Address = addr;
        break;
    case DEBUGINFO_ADDRESS_RELATIVE:
        Address = addr - BaseAddress;
        break;
    }
    pos = get_pos_in_address_list_by_address (Address, AssociatedDebugInfos);
    if (pos >= 0) {
        idx = AssociatedDebugInfos->addr_list[pos].label_idx;
        ret = AssociatedDebugInfos->label_list[idx].name;
    } else {
        ret = NULL;
    }
    END_RUNTIME_MEASSUREMENT
    return ret;
}


char *get_next_label (int32_t *pidx, DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata)
{
    char *ret;
    uint64_t BaseAddress;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return NULL;
    }
    BaseAddress = AssociatedProcess->BaseAddress;
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return NULL;
    }

    BEGIN_RUNTIME_MEASSUREMENT ("get_next_label")
    if (pappldata == NULL) {
        ret = NULL;
    } else if (AssociatedDebugInfos->label_list == NULL) {
        ret = NULL;
    } else if (*pidx >= (int32_t)AssociatedDebugInfos->label_list_entrys) {
        ret = NULL;
    } else {
        ret = AssociatedDebugInfos->label_list[(*pidx)++].name;
    }
    END_RUNTIME_MEASSUREMENT
    return ret;
}


int32_t get_next_sorted_label_index(int32_t *pidx, DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata)
{
    int32_t ret;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return -1;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return -1;
    }
    if (*pidx >= (int32_t)AssociatedDebugInfos->label_list_entrys) {
        ret = -1;
    } else {
        ret = (int32_t)AssociatedDebugInfos->sorted_label_list[*pidx];
        *pidx = *pidx + 1;
    }
    return ret;
}

char *get_next_label_ex (int32_t *pidx, uint64_t *paddr, int32_t *ptypenr, DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata)
{
    char *ret;
    uint64_t BaseAddress;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return NULL;
    }
    BaseAddress = AssociatedProcess->BaseAddress;
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return NULL;
    }

    BEGIN_RUNTIME_MEASSUREMENT ("get_next_label_ex")
    if (AssociatedDebugInfos == NULL) {
        ret = NULL;
    } else if (AssociatedDebugInfos->label_list == NULL) {
        ret = NULL;
    } else if (*pidx >= (int32_t)AssociatedDebugInfos->label_list_entrys) {
        ret = NULL;
    } else {
        switch (AssociatedDebugInfos->AbsoluteOrRelativeAddress) {
        default:
        case DEBUGINFO_ADDRESS_ABSOLUTE:
            *paddr = AssociatedDebugInfos->label_list[*pidx].adr;
            break;
        case DEBUGINFO_ADDRESS_RELATIVE:
            *paddr = BaseAddress + AssociatedDebugInfos->label_list[*pidx].adr;
            break;
        }
        *ptypenr = AssociatedDebugInfos->label_list[*pidx].typenr;
        ret = AssociatedDebugInfos->label_list[(*pidx)++].name;
    }
    END_RUNTIME_MEASSUREMENT
    return ret;
}


#define DEFAULT_RET_BUFFER_SIZE  1024
#define MAX_MEM_BUFFER_ENTRYS  64

char *get_return_buffer_func_thread (void *func_addr, int32_t needed_size)
{
    struct MEM_BUFFER_ELEM {
        void *func_addr;
        char *ptr;
        int32_t thread_id;
        int32_t len;
    };
    static struct MEM_BUFFER_ELEM mem_buffer[MAX_MEM_BUFFER_ENTRYS];
    int32_t x;

    if ((func_addr == NULL) && (needed_size == -1)) {
        // free all buffers
        EnterCriticalSection (&ReturnBufferCriticalSection);
        for (x = 0; x < MAX_MEM_BUFFER_ENTRYS; x++) {
            if (mem_buffer[x].ptr != NULL) my_free(mem_buffer[x].ptr);
            mem_buffer[x].ptr  = NULL;
            mem_buffer[x].func_addr = NULL;
        }
        LeaveCriticalSection (&ReturnBufferCriticalSection);
        return NULL;
    } else {
        int32_t thread_id = (int32_t)GetCurrentThreadId();
        for (x = 0; x < MAX_MEM_BUFFER_ENTRYS; x++) {
            if ((mem_buffer[x].func_addr == func_addr) && (mem_buffer[x].thread_id == thread_id)) {
                if (mem_buffer[x].len < needed_size) {
                    mem_buffer[x].ptr = (char*)my_realloc (mem_buffer[x].ptr, (size_t)needed_size);
                    mem_buffer[x].len = needed_size;
                }
                mem_buffer[x].ptr[0] = '\0';
                return mem_buffer[x].ptr;
            }
        }
        // Ther exist no thread/function entry
        EnterCriticalSection (&ReturnBufferCriticalSection);
        for (x = 0; x < MAX_MEM_BUFFER_ENTRYS; x++) {
            if ((mem_buffer[x].func_addr == NULL) && (mem_buffer[x].thread_id == 0)) {
                mem_buffer[x].ptr = (char*)my_malloc ((size_t)needed_size);
                if (mem_buffer[x].ptr != NULL) {
                    mem_buffer[x].func_addr = func_addr;
                    mem_buffer[x].thread_id = thread_id;
                    mem_buffer[x].len = needed_size;
                }
                LeaveCriticalSection (&ReturnBufferCriticalSection);
                mem_buffer[x].ptr[0] = '\0';
                return mem_buffer[x].ptr;
            }
        }
        LeaveCriticalSection (&ReturnBufferCriticalSection);
        ThrowError (1, "No free return buffer %s (%i)", __FILE__, __LINE__);
        return NULL;
    }
}

static int32_t local_strcat (char *dst, char *src, int32_t max_c, int32_t *end)
{
    int32_t pos;
    int32_t len;

    if (end != NULL) {
        pos = *end;
    } else {
        pos = (int32_t)strlen (dst);
    }
    len = (int32_t)strlen (src);
    if ((pos + len + 1) < max_c) {
        // Fits to the end
        MEMCPY (dst + pos, src, (size_t)(len + 1));
        *end = pos + len;
        return pos + len;
    } else {
        return -(pos + len + 1);    // If not fits to the end return the needed length as negative value
    }

}

int32_t get_array_type_string_internal (int32_t typenr, int32_t elem_or_array_flag, char *ret_type_string, int32_t max_c, DEBUG_INFOS_DATA *pappldata)
{
    int32_t ret;
    DTYPE_LIST_ELEM *itemptr;
    int32_t pos = 0;

    ret_type_string[0] = 0;
    itemptr = search_dtype (typenr, pappldata, NULL);
    if (itemptr != NULL) {
        if (itemptr->what == MODIFIER_ELEM) {
            do {
                DTYPE_LIST_ELEM *itemptr_y;
                if (itemptr->name[0] != '\0') {   // Only modifier with names
                    if ((ret = local_strcat (ret_type_string, itemptr->name, max_c, &pos)) <= 0) return ret;
                    if ((ret = local_strcat (ret_type_string, " ", max_c, &pos)) <= 0) return ret;
                }
                itemptr_y = search_dtype (itemptr->pointsto, pappldata, NULL);
                if (itemptr_y == NULL) {
                    return 0;
                }
                itemptr = itemptr_y;
            } while (itemptr->what == MODIFIER_ELEM);
        }
        if (itemptr != NULL) {
            if (itemptr->what == ARRAY_ELEM) {
                DTYPE_LIST_ELEM *itemptr_points_to;
                char *name;
                if ((name = get_base_type (itemptr->pointsto)) == NULL) {
                    itemptr_points_to = search_dtype (itemptr->pointsto, pappldata, NULL);
                    if (itemptr_points_to != NULL) {
                        if (itemptr_points_to->what == ARRAY_ELEM) {  // multidimensional arrays
                            DTYPE_LIST_ELEM *itemptr_y;
                            do {
                                if ((name = get_base_type (itemptr_points_to->pointsto)) != NULL) {  // Array of base data types
                                    goto __BASE_TYPE;
                                }
                                itemptr_y = search_dtype (itemptr_points_to->pointsto, pappldata, NULL);
                                if (itemptr_y == NULL) {
                                    return 0;
                                }
                                itemptr_points_to = itemptr_y;
                            } while (itemptr_points_to->what == ARRAY_ELEM);
                        }
                        if (itemptr_points_to->what == MODIFIER_ELEM) {
                            do {
                                DTYPE_LIST_ELEM *itemptr_y;
                                if (itemptr_points_to->name[0] != '\0') {  // Only modifier with namen
                                    if ((ret = local_strcat (ret_type_string, itemptr_points_to->name, max_c, &pos)) <= 0) return ret;
                                    if ((ret = local_strcat (ret_type_string, " ", max_c, &pos)) <= 0) return ret;
                                }
                                if ((name = get_base_type (itemptr_points_to->pointsto)) != NULL) {
                                    goto __BASE_TYPE;
                                }
                                itemptr_y = search_dtype (itemptr_points_to->pointsto, pappldata, NULL);
                                if (itemptr_y == NULL) {
                                    return 0;
                                }
                                itemptr_points_to = itemptr_y;
                            } while (itemptr_points_to->what == MODIFIER_ELEM);
                        }
                        name = itemptr_points_to->name;
                        goto __BASE_TYPE;
                    }
                } else {
                    // Array of base data types without modifier (only VC)
                    if ((ret = local_strcat (ret_type_string, name, max_c, &pos)) <= 0) return ret;
                    if (!elem_or_array_flag) {
                        char sizestr[32];
                        sprintf (sizestr, "[%i]", (int32_t)itemptr->array_elements);
                        if ((ret = local_strcat (ret_type_string, sizestr, max_c, &pos)) <= 0) return ret;
                    }
                    return pos;
                }
__BASE_TYPE:
                // Prefix the data type before the Array
                if (name != NULL) {
                    if ((ret = local_strcat (ret_type_string, name, max_c, &pos)) <= 0) return ret;
                } else {
                    return 0;
                }
                // Array dimensions
                if (get_base_type (itemptr->pointsto) == NULL) {
                    itemptr_points_to = itemptr;
                        if (itemptr_points_to->what == ARRAY_ELEM) {  // multidimensional arrays
                            DTYPE_LIST_ELEM *itemptr_y;
                            int32_t dim_count = 0;
                            do {
                                if (!elem_or_array_flag || (dim_count > 0))  {
                                    char sizestr[32];
                                    sprintf (sizestr, "[%i]", (int32_t)itemptr_points_to->array_elements);
                                    if ((ret = local_strcat (ret_type_string, sizestr, max_c, &pos)) <= 0) return ret;
                                }
                                if ((name = get_base_type (itemptr_points_to->pointsto)) != NULL) {  // Array of base -data types
                                    break;
                                }
                                itemptr_y = search_dtype (itemptr_points_to->pointsto, pappldata, NULL);
                                if (itemptr_y == NULL) {
                                    return 0;
                                }
                                itemptr_points_to = itemptr_y;
                                dim_count++;
                            } while (itemptr_points_to->what == ARRAY_ELEM);
                        }
                    //}
                } else {
                    if (!elem_or_array_flag) {
                        char sizestr[32];
                        sprintf (sizestr, "[%i]", (int32_t)itemptr->array_elements);
                        if ((ret = local_strcat (ret_type_string, sizestr, max_c, &pos)) <= 0) return ret;
                    }
                }
            }
        }
    }
    return pos;
}


int32_t get_array_type_string (int32_t typenr, int32_t elem_or_array_flag, char *ret_type_string, int32_t max_c, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return 0;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return 0;
    }
    return get_array_type_string_internal (typenr, elem_or_array_flag, ret_type_string, max_c, AssociatedDebugInfos);
}


int32_t get_pointer_type_string_internal (int32_t typenr, int32_t points_to_or_pointer_flag, char *ret_type_string, int32_t max_c, DEBUG_INFOS_DATA *pappldata)
{
    int32_t ret;
    DTYPE_LIST_ELEM *itemptr;
    int32_t pos = 0;

    ret_type_string[0] = 0;
    itemptr = search_dtype (typenr, pappldata, NULL);
    if (itemptr != NULL) {
        if (itemptr->what == MODIFIER_ELEM) {
            do {
                DTYPE_LIST_ELEM *itemptr_y;
                if (itemptr->name[0] != '\0') {   // Only modifier with name
                    if ((ret = local_strcat (ret_type_string, itemptr->name, max_c, &pos)) <= 0) return ret;
                    if ((ret = local_strcat (ret_type_string, " ", max_c, &pos)) <= 0) return ret;
                }
                itemptr_y = search_dtype (itemptr->pointsto, pappldata, NULL);
                if (itemptr_y == NULL) {
                    return 0;
                }
                itemptr = itemptr_y;
            } while (itemptr->what == MODIFIER_ELEM);
        }
        if (itemptr != NULL) {
            if (itemptr->what == POINTER_ELEM) {
                int32_t ptr_count = 0;
                DTYPE_LIST_ELEM *itemptr_points_to = itemptr;
                char *name;
                do {
                    DTYPE_LIST_ELEM *itemptr_y;
                    if ((name = get_base_type (itemptr_points_to->pointsto)) != NULL) {
                        break;
                    }
                    itemptr_y = search_dtype (itemptr_points_to->pointsto, pappldata, NULL);
                    if (itemptr_y == NULL) {
                        return 0;
                    }
                    itemptr_points_to = itemptr_y;
                } while ((itemptr_points_to->what == MODIFIER_ELEM) ||
                         (itemptr_points_to->what == POINTER_ELEM));

                // Prefix with data type the pointer points to
                if (name != NULL) {
                    if ((ret = local_strcat (ret_type_string, name, max_c, &pos)) <= 0) return ret;
                } else {
                    if ((ret = local_strcat (ret_type_string, itemptr_points_to->name, max_c, &pos)) <= 0) return ret;
                }
                // Than the "*"
                itemptr_points_to = itemptr;
                do {
                    DTYPE_LIST_ELEM *itemptr_y;
                    if (!points_to_or_pointer_flag || (ptr_count > 0))  {
                        if (itemptr_points_to->what == MODIFIER_ELEM) {
                            if ((ret = local_strcat (ret_type_string, itemptr_points_to->name, max_c, &pos)) <= 0) return ret;
                            if ((ret = local_strcat (ret_type_string, " ", max_c, &pos)) <= 0) return ret;
                        } else if (itemptr_points_to->what == POINTER_ELEM) {
                            if ((ret = local_strcat (ret_type_string, "*", max_c, &pos)) <= 0) return ret;
                        }
                    }
                    if ((name = get_base_type (itemptr_points_to->pointsto)) != NULL) {
                        break;
                    }
                    itemptr_y = search_dtype (itemptr_points_to->pointsto, pappldata, NULL);
                    if (itemptr_y == NULL) {
                        return 0;
                    }
                    itemptr_points_to = itemptr_y;
                    if (itemptr_points_to->what == POINTER_ELEM) {
                        ptr_count++;
                    }
                } while ((itemptr_points_to->what == MODIFIER_ELEM) ||
                         (itemptr_points_to->what == POINTER_ELEM));
            }
        }
    }
    return pos;
}

int32_t get_pointer_type_string (int32_t typenr, int32_t points_to_or_pointer_flag, char *ret_type_string, int32_t max_c, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return 0;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return 0;
    }
    return get_pointer_type_string_internal (typenr, points_to_or_pointer_flag, ret_type_string, max_c, AssociatedDebugInfos);
}


int32_t get_points_to_internal (int32_t *points_to_typenr, int32_t *points_to_what,
                            int32_t parent_typenr, DEBUG_INFOS_DATA  *pappldata)
{
    int32_t ret = 0;
    char *name;
    DTYPE_LIST_ELEM *itemptr;

    BEGIN_RUNTIME_MEASSUREMENT ("get_points_to")

    itemptr = search_dtype (parent_typenr, pappldata, NULL);
    if (itemptr != NULL) {
        if (itemptr->what == MODIFIER_ELEM) {
            do {
                DTYPE_LIST_ELEM *itemptr_y;

                if ((name = get_base_type (itemptr->pointsto)) != NULL) {
                    if (points_to_typenr != NULL) *points_to_typenr = itemptr->pointsto;
                    if (points_to_what != NULL) *points_to_what = BASE_DATA_TYPE;
                    goto __OUT;
                }

                itemptr_y = search_dtype (itemptr->pointsto, pappldata, NULL);
                if (itemptr_y == NULL) {
                    ret = 0;
                    goto __OUT;
                }
                itemptr = itemptr_y;
            } while (itemptr->what == MODIFIER_ELEM);
        }
        if (itemptr != NULL) {
            if ((itemptr->what == POINTER_ELEM) ||
                (itemptr->what == ARRAY_ELEM)) {
                DTYPE_LIST_ELEM *itemptr_points_to;

                // Pointer or array of Basis_Datentypen without Modifier (only VC)
                if ((name = get_base_type (itemptr->pointsto)) != NULL) {
                    if (points_to_typenr != NULL) *points_to_typenr = itemptr->pointsto;
                    if (points_to_what != NULL) *points_to_what = BASE_DATA_TYPE;
                    ret = 1;
                    goto __OUT;
                }

                itemptr_points_to = search_dtype (itemptr->pointsto, pappldata, NULL);
                if (itemptr_points_to != NULL) {
                    // gets_points() should not give back a modifyer, this is not accepted by getlabelbyaddress()
                    if ((itemptr_points_to->what == MODIFIER_ELEM) ||
                        (itemptr_points_to->what == TYPEDEF_ELEM)) {
                        do {
                            DTYPE_LIST_ELEM *itemptr_y;

                            if ((name = get_base_type (itemptr_points_to->pointsto)) != NULL) {
                                if (points_to_typenr != NULL) *points_to_typenr = itemptr->pointsto;
                                if (points_to_what != NULL) *points_to_what = BASE_DATA_TYPE;
                                ret = 1;
                                goto __OUT;
                            }

                            itemptr_y = search_dtype (itemptr_points_to->pointsto, pappldata, NULL);
                            if (itemptr_y == NULL) {
                                ret = 0;
                                goto __OUT;
                            }

                            itemptr_points_to = itemptr_y;
                        } while ((itemptr_points_to->what == MODIFIER_ELEM) || (itemptr_points_to->what == TYPEDEF_ELEM));
                    }
                    if (points_to_typenr != NULL) *points_to_typenr = itemptr_points_to->typenr;
                    if (points_to_what != NULL) *points_to_what = itemptr_points_to->what;
                } else {
                    ret = 0;
                    goto __OUT;
                }
                ret = 1;
                goto __OUT;
            } else {
                ret = 0;
                goto __OUT;
            }
        }
    }
    ret = 0;
__OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;

}

int32_t get_points_to (int32_t *points_to_typenr, int32_t *points_to_what,
                   int32_t parent_typenr, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return 0;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return 0;
    }
    return get_points_to_internal (points_to_typenr, points_to_what, parent_typenr, AssociatedDebugInfos);
}


int32_t get_modify_what_internal (int32_t *modify_typenr, int32_t *modify_what,
                              int32_t parent_typenr, DEBUG_INFOS_DATA *pappldata)
{
    int32_t ret = 0;
    char *name;
    DTYPE_LIST_ELEM *itemptr, *itemptr_y;

    BEGIN_RUNTIME_MEASSUREMENT ("get_modify_what")

    itemptr = search_dtype (parent_typenr, pappldata, NULL);
    if (itemptr != NULL) {
        if ((itemptr->what == MODIFIER_ELEM) || (itemptr->what == TYPEDEF_ELEM)) {
            do {
                if ((name = get_base_type (itemptr->pointsto)) != NULL) {
                    if (modify_typenr) *modify_typenr = itemptr->pointsto;
                    if (modify_what) *modify_what = BASE_DATA_TYPE;
                    ret = 1;
                    goto __OUT;
                }
                itemptr_y = search_dtype (itemptr->pointsto, pappldata, NULL);
                if (itemptr_y == NULL) {
                    ret = 0;
                    goto __OUT;
                }
                itemptr = itemptr_y;
            } while ((itemptr->what == MODIFIER_ELEM) || (itemptr->what == TYPEDEF_ELEM));
            if (modify_typenr) *modify_typenr = itemptr->typenr;
            if (modify_what) *modify_what = itemptr->what;
            ret = 1;
            goto __OUT;
        } else {
            ret = 0;   // It is not a modifier (this is not an error)
            goto __OUT;
        }

    }
    ret = 0;  // It is not a valid data type
__OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;

}

int32_t get_modify_what (int32_t *modify_typenr, int32_t *modify_what,
                     int32_t parent_typenr, DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata)
{
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return 0;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return 0;
    }
    return get_modify_what_internal (modify_typenr, modify_what, parent_typenr, AssociatedDebugInfos);
}


char *get_next_struct_entry_internal (int32_t *typenr, uint32_t *address_offset, int32_t *pbclass,
                                      int32_t parent_typenr, int32_t flag, int32_t *pos, DEBUG_INFOS_DATA *pappldata)
{
    int32_t y, idx = -1;
    char *name;
    DTYPE_LIST_ELEM *itemptr;
    FIELD_LIST_ELEM *itemptr_y;
    char *ret;

    BEGIN_RUNTIME_MEASSUREMENT ("get_next_struct_entry")
    if ((pappldata->dtype_list == NULL) ||
        (pappldata->dtype_list_entrys == 0)) {
        ret = NULL;
        goto __OUT;
    }
    if (flag) {
        *pos = 0;

        itemptr = search_dtype (parent_typenr, pappldata, NULL);
        if (itemptr != NULL) {
            if ((itemptr->what == MODIFIER_ELEM) || (itemptr->what == TYPEDEF_ELEM)) {
                return get_next_struct_entry_internal (typenr, address_offset, pbclass,
                                                       itemptr->pointsto, flag, pos, pappldata);
            }
            if (itemptr->what != STRUCT_ELEM) {
                ret = NULL;
                goto __OUT;
            }
            if (itemptr->typenr_fields_idx >= 0) {  // The typenr was searched before
                y = itemptr->typenr_fields_idx;
                itemptr_y = pappldata->field_list[y];
                goto __FOUND;
            }

            itemptr_y = search_field (itemptr->typenr_fields, pappldata, &y);
            if (itemptr_y != NULL) {
                if (itemptr->typenr_fields == itemptr_y->fieldnr) {
                    itemptr->typenr_fields_idx = y;
                  __FOUND:
                    idx = itemptr_y->start_fieldidx;
                }
            }
        }
    } else idx = pappldata->fieldidx_list[*pos].nextidx;
    if (idx == -1) {
        ret = NULL;   // End of the list
        goto __OUT;
    }
    name = pappldata->fieldidx_list[idx].name;
    *address_offset = (uint32_t)pappldata->fieldidx_list[idx].offset;
    *pos = idx;
    *typenr = pappldata->fieldidx_list[idx].typenr;
    // Structure member with namen "public" are  base classes
    // this will be handle like member objects with the class name as member name
    if (!strcmp ("public", name)) {
        char *typestr;
        if ((typestr = get_struct_entry_typestr_internal (NULL, idx, NULL, pappldata)) != NULL) {
            *pbclass = 1;
            ret = typestr;
            goto __OUT;
        }
    }
    *pbclass = 0;
    ret = name;
  __OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}

char *get_next_struct_entry (int32_t *typenr, uint32_t *address_offset, int32_t *pbclass,
                             int32_t parent_typenr, int32_t flag, int32_t *pos, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return NULL;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return NULL;
    }
    return get_next_struct_entry_internal (typenr, address_offset, pbclass,
                                           parent_typenr, flag, pos, AssociatedDebugInfos);
}


int32_t get_type_by_name (int32_t *ptypenr, char *label, int32_t *what, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    int32_t x;
    char *name;
    int32_t ret;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        if (ptypenr != NULL) *ptypenr = -1;
        return 0;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        if (ptypenr != NULL) *ptypenr = -1;
        return 0;
    }

    BEGIN_RUNTIME_MEASSUREMENT ("get_typestr")

    x = SearchLabelByNameInternal (label, AssociatedDebugInfos);
    if (x >= 0) {
        if (!strcmp (AssociatedDebugInfos->label_list[x].name, label)) {
            if ((name = get_base_type (AssociatedDebugInfos->label_list[x].typenr)) != NULL) {
                if (what != NULL) *what = 1;    // Base data typ
                if (ptypenr != NULL) *ptypenr = AssociatedDebugInfos->label_list[x].typenr;
                ret = 1;
                goto __OUT;
            } else {
                int32_t typenr_help = AssociatedDebugInfos->label_list[x].typenr;
                ret = (get_typestr_by_typenr_internal (&typenr_help, what, AssociatedDebugInfos) != NULL);
                if (ptypenr != NULL) *ptypenr = typenr_help;
                goto __OUT;
            }
        }
    }
    if (ptypenr != NULL) *ptypenr = -1;
    ret = 0;
  __OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}


// es ist sinnvoll ueber die TypeId zu suchen weil das Label nicht immer eindeutig ist
char *get_typestr_by_typenr_internal (int32_t *typenr,  // in out if typenr is a modifier the value will be set to the data type after the
                                                        // otherwise it will be unchanged
                                      int32_t *what, DEBUG_INFOS_DATA *pappldata)
{
    DTYPE_LIST_ELEM *itemptr;
    char *name;
    char *ret;

    BEGIN_RUNTIME_MEASSUREMENT ("get_typestr_by_typenr")

    if ((name = get_base_type (*typenr)) != NULL) {
        if (what != NULL) *what = 1;    // Base data type
        ret = name;
        goto __OUT;
    } else {
        int32_t pos = 0;
        ret = get_return_buffer_func_thread ((void*)get_typestr_by_typenr, DEFAULT_RET_BUFFER_SIZE);
        ret[0] = '\0';
        itemptr = search_dtype (*typenr, pappldata, NULL);
        if (itemptr != NULL) {
            if ((itemptr->what == MODIFIER_ELEM) || (itemptr->what == TYPEDEF_ELEM)) {
                DTYPE_LIST_ELEM *itemptr_y;

                do {
                    local_strcat (ret, itemptr->name, DEFAULT_RET_BUFFER_SIZE, &pos);
                    local_strcat (ret, " ", DEFAULT_RET_BUFFER_SIZE, &pos);

                    if ((name = get_base_type (itemptr->pointsto)) != NULL) {
                        local_strcat (ret, name, DEFAULT_RET_BUFFER_SIZE, &pos);
                        if (what != NULL) *what = BASE_DATA_TYPE;
                        goto __OUT;
                    }

                    itemptr_y = search_dtype (itemptr->pointsto, pappldata, NULL);
                    if (itemptr_y == NULL) {
                        ret = NULL;
                        goto __OUT;
                    }

                    itemptr = itemptr_y;
                } while ((itemptr->what == MODIFIER_ELEM) || (itemptr->what == TYPEDEF_ELEM));
            }
            if (itemptr != NULL) {
                if (itemptr->what == POINTER_ELEM) {
                    pos += get_pointer_type_string_internal (itemptr->typenr, 0, ret+pos, DEFAULT_RET_BUFFER_SIZE-pos, pappldata);
                    if (what != NULL) *what = itemptr->what;
                    goto __OUT;
                } else if ((itemptr->what == MODIFIER_ELEM) ||(itemptr->what == TYPEDEF_ELEM)) {
                    ret = NULL;
                    goto __OUT;
                } else if (itemptr->what == ARRAY_ELEM) {
                    pos += get_array_type_string_internal (itemptr->typenr, 0, ret+pos, DEFAULT_RET_BUFFER_SIZE-pos, pappldata);
                    if (what != NULL) *what = itemptr->what;
                    goto __OUT;
                } else if (itemptr->what == STRUCT_ELEM) {
                    if (what != NULL) *what = itemptr->what;
                    local_strcat (ret, itemptr->name, DEFAULT_RET_BUFFER_SIZE, &pos);
                    goto __OUT;
                } else if (itemptr->what == BASE_DATA_TYPE) {
                    if (what != NULL) *what = itemptr->what;
                    name = get_base_type (itemptr->typenr);
                    if (name != NULL) {
                        local_strcat (ret, get_base_type (itemptr->typenr), DEFAULT_RET_BUFFER_SIZE, &pos);
                    }
                    goto __OUT;
                }
            }
        }
    }
    ret = NULL;
  __OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}

char *get_typestr_by_typenr (int32_t *typenr, // in out if typenr is a modifier the value will be set to the data type after the
                                              // otherwise it will be unchanged
                             int32_t *what, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return NULL;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return NULL;
    }
    return get_typestr_by_typenr_internal (typenr, what, AssociatedDebugInfos);
}


// Give back the address of an label
uint64_t get_label_adress (char *label, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    int32_t x;
    uint64_t ret;
    uint64_t BaseAddress;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return 0;
    }
    BaseAddress = (uint64_t)AssociatedProcess->BaseAddress;
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return 0;
    }

    BEGIN_RUNTIME_MEASSUREMENT ("get_label_adress")            
    x = SearchLabelByNameInternal (label, AssociatedDebugInfos);
    if (x >= 0) {
        if (!strcmp (AssociatedDebugInfos->label_list[x].name, label)) {
            switch (AssociatedDebugInfos->AbsoluteOrRelativeAddress) {
            default:
            case DEBUGINFO_ADDRESS_ABSOLUTE:
                ret = AssociatedDebugInfos->label_list[x].adr;
                break;
            case DEBUGINFO_ADDRESS_RELATIVE:
                ret = BaseAddress + AssociatedDebugInfos->label_list[x].adr;
                break;
            }
            goto __OUT;
        }
    }
    ret = 0UL;
  __OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}

char *get_struct_entry_typestr_internal(int32_t *ptypenr, int32_t pos, int32_t *what, DEBUG_INFOS_DATA  *pappldata)
{
    char *ret;
    int32_t typenr_help;

    BEGIN_RUNTIME_MEASSUREMENT ("get_struct_entry_typestr")
    if (pos >= (int32_t)pappldata->fieldidx_list_entrys) {
        ThrowError (1, "sanity check %s %li", __FILE__, (int32_t)__LINE__);
        ret = NULL;
        goto __OUT;
    }

    typenr_help = pappldata->fieldidx_list[pos].typenr;
    ret = get_typestr_by_typenr_internal (&typenr_help, what, pappldata);
    if (ret != NULL) {
        if (ptypenr != NULL) {
            *ptypenr = typenr_help;
        }
    }
__OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}

char *get_struct_entry_typestr(int32_t *ptypenr, int32_t pos, int32_t *what, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return NULL;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return NULL;
    }
    return get_struct_entry_typestr_internal(ptypenr, pos, what, AssociatedDebugInfos);
}

int32_t get_array_elem_type_and_size (int32_t *arrayelem_typenr, int32_t *arrayelem_of_what,
                                  int32_t *array_size, int32_t *array_elements,
                                  int32_t array_typenr, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    int32_t array_of_what;
    DTYPE_LIST_ELEM *itemptr, *itemptr_y;
    int32_t ret = 0;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        goto __ERROR;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        goto __ERROR;
    }

    BEGIN_RUNTIME_MEASSUREMENT ("get_array_name_size")
    itemptr = search_dtype (array_typenr, AssociatedDebugInfos, NULL);
    if (itemptr != NULL) {
        if ((itemptr->what == MODIFIER_ELEM) || (itemptr->what == TYPEDEF_ELEM)) {
            do {
                itemptr_y = search_dtype (itemptr->pointsto, AssociatedDebugInfos, NULL);
                if (itemptr_y != NULL) {
                    if (itemptr_y->typenr == itemptr->pointsto) {
                        itemptr = itemptr_y;
                    }
                }
            } while ((itemptr->what == MODIFIER_ELEM) || (itemptr->what == TYPEDEF_ELEM));
        }
        if (itemptr->what != ARRAY_ELEM) {
            ThrowError (1, "sanity check %s %li", __FILE__, (int32_t)__LINE__);
        }
        if (arrayelem_typenr != NULL) *arrayelem_typenr = itemptr->array_elem_typenr;
        if (array_size != NULL) *array_size = itemptr->array_size;
        if (array_elements != NULL) *array_elements = itemptr->array_elements;
        ret = get_points_to_internal (NULL, &array_of_what, itemptr->typenr, AssociatedDebugInfos);
        if (arrayelem_of_what != NULL) *arrayelem_of_what = array_of_what;

        goto __OUT;
    }
 __ERROR:
    if (arrayelem_typenr != NULL) *arrayelem_typenr = -1;
    if (arrayelem_of_what != NULL) *arrayelem_of_what = 0;
    if (array_size != NULL) *array_size = 0;
    if (array_elements != NULL) *array_elements = 0;
    ret = 0;
  __OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}


int32_t get_array_element_count (int32_t array_typenr, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    DTYPE_LIST_ELEM *itemptr;
    int32_t ret;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return 0;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return 0;
    }

    BEGIN_RUNTIME_MEASSUREMENT ("get_array_element_count")

    itemptr = search_dtype (array_typenr, AssociatedDebugInfos, NULL); //&y);
    if (itemptr != NULL) {
        if (itemptr->typenr == array_typenr) {
            // for example: volatile int32_t array[10];
            if (itemptr->what == MODIFIER_ELEM) {
                do {
                    DTYPE_LIST_ELEM *itemptr_y;
                    itemptr_y = search_dtype (itemptr->pointsto, AssociatedDebugInfos, NULL);
                    if (itemptr_y == NULL) {
                        return 0;
                    }
                    itemptr = itemptr_y;
                } while (itemptr->what == MODIFIER_ELEM);
            }
            if ((itemptr->what != ARRAY_ELEM) && (itemptr->what != TYPEDEF_ELEM)) {
                ThrowError (1, "sanity check %s %li", __FILE__, (int32_t)__LINE__);
            }
            ret = itemptr->array_elements;
            goto __OUT;
        }
    }
    ret = 0;
  __OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}

int32_t get_structure_element_count (int32_t structure_typenr, DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    int32_t y;
    DTYPE_LIST_ELEM *itemptr;
    FIELD_LIST_ELEM *itemptr_f;
    int32_t ret = 0;
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
        return 0;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return 0;
    }

    BEGIN_RUNTIME_MEASSUREMENT ("get_structure_element_count")

    itemptr = search_dtype (structure_typenr, AssociatedDebugInfos, NULL);
    if (itemptr != NULL) {
        if (itemptr->typenr == structure_typenr) {
            // for example volatile int32_t array[10];
            if ((itemptr->what == MODIFIER_ELEM) || (itemptr->what == TYPEDEF_ELEM)) {
                do {
                    DTYPE_LIST_ELEM *itemptr_y;
                    itemptr_y = search_dtype (itemptr->pointsto, AssociatedDebugInfos, NULL);
                    if (itemptr_y == NULL) {
                        return 0;
                    }
                    itemptr = itemptr_y;
                } while ((itemptr->what == MODIFIER_ELEM) || (itemptr->what == TYPEDEF_ELEM));
            }
            if (itemptr->what != STRUCT_ELEM) {
                ThrowError (1, "sanity check %s %li", __FILE__, (int32_t)__LINE__);
                goto __OUT;
            }

            if (itemptr->typenr_fields_idx >= 0) {  // Typenr was searched before
                y = itemptr->typenr_fields_idx;
                itemptr_f = AssociatedDebugInfos->field_list[y];
                goto __FOUND;
            }
            // Fieldlist suchen
            itemptr_f = search_field (itemptr->typenr_fields, AssociatedDebugInfos, &y);
            if (itemptr_f != NULL) {
                if (itemptr->typenr_fields == itemptr_f->fieldnr) {
                    itemptr->typenr_fields_idx = y;
                  __FOUND:
                    ret = itemptr_f->fields;
                }
            }
            goto __OUT;
        }
    }
    ret = 0;
  __OUT:
    END_RUNTIME_MEASSUREMENT
    return ret;
}


static int32_t calc_all_array_size_recursive (int32_t parent_typenr, DEBUG_INFOS_DATA *pappldata, int NoErrMsgFlag)
{
    DTYPE_LIST_ELEM *itemptr;
    int32_t size = 0;

    if (parent_typenr == 33) return 0;  // datatype "void"

    size = get_base_type_size (parent_typenr);
    if (size) return size;

   itemptr = search_dtype (parent_typenr, pappldata, NULL);
    if (itemptr != NULL) {
        switch (itemptr->what) {
        case STRUCT_ELEM:
            return itemptr->array_size;
        case MODIFIER_ELEM:
        case TYPEDEF_ELEM:
            return calc_all_array_size_recursive (itemptr->pointsto, pappldata, NoErrMsgFlag);
        case POINTER_ELEM:
            return pappldata->PointerSize;
        case ARRAY_ELEM:
            size = itemptr->array_elements * get_base_type_size (itemptr->array_elem_typenr);
            if (size) {
                return size;
            } else {
                return calc_all_array_size_recursive (itemptr->pointsto, pappldata, NoErrMsgFlag) * itemptr->array_elements;
            }
        default:
            printf ("debug");
            break;
        }
    }
    if (!NoErrMsgFlag) ThrowError (1, "sanity check %s %li parent_typenr = 0x%X", __FILE__, (int32_t)__LINE__, parent_typenr);
    return 1;
}



// This is only necessary for DWARF
int32_t calc_all_array_sizes (DEBUG_INFOS_DATA *pappldata, int NoErrMsgFlag)
{
#if 1
    uint32_t x;
    int32_t item_size;

    BEGIN_RUNTIME_MEASSUREMENT ("calc_all_array_sizes")

    for (x = 0; x < pappldata->dtype_list_entrys; x++) {
        if ((pappldata->dtype_list[x]->what == ARRAY_ELEM) &&
            ((pappldata->dtype_list[x]->array_size == 0) ||
             (pappldata->dtype_list[x]->array_elements == 0))) {
            item_size = get_base_type_size (pappldata->dtype_list[x]->array_elem_typenr);
            if (item_size) {
                if (pappldata->dtype_list[x]->array_size == 0) pappldata->dtype_list[x]->array_size = item_size * pappldata->dtype_list[x]->array_elements;
                if (pappldata->dtype_list[x]->array_elements == 0) pappldata->dtype_list[x]->array_elements = pappldata->dtype_list[x]->array_size / item_size;
            } else {
                if (pappldata->dtype_list[x]->array_size == 0)
                {
                    if (pappldata->dtype_list[x]->array_elements != 0)
                    {
                        pappldata->dtype_list[x]->array_size = pappldata->dtype_list[x]->array_elements *
                            calc_all_array_size_recursive (pappldata->dtype_list[x]->array_elem_typenr, pappldata, NoErrMsgFlag);
                    } else {
                        pappldata->dtype_list[x]->array_size = calc_all_array_size_recursive (pappldata->dtype_list[x]->array_elem_typenr, pappldata, NoErrMsgFlag);
                    }
                }
                if (pappldata->dtype_list[x]->array_elements == 0) pappldata->dtype_list[x]->array_elements = pappldata->dtype_list[x]->array_size / calc_all_array_size_recursive (pappldata->dtype_list[x]->array_elem_typenr, pappldata, NoErrMsgFlag);
            }
        }
    }
    // List of additional automatic generated data type ids (0x7FFFFFFF downwards to 0x40000000)
    for (x = 0; x < pappldata->h_dtype_list_entrys; x++) {
        if ((pappldata->h_dtype_list[x]->what == ARRAY_ELEM) &&
            ((pappldata->h_dtype_list[x]->array_size == 0) ||
             (pappldata->h_dtype_list[x]->array_elements == 0))) {
            item_size = get_base_type_size (pappldata->h_dtype_list[x]->array_elem_typenr);
            if (item_size) {
                if (pappldata->h_dtype_list[x]->array_size == 0) pappldata->h_dtype_list[x]->array_size = item_size * pappldata->h_dtype_list[x]->array_elements;
                if (pappldata->h_dtype_list[x]->array_elements == 0) pappldata->h_dtype_list[x]->array_elements = pappldata->h_dtype_list[x]->array_size / item_size;
            } else {
                if (pappldata->h_dtype_list[x]->array_size == 0)
                {
                    if (pappldata->h_dtype_list[x]->array_elements != 0)
                    {
                        pappldata->h_dtype_list[x]->array_size = pappldata->h_dtype_list[x]->array_elements *
                            calc_all_array_size_recursive (pappldata->h_dtype_list[x]->array_elem_typenr, pappldata, NoErrMsgFlag);
                    } else {
                        pappldata->h_dtype_list[x]->array_size = calc_all_array_size_recursive (pappldata->h_dtype_list[x]->array_elem_typenr, pappldata, NoErrMsgFlag);
                    }
                }
                if (pappldata->h_dtype_list[x]->array_elements == 0) pappldata->h_dtype_list[x]->array_elements = pappldata->h_dtype_list[x]->array_size / calc_all_array_size_recursive (pappldata->h_dtype_list[x]->array_elem_typenr, pappldata, NoErrMsgFlag);
            }
        }
    }

#endif
    END_RUNTIME_MEASSUREMENT
    return 0;
}



PROCESS_APPL_DATA *ConnectToProcessDebugInfos (int32_t par_UniqueId,
                                               const char *par_ProcessName,
                                               int32_t *ret_Pid,
                                               int32_t (*par_LoadUnloadCallBackFunc)(void *par_User, int32_t par_Flags, int32_t par_Action),
                                               int32_t (*par_LoadUnloadCallBackFuncSchedThread)(int32_t par_UniqueId, void *par_User, int32_t par_Flags, int32_t par_Action),
                                               void *par_LoadUnloadCallBackFuncParUsers,
                                               int32_t par_NoErrMsgFlag)
{
    UNUSED(par_NoErrMsgFlag);
    int32_t Pid = -1;
    char ShortProcessName[MAX_PATH];
    int32_t x, i;
    PROCESS_APPL_DATA *Ret = NULL;
    int32_t Found = 0;

    TruncatePathFromProcessName (ShortProcessName, par_ProcessName);

    ENTER_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);

    // schaue ob Connection schon vorhanden
    for (x = 0; x < DEBUG_INFOS_MAX_CONNECTIONS; x++) {
        if (DebugInfosAssociatedConnections[x].UsedFlag) {
            if (DebugInfosAssociatedConnections[x].UsedByUniqueId == par_UniqueId) {
                ThrowError (1, "Unique id already in there! DebugInfosAssociatedConnections[%i].UsedByUniqueId == par_UniqueId == %i", DebugInfosAssociatedConnections[x].UsedByUniqueId, par_UniqueId);
                LEAVE_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
                return &DebugInfosAssociatedConnections[x];
            }
        }
    }
    for (x = 0; x < DEBUG_INFOS_MAX_CONNECTIONS; x++) {
        if (!DebugInfosAssociatedConnections[x].UsedFlag) {
            // New connection
            memset(&(DebugInfosAssociatedConnections[x]), 0, sizeof (DebugInfosAssociatedConnections[x]));
            DebugInfosAssociatedConnections[x].UsedFlag = 1;
            DebugInfosAssociatedConnections[x].UsedByUniqueId = par_UniqueId;
            DebugInfosAssociatedConnections[x].LoadUnloadCallBackFuncs = par_LoadUnloadCallBackFunc;
            DebugInfosAssociatedConnections[x].LoadUnloadCallBackFuncsSchedThread = par_LoadUnloadCallBackFuncSchedThread;
            DebugInfosAssociatedConnections[x].LoadUnloadCallBackFuncParUsers = par_LoadUnloadCallBackFuncParUsers;
            // Is this process already running?
            for (i = 0; i < DEBUG_INFOS_MAX_PROCESSES; i++) {
                if (DebugInfosAssociatedProcesses[i].UsedFlag) {
                    if (!Compare2ProcessNames(DebugInfosAssociatedProcesses[i].ProcessName, ShortProcessName)) {
                        if (DebugInfosAssociatedProcesses[i].Pid > 0) {
                            // Try to load the debug infos if the proess is running
                            int32_t LoadFlag;
                            // Are thee debug infos already loaded?
                            if ((DebugInfosAssociatedProcesses[i].AssociatedDebugInfos == NULL) ||
                                (DebugInfosAssociatedProcesses[i].AssociatedDebugInfos == &DebugInfosEmptyData)) {
                                DebugInfosAssociatedProcesses[i].AssociatedDebugInfos = check_and_load_debug_infos (DebugInfosAssociatedProcesses[i].ExeOrDllName, &LoadFlag, DebugInfosAssociatedProcesses[i].IsRealtimeProcess, DebugInfosAssociatedProcesses[i].MachineType);
                                if (DebugInfosAssociatedProcesses[i].AssociatedDebugInfos != NULL) {
                                    DebugInfosAssociatedProcesses[i].AssociatedDebugInfos->AttachCounter++;
                                }
                                if (LoadFlag) {
                                    Pid = DebugInfosAssociatedProcesses[i].Pid;
                                    DebugInfosAssociatedProcesses[i].AssociatedDebugInfos->AttachCounter++;
                                }
                            }
                        }
                        DebugInfosAssociatedConnections[x].AssociatedProcess = &DebugInfosAssociatedProcesses[i];
                        Pid = DebugInfosAssociatedProcesses[i].Pid;
                        DebugInfosAssociatedProcesses[i].AttachCounter++;
                        Found = 1;
                        break;
                    }
                }
            }
            if (!Found) {
                // It is a new process
                for (i = 0; i < DEBUG_INFOS_MAX_PROCESSES; i++) {
                    if (!DebugInfosAssociatedProcesses[i].UsedFlag) {
                        int32_t Pid;
                        memset(&(DebugInfosAssociatedProcesses[i]), 0, sizeof (DebugInfosAssociatedProcesses[i]));
                        DebugInfosAssociatedProcesses[i].UsedFlag = 1;
                        DebugInfosAssociatedProcesses[i].ProcessName = StringMalloc (ShortProcessName);
                        DebugInfosAssociatedProcesses[i].ExeOrDllName = NULL;
                        Pid = DebugInfosAssociatedProcesses[i].Pid = -1;
                        DebugInfosAssociatedProcesses[i].AttachCounter = 0;
                        DebugInfosAssociatedProcesses[i].AssociatedDebugInfos = &DebugInfosEmptyData;
                        DebugInfosAssociatedProcesses[i].AttachCounter++;
                        DebugInfosAssociatedConnections[x].AssociatedProcess = &DebugInfosAssociatedProcesses[i];
                        Found = 1;
                        break;
                    }
                }
                if (!Found) {
                    ThrowError (1, "Out of process (max = %i)", DEBUG_INFOS_MAX_PROCESSES);
                    Ret = NULL;
                }
            }
            Ret = &DebugInfosAssociatedConnections[x];
            break;
        }
    }
    if (x == DEBUG_INFOS_MAX_CONNECTIONS) {
        ThrowError (1, "Out of connections (max = %i)", DEBUG_INFOS_MAX_CONNECTIONS);
        Ret = NULL;
    }
    LEAVE_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
    if (ret_Pid != NULL) *ret_Pid = Pid;
    return Ret;
}


int32_t RemoveConnectFromProcessDebugInfos (int32_t par_UniqueId)
{
    int32_t x;
    int32_t y;

    ENTER_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
    for (x = 0; x < DEBUG_INFOS_MAX_CONNECTIONS; x++) {
        if (DebugInfosAssociatedConnections[x].UsedFlag) {
            if (DebugInfosAssociatedConnections[x].UsedByUniqueId == par_UniqueId) {
                for (y = 0; y < DEBUG_INFOS_MAX_PROCESSES; y++) {
                    if (DebugInfosAssociatedProcesses[y].UsedFlag) {
                        if (DebugInfosAssociatedConnections[x].AssociatedProcess == &DebugInfosAssociatedProcesses[y]) {
                            if (DebugInfosAssociatedProcesses[y].AttachCounter <= 0) {
                                ThrowError (1, "Wrong attach counter %s(%i)", __FILE__, __LINE__);
                            } else {
                                DebugInfosAssociatedProcesses[y].AttachCounter--;
                            }
                            if (DebugInfosAssociatedProcesses[y].AttachCounter == 0) {  // No access to this debug infos
                                if (DebugInfosAssociatedProcesses[y].Pid <= 0) {        // and the process in not active
                                    // than delete it
                                    if (DebugInfosAssociatedProcesses[y].ProcessName != NULL) {
                                        my_free (DebugInfosAssociatedProcesses[y].ProcessName);
                                        DebugInfosAssociatedProcesses[y].ProcessName = NULL;
                                    }
                                    if (DebugInfosAssociatedProcesses[y].ExeOrDllName != NULL) {
                                        my_free (DebugInfosAssociatedProcesses[y].ExeOrDllName);
                                        DebugInfosAssociatedProcesses[y].ExeOrDllName = NULL;
                                    }
                                    DebugInfosAssociatedProcesses[y].UsedFlag = 0;
                                }
                            }
                            break;
                        }
                    }
                }
                if (y == DEBUG_INFOS_MAX_PROCESSES) {
                    LEAVE_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
                    ThrowError (1, "Associated process doesn't exist  %s(%i)", __FILE__, __LINE__);
                    return -1;
                }
                DebugInfosAssociatedConnections[x].UsedFlag = 0;
                LEAVE_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
                return 0;
            }
        }
    }
    LEAVE_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
    ThrowError (1, "remove unknown connection from debug infos");
    return -1;
}


int32_t GetConnectionToProcessDebugInfos (int32_t par_UniqueId,
                                          int32_t (**ret_LoadUnloadCallBackFunc)(void *par_User, int32_t par_Flags, int32_t par_Action),
                                          void** ret_User,
                                          int32_t par_NoErrMsgFlag)
{
    int32_t x;

    ENTER_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
    for (x = 0; x < DEBUG_INFOS_MAX_CONNECTIONS; x++) {
        if (DebugInfosAssociatedConnections[x].UsedByUniqueId == par_UniqueId) {
            *ret_LoadUnloadCallBackFunc = DebugInfosAssociatedConnections[x].LoadUnloadCallBackFuncs;
            *ret_User = DebugInfosAssociatedConnections[x].LoadUnloadCallBackFuncParUsers;
            LEAVE_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
            return 0;
        }
    }
    LEAVE_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
    // Can this happen if a CalibrationTreeWidget will be closed in parallel a process will be statred or terminate?
    if (!par_NoErrMsgFlag) ThrowError (1, "connection %i to debug infos are invalid", par_UniqueId);
    return -1;
}

int32_t SortDtypeList (DEBUG_INFOS_DATA *pappldata)
{
    BEGIN_RUNTIME_MEASSUREMENT ("SortDtypeList")
    qsort (pappldata->dtype_list, pappldata->dtype_list_entrys,
           sizeof (DTYPE_LIST_ELEM*), (CMP_FUNC_PTR)comp_typenr_func);
    END_RUNTIME_MEASSUREMENT
    return 0;
}

void SetDebugInfosLoadedFlag (DEBUG_INFOS_DATA *pappldata, int32_t par_DebugInfoType, int32_t par_AbsoluteOrRelativeAddress)
{
    pappldata->DebugInfoType = par_DebugInfoType;
    pappldata->associated_exe_file = 1;
    pappldata->AbsoluteOrRelativeAddress = par_AbsoluteOrRelativeAddress;
}


int32_t get_number_of_labels(DEBUG_INFOS_ASSOCIATED_CONNECTION *par_pappldata)
{
    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
    DEBUG_INFOS_DATA *AssociatedDebugInfos;

    AssociatedProcess = par_pappldata->AssociatedProcess;
    if (AssociatedProcess == NULL) {
       // (AssociatedProcess == &DebugInfosAssociatedEmptyProcess)) {
        return 0;
    }
    AssociatedDebugInfos = AssociatedProcess->AssociatedDebugInfos;
    if (AssociatedDebugInfos == NULL) {
        return 0;
    }
    return (int32_t)AssociatedDebugInfos->label_list_entrys;
}

// Renaming list
static int32_t ReadRenamingListFromIni(DEBUG_INFOS_DATA *pappldata)
{
    int32_t i;
    char Section[INI_MAX_SECTION_LENGTH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char Line[INI_MAX_LINE_LENGTH];
    char *From, *To;

    sprintf (Section, "DebugLabelRenamingListForProcesses %s", pappldata->ShortExecutableFileName);
    for(i = 0; i < 10000; i++) {
        sprintf (Entry, "e_%i", i);
        if (IniFileDataBaseReadString(Section, Entry, "", Line, sizeof(Line), GetMainFileDescriptor()) <= 0) break;
        pappldata->RenamingListSize++;
        if (StringCommaSeparate(Line, &From, &To, NULL) == 2) {
            pappldata->RenamingListFrom = my_realloc(pappldata->RenamingListFrom, (size_t)pappldata->RenamingListSize * sizeof(char*));
            pappldata->RenamingListTo = my_realloc(pappldata->RenamingListTo, (size_t)pappldata->RenamingListSize * sizeof(char*));
            pappldata->RenamingListFrom[pappldata->RenamingListSize-1] = StringMalloc(From);
            pappldata->RenamingListTo[pappldata->RenamingListSize-1] = StringMalloc(To);
        }
    }
    return i;
}

static int32_t Replace(char *String, char *From, char *To, int32_t MaxChar)
{
    char *p;
    size_t Len;
    size_t LenTo, LenFrom;
    int32_t Ret = 0;

    p = String;
    Len = strlen(p) + 1;
    LenFrom = strlen(From);
    if (From[0] == '^') { // Zeilenanfang
        LenFrom--;  // '$' am Anfang entfernen
        From++;
        if (strncmp (String, From, LenFrom) == 0) {
            LenTo = strlen(To);
            Len -= LenFrom;
            Len += LenTo;
            if (Len >= (size_t)MaxChar) return -1;
            if (LenFrom != LenTo) memmove(p + LenTo, p + LenFrom, Len - (size_t)(p - String) - LenTo);
            memmove (p, To, LenTo);
            return 1;
        }
    } else if ((LenFrom >= 1) && (From[LenFrom-1] == '$')) { // Zeilenende
        if (LenFrom <= Len) {
            LenFrom--;  // '$' am Ende anschneiden
            if (strncmp (String + Len - 1 - LenFrom, From, LenFrom) == 0) {
                LenTo = strlen(To);
                if ((Len - LenFrom + LenTo) >= (size_t)MaxChar) return -1;
                memmove (String + Len - 1 - LenFrom, To, LenTo + 1);
                return 1;
            }
        }
    } else {
        while(1) {
            p = strstr(p, From);
            if (p != NULL) {
                LenTo = strlen(To);
                Len -= LenFrom;
                Len += LenTo;
                if (Len >= (size_t)MaxChar) return -1;
                if (LenFrom != LenTo) memmove(p + LenTo, p + LenFrom, Len - (size_t)(p - String) - LenTo);
                memmove (p, To, LenTo);
                p += LenTo;
                Ret++;
            } else {
                break;
            }
        }
    }
    return Ret;
}

int32_t RenameLabel(DEBUG_INFOS_DATA *pappldata, char *Label, int32_t MaxChars)
{
    int32_t i;
    int32_t Ret = 0;

    for (i = 0; i < pappldata->RenamingListSize; i++) {
        char *From, *To;
        From = pappldata->RenamingListFrom[i];
        To = pappldata->RenamingListTo[i];

        Ret += Replace(Label, From, To, MaxChars);

    }
    return Ret;
}

DEBUG_INFOS_ASSOCIATED_CONNECTION *ReadDebufInfosNotConnectingToProcess (const char *par_Type, const char *par_DebugInfosFileName)
{
    int Ret;
    DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata = my_calloc(1, sizeof(DEBUG_INFOS_ASSOCIATED_CONNECTION));
    pappldata->AssociatedProcess = my_calloc(1, sizeof(DEBUG_INFOS_ASSOCIATED_PROCESSE));
    pappldata->AssociatedProcess->Pid = -1;
    pappldata->AssociatedProcess->AssociatedDebugInfos = my_calloc(1, sizeof(DEBUG_INFOS_DATA));

    init_lists (pappldata->AssociatedProcess->AssociatedDebugInfos);
    if (strcmpi(par_Type, "ELF") == 0) {
        Ret = parse_dwarf_from_exe_file ((char*)par_DebugInfosFileName, pappldata->AssociatedProcess->AssociatedDebugInfos, 0, 3);  // mashine type linux
        calc_all_array_sizes (pappldata->AssociatedProcess->AssociatedDebugInfos, 0);
    } else if (strcmpi(par_Type, "EXE") == 0) {
        Ret = LoadDebugInfos(pappldata->AssociatedProcess->AssociatedDebugInfos, (char*)par_DebugInfosFileName, 0, 0, 0);
    } else {
        Ret = -1;
    }
    if (Ret != 0) {
        DeleteDebugInfos (pappldata->AssociatedProcess->AssociatedDebugInfos);
        ThrowError (ERROR_SYSTEM_CRITICAL, "cannot read debug infos from \"%s\"", par_DebugInfosFileName);
        return NULL;
    }
    return pappldata;
}

void DeleteDebufInfosNotConnectingToProcess(DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata)
{
    DeleteDebugInfos(pappldata->AssociatedProcess->AssociatedDebugInfos);
    my_free(pappldata->AssociatedProcess->AssociatedDebugInfos);
    my_free(pappldata->AssociatedProcess);
    my_free(pappldata);
}

void TerminateDebugInfos (void)
{
    int x;
    ENTER_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
    for (x = 0; x < DEBUG_INFOS_MAX_DEBUG_INFOS; x++) {
        if (DebugInfosDatas[x].UsedFlag && (DebugInfosDatas[x].AttachCounter == 0) && !DebugInfosDatas[x].TryToLoadFlag) {
            DeleteDebugInfos (&DebugInfosDatas[x]);
        }
    }
    get_return_buffer_func_thread (NULL, -1);
    LEAVE_GLOBAL_CRITICAL_SECTION (&DebugInfosCriticalSection);
}
