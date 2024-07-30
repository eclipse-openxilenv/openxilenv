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
#include <malloc.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "IniSectionEntryDefines.h"

#include "Config.h"
#include "MyMemory.h"
#include "Files.h"
#include "ThrowError.h"
#include "MainValues.h"
#include "IniFileDontExist.h"
#include "IniDataBase.h"
#include "RunTimeMeasurement.h"
#include "Wildcards.h"
#include "StringMaxChar.h"
#include "MainWinowSyncWithOtherThreads.h"
#include "IniDatabaseInOutputFilter.h"

#define FILE_DESCRIPTOR_OFFSET  0x100

#define INI_ENTER_CS(cs)\
ENTER_CS(cs);\
IniDBCriticalSectionUserFile = __FILE__;\
IniDBCriticalSectionUserLine = __LINE__;

#define INI_LEAVE_CS(cs)\
IniDBCriticalSectionUserFile = NULL;\
IniDBCriticalSectionUserLine = 0;\
LEAVE_CS(cs);

static CRITICAL_SECTION IniDBCriticalSection;
static CRITICAL_SECTION IniDBCriticalSectionUser;
static const char *IniDBCriticalSectionUserFile;
static int IniDBCriticalSectionUserLine;

// Do not use the standard isscpace
#define IsSpace(x) (((x >= 9) && (x <= 13)) || (x == 32))

typedef struct {
    char *Name;
    int EntryCount;
    int EntrySize;
    char **Entrys;
} INI_DB_SECTION_ELEM;

typedef struct {
    char *Filename;
    int FileDescriptor;
    INI_DB_SECTION_ELEM **Sections;
    int SectionCount;
    int SectionSize;
    int CachedIndex;
    int CacheHitCounter;
    int CacheMissCounter;
} INI_DB_FILE;

#define MAX_INI_FILES 16
static INI_DB_FILE AllLoadedIniFiles[MAX_INI_FILES];
static int AllLoadedIniFilesCounter;

static int MainFileDescriptor;

char ini_path[MAX_PATH];


int IniFileDataBaseInit(void)
{
    int x;
    INIT_CS (&IniDBCriticalSection);
    INIT_CS (&IniDBCriticalSectionUser);
    MainFileDescriptor = -1;
    for (x = 0; x < MAX_INI_FILES; x++) {
        AllLoadedIniFiles[x].FileDescriptor = -1;
        AllLoadedIniFiles[x].CachedIndex = -1;
    }
    return 0;
}

static void DeleteFromIniFileDataBase(int par_FileIndex);

void IniFileDataBaseTerminate(void)
{
    int x;
    INI_ENTER_CS(&IniDBCriticalSection);
    for (x = 0; x < MAX_INI_FILES; x++) {
        if (AllLoadedIniFiles[x].FileDescriptor > 0) {
            DeleteFromIniFileDataBase(x);
            AllLoadedIniFiles[x].FileDescriptor = -1;
            AllLoadedIniFiles[x].CachedIndex = -1;
        }
    }
    INI_LEAVE_CS(&IniDBCriticalSection);
}


static char *GetLine (char **line, int *line_size, FILE *fh)
{
    int c;
    int x = 0;

    if ((*line == NULL) || (*line_size <= 0)) {
        *line_size = 1024;   // Buffer must be at least 1024 bytes large
        *line = my_realloc (*line, (size_t)*line_size);
        if (*line == NULL) {
            *line_size = 0;
            return NULL;
        }
    }

    while (1) {
        c = fgetc(fh);
        if (c == EOF) {
            (*line)[x] = 0;
            break;
        }
        if (x >= *line_size) {
            *line_size = *line_size + (*line_size >> 2) + 1024;   // Increase buffer 1/4 + 1024
            *line = my_realloc (*line, (size_t)*line_size);
            if (*line == NULL) {
                *line_size = 0;
                return NULL;
            }
        }
        if (c == '\n') {
            // Remove the tailing whitespaces
            c = (*line)[x-1];
            while ((x > 1) && isascii ((*line)[x-1]) && IsSpace((*line)[x-1])) {
                x--;
            }
            (*line)[x] = 0;
            break;
        }
        (*line)[x] = (char)c;
        x++;
    }
    if ((c == EOF) && (x == 0)) return NULL;
    return *line;
}

static int IniFileDataBaseWriteStringNoLock (const char* par_Section, const char* par_Entry, const char* par_Text, int par_FileDescriptor);

static int ParserIniFile(FILE* par_FileHanlde, int par_FileDescriptor)
{
    char *Line = NULL;
    int LineSize = 0;
    char Section[INI_MAX_SECTION_LENGTH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char *p, *n;
    int c;
    int32_t IniFileSize;
    int32_t IniFileProcess = 0;
    int32_t Percent, Percent_old=0L;
    int ProgressBarId;

    ProgressBarId = OpenProgressBarFromOtherThread("(Read INI)");
    //fseek(par_FileHanlde, 0L, SEEK_END);
    IniFileSize = 10000; //ftell(par_FileHanlde);
    //fseek(par_FileHanlde, 0L, SEEK_SET);

    while (GetLine (&Line, &LineSize, par_FileHanlde) != NULL) {
        IniFileProcess += (int32_t)strlen (Line);
        Percent = (int32_t)(100.0 * (double)IniFileProcess / (double)IniFileSize);
        if (Percent > Percent_old) {
            Percent_old = Percent;
            if (ProgressBarId >= 0) SetProgressBarFromOtherThread(ProgressBarId, Percent);
        }
        p = Line;
        while (IsSpace (*p)) p++;
        if (*p == ';') continue;           // Comment line
        if (*p == '[') {   // section
            p++;
            c = 0;
            while (*p != ']') {
                if (*p == '\0') break; //  [-bracket without a ]-bracket
                if (c >= INI_MAX_SECTION_LENGTH-1) break;  // section max. 1023 characters long
                Section[c++] = *p++;
            }
            Section[c] = '\0';
        } else {           // entry
            c = 0;
            while (*p != '=') {
                if (*p == '\0') break; // no entry new line
                if (c >= INI_MAX_ENTRYNAME_LENGTH-1) break;  // entry max. 1023  characters long
                Entry[c++] = *p++;
            }
            // Remove whitespaces between entry and "="-character
            while ((c != 0) && IsSpace(Entry[c-1])) c--;
            Entry[c] = '\0';
            if (!c) continue;   // New line because entry has 0 characters
            while ((c != 0) && IsSpace(Entry[c])) c--;
            Entry[c+1] = '\0';

            if (*p++ != '=') continue;
            while (IsSpace (*p)) p++; // Remove whitespaces after the "="-character
            n = p;
            while ((*n != '\0') && (*n != '\n')) n++;
            *n = '\0';
            if ((*p != '\0') && (*p != '\n')) {
                if (!IniFileDataBaseWriteStringNoLock(Section, Entry, p, par_FileDescriptor)) {
                    close_file (par_FileHanlde);
                    if (ProgressBarId >= 0) CloseProgressBarFromOtherThread(ProgressBarId);
                    return -1;
                }
            }
        }
    }
    if (Line != NULL) my_free(Line);
    if (ProgressBarId >= 0) {
        SetProgressBarFromOtherThread(ProgressBarId, 100);
        CloseProgressBarFromOtherThread(ProgressBarId);
    }
    return 0;
}

static void FreeFullPath(char *par_FullPath)
{
    if (par_FullPath != NULL) {
#ifdef _WIN32
        my_free(par_FullPath);
#else
        free(par_FullPath);
#endif
    }
}

#ifdef _WIN32
#define FILE_CHAR_UPPER(x) ((((x) >= 'a') && ((x) <= 'z')) ? (uint32_t)(x) - (uint32_t)('a'-'A') : (uint32_t)(x))
#else
#define FILE_CHAR_UPPER(x) (x)
#endif

static char *GetFullPath(const char *par_Filename)
{
    char *p;
    char *FullName = NULL;
#ifdef _WIN32
    int NeedSize, Size = MAX_PATH;
    FullName = (char*)my_malloc(Size);
    if (FillPath == NULL) {
        goto __ERROUT;
    }
    NeedSize = GetFullPathName (par_Filename, Size, FullName, NULL);
    if (NeedSize == 0) {
        goto __ERROUT;
    }
    if (NeedSize > Size) {
        Size = NeedSize;
        FullName = (char*)my_realloc(FullName, Size);
        if (FillPath == NULL) {
            goto __ERROUT;
        }
        NeedSize = GetFullPathName (par_Filename, Size, FullName, NULL);
        if ((NeedSize == 0)  || (NeedSize > Size)) {
            goto __ERROUT;
        }
    }
#else
    FullName = realpath (par_Filename, NULL);
    if (FullName == NULL) {
        goto __ERROUT;
    }
#endif
    p = FullName;
    while (*p != 0) {
        if (*p == '\\') *p = '/';
        else *p = FILE_CHAR_UPPER(*p);
        p++;
    }
    return FullName;
__ERROUT:
    if (FullName != NULL)  FreeFullPath(FullName);
    return NULL;
}

static int IniFileDataBaseOpenInternal(const char *par_Filename, int par_FilterPossible)
{
    int Ret;
    int x;
    int FirstEmptyEntry = -1;
    char* FullName = NULL;
    FILE* FileHandle = NULL;
    int FileDescriptor = -1;
    char* p;
    int Version;
    int MinorVersion;
    int PatchVersion;
    char VersionString[64];

    INI_ENTER_CS(&IniDBCriticalSection);

    if (!strcmp("stdin", par_Filename) || !strcmp("stdout", par_Filename)) {
        FullName = (char*)my_malloc(strlen(par_Filename) + 1);
        if (FullName == NULL) {
            Ret = -1;
            goto __ERROUT;
        }
        strcpy(FullName, par_Filename);
    } else {
#ifdef _WIN32
        int NeedSize, Size = MAX_PATH;
        FullName = (char*)my_malloc(Size);
        if (FillPath == NULL) {
            Ret = -1;
            goto __ERROUT;
        }
        NeedSize = GetFullPathName(par_Filename, Size, FullName, NULL);
        if (NeedSize == 0) {
            Ret = -1;
            goto __ERROUT;
        }
        if (NeedSize > Size) {
            Size = NeedSize;
            FullName = (char*)my_realloc(FullName, Size);
            if (FillPath == NULL) {
                Ret = -1;
                goto __ERROUT;
            }
            NeedSize = GetFullPathName(par_Filename, Size, FullName, NULL);
            if ((NeedSize == 0) || (NeedSize > Size)) {
                Ret = -1;
                goto __ERROUT;
            }
        }
#else
        p = realpath(par_Filename, NULL);
        if (p == NULL) {
            Ret = -1;
            goto __ERROUT;
        }
        // make a copy with my_malloc
        FullName = (char*)my_malloc(strlen(p) + 1);
        if (FullName == NULL) {
            Ret = -1;
            goto __ERROUT;
        }
        strcpy(FullName, p);
        free(p);
#endif
        p = FullName;
        while (*p != 0) {
            if (*p == '\\') *p = '/';
            else *p = FILE_CHAR_UPPER(*p);
            p++;
        }
    }
    // Check if file already loaded
    for (x = 0; x < MAX_INI_FILES; x++) {
        if (AllLoadedIniFiles[x].Filename != NULL) {
            if (!strcmp (FullName, AllLoadedIniFiles[x].Filename)) {
                Ret = -2;   // file is loaded
                goto __ERROUT;
            }
        } else {
            if (FirstEmptyEntry == -1) {
                FirstEmptyEntry = x;
            }
        }
    }
    if (FirstEmptyEntry == -1) {
        Ret = -3;  // no free entry
        goto __ERROUT;
    }

    // Now open the file
    if (par_FilterPossible && ((s_main_ini_val.IniFilterProgramFlags & INI_FILTER_PROGRAM_INPUT) == INI_FILTER_PROGRAM_INPUT)) {
        if ((FileHandle = CreateInOrOutputFilterProcessPipe (s_main_ini_val.IniFilterProgram,
                                                             FullName, 0)) == NULL) {
            Ret = -1;
            goto __ERROUT;
        }
    } else {
        if ((FileHandle = open_file (FullName, "rt")) == NULL) {
            Ret = -1;
            goto __ERROUT;
        }
    }

    // Build a hanlde
    FileDescriptor = FirstEmptyEntry + FILE_DESCRIPTOR_OFFSET;

    AllLoadedIniFiles[FirstEmptyEntry].SectionCount = 0;
    AllLoadedIniFiles[FirstEmptyEntry].SectionSize = 0;
    AllLoadedIniFiles[FirstEmptyEntry].CachedIndex = -1;
    AllLoadedIniFiles[FirstEmptyEntry].CacheHitCounter = 0;
    AllLoadedIniFiles[FirstEmptyEntry].CacheMissCounter = 0;
    AllLoadedIniFiles[FirstEmptyEntry].Filename = FullName;
    AllLoadedIniFiles[FirstEmptyEntry].FileDescriptor = FileDescriptor;
    AllLoadedIniFilesCounter++;

    if (strcmp("stdout", par_Filename)) {
        if (ParserIniFile(FileHandle, FileDescriptor)) {
            Ret = -1;
            goto __ERROUT;
        }
    }
    if (FileHandle != NULL) {
        if (IsInOrOutputFilterProcessPipe(FileHandle)) {
            if (strcmp("stdin", par_Filename) && strcmp("stdout", par_Filename)) {
                CloseInOrOutputFilterProcessPipe(FileHandle);
            }
        } else {
            close_file(FileHandle);
        }
    }
    INI_LEAVE_CS (&IniDBCriticalSection);

    // now check if it is the right version
    if (par_FilterPossible && ((s_main_ini_val.IniFilterProgramFlags & INI_FILTER_PROGRAM_NO_FILE_VERSION_CKECK) != INI_FILTER_PROGRAM_NO_FILE_VERSION_CKECK)) {
        Version = 0;
        MinorVersion = 0;
        PatchVersion = 0;
        IniFileDataBaseReadString(OPT_FILE_INFOS, OPT_FILE_VERSION, "0.0.0", VersionString, sizeof(VersionString), FileDescriptor);
        Version = strtoul(VersionString, &p, 10);
        if ((p != NULL) && (*p == '.')) {
            MinorVersion = strtoul(p + 1, &p, 10);
            if ((p != NULL) && (*p == '.')) {
                PatchVersion = strtoul(p + 1, &p, 10);
            }
        }
        if ((Version < XILENV_INIFILE_MIN_VERSION) ||
            ((Version == XILENV_INIFILE_MIN_VERSION) && (MinorVersion < XILENV_INIFILE_MIN_MINOR_VERSION)) ||
            ((Version == XILENV_INIFILE_MIN_VERSION) && (MinorVersion == XILENV_INIFILE_MIN_MINOR_VERSION) && (PatchVersion < XILENV_INIFILE_MIN_PATCH_VERSION))) {
            int Ret = ThrowError(ERROR_OKCANCEL,
                "the file \"%s\" has not the correct version information >= %d.%d.%d (%d.%d.%d)\n"
                "Press (OK) to ignore or (Cancel) to stop loading",
                par_Filename,
                XILENV_INIFILE_MIN_VERSION, XILENV_INIFILE_MIN_MINOR_VERSION, XILENV_INIFILE_MIN_PATCH_VERSION,
                Version, MinorVersion, PatchVersion);
            if (Ret == IDCANCEL) {
                IniFileDataBaseClose(FileDescriptor);
                return -1;
            }
            else {
                sprintf(VersionString, "%d.%d.%d", XILENV_INIFILE_MIN_VERSION, XILENV_INIFILE_MIN_MINOR_VERSION, XILENV_INIFILE_MIN_PATCH_VERSION);
                IniFileDataBaseWriteString(OPT_FILE_INFOS, OPT_FILE_VERSION, VersionString, FileDescriptor);
            }
        }
    }
    return FileDescriptor;
__ERROUT:
    if (FullName != NULL) {
        if (FileDescriptor > 0) IniFileDataBaseClose(FileDescriptor);
        if (FileHandle != NULL) {
            if (IsInOrOutputFilterProcessPipe(FileHandle)) {
                CloseInOrOutputFilterProcessPipe(FileHandle);
            } else {
                close_file(FileHandle);
            }
        }
        if (FullName != NULL) my_free(FullName);
    }
    INI_LEAVE_CS (&IniDBCriticalSection);
    return Ret;
}

int IniFileDataBaseOpen(const char *par_Filename)
{
    return IniFileDataBaseOpenInternal(par_Filename, 1);
}

int IniFileDataBaseOpenNoFilterPossible(const char *par_Filename)
{
    return IniFileDataBaseOpenInternal(par_Filename, 0);
}

static int Compare2FileNames(const char *par_Name, const char *par_RefName)
{
    const char *n = par_Name;
    const char *r = par_RefName;
    while (*r != 0) {
        if (*r != FILE_CHAR_UPPER(*n)) {
            if ((*r == '/') && (*n == '\\')) {  // '\' is same as '/'
                continue;
            }
            return 1; // not equal
        }
        n++;
        r++;
    }
    return 0;  // equal
}

int GetFileDescriptorByName(const char *par_Filename)
{
    int x;
    for (x = 0; x < MAX_INI_FILES; x++) {
        if (AllLoadedIniFiles[x].Filename != NULL) {
            if (!Compare2FileNames (par_Filename, AllLoadedIniFiles[x].Filename)) {
                return  AllLoadedIniFiles[x].FileDescriptor;
            }
        }
    }
    return -1;
}

void SetMainFileDescriptor(int par_FileDescriptor)
{
    MainFileDescriptor = par_FileDescriptor;
}

void SetMainIniFilename(const char *par_FileName)
{
    StringCopyMaxCharTruncate(ini_path, par_FileName, sizeof(ini_path));
}

int GetMainFileDescriptor(void)
{
    return MainFileDescriptor;
}


static char *GetFileNameByDescriptor(int par_FileDescriptor)
{
    int x;
    for (x = 0; x < MAX_INI_FILES; x++) {
        if (AllLoadedIniFiles[x].FileDescriptor == par_FileDescriptor) {
            return  AllLoadedIniFiles[x].Filename;
        }
    }
    return NULL;
}

int IniFileDataBaseGetFileNameByDescriptor(int par_FileDescriptor, char *ret_Name, int par_MaxSize)
{
    char *Filename;
    int Ret = -1;
    INI_ENTER_CS(&IniDBCriticalSection);
    Filename = GetFileNameByDescriptor(par_FileDescriptor);
    if (Filename != NULL) {
        StringCopyMaxCharTruncate(ret_Name, Filename, par_MaxSize);
        Ret = 0;
    }
    INI_LEAVE_CS(&IniDBCriticalSection);
    return Ret;
}

static int GetIndexByDescriptor(int par_FileDescriptor)
{
    if ((par_FileDescriptor >= FILE_DESCRIPTOR_OFFSET) &&
        (par_FileDescriptor < (FILE_DESCRIPTOR_OFFSET + MAX_INI_FILES))) {
        int Index = par_FileDescriptor - FILE_DESCRIPTOR_OFFSET;
        if (AllLoadedIniFiles[Index].FileDescriptor == par_FileDescriptor) {
            return Index;
        }
    }
    return -1;
}

static void DeleteFromIniFileDataBase(int par_FileIndex)
{
    int s;
    for (s = 0; s < AllLoadedIniFiles[par_FileIndex].SectionCount; s++) {
        int e;
        for (e = 0; e < AllLoadedIniFiles[par_FileIndex].Sections[s]->EntryCount; e++) {
            my_free(AllLoadedIniFiles[par_FileIndex].Sections[s]->Entrys[e]);
        }
        if (AllLoadedIniFiles[par_FileIndex].Sections[s]->Entrys != NULL) {
            my_free(AllLoadedIniFiles[par_FileIndex].Sections[s]->Entrys);
        }
        my_free(AllLoadedIniFiles[par_FileIndex].Sections[s]->Name);
        my_free(AllLoadedIniFiles[par_FileIndex].Sections[s]);
    }
    if (AllLoadedIniFiles[par_FileIndex].Sections != NULL) {
        my_free(AllLoadedIniFiles[par_FileIndex].Sections);
    }
    if (AllLoadedIniFiles[par_FileIndex].Filename != NULL) {
        my_free(AllLoadedIniFiles[par_FileIndex].Filename);
    }
    AllLoadedIniFiles[par_FileIndex].Filename = NULL;
    AllLoadedIniFiles[par_FileIndex].Sections = NULL;
    AllLoadedIniFiles[par_FileIndex].SectionCount = 0;
    AllLoadedIniFiles[par_FileIndex].SectionSize = 0;
    AllLoadedIniFiles[par_FileIndex].CachedIndex = -1;
    AllLoadedIniFiles[par_FileIndex].CacheHitCounter = 0;
    AllLoadedIniFiles[par_FileIndex].CacheMissCounter = 0;
    AllLoadedIniFiles[par_FileIndex].FileDescriptor = -1;
}

int IniFileDataBaseClose(int par_FileDescriptor)
{
    int x;
    int Ret = -1;
    if (par_FileDescriptor >= FILE_DESCRIPTOR_OFFSET) {
        INI_ENTER_CS (&IniDBCriticalSection);
        // Check if file is loaded
        for (x = 0; x < MAX_INI_FILES; x++) {
            if (AllLoadedIniFiles[x].FileDescriptor == par_FileDescriptor) {
                DeleteFromIniFileDataBase(x);
                Ret = 0;
                break;
            }
        }
        INI_LEAVE_CS (&IniDBCriticalSection);
    }
    return Ret;
}

static int SortSectionCompareFunction (const void *a, const void *b)
{
    INI_DB_SECTION_ELEM *SectionA;
    INI_DB_SECTION_ELEM *SectionB;

    SectionA = (INI_DB_SECTION_ELEM*)a;
    SectionB = (INI_DB_SECTION_ELEM*)b;
    return strcmp(SectionA->Name, SectionB->Name);
}

static int SortEntryCompareFunction (const void *a, const void *b)
{
    char **EntryA;
    char **EntryB;

    EntryA = (char**)a;
    EntryB = (char**)b;
    return strcmp(*EntryA, *EntryB);
 }


/* par_Operation == 0 -> Only save it. Do nothing more
   par_Operation == 1 -> rename file to DestFileName
   par_Operation == 2 -> remove infos from INI-DB 
   par_Operation == 3 -> write it to stdout (with [FileInfos] Section)
   par_Operation == 4 -> write it to stdout (without [FileInfos] Section)
   par_Operation == 5 -> Only save it, same as 0 but without [FileInfos] Section.
   */
static int IniFileDataBaseSaveInternal(int par_FileDescriptor, const char *par_DstFileName, int par_Operation, int par_FilterPossible)
{
    FILE *fh = NULL;
    char txt[MAX_PATH+100];
    int ProgressBarID;
    int32_t Percent, PercentOld = 0;
    int SortedStore;
    char *DstFileNameFullPath = NULL;
    int FileIndex;
    int s;

    switch(par_Operation) {
    case INIFILE_DATABAE_OPERATION_WRITE_ONLY:
    case INIFILE_DATABAE_OPERATION_REMOVE:
    case INIFILE_DATABAE_OPERATION_WRITE_ONLY_WITHOUT_VERSION_INFO:
        DstFileNameFullPath = GetFileNameByDescriptor(par_FileDescriptor);
        if (DstFileNameFullPath == NULL) {
            goto __ERROUT;
        }
        break;
    case INIFILE_DATABAE_OPERATION_RENAME:
        DstFileNameFullPath = GetFullPath(par_DstFileName);
        if (DstFileNameFullPath == NULL) {
            goto __ERROUT;
        }
        if (GetFileDescriptorByName(DstFileNameFullPath) > 0) {
            par_Operation = INIFILE_DATABAE_OPERATION_WRITE_ONLY;   // same name "Save as" to current namen
        }
        break;
    case INIFILE_DATABAE_OPERATION_WRITE_TO_STDOUT:
    case INIFILE_DATABAE_OPERATION_WRITE_TO_STDOUT_WITHOUT_VERSION_INFO:
        DstFileNameFullPath = StringMalloc("stdout");
        break;
    default:
        goto __ERROUT;
    }

    // Inside every INI file store the version
    if ((par_Operation != INIFILE_DATABAE_OPERATION_WRITE_TO_STDOUT_WITHOUT_VERSION_INFO) && 
        (par_Operation != INIFILE_DATABAE_OPERATION_WRITE_ONLY_WITHOUT_VERSION_INFO)) {
        sprintf(txt, "%d.%d.%d", XILENV_VERSION, XILENV_MINOR_VERSION, XILENV_PATCH_VERSION);
        IniFileDataBaseWriteString(OPT_FILE_INFOS, OPT_FILE_VERSION, txt, par_FileDescriptor);
    }
    SortedStore = s_main_ini_val.SaveSortedINIFiles;   // Should the INI file stored sorted?

    INI_ENTER_CS (&IniDBCriticalSection);

    if (s_main_ini_val.MakeBackupBeforeSaveINIFiles &&
        (par_Operation != INIFILE_DATABAE_OPERATION_WRITE_TO_STDOUT) &&
        (par_Operation != INIFILE_DATABAE_OPERATION_WRITE_TO_STDOUT_WITHOUT_VERSION_INFO)) {              // Should be stored a backup?
        char *SrcFilename = GetFileNameByDescriptor(par_FileDescriptor);
        char *Filename;
        if (par_Operation == INIFILE_DATABAE_OPERATION_RENAME) {
            Filename = DstFileNameFullPath;
        } else {
            Filename = SrcFilename;
        }
        if (_access (Filename, 0) == 0) {  // Existence only
            char *BackupFile = my_malloc(strlen(Filename) + 5);     // than make a backup file
            strcpy (BackupFile, Filename);
            strcat (BackupFile, ".bak");
            if (_access(BackupFile, 0) == 0) { // Exists a backup file
                remove (BackupFile);           // remove it before
            }
            rename (Filename, BackupFile);
            my_free(BackupFile);
        }
    }

    // Now open the file
    if (par_FilterPossible && (((s_main_ini_val.IniFilterProgramFlags & INI_FILTER_PROGRAM_OUTPUT) == INI_FILTER_PROGRAM_OUTPUT) ||
                               (par_Operation == INIFILE_DATABAE_OPERATION_WRITE_TO_STDOUT) || 
                               (par_Operation == INIFILE_DATABAE_OPERATION_WRITE_TO_STDOUT_WITHOUT_VERSION_INFO))) {
        if ((fh = CreateInOrOutputFilterProcessPipe (s_main_ini_val.IniFilterProgram,
                                                     DstFileNameFullPath, 1)) == NULL) {
            goto __ERROUT;
        }
    } else {
        if ((fh = open_file (DstFileNameFullPath, "wt")) == NULL) {
            goto __ERROUT;
        }
    }

    FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex < 0) {
        goto __ERROUT;
    }

    if (fprintf (fh, "; This is an OpenXilEnv configuration file.\n"
                     "; The syntax is similar to an INI file except that the section and entry name are case sensitive\n") == EOF) {
        goto __ERROUT;
    }

    sprintf (txt, "Write INI-File %s", GetFileNameByDescriptor(par_FileDescriptor));
    ProgressBarID = OpenProgressBarFromOtherThread("(Write INI)");

    if (SortedStore) {
        qsort ((void*)AllLoadedIniFiles[FileIndex].Sections, (size_t)AllLoadedIniFiles[FileIndex].SectionCount,
              sizeof(AllLoadedIniFiles[FileIndex].Sections[0]), SortSectionCompareFunction);
        for (s = 0; s < AllLoadedIniFiles[FileIndex].SectionCount; s++) {
            INI_DB_SECTION_ELEM *Section = AllLoadedIniFiles[FileIndex].Sections[s];
            qsort ((void*)Section->Entrys, (size_t)Section->EntryCount,
                  sizeof(Section->Entrys[0]), SortEntryCompareFunction);
        }
    }
    for (s = 0; s < AllLoadedIniFiles[FileIndex].SectionCount; s++) {
        INI_DB_SECTION_ELEM *Section = AllLoadedIniFiles[FileIndex].Sections[s];
        if (!s_main_ini_val.DontSaveBlackbardVariableInfosIniSection ||
            (s_main_ini_val.DontSaveBlackbardVariableInfosIniSection && strcmp (Section->Name, "Variables"))) {
            int e;
            if (fprintf (fh, "\n[%s]\n", Section->Name) == EOF) {
                goto __ERROUT;
            }
            for (e = 0; e < Section->EntryCount; e++) {
                if (fprintf (fh, "  %s\n", Section->Entrys[e]) == EOF) {
                    goto __ERROUT;
                }
            }
        }
        Percent = (100 * s) / (AllLoadedIniFiles[FileIndex].SectionCount);
        if (Percent > PercentOld) {
            PercentOld = Percent;
            if (ProgressBarID >= 0) SetProgressBarFromOtherThread(ProgressBarID, PercentOld);
        }
    }
    switch(par_Operation) {
    case INIFILE_DATABAE_OPERATION_REMOVE:
        DeleteFromIniFileDataBase(FileIndex);
        break;
    case INIFILE_DATABAE_OPERATION_RENAME:
        IniFileDataBaseRenameFile(par_DstFileName, par_FileDescriptor);
        break;
    case INIFILE_DATABAE_OPERATION_WRITE_ONLY:
    case INIFILE_DATABAE_OPERATION_WRITE_TO_STDOUT:
    case INIFILE_DATABAE_OPERATION_WRITE_TO_STDOUT_WITHOUT_VERSION_INFO:
    case INIFILE_DATABAE_OPERATION_WRITE_ONLY_WITHOUT_VERSION_INFO:
    default:
        break;
    }

    if (IsInOrOutputFilterProcessPipe(fh)) {
        if (strcmp("stdin", DstFileNameFullPath) && strcmp("stdout", DstFileNameFullPath)) {
            CloseInOrOutputFilterProcessPipe(fh);
        }
    } else {
        close_file(fh);
    }
    if (ProgressBarID >= 0) CloseProgressBarFromOtherThread(ProgressBarID);
    INI_LEAVE_CS (&IniDBCriticalSection);
    return 0;
__ERROUT:
    if (fh != NULL) {
        if (IsInOrOutputFilterProcessPipe(fh)) {
            CloseInOrOutputFilterProcessPipe(fh);
        } else {
            close_file(fh);
        }
    }
    INI_LEAVE_CS (&IniDBCriticalSection);
    return -1;
}

int IniFileDataBaseSave(int par_FileDescriptor, const char *par_DstFileName, int par_Operation)
{
    return IniFileDataBaseSaveInternal(par_FileDescriptor, par_DstFileName, par_Operation, 1);
}

int IniFileDataBaseSaveNoFilterPossible(int par_FileDescriptor, const char *par_DstFileName, int par_Operation)
{
    return IniFileDataBaseSaveInternal(par_FileDescriptor, par_DstFileName, par_Operation, 0);
}

#define CHAR_UPPER(c) (c)
static __inline const char *CompareEntryOfLine (const char *par_Entry, const char *par_Line)
{
    const char *l = par_Line;
    const char *e = par_Entry;
    uint32_t a, b;

    while (*l != '\0') {
        if (*l == '=') {
            if (*e != '\0') return NULL;
            return (l+1);         // equal
        } else {
            if (*e == '\0') return NULL;
            a = CHAR_UPPER (*l);
            b = CHAR_UPPER (*e);
            if (a != b) return NULL;   // not equal
        }
        l++; e++;
    }
    return NULL;
}

static const char *GetEntry(INI_DB_SECTION_ELEM *par_Section, const char *par_Entry)
{
    int e;
    for (e = 0; e < par_Section->EntryCount; e++) {
        const char *Text = CompareEntryOfLine(par_Entry, par_Section->Entrys[e]);
        if (Text != NULL) {
            return Text;
        }
    }
    return NULL;
}

static char* AddNewEntry(INI_DB_SECTION_ELEM *par_Section, const char *par_Entry, int par_EntrySize,
                         const char *par_Text, int par_TextSize)
{
    char *NewEntryLine;
    if (par_Section->EntryCount >= par_Section->EntrySize) {
        par_Section->EntrySize += par_Section->EntrySize / 4 + 8;
        par_Section->Entrys = (char**)my_realloc(par_Section->Entrys,
                                                 sizeof(char*) * par_Section->EntrySize);
        if (par_Section->Entrys == NULL) {
            par_Section->EntrySize = 0;
            par_Section->EntryCount = 0;
            return NULL;
        }
    }
    NewEntryLine = (char*)my_malloc(par_EntrySize + par_TextSize + 2);   // +2 becaus of "=" and termination 0 character
    if (NewEntryLine == NULL) {
        return NULL;
    }
    memcpy(NewEntryLine, par_Entry, par_EntrySize);
    NewEntryLine[par_EntrySize] = '=';
    memcpy(NewEntryLine + par_EntrySize + 1, par_Text, par_TextSize + 1);
    par_Section->Entrys[par_Section->EntryCount] = NewEntryLine;
    par_Section->EntryCount++;
    return NewEntryLine;
}

static int RplaceOrAddNewEntry(INI_DB_SECTION_ELEM *par_Section, const char *par_Entry, const char *par_Text)
{
    int e;
    int Ret = 0;
    int EntryLen = strlen(par_Entry);
    int TextLen = strlen(par_Text);
    for (e = 0; e < par_Section->EntryCount; e++) {
        char *EntryLine = par_Section->Entrys[e];
        const char *Text = CompareEntryOfLine(par_Entry, EntryLine);
        if (Text != NULL) {
            EntryLine = my_realloc(EntryLine, EntryLen + TextLen + 2);   // + 2 for "=" and the terminating 0
            if (EntryLine == NULL) {
                // we have destroyed this entry (remove it)
                par_Section->EntryCount--;
                for ( ; e < par_Section->EntryCount; e++) {
                    par_Section->Entrys[e] = par_Section->Entrys[e + 1];
                }
            } else {
                memcpy(EntryLine, par_Entry, EntryLen);
                EntryLine[EntryLen] = '=';
                memcpy(EntryLine + EntryLen + 1, par_Text, TextLen + 1);
                par_Section->Entrys[e] = EntryLine;
            }
            Ret = 1;
            break;
        }
    }
    if (!Ret) {
        Ret = (AddNewEntry(par_Section, par_Entry, EntryLen, par_Text, TextLen) != NULL);
    }
    return Ret;
}

static int DeleteEntry(INI_DB_SECTION_ELEM *par_Section, const char *par_Entry)
{
    int e;
    for (e = 0; e < par_Section->EntryCount; e++) {
        char *EntryLine = par_Section->Entrys[e];
        const char *Text = CompareEntryOfLine(par_Entry, EntryLine);
        if (Text != NULL) {
            my_free(EntryLine);
            par_Section->EntryCount--;
            for ( ; e < par_Section->EntryCount; e++) {
                par_Section->Entrys[e] = par_Section->Entrys[e + 1];
            }
            return 1;
        }
    }
    return 0;
}

static INI_DB_SECTION_ELEM *GetSection(int par_FileIndex, const char* par_Section)
{
    int s = AllLoadedIniFiles[par_FileIndex].CachedIndex;
    INI_DB_SECTION_ELEM *Ret = NULL;
    if ((s >= 0) &&
        (s < AllLoadedIniFiles[par_FileIndex].SectionCount)) {
        INI_DB_SECTION_ELEM *Section = AllLoadedIniFiles[par_FileIndex].Sections[s];
        if (!strcmp(par_Section, Section->Name)) {
            AllLoadedIniFiles[par_FileIndex].CacheHitCounter++;
            Ret = Section;
            return Ret;
        }
    }
    for (s = 0; s < AllLoadedIniFiles[par_FileIndex].SectionCount; s++) {
        INI_DB_SECTION_ELEM *Section = AllLoadedIniFiles[par_FileIndex].Sections[s];
        if (!strcmp(par_Section, Section->Name)) {
            AllLoadedIniFiles[par_FileIndex].CacheMissCounter++;
            AllLoadedIniFiles[par_FileIndex].CachedIndex = s;
            Ret = Section;
            break;
        }
    }
    return Ret;
}

static INI_DB_SECTION_ELEM *AddNewSection(int par_FileIndex, const char *par_Section)
{
    int Len;
    INI_DB_SECTION_ELEM *NewSection;
    if (AllLoadedIniFiles[par_FileIndex].SectionCount >= AllLoadedIniFiles[par_FileIndex].SectionSize) {
        AllLoadedIniFiles[par_FileIndex].SectionSize += AllLoadedIniFiles[par_FileIndex].SectionSize / 4 + 8;
        AllLoadedIniFiles[par_FileIndex].Sections = (INI_DB_SECTION_ELEM**)my_realloc(AllLoadedIniFiles[par_FileIndex].Sections,
                                                                                      sizeof(INI_DB_SECTION_ELEM*) * AllLoadedIniFiles[par_FileIndex].SectionSize);
        if (AllLoadedIniFiles[par_FileIndex].Sections == NULL) {
            AllLoadedIniFiles[par_FileIndex].SectionSize = 0;
            AllLoadedIniFiles[par_FileIndex].SectionCount = 0;
            return NULL;
        }
    }
    NewSection = (INI_DB_SECTION_ELEM*)my_calloc(1, sizeof(INI_DB_SECTION_ELEM));
    if (NewSection == NULL) {
        return NULL;
    }
    Len = strlen(par_Section) + 1;
    NewSection->Name = (char*)my_malloc(Len);
    if (NewSection->Name == NULL) {
        my_free(NewSection);
        return NULL;
    }
    memcpy(NewSection->Name, par_Section, Len);
    AllLoadedIniFiles[par_FileIndex].Sections[AllLoadedIniFiles[par_FileIndex].SectionCount] = NewSection;
    AllLoadedIniFiles[par_FileIndex].SectionCount++;
    return NewSection;
}

static void FreeSection(INI_DB_SECTION_ELEM *par_Section)
{
    if (par_Section != NULL) {
        if (par_Section->Entrys != NULL) {
            int e;
            for (e = 0; e < par_Section->EntryCount; e++) {
                if (par_Section->Entrys[e] != NULL) {
                    my_free(par_Section->Entrys[e]);
                }
            }
            my_free(par_Section->Entrys);
        }
        if (par_Section->Name != NULL) {
            my_free(par_Section->Name);
        }
        my_free(par_Section);
    }
}

static int DeleteSection(int par_FileIndex, const char *par_Section)
{
    int s;
    INI_DB_SECTION_ELEM *Ret = NULL;
    for (s = 0; s < AllLoadedIniFiles[par_FileIndex].SectionCount; s++) {
        INI_DB_SECTION_ELEM *Section = AllLoadedIniFiles[par_FileIndex].Sections[s];
        if (!strcmp(par_Section, Section->Name)) {
            FreeSection(AllLoadedIniFiles[par_FileIndex].Sections[s]);
            AllLoadedIniFiles[par_FileIndex].SectionCount--;
            for ( ; s < AllLoadedIniFiles[par_FileIndex].SectionCount; s++) {
                AllLoadedIniFiles[par_FileIndex].Sections[s] = AllLoadedIniFiles[par_FileIndex].Sections[s + 1];
            }
            return 1;
        }
    }
    return 0;
}

static int IniFileDataBaseWriteStringNoLock (const char* par_Section, const char* par_Entry, const char* par_Text, int par_FileDescriptor)
{
    int e;
    int FileIndex;
    int Ret = 0;

    FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex >= 0) {
        if (par_Entry == NULL) {
            DeleteSection(FileIndex, par_Section);
        } else {
            INI_DB_SECTION_ELEM *Section;
            Section = GetSection(FileIndex, par_Section);
            if (Section == NULL) {
                Section = AddNewSection(FileIndex, par_Section);
            }
            if (Section != NULL) {
                if (par_Text == NULL) {
                    // delete the entry
                    Ret = DeleteEntry(Section, par_Entry);
                    if (Section->EntryCount == 0) {
                        // if section is empty delete it
                        DeleteSection(FileIndex, par_Section);
                    }
                    goto __OUT;
                } else {
                    Ret = RplaceOrAddNewEntry(Section, par_Entry, par_Text);
                }
            }
        }
    }
__OUT:
    return Ret;
}

int IniFileDataBaseWriteString (const char* par_Section, const char* par_Entry, const char* par_Text, int par_FileDescriptor)
{
    int Ret;
    INI_ENTER_CS(&IniDBCriticalSection);
    Ret = IniFileDataBaseWriteStringNoLock (par_Section, par_Entry, par_Text, par_FileDescriptor);
    INI_LEAVE_CS(&IniDBCriticalSection);
    return Ret;
}

int IniFileDataBaseWriteInt (const char* par_Section, const char* par_Entry, int par_DefaultValue, int par_FileDescriptor)
{
    char Help[64];
    sprintf (Help, "%i", par_DefaultValue);
    return IniFileDataBaseWriteString (par_Section, par_Entry, Help, par_FileDescriptor);
}

int IniFileDataBaseWriteUInt (const char* par_Section, const char* par_Entry, unsigned int par_DefaultValue, int par_FileDescriptor)
{
    char Help[64];
    sprintf (Help, "%u", par_DefaultValue);
    return IniFileDataBaseWriteString (par_Section, par_Entry, Help, par_FileDescriptor);
}

int IniFileDataBaseWriteUIntHex (const char* par_Section, const char* par_Entry, unsigned int par_DefaultValue, int par_FileDescriptor)
{
    char Help[64];
    sprintf (Help, "0x%X", par_DefaultValue);
    return IniFileDataBaseWriteString (par_Section, par_Entry, Help, par_FileDescriptor);
}

int IniFileDataBaseWriteFloat (const char* par_Section, const char* par_Entry, double par_DefaultValue, int par_FileDescriptor)
{
    char Help[128];
    int Prec = 15;
    while (1) {
        sprintf (Help, "%.*g", Prec, par_DefaultValue);
        if ((Prec++) == 18 || (par_DefaultValue == strtod (Help, NULL))) break;
    }
    return IniFileDataBaseWriteString (par_Section, par_Entry, Help, par_FileDescriptor);
}

int IniFileDataBaseWriteYesNo (const char* par_Section, const char* par_Entry, int par_DefaultValue, int par_FileDescriptor)
{
    return IniFileDataBaseWriteString (par_Section, par_Entry, (par_DefaultValue) ? "yes" : "no", par_FileDescriptor);
}

int IniFileDataBaseWriteSection (const char* par_Section, const char* par_SectionText, int par_FileDescriptor)
{
    const char *p1 = par_SectionText;
    const char *p2;
    char entry[INI_MAX_ENTRYNAME_LENGTH];
    char *e;
    char section[INI_MAX_SECTION_LENGTH + MAX_PATH + 5];

    INI_ENTER_CS (&IniDBCriticalSection);
    // First delete section (if exist)
    IniFileDataBaseWriteString (par_Section, NULL, NULL, par_FileDescriptor);

    while (*p1 != '\0') {
        int char_counter = 0;
        e = entry;
        while (*p1 != '=') {
            if (*p1 == '\0') {
                *e = '\0';
                INI_LEAVE_CS (&IniDBCriticalSection);
                return 0;
            }
            char_counter++;
            if (char_counter == (sizeof (entry)-1)) {
                INI_LEAVE_CS (&IniDBCriticalSection);
                return 0;
            }
            *e++ = *p1++;
        }
        *e = '\0';
        p1++;   // jump over '='
        p2 = p1;
        while (*p2 != '\0') p2++;
        if (IniFileDataBaseWriteString ((char*)section, entry, p1, par_FileDescriptor) == -1) {
            INI_LEAVE_CS (&IniDBCriticalSection);
            return 0;
        }
        p2++;   // jump over '\0'
        p1 = p2;
    }
    INI_LEAVE_CS (&IniDBCriticalSection);
    return 1; // OK
}


int IniFileDataBaseReadString (const char* par_Section, const char* par_Entry, const char* par_DefaulText,
                               char* ret_Text, int par_MaxSize, int par_FileDescriptor)
{
    INI_DB_SECTION_ELEM *Section;
    int Ret;
    INI_ENTER_CS(&IniDBCriticalSection);
    int FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex >= 0) {
        Section = GetSection(FileIndex, par_Section);
        if (Section != NULL) {
            int e;
            for (e = 0; e < Section->EntryCount; e++) {
                char *EntryLine = Section->Entrys[e];
                const char *Text = CompareEntryOfLine(par_Entry, EntryLine);
                if (Text != NULL) {
                    StringCopyMaxCharTruncate(ret_Text, Text, par_MaxSize);
                    Ret = strlen(Text);
                    goto __OUT;
                }
            }
        }
    }
    StringCopyMaxCharTruncate(ret_Text, par_DefaulText, par_MaxSize);
    Ret = strlen(par_DefaulText);
 __OUT:
    INI_LEAVE_CS(&IniDBCriticalSection);
    return Ret;
}


char *IniFileDataBaseReadStringBufferNoDef (const char* par_Section, const char* par_Entry, int par_FileDescriptor)
{
    int FileIndex;
    char *Ret = NULL;
    INI_DB_SECTION_ELEM *Section;
    INI_ENTER_CS(&IniDBCriticalSection);
    FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex >= 0) {
        Section = GetSection(FileIndex, par_Section);
        if (Section != NULL) {
            int e;
            for (e = 0; e < Section->EntryCount; e++) {
                char *EntryLine = Section->Entrys[e];
                const char *Text = CompareEntryOfLine(par_Entry, EntryLine);
                if (Text != NULL) {
                    int Len = strlen(Text) + 1;
                    Ret = my_malloc(Len);
                    if (Ret != NULL) {
                        strcpy(Ret, Text);
                    }
                    break;
                }
            }
        }
    }
    INI_LEAVE_CS(&IniDBCriticalSection);
    return Ret;
}

char *IniFileDataBaseReadStringBuffer (const char* par_Section, const char* par_Entry, const char* par_DefaulText,
                                      int par_FileDescriptor)
{
    char *Ret = IniFileDataBaseReadStringBufferNoDef (par_Section, par_Entry, par_FileDescriptor);
    if (Ret == NULL) {
        int Len = strlen(par_DefaulText)+1;
        Ret = (char*)my_malloc(Len);
        if (Ret != NULL) {
            StringCopyMaxCharTruncate(Ret, par_DefaulText, Len);
        }
    }
    return Ret;
}

void IniFileDataBaseReadStringBufferFree (char *par_Buffer)
{
    if (par_Buffer != NULL) my_free(par_Buffer);
}

int IniFileDataBaseReadInt (const char* par_Section, const char* par_Entry, int par_DefaultValue, int par_FileDescriptor)
{
    char BufferDef[64];
    char Buffer[64];

    sprintf (BufferDef, "%i", (int32_t)par_DefaultValue);
    IniFileDataBaseReadString(par_Section, par_Entry, BufferDef, Buffer, 64, par_FileDescriptor);
    return strtol (Buffer, NULL, 0);
}

unsigned int IniFileDataBaseReadUInt (const char* par_Section, const char* par_Entry, unsigned int par_DefaultValue, int par_FileDescriptor)
{
    char BufferDef[64];
    char Buffer[64];

    sprintf (BufferDef, "%u", par_DefaultValue);
    IniFileDataBaseReadString(par_Section, par_Entry, BufferDef, Buffer, 64, par_FileDescriptor);
    return strtoul (Buffer, NULL, 0);
}

double IniFileDataBaseReadFloat (const char* par_Section, const char* par_Entry, double par_DefaultValue, int par_FileDescriptor)
{
    char BufferDef[64];
    char Buffer[64];

    sprintf (BufferDef, "%g", par_DefaultValue);
    IniFileDataBaseReadString(par_Section, par_Entry, BufferDef, Buffer, 64, par_FileDescriptor);
    return strtod (Buffer, NULL);
}

int IniFileDataBaseReadYesNo (const char* par_Section, const char* par_Entry, int par_DefaultValue, int par_FileDescriptor)
{
    int Ret;
    char Buffer[64];
    IniFileDataBaseReadString (par_Section, par_Entry, (par_DefaultValue) ? "yes" : "no", Buffer, sizeof (Buffer), par_FileDescriptor);
    if (!_stricmp(Buffer, "yes")) {
        Ret = 1;
    } else if (!_stricmp(Buffer, "no")) {
        Ret = 0;
    } else {
        // "0" and "1" are also allowed
        char *p = NULL;
        Ret = strtol(Buffer, &p, 0);
        if ((p > Buffer) && (p < (Buffer + sizeof(Buffer))) && (*p == 0)) {
            if (Ret != 0) {
                Ret = 1;
            } else {
                Ret = 1;
            }
        } else {
            Ret = 0;
        }
    }
    return Ret;
}

int IniFileDataBaseReadSection (const char* par_Section, char* par_Text, int par_MaxSize, int par_FileDescriptor)
{
    int Ret = 0;
    int FileIndex;

    if ((par_Section == NULL) || (par_Text == NULL) || (par_MaxSize < 2)) {
        return 0;
    }

    INI_ENTER_CS (&IniDBCriticalSection);

    FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex >= 0) {
        INI_DB_SECTION_ELEM *Section;
        Section = GetSection(FileIndex, par_Section);
        if (Section != NULL) {
            int e;
            int c = 1;   // tailing 0 character
            char *d = par_Text;
            for (e = 0; e < Section->EntryCount; e++) {
                char *EntryLine = Section->Entrys[e];
                int Len = strlen(EntryLine) + 1;
                c += Len;
                if (c < par_MaxSize) {
                    memcpy(d, EntryLine, Len);
                    d += Len;
                }
            }
            *d = 0;
            if (c < par_MaxSize) {
                Ret = c;
            } else {
                Ret = par_MaxSize - 2;
            }
            goto __OUT;
        } else {
            par_Text[0] = 0;
            par_Text[1] = 0;
            Ret = 0;
        }
    }
__OUT:
    INI_LEAVE_CS (&IniDBCriticalSection);
    return Ret;
}

char *IniFileDataBaseReadSectionBuffer (const char* par_Section, int par_FileDescriptor)
{
    char *Ret = NULL;
    int FileIndex;

    if (par_Section == NULL) {
        return NULL;
    }

    INI_ENTER_CS (&IniDBCriticalSection);

    FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex >= 0) {
        INI_DB_SECTION_ELEM *Section;
        Section = GetSection(FileIndex, par_Section);
        if (Section != NULL) {
            int e;
            char *d;
            int c = 1;   // for tailing 0 character
            // first only count the size
            for (e = 0; e < Section->EntryCount; e++) {
                char *EntryLine = Section->Entrys[e];
                int Len = strlen(EntryLine) + 1;
                c += Len;
            }
            // now copy it
            Ret = (char*)my_malloc(c);
            if (Ret != NULL) {
                d = Ret;
                for (e = 0; e < Section->EntryCount; e++) {
                    char *EntryLine = Section->Entrys[e];
                    int Len = strlen(EntryLine) + 1;
                    c += Len;
                    memcpy(d, EntryLine, Len);
                    d += Len;
                }
                *d = 0;
            }
            goto __OUT;
        }
    }
__OUT:
    INI_LEAVE_CS (&IniDBCriticalSection);
    return Ret;
}

void IniFileDataBaseReadSectionBufferFree (char *par_Buffer)
{
    if(par_Buffer != NULL) my_free(par_Buffer);
}

int IniFileDataBaseWriteByteImage (const char* par_Section, const char* par_EntryPrefix, int par_Type,
                                   void *par_Image, int par_Len, int par_FileDescriptor)
{
    char EntryHelp[INI_MAX_ENTRYNAME_LENGTH + 16];
    char *d;
    const char *s;
    int x;
    int FileIndex;
    int Ret = 0;

    if (par_Section == NULL) {
        return 0;
    }

    INI_ENTER_CS (&IniDBCriticalSection);

    FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex >= 0) {
        if (par_EntryPrefix == NULL) {
            DeleteSection(FileIndex, par_Section);
        } else {
            INI_DB_SECTION_ELEM *Section;
            Section = GetSection(FileIndex, par_Section);
            if (Section == NULL) {
                Section = AddNewSection(FileIndex, par_Section);
            }
            if (Section != NULL) {
                if (par_Image == NULL) {
                    // delete all extrys with prefix
                    for (x = 0; ; x++) {
                        if (DeleteEntry(Section, EntryHelp)) {
                            break;
                        }
                    }
                    Ret = (x >= 1);
                    goto __OUT;
                } else {
                    // The image will be stored inside hex values with with a length of 1024 chars (512 bytes).
                    // The firstline is a header (name, bytes count, ...)
                    static const char Table[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
                    char Line[16+16+16+512*2+1];  // Type,Len,CRC,max.512Bytes
                    int x;
                    char *d;
                    unsigned char *s;
                    uint32_t crc32;
                    uint32_t LineCounter = 0;

                    s = (unsigned char*)par_Image;
                    d = Line; // avoid warning
                    crc32 = 0;  // not implemented
                    for (x = 0; x < par_Len; x++) {
                        if ((x & 0x1FF) == 0) {
                            if (x > 0) {
                                // Write line into INI file
                                *d = 0;  // terminate line
                                sprintf (EntryHelp, "%s_%i", par_EntryPrefix, LineCounter);
                                LineCounter++;
                                if (!RplaceOrAddNewEntry(Section, EntryHelp, Line)) {
                                    Ret = 0;
                                    goto __OUT;
                                }
                                // additional new line
                                d = Line;
                            } else {
                                // First new line: put in front total length and CRC
                                sprintf (Line, "%i,%u,0x%08X,", par_Type, par_Len, crc32);
                                d = Line + strlen(Line);
                            }
                        }
                        // A byte will be represent with 2 chars
                        *d++ = Table[*s >> 4];
                        *d++ = Table[*s & 0xF];
                        s++;
                    }
                    if (d != Line) {
                        // Write last line to INI file
                        *d = 0;  // terminate line
                        sprintf (EntryHelp, "%s_%i", par_EntryPrefix, LineCounter);
                        LineCounter++;
                        if (!RplaceOrAddNewEntry(Section, EntryHelp, Line)) {
                            Ret = 0;
                            goto __OUT;
                        }
                    }
                    // Remove all additional entrys with prefix
                    for (; ; LineCounter++) {
                        sprintf (EntryHelp, "%s_%i", par_EntryPrefix, LineCounter);
                        if (!DeleteEntry(Section, (char*)EntryHelp)) {
                            break;
                        }
                    }
                }
            }
        }
    }
__OUT:
    INI_LEAVE_CS (&IniDBCriticalSection);
    return Ret;
}

// If par_BufferSize <= 0 and ret_Buffer == NULL than only the needed buffer size will returned
// If par_BufferSize <= 0 and ret_Buffer != NULL than the buffer will be allocated, This buffer must be give free with DBFreeByteImageBuffer
int IniFileDataBaseReadByteImage (const char* par_Section, const char* par_EntryPrefix, int *ret_Type,
                                  void *ret_Buffer, int par_BufferSize, int par_FileDescriptor)
{
    int x, index;
    char EntryHelp[INI_MAX_ENTRYNAME_LENGTH + 16];
    int FileIndex;
    int Ret = 0;
    const char *Text;
    char TextCopy[INI_MAX_LINE_LENGTH];
    char *d;
    const char *s;
    char *type_string;
    char *len_string;
    char *crc_string;
    char *bytes_string;
    int image_size;
    uint32_t crc32;
    int LineCounter = 0;

    static unsigned char Table[256] = {
        /* 0 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        /* 1 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        /* 2 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        /* 3 */ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,   0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 0...9
        /* 4 */ 0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // A...F
        /* 5 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        /* 6 */ 0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // a...f
        /* 7 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

        /* 8 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        /* 9 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        /* A */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        /* B */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        /* C */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        /* D */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        /* E */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        /* F */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };

    if ((par_Section == NULL) || (par_EntryPrefix == NULL)) {
        return 0;
    }

    INI_ENTER_CS(&IniDBCriticalSection);
    FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex >= 0) {
        INI_DB_SECTION_ELEM *Section;
        Section = GetSection(FileIndex, par_Section);
        if (Section != NULL) {
            // First line contains name and size
            sprintf (EntryHelp, "%s_%i", par_EntryPrefix, LineCounter);
            LineCounter++;
            Text = GetEntry(Section, EntryHelp);
            if (Text == NULL) {
                INI_LEAVE_CS (&IniDBCriticalSection);
                Ret = 0;    // First line doesn't exist
                goto __OUT;
            }
            StringCopyMaxCharTruncate(TextCopy, Text, sizeof(TextCopy));
            if (StringCommaSeparate(TextCopy, &type_string, &len_string, &crc_string, &bytes_string, NULL) < 3) {
                Ret = 0;    // Fist line contains not "Type, Anzahl Bytes, CRC, Bytes"
                goto __OUT;
            }
            if (ret_Type != NULL) {
                *ret_Type = strtol (type_string, NULL, 0);
            }
            Ret = image_size = strtol (len_string, NULL, 0);
            if ((image_size < 1) || (image_size > 0x1000000)) {   // Image 1...16MByte large
                Ret = 0;
                goto __OUT;
            }
            crc32 = strtoul (crc_string, NULL, 0);

            // Only return the needed buffer size
            if ((par_BufferSize <= 0) && (ret_Buffer == NULL)) {
                goto __OUT;
            }

            if ((par_BufferSize <= 0) && (ret_Buffer != NULL)) {
                // Alloc the buffer
                void *Buffer;
                Buffer = my_malloc ((size_t)image_size);
                if (Buffer == NULL) {
                    Ret = 0;
                    goto __OUT;
                }
                d = Buffer;
                // Return the base address of the buffer
                *(void**)ret_Buffer = Buffer;
            } else {
                d = ret_Buffer;
            }

            s = bytes_string;
            for (x = 0; x < image_size; x++) {
                int idx;
                if (((x & 0x1FF) == 0) && (x != 0)) {
                    // Read line from INI file
                    sprintf (EntryHelp, "%s_%i", par_EntryPrefix, LineCounter);
                    LineCounter++;
                    Text = GetEntry(Section, EntryHelp);
                    if (Text == NULL) {
                        Ret = 0;    // Expected line doesn't exist
                        goto __OUT;
                    }
                    StringCopyMaxCharTruncate(TextCopy, Text, sizeof(TextCopy));
                    s = TextCopy;
                }
                // One byte will be represent with to characters
                idx = *(const unsigned char*)s;
                if (Table[idx] == 0xFF) {
                    Ret = 0;    // Invalid character
                    goto __OUT;
                }
                d[x] = (char)Table[idx];
                d[x] <<= 4;
                s++;
                idx = *(const unsigned char*)s;
                if (Table[idx] == 0xFF) {
                    Ret = 0;    // Invalid character
                    goto __OUT;
                }
                d[x] |= Table[idx];
                s++;
            }
        }
    }
    INI_LEAVE_CS (&IniDBCriticalSection);
__OUT:
    return (DWORD)Ret;
}


void IniFileDataBaseFreeByteImageBuffer (void *par_Buffer)
{
    if (par_Buffer != NULL) my_free (par_Buffer);
}


int IniFileDataBaseGetNextSectionName (int par_Index, char *ret_Section, int par_MaxLen, int par_FileDescriptor)
{
    int FileIndex;
    int Ret = -1;
    INI_ENTER_CS (&IniDBCriticalSection);
    FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex >= 0) {
        if (par_Index < AllLoadedIniFiles[FileIndex].SectionCount) {
            StringCopyMaxCharTruncate(ret_Section, AllLoadedIniFiles[FileIndex].Sections[par_Index]->Name, par_MaxLen);
            Ret = par_Index + 1;
        }
    }
    INI_LEAVE_CS (&IniDBCriticalSection);
    return Ret;
}

static void CopyEntryName(char *ret_Dst, const char *par_Src, int par_MaxLen)
{
    int c = 1;
    while((*par_Src != '=') &&
           (*par_Src != 0) &&
           (c < par_MaxLen)) {
        *ret_Dst++ = *par_Src++;
    }
    *ret_Dst = 0;
}

int IniFileDataBaseGetNextEntryName (int par_Index, const char *par_Section, char *ret_Entry, int par_MaxLen,
                                    int par_FileDescriptor)
{
    int FileIndex;
    int Ret = -1;
    INI_ENTER_CS (&IniDBCriticalSection);
    FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex >= 0) {
        INI_DB_SECTION_ELEM *Section;
        Section = GetSection(FileIndex, par_Section);
        if (Section != NULL) {
            if (par_Index < Section->EntryCount) {
                CopyEntryName(ret_Entry, Section->Entrys[par_Index], par_MaxLen);
                Ret = par_Index + 1;
            }
        }
    }
    INI_LEAVE_CS (&IniDBCriticalSection);
    return Ret;
}

int IniFileDataBaseCopySection (int par_DstFileDescriptor, int par_SrcFileDescriptor,
                                const char *par_DstSection, const char *par_SrcSection)
{
    int SrcFileIndex;
    int DstFileIndex;
    INI_DB_SECTION_ELEM *DstSection = NULL;
    int Ret = -1;

    INI_ENTER_CS (&IniDBCriticalSection);
    SrcFileIndex = GetIndexByDescriptor(par_SrcFileDescriptor);
    if (SrcFileIndex >= 0) {
        DstFileIndex = GetIndexByDescriptor(par_DstFileDescriptor);
        if (DstFileIndex >= 0) {
            INI_DB_SECTION_ELEM *SrcSection;
            SrcSection = GetSection(SrcFileIndex, par_SrcSection);
            if (SrcSection != NULL) {
                DeleteSection(DstFileIndex, par_DstSection);
                DstSection = AddNewSection(DstFileIndex, par_DstSection);
                if (DstSection != NULL) {
                    DstSection->Entrys = (char**)my_malloc(SrcSection->EntrySize * sizeof(char*));
                    if (DstSection->Entrys != NULL) {
                        int e;
                        for (e = 0; e < SrcSection->EntryCount; e++) {
                            int Len = strlen(SrcSection->Entrys[e]) + 1;
                            DstSection->Entrys[e] = my_malloc(Len);
                            if (DstSection->Entrys[e] == NULL) goto __ERROUT;
                            memcpy(DstSection->Entrys[e], SrcSection->Entrys[e], Len);
                        }
                        DstSection->EntryCount = SrcSection->EntryCount;
                        DstSection->EntrySize = SrcSection->EntrySize;
                        Ret = 0;
                        goto __OUT;
                    }
                }
            }
        }
    }
__ERROUT:
    if (DstSection != NULL) {
        FreeSection(DstSection);
    }
__OUT:
    INI_LEAVE_CS (&IniDBCriticalSection);
    return Ret;
}

int IniFileDataBaseCopySectionSameName (int par_DstFileDescriptor, int par_SrcFileDescriptor, const char *par_Section)
{
    return IniFileDataBaseCopySection(par_DstFileDescriptor, par_SrcFileDescriptor, par_Section, par_Section);
}

int IniFileDataBaseCopySectionSameFile (int par_FileDescriptor, const char *par_DstSection, const char *par_SrcSection)
{
    return IniFileDataBaseCopySection(par_FileDescriptor, par_FileDescriptor, par_DstSection, par_SrcSection);
}

int IniFileDataBaseRenameFile (const char *par_NewName, int par_FileDescriptor)
{
    int FileIndex;
    int Ret = -1;
    INI_ENTER_CS (&IniDBCriticalSection);
    FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex >= 0) {
        char *NewName = my_malloc(strlen(par_NewName)+1);
        if (NewName != NULL) {
            strcpy(NewName, par_NewName);
            my_free(AllLoadedIniFiles[FileIndex].Filename);
            AllLoadedIniFiles[FileIndex].Filename = NewName;
            Ret = 0;
        }
    }
    INI_LEAVE_CS (&IniDBCriticalSection);
    return Ret;
}

int IniFileDataBaseFindNextSectionNameRegExp (int par_Index, const char *par_Filter, int par_CaseSensetive,
                                              char *ret_Name, int par_MaxLen, int par_FileDescriptor)
{
    char Section[INI_MAX_SECTION_LENGTH];
    int Index = par_Index;
    do {
        Index = IniFileDataBaseGetNextSectionName (Index, Section, sizeof(Section), par_FileDescriptor);
        if (Index >= 0) {
            if (!Compare2StringsWithWildcardsCaseSensitive (Section, par_Filter,par_CaseSensetive)) {
                StringCopyMaxCharTruncate(ret_Name, Section, par_MaxLen);
                return Index;
            }
        }
    } while (Index >= 0);
    return -1;
}

int IniFileDataBaseLookIfSectionExist (const char *par_Section, int par_FileDescriptor)
{
    int FileIndex;
    int Ret = 0;
    INI_ENTER_CS (&IniDBCriticalSection);
    FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex >= 0) {
        int s;
        for (s = 0; s < AllLoadedIniFiles[FileIndex].SectionCount; s++) {
            if (!strcmp(par_Section, AllLoadedIniFiles[FileIndex].Sections[s]->Name)) {
                Ret = 1;
                break;
            }
        }
    }
    INI_LEAVE_CS (&IniDBCriticalSection);
    return Ret;
}

int IniFileDataBaseGetSectionNumberOfEntrys (const char *par_Section, int par_FileDescriptor)
{
    int FileIndex;
    int Ret = -1;

    INI_ENTER_CS (&IniDBCriticalSection);
    FileIndex = GetIndexByDescriptor(par_FileDescriptor);
    if (FileIndex >= 0) {
        INI_DB_SECTION_ELEM *Section;
        Section = GetSection(FileIndex, par_Section);
        if (Section != NULL) {
            Ret = Section->EntryCount;
        }
    }
    INI_LEAVE_CS (&IniDBCriticalSection);
    return Ret;
}

void IniFileDataBaseEnterCriticalSectionUser(const char *par_File, int par_Line)
{
    INI_ENTER_CS (&IniDBCriticalSectionUser);
    IniDBCriticalSectionUserFile = par_File;
    IniDBCriticalSectionUserLine = par_Line;
}

void IniFileDataBaseLeaveCriticalSectionUser(void)
{
    INI_LEAVE_CS (&IniDBCriticalSectionUser);
}

int CreateNewEmptyIniFile(const char *par_Name)
{
    FILE *fh;

    if ((fh = open_file (par_Name, "wt")) == NULL) {
        return -1;
    }
    if (fprintf (fh, "; This is an OpenXilEnv configuration file.\n"
                     "; The syntax is similar to an INI file except that the section and entry name are case sensitive\n"
                     "[%s]\n"
                     "  %s=%g.%d\n",
                     OPT_FILE_INFOS, OPT_FILE_VERSION, XILENV_VERSION/1000.0, XILENV_MINOR_VERSION) == EOF) {
        close_file(fh);
        return -1;
    }

    close_file(fh);
    return 0;
}

int IniFileDataBaseCreateAndOpenNewIniFile(const char *par_Name)
{
    if (CreateNewEmptyIniFile(par_Name) == 0) {
        return IniFileDataBaseOpenNoFilterPossible(par_Name);
    }
    return -1;
}

int IsAValidSectionName(const char *par_Name)
{
    const char *p = par_Name;
    while(*p != 0) {
        if ((*p < ' ') || (*p > '~') || (*p == '[') || (*p == ']') || (*p == '\\')) {
            return 0;   // not valid
        }
    }
    return ((p - par_Name) < INI_MAX_SECTION_LENGTH);
}

int set_ini_path (const char *sz_inipath)
{
    char fn_buffer[MAX_PATH];

    if (expand_filename (sz_inipath, fn_buffer, sizeof (fn_buffer))) return -1;

    if ((strlen(fn_buffer) < MAX_PATH) && (access(fn_buffer, 0) == 0)) {

        // Store INI file into global variable
        SetMainIniFilename(fn_buffer);

        AddIniFileToHistory (fn_buffer);

        return 0;
    }
    return -1;
}

int SaveAllInfosToIniDataBase (void)
{
    WriteBasicConfigurationToIni(&s_main_ini_val);
    return 0;
}
