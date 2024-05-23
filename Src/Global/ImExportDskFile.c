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
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "Config.h"
#include "MainValues.h"
#include "Files.h"
#include "Scheduler.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "IniDataBase.h"
#include "WindowIniHelper.h"

#include "MainWindow.h"
#include "ImExportDskFile.h"

void TranslateEnumWindowName (char *wname)
{
    char *s, *d;
    s = d = wname;
    while (*s != 0) {
        if (*s != '#') *d++ = *s;
        s++;
    }
    *d = 0;
}

static void delete_all_window_of_one_type(char *type)
{
    char entry[16];
    char wname[INI_MAX_SECTION_LENGTH];
    int x;
    int Fd = GetMainFileDescriptor();

    for (x = 0;;x++) {
        sprintf (entry, "W%i", x);
        if (IniFileDataBaseReadString(type, entry, "", wname, sizeof (wname), Fd) == 0)
            break;
        else IniFileDataBaseWriteString (wname, NULL, NULL, Fd);
    }
    IniFileDataBaseWriteString (type, NULL, NULL, Fd);
}

// Close all windows and delete all window information from the INI file
int clear_desktop_ini (void)
{
    char wname[INI_MAX_SECTION_LENGTH];
    char section[INI_MAX_SECTION_LENGTH];
    int x;
    int Fd = GetMainFileDescriptor();

    delete_all_window_of_one_type("GUI/AllTextWindows");
    delete_all_window_of_one_type("GUI/AllCalibrationMapViewWindows");
    delete_all_window_of_one_type("GUI/AllEnumWindows");
    delete_all_window_of_one_type("GUI/AllOscilloscopeWindows");
    delete_all_window_of_one_type("GUI/AllTachometerWindows");
    delete_all_window_of_one_type("GUI/AllKnobWindows");
    delete_all_window_of_one_type("GUI/AllSliderWindows");
    delete_all_window_of_one_type("GUI/AllBargraphWindows");
    delete_all_window_of_one_type("GUI/AllCalibrationTreeWindows");
    delete_all_window_of_one_type("GUI/AllControlLampsViewWindows");
    delete_all_window_of_one_type("GUI/AllUserDrawWindows");
    delete_all_window_of_one_type("GUI/AllUserControlWindows");
    delete_all_window_of_one_type("GUI/AllA2lMapViewWindows");
    delete_all_window_of_one_type("GUI/AllA2lSingleViewWindows");
    delete_all_window_of_one_type("GUI/AllCanMessageWindows");
    delete_all_window_of_one_type("GUI/AllFlexrayMessageWindows");

    for (x = 0; x < 10000; x++) {
        sprintf (section, "GUI/OpenWindowsForSheet%i", x);
        if (IniFileDataBaseReadString (section, "SheetName", "",
                                       wname, sizeof(wname), Fd) == 0) break;
        IniFileDataBaseWriteString (section, NULL, NULL, Fd);
    }
    IniFileDataBaseWriteString ("BasicSettings", "Selected Sheet", NULL, Fd);

    return 0;
}

static int copy_all_window_of_one_type(char *par_Type, int par_DstFile, int par_SrcFile)
{
    char entry[16];
    char SectionPath[INI_MAX_SECTION_LENGTH];
    char *Name;
    int Len;
    int x;

    strcpy(SectionPath, "GUI/Widgets/");
    Len = strlen(SectionPath);
    Name = SectionPath + Len;

    for (x = 0;;x++) {
        sprintf (entry, "W%i", x);
        if (IniFileDataBaseReadString(par_Type, entry, "", Name, sizeof(SectionPath) - Len, par_SrcFile) == 0)
            break;
        if (IniFileDataBaseCopySectionSameName(par_DstFile, par_SrcFile, SectionPath)) return -1;
    }
    if (IniFileDataBaseCopySectionSameName(par_DstFile, par_SrcFile, par_Type)) return -1;
    return 0;
}

int copy_all_window_of_all_type(int par_DstFile, int par_SrcFile)
{
    if (copy_all_window_of_one_type("GUI/AllTextWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllCalibrationMapViewWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllEnumWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllOscilloscopeWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllTachometerWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllKnobWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllSliderWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllBargraphWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllCalibrationTreeWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllControlLampsViewWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllCanMessageWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllFlexrayMessageWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllUserDrawWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllUserControlWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllA2lMapViewWindows", par_DstFile, par_SrcFile)) return -1;
    if (copy_all_window_of_one_type("GUI/AllA2lSingleViewWindows", par_DstFile, par_SrcFile)) return -1;
    return 0;
}


int load_desktop_file_ini (const char *fname)
{
    char wname[INI_MAX_SECTION_LENGTH];
    char section[INI_MAX_SECTION_LENGTH];
    int x;
    int file_exists;

    clear_desktop_ini ();

    file_exists =  _access (fname, 4) == 0;
    if (file_exists) {
        int Fd = IniFileDataBaseOpen(fname);
        if (Fd > 0) {
            copy_all_window_of_all_type(GetMainFileDescriptor(), Fd);
            for (x = 0; x < 10000; x++) {
                sprintf (section, "GUI/OpenWindowsForSheet%i", x);
                if (IniFileDataBaseLookIfSectionExist(section, Fd)) {
                    IniFileDataBaseCopySectionSameName(GetMainFileDescriptor(), Fd, section);
                } else if (x) break;
            }
            IniFileDataBaseReadString ("BasicSettings", "Selected Sheet", "",
                                       wname, sizeof(wname), Fd);
            IniFileDataBaseWriteString ("BasicSettings", "Selected Sheet",
                                       wname, GetMainFileDescriptor());

            IniFileDataBaseClose (Fd);
            return 0;
        }
    }
    return -1;
}

int save_desktop_file_ini (const char *fname)
{
    FILE *fh;
    char wname[INI_MAX_SECTION_LENGTH];
    char section[INI_MAX_SECTION_LENGTH];
    int x;

    int Fd = IniFileDataBaseCreateAndOpenNewIniFile(fname);

    if (Fd > 0) {
        copy_all_window_of_all_type(Fd, GetMainFileDescriptor());

        for (x = 0; x < 10000; x++) {
            sprintf (section, "GUI/OpenWindowsForSheet%i", x);
            if (IniFileDataBaseLookIfSectionExist(section, GetMainFileDescriptor())) {
                IniFileDataBaseCopySectionSameName(Fd, GetMainFileDescriptor(), section);
            } else if (x) break;
        }
        IniFileDataBaseReadString ("BasicSettings", "Selected Sheet", "",
                                   wname, sizeof(wname), GetMainFileDescriptor());
        IniFileDataBaseWriteString ("BasicSettings", "Selected Sheet",
                                    wname, Fd);
        if (IniFileDataBaseSave(Fd, fname, 2)) {
            ThrowError (1, "cannot save desktop file \"%s\"", fname);
            return -1;
        }
        return 0;
    }
    return -1;
}


int script_command_load_desktop_file (char *fname)
{
    LoadDesktopFromOtherThread (fname);
    return 0;
}

int script_command_save_desktop_file (char *fname)
{
    SaveDesktopFromOtherThread (fname);
    return 0;
}

int script_command_clear_desktop (void)
{
    ClearDesktopFromOtherThread ();
    return 0;
}
