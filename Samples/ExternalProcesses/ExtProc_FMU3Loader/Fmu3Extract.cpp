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
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include "XilEnvExtProc.h"
}
#include "Fmu3Struct.h"
#include "Fmu3Extract.h"

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
            ThrowError(1, "cannot call powershell: %s", SystemCommandLine);
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

int DeleteDirectory (const char *par_Directory)
{
    char Pattern[MAX_PATH];
    WIN32_FIND_DATAA FileInfos;
    HANDLE hFile;
    DWORD Attributes;
    int Ret;

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
}

int ExtractFmuToUniqueTempDirectory(const char *par_FmuFileName, char *ret_ExtractedToPath)
{
    char UniqueTempDirectory[MAX_PATH];
    char FileName[MAX_PATH];
    char ZipFilePath[MAX_PATH];
    const char *p, *s, *e;

    p = par_FmuFileName + strlen(par_FmuFileName);
    while ((p > par_FmuFileName) && (*p != '.') && (*p != '\\') && (*p != '/')) {
        p--;
    }
    if ((*p == '.') && !stricmp(p, ".FMU")) {
        e = p;
        while ((p > par_FmuFileName) && (*p != '\\') && (*p != '/')) {
            p--;
        }
        if ((*p == '\\') || (*p == '/')) p++;
        s = p;
        memcpy(FileName, s, e - s);
        FileName[e - s] = 0;

        GetTempPathA(MAX_PATH, UniqueTempDirectory);
        sprintf (UniqueTempDirectory + strlen(UniqueTempDirectory), "%s_%u", FileName, GetProcessId(GetCurrentProcess()));
        
        // If there exist one delete it
        DeleteDirectory(UniqueTempDirectory);
        
        if (!CreateDirectoryA(UniqueTempDirectory, NULL)) {
            ThrowError (1, "cannot create directory %s", UniqueTempDirectory);
            return -1;
        }
        // copy FMU -> ZIP otherwise it cannot extracted
        sprintf (ZipFilePath, "%s\\%s.zip", UniqueTempDirectory, FileName);
        if (!CopyFileA(par_FmuFileName, ZipFilePath, TRUE)) {
            ThrowError (1, "cannot copy %s to %s", par_FmuFileName, ZipFilePath);
            return -1;
        }
        strcpy (ret_ExtractedToPath, UniqueTempDirectory);
        return UnzipFile(ZipFilePath, UniqueTempDirectory);
    }
    ThrowError (1, "%s in not a .FMU file", par_FmuFileName);
    return -1;
}
#endif
