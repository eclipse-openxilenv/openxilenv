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


#ifndef _COPY_PAGES_H
#define _COPY_PAGES_H

/* Max size of a section name */
#define MAXIMUM_DWARF_NAME_SIZE 64

#ifdef __cplusplus 

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#include <stdint.h>

class cXCPConnector;

class cCopyPages
{
private:
    int Pid;
    int LinkNo;
    char ExecutableName[MAX_PATH];
    uint64_t BaseStartAddresses[8];  // max. 8 processes allowed

    typedef struct {
        unsigned char SegmentNo;
        unsigned char PageNo;
        uint32_t Size;
        void *Buffer;
    } PAGE_IN_MEMORY;


    PAGE_IN_MEMORY *Pages;
    int NoOfPages;

    typedef struct {
        char ShortName[MAXIMUM_DWARF_NAME_SIZE];
        char LongName[MAX_PATH + 2 + MAXIMUM_DWARF_NAME_SIZE];
        int TaskNumber;
        int IsStandardSection;
        int IsSetAsCallibation;
        int IsSetAsCode;
        uint64_t StartAddress;
#ifdef _WIN32
        struct _IMAGE_SECTION_HEADER Header;
#else
        Elf32_Shdr Header;
#endif
    } SECTION_LIST_ELEM;

    SECTION_LIST_ELEM *SectionInfoArray;
    int SectionInfoArraySize;

    int ReadSectionInfosFromAllExecutable (void);
    int ReadSectionInfosFormOneExecutableFile (int TaskNumber, char *par_ExecutableFileName, uint64_t par_BaseAddress, char *par_SectionPrefix);

    int CheckExistanceOfPage (unsigned char SegmentNo, unsigned char PageNo);

public:
    cCopyPages (void);
    ~cCopyPages (void);

    void SetProcess (int par_Pid, int par_LinkNo);

    int AddPageToCallibationSegment (unsigned char SegmentNo, unsigned char PageNo, uint32_t Size, uint64_t Data);


    int copy_section_file (char *SelectedSection, int SegFrom, int PageFrom, int SegTo, int PageTo);

    // save_or_load != 0 than save otherwise load
    int save_or_load_section_to_or_from_file (int save_or_load, char *SelectedSection,
                                              int SegNo, int PageNo);

    void delete_all_page_files (void);
    int get_number_of_sections (void);
    char* get_sections_name (int Number);
    char* get_sections_short_name (int Number);
    int get_sections_task_number(int Number);
    uint32_t get_sections_size(int Number);
    uint64_t get_sections_start_address(int Number);
    int is_sections_callibation_able (int Number);
#ifdef _WIN32
    struct _IMAGE_SECTION_HEADER* get_sections_header (int Number);
#else
    Elf32_Shdr* get_sections_header (int Number);
#endif
    int switch_page_of_section (int A2LSection, int ExeSection, int FromPage, int ToPage, cXCPConnector *pConnector);
    int copy_section_page_file (int SegFrom, int PageFrom, int SegTo, int PageTo);
    int find_section_by_name (int StartIdx, char *Segment);

    uint64_t TranslateXCPOverEthernetAddress(uint64_t par_Address);
};

#endif

#ifdef __cplusplus 
extern "C" {
#endif

// Returns 1 if section was successfully written to a copy inside the temporary folder of the executable.
// If an error happens 0 will be returned
int write_section_to_exe_file (int Pid, char *SelectedSection);

#ifdef __cplusplus 
}
#endif

#endif
