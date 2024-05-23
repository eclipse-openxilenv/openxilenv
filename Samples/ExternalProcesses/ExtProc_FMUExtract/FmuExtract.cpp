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


#ifdef _WIN32
#include <Windows.h>
#include <comutil.h>
#include <Shldisp.h>
#else
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#define MAX_PATH 1024
#define strcmpi(a,b) strcasecmp((a),(b))
#endif


#include "FmuExtract.h"

#if _WIN32
int  UnzipFile(const char *par_ZipFileName, const char *par_ExtractToPath)
{
    int Ret = -1;

    char *SystemCommandLine = (char*)malloc(strlen(par_ZipFileName) + strlen(par_ExtractToPath) + 128);
    if (SystemCommandLine != nullptr) {
        strcpy (SystemCommandLine, "powershell.exe expand-archive -LiteralPath \"");
        strcat (SystemCommandLine, par_ZipFileName);
        strcat (SystemCommandLine, "\" -DestinationPath \"");
        strcat (SystemCommandLine, par_ExtractToPath);
        strcat (SystemCommandLine, "\"");

        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcessA(NULL,
                            SystemCommandLine,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &si,
                            &pi)) {
           printf("cannot call powershell: %s", SystemCommandLine);
        } else {
            DWORD ExitCode;
            WaitForSingleObject(pi.hProcess, INFINITE);
            if (GetExitCodeProcess(pi.hProcess, &ExitCode)) {
                Ret = ExitCode;
            }
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        free (SystemCommandLine);
    }
    return Ret;
}
#endif

int  UnzipFileWithUserTool(const char *par_Cmd, const char *par_ZipFileName, const char *par_ExtractToPath)
{
    const char *SrcFileKeyword = "TO_REPLACE_WITH_SOURCE_FMU";
    const char *DstPathKeyword = "TO_REPLACE_WITH_DESTINATION_PATH";
    int Ret = -1;
    int LenCmd, LenZipFileName, LenExtractToPath;
    LenCmd = (int)strlen(par_Cmd);
    LenZipFileName = (int)strlen(par_ZipFileName);
    LenExtractToPath = (int)strlen(par_ExtractToPath);
    char *SystemCommandLine = (char*)malloc(LenCmd + LenZipFileName + LenExtractToPath + 128);
    if (SystemCommandLine != nullptr) {
        const char *SrcFile = strstr(par_Cmd, SrcFileKeyword);
        const char *DstPath = strstr(par_Cmd, DstPathKeyword);
        if ((SrcFile == NULL) || (DstPath == NULL)) {
            printf("\"%s\" or \"%s\" is missing inside the FMU_EXTRACTOR_CMD environment varaible:\n \"%s\"\n", SrcFileKeyword, DstPathKeyword, par_Cmd);
            return -1;
        }
        int PosSrcFile =(int)(SrcFile - par_Cmd);
        int PosDstPath = (int)(DstPath - par_Cmd);
        int LenSrcFileKeyword = strlen(SrcFileKeyword);
        int LenDstPathKeyword = strlen(DstPathKeyword);
        if (SrcFile < DstPath) {
            int Pos;
            int Size;
            memcpy (SystemCommandLine, par_Cmd, PosSrcFile);
            SystemCommandLine[PosSrcFile] = '\"';
            Pos = PosSrcFile + 1;
            memcpy (SystemCommandLine + Pos, par_ZipFileName, LenZipFileName);    // copy zip file
            Pos += LenZipFileName;
            SystemCommandLine[Pos] = '\"';
            Pos++;
            Size = (int)(DstPath - SrcFile) - LenSrcFileKeyword;
            memcpy (SystemCommandLine + Pos, par_Cmd + PosSrcFile + LenSrcFileKeyword, Size);
            Pos += Size;
            SystemCommandLine[Pos] = '\"';
            Pos++;
            memcpy (SystemCommandLine + Pos, par_ExtractToPath, LenExtractToPath);  // copy extract path
            Pos += LenExtractToPath;
            SystemCommandLine[Pos] = '\"';
            Pos++;
            Size = LenCmd  - (DstPath - par_Cmd) - LenDstPathKeyword;
            memcpy (SystemCommandLine + Pos, par_Cmd + PosDstPath + LenDstPathKeyword, Size);
            Pos += Size;
            SystemCommandLine[Pos] = 0;
        } else {
            int Pos;
            int Size;
            memcpy (SystemCommandLine, par_Cmd, PosDstPath);
            SystemCommandLine[PosDstPath] = '\"';
            Pos = PosDstPath + 1;
            memcpy (SystemCommandLine + Pos, par_ExtractToPath, LenExtractToPath);  // copy extract path
            Pos += LenExtractToPath;
            SystemCommandLine[Pos] = '\"';
            Pos++;
            Size = (SrcFile - DstPath) - LenDstPathKeyword;
            memcpy (SystemCommandLine + Pos, par_Cmd + PosDstPath + LenDstPathKeyword, Size);
            Pos += Size;
            SystemCommandLine[Pos] = '\"';
            Pos++;
            memcpy (SystemCommandLine + Pos, par_ZipFileName, LenZipFileName);   // copy zip file
            Pos += LenZipFileName;
            SystemCommandLine[Pos] = '\"';
            Pos++;
            Size = LenCmd  - (SrcFile - par_Cmd) - LenSrcFileKeyword;
            memcpy (SystemCommandLine + Pos, par_Cmd + PosSrcFile + LenSrcFileKeyword, Size);
            Pos += Size;
            SystemCommandLine[Pos] = 0;
        }
#ifdef _WIN32
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcessA(NULL,
                            SystemCommandLine,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &si,
                            &pi)) {
            printf("cannot call powershell: %s\n", SystemCommandLine);
        } else {
            DWORD ExitCode;
            WaitForSingleObject(pi.hProcess, INFINITE);
            if (GetExitCodeProcess(pi.hProcess, &ExitCode)) {
                Ret = ExitCode;
            }
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
#else
        Ret = system(SystemCommandLine);
#endif
        free (SystemCommandLine);
    }
    return Ret;
}

int DeleteDirectory (const char *par_Directory)
{
    int Ret;
#ifdef _WIN32
    char Pattern[MAX_PATH];
    WIN32_FIND_DATAA FileInfos;
    HANDLE hFile;
    DWORD Attributes;


    if ((Attributes = GetFileAttributesA(par_Directory)) == INVALID_FILE_ATTRIBUTES) {
        return GetLastError();
    }
    if ((Attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
        strcpy (Pattern, par_Directory);
        strcat (Pattern, "\\*.*");

        if ((hFile = FindFirstFileA(Pattern, &FileInfos)) != INVALID_HANDLE_VALUE) {
            do {
                char Path[MAX_PATH];
                strcpy (Path, par_Directory);
                strcat (Path, "\\");
                strcat (Path, FileInfos.cFileName);
                if ((FileInfos.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
                    if (FileInfos.cFileName[0] != '.') {
                        Ret = DeleteDirectory(Path);
                        if (Ret != 0) {
                            FindClose(hFile);
                            return  Ret;
                        }
                    }
                } else {
                    // may be the files are in use so try and wait for about 2s
                    int x;
                    for (x = 0; x < 20; x++) {
                        if ((FileInfos.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) != FILE_ATTRIBUTE_NORMAL) {
                            SetFileAttributesA(Path, FILE_ATTRIBUTE_NORMAL);
                        }
                        if (DeleteFileA(Path)) {
                            // succesfull deleted
                            break;
                        }
                        Sleep(100);
                    }
                    if (x == 10) {
                        // file could not delete
                        FindClose(hFile);
                        return GetLastError();
                    }
                }
            } while (FindNextFileA(hFile, &FileInfos) == TRUE);
            FindClose(hFile);
        }
        if ((Attributes & FILE_ATTRIBUTE_NORMAL) != FILE_ATTRIBUTE_NORMAL) {
            SetFileAttributesA(par_Directory, FILE_ATTRIBUTE_NORMAL);
        }
        if (!RemoveDirectoryA(par_Directory)) {
            return GetLastError();
        }
        return 0;   // OK
    }
    return ERROR_CURRENT_DIRECTORY;
#else
    char SystemCommandLine[1024];
    sprintf(SystemCommandLine, "rm -r %s", par_Directory);
    Ret = system(SystemCommandLine);
    return Ret;
#endif
}

int ExtractFmuToUniqueTempDirectory(const char *par_FmuFileName, char *ret_ExtractedToPath)
{
    char UniqueTempDirectory[MAX_PATH];
    char FileName[MAX_PATH];
    char ZipFilePath[MAX_PATH];
    char *FMUExtractorCmd;
    const char *p, *s, *e;
    int Ret = -1;

    p = par_FmuFileName + strlen(par_FmuFileName);
    while ((p > par_FmuFileName) && (*p != '.') && (*p != '\\') && (*p != '/')) {
        p--;
    }
    if ((*p == '.') && !strcmpi(p, ".FMU")) {
        e = p;
        while ((p > par_FmuFileName) && (*p != '\\') && (*p != '/')) {
            p--;
        }
        if ((*p == '\\') || (*p == '/')) p++;
        s = p;
        memcpy(FileName, s, e - s);
        FileName[e - s] = 0;
#ifdef _WIN32
        GetTempPathA(MAX_PATH, UniqueTempDirectory);
        sprintf(UniqueTempDirectory + strlen(UniqueTempDirectory), "%s_%u", FileName, GetProcessId(GetCurrentProcess()));
#else
        char const *Folder = getenv("TMPDIR");
        if (Folder == NULL) {
            Folder = getenv("TMP");
            if (Folder == NULL) {
                Folder = getenv("TEMP");
                if (Folder == NULL) {
                    Folder = getenv("TEMPDIR");
                    if (Folder == NULL) {
                        Folder = "/tmp";
                    }
                }
            }
        }
        sprintf(UniqueTempDirectory, "%s/%s_%u", Folder, FileName, getpid());
#endif

        DeleteDirectory(UniqueTempDirectory);

#if _WIN32
        if (!CreateDirectoryA(UniqueTempDirectory, NULL)) {
            printf("cannot create directory %s\n", UniqueTempDirectory);
            return -1;
        }
        // Copy FMU -> ZIP otherwise it cannot unpack
        sprintf (ZipFilePath, "%s\\%s.zip", UniqueTempDirectory, FileName);
        if (!CopyFileA(par_FmuFileName, ZipFilePath, TRUE)) {
            printf("cannot copy %s to %s\n", par_FmuFileName, ZipFilePath);
            return -1;
        }
#else
        if (mkdir(UniqueTempDirectory, 0777)) {
            printf("cannot create directory %s\n", UniqueTempDirectory);
        }
        // Copy FMU -> ZIP otherwise it cannot unpack
        sprintf(ZipFilePath, "%s/%s.zip", UniqueTempDirectory, FileName);

        char SystemCommandLine[1024];
        sprintf(SystemCommandLine, "cp %s %s", par_FmuFileName, ZipFilePath);
        if (system(SystemCommandLine)) {
            printf("cannot copy %s to %s\n", par_FmuFileName, ZipFilePath);
            return -1;
        }
#endif

        strcpy (ret_ExtractedToPath, UniqueTempDirectory);

        FMUExtractorCmd = (char*)malloc(32*1024);
        if (FMUExtractorCmd == NULL) {
            printf("out of memory\n");
            return -1;
        }
#ifdef _WIN32
        if ((FMUExtractorCmd != NULL) &&
            (GetEnvironmentVariable ("FMU_EXTRACTOR_CMD",
                                    FMUExtractorCmd, 32*1024) > 0)) {
            Ret = UnzipFileWithUserTool(FMUExtractorCmd, ZipFilePath, UniqueTempDirectory);
        } else {
            Ret = UnzipFile(ZipFilePath, UniqueTempDirectory);
        }
#else
        const char *Env = getenv("FMU_EXTRACTOR_CMD");
        if (Env == NULL) {
            strcpy(FMUExtractorCmd, "cd TO_REPLACE_WITH_DESTINATION_PATH && unzip TO_REPLACE_WITH_SOURCE_FMU");
        } else {
            strcpy(FMUExtractorCmd, Env);
        }
        Ret = UnzipFileWithUserTool(FMUExtractorCmd, ZipFilePath, UniqueTempDirectory);
#endif
        if (FMUExtractorCmd != NULL) free(FMUExtractorCmd);

#ifdef _WIN32
        DeleteFile(ZipFilePath);
#else
        remove(ZipFilePath);
#endif
    } else {
        printf("%s in not a .FMU file\n", par_FmuFileName);
    }
    return Ret;
}


int GetFmuPath(int argc, char* argv[], char *ret_Fmu, int par_MaxLen)
{
    int x;

    for (x = 1; x < argc; x++) {
        if (!strcmpi("-fmu", argv[x])) {
            if (x < argc) {
                if (strlen(argv[x + 1]) >= par_MaxLen) {
                    ret_Fmu[0] = 0;
                    return -1;
                } else {
                    strcpy(ret_Fmu, argv[x + 1]);
                    return 0;
                }
            }
        }
    }
    return -1;
}

#ifndef _WIN32
int ScanDirFilter (const struct dirent *entry)
{
    size_t Len = strlen(entry->d_name);
    if (Len < 4) return 0;
    const char *p = entry->d_name + Len - 3;
    return (strcmp(p, ".so") == 0);
}
#endif

int CheckIfOneDllExists(const char *par_Directory)
{
#if _WIN32
    char Pattern[MAX_PATH];
    WIN32_FIND_DATAA FileInfos;
    HANDLE hFile;
    int Ret = 0;

    if (GetFileAttributesA(par_Directory) == FILE_ATTRIBUTE_DIRECTORY) {
        strcpy (Pattern, par_Directory);
        strcat (Pattern, "\\*.DLL");

        if ((hFile = FindFirstFileA(Pattern, &FileInfos)) != INVALID_HANDLE_VALUE) {
            FindClose(hFile);
            Ret = 1;
        }
    }
    return Ret;
#else
    int x = 0;
    struct dirent *Dir;
    struct dirent **Filtered = NULL;
    int n, e;

    n = scandir(par_Directory, &Filtered, ScanDirFilter, NULL);
    if (n > 0) {
        for (e = 0; e < n; e++) {
            Dir = Filtered[e];
            printf("%s\n", Dir->d_name);
        }
    }
__OUT:
    if (Filtered != NULL) {
        for (e = 0; e < n; e++) {
            free(Filtered[e]);
        }
        free(Filtered);
    }
    return (n >= 1);
#endif
}

int CopyBackDllIfChanged(const char *par_TempDirectory, const char *par_BinaryPathInsideExtractedFmuDirectrory)
{
#ifdef _WIN32
    char Pattern[MAX_PATH];
    WIN32_FIND_DATAA FileInfos;
    HANDLE hFile;
    int Ret = 0;

    if (GetFileAttributesA(par_TempDirectory) == FILE_ATTRIBUTE_DIRECTORY) {
        strcpy (Pattern, par_TempDirectory);
        strcat (Pattern, "\\*.DLL");

        if ((hFile = FindFirstFileA(Pattern, &FileInfos)) != INVALID_HANDLE_VALUE) {
            char Dst[MAX_PATH];
            char Src[MAX_PATH];
            strcpy (Dst, par_BinaryPathInsideExtractedFmuDirectrory);
            strcat (Dst, "\\");
            strcat (Dst, FileInfos.cFileName);
            strcpy (Src, par_TempDirectory);
            strcat (Src, "\\");
            strcat (Src, FileInfos.cFileName);

            FindClose(hFile);

            // try to copy back the DLL 100 * 100ms
            int x;
            for (x = 0; x < 100; x++) {
                if (CopyFile (Src, Dst, FALSE)) {
                    break;
                }
                Sleep(100);
            }
            if (x != 100) {
                Ret = 1;
            }else {
                printf("cannot copy \"%s\" back to \"%s\"\n", Src, Dst);
            }

        }
        DeleteDirectory(par_TempDirectory);
    }
    return Ret;
#else
    // this is not supported with Linux
    return 0;
#endif
}

int main(int argc, char* argv[])
{
    char FmuFileName[MAX_PATH];
    char ExtractedFmuDirectrory[MAX_PATH];
    char ProcessName[MAX_PATH];
    char ExecName[MAX_PATH];
    char BinaryPathInsideExtractedFmuDirectrory[MAX_PATH];
    char XcpExeWriteBackPath[MAX_PATH];
    int CommandLineLen;
    char *SystemCommandLine;
    char *p;
    int x;

    int Ret = 1;

    CommandLineLen = 0;
    for (x = 1; x < argc; x++) {
        CommandLineLen += strlen(argv[x]) + 4;  // ""
    }
    if (GetFmuPath(argc, argv, FmuFileName, sizeof(FmuFileName)) == 0) {
#ifdef _WIN32
        GetFullPathNameA(FmuFileName, sizeof(ProcessName), ProcessName, NULL);
#else
        realpath(FmuFileName, ProcessName);
#endif
        ExtractFmuToUniqueTempDirectory(FmuFileName, ExtractedFmuDirectrory);

        strcpy(XcpExeWriteBackPath, ExtractedFmuDirectrory);
#ifdef _WIN32
        strcat(XcpExeWriteBackPath, "\\XcpTemp");
        if (!CreateDirectory(XcpExeWriteBackPath, nullptr)) {
            printf("cannot create directory \"%s\"\n", XcpExeWriteBackPath);
        }
#else
        strcat(XcpExeWriteBackPath, "/XcpTemp");
        if (mkdir(XcpExeWriteBackPath, 0777)) {
            printf("cannot create directory \"%s\"\n", XcpExeWriteBackPath);
        }
#endif

        SystemCommandLine = (char*)malloc(3*MAX_PATH + CommandLineLen + 128);
#ifdef _WIN32
        GetModuleFileNameA(NULL, ExecName, MAX_PATH);
#else
        readlink("/proc/self/exe", ExecName, sizeof(ExecName));
#endif
        p = ExecName;
        while (*p != 0) p++;
        while ((p > ExecName) && (*p != '\\') && (*p != '/')) p--;
        *p = 0;

#ifdef BUILD_WITH_FMU2_SUPPORT
        strcpy(BinaryPathInsideExtractedFmuDirectrory, ExtractedFmuDirectrory);
#ifdef _WIN32
        strcat(BinaryPathInsideExtractedFmuDirectrory, "\\binaries\\win64");
#else
        strcat(BinaryPathInsideExtractedFmuDirectrory, "/binaries/linux64");
#endif
        if (CheckIfOneDllExists(BinaryPathInsideExtractedFmuDirectrory)) {
            // ExtProc_FMULoader64.EXE -fmuextracted PathToExtractedFMUDirectory -fmu PathToFMUFile ...
#ifdef _WIN32
            strcat (ExecName, "\\ExtProc_FMU2Loader64.EXE");
#else
            strcat(ExecName, "/ExtProc_FMU2Loader32.EXE");
#endif
        } else {
            strcpy(BinaryPathInsideExtractedFmuDirectrory, ExtractedFmuDirectrory);
#ifdef _WIN32
            strcat(BinaryPathInsideExtractedFmuDirectrory, "\\binaries\\win32");
#else
            strcat(BinaryPathInsideExtractedFmuDirectrory, "/binaries/linux32");
#endif
            if (CheckIfOneDllExists(BinaryPathInsideExtractedFmuDirectrory)) {
                // ExtProc_FMULoader32.EXE -fmuextracted PathToExtractedFMUDirectory -fmu PathToFMUFile ...
#ifdef _WIN32
                strcat (ExecName, "\\ExtProc_FMU2Loader32.EXE");
#else
                strcat(ExecName, "/ExtProc_FMU2Loader64.EXE");
#endif                
            } else {
#else
        {
#endif
#ifdef BUILD_WITH_FMU3_SUPPORT
                // Now we check if it is a FMU3
                strcpy(BinaryPathInsideExtractedFmuDirectrory, ExtractedFmuDirectrory);
#ifdef _WIN32
                strcat(BinaryPathInsideExtractedFmuDirectrory, "\\binaries\\x86_64-windows");
#else
                strcat(BinaryPathInsideExtractedFmuDirectrory, "/binaries/x86_64-linux");
#endif
                if (CheckIfOneDllExists(BinaryPathInsideExtractedFmuDirectrory)) {
                    // ExtProc_FMU3Loader64.EXE -fmuextracted PathToExtractedFMUDirectory -fmu PathToFMUFile ...
#ifdef _WIN32
                    strcat (ExecName, "\\ExtProc_FMU3Loader64.EXE");
#else
                    strcat(ExecName, "/ExtProc_FMU3Loader64.EXE");
#endif
                } else {
                    strcpy(BinaryPathInsideExtractedFmuDirectrory, ExtractedFmuDirectrory);
#ifdef _WIN32
                    strcat(BinaryPathInsideExtractedFmuDirectrory, "\\binaries\\x86-windows");
#else
                    strcat(BinaryPathInsideExtractedFmuDirectrory, "/binaries/x86-linux");
#endif
                    if (CheckIfOneDllExists(BinaryPathInsideExtractedFmuDirectrory)) {
                        // ExtProc_FMU3Loader64.EXE -fmuextracted PathToExtractedFMUDirectory -fmu PathToFMUFile ...
#ifdef _WIN32
                        strcat (ExecName, "\\ExtProc_FMU3Loader32.EXE");
#else
                        strcat(ExecName, "/ExtProc_FMU3Loader32.EXE");
#endif
                    } else {
#ifdef _WIN32
                        printf("there are no x86-windows/win32 or x86_64-windows/win64 binary .DLL inside \"%s\"\n", FmuFileName);
#else
                        printf("there are no x86-linux/linux32 or x86_64-linux/linux64 binary .so inside \"%s\"\n", FmuFileName);
#endif
                        return 1;
                    }
                }
#endif
            }
        }
        strcpy (SystemCommandLine, "-fmuextracted \"");
        strcat (SystemCommandLine, ExtractedFmuDirectrory);
        strcat (SystemCommandLine, "\" ");

        strcat (SystemCommandLine, " -WriteBackExeToDir \"");
        strcat (SystemCommandLine, XcpExeWriteBackPath);
        strcat (SystemCommandLine, "\" ");

        // Alle Aufruf-Parameter uebertragen
        for (x = 1; x < argc; x++) {
            int WithWhiteSpaces;
            if (strchr(argv[x], ' ') != NULL) WithWhiteSpaces = 1;
            else WithWhiteSpaces = 0;
            if (WithWhiteSpaces) strcat (SystemCommandLine, " \"");
            else strcat (SystemCommandLine, " ");
            strcat (SystemCommandLine, argv[x]);
            if (WithWhiteSpaces) strcat (SystemCommandLine, "\"");
        }
        //printf ("\n%s\n", ExecName);
        //printf ("\n%s\n", SystemCommandLine);
#ifdef _WIN32
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcessA(ExecName,
                            SystemCommandLine,
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &si,
                            &pi)) {
            printf("cannot create process %s %s\n", ExecName, SystemCommandLine);
        } else {
            DWORD ExitCode;
            WaitForSingleObject(pi.hProcess, INFINITE);
            if (GetExitCodeProcess(pi.hProcess, &ExitCode)) {
                Ret = ExitCode;
            }
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            // check if a DLL or EXE is inside the XcpTemp folder
            if (CopyBackDllIfChanged(XcpExeWriteBackPath, BinaryPathInsideExtractedFmuDirectrory)) {
                char NewZipFile[MAX_PATH];
                strcpy(NewZipFile, ExtractedFmuDirectrory);
                strcat(NewZipFile, ".zip");

                strcpy(SystemCommandLine, "powershell.exe Compress-Archive -Path \"");
                strcat(SystemCommandLine, ExtractedFmuDirectrory);
                strcat(SystemCommandLine, "\\*\" -DestinationPath \"");
                strcat(SystemCommandLine, NewZipFile);
                strcat(SystemCommandLine, "\"");

                if (!CreateProcessA(NULL, //"powershell.exe",
                                    SystemCommandLine,
                                    NULL,
                                    NULL,
                                    FALSE,
                                    0,
                                    NULL,
                                    NULL,
                                    &si,
                                    &pi)) {
                    printf("cannot call powershell %s\n", SystemCommandLine);
                } else {
                    DWORD ExitCode;
                    WaitForSingleObject(pi.hProcess, INFINITE);
                    if (GetExitCodeProcess(pi.hProcess, &ExitCode)) {
                        Ret = ExitCode;
                    }
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                    if (Ret == 0) {
                        // successfully zipped now copy back.
                        if (!CopyFile (NewZipFile, FmuFileName, FALSE)) {
                            printf("cannot copy \"%s\" back to \"%s\"\n", NewZipFile, FmuFileName);
                            Ret = 1;
                        }

                    }
                }
            }
        }
#else
        char *ExecAndCommandLine = (char*)malloc(strlen(ExecName) + 1 + strlen(SystemCommandLine) + 1);
        sprintf(ExecAndCommandLine, "%s %s", ExecName, SystemCommandLine);
        printf("system(%s)\n", ExecAndCommandLine);
        if (system(ExecAndCommandLine) == 0) {
            // check if a DLL or EXE is inside the XcpTemp folder
            if (CopyBackDllIfChanged(XcpExeWriteBackPath, BinaryPathInsideExtractedFmuDirectrory)) {
                char NewZipFile[MAX_PATH];
                strcpy(NewZipFile, ExtractedFmuDirectrory);
                strcat(NewZipFile, ".zip");

                strcpy(SystemCommandLine, "gzip \"");
                strcat(SystemCommandLine, ExtractedFmuDirectrory);
                strcat(SystemCommandLine, " \"");
                strcat(SystemCommandLine, NewZipFile);
                strcat(SystemCommandLine, "\"");

                if (system(SystemCommandLine)) {
                    printf("cannot compress fmu\n");
                    return -1;
                }
                char Line[1024];
                sprintf(Line, "cp %s %s", NewZipFile, FmuFileName);
                if (system(Line)) {
                    printf("cannot copy back fmu\n");
                    return -1;
                }
            }
        }
#endif
        DeleteDirectory(ExtractedFmuDirectrory);
    }
    return Ret;
}
