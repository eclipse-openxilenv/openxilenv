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


#ifndef DEBUGINFOS_H
#define DEBUGINFOS_H

#include <stdint.h>
#include "Platform.h"

typedef struct {
    char *name;
    int32_t typenr;       // 0 ... 0x0FFF Base data types
    int32_t typenr_idx;   // if this is -1 it is not valid (must be calculate later)
    uint64_t adr;
} LABEL_LIST_ELEM;

typedef struct {
    uint64_t adr;
    int32_t label_idx;
} ADDR_LIST_ELEM;

typedef struct {
    uint32_t hash_code;
    int32_t label_idx;
} HASH_LABEL_LIST_ELEM;


typedef struct {
    char *name;
    int32_t typenr;
    int32_t typenr_idx;   // if this is -1 it is not valid (must be calculate later)
    int32_t offset;
    int32_t nextidx;      // -1 end of the list
} FIELDIDX_LIST_ELEM;

typedef struct {
    uint16_t what;
    #define BASE_DATA_TYPE 1
    #define STRUCT_ELEM    2
    #define ARRAY_ELEM     3
    #define POINTER_ELEM   4
    #define FIELD_ELEM     5     // this are inside an own array
    #define UNION_ELEM     6
    #define MODIFIER_ELEM  7
    #define PRE_DEC_STRUCT 8
    #define PRE_DEC_VAR    9
    #define PRE_DEC_FIELD  10
    #define PRE_SPEC_LABEL 11
    #define SPEC_STRUCT    12
    #define SPEC_VAR       13
    #define SPEC_FIELD     14
    #define TYPEDEF_ELEM   15

    char *name;
    int32_t typenr;
    int32_t typenr_fields;
    int32_t typenr_fields_idx;      // if this is -1 it is not valid (must be calculate later)
    int32_t array_size;
    int32_t array_elements;
    int32_t array_elem_typenr;
    int32_t array_elem_typenr_idx;  // if this is -1 it is not valid (must be calculate later)
    int32_t pointsto;
    int32_t pointsto_idx;           // if this is -1 it is not valid (must be calculate later)
} DTYPE_LIST_ELEM;

typedef struct {
    int32_t fieldnr;
    int32_t fields;         // This is only for checks
    int32_t start_fieldidx;
} FIELD_LIST_ELEM;



typedef struct PROCESS_DEBUG_DATA_STRUCT {
    int32_t UsedFlag;    // If 1 than the structure is in use, if 0 than it is free

    int32_t AttachCounter;

    CRITICAL_SECTION CriticalSection;
    int32_t DebugCriticalSectionLineNr;
    DWORD DebugCriticalSectionThreadId;

    int32_t TryToLoadFlag;
    int32_t LoadedFlag;
    char ShortExecutableFileName[MAX_PATH];
    char ExecutableFileName[MAX_PATH];
    char DebugInfoFileName[MAX_PATH];

    int32_t associated_exe_file;   // If 0 than the debug info is loaded but there are no assigned process

    int32_t dynamic_base_address;  // This is 1 if insied the EXE file the flag for dynamic base address is set
                                   // Than each time the process will be started the addresse must be recalculated

    int32_t DebugInfoType;
#define DEBUGINFOTYPE_VISUAL6          0x101
#define DEBUGINFOTYPE_VISUALNET_2003   0x102
#define DEBUGINFOTYPE_VISUALNET_2008   0x103
#define DEBUGINFOTYPE_VISUALNET_2010   0x104
#define DEBUGINFOTYPE_VISUALNET_2012   0x105
#define DEBUGINFOTYPE_VISUALNET_2012   0x105
#define DEBUGINFOTYPE_VISUALNET_2015   0x107
#define DEBUGINFOTYPE_VISUALNET_2017   0x108
#define DEBUGINFOTYPE_VISUALNET_2019   0x109

#define DEBUGINFOTYPE_DWARF            0x201

    int32_t AbsoluteOrRelativeAddress;
#define DEBUGINFO_ADDRESS_ABSOLUTE  0
#define DEBUGINFO_ADDRESS_RELATIVE  1

    uint64_t LoadedFileTime;   // Age of the loaded PDB file
    DWORD Checksum;            // Checksumme of the EXE file Remark: this must be activate during linking

    uint32_t WhenWasTheAssociatedExeRemoved;  // This will overflow each 49 days

    // Debug data entrys
    struct SYMBOL_BUFFER {
        char *symbol_name_list;
        uint32_t symbol_name_list_entrys;  // in Bytes
        uint32_t size_symbol_name_list;   // in Bytes
#ifndef _M_X64
        int32_t fill;
#endif
    } *symbol_buffer;
    uint32_t  symbol_buffer_entrys;
    uint32_t  size_symbol_buffer;
    uint32_t  symbol_buffer_active_entry;

    LABEL_LIST_ELEM *label_list;  
    ADDR_LIST_ELEM *addr_list;    // Contains a sorted by address label list
    uint32_t *sorted_label_list;
    uint32_t  label_list_entrys;
    uint32_t  size_label_list;

    LABEL_LIST_ELEM *label_list_no_addr;  
    uint32_t  label_list_no_addr_entrys;
    uint32_t  size_label_list_no_addr;

    DTYPE_LIST_ELEM **dtype_list;
    uint32_t  dtype_list_entrys;
    uint32_t  size_dtype_list;

    DTYPE_LIST_ELEM **dtype_list_1k_blocks;
    uint32_t  dtype_list_1k_blocks_entrys;
    uint32_t  size_dtype_list_1k_blocks;
    uint32_t  dtype_list_1k_blocks_active_entry;

        // List of additionally automatically generated data types (0x7FFFFFFF downwards)
    DTYPE_LIST_ELEM **h_dtype_list;
    uint32_t  h_dtype_list_entrys;
    uint32_t  h_size_dtype_list;

    DTYPE_LIST_ELEM **h_dtype_list_1k_blocks;
    uint32_t  h_dtype_list_1k_blocks_entrys;
    uint32_t  h_size_dtype_list_1k_blocks;
    uint32_t  h_dtype_list_1k_blocks_active_entry;

    // Field-List
    int32_t unique_fieldnr_generator;   // This will start from 0x10000000 and will be incremented
    FIELD_LIST_ELEM ** field_list;
    uint32_t  field_list_entrys;
    uint32_t  size_field_list;

    FIELD_LIST_ELEM **field_list_1k_blocks;
    uint32_t  field_list_1k_blocks_entrys;
    uint32_t  size_field_list_1k_blocks;
    uint32_t  field_list_1k_blocks_active_entry;


    FIELDIDX_LIST_ELEM *fieldidx_list;
    uint32_t  fieldidx_list_entrys;
    uint32_t  size_fieldidx_list;


    uint64_t ImageBaseAddr;
    int32_t NumOfSections;
    char **SectionNames;  // GCC will use section name longer a 8 byte
    uint64_t *SectionVirtualAddress;
    uint32_t *SectionVirtualSize;
    uint32_t *SectionSizeOfRawData;
    uint64_t *SectionPointerToRawData;

    int PointerSize;  // this should be 4 or 8

    int SignatureType;   // 0 -> ignore, 1 -> Signature are valid, 2 -> SignatureGuid are valid
    DWORD Signature;
    GUID SignatureGuid;
    DWORD Age;

    // Remaning list
    int32_t RenamingListSize;
    char **RenamingListFrom;
    char **RenamingListTo;

} DEBUG_INFOS_DATA;

// max. 100 windows or other things using debug ifos
#define DEBUG_INFOS_MAX_CONNECTIONS 100
#define DEBUG_INFOS_MAX_PROCESSES   64
#define DEBUG_INFOS_MAX_DEBUG_INFOS   64

typedef struct {
    int32_t UsedFlag;    // If 1 the structure is in use, if 0 than it is free
    int32_t AttachCounter;
    char *ProcessName;
    char *ExeOrDllName;
    int32_t Pid;
    int32_t IsRealtimeProcess;
    int32_t MachineType;
    uint64_t  BaseAddress;

    DEBUG_INFOS_DATA *AssociatedDebugInfos;

} DEBUG_INFOS_ASSOCIATED_PROCESSE;

typedef struct {
    int32_t UsedFlag;    // If 1 the structure is in use, if 0 than it is free
    int32_t UsedByUniqueId;
    int32_t (*LoadUnloadCallBackFuncs)(void *par_User, int32_t par_Flags, int32_t par_Action);
    int32_t (*LoadUnloadCallBackFuncsSchedThread)(int32_t par_UniqueId, void *par_User, int32_t par_Flags, int32_t par_Action);
    void *LoadUnloadCallBackFuncParUsers;
#define DEBUG_INFOS_FLAG_LOADED  0x00000001UL
#define DEBUG_INFOS_ACTION_PROCESS_TERMINATED   1
#define DEBUG_INFOS_ACTION_PROCESS_STARTED      2

    DEBUG_INFOS_ASSOCIATED_PROCESSE *AssociatedProcess;
/* 40Bytes */
    char Fill[64-40];
} DEBUG_INFOS_ASSOCIATED_CONNECTION;


typedef DEBUG_INFOS_ASSOCIATED_CONNECTION PROCESS_APPL_DATA;

#endif
