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


#ifndef COPY_PAGES_H
#define COPY_PAGES_H

// Not direct include stdint.h because VS 2003don't now this.
#include "my_stdint.h"

int AddPageToCallibationSegment (unsigned char SegmentNo, unsigned char PageNo, long Size, void *Data);


int copy_section_file (char *SelectedSection, int SegFrom, int PageFrom, int SegTo, int PageTo);

// save_or_load != 0 Save sonst Load
int save_or_load_section_to_or_from_file (int save_or_load, char *SelectedSection, 
                                          int SegNo, int PageNo);

void delete_all_page_files (void);
int get_number_of_sections (void);
char* get_sections_name (int Number);
char* get_sections_short_name (int Number);
int get_sections_task_number(int Number);
long get_sections_size (int Number);
void *get_sections_start_address (int Number);
int is_sections_callibation_able (int Number);
struct _IMAGE_SECTION_HEADER* get_sections_header (int Number);
uint64_t get_sections_file_offset(int Number);

int switch_page_of_section (int A2LSection, int ExeSection, int FromPage, int ToPage);
int copy_section_page_file (int SegFrom, int PageFrom, int SegTo, int PageTo);
int find_section_by_name (int StartIdx, char *Segment);


#ifdef __cplusplus 
extern "C" {
#endif

#include "ExtpProcessAndTaskInfos.h"

int change_section_access_protection_attributes (char *SelectedSection, DWORD NewProtect);

int change_all_none_code_section_access_protection_attributes_to_read_wite (void);

int XilEnvInternal_CheckIfAddessIsInsideAWritableSection (void *Address);

void *MemoryCopyToWriteProtectedSection (void *Dst, const void *Src, int Size);

// Returns 1 if section are sucessful write back to the executable otherwise 0
int XilEnvInternal_write_section_to_exe_file (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, char *SelectedSection);

#ifdef __cplusplus 
}
#endif

#endif // COPY_PAGES_H
