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


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Platform.h"
#ifdef _WIN32
#include <Psapi.h>
#include <process.h>
#endif
#include "XcpOverEthernet.h"
extern "C" {
#include "StringMaxChar.h"
#include "Scheduler.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
}

#include "XcpConnector.h"
#include "XcpCopyPages.h"

/* Max size of a section name */
#define MAXIMUM_DWARF_NAME_SIZE 64
/* Siz of a symbols inside the COFF symbol table */
#define SYMBOL_SIZE   18 


cCopyPages::cCopyPages (void)
{
    Pid = -1;
    LinkNo = -1;
    STRING_COPY_TO_ARRAY (ExecutableName, "");
    Pages = nullptr;
    NoOfPages = 0;
    SectionInfoArray = nullptr;
    SectionInfoArraySize = 0;
}

cCopyPages::~cCopyPages (void)
{
    delete_all_page_files ();
    if (SectionInfoArray != nullptr) my_free (SectionInfoArray);
    SectionInfoArray = nullptr;
}

void cCopyPages::SetProcess (int par_Pid, int par_LinkNo)
{
    Pid = par_Pid;
    LinkNo = par_LinkNo;
    if (GetProcessInsideExecutableName (Pid, ExecutableName, sizeof(ExecutableName)) < 0) {
        ThrowError (1, "not valid Pid %i", par_Pid);
        STRING_COPY_TO_ARRAY (ExecutableName, "not valid");
    }
}

int cCopyPages::CheckExistanceOfPage (unsigned char SegmentNo, unsigned char PageNo)
{
    int p;
    if (Pages == nullptr) return -1;
    for (p = 0; p < NoOfPages; p++) {
        if ((Pages[p].SegmentNo == SegmentNo) && (Pages[p].PageNo == PageNo)) {
            return p;
        }
    }
    return -1;
}

int cCopyPages::AddPageToCallibationSegment (unsigned char SegmentNo, unsigned char PageNo, uint32_t Size, uint64_t Data)
{
    int p;
    
    if (Size <= 0) return -1;
    p = CheckExistanceOfPage (SegmentNo, PageNo);
    // already exist?
    if (p >= 0) return -1;
    // setup a new one
    p = NoOfPages;
    NoOfPages++;
    Pages = static_cast<PAGE_IN_MEMORY*>(my_realloc (Pages, sizeof (PAGE_IN_MEMORY) * static_cast<size_t>(NoOfPages)));
    if (Pages == nullptr) {
        ThrowError (1, "cannot allocate memory for page buffer");
        return -1;
    }
    Pages[p].SegmentNo = SegmentNo;
    Pages[p].PageNo = PageNo;
    Pages[p].Size = Size;
    Pages[p].Buffer = my_malloc (Size);
    if (Pages[p].Buffer == nullptr) {
        ThrowError (1, "cannot allocate memory for page buffer");
        return -1;
    }
    if (Data != 0) {
        XCPReadMemoryFromExternProcess (LinkNo, Data, Pages[p].Buffer, static_cast<int>(Size));
    } else {
        MEMSET (Pages[p].Buffer, 0, Size);
    }
    return 0;
}

void cCopyPages::delete_all_page_files (void)
{
    int p;
    
    if (Pages != nullptr) {
        for (p = 0; p < NoOfPages; p++) {
            if (Pages[p].Buffer != nullptr) {
                my_free (Pages[p].Buffer);
            }
        }
        my_free (Pages);
    }
    Pages = nullptr;
    NoOfPages = 0;
}

int cCopyPages::ReadSectionInfosFormOneExecutableFile (int TaskNumber, char *par_ExecutableFileName, uint64_t par_BaseAddress, char *par_SectionPrefix)
{
#ifdef _WIN32
    IMAGE_NT_HEADERS32 NtHeader;
    struct _IMAGE_DOS_HEADER DosHeader;
    struct _IMAGE_SECTION_HEADER SectionHeader;
    char SectionName[MAXIMUM_DWARF_NAME_SIZE];
    HANDLE hFileExe;
    DWORD x, xx;

    hFileExe = CreateFile (par_ExecutableFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           nullptr);
    if (hFileExe == reinterpret_cast<HANDLE>(HFILE_ERROR)) {
        ThrowError (1, "Unable to open %s file\n", par_ExecutableFileName);
        return -1;
    }
    ReadFile (hFileExe, &DosHeader, sizeof (DosHeader), &xx, nullptr);
    if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        CloseHandle (hFileExe);
        ThrowError (1, "file %s in not an executable", par_ExecutableFileName);
        return -1;
    }
    SetFilePointer (hFileExe, DosHeader.e_lfanew, nullptr, FILE_BEGIN);
    ReadFile (hFileExe, &NtHeader, sizeof (NtHeader), &xx, nullptr);
    if (NtHeader.Signature != IMAGE_NT_SIGNATURE) {
        CloseHandle (hFileExe);
        ThrowError (1, "file %s in not an executable", par_ExecutableFileName);
        return -1;
    }

    for (x = 0; x < NtHeader.FileHeader.NumberOfSections; x++) {
        ReadFile (hFileExe, &SectionHeader, sizeof (struct _IMAGE_SECTION_HEADER), &xx, nullptr);

        // GCC will use section names longet than 8 bytes
        if (SectionHeader.Name[0] == '/') {
            LARGE_INTEGER SaveFilePos;
            LARGE_INTEGER VirtualOffset;
            LARGE_INTEGER Help;
            DWORD BytesRead;
            char c;
            int i;

            VirtualOffset.QuadPart = NtHeader.FileHeader.PointerToSymbolTable +
                                     (NtHeader.FileHeader.NumberOfSymbols * SYMBOL_SIZE) +
                                     strtoul (reinterpret_cast<char*>(SectionHeader.Name+1), nullptr, 0);
            Help.QuadPart = 0;
            SetFilePointerEx (hFileExe, Help, &SaveFilePos, FILE_CURRENT);
            SetFilePointerEx (hFileExe, VirtualOffset, nullptr, FILE_BEGIN);
            for (i = 0; i < MAXIMUM_DWARF_NAME_SIZE; i++) {
                ReadFile (hFileExe, &c, 1, &BytesRead, nullptr);
                SectionName[i] = c;
                if (c == 0) break;
            }
            SectionName[MAXIMUM_DWARF_NAME_SIZE - 1] = 0;
            SetFilePointerEx (hFileExe, SaveFilePos, nullptr, FILE_BEGIN);
        } else {
            MEMCPY (SectionName, SectionHeader.Name, 8);
            SectionName[8] = 0; // Terminate section name (max. 8 characters)
        }
        if (!strlen (SectionName)) {
            // Ignore section without namen!
            continue;
        } else {
            SectionInfoArray = static_cast<SECTION_LIST_ELEM*>(my_realloc (SectionInfoArray, sizeof (SECTION_LIST_ELEM) * static_cast<size_t>(SectionInfoArraySize + 1)));
            if (SectionInfoArray == nullptr) {
                SectionInfoArraySize = -1;
                ThrowError (1, "out of memory. Cannot load section infos oof executable \"%s\" into memory", par_ExecutableFileName);
                return -1;
            }
            if (par_SectionPrefix != nullptr) {
                STRING_COPY_TO_ARRAY (SectionInfoArray[SectionInfoArraySize].LongName, par_SectionPrefix);
                STRING_APPEND_TO_ARRAY (SectionInfoArray[SectionInfoArraySize].LongName, "::");
                STRING_APPEND_TO_ARRAY (SectionInfoArray[SectionInfoArraySize].LongName, SectionName);
                SectionInfoArray[SectionInfoArraySize].TaskNumber = TaskNumber;
            } else {
                STRING_COPY_TO_ARRAY (SectionInfoArray[SectionInfoArraySize].LongName, SectionName);
                SectionInfoArray[SectionInfoArraySize].TaskNumber = -1;
            }
            STRING_COPY_TO_ARRAY (SectionInfoArray[SectionInfoArraySize].ShortName, SectionName);
            if (!strcmp (SectionName, ".text") ||
                !strcmp (SectionName, ".rdata") ||
                !strcmp (SectionName, ".data") ||
                !strcmp (SectionName, ".xdata") ||
                !strcmp (SectionName, ".rsrc") ||
                // Additionally this from GCC
                !strcmp (SectionName, ".eh_frame") ||
                !strcmp (SectionName, ".bss") ||
                !strcmp (SectionName, ".idata") ||
                !strcmp (SectionName, ".CRT") ||
                !strcmp (SectionName, ".tls") ||
                !strcmp (SectionName, ".debug_aranges") ||
                !strcmp (SectionName, ".debug_pubnames") ||
                !strcmp (SectionName, ".debug_pubtypes") ||
                !strcmp (SectionName, ".debug_info") ||
                !strcmp (SectionName, ".debug_abbrev") ||
                !strcmp (SectionName, ".debug_line") ||
                !strcmp (SectionName, ".debug_frame") ||
                !strcmp (SectionName, ".debug_str") ||
                !strcmp (SectionName, ".debug_loc") ||
                !strcmp (SectionName, ".debug_macinfo")) {
                SectionInfoArray[SectionInfoArraySize].IsStandardSection = 1;
            } else {
                SectionInfoArray[SectionInfoArraySize].IsStandardSection = 0;
            }
            SectionInfoArray[SectionInfoArraySize].IsSetAsCallibation = 0;
            SectionInfoArray[SectionInfoArraySize].IsSetAsCode = 0;
            SectionInfoArray[SectionInfoArraySize].StartAddress = static_cast<uint64_t>(SectionHeader.VirtualAddress) + par_BaseAddress;

            SectionInfoArray[SectionInfoArraySize].Header = SectionHeader;
            SectionInfoArraySize++;

        }
    }
    CloseHandle (hFileExe);
    return 0;
#else
    ThrowError (1, "xcp over ethernet not supported");
    return -1;
#endif
}

int cCopyPages::ReadSectionInfosFromAllExecutable (void)
{
    char ExecutableFileName[MAX_PATH];
    char DllFileName[MAX_PATH];
    char ProcessName[MAX_PATH];
    char *DllOrExecutableName;
    int ProcessInsideExecutableNumber;
    uint64_t ModuleBaseAddress;
    bool FirstWithoutDll = false;
    bool WithDll = false;

    char *d, *s;
    char *ProcessNames = GetAllProcessNamesSemicolonSeparated();

    s = ProcessNames;
    while (*s != 0) {
        d = ProcessName;
        while ((*s != 0) && (*s != ';')) {
            *d++ = *s++;
        }
        *d = 0;
        int Pid;

        if ((Pid = GetProcessPidAndExecutableAndDllName (ProcessName, ExecutableFileName, sizeof(ExecutableFileName),
                                                        DllFileName, sizeof(DllFileName), &ProcessInsideExecutableNumber)) > 0) {
            if ((ProcessInsideExecutableNumber < 0) || (ProcessInsideExecutableNumber >= 8)) {
                ThrowError (1, "not more than 8 processes can be in one executable not %i", ProcessInsideExecutableNumber);
            }
            if (!stricmp (ExecutableName, ExecutableFileName)) {
                if (strlen(DllFileName)) {
                    DllOrExecutableName = DllFileName;
                    WithDll = true;
                } else {
                    DllOrExecutableName = ExecutableFileName;
                    WithDll = false;
                }
                if ((!WithDll && !FirstWithoutDll) || WithDll) {
                    if (GetExternProcessBaseAddress(Pid, &ModuleBaseAddress, DllOrExecutableName) != 0) { // besser waere die Basis-Adresse beim LOGIN mit zu uebertragen
                        ThrowError (1, "cannot get base address of process \"%s\" using DLL \"%s\"", ProcessName, DllOrExecutableName);
                        return -1;
                    }
                    BaseStartAddresses[ProcessInsideExecutableNumber] = ModuleBaseAddress;

                    char FileNameWithoutPath[MAX_PATH];
                    char *p = DllOrExecutableName;
                    while (*p != 0) p++;
                    while ((p > DllOrExecutableName) && (*p != '/') && (*p != '\\')) p--;
                    if ((*p != '/') || (*p != '\\')) p++;
                    STRING_COPY_TO_ARRAY
                        (FileNameWithoutPath, p);
#ifdef _WIN32
                    strupr (FileNameWithoutPath);    // Only uppercase
#endif

                    if (ReadSectionInfosFormOneExecutableFile (ProcessInsideExecutableNumber,
                                                               DllOrExecutableName, ModuleBaseAddress, FileNameWithoutPath)) {
                        return -1;
                    }
                    if (!WithDll) {
                        FirstWithoutDll = 1;
                    }
                }
            }
        }

        if (*s == ';') {
            s++;
        }

        if (*s == 0) {
            break;    // End of the list
        }
    }

    if (SectionInfoArraySize == 0) {
        ThrowError (1, "error no section found in executable file \"%s\"\n", ExecutableName);
    }
    return SectionInfoArraySize;
}

int cCopyPages::get_number_of_sections (void)
{
    // Section infos already readed?
    if (SectionInfoArraySize == 0) ReadSectionInfosFromAllExecutable ();
    return SectionInfoArraySize;
}

char* cCopyPages::get_sections_name (int Number)
{
    // Section infos already readed?
    if (SectionInfoArraySize == 0) ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return nullptr;
    else return SectionInfoArray[Number].LongName;
}

char *cCopyPages::get_sections_short_name(int Number)
{
    // Section infos already readed?
    if (SectionInfoArraySize == 0) ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return nullptr;
    else return SectionInfoArray[Number].ShortName;
}

int cCopyPages::get_sections_task_number(int Number)
{
    // Section infos already readed?
    if (SectionInfoArraySize == 0) ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return 0;
    else return SectionInfoArray[Number].TaskNumber;
}

#define MinSectionSize(s) ((s.SizeOfRawData < s.Misc.VirtualSize) ? s.SizeOfRawData : s.Misc.VirtualSize)

uint32_t cCopyPages::get_sections_size (int Number)
{
    // Section infos already readed?
    if (SectionInfoArraySize == 0) ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return 0;
#ifdef _WIN32
    else return MinSectionSize(SectionInfoArray[Number].Header);
#else
    else return SectionInfoArray[Number].Header.sh_size;
#endif
}

uint64_t cCopyPages::get_sections_start_address (int Number)
{
    // Section infos already readed?
    if (SectionInfoArraySize == 0) ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return 0;
    else return SectionInfoArray[Number].StartAddress;
}

int cCopyPages::is_sections_callibation_able (int Number)
{
    // Section infos already readed?
    if (SectionInfoArraySize == 0) ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return 0;
#ifdef _WIN32
    else return (((SectionInfoArray[Number].Header.Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) == IMAGE_SCN_CNT_INITIALIZED_DATA));
#else
    else return 0;
#endif
}

#ifdef _WIN32
struct _IMAGE_SECTION_HEADER* cCopyPages::get_sections_header (int Number)
{
    // Section infos already readed?
    if (SectionInfoArraySize == 0) ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return nullptr;
    else return &(SectionInfoArray[Number].Header);
}
#else
Elf32_Shdr* cCopyPages::get_sections_header (int Number)
{
    // Section infos already readed?
    if (SectionInfoArraySize == 0) ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return nullptr;
    else return &(SectionInfoArray[Number].Header);
}
#endif

int cCopyPages::find_section_by_name (int StartIdx, char *Segment)
{
    int x;
    // Section infos already readed?
    if (SectionInfoArraySize == 0) ReadSectionInfosFromAllExecutable ();
    for (x = StartIdx; x < SectionInfoArraySize; x++) {
        char *LongName = SectionInfoArray[x].LongName;
        if (!strcmp (Segment, LongName)) {
            return x;
        }
    }
    return -1;
}

// save_or_load != 0 than save otherwise load
int cCopyPages::save_or_load_section_to_or_from_file (int save_or_load, char *SelectedSection, 
                                                     int SegNo, int PageNo)
{
    int x;
    // Section infos already readed?
    if (SectionInfoArraySize == 0) ReadSectionInfosFromAllExecutable ();
    for (x = 0; x < SectionInfoArraySize; x++) {
        if (!strcmp (SelectedSection, SectionInfoArray[x].LongName)) {
            int p = CheckExistanceOfPage (static_cast<unsigned char>(SegNo), static_cast<unsigned char>(PageNo));

            if (p < 0) {
                ThrowError (1, "segment %i page %i doesn't exists", SegNo, PageNo);
                return 1;
            }
#ifdef _WIN32
            if (Pages[p].Size != MinSectionSize(SectionInfoArray[x].Header)) {
#else
            if (Pages[p].Size != SectionInfoArray[x].Header.sh_size) {
#endif
                ThrowError (1, "segment %i page %i have wrong size", SegNo, PageNo);
                return 1;
            }

            if (save_or_load) {
                XCPReadMemoryFromExternProcess (LinkNo, SectionInfoArray[x].StartAddress, Pages[p].Buffer, static_cast<int>(Pages[p].Size));
            } else {
                XCPWriteMemoryToExternProcess (LinkNo, SectionInfoArray[x].StartAddress, Pages[p].Buffer, static_cast<int>(Pages[p].Size));
            }

            return 0;
        }
    }
    ThrowError (1, "error section %s not found\n", SelectedSection);
    return 1;
}

int cCopyPages::switch_page_of_section (int A2LSection, int ExeSection, int FromPage, int ToPage, cXCPConnector *pConnector)
{
    // Section infos already readed?
    if (SectionInfoArraySize == 0) ReadSectionInfosFromAllExecutable ();
    if (ExeSection >= SectionInfoArraySize) {
        ThrowError (1, "Section number %i is lager than available sections %i cannot switch page", ExeSection, SectionInfoArraySize);
        return -1;
    }

    int p = CheckExistanceOfPage (static_cast<unsigned char>(A2LSection), static_cast<unsigned char>(FromPage));

    if (p < 0) {
        ThrowError (1, "segment %i page %i doesn't exists", A2LSection, FromPage);
        return 1;
    }

    if (Pages[p].Size != get_sections_size (ExeSection)) {
        ThrowError (1, "segment %i page %i have wrong size", A2LSection, FromPage);
        return 1;
    }

    pConnector->WriteLogString("XCPReadMemoryFromExternProcess(%p, ...) p=%i", get_sections_start_address (ExeSection), p);
    pConnector->WriteLogString("before Pages[%i].Size =  %i, Pages[%i].Buffer =", p, Pages[p].Size, p);
    pConnector->WriteLogByteBuffer(static_cast<unsigned char*>(Pages[p].Buffer), static_cast<int>(Pages[p].Size));
    XCPReadMemoryFromExternProcess (LinkNo, get_sections_start_address (ExeSection), Pages[p].Buffer, static_cast<int>(Pages[p].Size));
    pConnector->WriteLogString("behind Pages[%i].Size =  %i, Pages[%i].Buffer =", p, Pages[p].Size, p);
    pConnector->WriteLogByteBuffer(static_cast<unsigned char*>(Pages[p].Buffer), static_cast<int>(Pages[p].Size));

    if (ToPage >= 0) {
        p = CheckExistanceOfPage (static_cast<unsigned char>(A2LSection), static_cast<unsigned char>(ToPage));

        if (p < 0) {
            ThrowError (1, "segment %i page %i doesn't exists", A2LSection, ToPage);
            return 1;
        }

        if (Pages[p].Size != get_sections_size (ExeSection)) {
            ThrowError (1, "segment %i page %i have wrong size", A2LSection, ToPage);
            return 1;
        }
        pConnector->WriteLogString("XCPWriteMemoryToExternProcess(%p, ...) p=%i", get_sections_start_address (ExeSection), p);
        pConnector->WriteLogString("before Pages[%i].Size =  %i, Pages[%i].Buffer =", p, Pages[p].Size, p);
        pConnector->WriteLogByteBuffer(static_cast<unsigned char*>(Pages[p].Buffer), static_cast<int>(Pages[p].Size));
        XCPWriteMemoryToExternProcess (LinkNo, get_sections_start_address (ExeSection), Pages[p].Buffer, static_cast<int>(Pages[p].Size));
        pConnector->WriteLogString("behind Pages[%i].Size =  %i, Pages[%i].Buffer =", p, Pages[p].Size, p);
        pConnector->WriteLogByteBuffer(static_cast<unsigned char*>(Pages[p].Buffer), static_cast<int>(Pages[p].Size));
    }
    return 0;
}

int cCopyPages::copy_section_page_file (int SegFrom, int PageFrom, int SegTo, int PageTo)
{
    int Src;
    int Dst;

    Src = CheckExistanceOfPage (static_cast<unsigned char>(SegFrom), static_cast<unsigned char>(PageFrom));
    if (Src < 0) {
        ThrowError (1, "source section %i page %i dons't exists", SegFrom, PageFrom);
        return 1;
    }
    Dst = CheckExistanceOfPage (static_cast<unsigned char>(SegTo), static_cast<unsigned char>(PageTo));
    if (Dst < 0) {
        ThrowError (1, "destination section %i page %i dons't exists", SegTo, PageTo);
        return 1;
    }
    if (Pages[Dst].Size != Pages[Src].Size) {
        ThrowError (1, "source section %i page % and destination section %i page % have not the same size", SegFrom, PageFrom, SegTo, PageTo);
        return 1;
    }
    MEMCPY(Pages[Dst].Buffer, Pages[Src].Buffer, Pages[Dst].Size);

    return 0;
}

// The address is 64 wide but currnt only 32 bit executables are possible
uint64_t cCopyPages::TranslateXCPOverEthernetAddress(uint64_t par_Address)
{
    uint64_t Address = par_Address;
    if ((Address & 0x80000000) == 0x80000000) {
        // It is a DLL address because the firts bit is set
        // This is a definition here: The highest 3 bits represent an index into the process table, so only 28 bits are left for a relative address
        int Index;
        Index = (Address >> 28) & 0x7;
        if (Index >= 8) {
            ThrowError (1, "cannot translate address 0x%08X because 3 bit index %i inside address is out of range (0...%i)", Address, Index, 8);
            return par_Address;
        }
        // Calculate the DLL address
        Address = BaseStartAddresses[Index] + (Address & 0x0FFFFFFF);
        return Address;
    } else {
        return par_Address;
    }
}

// Returns 1 if section was successfully written to a copy inside the temporary folder of the executable.
// If an error happens 0 will be returned
extern "C" int write_section_to_exe_file (int Pid, char *SelectedSection)
{
    return scm_write_section_to_exe (Pid, SelectedSection);
}
