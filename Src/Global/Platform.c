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


#include <inttypes.h>
#include <stdio.h>
#include "Files.h"
#include "Platform.h"

/* do not include: #include "ThrowError.h"
   instead own declaration */
int ThrowError (int level, const char *format, ...);

#define UNUSED(x) (void)(x)

#ifndef _WIN32
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
//#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/auxv.h>
#include <sys/mman.h>

uint64_t GetTickCount64(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64_t)(ts.tv_nsec / 1000000) + ((uint64_t)ts.tv_sec *1000ull);
}

uint32_t GetTickCount(void)
{
    return (uint32_t)GetTickCount64();
}


int GetEnvironmentVariable(const char *EnvVarName,
                           char *EnvVarValue,
                           int SizeofEnvVarValue)
{
    char *env;

    env = getenv(EnvVarName);
    if (env != NULL) {
        int len = (int)strlen(env);
        strncpy(EnvVarValue, env, SizeofEnvVarValue);
        if (len >= SizeofEnvVarValue) EnvVarValue[SizeofEnvVarValue-1] = 0;
        return len;
    } else {
        return 0;
    }
}

int SetEnvironmentVariable(const char *EnvVarName,
                           const char *EnvVarValue)
{
    return !setenv(EnvVarName, EnvVarValue, 1);
}

int sc_pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, int time_ms)
{
    struct timespec abstime;
    double help;

    clock_gettime(CLOCK_MONOTONIC, &abstime);
    help = (double)abstime.tv_sec + (double)abstime.tv_nsec * 0.000000001;
    help += (double)time_ms * 1000.0;
    abstime.tv_sec = (time_t)help;
    abstime.tv_nsec = (time_t)(help - (double)abstime.tv_sec);
    return pthread_cond_timedwait(cond, mutex, &abstime);
}

HANDLE CreateFile(const char *Filename, DWORD dwDesiredAccess, DWORD dwSharedMode, void *lpSecurityAttributes, DWORD dwCreatoionDisposition,
                  DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    UNUSED(dwSharedMode);
    UNUSED(lpSecurityAttributes);
    UNUSED(dwCreatoionDisposition);
    UNUSED(dwFlagsAndAttributes);
    UNUSED(hTemplateFile);
    int oflags;

    oflags = (int)dwDesiredAccess;

    return (HANDLE)open(Filename, oflags);
}

int ReadFile(HANDLE hFile, void *lpBuffer, DWORD nNumberOfBytesToRead, DWORD *lpNumberOfBytesRead, void *lpOverlapped)
{
    UNUSED(lpOverlapped);
    ssize_t ret = read((int)hFile, lpBuffer, (size_t)nNumberOfBytesToRead);
    if (ret < 0) return 0;
    *lpNumberOfBytesRead = (DWORD)ret;
    return 1;
}

int WriteFile(HANDLE hFile, void *lpBuffer, DWORD nNumberOfBytesToWrite, DWORD *lpNumberOfBytesWritten, void *lpOverlapped)
{
    UNUSED(lpOverlapped);
    ssize_t ret = write((int)hFile, lpBuffer, (size_t)nNumberOfBytesToWrite);
    if (ret < 0) return 0;
    *lpNumberOfBytesWritten = (DWORD)ret;
    return 1;
}

DWORD GetFileSize(HANDLE hFile,LPDWORD lpFileSizeHigh)
{
    struct stat statbuf;
    if (fstat((int)hFile, &statbuf) == 0) {
        if (lpFileSizeHigh != NULL) *lpFileSizeHigh = (DWORD)(statbuf.st_size >> 32);
        return statbuf.st_size;
    } else {
        return 0;
    }
}

void CloseHandle(HANDLE hFile)
{
    close((int)hFile);
}

int CopyFile(const char *lpExistingFileName, const char *lpNewFileName, int bFailIfExists)
{
    UNUSED(bFailIfExists);
    char buf[BUFSIZ];
    size_t size;

    int source = open(lpExistingFileName, O_RDONLY, 0);
    int dest = open(lpNewFileName, O_WRONLY | O_CREAT /*| O_TRUNC*/, 0644);
    if ((source > 0) && (dest > 0)) {
        while ((size = read(source, buf, BUFSIZ)) > 0) {
            write(dest, buf, size);
        }
    }

    if (source > 0) close(source);
    if (dest > 0) close(dest);
    return ((source > 0) && (dest > 0)) ? 1 : 0;
}


void GetXilEnvHomeDirectory(char *ret_Directory)
{
    char *HomeDir;
    HomeDir = getenv("HOME");
    if (HomeDir == NULL) {
        HomeDir = getpwuid(getuid())->pw_dir;
    }
    if (HomeDir != NULL) {
        strcpy(ret_Directory, HomeDir);
    }
}

int GetModuleFileName(void *hModule, char *lpFilename, int nSize)
{
    UNUSED(hModule);
    char *Path = (char*)getauxval(AT_EXECFN);
    if (Path == NULL) Path = "unknown";
    else Path = realpath(Path, NULL);
    if (Path == NULL) Path = "unknown";

    if (((int)strlen(Path) + 1) >= nSize) {
        strncpy(lpFilename, Path, nSize);
        lpFilename[nSize - 1] = 0;
    } else strcpy(lpFilename, Path);
    return (int)strlen(lpFilename);
}

int CheckOpenIPCFile(char *Instance, char *Name, char *ret_Path, int DirCraeteOrMustExists, int FileCraeteOrMustExists)
{
    int fh;
    DIR *dir;
    char Path[1024];

    GetXilEnvHomeDirectory(Path);
    strcat(Path, "/.xilenv");
    dir = opendir(Path);
    if (dir == NULL) {
        if (DirCraeteOrMustExists == DIR_CREATE_EXIST) {
            if (mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
                return -1;
            }
        } else {
            return -1;
        }
    } else {
        closedir(dir);
    }
    strcat(Path, "/");
    if ((Instance != NULL) && (strlen(Instance) > 0)) {
        strcat(Path, Instance);
    }
    else {
        strcat(Path, "_");
    }
    dir = opendir(Path);
    if (dir == NULL) {
        if (DirCraeteOrMustExists == DIR_CREATE_EXIST) {
            if (mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
                return -1;
            }
        } else {
            return -1;
        }
    } else {
        closedir(dir);
    }
    strcat(Path, "/");
    strcat(Path, Name);
    switch (FileCraeteOrMustExists) {
    default:
    case FILENAME_MUST_EXIST:
    {
        struct stat buffer;
        if (stat(Path, &buffer) != 0) {
            return -1;
        }
    }
    break;
    case FILENAME_CREATE_EXIST:
        if ((fh = open(Path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) < 0) {
            return -1;
        }
        close(fh);
        break;
    case FILENAME_IGNORE:
        break;
    }
    strcpy(ret_Path, Path);
    return 0;
}

char *GetCommandLine(void)
{
    static char *ret;
    int ParameterCounter = 0;
    int CharCounter = 1;
    int CharPos = 0;
    int BufferSize = 100;
    if (ret == NULL) {
        ret = malloc(BufferSize);
        int fd = open("/proc/self/cmdline", O_RDONLY);
        if (fd > 0) {
            char c;
            while (read(fd, &c, 1) == 1) {
                if (c == 0) {
                    if (ParameterCounter >= 1) {
                        CharCounter++;
                        if (CharCounter > BufferSize) {
                            BufferSize += 100;
                        }
                        ret = realloc(ret, BufferSize);
                        ret[CharPos] = ' ';
                        CharPos++;
                    }
                    ParameterCounter++;
                } else {
                    if (ParameterCounter >= 1) {   // erster Parameter ist der Programmname (ignorieren)
                        CharCounter++;
                        if (CharCounter > BufferSize) {
                            BufferSize += 100;
                        }
                        ret = realloc(ret, BufferSize);
                        ret[CharPos] = c;
                        CharPos++;
                    }
                }
            }
            ret[CharPos] = 0;
        }
    }
    return ret;
}

#endif

int GetRealPathForOnlyUpperCasePath (const char *SrcPath, char *DstPath, int MaxPath)
{
#ifdef _WIN32
    strncpy(DstPath, SrcPath, (size_t)MaxPath);
#else
    if (((SrcPath[0] >= 'A') && (SrcPath[0] <= 'Z')) && (SrcPath[1] >= ':')) {
        FILE *fp;
        char Command[1024];
        char *p;

        sprintf (Command, "winepath -u \"%s\"", SrcPath);
        fp = popen(Command, "r");
        if (fp == NULL) {
            ThrowError(1, "Failed to run command \"%s\"\n", Command);
            return -1;
        }
        DstPath[0] = 0;
        fgets(DstPath, MaxPath, fp);
        p = DstPath;
        // das \n loeschen falls vorhanden
        while ((*p != '\n') & (*p != 0)) p++;
        *p = 0;
        pclose(fp);
    } else {
        strncpy(DstPath, SrcPath, (size_t)MaxPath);
    }
#endif
    return 0;
}

MY_FILE_HANDLE my_open (const char *Name)
{
#ifdef _WIN32
    LogFileAccess (Name);
    return CreateFile (Name,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
#else
    if (((Name[0] >= 'A') && (Name[0] <= 'Z')) && (Name[1] >= ':')) {
        char UnixPath[1024];
        if (GetRealPathForOnlyUpperCasePath(Name, UnixPath, sizeof(UnixPath)) == 0) {
            return open(UnixPath, O_RDONLY);
        } else return MY_INVALID_HANDLE_VALUE;
    } else {
        return open(Name, O_RDONLY);
    }
#endif
}

void my_close(MY_FILE_HANDLE File)
{
#ifdef _WIN32
    CloseHandle(File);
#else
    close(File);
#endif
}

int my_read(MY_FILE_HANDLE File, void* DstBuf, unsigned int Count)
{
#ifdef _WIN32
    DWORD xx, i;
    char *Ptr = (char*)DstBuf;
    for (i = 0; i < Count; i += xx) {
        if (!ReadFile (File, Ptr + i,
            Count - i, &xx, NULL)) {
            return 0;
        }
    }
    return (int)i;
#else
    int ReadBytes;
    char *Ptr = (char*)DstBuf;
    unsigned int Pos = 0;
    do {
        ReadBytes = read(File, Ptr + Pos, Count - Pos);
        if (ReadBytes <= 0){
            return 0;
        } else  {
            Pos += (unsigned int)ReadBytes;
        }
    } while(Pos < Count);
    return (unsigned int)Pos;
#endif
}

void my_lseek(MY_FILE_HANDLE File, uint64_t Offset)
{
#ifdef _WIN32
    LARGE_INTEGER LiOffset;
    BOOL Success;

    GetFileSizeEx(File, &LiOffset);
    if (LiOffset.QuadPart < (int64_t)Offset) {
        ThrowError (1, "cannot set to file position %" PRIu64 "", Offset);
    }
    LiOffset.QuadPart = (int64_t)Offset;
    Success = SetFilePointerEx (File, LiOffset, NULL, FILE_BEGIN);
    if (!Success) {
        ThrowError (1, "cannot set to file position %" PRIu64 "", Offset);
    }
#else
    lseek(File, Offset, SEEK_SET);
#endif
}

uint64_t my_ftell(MY_FILE_HANDLE File)
{
#ifdef _WIN32
    LARGE_INTEGER Help, SaveFilePos;
    BOOL Success;

    Help.QuadPart = 0;
    Success = SetFilePointerEx (File, Help, &SaveFilePos, FILE_CURRENT);
    if (!Success) {
        ThrowError (1, "cannot get file position");
        return 0;
    } else return (uint64_t)SaveFilePos.QuadPart;
#else
    return (uint64_t)lseek(File, 0, SEEK_CUR);
#endif
}

uint64_t my_get_file_size(MY_FILE_HANDLE File)
{
#ifdef _WIN32
    LARGE_INTEGER Help;
    if (GetFileSizeEx(File, &Help)) {
        return Help.QuadPart;
    } else {
        return 0;
    }
#else
    struct stat statbuf;
    if (fstat(File, &statbuf) == 0) {
        return statbuf.st_size;
    } else {
        return 0;
    }
#endif
}
