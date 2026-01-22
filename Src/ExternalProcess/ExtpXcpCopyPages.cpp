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
#include <string.h>
#include "Platform.h"
#ifdef _WIN32
#include <process.h>
#endif

extern "C" {
#include "PrintFormatToString.h"
#include "StringMaxChar.h"
#include "ExtpBaseMessages.h"
}

#include "ExtpXcpCopyPages.h"

#define MinSectionSize(s) ((s.SizeOfRawData < s.Misc.VirtualSize) ? s.SizeOfRawData : s.Misc.VirtualSize)

/* Max sisze of a DWARF name */
#define MAXIMUM_DWARF_NAME_SIZE 64
/* Size of a symbol inside the COFF symbol table */
#define SYMBOL_SIZE   18

static const char *GetErrCodeToString (int ErrorCode)
{
#ifdef _WIN32
    static LPVOID lpMsgBuf;

    if (lpMsgBuf == NULL) LocalFree (lpMsgBuf);
    lpMsgBuf = NULL;
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   ErrorCode,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                   (LPTSTR) &lpMsgBuf,
                   0, NULL);
    if (lpMsgBuf == NULL) return "no explanation";
    return (char*)lpMsgBuf;
#else
    return strerror(errno);
#endif
}

typedef struct {
    unsigned char SegmentNo;
    unsigned char PageNo;
    unsigned long Size;
    void *Buffer;
} PAGE_IN_MEMORY;


static PAGE_IN_MEMORY *Pages;
static int NoOfPages;

static int CheckExistanceOfPage (unsigned char SegmentNo, unsigned char PageNo)
{
    int p;
    if (Pages == NULL) return -1;
    for (p = 0; p < NoOfPages; p++) {
        if ((Pages[p].SegmentNo == SegmentNo) && (Pages[p].PageNo == PageNo)) {
            return p;
        }
    }
    return -1;
}

int AddPageToCallibationSegment (unsigned char SegmentNo, unsigned char PageNo, long Size, void *Data)
{
    int p;

    if (Size <= 0) return -1;
    p = CheckExistanceOfPage (SegmentNo, PageNo);
    // Is it alread exists?
    if (p >= 0) return -1;
    // add new
    p = NoOfPages;
    NoOfPages++;
    Pages = (PAGE_IN_MEMORY*)realloc (Pages, sizeof (PAGE_IN_MEMORY) * NoOfPages);
    if (Pages == NULL) {
        ThrowError (1, "cannot allocate memory for page buffer");
        return -1;
    }
    Pages[p].SegmentNo = SegmentNo;
    Pages[p].PageNo = PageNo;
    Pages[p].Size = Size;
    Pages[p].Buffer = malloc (Size);
    if (Pages[p].Buffer == NULL) {
        ThrowError (1, "cannot allocate memory for page buffer");
        return -1;
    }
    if (Data != NULL) {
        MEMCPY (Pages[p].Buffer, Data, Size);
    } else {
        MEMSET (Pages[p].Buffer, 0, Size);
    }
    return 0;
}

void delete_all_page_files (void)
{
    int p;

    if (Pages != NULL) {
        for (p = 0; p < NoOfPages; p++) {
            if (Pages[p].Buffer != NULL) {
                free (Pages[p].Buffer);
            }
        }
        free (Pages);
    }
    Pages = NULL;
    NoOfPages = 0;
}

#define PEHEADER_OFFSET(a)  (a.e_lfanew      +	\
			 sizeof (DWORD)		     +	\
			 sizeof (IMAGE_FILE_HEADER))


#ifdef _WIN32
extern "C" int get_image_base_and_size (unsigned long *ret_base_address, unsigned long *ret_size)
{
    IMAGE_NT_HEADERS NtHeader;
    struct _IMAGE_DOS_HEADER DosHeader;
    char ExeFileName[MAX_PATH];
    HANDLE hFileExe;
    DWORD xx;

    GetModuleFileName (NULL, ExeFileName, sizeof (ExeFileName));
    hFileExe = CreateFile (ExeFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    if (hFileExe == (HANDLE)HFILE_ERROR) {
        ThrowError (1, "Unable to open %s file\n", ExeFileName);
        return -1;
    }
    ReadFile (hFileExe, &DosHeader, sizeof (DosHeader), &xx, NULL);
    if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        CloseHandle (hFileExe);
        ThrowError (1, "file %s in not an executable", ExeFileName);
        return -1;
    }
    SetFilePointer (hFileExe, DosHeader.e_lfanew, 0, FILE_BEGIN);
    ReadFile (hFileExe, &NtHeader, sizeof (NtHeader), &xx, NULL);
    if (NtHeader.Signature != IMAGE_NT_SIGNATURE) {
        CloseHandle (hFileExe);
        ThrowError (1, "file %s in not an executable", ExeFileName);
        return -1;
    }
    *ret_base_address = NtHeader.OptionalHeader.ImageBase;
    *ret_size = NtHeader.OptionalHeader.SizeOfImage;
    CloseHandle (hFileExe);
    return 0;
}
#endif

static int ExtractCopyExeBackImage (char *ret_TempPathBuffer, int par_Maxc)
{
    FILE *fh;
#ifdef _WIN32
    DWORD dwRetVal;
    char CmdPath[MAX_PATH];
    dwRetVal = GetTempPath (par_Maxc,
                            ret_TempPathBuffer);
    if (dwRetVal > MAX_PATH || (dwRetVal == 0)) {
        StringCopyMaxCharTruncate(ret_TempPathBuffer, "c:\\temp", par_Maxc);
    }
    GetSystemDirectory(CmdPath, sizeof(CmdPath));

    PrintFormatToString (ret_TempPathBuffer+strlen(ret_TempPathBuffer), par_Maxc - strlen(ret_TempPathBuffer), "%i_CopyExeBack.bat", GetCurrentProcessId());

    fh = fopen(ret_TempPathBuffer, "wt");
    if (fh == NULL)  {
        ThrowError (1, "cannot open batch file %s", ret_TempPathBuffer);
        return -1;
    }
    fprintf (fh, "echo %%1 %%2 %%3\n"
                 "set /A LoopCounter=0\n"
                 ":LOOP1\n"
                 "%s\\taskkill /PID %%1\n"
                 "if %%errorlevel%% == 128 goto PROCESS_NOT_RUNNING\n"
                 "if %%LoopCounter%% == 100 goto ERROR_OUT\n"
                 "set /A LoopCounter+=1\n"
                 "goto LOOP1\n"
                 ":PROCESS_NOT_RUNNING\n"
                 "set /A LoopCounter=0\n"
                 ":LOOP2\n"
                 "copy %%2 %%3\n"
                 "if %%errorlevel%% == 0 goto PROCESS_RELEASE_EXE\n"
                 "if %%LoopCounter%% == 100 goto ERROR_OUT\n"
                 "set /A LoopCounter+=1\n"
                 "goto LOOP2\n"
                 ":PROCESS_RELEASE_EXE\n"
                 ":ERROR_OUT\n",
                 CmdPath);
    fclose (fh);
#else
    char const *Folder = getenv("TMPDIR");
    if (Folder == NULL) {
        Folder = "/tmp";
    }
    StringCopyMaxCharTruncate (ret_TempPathBuffer, Folder, par_Maxc);
    PrintFormatToString (ret_TempPathBuffer+strlen(ret_TempPathBuffer), par_Maxc - strlen(ret_TempPathBuffer), "/%i_CopyExeBack.sh", getpid());

    fh = fopen(ret_TempPathBuffer, "wt");
    if (fh == NULL)  {
        ThrowError (1, "cannot open batch file %s", ret_TempPathBuffer);
        return -1;
    }
    fprintf (fh, "echo $1 $2 $3\n"
                 "for ((i=0; i<100; ++i)); do\n"
                 "    [ -e /proc/$1/cmdline ] && break\n"
                 "    sleep 0.1\n"
                 "done\n"
                 "echo \"process was stopped\"\n"
                 "for ((i=0; i<100; ++i)); do\n"
                 "    if cp $2 $3 ; then\n"
                 "        echo \"success\"\n"
                 "        break\n"
                 "    else\n"
                 "        echo \"failed\"\n"
                 "    fi\n"
                 "    sleep 0.1\n"
                 "done\n");
    fclose (fh);
#endif
    return 0;
}

static int copy_exe_file_back_after_terminate (char *CopyExeBackImagePath, char *ExeDst, char *ExeSrc)
{
    #ifdef _WIN32
    char CommandLine[MAX_PATH * 3 + 100];
    DWORD dwRetVal;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcInfo;
    char TempPathBuffer[MAX_PATH];
    char CmdPath[MAX_PATH];

    dwRetVal = GetTempPath (MAX_PATH,
                            TempPathBuffer);
    if (dwRetVal > MAX_PATH || (dwRetVal == 0)) {
        STRING_COPY_TO_ARRAY (TempPathBuffer, "c:\\temp");
    }

    GetStartupInfo (&StartupInfo);

    PrintFormatToString (CommandLine, sizeof(CommandLine), "/C %s 0x%X \"%s\" \"%s\"", CopyExeBackImagePath, (unsigned long)GetCurrentProcessId(), ExeSrc, ExeDst);

    GetSystemDirectory(CmdPath, sizeof(CmdPath) - 8);
    STRING_APPEND_TO_ARRAY(CmdPath, "\\cmd.exe");
    if (!CreateProcess (CmdPath,
                        CommandLine, NULL, NULL,
                        TRUE, 0, NULL, NULL,
                        &StartupInfo, &ProcInfo)) {
        char *lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       dw,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &lpMsgBuf,
                       0, NULL );
        ThrowError (1, "cannot start copy back executable batch file %s (%s)", CommandLine, lpMsgBuf);
        LocalFree (lpMsgBuf);
        return -1;
    }
#else
    char OwnPidString[32];
    PrintFormatToString (OwnPidString, sizeof(OwnPidString), "%i", getpid());
    int Pid = fork();
    if (Pid == 0) {
        execl("/bin/bash", "bash", CopyExeBackImagePath, OwnPidString, ExeSrc, ExeDst, NULL);
        perror("Launch bash has failed");
    }
#endif

    return 0;
}

static int write_section_to_exe_file_after_terminate (char *ExecutableName, char *SelectedSection,
                                                      char *TargetExe, int TargetExe_Maxc,
                                                      char *TempExe, int TempExe_Maxc,
                                                      unsigned long long BaseAddress)
{
    char TempPathBuffer[MAX_PATH];
    char Help[100];
    char *ExeName;
    int FoundSectionCounter = 0;

#ifdef _WIN32
    char SectionName[MAXIMUM_DWARF_NAME_SIZE];
    IMAGE_NT_HEADERS NtHeader;
    IMAGE_NT_HEADERS64 NtHeader64;
    WORD NumberOfSections;
    LONGLONG GccSymbolOffset;
    struct _IMAGE_DOS_HEADER DosHeader;
    uint64_t BaseAddr;
    struct _IMAGE_SECTION_HEADER SectionHeader;
    HANDLE hFileExe;
    DWORD x, xx, i, dwRetVal;
    DWORD SectionSize;
#else
    int fh;
    ssize_t ReadResult;
    Elf32_Ehdr Ehdr32;
    Elf64_Ehdr Ehdr64;
    int LSBFirst;
    unsigned int NumOfSections;
    unsigned int SectionNameStringTable;
    unsigned int SectionHeaderOffset;
    uint64_t StringBaseOffset;
    Elf32_Shdr Shdr32;
    Elf32_Shdr stShdr32;
    Elf64_Shdr Shdr64;
    Elf64_Shdr stShdr64;
    unsigned int i;
    int Is64Bit;
#endif

    void *Address;

    if (strlen (TempExe) == 0) {
        // First time a section will be write back to the executable
        StringCopyMaxCharTruncate (TargetExe, ExecutableName, TargetExe_Maxc);

        ExeName = TargetExe;
        while (*ExeName != 0) ExeName++;
        ExeName--;
#ifdef _WIN32
        while ((*ExeName != '\\') && (ExeName > TargetExe)) ExeName--;
#else
        while ((*ExeName != '/') && (ExeName > TargetExe)) ExeName--;
#endif
        ExeName++;

        EXTERN_PROCESS_INFOS_STRUCT *ProcessInfos = XilEnvInternal_GetGlobalProcessInfos();
        if (strlen(ProcessInfos->WriteBackExeToDir)) {
            // There was defined a path where the temp EXE file should be writen with the parameter -WriteBackExeToDir
            StringCopyMaxCharTruncate(TempExe, ProcessInfos->WriteBackExeToDir, TempExe_Maxc);
#ifdef _WIN32
            if (TempExe[strlen(TempExe)-1] != '\\') {
                StringAppendMaxCharTruncate (TempExe, "\\", TempExe_Maxc);
            }
#else
            if (TempExe[strlen(TempExe)-1] != '/') StringAppendMaxCharTruncate (TempExe, "/", TempExe_Maxc);
#endif
        } else {
            // If not then this should be written to the system temporary folder "c:\temp" with the name <PID>_Name.EXE bzw <PID>_Name.DLL
#ifdef _WIN32
            dwRetVal = GetTempPath (MAX_PATH,          // length of the buffer
                                    TempPathBuffer); // buffer for path
            if (dwRetVal > MAX_PATH || (dwRetVal == 0)) {
                STRING_COPY_TO_ARRAY (TempPathBuffer, "c:\\temp");
            }
#else
            STRING_COPY_TO_ARRAY(TempPathBuffer, "/tmp/");
#endif
            StringCopyMaxCharTruncate (TempExe, TempPathBuffer, TempExe_Maxc);
#ifdef _WIN32
            PrintFormatToString (Help, sizeof(Help), "%i_", (unsigned long)GetCurrentProcessId());
#else
            PrintFormatToString (Help, sizeof(Help), "%i_", getpid());
#endif
            StringAppendMaxCharTruncate (TempExe, Help, TempExe_Maxc);
        }
        StringAppendMaxCharTruncate (TempExe, ExeName, TempExe_Maxc);

        if (!CopyFile (TargetExe, TempExe, FALSE)) {
            ThrowError (1, "Unable to copy file \"%s\" to file \"%s\" (1)\n", TargetExe, TempExe);
            StringCopyMaxCharTruncate (TargetExe, "", TargetExe_Maxc);
            StringCopyMaxCharTruncate (TempExe, "", TempExe_Maxc);
            return 0;
        }
    }
#ifdef _WIN32
    hFileExe = CreateFile (TempExe,
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    if (hFileExe == (HANDLE)HFILE_ERROR) {
        ThrowError (1, "Unable to open %s file\n", TempExe);
        StringCopyMaxCharTruncate (TargetExe, "", TargetExe_Maxc);
        StringCopyMaxCharTruncate (TempExe, "", TempExe_Maxc);
        return 0;
    }
    ReadFile (hFileExe, &DosHeader, sizeof (DosHeader), &xx, NULL);
    if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        CloseHandle (hFileExe);
        ThrowError (1, "file %s in not an executable", TempExe);
        StringCopyMaxCharTruncate (TargetExe, "", TargetExe_Maxc);
        StringCopyMaxCharTruncate (TempExe, "", TempExe_Maxc);
        return 0;
    }
    SetFilePointer (hFileExe, DosHeader.e_lfanew, 0, FILE_BEGIN);
    ReadFile (hFileExe, &NtHeader, sizeof (NtHeader), &xx, NULL);
    if (NtHeader.Signature != IMAGE_NT_SIGNATURE) {
        CloseHandle (hFileExe);
        ThrowError (1, "file %s in not an executable", TempExe);
        StringCopyMaxCharTruncate (TargetExe, "", TargetExe_Maxc);
        StringCopyMaxCharTruncate (TempExe, "", TempExe_Maxc);
        return 0;
    }
    // is it a 64 bit EXE
    if (NtHeader.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
        LARGE_INTEGER Offset;
        Offset.QuadPart = (uint64_t)DosHeader.e_lfanew;
        SetFilePointerEx (hFileExe, Offset, NULL, FILE_BEGIN);
        ReadFile (hFileExe, &NtHeader64, sizeof (NtHeader64), &xx, NULL);
        BaseAddr = NtHeader64.OptionalHeader.ImageBase;
        NumberOfSections = NtHeader64.FileHeader.NumberOfSections;
        GccSymbolOffset = NtHeader64.FileHeader.PointerToSymbolTable + (NtHeader64.FileHeader.NumberOfSymbols * SYMBOL_SIZE);
    } else {
        BaseAddr = NtHeader.OptionalHeader.ImageBase;
        NumberOfSections = NtHeader.FileHeader.NumberOfSections;
        GccSymbolOffset = NtHeader.FileHeader.PointerToSymbolTable + (NtHeader.FileHeader.NumberOfSymbols * SYMBOL_SIZE);
    }

    for (x = 0; x < NumberOfSections; x++) {
        ReadFile (hFileExe, &SectionHeader, sizeof (struct _IMAGE_SECTION_HEADER), &xx, NULL);

        // GCC will use longer as 8 byte section names
        if (SectionHeader.Name[0] == '/') {
            LARGE_INTEGER SaveFilePos;
            LARGE_INTEGER VirtualOffset;
            LARGE_INTEGER Help;
            DWORD BytesRead;
            char c;
            int i;

            VirtualOffset.QuadPart = GccSymbolOffset +
                                     atoi ((char*)SectionHeader.Name+1);
            Help.QuadPart = 0;
            SetFilePointerEx (hFileExe, Help, &SaveFilePos, FILE_CURRENT);
	        SetFilePointerEx (hFileExe, VirtualOffset, NULL, FILE_BEGIN);
            for (i = 0; i < MAXIMUM_DWARF_NAME_SIZE; i++) {
                ReadFile (hFileExe, &c, 1, &BytesRead, NULL);
                SectionName[i] = c;
                if (c == 0) break;
            }
            SetFilePointerEx (hFileExe, SaveFilePos, NULL, FILE_BEGIN);
        } else {
            MEMCPY (SectionName, SectionHeader.Name, 8);
            SectionName[8] = 0; // 0 termination
        }
        if (!strlen (SectionName)) {
            continue;
        }
        if (!strcmp (SectionName, SelectedSection)) {
            LARGE_INTEGER SaveFilePos;
            LARGE_INTEGER Help;

            Address = (void*)(SectionHeader.VirtualAddress + BaseAddress);

            SectionSize = MinSectionSize(SectionHeader); 

            Help.QuadPart = 0;
            Help.QuadPart = SectionHeader.PointerToRawData;
            SetFilePointerEx (hFileExe, Help, &SaveFilePos, FILE_BEGIN);

            for (i = 0; i < SectionSize; i += xx) {
                if (!WriteFile (hFileExe, (unsigned char *)Address + i,
                    SectionSize - i, &xx, NULL)) {
                    char *lpMsgBuf;
                    DWORD dw = GetLastError();
                    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                   FORMAT_MESSAGE_FROM_SYSTEM |
                                   FORMAT_MESSAGE_IGNORE_INSERTS,
                                   NULL,
                                   dw,
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                   (LPTSTR) &lpMsgBuf,
                                   0, NULL );
                    CloseHandle (hFileExe);
                    ThrowError (1, "cannot write to executable file %s (%s)", TempExe, lpMsgBuf);
                    LocalFree (lpMsgBuf);
                    StringCopyMaxCharTruncate (TargetExe, "", TargetExe_Maxc);
                    StringCopyMaxCharTruncate (TempExe, "", TempExe_Maxc);
                    return 0;
                }
            }
            SetFilePointerEx (hFileExe, SaveFilePos, NULL, FILE_BEGIN);

            FoundSectionCounter++;
        }
    }

    CloseHandle (hFileExe);

#else

    fh = open(TempExe, O_RDWR);
    //fh = open(TempExe, O_RDWR);
    if (fh < 0) {
        ThrowError(1, "Unable to open %s file (2)\n", TempExe);
        return -1;
    }
    ReadResult = read(fh, &Ehdr32, sizeof(Ehdr32));
    if (ReadResult != sizeof(Ehdr32)) {
        ThrowError(1, "cannot read from file %s", TempExe);
        return -1;
    }
    if ((Ehdr32.e_ident[0] != 0x7F) ||
    (Ehdr32.e_ident[1] != 'E') ||
    (Ehdr32.e_ident[2] != 'L') ||
    (Ehdr32.e_ident[3] != 'F')) {
        close(fh);
        ThrowError(1, "file %s in not an ELF executable", TempExe);
        return -1;
    }
    if ((Ehdr32.e_ident[4] != 2) &&
    (Ehdr32.e_ident[4] != 1)) {
        close(fh);
        ThrowError(1, "file %s in not 32/64 bit ELF executable", TempExe);
        return -1;
    }
    if (Ehdr32.e_ident[5] == 1) {
        LSBFirst = 1;
    }
    else if (Ehdr32.e_ident[5] == 2) {
        LSBFirst = 0;
        close(fh);
        ThrowError(1, "file %s in not a lsb first ELF executable (msb first are not supported)", TempExe);
        return -1;
    }
    if (Ehdr32.e_ident[4] == 2) {
        lseek(fh, 0, SEEK_SET);
        ReadResult = read(fh, &Ehdr64, sizeof(Ehdr64));
        if (ReadResult != sizeof(Ehdr64)) {
            close(fh);
            ThrowError(1, "cannot read from file %s", TempExe);
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
        lseek(fh, SectionHeaderOffset + SectionNameStringTable * sizeof(Shdr64), SEEK_SET);
        ReadResult = read(fh, &stShdr64, sizeof(stShdr64));
        if (ReadResult != sizeof(stShdr64)) {
            close(fh);
            ThrowError(1, "cannot read from file %s", TempExe);
            return -1;
        }
        StringBaseOffset = stShdr64.sh_offset;
    }
    else {
        lseek(fh, SectionHeaderOffset + SectionNameStringTable * sizeof(Shdr32), SEEK_SET);
        ReadResult = read(fh, &stShdr32, sizeof(stShdr32));
        if (ReadResult != sizeof(stShdr32)) {
            close(fh);
            ThrowError(1, "cannot read from file %s", TempExe);
            return -1;
        }
        StringBaseOffset = stShdr32.sh_offset;
    }
    for (i = 0; i < NumOfSections; i++) {
        char SectionName[256];
        char c;
        int x;

        if (Is64Bit) {
            int file_pos = SectionHeaderOffset + i * sizeof(Shdr64);
            lseek(fh, file_pos, SEEK_SET);
            ReadResult = read(fh, &Shdr64, sizeof(Shdr64));
            if (ReadResult != sizeof(Shdr64)) {
                close(fh);
                ThrowError(1, "cannot read from file %s", TempExe);
                return -1;
            }
            lseek(fh, StringBaseOffset + Shdr64.sh_name, SEEK_SET);
        } else {
            int file_pos = SectionHeaderOffset + i * sizeof(Shdr32);
            lseek(fh, file_pos, SEEK_SET);
            ReadResult = read(fh, &Shdr32, sizeof(Shdr32));
            if (ReadResult != sizeof(Shdr32)) {
                close(fh);
                ThrowError(1, "cannot read from file %s", TempExe);
                return -1;
            }
            lseek(fh, StringBaseOffset + Shdr32.sh_name, SEEK_SET);
        }
        for (x = 0; x < (int)sizeof(SectionName); x++) {
            ReadResult = read(fh, &c, 1);
            if (ReadResult != 1) {
                close(fh);
                ThrowError(1, "cannot read from file %s", TempExe);
                return -1;
            }
            SectionName[x] = c;
            if (c == 0) break;
        }
        if (!strcmp(SectionName, SelectedSection)) {
            int Size;
            if (Is64Bit) {
                Size = Shdr64.sh_size;
                Address = (void*)Shdr64.sh_addr;
                lseek(fh, Shdr64.sh_offset, SEEK_SET);
            } else {
                Size = Shdr32.sh_size;
                Address = (void*)(size_t)Shdr32.sh_addr;
                lseek(fh, Shdr32.sh_offset, SEEK_SET);
            }

            if (write(fh, Address, Size) != Size) {
                ThrowError(1, "cannot write to executable file %s", TempExe);
                StringCopyMaxCharTruncate (TargetExe, "", TargetExe_Maxc);
                StringCopyMaxCharTruncate (TempExe, "", TempExe_Maxc);
                close(fh);
                return 0;
            }
            FoundSectionCounter++;
        }
    }
    close(fh);
#endif

    if (FoundSectionCounter == 0) {
        ThrowError (1, "error section %s not found\n", SelectedSection);
        StringCopyMaxCharTruncate (TargetExe, "", TargetExe_Maxc);
        StringCopyMaxCharTruncate (TempExe, "", TempExe_Maxc);
        return 0;
    } else {
        return 1;
    }
}

typedef struct {
    char ShortName[MAXIMUM_DWARF_NAME_SIZE];
    char LongName[MAX_PATH + 2 + MAXIMUM_DWARF_NAME_SIZE];
    int TaskNumber;
    int IsStandardSection;
    int IsSetAsCallibation;
    int IsSetAsCode;
    int IsSetAsWriteable;
    void *StartAddress;
    int Size;
    uint64_t FileOffset;
} SECTION_LIST_ELEM;

static SECTION_LIST_ELEM *SectionInfoArray;
static int SectionInfoArraySize;


static int XilEnvInternal_AddSectionToSectionArray(const char * par_SectionPrefix, const char *par_SectionName, 
                                                    void *par_Address, int par_Size, uint64_t par_FileOffset,
                                                    int par_IsSetAsCallibation, int par_IsSetAsWriteable,
                                                    const char *par_ExecutableFileName, int par_TaskNumber)
{
    if (!strlen(par_SectionName)) {
        // Ignore section without name
        return 0;
    } else {
        SectionInfoArray = (SECTION_LIST_ELEM*)realloc(SectionInfoArray, sizeof(SECTION_LIST_ELEM) * (SectionInfoArraySize + 1));
        if (SectionInfoArray == NULL) {
            SectionInfoArraySize = -1;
            ThrowError(1, "out of memory. Cannot load section infos oof executable \"%s\" into memory", par_ExecutableFileName);
            return -1;
        }
        if (par_SectionPrefix != NULL) {
            STRING_COPY_TO_ARRAY(SectionInfoArray[SectionInfoArraySize].LongName, par_SectionPrefix);
            STRING_APPEND_TO_ARRAY(SectionInfoArray[SectionInfoArraySize].LongName, "::");
            STRING_APPEND_TO_ARRAY(SectionInfoArray[SectionInfoArraySize].LongName, par_SectionName);
            SectionInfoArray[SectionInfoArraySize].TaskNumber = par_TaskNumber;
        }
        else {
            STRING_COPY_TO_ARRAY(SectionInfoArray[SectionInfoArraySize].LongName, par_SectionName);
            SectionInfoArray[SectionInfoArraySize].TaskNumber = -1;
        }
        STRING_COPY_TO_ARRAY(SectionInfoArray[SectionInfoArraySize].ShortName, par_SectionName);
        if (!strcmp(par_SectionName, ".text") ||
            !strcmp(par_SectionName, ".rdata") ||
            !strcmp(par_SectionName, ".data") ||
            !strcmp(par_SectionName, ".xdata") ||
            !strcmp(par_SectionName, ".rsrc") ||
            // Additionaly this for GCC
            !strcmp(par_SectionName, ".eh_frame") ||
            !strcmp(par_SectionName, ".bss") ||
            !strcmp(par_SectionName, ".idata") ||
            !strcmp(par_SectionName, ".CRT") ||
            !strcmp(par_SectionName, ".tls") ||
            !strcmp(par_SectionName, ".debug_aranges") ||
            !strcmp(par_SectionName, ".debug_pubnames") ||
            !strcmp(par_SectionName, ".debug_pubtypes") ||
            !strcmp(par_SectionName, ".debug_info") ||
            !strcmp(par_SectionName, ".debug_abbrev") ||
            !strcmp(par_SectionName, ".debug_line") ||
            !strcmp(par_SectionName, ".debug_frame") ||
            !strcmp(par_SectionName, ".debug_str") ||
            !strcmp(par_SectionName, ".debug_loc") ||
            !strcmp(par_SectionName, ".debug_macinfo")) {
            SectionInfoArray[SectionInfoArraySize].IsStandardSection = 1;
        } else {
            SectionInfoArray[SectionInfoArraySize].IsStandardSection = 0;
        }
        SectionInfoArray[SectionInfoArraySize].IsSetAsCallibation = par_IsSetAsCallibation;
        SectionInfoArray[SectionInfoArraySize].IsSetAsWriteable = par_IsSetAsWriteable;
        SectionInfoArray[SectionInfoArraySize].Size = par_Size;
        SectionInfoArray[SectionInfoArraySize].FileOffset = par_FileOffset;
        SectionInfoArray[SectionInfoArraySize].IsSetAsCode = 0;
        SectionInfoArray[SectionInfoArraySize].StartAddress = par_Address;

        SectionInfoArraySize++;
        return 0;
    }
}


static int XilEnvInternal_ReadSectionInfosFormOneExecutableFile (int TaskNumber, char *par_ExecutableFileName, void *par_BaseAddress, char *par_SectionPrefix)
{
#ifdef _WIN32
    IMAGE_NT_HEADERS NtHeader;
    struct _IMAGE_DOS_HEADER DosHeader;
    struct _IMAGE_SECTION_HEADER SectionHeader;
    char SectionName[MAXIMUM_DWARF_NAME_SIZE];
    int FoundSectionCounter = 0;
    HANDLE hFileExe;
    DWORD x, xx;
    DWORD SectionSize;

    hFileExe = CreateFile (par_ExecutableFileName,
                           GENERIC_READ,
                           FILE_SHARE_READ,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
    if (hFileExe == (HANDLE)HFILE_ERROR) {
        ThrowError (1, "Unable to open %s file\n", par_ExecutableFileName);
        return -1;
    }
    ReadFile (hFileExe, &DosHeader, sizeof (DosHeader), &xx, NULL);
    if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        CloseHandle (hFileExe);
        ThrowError (1, "file %s in not an executable", par_ExecutableFileName);
        return -1;
    }
    SetFilePointer (hFileExe, DosHeader.e_lfanew, 0, FILE_BEGIN);
    ReadFile (hFileExe, &NtHeader, sizeof (NtHeader), &xx, NULL);
    if (NtHeader.Signature != IMAGE_NT_SIGNATURE) {
        CloseHandle (hFileExe);
        ThrowError (1, "file %s in not an executable", par_ExecutableFileName);
        return -1;
    }
    for (x = 0; x < NtHeader.FileHeader.NumberOfSections; x++) {
        ReadFile (hFileExe, &SectionHeader, sizeof (struct _IMAGE_SECTION_HEADER), &xx, NULL);

        // GCC will use longer as 8 byte section names
        if (SectionHeader.Name[0] == '/') {
            LARGE_INTEGER SaveFilePos;
            LARGE_INTEGER VirtualOffset;
            LARGE_INTEGER Help;
            DWORD BytesRead;
            char c;
            int i;

            VirtualOffset.QuadPart = NtHeader.FileHeader.PointerToSymbolTable +
                                     (NtHeader.FileHeader.NumberOfSymbols * SYMBOL_SIZE) +
                                     atoi ((char*)SectionHeader.Name+1);
            Help.QuadPart = 0;
            SetFilePointerEx (hFileExe, Help, &SaveFilePos, FILE_CURRENT);
	        SetFilePointerEx (hFileExe, VirtualOffset, NULL, FILE_BEGIN);
            for (i = 0; i < MAXIMUM_DWARF_NAME_SIZE; i++) {
                ReadFile (hFileExe, &c, 1, &BytesRead, NULL);
                SectionName[i] = c;
                if (c == 0) break;
            }
            SectionName[MAXIMUM_DWARF_NAME_SIZE - 1] = 0;
            SetFilePointerEx (hFileExe, SaveFilePos, NULL, FILE_BEGIN);
        } else {
            MEMCPY (SectionName, SectionHeader.Name, 8);
            SectionName[8] = 0; // 0 termination
        }
        SectionSize = MinSectionSize(SectionHeader); 
        if (XilEnvInternal_AddSectionToSectionArray(par_SectionPrefix, SectionName,
            (void*)(SectionHeader.VirtualAddress + (char*)par_BaseAddress), 
            SectionSize,
            (uint64_t)SectionHeader.PointerToRawData,
            ((SectionHeader.Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA) == IMAGE_SCN_CNT_INITIALIZED_DATA),
            ((SectionHeader.Characteristics & IMAGE_SCN_MEM_WRITE) == IMAGE_SCN_MEM_WRITE),
            par_ExecutableFileName, TaskNumber)) {
            return - 1;
        }
    }
    CloseHandle (hFileExe);
#else
    int fh;
    ssize_t ReadResult;
    Elf32_Ehdr Ehdr32;
    Elf64_Ehdr Ehdr64;
    int LSBFirst;
    unsigned int NumOfSections;
    unsigned int SectionNameStringTable;
    unsigned int SectionHeaderOffset;
    uint64_t StringBaseOffset;
    Elf32_Shdr Shdr32;
    Elf32_Shdr stShdr32;
    Elf64_Shdr Shdr64;
    Elf64_Shdr stShdr64;
    unsigned int i;
    int Is64Bit;

    fh = open(par_ExecutableFileName, O_RDONLY);
    if (fh < 0) {
        ThrowError(1, "Unable to open %s file (3)\n", par_ExecutableFileName);
        return -1;
    }
    ReadResult = read(fh, &Ehdr32, sizeof(Ehdr32));
    if (ReadResult != sizeof(Ehdr32)) {
        ThrowError(1, "cannot read from file %s", par_ExecutableFileName);
        return -1;
    }
    if ((Ehdr32.e_ident[0] != 0x7F) ||
    (Ehdr32.e_ident[1] != 'E') ||
    (Ehdr32.e_ident[2] != 'L') ||
    (Ehdr32.e_ident[3] != 'F')) {
        close(fh);
        ThrowError(1, "file %s in not an ELF executable", par_ExecutableFileName);
        return -1;
    }
    if ((Ehdr32.e_ident[4] != 2) &&
    (Ehdr32.e_ident[4] != 1)) {
        close(fh);
        ThrowError(1, "file %s in not 32/64 bit ELF executable", par_ExecutableFileName);
        return -1;
    }
    if (Ehdr32.e_ident[5] == 1) {
        LSBFirst = 1;
    }
    else if (Ehdr32.e_ident[5] == 2) {
        LSBFirst = 0;
        close(fh);
        ThrowError(1, "file %s in not a lsb first ELF executable (msb first are not supported)", par_ExecutableFileName);
        return -1;
    }
    if (Ehdr32.e_ident[4] == 2) {
        lseek(fh, 0, SEEK_SET);
        ReadResult = read(fh, &Ehdr64, sizeof(Ehdr64));
        if (ReadResult != sizeof(Ehdr64)) {
            close(fh);
            ThrowError(1, "cannot read from file %s", par_ExecutableFileName);
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
    lseek(fh, SectionHeaderOffset + SectionNameStringTable * sizeof(Shdr64), SEEK_SET);
    if (Is64Bit) {
        ReadResult = read(fh, &stShdr64, sizeof(stShdr64));
        if (ReadResult != sizeof(stShdr64)) {
            close(fh);
            ThrowError(1, "cannot read from file %s", par_ExecutableFileName);
            return -1;
        }
        StringBaseOffset = stShdr64.sh_offset;
    } else {
        ReadResult = read(fh, &stShdr32, sizeof(stShdr32));
        if (ReadResult != sizeof(stShdr32)) {
            close(fh);
            ThrowError(1, "cannot read from file %s", par_ExecutableFileName);
            return -1;
        }
        StringBaseOffset = stShdr32.sh_offset;
    }
    for (i = 0; i < NumOfSections; i++) {
        char SectionName[256];
        char *p;
        char c;
        int x, b;
        int xx;
        void *Address;
        int Size;
        uint64_t FileOffset;
        int IsSetAsWriteable;
        int IsSetAsCode;

        if (Is64Bit) {
            int file_pos = SectionHeaderOffset + i * sizeof(Shdr64);
            lseek(fh, file_pos, SEEK_SET);
            ReadResult = read(fh, &Shdr64, sizeof(Shdr64));
            if (ReadResult != sizeof(Shdr64)) {
                close(fh);
                ThrowError(1, "cannot read from file %s", par_ExecutableFileName);
                return -1;
            }
            lseek(fh, StringBaseOffset + Shdr64.sh_name, SEEK_SET);
        } else {
            int file_pos = SectionHeaderOffset + i * sizeof(Shdr32);
            lseek(fh, file_pos, SEEK_SET);
            ReadResult = read(fh, &Shdr32, sizeof(Shdr32));
            if (ReadResult != sizeof(Shdr32)) {
                close(fh);
                ThrowError(1, "cannot read from file %s", par_ExecutableFileName);
                return -1;
            }
            lseek(fh, StringBaseOffset + Shdr32.sh_name, SEEK_SET);
        }
        for (x = 0; x < (int)sizeof(SectionName); x++) {
            ReadResult = read(fh, &c, 1);
            if (ReadResult != 1) {
                close(fh);
                ThrowError(1, "cannot read from file %s", par_ExecutableFileName);
                return -1;
            }
            SectionName[x] = c;
            if (c == 0) break;
        }
        if (Is64Bit) {
            Size = Shdr64.sh_size;
            Address = (void*)Shdr64.sh_addr;
            FileOffset = Shdr64.sh_offset;
            IsSetAsCode = (Shdr64.sh_flags & SHF_EXECINSTR) == SHF_EXECINSTR;
            IsSetAsWriteable = (Shdr64.sh_flags & SHF_WRITE) == SHF_WRITE;
        }
        else {
            Size = Shdr32.sh_size;
            Address = (void*)(size_t)Shdr32.sh_addr;
            FileOffset = Shdr32.sh_offset;
            IsSetAsCode = (Shdr32.sh_flags & SHF_EXECINSTR) == SHF_EXECINSTR;
            IsSetAsWriteable = (Shdr32.sh_flags & SHF_WRITE) == SHF_WRITE;
        }

        if (XilEnvInternal_AddSectionToSectionArray(par_SectionPrefix, SectionName, Address, Size, 
            IsSetAsCode, IsSetAsWriteable, FileOffset,
            par_ExecutableFileName, TaskNumber)) {
            return - 1;
        }
    }
    close(fh);

#endif
    return 0;
}

extern "C"
int XilEnvInternal_ReadSectionInfosFromAllExecutable (void)
{
    int x;
    char ExeFileName[MAX_PATH];

    EXTERN_PROCESS_INFOS_STRUCT *ProcessInfos = XilEnvInternal_GetGlobalProcessInfos ();

    // First read all section from the EXE file
    GetModuleFileName (NULL, ExeFileName, MAX_PATH);
    if (XilEnvInternal_ReadSectionInfosFormOneExecutableFile (-1, ExeFileName, ProcessInfos->ExecutableBaseAddress, NULL)) {
        return 0;
    }

    // Than read all sections from the DLL(s) with DLL-Name without folder as rrefix
    for (x = 0; x < ProcessInfos->ThreadCount; x++) {
        if (strlen (ProcessInfos->TasksInfos[x]->DllName)) {   // is process inside a DLL
            char DllFileName[MAX_PATH];
            char *p = ProcessInfos->TasksInfos[x]->DllName;
            while (*p != 0) p++;
            while ((p > ProcessInfos->TasksInfos[x]->DllName) && (*p != '/') && (*p != '\\')) p--;
            if ((*p != '/') || (*p != '\\')) p++;
            STRING_COPY_TO_ARRAY (DllFileName, p);
#ifdef _WIN32
            _strupr (DllFileName);    // Only upper-case letter
#else
#endif
            if (XilEnvInternal_ReadSectionInfosFormOneExecutableFile (x, ProcessInfos->TasksInfos[x]->DllName, ProcessInfos->TasksInfos[x]->DllBaseAddress, DllFileName)) {
                return 0;
            }
        } else {
            // do nothing
        }
    }

    if (SectionInfoArraySize == 0) {
        ThrowError (1, "error no section found in executable file \"%s\"\n", ExeFileName);
    }
    return SectionInfoArraySize;
}

int get_number_of_sections (void)
{
    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();
    return SectionInfoArraySize;
}

char* get_sections_name (int Number)
{
    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return NULL;
    else return SectionInfoArray[Number].LongName;
}

char* get_sections_short_name (int Number)
{
    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return nullptr;
    else return SectionInfoArray[Number].ShortName;
}

int get_sections_task_number(int Number)
{
    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return 0;
    else return SectionInfoArray[Number].TaskNumber;
}

long get_sections_size (int Number)
{
    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return -1;
    else return SectionInfoArray[Number].Size;
}

void *get_sections_start_address (int Number)
{
    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return NULL;
    else return SectionInfoArray[Number].StartAddress;
}

int is_sections_callibation_able (int Number)
{
    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();
    if (Number >= SectionInfoArraySize) return 0;
    else return SectionInfoArray[Number].IsSetAsCallibation;
}

uint64_t get_sections_file_offset(int Number)
{
    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable();
    if (Number >= SectionInfoArraySize) return -1;
    else return SectionInfoArray[Number].FileOffset;

}

// Declaration of this function is inside OpenXilEnvExtp.h
extern "C" {
EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__  get_section_addresse_size (const char *par_section_name, unsigned long long *ret_start_address, unsigned long *ret_size)
{
    int x;
    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();
    for (x = 0; x < SectionInfoArraySize; x++) {
        if (!strcmp (par_section_name, SectionInfoArray[x].LongName)) {
            *ret_start_address = (unsigned long long)SectionInfoArray[x].StartAddress;
            *ret_size = SectionInfoArray[x].Size;
            return 0;
        }
    }
    return -1;
}
}

int find_section_by_name (int StartIdx, char *Segment)
{
    int x;
    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();
    for (x = StartIdx; x < SectionInfoArraySize; x++) {
        if (!strcmp (Segment, SectionInfoArray[x].LongName)) {
            return x;
        }
    }
    return -1;
}

// save_or_load != 0 Save sonst Load
int save_or_load_section_to_or_from_file (int save_or_load, char *SelectedSection,
                                          int SegNo, int PageNo)
{
    int x;
    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();
    for (x = 0; x < SectionInfoArraySize; x++) {
        if (!strcmp (SelectedSection, SectionInfoArray[x].LongName)) {
            int p = CheckExistanceOfPage ((unsigned char)SegNo, (unsigned char)PageNo);

            if (p < 0) {
                ThrowError (1, "segment %i page %i doesn't exists", SegNo, PageNo);
                return 1;
            }

            if (Pages[p].Size != SectionInfoArray[x].Size) {
                ThrowError (1, "segment %i page %i have wrong size", SegNo, PageNo);
                return 1;
            }

            if (save_or_load) {
                MEMCPY (Pages[p].Buffer, (void*)SectionInfoArray[x].StartAddress, Pages[p].Size);
            } else {
                MEMCPY ((void*)SectionInfoArray[x].StartAddress, Pages[p].Buffer, Pages[p].Size);
            }

            return 0;
        }
    }
    ThrowError (1, "error section %s not found\n", SelectedSection);
    return 1;
}


int switch_page_of_section (int A2LSection, int ExeSection, int FromPage, int ToPage)
{
    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();
    if (ExeSection >= SectionInfoArraySize) {
        ThrowError (1, "Section number %i is lager than available sections %i cannot switch page", ExeSection, SectionInfoArraySize);
        return -1;
    }

    int p = CheckExistanceOfPage ((unsigned char)A2LSection, (unsigned char)FromPage);

    if (p < 0) {
        ThrowError (1, "segment %i page %i doesn't exists", A2LSection, FromPage);
        return 1;
    }

    if (Pages[p].Size != get_sections_size (ExeSection)) {
        ThrowError (1, "segment %i page %i have wrong size", A2LSection, FromPage);
        return 1;
    }

    MEMCPY (Pages[p].Buffer, get_sections_start_address (ExeSection), Pages[p].Size);

    if (ToPage >= 0) {
        int p = CheckExistanceOfPage ((unsigned char)A2LSection, (unsigned char)ToPage);

        if (p < 0) {
            ThrowError (1, "segment %i page %i doesn't exists", A2LSection, ToPage);
            return 1;
        }

        if (Pages[p].Size != get_sections_size (ExeSection)) {
            ThrowError (1, "segment %i page %i have wrong size", A2LSection, ToPage);
            return 1;
        }

        MEMCPY (get_sections_start_address (ExeSection), Pages[p].Buffer, Pages[p].Size);

    }
    return 0;
}


int copy_section_page_file (int SegFrom, int PageFrom, int SegTo, int PageTo)
{
    int Src;
    int Dst;

    Src = CheckExistanceOfPage ((unsigned char)SegFrom, (unsigned char)PageFrom);
    if (Src < 0) {
        ThrowError (1, "source section %i page %i dons't exists", SegFrom, PageFrom);
        return 1;
    }
    Dst = CheckExistanceOfPage ((unsigned char)SegTo, (unsigned char)PageTo);
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


#ifdef _WIN32
extern "C" int change_section_access_protection_attributes (char *SelectedSection, DWORD NewProtect)
{
    int x;
    int FoundSectionCounter = 0;

    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();

    for (x = 0; x < SectionInfoArraySize; x++) {
        if (!strcmp (SectionInfoArray[x].LongName, SelectedSection)) {
            DWORD OldFlags;
            if (VirtualProtect (SectionInfoArray[x].StartAddress, SectionInfoArray[x].Size, NewProtect, &OldFlags) == 0) {
                ThrowError (1, "cannot set access protection attributes at memory block start = 0x%X, length = %i  \"%s\"",
                       SectionInfoArray[x].StartAddress, SectionInfoArray[x].Size, GetErrCodeToString (GetLastError ()));
                return 0;
            }
            FoundSectionCounter++;
        }
    }
    if (FoundSectionCounter == 0) {
        ThrowError (1, "error section %s not found\n", SelectedSection);
        return 0;
    } else {
        return 1;
    }
}
#endif

extern "C" int XilEnvInternal_CheckIfAddessIsInsideAWritableSection (void *Address)
{
    int x;

    // Are section information already readed
    if (SectionInfoArraySize == 0) XilEnvInternal_ReadSectionInfosFromAllExecutable ();

    for (x = 0; x < SectionInfoArraySize; x++) {
        if (Address >= SectionInfoArray[x].StartAddress) {
            if ((DWORD)((char*)Address - (char*)(SectionInfoArray[x].StartAddress)) < ((DWORD)SectionInfoArray[x].Size)) {
#ifdef _WIN32
                if ((SectionInfoArray[x].IsSetAsWriteable)) {
                    return 1;    // Address is inside a writable section
                } else {
                    return 0;    // Address is NOT inside a writable section
                }
#else
                return 1;   // Adresse ist in eiener schreibbaren Section
#endif

            }
        }
    }
    return -1;  // Address are not inside any section (for example heap, stack, ...)
}

//#ifdef _WIN32
// This class will check during termination if ther should be something to write back to the executable
class cWriteCallibrationDataBackToExeFile {
private:
    int ShouldExtarctCopyBackExe;
    char TargetExe[MAX_PATH];
    char TempExe[MAX_PATH];
    struct cDlls {
        char Target[MAX_PATH];
        char Temp[MAX_PATH];
    } Dlls[8];
public:
    int SetSectionWriteCallibrationDataBackToExeFile (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, char *SelectedSection)
    {
        ShouldExtarctCopyBackExe = 1;
        if ((TaskInfos->DllName != NULL) && (strlen(TaskInfos->DllName))) {
            if ((TaskInfos->Number >= 0) && (TaskInfos->Number < 8)) {
                return write_section_to_exe_file_after_terminate (TaskInfos->DllName, SelectedSection,
                                                                 Dlls[TaskInfos->Number].Target, sizeof(Dlls[TaskInfos->Number].Target),
                                                                 Dlls[TaskInfos->Number].Temp, sizeof(Dlls[TaskInfos->Number].Temp),
                                                                 (unsigned long long)TaskInfos->DllBaseAddress);
            } else {
                return -1;
            }
        } else {
            EXTERN_PROCESS_INFOS_STRUCT *ProcessInfos = XilEnvInternal_GetGlobalProcessInfos();
            return write_section_to_exe_file_after_terminate (TaskInfos->ExecutableName, SelectedSection,
                                                              TargetExe, sizeof(TargetExe),
                                                              TempExe, sizeof(TempExe),
                                                              (unsigned long long)ProcessInfos->ExecutableBaseAddress);
        }
    }
    cWriteCallibrationDataBackToExeFile (void)
    {
        TargetExe[0] = 0;
        TempExe[0] = 0;
    }
    ~cWriteCallibrationDataBackToExeFile (void)
    {
        EXTERN_PROCESS_INFOS_STRUCT *ProcessInfos = XilEnvInternal_GetGlobalProcessInfos();
        if ((strlen(ProcessInfos->WriteBackExeToDir) == 0) && 
            ShouldExtarctCopyBackExe) {
            char CopyExeBackImagePath[MAX_PATH];
            if (ExtractCopyExeBackImage (CopyExeBackImagePath, sizeof(CopyExeBackImagePath))) {
                return;
            }
            if (strlen (TargetExe) && strlen (TempExe)) {
                copy_exe_file_back_after_terminate (CopyExeBackImagePath, TargetExe, TempExe);
            }
            for (int x = 0; x < 8; x++) {
                if (strlen (Dlls[x].Target) && strlen (Dlls[x].Temp)) {
                    copy_exe_file_back_after_terminate (CopyExeBackImagePath, Dlls[x].Target, Dlls[x].Temp);
                }
            }
        }
    }
} WriteCallibrationDataBackToExeFile;
//#endif

// Returns 1 if section are sucessful write back to the executable otherwise 0
extern "C" int XilEnvInternal_write_section_to_exe_file (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, char *SelectedSection)
{
//#ifdef _WIN32
    return WriteCallibrationDataBackToExeFile.SetSectionWriteCallibrationDataBackToExeFile (TaskInfos, SelectedSection);
//#else
//    return 0;
//#endif
}

extern "C" int XilEnvInternal_TryAndCatchReadFromMemCopy (void *Dst, void *Src, int Size)
{
#ifdef _WIN32
    MEMORY_BASIC_INFORMATION MemBasicInfo;

    // Start address
    if (VirtualQuery(Src, &MemBasicInfo, sizeof (MemBasicInfo)) == 0) {
        return -1;
    }
    if ((MemBasicInfo.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY)) == 0) {
        return -1;
    }
    // End address if not inside the same 4K block
    if ((((unsigned long long)Src + Size) >> 12) != ((unsigned long long)Src >> 12)) {
        if (VirtualQuery(((char*)Src + Size), &MemBasicInfo, sizeof (MemBasicInfo)) == 0) {
            return -1;
        }
        if ((MemBasicInfo.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY)) == 0) {
            return -1;
        }
    }
#else
    // da gibt es unter Linux nichts passendes
#endif
	MEMCPY (Dst, Src, Size);
    return 0;
}
