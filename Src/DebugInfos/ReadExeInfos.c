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


#include <stdio.h>
#include <stdlib.h>
#include "Platform.h"

#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Files.h"
#include "ThrowError.h"
#include "VersionInfoSection.h"
#include "GetExeDebugSign.h"
#include "ReadExeInfos.h"

/* Maximale Groesse eines DWARF Namens */
#define MAXIMUM_DWARF_NAME_SIZE 64
/* Groesse in Byte eines Symbols in der COFF Symbol Table */
#define SYMBOL_SIZE   18 


#ifdef _WIN32
//typedef HANDLE MY_FILE_HANDLE;
//#define MY_INVALID_HANDLE_VALUE  INVALID_HANDLE_VALUE

#define EI_NIDENT       16
#pragma pack(push, 1)
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
typedef struct
{
  uint32_t	sh_name;		/* Section name (string tbl index) */
  uint32_t	sh_type;		/* Section type */
  uint32_t	sh_flags;		/* Section flags */
  uint32_t	sh_addr;		/* Section virtual addr at execution */
  uint32_t	sh_offset;		/* Section file offset */
  uint32_t	sh_size;		/* Section size in bytes */
  uint32_t	sh_link;		/* Link to another section */
  uint32_t	sh_info;		/* Additional section information */
  uint32_t	sh_addralign;		/* Section alignment */
  uint32_t	sh_entsize;		/* Entry size if section holds table */
} Elf32_Shdr;

typedef struct
{
  uint32_t	sh_name;		/* Section name (string tbl index) */
  uint32_t	sh_type;		/* Section type */
  uint64_t	sh_flags;		/* Section flags */
  uint64_t	sh_addr;		/* Section virtual addr at execution */
  uint64_t	sh_offset;		/* Section file offset */
  uint64_t	sh_size;		/* Section size in bytes */
  uint32_t	sh_link;		/* Link to another section */
  uint32_t	sh_info;		/* Additional section information */
  uint64_t	sh_addralign;		/* Section alignment */
  uint64_t	sh_entsize;		/* Entry size if section holds table */
} Elf64_Shdr;
#pragma pack(pop)

#if defined(_WIN32) && !defined(__GNUC__)
typedef size_t ssize_t;
#endif

#else

/*
#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_NT_SIGNATURE                  0x00004550  // PE00

#define IMAGE_FILE_MACHINE_AMD64             0x8664  // AMD64 (K8)

#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE 0x0040     // DLL can move.


typedef struct {
    uint16_t e_magic;
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    int32_t e_lfanew;
  } IMAGE_DOS_HEADER;

typedef struct {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER;

typedef struct {
    uint32_t VirtualAddress;
    uint32_t Size;
} IMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16

typedef struct {
    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;
    uint32_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32;

typedef struct {
    uint32_t Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} IMAGE_NT_HEADERS32;

typedef struct {
    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64;

typedef struct {
    uint32_t Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64;

#define IMAGE_SIZEOF_SHORT_NAME              8

typedef struct {
    uint8_t    Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
            uint32_t   PhysicalAddress;
            uint32_t   VirtualSize;
    } Misc;
    uint32_t   VirtualAddress;
    uint32_t   SizeOfRawData;
    uint32_t   PointerToRawData;
    uint32_t   PointerToRelocations;
    uint32_t   PointerToLinenumbers;
    uint16_t    NumberOfRelocations;
    uint16_t    NumberOfLinenumbers;
    uint32_t   Characteristics;
} IMAGE_SECTION_HEADER;
*/
//typedef int MY_FILE_HANDLE;
//#define MY_INVALID_HANDLE_VALUE  -1

#endif


int ReadExeInfos (char *Filename, uint64_t *pBaseAddr, unsigned int *pLinkerVersion,
                  int *pNumOfSections,
                  char ***pRetSectionNames,
                  uint64_t **pRetSectionVirtualAddresses,
                  uint32_t **pRetSectionVirtualSize,
                  uint32_t **pRetSectionSizeOfRawData,
                  uint64_t **pRetSectionPointerToRawData,

                  int *ret_SignatureType,   // 0 -> ignore, 1 -> Signature are valid, 2 -> SignatureGuid are valid
                  DWORD *ret_Signature,
                  GUID *ret_SignatureGuid,
                  DWORD *ret_Age,

                  int *pDynamicBaseAddress, int IsRealtimeProcess, int MachineType)
{
    char **pSectionNames;
    uint64_t *pSectionVirtualAddresses;
    uint32_t *pSectionVirtualSize;
    uint32_t *pSectionSizeOfRawData;
    uint64_t *pSectionPointerToRawData;

    if ((MachineType == 0 /* Win32 */) || (MachineType == 1 /* Win64 */)) {
        IMAGE_DOS_HEADER DosHeader;
        IMAGE_NT_HEADERS32 NtHeader;
        IMAGE_NT_HEADERS64 NtHeader64;
        MY_FILE_HANDLE hFile;
        DWORD x, i, ii;
        IMAGE_SECTION_HEADER SectionHeader;

        if (IsRealtimeProcess) {
            ThrowError (1, "ReadExeInfos() IsRealtimeProcess is set but MachineType are %i", MachineType);
            return -1;
        }
        *pDynamicBaseAddress = 0;

        hFile = my_open (Filename);

        if (hFile == MY_INVALID_HANDLE_VALUE) {
            ThrowError (1, "Unable to open file \"%s\"", Filename);
            return -1;
        }
        LogFileAccess (Filename);
        my_read (hFile, &DosHeader, sizeof (DosHeader));
        if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
            my_close (hFile);
            ThrowError (1, "file %s in not an executable", Filename);
            return -1;
        }
        my_lseek (hFile, (uint64_t)DosHeader.e_lfanew);
        my_read (hFile, &NtHeader, sizeof (NtHeader));
        if (NtHeader.Signature != IMAGE_NT_SIGNATURE) {
            my_close (hFile);
            ThrowError (1, "file %s in not an executable", Filename);
            return -1;
        }

        IMAGE_DATA_DIRECTORY *Directory;

        // is this a 64 bit executable?
        if (NtHeader.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
            my_lseek (hFile, (uint64_t)DosHeader.e_lfanew);
            my_read (hFile, &NtHeader64, sizeof (NtHeader64));
            *pBaseAddr = NtHeader64.OptionalHeader.ImageBase;
            Directory = &NtHeader64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
        } else {
            // Check if the linker option /DYNAMICBASE:YES was set (Address space layout randomization)
            if ((NtHeader.OptionalHeader.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE) == IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE) {
                 *pDynamicBaseAddress = 1;
            }
            *pBaseAddr = NtHeader.OptionalHeader.ImageBase;
            Directory = &NtHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
        }
        if (pLinkerVersion != NULL) *pLinkerVersion = NtHeader.OptionalHeader.MajorLinkerVersion;

        if (pNumOfSections != NULL) {
            *pNumOfSections = NtHeader.FileHeader.NumberOfSections;
        }
        if (pRetSectionNames != NULL) {
            pSectionNames = my_malloc (NtHeader.FileHeader.NumberOfSections * sizeof (char**));
            pSectionVirtualAddresses = my_malloc (NtHeader.FileHeader.NumberOfSections * sizeof (void**));
            pSectionVirtualSize = my_malloc (NtHeader.FileHeader.NumberOfSections * sizeof (uint32_t));
            pSectionSizeOfRawData = my_malloc (NtHeader.FileHeader.NumberOfSections * sizeof (uint32_t));
            pSectionPointerToRawData = my_malloc (NtHeader.FileHeader.NumberOfSections * sizeof (uint64_t));
            if ((pSectionNames == NULL) || (pSectionVirtualAddresses == NULL) || (pSectionVirtualSize == NULL) || (pSectionSizeOfRawData == NULL) || (pSectionPointerToRawData == NULL)) {
                my_close (hFile);
                ThrowError (1, "executable file %s has %i sections, cannot read so many sections", Filename, (int)NtHeader.FileHeader.NumberOfSections);
                return -1;
            }
            for (x = 0; x < NtHeader.FileHeader.NumberOfSections; x++) {
                my_read (hFile, &SectionHeader, sizeof (IMAGE_SECTION_HEADER));
                // GCC uses longer as 8 byte section names
                if (SectionHeader.Name[0] == '/') {
                    uint64_t SaveFilePos;
                    uint64_t VirtualOffset;
                    char c;
                    VirtualOffset = NtHeader.FileHeader.PointerToSymbolTable +
                                             (NtHeader.FileHeader.NumberOfSymbols * SYMBOL_SIZE) +
                                             strtoull ((char*)SectionHeader.Name+1, NULL, 0);
                    SaveFilePos = my_ftell(hFile);
                    my_lseek(hFile, VirtualOffset);
                    for (i = 0; i < MAXIMUM_DWARF_NAME_SIZE; i++) {
                        my_read (hFile, &c, 1);
                        if (c == 0) break;
                    }
                    pSectionNames[x] = (char*)my_malloc (i + 1);
                    my_lseek (hFile, VirtualOffset);
                    for (ii = 0; ii <= i; ii++) {
                        my_read (hFile, &c, 1);
                        pSectionNames[x][ii] = c;
                        if (c == 0) break;
                    }
                    my_lseek (hFile, SaveFilePos);
                } else {
                    pSectionNames[x] = (char*)my_malloc (8 + 1);
                    MEMCPY (pSectionNames[x], SectionHeader.Name, 8);
                    pSectionNames[x][8] = 0; // Null termination, max. 8 chars long.
                }
                pSectionVirtualAddresses[x] = (uint64_t)SectionHeader.VirtualAddress;
                pSectionVirtualSize[x] = SectionHeader.Misc.VirtualSize;
                pSectionSizeOfRawData[x] = SectionHeader.SizeOfRawData;
                pSectionPointerToRawData[x] = SectionHeader.PointerToRawData;
            }
            *pRetSectionNames = pSectionNames;
            *pRetSectionVirtualAddresses = pSectionVirtualAddresses;
            *pRetSectionVirtualSize = pSectionVirtualSize;
            *pRetSectionSizeOfRawData = pSectionSizeOfRawData;
            *pRetSectionPointerToRawData =pSectionPointerToRawData;

            GetExeDebugSign (hFile, Directory,
                             *pBaseAddr,
                             NtHeader.FileHeader.NumberOfSections,
                             pSectionNames,
                             pSectionVirtualAddresses,
                             pSectionSizeOfRawData,
                             pSectionPointerToRawData,

                             ret_SignatureType,   // 0 -> ignore, 1 -> Signature are valid, 2 -> SignatureGuid are valid
                             ret_Signature,
                             ret_SignatureGuid,
                             ret_Age);
        }

        my_close (hFile);
        return 0;
    } else {
        MY_FILE_HANDLE fh;
        int ReadResult;
        Elf32_Ehdr Ehdr32;
        Elf64_Ehdr Ehdr64;
        int LSBFirst;
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

        if ((MachineType != 2 /* Linux 32 */) && (MachineType != 3 /* Linux 64 */)) {
            ThrowError (1, "ReadExeInfos() MachineType must be 2 or 3 but MachineType are %i", MachineType);
            return -1;
        }
        if ((sizeof(Elf32_Ehdr) != 0x34) ||
            (sizeof(Elf64_Shdr) != 0x40) ||
            (sizeof(Elf32_Shdr) != 0x28) ||
            (sizeof(Elf64_Shdr) != 0x40)) {
            ThrowError (1, "internal sizeof error %s %i", __FILE__, __LINE__);
        }

        fh = my_open(Filename);
        if (fh == MY_INVALID_HANDLE_VALUE) {
            ThrowError (1, "Unable to open %s file\n", Filename);
            return -1;
        }
        LogFileAccess (Filename);
        ReadResult = my_read (fh, &Ehdr32, sizeof (Ehdr32));
        if (ReadResult != sizeof (Ehdr32)) {
            ThrowError (1, "cannot read from file %s", Filename);
            return -1;
        }
        if ((Ehdr32.e_ident[0] != 0x7F) ||
            (Ehdr32.e_ident[1] != 'E') ||
            (Ehdr32.e_ident[2] != 'L') ||
            (Ehdr32.e_ident[3] != 'F')) {
            my_close (fh);
            ThrowError (1, "file %s in not an ELF executable", Filename);
            return -1;
        }
        if ((Ehdr32.e_ident[4] != 2) &&
            (Ehdr32.e_ident[4] != 1)) {
            my_close (fh);
            ThrowError (1, "file %s in not 32/64 bit ELF executable", Filename);
            return -1;
        }
        if (Ehdr32.e_ident[5] == 1) {
            LSBFirst = 1;
        } else if (Ehdr32.e_ident[5] == 2) {
            LSBFirst = 0;
            my_close (fh);
            ThrowError (1, "file %s in not a lsb first ELF executable (msb first are not supported)", Filename);
            return -1;
        }
        if (Ehdr32.e_ident[4] == 2) {
            my_lseek(fh, 0);
            ReadResult = my_read (fh, &Ehdr64, sizeof (Ehdr64));
            if (ReadResult != sizeof (Ehdr64)) {
                ThrowError (1, "cannot read from file %s", Filename);
                return -1;
            }
            NumOfSections = Ehdr64.e_shnum;
            SectionNameStringTable = Ehdr64.e_shstrndx;
            SectionHeaderOffset = Ehdr64.e_shoff;
            Is64Bit = 1;
        }
        else {
            NumOfSections = Ehdr32.e_shnum;
            SectionNameStringTable = Ehdr32.e_shstrndx;
            SectionHeaderOffset = Ehdr32.e_shoff;
            Is64Bit = 0;
        }

        if (pRetSectionNames != NULL) {
            pSectionNames = my_malloc (NumOfSections * sizeof (char**));
            pSectionVirtualAddresses = my_malloc (NumOfSections * sizeof (void**));
            pSectionVirtualSize = my_malloc (NumOfSections * sizeof (uint32_t));
            pSectionSizeOfRawData = my_malloc (NumOfSections * sizeof (uint32_t));
            pSectionPointerToRawData = my_malloc (NumOfSections * sizeof (uint64_t));
            if ((pSectionNames == NULL) || (pSectionVirtualAddresses == NULL) || (pSectionVirtualSize == NULL) || (pSectionSizeOfRawData == NULL) || (pSectionPointerToRawData == NULL)) {
                my_close (fh);
                ThrowError (1, "executable file %s has %i sections, cannot read so many sections", Filename, NumOfSections);
                return -1;
            }
            if (Is64Bit) {
                my_lseek(fh, SectionHeaderOffset + SectionNameStringTable * sizeof(Shdr64));
                ReadResult = my_read(fh, &stShdr64, sizeof(stShdr64));
                if (ReadResult != sizeof(stShdr64)) {
                    my_close(fh);
                    ThrowError(1, "cannot read from file %s", Filename);
                    return -1;
                }
                StringBaseOffset = stShdr64.sh_offset;
            } else {
                my_lseek(fh, SectionHeaderOffset + SectionNameStringTable * sizeof(Shdr32));
                ReadResult = my_read(fh, &stShdr32, sizeof(stShdr32));
                if (ReadResult != sizeof(stShdr32)) {
                    my_close(fh);
                    ThrowError(1, "cannot read from file %s", Filename);
                    return -1;
                }
                StringBaseOffset = stShdr32.sh_offset;
            }
            for (i = 0; i < NumOfSections; i++) {
                char SectionName[256];
                char c;
                int x;

                SectionName[0] = 0;
                if (Is64Bit) {
                    uint64_t file_pos = SectionHeaderOffset + i * sizeof(Shdr64);
                    my_lseek(fh, file_pos);
                    ReadResult = my_read(fh, &Shdr64, sizeof(Shdr64));
                    if (ReadResult != sizeof(Shdr64)) {
                        my_close(fh);
                        ThrowError(1, "cannot read from file %s", Filename);
                        return -1;
                    }
                    my_lseek(fh, StringBaseOffset + Shdr64.sh_name);
                } else {
                    uint64_t file_pos = SectionHeaderOffset + i * sizeof(Shdr32);
                    my_lseek(fh, file_pos);
                    ReadResult = my_read(fh, &Shdr32, sizeof(Shdr32));
                    if (ReadResult != sizeof(Shdr32)) {
                        my_close(fh);
                        ThrowError(1, "cannot read from file %s", Filename);
                        return -1;
                    }
                    my_lseek(fh, StringBaseOffset + Shdr32.sh_name);
                }
                for (x = 0; x < (int)sizeof(SectionName); x++) {
                    ReadResult = my_read(fh, &c, 1);
                    if (ReadResult != 1) {
                        my_close(fh);
                        ThrowError(1, "cannot read from file %s", Filename);
                        return -1;
                    }
                    SectionName[x] = c;
                    if (c == 0) break;
                }
                pSectionNames[i] = my_malloc(strlen(SectionName)+1);
                strcpy(pSectionNames[i], SectionName);
                if (Is64Bit) {
                    pSectionVirtualAddresses[i] = Shdr64.sh_addr;
                    pSectionVirtualSize[i] = (uint32_t)Shdr64.sh_size;
                    pSectionSizeOfRawData[i] = (uint32_t)Shdr64.sh_size;
                    pSectionPointerToRawData[i] = Shdr64.sh_offset;
                } else {
                    pSectionVirtualAddresses[i] = Shdr32.sh_addr;
                    pSectionVirtualSize[i] = Shdr32.sh_size;
                    pSectionSizeOfRawData[i] = Shdr32.sh_size;
                    pSectionPointerToRawData[i] = Shdr32.sh_offset;
                }
            }
            *pRetSectionNames = pSectionNames;
            *pRetSectionVirtualAddresses = pSectionVirtualAddresses;
            *pRetSectionVirtualSize = pSectionVirtualSize;
            *pRetSectionSizeOfRawData = pSectionSizeOfRawData;
            *pRetSectionPointerToRawData = pSectionPointerToRawData;

            *ret_SignatureType = 0;   // 0 -> ignore: GetExeDebugSign() not available
        }
        *pBaseAddr = 0;
        *pLinkerVersion = 0;
        *pLinkerVersion = 0;
        *pNumOfSections = (int)NumOfSections;

        my_close (fh);
        return 0;
    }
}


#define PEHEADER_OFFSET(a)  ((uint64_t)a.e_lfanew      +	\
             (uint64_t)sizeof (DWORD)		     +	\
             (uint64_t)sizeof (IMAGE_FILE_HEADER))

int32_t GetExeFileTimeAndChecksum (char *ExeName, uint64_t *RetLastWriteTime, DWORD *RetCheckSum, int32_t *Linker, int IsRealtimeProcess, int MachineType)
{

    MY_FILE_HANDLE hFile;
#ifdef _WIN32
    FILETIME LastWriteTime;
#else
    struct stat attrib;
    char unix_path[1024];
#endif
    int32_t ret = -1;

    hFile = my_open (ExeName);
    if (hFile == MY_INVALID_HANDLE_VALUE) {
        ret = -1;   // Error case is not equal
    } else {
        LogFileAccess (ExeName);
#ifdef _WIN32
        if (!GetFileTime (hFile, NULL, NULL, &LastWriteTime)) {
#else
       if (GetRealPathForOnlyUpperCasePath(ExeName, unix_path, sizeof(unix_path))) {
           ret = -1;
       } else if (stat (unix_path, &attrib)) {
#endif
            ret = -1;
        } else {
#ifdef _WIN32
            *RetLastWriteTime = ((uint64_t)LastWriteTime.dwHighDateTime << 32) + (uint64_t)LastWriteTime.dwLowDateTime;
#else
            *RetLastWriteTime = attrib.st_mtime;
#endif
            if ((MachineType == 2 /* Linux 32 */) || (MachineType == 3 /* Linux 64 */)) {
                // ELF files have no checksum
                *RetCheckSum = 0;
                *Linker = LINKER_TYPE_GCC;
                ret = 0;
            } else if (((MachineType != 0 /* Win32 */) && (MachineType != 1 /* Win64 */)) || IsRealtimeProcess) {
                ThrowError (1, "GetExeFileTimeAndChecksum() MachineType must be 0 or 1 and IsRealtimeProcess must be 0."
                          " But MachineType are %i and IsRealtimeProcess are %i", MachineType, IsRealtimeProcess);
                ret = -1;
            } else {
                IMAGE_DOS_HEADER DosHeader;
                IMAGE_NT_HEADERS32 NtHeader;
                IMAGE_NT_HEADERS64 NtHeader64;                    

                my_read (hFile, &DosHeader, sizeof (DosHeader));
                if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
                    my_close (hFile);
                    ThrowError (1, "file %s in not an executable", ExeName);
                    ret = -1;
                }
                my_lseek (hFile, (uint64_t)DosHeader.e_lfanew);
                my_read (hFile, &NtHeader, sizeof (NtHeader));
                if (NtHeader.Signature != IMAGE_NT_SIGNATURE) {
                    my_close (hFile);
                    ThrowError (1, "file %s in not an executable", ExeName);
                    ret = -1;
                }
                // is this a 64 bit executable?
                if (NtHeader.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
                    my_lseek (hFile, (uint64_t)DosHeader.e_lfanew);
                    my_read (hFile, &NtHeader64, sizeof (NtHeader64));
                    *RetCheckSum = NtHeader64.OptionalHeader.CheckSum;
                    if ((NtHeader64.OptionalHeader.MajorLinkerVersion == 2)
                        || (NtHeader64.OptionalHeader.MinorLinkerVersion == 25)) {
                        *Linker = LINKER_TYPE_BORLAND;
                    } else if ((NtHeader64.OptionalHeader.MajorLinkerVersion == 7)
                        || (NtHeader64.OptionalHeader.MinorLinkerVersion == 1)) {
                        *Linker = LINKER_TYPE_VISUALC7;
                    } else {
                        *Linker = LINKER_TYPE_VISUALC7;
                    }
                    ret = 0;
                } else {
                    *RetCheckSum = NtHeader.OptionalHeader.CheckSum;
                    if ((NtHeader.OptionalHeader.MajorLinkerVersion == 2)
                        || (NtHeader.OptionalHeader.MinorLinkerVersion == 25)) {
                        *Linker = LINKER_TYPE_BORLAND;
                    } else if ((NtHeader.OptionalHeader.MajorLinkerVersion == 7)
                        || (NtHeader.OptionalHeader.MinorLinkerVersion == 1)) {
                        *Linker = LINKER_TYPE_VISUALC7;
                    } else {
                        *Linker = LINKER_TYPE_VISUALC7;
                    }
                    ret = 0;
                }
            }
        }
    }
    if (hFile != MY_INVALID_HANDLE_VALUE) my_close (hFile);
    return ret;
}


int GetElfSectionVersionInfos (const char *par_Filename, const char *par_Section,
                               int *ret_OwnVersion,
                               int *ret_OwnMinorVersion,
                               int *ret_RemoteMasterLibraryAPIVersion)
{
    MY_FILE_HANDLE fh;
    int ReadResult;
    Elf32_Ehdr Ehdr32;
    Elf64_Ehdr Ehdr64;
    int LSBFirst;
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
    uint64_t VirtualAddresses;
    uint32_t VirtualSize;
    uint32_t SizeOfRawData;
    uint64_t Offset;
    struct OwnVersionInfosStruct OwnVersionInfos;
    const char *OwnVersionSignString = OWN_VERSION_SIGN_STRING;

    if ((sizeof(Elf32_Ehdr) != 0x34) ||
        (sizeof(Elf64_Shdr) != 0x40) ||
        (sizeof(Elf32_Shdr) != 0x28) ||
        (sizeof(Elf64_Shdr) != 0x40)) {
        ThrowError (1, "internal sizeof error %s %i", __FILE__, __LINE__);
    }

    fh = my_open(par_Filename);
    if (fh == MY_INVALID_HANDLE_VALUE) {
        ThrowError (1, "Unable to open %s file\n", par_Filename);
        return -1;
    }
    LogFileAccess (par_Filename);
    ReadResult = my_read (fh, &Ehdr32, sizeof (Ehdr32));
    if (ReadResult != sizeof (Ehdr32)) {
        ThrowError (1, "cannot read from file %s", par_Filename);
        return -1;
    }
    if ((Ehdr32.e_ident[0] != 0x7F) ||
        (Ehdr32.e_ident[1] != 'E') ||
        (Ehdr32.e_ident[2] != 'L') ||
        (Ehdr32.e_ident[3] != 'F')) {
        my_close (fh);
        ThrowError (1, "file %s in not an ELF executable", par_Filename);
        return -1;
    }
    if ((Ehdr32.e_ident[4] != 2) &&
        (Ehdr32.e_ident[4] != 1)) {
        my_close (fh);
        ThrowError (1, "file %s in not 32/64 bit ELF executable", par_Filename);
        return -1;
    }
    if (Ehdr32.e_ident[5] == 1) {
        LSBFirst = 1;
    } else if (Ehdr32.e_ident[5] == 2) {
        LSBFirst = 0;
        my_close (fh);
        ThrowError (1, "file %s in not a lsb first ELF executable (msb first are not supported)", par_Filename);
        return -1;
    }
    if (Ehdr32.e_ident[4] == 2) {
        my_lseek(fh, 0);
        ReadResult = my_read (fh, &Ehdr64, sizeof (Ehdr64));
        if (ReadResult != sizeof (Ehdr64)) {
            ThrowError (1, "cannot read from file %s", par_Filename);
            return -1;
        }
        NumOfSections = Ehdr64.e_shnum;
        SectionNameStringTable = Ehdr64.e_shstrndx;
        SectionHeaderOffset = Ehdr64.e_shoff;
        Is64Bit = 1;
    }
    else {
        NumOfSections = Ehdr32.e_shnum;
        SectionNameStringTable = Ehdr32.e_shstrndx;
        SectionHeaderOffset = Ehdr32.e_shoff;
        Is64Bit = 0;
    }

    if (Is64Bit) {
        my_lseek(fh, SectionHeaderOffset + SectionNameStringTable * sizeof(Shdr64));
        ReadResult = my_read(fh, &stShdr64, sizeof(stShdr64));
        if (ReadResult != sizeof(stShdr64)) {
            my_close(fh);
            ThrowError(1, "cannot read from file %s", par_Filename);
            return -1;
        }
        StringBaseOffset = stShdr64.sh_offset;
    } else {
        my_lseek(fh, SectionHeaderOffset + SectionNameStringTable * sizeof(Shdr32));
        ReadResult = my_read(fh, &stShdr32, sizeof(stShdr32));
        if (ReadResult != sizeof(stShdr32)) {
            my_close(fh);
            ThrowError(1, "cannot read from file %s", par_Filename);
            return -1;
        }
        StringBaseOffset = stShdr32.sh_offset;
    }
    for (i = 0; i < NumOfSections; i++) {
        char SectionName[256];
        char c;
        int x;

        SectionName[0] = 0;
        if (Is64Bit) {
            uint64_t file_pos = SectionHeaderOffset + i * sizeof(Shdr64);
            my_lseek(fh, file_pos);
            ReadResult = my_read(fh, &Shdr64, sizeof(Shdr64));
            if (ReadResult != sizeof(Shdr64)) {
                my_close(fh);
                ThrowError(1, "cannot read from file %s", par_Filename);
                return -1;
            }
            my_lseek(fh, StringBaseOffset + Shdr64.sh_name);
        } else {
            uint64_t file_pos = SectionHeaderOffset + i * sizeof(Shdr32);
            my_lseek(fh, file_pos);
            ReadResult = my_read(fh, &Shdr32, sizeof(Shdr32));
            if (ReadResult != sizeof(Shdr32)) {
                my_close(fh);
                ThrowError(1, "cannot read from file %s", par_Filename);
                return -1;
            }
            my_lseek(fh, StringBaseOffset + Shdr32.sh_name);
        }
        for (x = 0; x < (int)sizeof(SectionName); x++) {
            ReadResult = my_read(fh, &c, 1);
            if (ReadResult != 1) {
                my_close(fh);
                ThrowError(1, "cannot read from file %s", par_Filename);
                return -1;
            }
            SectionName[x] = c;
            if (c == 0) break;
        }

        if (!strcmp (SectionName, par_Section)) {
            if (Is64Bit) {
                VirtualAddresses = Shdr64.sh_addr;
                VirtualSize = (uint32_t)Shdr64.sh_size;
                SizeOfRawData = (uint32_t)Shdr64.sh_size;
                Offset = Shdr64.sh_offset;
            } else {
                VirtualAddresses = Shdr32.sh_addr;
                VirtualSize = Shdr32.sh_size;
                SizeOfRawData = Shdr32.sh_size;
                Offset = Shdr32.sh_offset;
            }

            if (SizeOfRawData < sizeof(struct OwnVersionInfosStruct)) {
                my_close(fh);
                ThrowError(1, "section %s inside file %s are smaler than %i (%i < %i)", par_Section, par_Filename, sizeof(struct OwnVersionInfosStruct), SizeOfRawData, sizeof(struct OwnVersionInfosStruct));
                return -1;
            }
            my_lseek(fh, Offset);
            ReadResult = my_read(fh, &OwnVersionInfos, sizeof(struct OwnVersionInfosStruct));
            if (ReadResult != sizeof(struct OwnVersionInfosStruct)) {
                my_close(fh);
                ThrowError(1, "cannot read section %s inside file %s)", par_Section, par_Filename);
                return -1;
            }
            if (strncmp(OwnVersionSignString, OwnVersionInfos.SignString, strlen(OwnVersionSignString))) {
                ThrowError(1, "missing sign string inside section %s inside file %s)", par_Section, par_Filename);
                return -1;

            }
            *ret_OwnVersion = OwnVersionInfos.Version;
            *ret_OwnMinorVersion = OwnVersionInfos.MinorVersion;
            *ret_RemoteMasterLibraryAPIVersion = OwnVersionInfos.RemoteMasterLibraryAPIVersion;
            my_close (fh);
            return 0;
        }
    }
    my_close (fh);
    ThrowError(1, "cannot find section %s inside file %s)", par_Section, par_Filename);
    return -1;
}
