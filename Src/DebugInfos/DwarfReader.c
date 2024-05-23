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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Platform.h"

#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "MainValues.h"
#include "Files.h"
#include "Scheduler.h"
#include "DebugInfoDB.h"
#include "DwarfReader.h"

// If DebugOut exists write some debug infos to c:\\temp\\debug_dwarf.txt
//static FILE *DebugOut;
#define DebugOut NULL
//#define FFLUSH_DEBUGOUT fflush(DebugOut)
#define FFLUSH_DEBUGOUT
//#define DEBUGFPRINTF(d, f, ...) fprintf(d, f, __VA_ARGS__), fflush(DebugOut)
//#define DEBUGFPRINTF0(d, f) fprintf(d, f), fflush(DebugOut)
#define DEBUGFPRINTF(d, f, ...)
#define DEBUGFPRINTF0(d, f)
//#define PER_PROCESS_LOGGING

#define UNUSED(x) (void)(x)


typedef struct {
    char *Name;
    uint32_t Code;
} DW_NAME_TABLE;

#include "DwarfDwTags.h"
#include "DwarfDwAt.h"
#include "DwarfDwForm.h"
#include "DwarfDecodeLocation.h"


// Only internal for the DWarf reader temporary store DW_AT_declaration solved with DW_AT_specification
typedef struct {
    int32_t TypeNr;
    int32_t FieldTypeNr;
    char *Name;
} LIST_OF_STRUCT_DECLARATION_ELEM;

typedef struct {
    DEBUG_INFOS_DATA *pappldata;
    void *debug_info;
    uint64_t debug_info_len;
    void *debug_abbrev;
    uint64_t debug_abbrev_len;
    void *debug_str;
    uint64_t debug_str_len;
    // new for Dwarf5
    void *debug_line_str;
    uint64_t debug_line_str_len;

    void *debug_pubtypes;
    uint64_t debug_pubtypes_len;
    void *debug_pubnames;
    uint64_t debug_pubnames_len;

    uint64_t BaseAddr;

    int Is64BitFormat;
    uint32_t pointer_size;   // 4 or 8
    uint32_t offset_size;    // 4 or 8
    int32_t dwarf_version;  // -1, 2,3,4
    int32_t HelpIdActiveFieldList;

    // Only internal for the DWarf reader temporary store DW_AT_declaration solved with DW_AT_specification
    LIST_OF_STRUCT_DECLARATION_ELEM *ListOfStructDeclarations;
    int32_t SizeOfListOfStructDeclarations;
    int32_t PosInListOfStructDeclarations;
    // End of only internal for the DWarf

    // begin CompileUnit
    char CurrentCompilationUnitName[64*1024];
    uint32_t PosOfLastCharInCurrentCompilationUnitName;

    char DemangledSymbolNameBuffer[64*1024];

    // begin Namespace
    char EntireNameSpace[8*1024];
    uint32_t PosOfLastCharInNameSpace;
    uint32_t CounterNameSpace;
    uint32_t BeginingOfNamenSpaces[128];

    // begin Classname
    char EntireClassName[128*1024];
    uint32_t PosOfLastCharInClassName;
    uint32_t CounterClassName;
    uint32_t BeginingOfClassNamens[128];
} DEBUG_SECTIONS;



typedef struct {
    struct {
        int8_t Name;
        int8_t Type;
        int8_t Size;
        int8_t Location;
        int8_t External;
        int8_t Declaration;
        int8_t Encoding;
        int8_t Sibling;
        int8_t UpperBound;
        int8_t Count;
        int8_t Specification;
        int8_t MIPSLinkageName;
        int8_t LinkageName;
    } Flags;
    char *Name;
    int32_t Type;
    int32_t Size;
    uint64_t Location;
    int32_t External;
    int32_t Declaration;
    uint32_t Encoding;
    uint32_t Sibling;
    int32_t UpperBound;
    int32_t Count;
    int32_t Specification;
    char *MIPSLinkageName;
    char *LinkageName;
} ONE_DIE_ATTRIBUTES_DATA;


/* Maximal size of DWARF namens */
#define MAXIMUM_DWARF_NAME_SIZE 64
/* Size in bytes of symbols inside COFF symbol table */
#define SYMBOL_SIZE   18

#define PEHEADER_OFFSET(a)  (a.e_lfanew      +	\
             sizeof (DWORD)		     +	\
             sizeof (IMAGE_FILE_HEADER))


int32_t SearchPubTypesOrName (DEBUG_SECTIONS *ds, int32_t CompileUnit, const char *Name,
                              int32_t NameOrTypeFlag, uint32_t *ret_OffsetInDebugInfo);


int32_t get_basetype(int32_t byteSize, uint32_t typeEncoding)
{
    switch (typeEncoding) {
    case 0x1:          // void
        return 33;
    case 0x2:
    case 0x5:
    case 0x6:
        switch (byteSize) {
        case 1:
            return 1;   // BB_BYTE
        case 2:
            return 3;   // BB_WORD
        case 4:
            return 5;   // BB_DWORD
        case 8:
            return 7;   // BB_QWORD
        default:
            return TYPEID_OFFSET - 1;  // unknown
        }
    case 0x7:
    case 0x8:
        switch (byteSize) {
        case 1:
            return 2;   // BB_UBYTE
        case 2:
            return 4;   // BB_UWORD
        case 4:
            return 6;   // BB_UDWORD
        case 8:
            return 8;  // BB_UQWORD
        default:
            return TYPEID_OFFSET - 1;  // unknown
        }
    case 0x3:
    case 0x4:
    case 0x9:
        switch (byteSize) {
        case 4:
            return 9;   // BB_FLOAT
        case 8:
            return 10;   // BB_DOUBLE
        default:
            return TYPEID_OFFSET - 1;  // unknown
        }
    case 0xD:   // btULong
        switch (byteSize) {
        case 4:
            return 6;   // BB_UDWORD
        default:
            return TYPEID_OFFSET - 1;  // unknown
        }
    case 0xE:  // btLong
        switch (byteSize) {
        case 4:
            return 5;   // BB_DWORD
        default:
            return TYPEID_OFFSET - 1;  // unknown
        }
    default:
        // fprintf(fp, "internal error %s (%i)", __FILE__, __LINE__);
        return TYPEID_OFFSET - 1;  // unknown
    }
}

#ifdef _WIN32
// ELF file structures for Windows

#define EI_NIDENT       16
#pragma pack(push, 4)
typedef struct {
  unsigned char     e_ident[EI_NIDENT];
  uint16_t          e_type;
  uint16_t          e_machine;
  uint32_t          e_version;
  uint32_t          e_entry;
  uint32_t          e_phoff;
  uint32_t          e_shoff;
  uint32_t          e_flags;
  uint16_t          e_ehsize;
  uint16_t          e_phentsize;
  uint16_t          e_phnum;
  uint16_t          e_shentsize;
  uint16_t          e_shnum;
  uint16_t          e_shstrndx;
} Elf32_Ehdr;
typedef struct {
  unsigned char     e_ident[EI_NIDENT];
  uint16_t          e_type;
  uint16_t          e_machine;
  uint32_t          e_version;
  uint64_t          e_entry;
  uint64_t          e_phoff;
  uint64_t          e_shoff;
  uint32_t          e_flags;
  uint16_t          e_ehsize;
  uint16_t          e_phentsize;
  uint16_t          e_phnum;
  uint16_t          e_shentsize;
  uint16_t          e_shnum;
  uint16_t          e_shstrndx;
} Elf64_Ehdr;

typedef struct {
  uint32_t    sh_name;
  uint32_t    sh_type;
  uint32_t    sh_flags;
  uint32_t    sh_addr;
  uint32_t    sh_offset;
  uint32_t    sh_size;
  uint32_t    sh_link;
  uint32_t    sh_info;
  uint32_t    sh_addralign;
  uint32_t    sh_entsize;
} Elf32_Shdr;
typedef struct {
  uint32_t    sh_name;
  uint32_t    sh_type;
  uint64_t    sh_flags;
  uint64_t    sh_addr;
  uint64_t    sh_offset;
  uint64_t    sh_size;
  uint32_t    sh_link;
  uint32_t    sh_info;
  uint64_t    sh_addralign;
  uint64_t    sh_entsize;
} Elf64_Shdr;
#pragma pack(pop)

#if defined(_WIN32) && !defined(__GNUC__)
typedef int64_t ssize_t;
#endif
#endif

static int32_t ReadSectionToMem (char *par_ExeFileName, char *par_Section, void **ret_Addr, uint64_t *ret_Size, uint64_t *ret_BaseAddr, int32_t par_ErrMsgFlag, int IsRealtimeProcess, int MachineType)
{
    if (!IsRealtimeProcess && ((MachineType == 0 /* Win32 */) || (MachineType == 1 /* Win64 */))) {
        IMAGE_NT_HEADERS32 NtHeader;
        IMAGE_NT_HEADERS64 NtHeader64;
        IMAGE_DOS_HEADER DosHeader;
        uint64_t BaseAddr;
        IMAGE_SECTION_HEADER SectionHeader;
        char SectionName[MAXIMUM_DWARF_NAME_SIZE]; 
        MY_FILE_HANDLE hFileExe;
        DWORD x, xx, i;

        hFileExe = my_open (par_ExeFileName);
        if (hFileExe == MY_INVALID_HANDLE_VALUE) {
            if (par_ErrMsgFlag) ThrowError (1, "Unable to open %s file\n", par_ExeFileName);
            return -1;
        }
        LogFileAccess (par_ExeFileName);
        my_read (hFileExe, &DosHeader, sizeof (DosHeader));
        if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
            my_close(hFileExe);
            if (par_ErrMsgFlag) ThrowError (1, "file %s in not an executable", par_ExeFileName);
            return -1;
        }
        my_lseek (hFileExe, DosHeader.e_lfanew);
        my_read (hFileExe, &NtHeader, sizeof (NtHeader));
        if (NtHeader.Signature != IMAGE_NT_SIGNATURE) {
            my_close (hFileExe);
            if (par_ErrMsgFlag) ThrowError (1, "file %s in not an executable", par_ExeFileName);
            return -1;
        }
        // is it a 64 Bit EXE/SYS?
        if (NtHeader.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
            my_lseek (hFileExe, (uint64_t)DosHeader.e_lfanew);
            my_read (hFileExe, &NtHeader64, sizeof (NtHeader64));
            BaseAddr = NtHeader64.OptionalHeader.ImageBase;
        } else {
            BaseAddr = NtHeader.OptionalHeader.ImageBase;
        }
        for (x = 0; x < NtHeader.FileHeader.NumberOfSections; x++) {
            my_read (hFileExe, &SectionHeader, sizeof (IMAGE_SECTION_HEADER));

            // GCC have section names longer than 8 bytes
            if (SectionHeader.Name[0] == '/') {
                uint64_t SaveFilePos;
                uint64_t VirtualOffset;
                char c;
                int32_t i;

                VirtualOffset = (uint64_t)NtHeader.FileHeader.PointerToSymbolTable +
                                (uint64_t)(NtHeader.FileHeader.NumberOfSymbols * SYMBOL_SIZE) +
                                (uint64_t)strtoull((char*)SectionHeader.Name+1, NULL, 0);
                SaveFilePos = my_ftell (hFileExe);
                my_lseek (hFileExe, VirtualOffset);
                for (i = 0; i < MAXIMUM_DWARF_NAME_SIZE; i++) {
                    my_read (hFileExe, &c, 1);
                    SectionName[i] = c;
                    if (c == 0) break;
                }
                my_lseek (hFileExe, SaveFilePos);
            } else {
                MEMCPY (SectionName, SectionHeader.Name, 8);
                SectionName[8] = 0; // Windows section namen are max. 8 chars long
            }
            if (!strlen (SectionName)) {
                continue;
                my_close (hFileExe);
                if (par_ErrMsgFlag) ThrowError (1, "section %s not found\n", par_Section);
                return -1;
            }
            //printf ("section %s\n", SectionName);
            if (!strcmp (SectionName, par_Section)) {
                //printf ("section %s found\n", SelectedSection);
                //Address = SectionHeader.VirtualAddress + NtHeader.OptionalHeader.ImageBase;
                //printf ("  section base address: 0x%08X\n", Address);
                //printf ("  size of section:      0x%08X\n", SectionHeader.Misc.VirtualSize);

                my_lseek (hFileExe, (uint64_t)SectionHeader.PointerToRawData);

                *ret_Addr = my_malloc (SectionHeader.Misc.VirtualSize);
                if (*ret_Addr == NULL) {
                    if (par_ErrMsgFlag) ThrowError (1, "cannot alloc %i bytes memory to read section %s of file %s",
                           SectionHeader.Misc.VirtualSize, par_Section, par_ExeFileName);
                    my_close (hFileExe);
                    return -1;
                }
                for (i = 0; i < SectionHeader.Misc.VirtualSize; i += xx) {
                    if ((xx = my_read (hFileExe, (unsigned char *)(*ret_Addr),
                        SectionHeader.Misc.VirtualSize - i)) == 0) {
                        my_close (hFileExe);
                        if (par_ErrMsgFlag) ThrowError (1, "cannot read from executable file %s", par_ExeFileName);
                        return -1;
                    }
                }
                *ret_Size = SectionHeader.Misc.VirtualSize;
                if (ret_BaseAddr != NULL) *ret_BaseAddr = BaseAddr;
                my_close (hFileExe);
                return 0;

            }
        }
        my_close (hFileExe);
        if (par_ErrMsgFlag) ThrowError (1, "error section \"%s\" not found inside executable \"%s\"\n", par_Section, par_ExeFileName);
        return -1;
    } else {
        MY_FILE_HANDLE fh;
        ssize_t ReadResult;
        Elf32_Ehdr Ehdr32;
        Elf64_Ehdr Ehdr64;
        unsigned int NumOfSections;
        unsigned int SectionNameStringTable;
        uint64_t SectionHeaderOffset;
        uint64_t StringBaseOffset;
        Elf32_Shdr Shdr32;
        Elf32_Shdr stShdr32;
        Elf64_Shdr Shdr64;
        Elf64_Shdr stShdr64;
        unsigned int i;
        int Is64Bit;

        fh = my_open(par_ExeFileName);
        if (fh == MY_INVALID_HANDLE_VALUE) {
            if (par_ErrMsgFlag) ThrowError (1, "Unable to open %s file\n", par_ExeFileName);
            return -1;
        }
        LogFileAccess (par_ExeFileName);
        ReadResult = my_read (fh, &Ehdr32, sizeof (Ehdr32));
        if (ReadResult != sizeof (Ehdr32)) {
            if (par_ErrMsgFlag) ThrowError (1, "cannot read from file %s", par_ExeFileName);
            return -1;
        }
        if ((Ehdr32.e_ident[0] != 0x7F) ||
            (Ehdr32.e_ident[1] != 'E') ||
            (Ehdr32.e_ident[2] != 'L') ||
            (Ehdr32.e_ident[3] != 'F')) {
            my_close (fh);
            if (par_ErrMsgFlag) ThrowError (1, "file %s is not an ELF executable", par_ExeFileName);
            return -1;
        }
        if ((Ehdr32.e_ident[4] != 2) &&
            (Ehdr32.e_ident[4] != 1)) {
            my_close (fh);
            if (par_ErrMsgFlag) ThrowError (1, "file %s is not 32/64 bit ELF executable", par_ExeFileName);
            return -1;
        }
        if (Ehdr32.e_ident[5] != 1) {
            my_close (fh);
            if (par_ErrMsgFlag) ThrowError (1, "file %s is not a lsb first ELF executable (msb first are not supported)", par_ExeFileName);
            return -1;
        }
        if (Ehdr32.e_ident[4] == 2) {
            my_lseek(fh, 0);
            ReadResult = my_read (fh, &Ehdr64, sizeof (Ehdr64));
            if (ReadResult != sizeof (Ehdr64)) {
                my_close (fh);
                ThrowError (1, "cannot read from file %s", par_ExeFileName);
                return -1;
            }
            NumOfSections = Ehdr64.e_shnum;
            SectionNameStringTable = Ehdr64.e_shstrndx;
            SectionHeaderOffset = Ehdr64.e_shoff;
            Is64Bit = 1;
        } else {
            NumOfSections = Ehdr32.e_shnum;
            SectionNameStringTable = Ehdr32.e_shstrndx;
            SectionHeaderOffset = Ehdr32.e_shoff;
            Is64Bit = 0;
        }
        if (Is64Bit) {
            my_lseek(fh, SectionHeaderOffset + SectionNameStringTable * sizeof(Shdr64));
            ReadResult = my_read (fh, &stShdr64, sizeof (stShdr64));
            if (ReadResult != sizeof (stShdr64)) {
                my_close (fh);
                if (par_ErrMsgFlag) ThrowError (1, "cannot read from file %s", par_ExeFileName);
                return -1;
            }
            StringBaseOffset = stShdr64.sh_offset;
        } else {
            my_lseek(fh, SectionHeaderOffset + SectionNameStringTable * sizeof(Shdr32));
            ReadResult = my_read (fh, &stShdr32, sizeof (stShdr32));
            if (ReadResult != sizeof (stShdr32)) {
                my_close (fh);
                if (par_ErrMsgFlag) ThrowError (1, "cannot read from file %s", par_ExeFileName);
                return -1;
            }
            StringBaseOffset = stShdr32.sh_offset;
        }
        for (i = 0; i < NumOfSections; i++) {
            char SectionName[256];
            char c;
            int x;

            if (Is64Bit) {
                uint64_t file_pos = SectionHeaderOffset + i * sizeof(Shdr64);
                my_lseek(fh, file_pos);
                ReadResult = my_read (fh, &Shdr64, sizeof (Shdr64));
                if (ReadResult != sizeof (Shdr64)) {
                    my_close (fh);
                    if (par_ErrMsgFlag) ThrowError (1, "cannot read from file %s", par_ExeFileName);
                    return -1;
                }
                my_lseek(fh, StringBaseOffset + Shdr64.sh_name);
            } else  {
                uint64_t file_pos = SectionHeaderOffset + i * sizeof(Shdr32);
                my_lseek(fh, file_pos);
                ReadResult = my_read (fh, &Shdr32, sizeof (Shdr32));
                if (ReadResult != sizeof (Shdr32)) {
                    my_close (fh);
                    if (par_ErrMsgFlag) ThrowError (1, "cannot read from file %s", par_ExeFileName);
                    return -1;
                }
                my_lseek(fh, StringBaseOffset + Shdr32.sh_name);
            }
            for(x = 0; x < (int)sizeof(SectionName); x++) {
                ReadResult = my_read(fh, &c, 1);
                if (ReadResult != 1) {
                    my_close (fh);
                    if (par_ErrMsgFlag) ThrowError (1, "cannot read from file %s", par_ExeFileName);
                    return -1;
                }
                SectionName[x] = c;
                if (c == 0) break;
            }
            if (!strcmp(SectionName, par_Section)) {
                uint64_t Size, b, bxx;
                if (Is64Bit) {
                    Size = Shdr64.sh_size;
                    my_lseek(fh, Shdr64.sh_offset);
                } else {
                    Size = Shdr32.sh_size;
                    my_lseek(fh, Shdr32.sh_offset);
                }
                *ret_Addr = my_malloc (Size);
                if (*ret_Addr == NULL) {
                    if (par_ErrMsgFlag) ThrowError (1, "cannot alloc %i bytes memory to read section %s of file %s",
                                               Size, par_Section, par_ExeFileName);
                    my_close (fh);
                    return -1;
                }
                for (b = 0; b < Size; b += bxx) {
                    ReadResult = my_read(fh, (unsigned char *)(*ret_Addr), (unsigned int)(Size - b));
                    if (ReadResult < 0) {
                        my_close(fh);
                        if (par_ErrMsgFlag) ThrowError (1, "cannot read from executable file %s", par_ExeFileName);
                        return -1;
                    }
                    bxx = (uint64_t)ReadResult;
                }
                *ret_Size = (uint32_t)Size;
                if (Is64Bit) {
                    if (ret_BaseAddr != NULL) *ret_BaseAddr = Shdr64.sh_addr;
                } else {
                    if (ret_BaseAddr != NULL) *ret_BaseAddr = Shdr32.sh_addr;
                }
                my_close (fh);
                return 0;

            }
        }
        my_close(fh);
        if (par_ErrMsgFlag) ThrowError (1, "error section \"%s\" not found inside executable \"%s\"\n", par_Section, par_ExeFileName);
        return -1;
    }
}

#define byte_get byte_get_little_endian
uint64_t byte_get_little_endian (unsigned char *field, uint32_t size)
{
    switch (size) {
    case 1:
        return *field;
    case 2:
        return   ((uint64_t) (field[0]))
               | (((uint64_t) (field[1])) << 8);
    case 3:
        return   ((uint64_t) (field[0]))
               | (((uint64_t) (field[1])) << 8)
               | (((uint64_t) (field[2])) << 16);
    case 4:
        return   ((uint64_t) (field[0]))
               | (((uint64_t) (field[1])) << 8)
               | (((uint64_t) (field[2])) << 16)
               | (((uint64_t) (field[3])) << 24);
    case 8:
        return  ((uint64_t) (field[0]))
              | (((uint64_t) (field[1])) << 8)
              | (((uint64_t) (field[2])) << 16)
              | (((uint64_t) (field[3])) << 24)
              | (((uint64_t) (field[4])) << 32)
              | (((uint64_t) (field[5])) << 40)
              | (((uint64_t) (field[6])) << 48)
              | (((uint64_t) (field[7])) << 56);
    default:
        ThrowError (1, "Unhandled data length: %d", size);
        return 0;
    }
}

uint64_t byte_get_big_endian (unsigned char *field, int32_t size)
{
    switch (size) {
        case 1:
        return *field;
    case 2:
        return ((uint64_t) (field[1])) | (((uint64_t) (field[0])) << 8);
    case 3:
        return     ((uint64_t) (field[2]))
               |   (((uint64_t) (field[1])) << 8)
               |   (((uint64_t) (field[0])) << 16);
    case 4:
        return     ((uint64_t) (field[3]))
               |   (((uint64_t) (field[2])) << 8)
               |   (((uint64_t) (field[1])) << 16)
               |   (((uint64_t) (field[0])) << 24);
    case 8:
        return ((uint64_t) (field[7]))
               |   (((uint64_t) (field[6])) << 8)
               |   (((uint64_t) (field[5])) << 16)
               |   (((uint64_t) (field[4])) << 24)
               |   (((uint64_t) (field[3])) << 32)
               |   (((uint64_t) (field[2])) << 40)
               |   (((uint64_t) (field[1])) << 48)
               |   (((uint64_t) (field[0])) << 56);
    default:
        ThrowError (1, "Unhandled data length: %d", size);
        return 0;
    }
}

int64_t byte_get_signed (unsigned char *field, uint32_t size)
{
    int64_t x = byte_get (field, size);

    switch (size) {
    case 1:
        return (x ^ 0x80) - 0x80;
    case 2:
        return (x ^ 0x8000) - 0x8000;
    case 4:
        return (x ^ 0x80000000) - 0x80000000;
    case 8:
        return (x ^ 0x8000000000000000ULL) - 0x8000000000000000ULL;
    default:
        ThrowError (1, "size %i not equal 1,2,4,8", size);
    }
    return 0;
}

static uint64_t read_leb128 (unsigned char *data, uint32_t *length_return, int32_t sign)
{
    uint64_t result = 0;
    uint32_t num_read = 0;
    uint32_t shift = 0;
    unsigned char byte;

    do {
        byte = *data++;
        num_read++;
        result |= ((uint64_t) (byte & 0x7f)) << shift;
        shift += 7;
    } while (byte & 0x80);

    if (length_return != NULL)
    *length_return = num_read;

    if (sign && (shift < 8 * sizeof (result)) && (byte & 0x40))
    result |= UINT64_MAX << shift;
    return result;
}

static int64_t read_sleb128 (unsigned char *data, uint32_t *length_return)
{
    return (int64_t)read_leb128 (data, length_return, 1);
}

static uint64_t read_uleb128 (unsigned char *data, uint32_t *length_return)
{
    return read_leb128 (data, length_return, 0);
}

static int read_uleb128_to_uint64 (const unsigned char *data, const unsigned char *data_end,
                                   uint64_t *ret_value)
{
    const unsigned char *p = data;
    unsigned int shift = 0;
    unsigned long long result = 0;
    unsigned char byte;

    while (1) {
        if (p >= data_end) {
            return 0;
        }
        byte = *p++;
        result |= ((uint64_t)(byte & 0x7f)) << shift;
        if ((byte & 0x80) == 0) {
            break;   // while
        }
        shift += 7;
    }
    *ret_value = result;
    return (int)(p - data);
}

static const char * fetch_indirect_string (uint32_t offset, DEBUG_SECTIONS *ds)
{
    if (ds->debug_str == NULL) {
        return "<no .debug_str section>";
    }
    if (offset > ds->debug_str_len) {
        ThrowError (1, "DW_FORM_strp offset too big: %u", offset);
        return "<error offset is too big>";
    }
    if (ds->debug_str == NULL) {
        ThrowError (1, "Internal error: using none exiting \".debug_str\" section");
        return NULL;
    }
    return (const char *)ds->debug_str + offset;
}

static const char * fetch_indirect_string_from_line_strp (uint32_t offset, DEBUG_SECTIONS *ds)
{
    if (ds->debug_str == NULL) {
        return "<no .debug_str section>";
    }
    if (offset > ds->debug_line_str_len) {
        ThrowError (1, "DW_FORM_line_strp offset too big: %u", offset);
        return "<error offset is too big>";
    }
    if (ds->debug_line_str == NULL) {
        ThrowError (1, "Internal error: using none exiting \".debug_str\" section");
        return NULL;
    }
    return (const char *)ds->debug_line_str + offset;
}


/* begin Namespace */

void ResetNameSpace (DEBUG_SECTIONS *ds)
{
    ds->PosOfLastCharInNameSpace = 0;
    ds->CounterNameSpace = 0;
}

int AddNameSpace (char *NameSpace, DEBUG_SECTIONS *ds)
{
    uint32_t Len = (uint32_t)strlen (NameSpace);  //namespace
    if (ds->CounterNameSpace >= 128) {
        ThrowError (1, "more than 127 nested namespaces not possible");
        return -1;
    }
    if ((Len + ds->PosOfLastCharInNameSpace) >= (sizeof (ds->EntireNameSpace) - 3)) {
        ThrowError (1, "namespace larger than %i chars", sizeof (ds->EntireNameSpace));
        return -1;
    } else {
        ds->BeginingOfNamenSpaces[ds->CounterNameSpace] = ds->PosOfLastCharInNameSpace;
        MEMCPY (ds->EntireNameSpace + ds->PosOfLastCharInNameSpace, NameSpace, Len);
        ds->PosOfLastCharInNameSpace += Len;
        MEMCPY (ds->EntireNameSpace + ds->PosOfLastCharInNameSpace, "::", 2);
        ds->PosOfLastCharInNameSpace += 2;
        ds->EntireNameSpace[ds->PosOfLastCharInNameSpace] = 0;
        ds->CounterNameSpace++;
    }
    return 0;
}

int RemoveNameSpace (DEBUG_SECTIONS *ds)
{
    if ((ds->CounterNameSpace <= 0) ||
        (ds->PosOfLastCharInNameSpace < 3) ||
        (ds->EntireNameSpace[ds->PosOfLastCharInNameSpace-1] != ':') ||
        (ds->EntireNameSpace[ds->PosOfLastCharInNameSpace-2] != ':')) {
        ThrowError (1, "internal namescape wrong!");
        return -1;
    }
    ds->CounterNameSpace--;
    ds->PosOfLastCharInNameSpace = ds->BeginingOfNamenSpaces[ds->CounterNameSpace];
    ds->EntireNameSpace[ds->PosOfLastCharInNameSpace] = 0;
    return 0;
}

char *ExtendWithNameSpace (char *Name, DEBUG_SECTIONS *ds)
{
    StringCopyMaxCharTruncate (ds->EntireNameSpace + ds->PosOfLastCharInNameSpace, Name, sizeof(ds->EntireNameSpace) - ds->PosOfLastCharInNameSpace);
    return ds->EntireNameSpace;
}
// End Namespace

/* begin Classname */

void ResetClassName (DEBUG_SECTIONS *ds)
{
    ds->PosOfLastCharInClassName = 0;
    ds->CounterClassName = 0;
}

int32_t AddClassName (char *ClassName, DEBUG_SECTIONS *ds)
{
    uint32_t Len = (uint32_t)strlen (ClassName);  //ClassName
    if (ds->CounterClassName >= 128) {
        ThrowError (1, "more than 127 nested classes not possible");
        return -1;
    }
    if ((Len + ds->PosOfLastCharInClassName) >= (sizeof (ds->EntireClassName) - 3)) {
        ThrowError (1, "ClassName larger than %i chars", sizeof (ds->EntireClassName));
        return -1;
    } else {
        ds->BeginingOfClassNamens[ds->CounterClassName] = ds->PosOfLastCharInClassName;
        MEMCPY (ds->EntireClassName + ds->PosOfLastCharInClassName, ClassName, Len);
        ds->PosOfLastCharInClassName += Len;
        MEMCPY (ds->EntireClassName + ds->PosOfLastCharInClassName, "::", 2);
        ds->PosOfLastCharInClassName += 2;
        ds->EntireClassName[ds->PosOfLastCharInClassName] = 0;
        ds->CounterClassName++;
    }
    return 0;
}

int32_t RemoveClassName (DEBUG_SECTIONS *ds)
{
    if ((ds->CounterClassName <= 0) ||
        (ds->PosOfLastCharInClassName < 3) ||
        (ds->EntireClassName[ds->PosOfLastCharInClassName-1] != ':') ||
        (ds->EntireClassName[ds->PosOfLastCharInClassName-2] != ':')) {
        ThrowError (1, "internal class name wrong!");
        return -1;
    }
    ds->CounterClassName--;
    ds->PosOfLastCharInClassName = ds->BeginingOfClassNamens[ds->CounterClassName];
    ds->EntireClassName[ds->PosOfLastCharInClassName] = 0;
    return 0;
}

char *ExtendWithClassName (char *Name, DEBUG_SECTIONS *ds)
{
    StringCopyMaxCharTruncate (ds->EntireClassName + ds->PosOfLastCharInClassName, Name, sizeof(ds->EntireClassName) + ds->PosOfLastCharInClassName);
    return ds->EntireClassName;
}
// End ClassName


static void SetCurrentCompilationUnitName (char *Name, DEBUG_SECTIONS *ds)
{
    char *d, *p = Name;

    while (*p != 0) p++;
    while ((*p != '\\') && (p > Name)) p--;
    if (*p == '\\') p++;
    d = ds->CurrentCompilationUnitName;
    while ((*p != 0) && (*p != '.')) {
        *d++ = *p++;
    }
    *d++ = ':';
    *d++ = ':';
    *d = 0;
    ds->PosOfLastCharInCurrentCompilationUnitName = (uint32_t)(d - ds->CurrentCompilationUnitName);
}

static char *ExtendWithCurrentCompilationUnit (char *Name, DEBUG_SECTIONS *ds)
{
    StringCopyMaxCharTruncate (ds->CurrentCompilationUnitName + ds->PosOfLastCharInCurrentCompilationUnitName, Name,
                               sizeof (ds->CurrentCompilationUnitName) - ds->PosOfLastCharInCurrentCompilationUnitName);
    return ds->CurrentCompilationUnitName;
}

char *GetDemangledSymbolName (char *Name, DEBUG_SECTIONS *ds)
{
    char *p = Name;
    if (*p++ == '_') {
        if (*p++ == 'Z') {
            if (*p++ == 'N') {
                char *pn, *d;
                uint32_t l;
                uint32_t x;

                d = ds->DemangledSymbolNameBuffer;
                while (1) {
                    l = strtoul (p, &pn, 10);
                    if ((p == pn) || (*p == 0)) goto __NOT_DEMANGLED;
                    p = pn;
                    for (x = 0; x < l; x++) {
                        if (*p == 0) goto __NOT_DEMANGLED;
                        *d++ = *p++;
                    }
                    if (*p == 'E') {
                        *d = 0;
                        return ds->DemangledSymbolNameBuffer;
                    }
                    *d++ = ':';
                    *d++ = ':';
                }
            }
        }
    }
__NOT_DEMANGLED:
    //ThrowError (1, "symbol \"%s\" are not demageled", Name);
    return Name;
}


static int32_t AddStructDeclaration (int32_t TypeNr, int32_t FieldTypeNr, char *Name, DEBUG_SECTIONS *ds)
{
    if ((ds->ListOfStructDeclarations == NULL) || (ds->PosInListOfStructDeclarations >= ds->SizeOfListOfStructDeclarations)) {
        ds->SizeOfListOfStructDeclarations += 1024;
        ds->ListOfStructDeclarations = (LIST_OF_STRUCT_DECLARATION_ELEM*)my_realloc (ds->ListOfStructDeclarations, sizeof (LIST_OF_STRUCT_DECLARATION_ELEM) * (size_t)ds->SizeOfListOfStructDeclarations);
        if (ds->ListOfStructDeclarations == NULL) {
            ThrowError (1, "out of memory");
            return -1;
        }
    }
    ds->ListOfStructDeclarations[ds->PosInListOfStructDeclarations].TypeNr = TypeNr;
    ds->ListOfStructDeclarations[ds->PosInListOfStructDeclarations].FieldTypeNr = FieldTypeNr;
    ds->ListOfStructDeclarations[ds->PosInListOfStructDeclarations].Name = insert_symbol (Name, ds->pappldata);
    ds->PosInListOfStructDeclarations++;
    return 0;
}

static int32_t SpecificationOfStruct (int32_t TypeNr, char **ret_Name, DEBUG_SECTIONS *ds)
{
    int32_t x;

    if (ds->ListOfStructDeclarations == NULL) return 0;
    for (x = 0; x < ds->PosInListOfStructDeclarations; x++) {
        if (TypeNr == ds->ListOfStructDeclarations[x].TypeNr) {
            if (ret_Name != NULL) {
                *ret_Name = ds->ListOfStructDeclarations[x].Name;
            }
            return ds->ListOfStructDeclarations[x].FieldTypeNr;
        }
    }
    return 0;
}

static void CleanListOfStructDeclarations (DEBUG_SECTIONS *ds)
{
    if (ds->ListOfStructDeclarations == NULL) return;
    my_free (ds->ListOfStructDeclarations);
    ds->ListOfStructDeclarations = NULL;
}

static uint64_t DecodeLocationExpression (unsigned char *data,
                                          uint32_t pointer_size,
                                          uint32_t offset_size,
                                          int dwarf_version,
                                          uint32_t length,  // Block length
                                          uint32_t cu_offset)
{
    unsigned op;
    uint32_t bytes_read;
    uint64_t uvalue = 0;
    unsigned char *end = data + length;
    uint64_t need_frame_base = 0;

    while (data < end) {
        op = *data++;
        switch (op) {
        case DW_OP_addr:
            uvalue = byte_get (data, pointer_size);
            DEBUGFPRINTF (DebugOut, "DW_OP_addr: %" PRIx64 "", uvalue);
            data += pointer_size;
            break;
        case DW_OP_deref:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_deref");
            break;
        case DW_OP_const1u:
            uvalue = byte_get (data++, 1);
            DEBUGFPRINTF (DebugOut, "DW_OP_const1u: %" PRIu64 "", uvalue);
            break;
        case DW_OP_const1s:
            uvalue = (uint64_t)byte_get_signed (data++, 1);
            DEBUGFPRINTF (DebugOut, "DW_OP_const1s: %" PRId64 "", uvalue);
            break;
        case DW_OP_const2u:
            uvalue = byte_get (data, 2);
            DEBUGFPRINTF (DebugOut, "DW_OP_const2u: %" PRIu64 "", uvalue);
            data += 2;
            break;
        case DW_OP_const2s:
            uvalue = (uint64_t)byte_get_signed (data, 2);
            DEBUGFPRINTF (DebugOut, "DW_OP_const2s: %" PRId64 "", uvalue);
            data += 2;
            break;
        case DW_OP_const4u:
            uvalue = byte_get (data, 4);
            DEBUGFPRINTF (DebugOut, "DW_OP_const4u: %" PRIu64 "", uvalue);
            data += 4;
            break;
        case DW_OP_const4s:
            uvalue = (uint64_t)byte_get_signed (data, 4);
            DEBUGFPRINTF (DebugOut, "DW_OP_const4s: %" PRId64 "", uvalue);
            data += 4;
            break;
        case DW_OP_const8u:
            uvalue = (uint64_t)byte_get (data + 4, 4) << 32 | (uint64_t)byte_get (data, 4);
            DEBUGFPRINTF (DebugOut, "DW_OP_const8u: %" PRIu64 "", uvalue);
            data += 8;
        break;
        case DW_OP_const8s:
            uvalue = (uint64_t)byte_get (data + 4, 4) << 32 | (uint64_t)byte_get (data, 4);
            DEBUGFPRINTF (DebugOut, "DW_OP_const8s: %" PRId64 "", (int64_t)uvalue);
            data += 8;
            break;
        case DW_OP_constu:
            uvalue = read_uleb128 (data, &bytes_read);
            DEBUGFPRINTF (DebugOut, "DW_OP_constu: %" PRIX64 "", uvalue);
            data += bytes_read;
            break;
        case DW_OP_consts:
            /*value = (uint64_t)*/read_sleb128 (data, &bytes_read);
            DEBUGFPRINTF (DebugOut, "DW_OP_consts: %" PRIi64 "", (int64_t)uvalue);
            data += bytes_read;
            break;
        case DW_OP_dup:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_dup");
            break;
        case DW_OP_drop:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_drop");
            break;
        case DW_OP_over:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_over");
            break;
        case DW_OP_pick:
            uvalue = byte_get (data++, 1);
            DEBUGFPRINTF (DebugOut, "DW_OP_pick: %" PRId64 "", uvalue);
            break;
        case DW_OP_swap:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_swap");
            break;
        case DW_OP_rot:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_rot");
            break;
        case DW_OP_xderef:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_xderef");
            break;
        case DW_OP_abs:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_abs");
            break;
        case DW_OP_and:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_and");
            break;
        case DW_OP_div:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_div");
            break;
        case DW_OP_minus:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_minus");
            break;
        case DW_OP_mod:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_mod");
            break;
        case DW_OP_mul:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_mul");
            break;
        case DW_OP_neg:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_neg");
            break;
        case DW_OP_not:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_not");
            break;
        case DW_OP_or:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_or");
            break;
        case DW_OP_plus:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_plus");
            break;
        case DW_OP_plus_uconst:
            uvalue = read_uleb128 (data, &bytes_read);
            DEBUGFPRINTF (DebugOut, "DW_OP_plus_uconst: %" PRIX64 "", uvalue);
            data += bytes_read;
            break;
        case DW_OP_shl:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_shl");
            break;
        case DW_OP_shr:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_shr");
            break;
        case DW_OP_shra:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_shra");
            break;
        case DW_OP_xor:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_xor");
            break;
        case DW_OP_bra:
            uvalue = (uint64_t)byte_get_signed (data, 2);
            DEBUGFPRINTF (DebugOut, "DW_OP_bra: %" PRId64 "", uvalue);
            data += 2;
            break;
        case DW_OP_eq:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_eq");
            break;
        case DW_OP_ge:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_ge");
            break;
        case DW_OP_gt:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_gt");
            break;
        case DW_OP_le:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_le");
            break;
        case DW_OP_lt:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_lt");
            break;
        case DW_OP_ne:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_ne");
            break;
        case DW_OP_skip:
            uvalue = (uint64_t)byte_get_signed (data, 2);
            DEBUGFPRINTF (DebugOut, "DW_OP_skip: %" PRId64 "", uvalue);
            data += 2;
            break;

        case DW_OP_lit0:
        case DW_OP_lit1:
        case DW_OP_lit2:
        case DW_OP_lit3:
        case DW_OP_lit4:
        case DW_OP_lit5:
        case DW_OP_lit6:
        case DW_OP_lit7:
        case DW_OP_lit8:
        case DW_OP_lit9:
        case DW_OP_lit10:
        case DW_OP_lit11:
        case DW_OP_lit12:
        case DW_OP_lit13:
        case DW_OP_lit14:
        case DW_OP_lit15:
        case DW_OP_lit16:
        case DW_OP_lit17:
        case DW_OP_lit18:
        case DW_OP_lit19:
        case DW_OP_lit20:
        case DW_OP_lit21:
        case DW_OP_lit22:
        case DW_OP_lit23:
        case DW_OP_lit24:
        case DW_OP_lit25:
        case DW_OP_lit26:
        case DW_OP_lit27:
        case DW_OP_lit28:
        case DW_OP_lit29:
        case DW_OP_lit30:
        case DW_OP_lit31:
            DEBUGFPRINTF (DebugOut, "DW_OP_lit%d", op - DW_OP_lit0);
            break;

        case DW_OP_reg0:
        case DW_OP_reg1:
        case DW_OP_reg2:
        case DW_OP_reg3:
        case DW_OP_reg4:
        case DW_OP_reg5:
        case DW_OP_reg6:
        case DW_OP_reg7:
        case DW_OP_reg8:
        case DW_OP_reg9:
        case DW_OP_reg10:
        case DW_OP_reg11:
        case DW_OP_reg12:
        case DW_OP_reg13:
        case DW_OP_reg14:
        case DW_OP_reg15:
        case DW_OP_reg16:
        case DW_OP_reg17:
        case DW_OP_reg18:
        case DW_OP_reg19:
        case DW_OP_reg20:
        case DW_OP_reg21:
        case DW_OP_reg22:
        case DW_OP_reg23:
        case DW_OP_reg24:
        case DW_OP_reg25:
        case DW_OP_reg26:
        case DW_OP_reg27:
        case DW_OP_reg28:
        case DW_OP_reg29:
        case DW_OP_reg30:
        case DW_OP_reg31:
            DEBUGFPRINTF (DebugOut, "DW_OP_reg%d", op - DW_OP_reg0);
            break;

        case DW_OP_breg0:
        case DW_OP_breg1:
        case DW_OP_breg2:
        case DW_OP_breg3:
        case DW_OP_breg4:
        case DW_OP_breg5:
        case DW_OP_breg6:
        case DW_OP_breg7:
        case DW_OP_breg8:
        case DW_OP_breg9:
        case DW_OP_breg10:
        case DW_OP_breg11:
        case DW_OP_breg12:
        case DW_OP_breg13:
        case DW_OP_breg14:
        case DW_OP_breg15:
        case DW_OP_breg16:
        case DW_OP_breg17:
        case DW_OP_breg18:
        case DW_OP_breg19:
        case DW_OP_breg20:
        case DW_OP_breg21:
        case DW_OP_breg22:
        case DW_OP_breg23:
        case DW_OP_breg24:
        case DW_OP_breg25:
        case DW_OP_breg26:
        case DW_OP_breg27:
        case DW_OP_breg28:
        case DW_OP_breg29:
        case DW_OP_breg30:
        case DW_OP_breg31:
            /*value =*/ read_sleb128 (data, &bytes_read);
            DEBUGFPRINTF (DebugOut, "DW_OP_breg%d", op - DW_OP_breg0);
            data += bytes_read;
            break;

        case DW_OP_regx:
            uvalue = read_uleb128 (data, &bytes_read);
            data += bytes_read;
            DEBUGFPRINTF (DebugOut, "DW_OP_regx: %" PRIu64 "", uvalue);
            break;
        case DW_OP_fbreg:
            need_frame_base = 1;
            uvalue = (uint64_t)read_sleb128 (data, &bytes_read);
            DEBUGFPRINTF (DebugOut, "DW_OP_fbreg: %" PRIi64 "", uvalue);
            data += bytes_read;
            break;
        case DW_OP_bregx:
            uvalue = read_uleb128 (data, &bytes_read);
            data += bytes_read;
            DEBUGFPRINTF (DebugOut, "DW_OP_bregx: %" PRIu64 " %" PRIi64 "", uvalue, read_sleb128 (data, &bytes_read));
            data += bytes_read;
            break;
        case DW_OP_piece:
            uvalue = read_uleb128 (data, &bytes_read);
            DEBUGFPRINTF (DebugOut, "DW_OP_piece: %" PRIu64 "", uvalue);
            data += bytes_read;
            break;
        case DW_OP_deref_size:
            uvalue = byte_get (data++, 1);
            DEBUGFPRINTF (DebugOut, "DW_OP_deref_size: %" PRId64 "", uvalue);
            break;
        case DW_OP_xderef_size:
            uvalue = byte_get (data++, 1);
            DEBUGFPRINTF (DebugOut, "DW_OP_xderef_size: %" PRId64 "", uvalue);
            break;
        case DW_OP_nop:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_nop");
            break;

        /* DWARF 3 extensions.  */
        case DW_OP_push_object_address:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_push_object_address");
            break;
        case DW_OP_call2:
            uvalue = byte_get (data, 2) + cu_offset;
            DEBUGFPRINTF (DebugOut, "DW_OP_call2: <0x%" PRIX64 ">", uvalue);
            data += 2;
            break;
        case DW_OP_call4:
            uvalue = byte_get (data, 4) + cu_offset;
            DEBUGFPRINTF (DebugOut, "DW_OP_call4: <0x%" PRIX64 ">", uvalue);
            data += 4;
            break;
        case DW_OP_call_ref:
            if (dwarf_version == -1) {
                DEBUGFPRINTF0 (DebugOut, "(DW_OP_call_ref in frame info)");
                return need_frame_base;
            }
            if (dwarf_version == 2) {
                uvalue = byte_get (data, pointer_size);
                DEBUGFPRINTF (DebugOut, "DW_OP_call_ref: <0x%" PRIX64 ">", uvalue);
                data += pointer_size;
            } else {
                uvalue = byte_get (data, pointer_size);
                DEBUGFPRINTF (DebugOut, "DW_OP_call_ref: <0x%" PRIX64 ">", uvalue);
                data += offset_size;
            }
            break;
        case DW_OP_form_tls_address:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_form_tls_address");
            break;
        case DW_OP_call_frame_cfa:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_call_frame_cfa");
            break;
        case DW_OP_bit_piece:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_bit_piece: ");
            uvalue = read_leb128 (data, &bytes_read, 0);
            DEBUGFPRINTF (DebugOut, "size: %" PRIu64 " ", uvalue);
            data += bytes_read;
            DEBUGFPRINTF (DebugOut, "offset: %" PRIu64 " ", read_uleb128 (data, &bytes_read));
            data += bytes_read;
            break;

        /* DWARF 4 extensions.  */
        case DW_OP_stack_value:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_stack_value");
            break;

        case DW_OP_implicit_value:
            DEBUGFPRINTF0 (DebugOut, "DW_OP_implicit_value");
            uvalue = read_uleb128 (data, &bytes_read);
            data += bytes_read;
            //display_block (data, uvalue);
            data += uvalue;
            break;

        default:
            if ((op >= DW_OP_lo_user) && (op <= DW_OP_hi_user)) {
                DEBUGFPRINTF0 (DebugOut, "(User defined location op)");
            } else {
                DEBUGFPRINTF0 (DebugOut, "(Unknown location op)");
            }
            return need_frame_base;
        }

        /* Separate the ops.  */
        if (data < end) {
            DEBUGFPRINTF0 (DebugOut, "; ");
        }
    }

    return uvalue;
}

static unsigned char *ReadAattrValue (uint32_t attribute,
                                      uint32_t form,
                                      unsigned char *data,
                                      uint32_t cu_offset,
                                      DEBUG_SECTIONS *ds,
                                      uint64_t *ret_AttribValue,
                                      unsigned char **AbbrevPtr)
{
    uint64_t uvalue = 0;
    unsigned char *block_start = NULL;
    uint32_t bytes_read;

    switch (form) {
    default:
        break;
    case DW_FORM_ref_addr:
        if (ds->dwarf_version == 2) {
            uvalue = byte_get (data, ds->pointer_size);
            data += ds->pointer_size;
        } else if (ds->dwarf_version == 3 || ds->dwarf_version == 4) {
            uvalue = byte_get (data, ds->offset_size);
            data += ds->offset_size;
        } else {
            ThrowError (1, "Internal error: DWARF version is not 2, 3 or 4");
        }
        break;
    case DW_FORM_addr:
        uvalue = byte_get (data, ds->pointer_size);
        data += ds->pointer_size;
        break;
    case DW_FORM_strp:
    //case DW_FORM_sec_offset:
    case DW_FORM_line_strp:  // Dwarf5
        uvalue = byte_get (data, ds->offset_size);
        data += ds->offset_size;
        break;
    case DW_FORM_flag_present:
        uvalue = 1;
        break;
    case DW_FORM_ref1:
    case DW_FORM_flag:
    case DW_FORM_data1:
        uvalue = byte_get (data, 1);
        data += 1;
        break;
    case DW_FORM_ref2:
    case DW_FORM_data2:
        uvalue = byte_get (data, 2);
        data += 2;
        break;
    case DW_FORM_ref4:
    case DW_FORM_data4:
        uvalue = byte_get (data, 4);
        data += 4;
        break;
    case DW_FORM_sdata:
        /*value =*/ read_sleb128 (data, &bytes_read);
        data += bytes_read;
        break;
/*    case DW_FORM_GNU_str_index:
        uvalue = read_leb128 (data, &bytes_read, 0);
        data += bytes_read;
        break;*/
    case DW_FORM_ref_udata:
    case DW_FORM_udata:
        uvalue = read_uleb128 (data, &bytes_read);
        data += bytes_read;
        break;
    case DW_FORM_indirect:
        form = (uint32_t)read_uleb128 (data, &bytes_read);
        data += bytes_read;
        return ReadAattrValue (attribute, form, data, cu_offset, ds, ret_AttribValue, AbbrevPtr);
        // no break neeeded
    /*case DW_FORM_GNU_addr_index:
        uvalue = read_leb128 (data, &bytes_read, 0);
        data += bytes_read;
        break;*/
    case DW_FORM_sec_offset:
        if (ds->Is64BitFormat) {
            uvalue = byte_get (data, 8);
            data += 8;
        } else {
            uvalue = byte_get (data, 4);
            data += 4;
        }
        break;
    case DW_FORM_implicit_const:
        uvalue = read_sleb128 (*AbbrevPtr, &bytes_read);
        *AbbrevPtr += bytes_read;
        break;
    }

    switch (form) {
    case DW_FORM_ref_addr:
        DEBUGFPRINTF (DebugOut, " <0x%" PRIX64 ">", uvalue);
        break;
    /*case DW_FORM_GNU_ref_alt:
        DEBUGFPRINTF (DebugOut, " <alt 0x%" PRIX64 ">", uvalue);
        break;*/
    case DW_FORM_ref1:
    case DW_FORM_ref2:
    case DW_FORM_ref4:
    case DW_FORM_ref_udata:
        DEBUGFPRINTF (DebugOut, " <0x%" PRIX64 ">", uvalue + cu_offset);
        uvalue += cu_offset;  // ref1,2,4 sind immer relativ zur Compalation Unit
        break;
    case DW_FORM_data4:
    case DW_FORM_addr:
    case DW_FORM_sec_offset:
        DEBUGFPRINTF (DebugOut, " 0x%" PRIX64 "", uvalue);
        break;
    case DW_FORM_flag_present:
    case DW_FORM_flag:
    case DW_FORM_data1:
    case DW_FORM_data2:
    case DW_FORM_sdata:
    case DW_FORM_udata:
        DEBUGFPRINTF (DebugOut, " %" PRId64 "", uvalue);
        break;
    case DW_FORM_ref8:
    case DW_FORM_data8:
        uvalue = byte_get (data, 8);
        DEBUGFPRINTF (DebugOut, " 0x%" PRIX64 "", uvalue);
        data += 8;
        break;
    case DW_FORM_string:
        uvalue = (uint64_t)data;
        DEBUGFPRINTF (DebugOut, " %s", data);
        data += strlen ((char *) data) + 1;
        break;
    case DW_FORM_block:
    case DW_FORM_exprloc:
        uvalue = read_uleb128 (data, &bytes_read);
        block_start = data + bytes_read;
        data = block_start + uvalue;
        break;
    case DW_FORM_block1:
        uvalue = byte_get (data, 1);
        block_start = data + 1;
        data = block_start + uvalue;
        break;
    case DW_FORM_block2:
        uvalue = byte_get (data, 2);
        block_start = data + 2;
        data = block_start + uvalue;
        break;
    case DW_FORM_block4:
        uvalue = byte_get (data, 4);
        block_start = data + 4;
        data = block_start + uvalue;
        break;
    case DW_FORM_strp:
        DEBUGFPRINTF (DebugOut, " (indirect string, offset: 0x%" PRIX64 "): %s", uvalue,
                                       fetch_indirect_string ((uint32_t)uvalue, ds));
        uvalue = (uint64_t)(const void*)fetch_indirect_string ((uint32_t)uvalue, ds);
        break;
    /*case DW_FORM_GNU_str_index:
        ThrowError (1, "dont know how to handle DW_FORM_GNU_str_index");
        break;*/
    /*case DW_FORM_GNU_strp_alt:
        DEBUGFPRINTF (DebugOut, " (alt indirect string, offset: 0x%" PRIX64 ")", uvalue);
        break;*/
    case DW_FORM_indirect:
        /* Handled above.  */
        break;
    case DW_FORM_ref_sig8:
        uvalue = byte_get (data, 8);
        DEBUGFPRINTF (DebugOut, " signature: 0x%" PRIX64 "", uvalue);
        data += 8;
        break;
   /* case DW_FORM_GNU_addr_index:
        ThrowError (1, "dont know how to handle DW_FORM_GNU_addr_index");
        break;*/
    case DW_FORM_line_strp:
        DEBUGFPRINTF (DebugOut, " (indirect string, offset: 0x%" PRIX64 "): %s", uvalue,
                                       fetch_indirect_string_from_line_strp ((uint32_t)uvalue, ds));
        uvalue = (uint64_t)(const void*)fetch_indirect_string_from_line_strp ((uint32_t)uvalue, ds);
        break;
    case DW_FORM_implicit_const:
        DEBUGFPRINTF (DebugOut, " %" PRId64 "", uvalue);
        break;
    default:
        ThrowError (1, "Unrecognized form: %lu", form);
        break;
    }

    if (block_start != NULL) {
        switch (attribute) {
        case DW_AT_location:
        case DW_AT_data_member_location:
            uvalue = DecodeLocationExpression (block_start,
                                               ds->pointer_size,
                                               ds->offset_size,
                                               ds->dwarf_version,
                                               (uint32_t)uvalue,  // Block length
                                               cu_offset);
            break;
        }
    }
    *ret_AttribValue = uvalue;
    return data;
}


char *GetDwAtName (uint32_t Idx)
{
    if (Idx >= (int32_t)DW_AT_NameTableSize) {
        int32_t x;
        for (x = 0; x < (int32_t)DW_AT_NameTableUserSize; x++) {
            if (DW_AT_NameTableUser[x].Code == Idx) {
                return DW_AT_NameTableUser[x].Name;
            }
        }
        return "out of table";
    } else {
        return DW_AT_NameTable[Idx].Name;
    }
}

char *GetDwFormName (uint32_t Idx)
{
    if (Idx >= (int32_t)DW_FORM_NameTableSize) {
        return "out of table";
    } else {
        return DW_FORM_NameTable[Idx].Name;
    }
}

char *GetDwTagName (uint32_t Idx)
{
    if (Idx >= (int32_t)DW_TAG_NameTableSize) {
        return "out of table";
    } else {
        return DW_TAG_NameTable[Idx].Name;
    }
}

typedef struct {
    uint32_t AbbrevCode;  // Code 0 doesn't exist that is the size of the table!
    union {
        uint32_t Size;
        unsigned char *Ptr;   // and this ist the real nummer of entrys in table
    } Start;
} ABBREV_CODE_TABLE_ENTRY;

ABBREV_CODE_TABLE_ENTRY *ParseAbbrevForOneCompileUnit (DEBUG_SECTIONS *ds,
                                                       uint32_t AbbrevOffset, ABBREV_CODE_TABLE_ENTRY *Old)
{
    ABBREV_CODE_TABLE_ENTRY *Ret;
    uint32_t AttribNum, EntryNum;
    unsigned char *AbbrevStart, *AbbrevEnd, *Ptr, *EntryPtr;
    uint64_t Help64;
    uint32_t AbbrevEntryNr, TagType, Children, AttribName, AttribForm;

    AbbrevStart = (unsigned char*)ds->debug_abbrev + AbbrevOffset;
    AbbrevEnd = (unsigned char*)ds->debug_abbrev + ds->debug_abbrev_len;

    if (Old == NULL) {
        Ret = (ABBREV_CODE_TABLE_ENTRY *)my_malloc (sizeof (ABBREV_CODE_TABLE_ENTRY) * 16);
        if (Ret == NULL) return NULL;
        Ret->AbbrevCode = 16;
        Ret->Start.Size = 0;
    } else {
        Ret = Old;
    }
    for (Ptr = AbbrevStart, EntryNum = 1; ; EntryNum++) {
        EntryPtr = Ptr;
        Ptr += read_uleb128_to_uint64 (Ptr, AbbrevEnd, &Help64);
        AbbrevEntryNr = (uint32_t)Help64;
        if (AbbrevEntryNr == 0) break;  // End of the AbbrevTable
        Ptr += read_uleb128_to_uint64 (Ptr, AbbrevEnd, &Help64);
        TagType = (uint32_t)Help64;
        DEBUGFPRINTF (DebugOut, "%i %s", AbbrevEntryNr, GetDwTagName (TagType));
        Ptr += read_uleb128_to_uint64 (Ptr, AbbrevEnd, &Help64);
        Children = (uint32_t)Help64;
        if (Children) {
            DEBUGFPRINTF0 (DebugOut, "  (has Children)");
        }
        DEBUGFPRINTF0 (DebugOut, "\n");
        for (AttribNum = 0; ; AttribNum++) {
            Ptr += read_uleb128_to_uint64 (Ptr, AbbrevEnd, &Help64);
            AttribName = (uint32_t)Help64;
            Ptr += read_uleb128_to_uint64 (Ptr, AbbrevEnd, &Help64);
            AttribForm = (uint32_t)Help64;
            DEBUGFPRINTF (DebugOut, "    %s   %s\n",
               GetDwAtName (AttribName),
               GetDwFormName (AttribForm));
            if (AttribForm == DW_FORM_implicit_const) {
                Ptr += read_uleb128_to_uint64 (Ptr, AbbrevEnd, &Help64);
            }
            if ((AttribName == 0) && (AttribForm == 0)) break;  // End of the attribute
        }
        if (EntryNum >= Ret->AbbrevCode) {
            // resize the table (2*)
            Ret->AbbrevCode <<= 1;
            Ret = my_realloc (Ret, sizeof (ABBREV_CODE_TABLE_ENTRY) * Ret->AbbrevCode);
            if (Ret == NULL) return NULL;
        }
        Ret[EntryNum].AbbrevCode = AbbrevEntryNr;
        Ret[EntryNum].Start.Ptr = EntryPtr;
    }

    Ret->Start.Size = EntryNum;  // Number of entrys in table
    return Ret;
}

static unsigned char *ParseOneTagAndHisAtributes (DEBUG_SECTIONS *ds,
                                                  int32_t CompileUnit,
                                                  ABBREV_CODE_TABLE_ENTRY *AbbrevCodeTable,
                                                  int32_t *ret_TreeDepth,
                                                  unsigned char *StartEntryPtr,
                                                  unsigned char *Ptr,
                                                  uint32_t AbbrevCode,
                                                  uint32_t CompileUnitOffset,
                                                  uint32_t *ret_TagType,
                                                  int32_t *ret_HasChildren,
                                                  ONE_DIE_ATTRIBUTES_DATA *ret_DieAttributes)
{
    uint32_t x;
    unsigned char *AbbrevEnd;
    uint64_t Help64 = 0;
    uint64_t AttribValue;

    memset (&(ret_DieAttributes->Flags), 0, sizeof (ret_DieAttributes->Flags));

    AbbrevEnd = (unsigned char*)ds->debug_abbrev + ds->debug_abbrev_len;

    if (AbbrevCode >= AbbrevCodeTable->Start.Size) {
        ThrowError (1, "abbrev code (%i) are larger then the abbrev of the compile unit (%i)", AbbrevCode, AbbrevCodeTable->Start.Size);
        return NULL;
    }
    for (x = 1; x < AbbrevCodeTable->Start.Size;  x++) {
        // Search the entry point into the abrev tabele
        if (AbbrevCodeTable[x].AbbrevCode == AbbrevCode) {
            unsigned char *AbbrevPtr;
            int32_t AttribNum;
            uint32_t AbbrevEntryNr, Children, AttribName, AttribForm;

            AbbrevPtr = AbbrevCodeTable[x].Start.Ptr;
            AbbrevPtr += read_uleb128_to_uint64 (AbbrevPtr, AbbrevEnd, &Help64);
            AbbrevEntryNr = (uint32_t)Help64;
            if (AbbrevEntryNr == 0) break;  // End of the AbbrevTable
            AbbrevPtr += read_uleb128_to_uint64 (AbbrevPtr, AbbrevEnd, &Help64);
            *ret_TagType = (uint32_t)Help64;

            DEBUGFPRINTF (DebugOut, "(%i) <%i> <%" PRIX64 "> Abbrev Number: %i (%s)", CompileUnit,
                                           *ret_TreeDepth, StartEntryPtr - (unsigned char*)ds->debug_info,
                                           AbbrevEntryNr, GetDwTagName (*ret_TagType));
            UNUSED(CompileUnit);
            UNUSED(StartEntryPtr);
            Children = (uint32_t)*AbbrevPtr;
            AbbrevPtr++;
            if (Children) {
                *ret_HasChildren = 1;
                (*ret_TreeDepth)++;
                DEBUGFPRINTF0 (DebugOut, "  (has Children)");
            } else {
                *ret_HasChildren = 0;
            }
            DEBUGFPRINTF0 (DebugOut, "\n");
            // Than go through all attributes
            for (AttribNum = 0; ; AttribNum++) {
                unsigned char *AttribStart = Ptr;
                AbbrevPtr += read_uleb128_to_uint64 (AbbrevPtr, AbbrevEnd, &Help64);
                AttribName = (uint32_t)Help64;
                AbbrevPtr += read_uleb128_to_uint64 (AbbrevPtr, AbbrevEnd, &Help64);
                AttribForm = (uint32_t)Help64;
                DEBUGFPRINTF (DebugOut, "  <%" PRIX64 ">  %s   %s  ",
                    AttribStart - (unsigned char*)ds->debug_info,
                    GetDwAtName (AttribName),
                    GetDwFormName (AttribForm));
                UNUSED(AttribStart);
                if ((AttribName == 0) && (AttribForm == 0)) {
                    DEBUGFPRINTF0 (DebugOut, "\n\n");
                    break;  // End of the attributes
                }
                Ptr = ReadAattrValue (AttribName,
                                      AttribForm,
                                      Ptr,
                                      CompileUnitOffset,
                                      ds,
                                      &AttribValue,
                                      &AbbrevPtr);
                DEBUGFPRINTF0 (DebugOut, "\n");
                if (Ptr == NULL) return NULL;

                switch (AttribName) {
                case DW_AT_name:
                    ret_DieAttributes->Flags.Name = 1;
                    ret_DieAttributes->Name = (char*)AttribValue;
                    break;
                case DW_AT_byte_size:
                    ret_DieAttributes->Flags.Size = 1;
                    ret_DieAttributes->Size = (int32_t)AttribValue;
                    break;
                case DW_AT_type:
                    ret_DieAttributes->Flags.Type = 1;
                    ret_DieAttributes->Type = (int32_t)AttribValue;
                    break;
                case DW_AT_data_member_location:
                    ret_DieAttributes->Flags.Location = 1;
                    ret_DieAttributes->Location = AttribValue;
                    break;
                case DW_AT_external:
                    ret_DieAttributes->Flags.External = 1;
                    ret_DieAttributes->External = (int32_t)AttribValue;
                    break;
                case DW_AT_declaration:
                    ret_DieAttributes->Flags.Declaration = 1;
                    ret_DieAttributes->Declaration = (int32_t)AttribValue;
                    break;
                case DW_AT_location:
                    ret_DieAttributes->Flags.Location = 1;
                    ret_DieAttributes->Location = AttribValue;
                    break;
                case DW_AT_encoding:
                    ret_DieAttributes->Flags.Encoding = 1;
                    ret_DieAttributes->Encoding = (uint32_t)AttribValue;
                    break;
                case DW_AT_sibling:
                    ret_DieAttributes->Flags.Sibling = 1;
                    ret_DieAttributes->Sibling = (uint32_t)AttribValue;
                    break;
                case DW_AT_upper_bound:
                    ret_DieAttributes->Flags.UpperBound = 1;
                    ret_DieAttributes->UpperBound = (int32_t)AttribValue;
                    break;
                case DW_AT_count:
                    ret_DieAttributes->Flags.Count = 1;
                    ret_DieAttributes->Count = (int32_t)AttribValue;
                    break;
                case DW_AT_specification:
                    ret_DieAttributes->Flags.Specification = 1;
                    ret_DieAttributes->Specification = (int32_t)AttribValue;
                    break;
                case DW_AT_MIPS_linkage_name:
                    ret_DieAttributes->Flags.MIPSLinkageName = 1;
                    ret_DieAttributes->MIPSLinkageName = (char*)AttribValue;
                    break;
                case DW_AT_linkage_name:
                    ret_DieAttributes->Flags.LinkageName = 1;
                    ret_DieAttributes->LinkageName = (char*)AttribValue;
                    break;
                case DW_AT_GNU_macros:
                    break;
                }
            }
            return Ptr;
        }
    }
    ThrowError (1, "unknown abbrev code (%i) in compile unit (%i)", AbbrevCode, CompileUnit);

    return Ptr;
}


unsigned char *ParseOneDie (DEBUG_SECTIONS *ds,
                            unsigned char *Ptr,
                            uint32_t AbbrevCode,
                            uint32_t CompileUnitOffset,
                            int32_t CompileUnit,
                            ABBREV_CODE_TABLE_ENTRY *AbbrevCodeTable,
                            int32_t *ret_TreeDepth,
                            int32_t *ret_HasChildren,
                            unsigned char *StartEntryPtr,
                            int32_t AddVariableAllowed);

static unsigned char *ParseUnknownTags (DEBUG_SECTIONS *ds,
                                        unsigned char *Ptr,
                                        uint32_t CompileUnitOffset,
                                        int32_t CompileUnit,
                                        ABBREV_CODE_TABLE_ENTRY *AbbrevCodeTable,
                                        int32_t *ret_TreeDepth,
                                        uint32_t Sibling)

{
    UNUSED(Sibling);
    unsigned char *StartEntryPtr;
    uint32_t DiesNum;
    uint64_t Help64;
    uint32_t AbbrevCode;
    int32_t HasChildren;

    for (DiesNum = 0; ; DiesNum++) {
        StartEntryPtr = Ptr;
        Ptr += read_uleb128_to_uint64 (Ptr, (unsigned char*)ds->debug_info + ds->debug_info_len, &Help64);
        AbbrevCode = (uint32_t)Help64;
        if (AbbrevCode == 0) {
            if (*ret_TreeDepth >= 1) {
                (*ret_TreeDepth)--;
                return Ptr;
            } else {
                ThrowError (1, "not expected null enty in debug infos");
                return NULL;
            }
        } else {
            Ptr = ParseOneDie (ds,
                               Ptr,
                               AbbrevCode,
                               CompileUnitOffset,
                               CompileUnit,
                               AbbrevCodeTable,
                               ret_TreeDepth,
                               &HasChildren,
                               StartEntryPtr, 0);
            if (Ptr == NULL) return NULL;
        }
    }
}


static unsigned char *ParseArraySubRangeTags (DEBUG_SECTIONS *ds,
                                              unsigned char *Ptr,
                                              uint32_t CompileUnitOffset,
                                              int32_t CompileUnit,
                                              ABBREV_CODE_TABLE_ENTRY *AbbrevCodeTable,
                                              int32_t *ret_TreeDepth,
                                              uint32_t Sibling,
                                              int32_t *ArrayDims,
                                              int32_t *ret_DimsCount,
                                              int32_t MaxDims)
{
    UNUSED(Sibling);
    UNUSED(MaxDims);
    int32_t Num, DimsCount = 0;
    uint64_t Help64;
    uint32_t AbbrevCode, TagType;
    ONE_DIE_ATTRIBUTES_DATA DieAttributes;
    unsigned char *StartEntryPtr;
    int32_t HasChildren;

    for (Num = 0; ; Num++) {
        StartEntryPtr = Ptr;
        Ptr += read_uleb128_to_uint64 (Ptr, (unsigned char*)ds->debug_info + ds->debug_info_len, &Help64);
        AbbrevCode = (uint32_t)Help64;
        if (AbbrevCode == 0) {
            if (*ret_TreeDepth > 0) {
                (*ret_TreeDepth)--;
                break;
            } else {
                ThrowError (1, "not expected null enty in debug infos");
                return NULL;
            }
        }
        memset (&DieAttributes, 0, sizeof (DieAttributes));
        Ptr = ParseOneTagAndHisAtributes (ds,
                                          CompileUnit,
                                          AbbrevCodeTable,
                                          ret_TreeDepth,
                                          StartEntryPtr,
                                          Ptr,
                                          AbbrevCode,
                                          CompileUnitOffset,
                                          &TagType,
                                          &HasChildren,
                                          &DieAttributes);
        if (Ptr == NULL) return NULL;

        switch (TagType) {
        case DW_TAG_subrange_type:
            if (DimsCount < 10) {
                if (DieAttributes.Flags.UpperBound) {  // gcc use this
                    ArrayDims[DimsCount] = DieAttributes.UpperBound + 1;
                } else if (DieAttributes.Flags.Count) { // clang this
                    ArrayDims[DimsCount] = DieAttributes.Count;
                } else {
                    ArrayDims[DimsCount] = 1; // dont know the size?
                }
                DimsCount++;
            } else {
                // No more than 10 dimensions
            }
            break;
        default:
            if (HasChildren) {
                Ptr = ParseUnknownTags (ds,
                                        Ptr,
                                        CompileUnitOffset,
                                        CompileUnit,
                                        AbbrevCodeTable,
                                        ret_TreeDepth,
                                        DieAttributes.Sibling);
            }
            break;
        }
    }
    *ret_DimsCount = DimsCount;
    return Ptr;
}

unsigned char *ParseDiesOfOneNameSpace (DEBUG_SECTIONS *ds,
                                        unsigned char *Ptr,
                                        unsigned char *NameSpaceEnd,
                                        int32_t *ret_TreeDepth,
                                        uint32_t CompileUnitOffset,
                                        int32_t CompileUnit,
                                        ABBREV_CODE_TABLE_ENTRY *AbbrevCodeTable);


static unsigned char *ParseStructMemberTags (DEBUG_SECTIONS *ds,
                                             unsigned char *Ptr,
                                             uint32_t CompileUnitOffset,
                                             int32_t CompileUnit,
                                             ABBREV_CODE_TABLE_ENTRY *AbbrevCodeTable,
                                             int32_t *ret_TreeDepth,
                                             //uint32_t Sibling,
                                             int32_t ParentTypeNr,
                                             int32_t ParentIsAUnion)
{
    int32_t Num, MemberCount = 0;
    uint64_t Help64;
    uint32_t AbbrevCode, TagType;
    ONE_DIE_ATTRIBUTES_DATA DieAttributes;
    unsigned char *StartEntryPtr;
    int32_t HasChildren;

    for (Num = 0; ; Num++) {
        StartEntryPtr = Ptr;
        Ptr += read_uleb128_to_uint64 (Ptr, (unsigned char*)ds->debug_info + ds->debug_info_len, &Help64);
        AbbrevCode = (uint32_t)Help64;
        if (AbbrevCode == 0) {
            if (*ret_TreeDepth > 0) {
                (*ret_TreeDepth)--;
                break;
            } else {
                ThrowError (1, "not expected null enty in debug infos");
                return NULL;
            }
        }
        Ptr = ParseOneTagAndHisAtributes (ds,
                                          CompileUnit,
                                          AbbrevCodeTable,
                                          ret_TreeDepth,
                                          StartEntryPtr,
                                          Ptr,
                                          AbbrevCode,
                                          CompileUnitOffset,
                                          &TagType,
                                          &HasChildren,
                                          &DieAttributes);
        if (Ptr == NULL) return NULL;

        switch (TagType) {
        case DW_TAG_inheritance:
            insert_struct_field (ParentTypeNr, DieAttributes.Type + TYPEID_OFFSET,
                                 "public", (int32_t)DieAttributes.Location, ds->pappldata);
            MemberCount++;
            break;
        case DW_TAG_member:
            if (DieAttributes.Flags.Name) {
                if (ParentIsAUnion) {
                    // Member of a union have always the offset 0
                    insert_struct_field (ParentTypeNr, DieAttributes.Type + TYPEID_OFFSET,
                                         DieAttributes.Name, 0, ds->pappldata);
                } else if (DieAttributes.Flags.Location) {
                    insert_struct_field (ParentTypeNr, DieAttributes.Type + TYPEID_OFFSET,
                                         DieAttributes.Name, (int32_t)DieAttributes.Location, ds->pappldata);
                } else {
                    // static member
                    if (DieAttributes.Flags.MIPSLinkageName) {
                        char *ExtendedName = GetDemangledSymbolName (DieAttributes.MIPSLinkageName, ds);
                        insert_label_no_address (ExtendedName, DieAttributes.Type + TYPEID_OFFSET,
                                                 (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET, ds->pappldata);
                    } else {
                        // static member DWARF4
                        insert_label_no_address (DieAttributes.Name, DieAttributes.Type + TYPEID_OFFSET,
                                                 (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET, ds->pappldata);
                    }
                }
                MemberCount++;
            }
            break;
// Begin that should be merged
    case DW_TAG_base_type:
        insert_struct (MODIFIER_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                       "", 0L, 0L,  get_basetype (DieAttributes.Size, DieAttributes.Encoding),
                       0L, 0L, 0L, ds->pappldata);
        break;
    case DW_TAG_unspecified_type:
        insert_struct (TYPEDEF_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                       "void", 0L, 0L, 33,  // 33 -> "void"
                       0L, 0L, 0L, ds->pappldata);
        break;
    case DW_TAG_enumeration_type:
        // enum class!
        insert_struct (MODIFIER_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                       "enum", 0L, 0L,  get_basetype (DieAttributes.Size, 2),
                       0L, 0L, 0L, ds->pappldata);
        if (HasChildren) {
            Ptr = ParseUnknownTags (ds,
                                    Ptr,
                                    CompileUnitOffset,
                                    CompileUnit,
                                    AbbrevCodeTable,
                                    ret_TreeDepth,
                                    DieAttributes.Sibling);
        }
        break;
    case DW_TAG_typedef:
        if (!DieAttributes.Flags.Name) {
            DieAttributes.Name = "";  // Structure without a namen
        } else {
            AddClassName (DieAttributes.Name, ds);
        }
        if (DieAttributes.Flags.Type) {
            char *ExtendedName = "";
            ExtendedName = ExtendWithNameSpace (DieAttributes.Name, ds);
            insert_struct (TYPEDEF_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                           ExtendedName, 0L, 0L, DieAttributes.Type + TYPEID_OFFSET,
                           0L, 0L, 0L, ds->pappldata);
        }
        if (DieAttributes.Flags.Name) {
            RemoveClassName (ds);
        }
        break;
    case DW_TAG_const_type:
        if (DieAttributes.Flags.Type) {
            insert_struct (MODIFIER_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                           "const", 0L, 0L, DieAttributes.Type + TYPEID_OFFSET,
                           0L, 0L, 0L, ds->pappldata);
        }
        break;
    case DW_TAG_volatile_type:
        if (DieAttributes.Flags.Type) {
            insert_struct (MODIFIER_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                           "volatile", 0L, 0L, DieAttributes.Type + TYPEID_OFFSET,
                           0L, 0L, 0L, ds->pappldata);
        }
        break;
    case DW_TAG_pointer_type:
    case DW_TAG_reference_type:
        insert_struct (POINTER_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                       "", 0L, 0L, (DieAttributes.Flags.Type) ? DieAttributes.Type + TYPEID_OFFSET : 33,   // 33 -> "void", 133 -> "*void"
                       0L, 0L, 0L, ds->pappldata);
        break;
    case DW_TAG_array_type:
        if (1) {
            char Help[32];
            int32_t ArrayDims[10];  // max. 10 dimensions
            int32_t x, DimCount;
            int32_t ArrayOfTypeId, ArrayTypeId;

            Ptr = ParseArraySubRangeTags (ds,
                                          Ptr,
                                          CompileUnitOffset,
                                          CompileUnit,
                                          AbbrevCodeTable,
                                          ret_TreeDepth,
                                          DieAttributes.Sibling, ArrayDims, &DimCount, 10);
            if (DimCount == 1) {
                ArrayTypeId = (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET;
                ArrayOfTypeId = DieAttributes.Type + TYPEID_OFFSET;
            } else {
                ArrayTypeId = (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET;
                ArrayOfTypeId = ds->HelpIdActiveFieldList + TYPEID_OFFSET;
                ds->HelpIdActiveFieldList++;
            }
            sprintf (Help, "[%i]", ArrayDims[0]);
            insert_struct (ARRAY_ELEM, ArrayTypeId, Help, 0L, 0L, ArrayOfTypeId,
                           ArrayDims[0], ArrayOfTypeId, (DWORD)0, ds->pappldata);

            for (x = 1; x < DimCount; x++) {
                sprintf (Help, "[%u]", ArrayDims[x]);
                ArrayTypeId = ArrayOfTypeId;
                if (x < (DimCount-1)) {
                    ArrayOfTypeId = ds->HelpIdActiveFieldList;
                    ds->HelpIdActiveFieldList++;
                } else {
                    ArrayOfTypeId = DieAttributes.Type + TYPEID_OFFSET;
                }
                insert_struct (ARRAY_ELEM, ArrayTypeId, Help, 0L, 0L, ArrayOfTypeId,
                               ArrayDims[x], ArrayOfTypeId, (DWORD)0, ds->pappldata);
            }
        }
        break;
    case DW_TAG_union_type:
    case DW_TAG_class_type:
    case DW_TAG_structure_type:
        if (!DieAttributes.Flags.Name) {
            DieAttributes.Name = "";  // Structure without namen
        } else {
            AddClassName (DieAttributes.Name, ds);
        }
        {
            int32_t StructSizeInBytes;
            char *ExtendedName = "";
            int32_t HelpTypeNr;

            // If it is a spezification of a previous decleration
            if (DieAttributes.Flags.Specification && (DieAttributes.Specification != 0)) {
                HelpTypeNr = SpecificationOfStruct (DieAttributes.Specification + TYPEID_OFFSET,&ExtendedName, ds);
                if (HelpTypeNr > 0) {
                    if (SetDeclarationToSpecification (DieAttributes.Specification + TYPEID_OFFSET, ds->pappldata)) {
                        ThrowError (1, "Internal error: %s (%i)", __FILE__, __LINE__);
                    }
                }
            } else {
                HelpTypeNr = ds->pappldata->unique_fieldnr_generator;
                ds->pappldata->unique_fieldnr_generator++;

                ExtendedName = ExtendWithNameSpace (DieAttributes.Name, ds);

                insert_field (HelpTypeNr, ds->pappldata);
            }
            if (DieAttributes.Flags.Size) StructSizeInBytes = DieAttributes.Size;
            else StructSizeInBytes = 0;

            if ((DieAttributes.Flags.Declaration) && (DieAttributes.Declaration)) {
                insert_struct (PRE_DEC_STRUCT, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                               ExtendedName, 0L, HelpTypeNr, CompileUnit /*pointsto will be used as CompileUnit*/,
                               0L, 0L, StructSizeInBytes, ds->pappldata);
            } else {
                insert_struct (STRUCT_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                               ExtendedName, 0L, HelpTypeNr, CompileUnit /*pointsto will be used as CompileUnit*/,
                               0L, 0L, StructSizeInBytes, ds->pappldata);
            }

            if (DieAttributes.Flags.Declaration && DieAttributes.Declaration) {   // Not a complete definition: there can be added something later
                AddStructDeclaration ((int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET, HelpTypeNr, ExtendedName, ds);
            }

            if (HelpTypeNr != 0) {
                if (HasChildren) {
                    Ptr = ParseStructMemberTags (ds,
                                                 Ptr,
                                                 CompileUnitOffset,
                                                 CompileUnit,
                                                 AbbrevCodeTable,
                                                 ret_TreeDepth,
                                                 HelpTypeNr,
                                                 TagType == DW_TAG_union_type);
                } else {
                    // this cann happen with classes without data
                }
            } else {
                ThrowError (1, "Internal error: %s (%i)", __FILE__, __LINE__);
            }
        }
        if (DieAttributes.Flags.Name) {
            RemoveClassName (ds);
        }
        break;
// End that should be merged
        default:
            if (HasChildren) {
                Ptr = ParseUnknownTags (ds,
                                        Ptr,
                                        CompileUnitOffset,
                                        CompileUnit,
                                        AbbrevCodeTable,
                                        ret_TreeDepth,
                                        DieAttributes.Sibling);
            }
            break;
        }
    }
    return Ptr;
}

unsigned char *ParseOneDie (DEBUG_SECTIONS *ds,
                            unsigned char *Ptr,
                            uint32_t AbbrevCode,
                            uint32_t CompileUnitOffset,
                            int32_t CompileUnit,
                            ABBREV_CODE_TABLE_ENTRY *AbbrevCodeTable,
                            int32_t *ret_TreeDepth,
                            int32_t *ret_HasChildren,
                            unsigned char *StartEntryPtr,
                            int32_t AddVariableAllowed)
{
    ONE_DIE_ATTRIBUTES_DATA DieAttributes;
    uint32_t TagType;

    Ptr = ParseOneTagAndHisAtributes (ds,
                                      CompileUnit,
                                      AbbrevCodeTable,
                                      ret_TreeDepth,
                                      StartEntryPtr,
                                      Ptr,
                                      AbbrevCode,
                                      CompileUnitOffset,
                                      &TagType,
                                      ret_HasChildren,
                                      &DieAttributes);
    if (Ptr == NULL) return NULL;

    switch (TagType) {
    case DW_TAG_variable:
        if (AddVariableAllowed) {
            if ((DieAttributes.Flags.Specification && DieAttributes.Flags.Location)) {
                char *Name;
                int32_t TypeNr;
                if (search_label_no_addr_by_typenr (DieAttributes.Specification + TYPEID_OFFSET, &Name, &TypeNr, ds->pappldata) == 0) {
                    char *ExtendedName;
                    if (DieAttributes.Flags.MIPSLinkageName) {
                        ExtendedName = GetDemangledSymbolName (DieAttributes.MIPSLinkageName, ds);
                    } else if (DieAttributes.Flags.LinkageName) {
                        ExtendedName = GetDemangledSymbolName (DieAttributes.LinkageName, ds);
                    } else {
                        ExtendedName = _alloca(strlen (Name) + 1);  // Why copy this on the stack?
                        StringCopyMaxCharTruncate (ExtendedName, Name, sizeof(ExtendedName));
                    }
                    insert_label (ExtendedName, TypeNr,
                                  DieAttributes.Location - ds->BaseAddr,
                                  ds->pappldata);
                }
            } else if (DieAttributes.Flags.Name && DieAttributes.Flags.Type) {
                char *ExtendedName;

                if (DieAttributes.Flags.MIPSLinkageName) {
                    ExtendedName = GetDemangledSymbolName (DieAttributes.MIPSLinkageName, ds);
                } else {
                    ExtendedName = ExtendWithNameSpace (DieAttributes.Name, ds);
                }
                if (s_main_ini_val.ViewStaticSymbols || (DieAttributes.Flags.External && DieAttributes.External)) {
                    if (s_main_ini_val.ExtendStaticLabelsWithFilename &&
                        (!DieAttributes.Flags.External || !DieAttributes.External)) {
                        ExtendedName = ExtendWithCurrentCompilationUnit (ExtendedName, ds);
                    }
                    if (DieAttributes.Flags.Location) {
                        insert_label (ExtendedName, DieAttributes.Type + TYPEID_OFFSET,
                                      DieAttributes.Location - ds->BaseAddr,
                                      ds->pappldata);
                    } else {
                        insert_label_no_address (ExtendedName, DieAttributes.Type + TYPEID_OFFSET,
                                                (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET, ds->pappldata);
                    }
                }
            } else {
                // ??
            }
        }
        break;
    case DW_TAG_base_type:
        insert_struct (MODIFIER_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                       "", 0L, 0L,  get_basetype (DieAttributes.Size, DieAttributes.Encoding),
                       0L, 0L, 0L, ds->pappldata);
        break;
    case DW_TAG_unspecified_type:
        insert_struct (TYPEDEF_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                       "void", 0L, 0L, 33,  // 33 -> "void"
                       0L, 0L, 0L, ds->pappldata);
        break;
    case DW_TAG_enumeration_type:
        insert_struct (MODIFIER_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                       "enum", 0L, 0L,  get_basetype (DieAttributes.Size, 2),
                       0L, 0L, 0L, ds->pappldata);
        if (*ret_HasChildren) {
            Ptr = ParseUnknownTags (ds,
                                    Ptr,
                                    CompileUnitOffset,
                                    CompileUnit,
                                    AbbrevCodeTable,
                                    ret_TreeDepth,
                                    DieAttributes.Sibling);
        }
        break;
    case DW_TAG_typedef:
        if (!DieAttributes.Flags.Name) {
            DieAttributes.Name = "";  // Structure without namen
        } else {
            AddClassName (DieAttributes.Name, ds);
        }
        if (DieAttributes.Flags.Type) {
            char *ExtendedName = "";
            ExtendedName = ExtendWithNameSpace (DieAttributes.Name, ds);
            insert_struct (TYPEDEF_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                           ExtendedName, 0L, 0L, DieAttributes.Type + TYPEID_OFFSET,
                           0L, 0L, 0L, ds->pappldata);
        }
        if (DieAttributes.Flags.Name) {
            RemoveClassName (ds);
        }
        break;
    case DW_TAG_const_type:
        if (DieAttributes.Flags.Type) {
            insert_struct (MODIFIER_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                           "const", 0L, 0L, DieAttributes.Type + TYPEID_OFFSET,
                           0L, 0L, 0L, ds->pappldata);
        }
        break;
    case DW_TAG_volatile_type:
        if (DieAttributes.Flags.Type) {
            insert_struct (MODIFIER_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                           "volatile", 0L, 0L, DieAttributes.Type + TYPEID_OFFSET,
                           0L, 0L, 0L, ds->pappldata);
        }
        break;
    case DW_TAG_pointer_type:
    case DW_TAG_reference_type:
        insert_struct (POINTER_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                       "", 0L, 0L, (DieAttributes.Flags.Type) ? DieAttributes.Type + TYPEID_OFFSET : 33,   // 33 -> "void", 133 -> "*void"
                       0L, 0L, 0L, ds->pappldata);
        break;
    case DW_TAG_array_type:
        if (1) {
            char Help[32];
            int32_t ArrayDims[10];  // max. 10 dimensions
            int32_t x, DimCount;
            int32_t ArrayOfTypeId, ArrayTypeId;

            Ptr = ParseArraySubRangeTags (ds,
                                          Ptr,
                                          CompileUnitOffset,
                                          CompileUnit,
                                          AbbrevCodeTable,
                                          ret_TreeDepth,
                                          DieAttributes.Sibling, ArrayDims, &DimCount, 10);
            if (DimCount == 1) {
                ArrayTypeId = (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET;
                ArrayOfTypeId = DieAttributes.Type + TYPEID_OFFSET;
            } else {
                ArrayTypeId = (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET;
                ArrayOfTypeId = ds->HelpIdActiveFieldList + TYPEID_OFFSET;
                ds->HelpIdActiveFieldList++;
            }
            sprintf (Help, "[%u]", ArrayDims[0]);
            insert_struct (ARRAY_ELEM, ArrayTypeId, Help, 0L, 0L, ArrayOfTypeId,
                           ArrayDims[0], ArrayOfTypeId, (DWORD)0, ds->pappldata);

            for (x = 1; x < DimCount; x++) {
                sprintf (Help, "[%u]", ArrayDims[x]);
                ArrayTypeId = ArrayOfTypeId;
                if (x < (DimCount-1)) {
                    ArrayOfTypeId = ds->HelpIdActiveFieldList + TYPEID_OFFSET;
                    ds->HelpIdActiveFieldList++;
                } else {
                    ArrayOfTypeId = DieAttributes.Type + TYPEID_OFFSET;
                }
                insert_struct (ARRAY_ELEM, ArrayTypeId, Help, 0L, 0L, ArrayOfTypeId,
                               ArrayDims[x], ArrayOfTypeId, (DWORD)0, ds->pappldata);
            }
        }
        break;
    case DW_TAG_union_type:
    case DW_TAG_class_type:
    case DW_TAG_structure_type:
        if (!DieAttributes.Flags.Name) {
            DieAttributes.Name = "";  // Structure without namen
        } else {
            AddClassName (DieAttributes.Name, ds);
        }
        {
            int32_t StructSizeInBytes;
            char *ExtendedName = "";
            int32_t HelpTypeNr;

            // If it is a spezification of a previous decleration
            if (DieAttributes.Flags.Specification && (DieAttributes.Specification != 0)) {
                HelpTypeNr = SpecificationOfStruct (DieAttributes.Specification + TYPEID_OFFSET, &ExtendedName, ds);
                if (HelpTypeNr > 0) {
                    if (SetDeclarationToSpecification (DieAttributes.Specification + TYPEID_OFFSET, ds->pappldata)) {
                        ThrowError (1, "Internal error: %s (%i)", __FILE__, __LINE__);
                    }
                }
            } else {
                HelpTypeNr = ds->pappldata->unique_fieldnr_generator;
                ds->pappldata->unique_fieldnr_generator++;
                ExtendedName = ExtendWithNameSpace (DieAttributes.Name, ds);
                insert_field (HelpTypeNr, ds->pappldata);
            }
            if (DieAttributes.Flags.Size) StructSizeInBytes = DieAttributes.Size;
            else StructSizeInBytes = 0;

            if ((DieAttributes.Flags.Declaration) && (DieAttributes.Declaration)) {
                insert_struct (PRE_DEC_STRUCT, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                               ExtendedName, 0L, HelpTypeNr, CompileUnit /*pointsto will be used as CompileUnit*/,
                               0L, 0L, StructSizeInBytes, ds->pappldata);
            } else {
                insert_struct (STRUCT_ELEM, (int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET,
                               ExtendedName, 0L, HelpTypeNr, CompileUnit /*pointsto will be used as CompileUnit*/,
                               0L, 0L, StructSizeInBytes, ds->pappldata);
            }

            if (DieAttributes.Flags.Declaration && DieAttributes.Declaration) {   // Not a complete definition: there can be added something later
                AddStructDeclaration ((int32_t)(StartEntryPtr - (unsigned char*)ds->debug_info) + TYPEID_OFFSET, HelpTypeNr, ExtendedName, ds);
            }

            if (HelpTypeNr != 0) {
                if (*ret_HasChildren) {
                    Ptr = ParseStructMemberTags (ds,
                                                 Ptr,
                                                 CompileUnitOffset,
                                                 CompileUnit,
                                                 AbbrevCodeTable,
                                                 ret_TreeDepth,
                                                 HelpTypeNr,
                                                 TagType == DW_TAG_union_type);
                } else {
                    // this can happen: class without data
                }
            } else {
                ThrowError (1, "Internal error: %s (%i)", __FILE__, __LINE__);
            }
        }

        if (DieAttributes.Flags.Name) {
            RemoveClassName (ds);
        }
        break;
    case DW_TAG_namespace:
        if (DieAttributes.Flags.Sibling) {
            if (DieAttributes.Flags.Name) {
                AddNameSpace (DieAttributes.Name, ds);
            } else {
                // do nothing
            }
            Ptr = ParseDiesOfOneNameSpace (ds,
                                           Ptr,
                                           (unsigned char*)ds->debug_info + DieAttributes.Sibling,
                                           ret_TreeDepth,
                                           CompileUnitOffset,
                                           CompileUnit,
                                           AbbrevCodeTable);
            if (DieAttributes.Flags.Name) {
                RemoveNameSpace (ds);
            }
        }
        if (Ptr == NULL) return NULL;
        break;
    case DW_TAG_compile_unit:
        if (DieAttributes.Flags.Name) {
            SetCurrentCompilationUnitName (DieAttributes.Name, ds);
        } else {
            SetCurrentCompilationUnitName ("", ds);
        }
        break;
    default:
        if (*ret_HasChildren) {
            Ptr = ParseUnknownTags (ds,
                                    Ptr,
                                    CompileUnitOffset,
                                    CompileUnit,
                                    AbbrevCodeTable,
                                    ret_TreeDepth,
                                    DieAttributes.Sibling);
        }
        break;
    }

    return Ptr;
}

unsigned char *ParseDiesOfOneNameSpace (DEBUG_SECTIONS *ds,
                                        unsigned char *Ptr,
                                        unsigned char *NameSpaceEnd,
                                        int32_t *ret_TreeDepth,
                                        uint32_t CompileUnitOffset,
                                        int32_t CompileUnit,
                                        ABBREV_CODE_TABLE_ENTRY *AbbrevCodeTable)
{
    unsigned char *StartEntryPtr;
    uint32_t DiesNum;
    uint64_t Help64;
    uint32_t AbbrevCode;
    int32_t HasChildren;

    for (DiesNum = 0; ; DiesNum++) {
        StartEntryPtr = Ptr;
        Ptr += read_uleb128_to_uint64 (Ptr, NameSpaceEnd, &Help64);
        AbbrevCode = (uint32_t)Help64;
        if (AbbrevCode == 0) {
            if (*ret_TreeDepth >= 1) {
                (*ret_TreeDepth)--;
                return Ptr;
            } else {
                ThrowError (1, "not expected null enty in debug infos");
                return NULL;
            }
        } else {
            Ptr = ParseOneDie (ds,
                               Ptr,
                               AbbrevCode,
                               CompileUnitOffset,
                               CompileUnit,
                               AbbrevCodeTable,
                               ret_TreeDepth,
                               &HasChildren,
                               StartEntryPtr, 1);
            if (Ptr == NULL) return NULL;
        }
    }
}

uint32_t ParseDiesOfOneCompileUnit (DEBUG_SECTIONS *ds,
                               unsigned char *CompileUnitStart,
                               uint32_t CompileUnitOffset, uint32_t CompileUnitLen,
                               int32_t CompileUnit,
                               ABBREV_CODE_TABLE_ENTRY *AbbrevCodeTable)
{
    unsigned char *CompileUnitEnd, *Ptr, *StartEntryPtr;
    uint32_t DiesNum;
    uint64_t Help64;
    uint32_t AbbrevCode;
    int32_t TreeDepth = 0;
    int32_t HasChildren;

    CompileUnitEnd = CompileUnitStart + CompileUnitLen;

    for (Ptr = CompileUnitStart, DiesNum = 0; Ptr < CompileUnitEnd; DiesNum++) {
        StartEntryPtr = Ptr;
        Ptr += read_uleb128_to_uint64 (Ptr, CompileUnitEnd, &Help64);
        AbbrevCode = (uint32_t)Help64;
        if (AbbrevCode == 0) {
            if (TreeDepth > 0) {
                TreeDepth--;
                if (TreeDepth == 0) return 0;
            } else {
                ThrowError (1, "not expected null enty in debug infos");
                return 0;
            }
        } else {
            Ptr = ParseOneDie (ds,
                               Ptr,
                               AbbrevCode,
                               CompileUnitOffset,
                               CompileUnit,
                               AbbrevCodeTable,
                               &TreeDepth,
                               &HasChildren,
                               StartEntryPtr, 1);
            if (Ptr == NULL) return 0;
            if (!HasChildren && (DiesNum == 0)) break;  // First are always a compile unit and if it has no child it is empty
        }
    }
    return DiesNum;
}

int32_t ParseCompileUnits (DEBUG_SECTIONS *ds)
{
    unsigned char *section_begin, *section_end, *ptr;
    int32_t num_units = 0;
    uint32_t length;
    uint32_t CompileUnitOffset;
    int32_t Version;
    uint32_t AbbrevOffset;
    unsigned char PointerSize;
    int32_t Is64BitFormat = 0;
    uint8_t UnitType;
    ABBREV_CODE_TABLE_ENTRY *AbbrevCodeTable = NULL;

    section_end = (unsigned char*)ds->debug_info + ds->debug_info_len;

    for (section_begin = (unsigned char *)ds->debug_info, num_units = 0; section_begin < section_end;
         num_units++) {
        CompileUnitOffset = (uint32_t)(section_begin - (unsigned char*)ds->debug_info);
        /* Read the first 4 bytes.  For a 32-bit DWARF section, this
           will be the length.  For a 64-bit DWARF section, it will be
           the escape code 0xffffffff followed by an 8 byte length.  */
        length = byte_get (section_begin, 4);

        if (length == 0xffffffff) {
            length = byte_get (section_begin + 4, 8);
            ptr = section_begin + 12;
            section_begin += length + 12;
            Is64BitFormat = 1;
        } else if (length >= 0xfffffff0 && length < 0xffffffff) {
            ThrowError (1, "Reserved length value found in section\n");
            return 0;
        } else {
            ptr = section_begin + 4;
            section_begin += length + 4;
            Is64BitFormat = 0;
        }

        /* Negative values are illegal, they may even cause infinite
           looping.  This can happen if we can't accurately apply
           relocations to an object file.  */
        if ((int32_t) length <= 0) {
            ThrowError (1, "Corrupt unit length found in section\n");
            return 0;
        }
        Version = (int32_t)byte_get (ptr, 2);
        ptr += 2;
        if (Version >= 5) {
            UnitType = (int32_t)byte_get (ptr, 1);
            ptr++;
            PointerSize = (unsigned char)byte_get (ptr, 1);
            ptr++;
            if (Is64BitFormat) {
                AbbrevOffset = byte_get (ptr, 8);
                ptr += 8;
            } else {
                AbbrevOffset = byte_get (ptr, 4);
                ptr += 4;
            }
        } else {
            if (Is64BitFormat) {
                AbbrevOffset = byte_get (ptr, 8);
                ptr += 8;
            } else {
                AbbrevOffset = byte_get (ptr, 4);
                ptr += 4;
            }
            PointerSize = (unsigned char)byte_get (ptr, 1);
            ptr++;
        }
        DEBUGFPRINTF0 (DebugOut, "\n");
        DEBUGFPRINTF (DebugOut, "Compilation Unit (%i) @ offset 0x%X:\n", num_units+1, CompileUnitOffset);
        DEBUGFPRINTF (DebugOut, "Length:        0x%X (32-bit)\n", length);
        DEBUGFPRINTF (DebugOut, "Version:       %i\n", Version);
        if (Version >= 5) {
            DEBUGFPRINTF (DebugOut, "Unit type:     %i\n", UnitType);
        }
        DEBUGFPRINTF (DebugOut, "Abbrev Offset: %u\n", AbbrevOffset);
        DEBUGFPRINTF (DebugOut, "Pointer Size:  %i\n", (int32_t)PointerSize);

        ds->Is64BitFormat = Is64BitFormat;
        ds->pointer_size = PointerSize;   // 4 or 8
        ds->pappldata->PointerSize = PointerSize;
        ds->offset_size = 4;    // 4 or 8
        ds->dwarf_version = Version;  //-1, 2,3,4,5

        DEBUGFPRINTF0 (DebugOut, "\n\nAbbrev:\n");
        AbbrevCodeTable = ParseAbbrevForOneCompileUnit (ds, AbbrevOffset, AbbrevCodeTable);
        if (AbbrevCodeTable == NULL) return 0;

        DEBUGFPRINTF0 (DebugOut, "\n\nDebug Infos:\n");

        ParseDiesOfOneCompileUnit (ds,
                                   ptr,
                                   CompileUnitOffset, length,
                                   num_units,
                                   AbbrevCodeTable);
    }

    if (AbbrevCodeTable != NULL) my_free (AbbrevCodeTable);

    return num_units;
}

#if 1
// only for debug
int32_t ParsePubTypesOfCompileUnits (DEBUG_SECTIONS *ds)
{
    unsigned char *section_begin, *this_section_begin, *section_end, *ptr;
    int32_t num_units = 0;
    uint32_t Offset, length;
    uint32_t CompileUnitOffset, CompileUnitLength;
    uint16_t Version;
    int32_t Is64BitFormat = 0;
    int32_t NumOfPupTypes;

    section_end = (unsigned char*)ds->debug_pubtypes + ds->debug_pubtypes_len;

    for (section_begin = (unsigned char*)ds->debug_pubtypes, num_units = 0; section_begin < section_end;
         num_units++) {
        this_section_begin = section_begin;
        Offset = (uint32_t)(section_begin - (unsigned char*)ds->debug_pubtypes);
        /* Read the first 4 bytes.  For a 32-bit DWARF section, this
           will be the length.  For a 64-bit DWARF section, it will be
           the escape code 0xffffffff followed by an 8 byte length.  */
        length = byte_get (section_begin, 4);

        if (length == 0xffffffff) {
            length = byte_get (section_begin + 4, 8);
            ptr = section_begin + 12;
            section_begin += length + 12;
            Is64BitFormat = 1;
        } else if (length >= 0xfffffff0 && length < 0xffffffff) {
            ThrowError (1, "Reserved length value found in section\n");
            return 0;
        } else {
            ptr = section_begin + 4;
            section_begin += length + 4;
            Is64BitFormat = 0;
        }

        /* Negative values are illegal, they may even cause infinite
           looping.  This can happen if we can't accurately apply
           relocations to an object file.  */
        if ((int32_t) length <= 0) {
            ThrowError (1, "Corrupt unit length found in section\n");
            return 0;
        }
        Version = (uint16_t)byte_get (ptr, 2);
        ptr += 2;
        if (Is64BitFormat) {
            CompileUnitOffset = byte_get (ptr, 8);
            ptr += 8;
            CompileUnitLength = byte_get (ptr, 8);
            ptr += 8;
         } else {
            CompileUnitOffset = byte_get (ptr, 4);
            ptr += 4;
            CompileUnitLength =  byte_get (ptr, 4);
            ptr += 4;
        }
        DEBUGFPRINTF0 (DebugOut, "\n");
        DEBUGFPRINTF (DebugOut, "Compilation Unit (%i) @ offset 0x%X:\n", num_units+1, Offset);
        DEBUGFPRINTF (DebugOut, "Length:        0x%X (32-bit)\n", length);
        DEBUGFPRINTF (DebugOut, "Version:       %i\n", (int32_t)Version);
        DEBUGFPRINTF (DebugOut, "Compile Unit Offset in .debug_info: %u\n", CompileUnitOffset);
        DEBUGFPRINTF (DebugOut, "Compile Unit Size in .debug_info: %u\n", CompileUnitLength);

        DEBUGFPRINTF0 (DebugOut, "\n\npublic types:\n");

        for (NumOfPupTypes = 0; ptr < (this_section_begin + length); NumOfPupTypes++) {
            uint32_t OffsetInCompileUnit;
            if (Is64BitFormat) {
                OffsetInCompileUnit = byte_get (ptr, 8);
                ptr += 8;
            } else {
                OffsetInCompileUnit = byte_get (ptr, 4);
                ptr += 4;
            }
            DEBUGFPRINTF (DebugOut, "  %X   %s\n", OffsetInCompileUnit, ptr);
            ptr += strlen ((char*)ptr) + 1;
        }
        DEBUGFPRINTF0 (DebugOut, "\n");
    }

    if (num_units == 0) {
        ThrowError (1, "No comp units in section");
        return 0;
    }

    return num_units;
}
#endif

int32_t SearchPubTypesOrName (DEBUG_SECTIONS *ds, int32_t CompileUnit, const char *Name, int32_t NameOrTypeFlag, uint32_t *ret_OffsetInDebugInfo)
{
    unsigned char *section_begin, *this_section_begin, *section_end, *ptr;
    int32_t num_units = 0;
    uint32_t Offset, length;
    uint32_t CompileUnitOffset, CompileUnitLength;
    uint16_t Version;
    int32_t Is64BitFormat = 0;
    int32_t NumOfPupTypes;

    if (NameOrTypeFlag) {
        section_begin = (unsigned char *)ds->debug_pubnames;
        section_end = (unsigned char*)ds->debug_pubnames + ds->debug_pubnames_len;
    } else {
        section_begin = (unsigned char *)ds->debug_pubtypes;
        section_end = (unsigned char*)ds->debug_pubtypes + ds->debug_pubtypes_len;
    }
    for (num_units = 0; section_begin < section_end; num_units++) {
        this_section_begin = section_begin;
        if (NameOrTypeFlag) {
            Offset = (uint32_t)(section_begin - (unsigned char*)ds->debug_pubnames);
        } else {
            Offset = (uint32_t)(section_begin - (unsigned char*)ds->debug_pubtypes);
        }
        /* Read the first 4 bytes.  For a 32-bit DWARF section, this
           will be the length.  For a 64-bit DWARF section, it will be
           the escape code 0xffffffff followed by an 8 byte length.  */
        length = byte_get (section_begin, 4);

        if (length == 0xffffffff) {
            length = byte_get (section_begin + 4, 8);
            ptr = section_begin + 12;
            section_begin += length + 12;
            Is64BitFormat = 1;
        } else if (length >= 0xfffffff0 && length < 0xffffffff) {
            ThrowError (1, "Reserved length value found in section\n");
            return 0;
        } else {
            ptr = section_begin + 4;
            section_begin += length + 4;
            Is64BitFormat = 0;
        }

        /* Negative values are illegal, they may even cause infinite
           looping.  This can happen if we can't accurately apply
           relocations to an object file.  */
        if ((int32_t) length <= 0) {
            ThrowError (1, "Corrupt unit length found in section\n");
            return 0;
        }
        Version = (uint16_t)byte_get (ptr, 2);
        ptr += 2;
        if (Is64BitFormat) {
            CompileUnitOffset = byte_get (ptr, 8);
            ptr += 8;
            CompileUnitLength = byte_get (ptr, 8);
            ptr += 8;
         } else {
            CompileUnitOffset = byte_get (ptr, 4);
            ptr += 4;
            CompileUnitLength =  byte_get (ptr, 4);
            ptr += 4;
        }

        if ((CompileUnit == -1) || (CompileUnit == num_units)) {
            for (NumOfPupTypes = 0; ptr < (this_section_begin + length); NumOfPupTypes++) {
                uint32_t OffsetInCompileUnit;
                if (Is64BitFormat) {
                    OffsetInCompileUnit = byte_get (ptr, 8);
                    ptr += 8;
                } else {
                    OffsetInCompileUnit = byte_get (ptr, 4);
                    ptr += 4;
                }
                if (!strcmp ((char*)ptr, Name)) {
                    *ret_OffsetInDebugInfo = OffsetInCompileUnit + CompileUnitOffset;
                    return 0;
                }
                ptr += strlen ((char*)ptr) + 1;
            }
        }
    }

    if (num_units == 0) {
        ThrowError (1, "No comp units in section");
        return 0;
    }

    return -1;
}


int32_t parse_dwarf_from_exe_file (char *par_ExeFileName, DEBUG_INFOS_DATA *pappldata, int IsRealtimeProcess, int MachineType)
{
    char ExtractedDebugInfoFile[MAX_PATH];
    DEBUG_SECTIONS *DebugSections;
    int32_t num_units;
    int32_t Ret = 0;
    char *p;
    char *DebugInfoFile;

#ifndef DebugOut
    char DebugPrintFileName[MAX_PATH];
#ifdef _WIN32
    strcpy (DebugPrintFileName, "c:\\temp\\debug_dwarf.txt");
#else
    strcpy (DebugPrintFileName, "/tmp/debug_dwarf.txt");
#endif
#ifdef PER_PROCESS_LOGGING
    strcpy (DebugPrintFileName, "c:\\temp\\");
    if (GetProcessNameWithoutPath (pappldata->pid, DebugPrintFileName + strlen (DebugPrintFileName)) == 0) {
        strcat (DebugPrintFileName, ".txt");
#endif
        DebugOut = fopen (DebugPrintFileName, "wt");
        // no file buffer!
        //setbuf(DebugOut, NULL);
        // huge file buffer:
        setvbuf (DebugOut , NULL , _IOFBF , 256*1024);
#ifdef PER_PROCESS_LOGGING
    } else {
        DebugOut = NULL;
    }
#endif
#endif

    DebugSections = my_malloc (sizeof (DEBUG_SECTIONS));
    if (DebugSections == NULL) {
        ThrowError (1, "out of memmory");
        Ret = -1;
    } else {
        memset (DebugSections, 0, sizeof (DEBUG_SECTIONS));

        // Are the debug infos inside an own *.dbg file?
        DebugInfoFile = par_ExeFileName;
        p =  par_ExeFileName;
        while (*p != 0) p++;
        if (((p - par_ExeFileName) > 4) &&
             !strcmpi (p-4, ".exe")) {
#ifdef _WIN32
            HANDLE hFile;
#else
            int fh;
#endif
            MEMCPY (ExtractedDebugInfoFile, par_ExeFileName, (size_t)((p - par_ExeFileName) - 3));
            StringCopyMaxCharTruncate (ExtractedDebugInfoFile + (p - par_ExeFileName) - 3, "dbg", 4);
#ifdef WIN32
            hFile = CreateFile (ExtractedDebugInfoFile,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
            if (hFile != (HANDLE)HFILE_ERROR) {
                LogFileAccess (ExtractedDebugInfoFile);
                CloseHandle (hFile);
                DebugInfoFile = ExtractedDebugInfoFile;
            }
#else
            fh = open(ExtractedDebugInfoFile, O_RDONLY);
            if (fh > 0) {
                LogFileAccess (ExtractedDebugInfoFile);
                close(fh);
                DebugInfoFile = ExtractedDebugInfoFile;
            }
#endif

        }

        DebugSections->HelpIdActiveFieldList = 0x40000000;
        DebugSections->pappldata = pappldata;
        DebugSections->debug_info = NULL;
        DebugSections->debug_abbrev = NULL;
        DebugSections->debug_str = NULL;
        if (ReadSectionToMem (DebugInfoFile, ".debug_info", &DebugSections->debug_info, &DebugSections->debug_info_len, &DebugSections->BaseAddr, 1, IsRealtimeProcess, MachineType) ||
            ReadSectionToMem (DebugInfoFile, ".debug_abbrev", &DebugSections->debug_abbrev, &DebugSections->debug_abbrev_len, &DebugSections->BaseAddr, 1, IsRealtimeProcess, MachineType)) {
            ThrowError (1, "cannot read sections to memory");
            Ret = -1;
        } else {
            if (ReadSectionToMem (DebugInfoFile, ".debug_str", &DebugSections->debug_str, &DebugSections->debug_str_len, &DebugSections->BaseAddr, 0, IsRealtimeProcess, MachineType)) {
                DebugSections->debug_str = NULL;
            }
            if (ReadSectionToMem (DebugInfoFile, ".debug_line_str", &DebugSections->debug_line_str, &DebugSections->debug_line_str_len, &DebugSections->BaseAddr, 0, IsRealtimeProcess, MachineType)) {
                DebugSections->debug_line_str = NULL;
                DebugSections->debug_line_str_len = 0;
            }
            DEBUGFPRINTF (DebugOut, ".debug_info section size = %" PRIu64 "\n", DebugSections->debug_info_len);
            DEBUGFPRINTF (DebugOut, ".debug_abbrev section size = %" PRIu64 "\n", DebugSections->debug_abbrev_len);
            DEBUGFPRINTF (DebugOut, ".debug_str section size = %" PRIu64 "\n", DebugSections->debug_str_len);
            DEBUGFPRINTF (DebugOut, ".debug_line_str section size = %" PRIu64 "\n", DebugSections->debug_line_str_len);
            DEBUGFPRINTF (DebugOut, ".Base Address = 0x%" PRIX64 "\n", DebugSections->BaseAddr);

            DebugSections->BaseAddr = pappldata->ImageBaseAddr;

            num_units = ParseCompileUnits (DebugSections);
            CleanListOfStructDeclarations (DebugSections);

            DEBUGFPRINTF (DebugOut, "\n\nnumber of compilation units found = %i\n", num_units);
    #ifndef DebugOut
            if (DebugOut != NULL) { fclose (DebugOut); DebugOut = NULL; }
    #endif

            pappldata->AbsoluteOrRelativeAddress = DEBUGINFO_ADDRESS_RELATIVE;

            pappldata->DebugInfoType = DEBUGINFOTYPE_DWARF;

            pappldata->associated_exe_file = 1;
        }
        if (DebugSections->debug_info != NULL) my_free (DebugSections->debug_info);
        if (DebugSections->debug_abbrev != NULL) my_free (DebugSections->debug_abbrev);
        if (DebugSections->debug_str != NULL) my_free (DebugSections->debug_str);
        if (DebugSections->debug_line_str != NULL) my_free (DebugSections->debug_line_str);
    }
    if (DebugSections != NULL) {
        my_free (DebugSections);
    }
    return Ret;
}
