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


#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32 
#define EXTP_CONFIG_HAVE_DOUBLE
#define EXTP_CONFIG_HAVE_KILL_EVENT
#define EXTP_CONFIG_HAVE_COMMANDLINE
#define EXTP_CONFIG_HAVE_XCP
#define EXTP_CONFIG_HAVE_NAMEDPIPES
#define EXTP_CONFIG_HAVE_SOCKETS
#define EXTP_CONFIG_HAVE_START_XILENV

#if (_MSC_VER == 1310)
#include <my_stdint.h>
#else
#include <stdint.h>
#endif
#include <Windows.h>
#include <io.h>
#include <ShlObj.h>

#define SleepConditionVariableCS_INFINITE(v,s) SleepConditionVariableCS(v,s, INFINITE)

typedef HANDLE MY_FILE_HANDLE;
#define MY_INVALID_HANDLE_VALUE  INVALID_HANDLE_VALUE

#define NULL_INT_OR_PTR  NULL

#elif defined(__linux__)
#define EXTP_CONFIG_HAVE_DOUBLE
#define EXTP_CONFIG_HAVE_KILL_EVENT
#define EXTP_CONFIG_HAVE_COMMANDLINE
#define EXTP_CONFIG_HAVE_XCP
#define EXTP_CONFIG_HAVE_SOCKETS
#define EXTP_CONFIG_HAVE_UNIX_DOMAIN_SOCKETS
#define EXTP_CONFIG_HAVE_START_XILENV

#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <pthread.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <alloca.h>
#include <errno.h>

#define MAX_PATH   260

typedef int BOOL;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;

typedef int HANDLE;
#define NULL_INT_OR_PTR  0
//typedef void *HANDLE;

typedef DWORD *LPDWORD;

typedef char *LPSTR;
typedef char *LPTSTR;
typedef const char *LPCSTR;
typedef const char *LPCTSTR;

typedef void *LPVOID;
typedef const void *LPCVOID;

typedef pthread_mutex_t CRITICAL_SECTION;
typedef pthread_cond_t CONDITION_VARIABLE;
typedef time_t FILETIME;

typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    unsigned char Data4[8];
} GUID;


#define __cdecl
#define __stdcall

#define IDOK                1
#define IDCANCEL            2
#define IDIGNORE            5

#define TRUE 1
#define FALSE 0

// access() mode parameter
#define SW_SHOW   R_OK

#define strcmpi(a,b) strcasecmp(a,b)
#define _strcmpi(a,b) strcasecmp(a,b)
#define _stricmp(a,b) strcasecmp(a,b)
#define stricmp(a,b) strcasecmp(a,b)
#define strnicmp(a,b,c) strncasecmp(a,b,c)
#define _strnicmp(a,b,c) strncasecmp(a,b,c)

#define _alloca(s) alloca(s)

#define EnterCriticalSection(s) pthread_mutex_lock(s)
#define TryEnterCriticalSection(s)  pthread_mutex_trylock(s)
#define LeaveCriticalSection(s) pthread_mutex_unlock(s)
#define InitializeCriticalSection(s) pthread_mutex_init(s, NULL)
#define DeleteCriticalSection(s) pthread_mutex_destroy(s)
#define InitializeConditionVariable(v) pthread_cond_init(v, NULL)
#define SleepConditionVariableCS_INFINITE(v,s) pthread_cond_wait(v,s)
#define SleepConditionVariableCS(v,s,t) sc_pthread_cond_timedwait(v,s,t)
#define WakeAllConditionVariable(v) pthread_cond_signal(v)

//#define INVALID_HANDLE_VALUE   ((void*)(unsigned long long)-1)
#define INVALID_HANDLE_VALUE   (-1)
#define INFINITE               0xFFFFFFFF

#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_LOWEST         -2
#define THREAD_PRIORITY_BELOW_NORMAL   -1
#define THREAD_PRIORITY_IDLE          -15
#define THREAD_PRIORITY_TIME_CRITICAL  15
#define THREAD_PRIORITY_ABOVE_NORMAL    1
#define THREAD_PRIORITY_HIGHEST         2

#define GetCurrentThreadId() ((DWORD)syscall(__NR_gettid))

#define _isnan(v) isnan(v)
#define _access(f,x) access(f,x)

#define SetCurrentDirectory(d) ((chdir(d) == 0) ? 1 : 0)
#define GetCurrentDirectory(s,b) getcwd(b,s)

#define CALLBACK

#define RGB(r,g,b) ((int)((r) & 0xFF) | ((int)((g) & 0xFF) << 8)  | ((int)((b) & 0xFF) << 16))
#define GetRValue(rgb) ((rgb)&0xFF)
#define GetGValue(rgb) (((rgb)>>8)&0xFF)
#define GetBValue(rgb) (((rgb)>>16)&0xFF)

#ifdef __cplusplus
extern "C" {
#endif

uint64_t GetTickCount64(void);
uint32_t GetTickCount(void);

int GetEnvironmentVariable(const char *EnvVarName,
                           char *EnvVarValue,
                           int SizeofEnvVarValue);
int SetEnvironmentVariable(const char *EnvVarName,
                           const char *EnvVarValue);


int sc_pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, int time_ms);


HANDLE CreateFile(const char *Filename, DWORD dwDesiredAccess, DWORD dwSharedMode, void *lpSecurityAttributes, DWORD dwCreatoionDisposition,
                  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
#define GENERIC_READ  O_RDONLY
#define GENERIC_WRITE  O_WRONLY
#define FILE_READ_DATA   0x1
#define FILE_WRITE_DATA  0x2
#define FILE_SHARE_READ  0x1
#define FILE_SHARE_WRITE 0x2
#define CREATE_ALWAYS     2
#define OPEN_EXISTING     3
#define FILE_ATTRIBUTE_NORMAL  0x80
#define HFILE_ERROR (-1)
int ReadFile(HANDLE hFile, void *lpBuffer, DWORD nNumberOfBytesToRead, DWORD *lpNumberOfBytesRead, void *lpOverlapped);
int WriteFile(HANDLE hFile, void *lpBuffer, DWORD nNumberOfBytesToWrite, DWORD *lpNumberOfBytesWritten, void *lpOverlapped);
DWORD GetFileSize(HANDLE hFile,LPDWORD lpFileSizeHigh);
void CloseHandle(HANDLE hFile);
int CopyFile(const char *lpExistingFileName, const char *lpNewFileName, int bFailIfExists);
int GetModuleFileName(void *hModule, char *lpFilename, int nSize);
int VirtualProtect(LPVOID lpAddress, int dwSize, DWORD flNewProtect, DWORD *lpflOldProtect);
#define PAGE_READWRITE  0x04

void GetXilEnvHomeDirectory(char *ret_Directory);

#define DIR_MUST_EXIST     0
#define DIR_CREATE_EXIST   1
#define FILENAME_MUST_EXIST     0
#define FILENAME_CREATE_EXIST   1
#define FILENAME_IGNORE         2
int CheckOpenIPCFile(char *Instance, char *Name, char *ret_Path, int DirCraeteOrMustExists, int FileCraeteOrMustExists);

char *GetCommandLine(void);

#define _fseeki64(fh, offset, whence) fseek(fh, offset, whence)
#define _ftelli64(fh) ftell(fh)

typedef int MY_FILE_HANDLE;
#define MY_INVALID_HANDLE_VALUE  -1

#ifdef __cplusplus
}
#endif

#elif defined(__QEMU__)
#define EXTP_CONFIG_HAVE_QEMU_SIL_CON
#define EXTP_CONFIG_USE_OWN_HEAP
#define EXTP_CONFIG_OWN_HEAP_BASE_ADRESS   0xf8000000U
#define EXTP_CONFIG_OWN_HEAP_SIZE          (16*1024*1024)

#define InitializeCriticalSection(a)
#define EnterCriticalSection(a)
#define LeaveCriticalSection(a)

#define MAX_PATH 260
#define TRUE 1
#define FALSE 0

typedef int             HANDLE;

typedef unsigned int    DWORD;
typedef int             BOOL;
typedef void            *PVOID;
typedef void            *LPVOID;
typedef const void      *LPCVOID;
typedef DWORD           *LPDWORD;

#define __cdecl
#define __stdcall

#define pthread_mutex_t int

#define VirtualProtect(a,b,c,d)
#define PAGE_READWRITE 4


#define IDOK                1
#define IDCANCEL            2
#define IDIGNORE            5

#define INVALID_HANDLE_VALUE   -1

extern int errno;

#define CRITICAL_SECTION int

#else
#error "no target defined! posible are _WIN32, __linux__, __QEMU__"
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __linux__
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

#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory

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

typedef struct {
    uint32_t   Characteristics;
    uint32_t   TimeDateStamp;
    uint16_t    MajorVersion;
    uint16_t    MinorVersion;
    uint32_t   Type;
    uint32_t   SizeOfData;
    uint32_t   AddressOfRawData;
    uint32_t   PointerToRawData;
} IMAGE_DEBUG_DIRECTORY;

#define IMAGE_DEBUG_TYPE_CODEVIEW               2
#endif

int GetRealPathForOnlyUpperCasePath (const char *SrcPath, char *DstPath, int MaxPath);
//#ifdef __linux__
MY_FILE_HANDLE my_open (const char *Name);
void my_close(MY_FILE_HANDLE File);
int my_read(MY_FILE_HANDLE File, void* DstBuf, unsigned int Count);
void my_lseek(MY_FILE_HANDLE File, uint64_t Offset);
uint64_t my_ftell(MY_FILE_HANDLE File);
uint64_t my_get_file_size(MY_FILE_HANDLE File);
//#endif
#ifdef __cplusplus
}
#endif

#endif
