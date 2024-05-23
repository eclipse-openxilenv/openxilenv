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


#include "Platform.h"
#ifdef _WIN32
#include <ShlObj.h>
#else
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "Config.h"
#include "StringMaxChar.h"
#include "ConfigurablePrefix.h"
#include "ThrowError.h"
#include "MainValues.h"
#include "FileExtensions.h"
#include "IniDataBase.h"
#include "ParseCommandLine.h"
#include "IniFileDontExist.h"

#ifndef _WIN32
int CheckIfDirectoryExists(const char *par_Path)
{
    if (access(par_Path, 0) == 0) {
        struct stat Status;
        stat(par_Path, &Status);
        if (Status.st_mode & S_IFDIR) {
            return 1;
        } else {
            ThrowError(1, "it exist a file not a folder with the ame \"%s\"", par_Path);
            return 0;
        }
    } else {
        if (mkdir(par_Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            ThrowError(1, "folder \"%s\" doesn't exist and it cannot create", par_Path);
            return 0;
        } else {
            return 1;
        }
    }
}
#endif

void AddIniFileToHistory (char *par_IniFile)
{
    char Directory[MAX_PATH];
    char Entry[64];
    char Last[MAX_PATH];
    char Help[MAX_PATH];
    int x, xx, NumOfEntrys;
    const char *ProgramName = GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME);

    if (!OpenWithNoGui() && (par_IniFile != NULL) && strlen (par_IniFile)) {
        int Fd;
#ifdef _WIN32
        HRESULT Result;
        Result = SHGetFolderPath (NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, Directory);
        if (Result == S_OK) {
            int Fd;
            StringAppendMaxCharTruncate(Directory, "\\", sizeof(Directory));
            StringAppendMaxCharTruncate(Directory, ProgramName, sizeof(Directory));
            // If folder don't exists create it
            CreateDirectory(Directory, NULL);
            StringAppendMaxCharTruncate(Directory, "\\History.ini", sizeof(Directory));
#else
            GetXilEnvHomeDirectory(Directory);
            StringAppendMaxCharTruncate(Directory, "/.", sizeof(Directory));
            StringAppendMaxCharTruncate(Directory, ProgramName, sizeof(Directory));
            CheckIfDirectoryExists(Directory);
            StringAppendMaxCharTruncate(Directory, "/LastLoadedFiles.ini", sizeof(Directory));
#endif
            Fd = IniFileDataBaseOpen(Directory);
            if (Fd <= 0) {
                // If file not exist create it.
                Fd = IniFileDataBaseCreateAndOpenNewIniFile(Directory);
            }
            if (Fd > 0) {
                xx = 9999;
                if (IniFileDataBaseReadString (ProgramName, "last_used_ini_file", "", Last, sizeof (Last), Fd) > 0) {
                    if (stricmp (par_IniFile, Last)) {   // If it was not the same as last call
                        // first only count
                        for (NumOfEntrys = 0; NumOfEntrys < 20; NumOfEntrys++) {
                            sprintf (Entry, "X_%i", NumOfEntrys);
                            if (IniFileDataBaseReadString (ProgramName, Entry, "", Help, sizeof (Help), Fd) <= 0) {
                                break;
                            }
                            if (!strcmpi (Help, par_IniFile)) {
                                xx = NumOfEntrys;
                            }
                        }
                        for (x = NumOfEntrys; x >= 0; x--) {
                            if (x < xx) {
                                sprintf (Entry, "X_%i", x);
                                IniFileDataBaseReadString (ProgramName, Entry, "", Help, sizeof (Help), Fd);
                                sprintf (Entry, "X_%i", x + 1);
                                if (strlen (Help)) IniFileDataBaseWriteString (ProgramName, Entry, Help, Fd);
                                else IniFileDataBaseWriteString (ProgramName, Entry, NULL, Fd);
                            }
                        }
                        IniFileDataBaseWriteString (ProgramName, "X_0", Last, Fd);
                        IniFileDataBaseWriteString (ProgramName, "last_used_ini_file", par_IniFile, Fd);
                    }
                } else {  // It is not inside
                    IniFileDataBaseWriteString (ProgramName, "last_used_ini_file", par_IniFile, Fd);
                }
                IniFileDataBaseSave(Fd, Directory, 2);
            }
#ifdef _WIN32
        }
#endif
    }
}
