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
#include "Platform.h"
#include "UtilsWindow.h"
#include "IniSectionEntryDefines.h"
#include "IniDataBase.h"
#include "Files.h"
#include "StringMaxChar.h"
#include "WindowIniHelper.h"
#include "DeleteWindowFromIni.h"

#include "MainWinowSyncWithOtherThreads.h"


static char *GetWindowNameFromSectionPath(char *par_SectionPath)
{
    if (!strcmp(par_SectionPath, "GUI/Widgets/")) {
        return par_SectionPath + strlen("GUI/Widgets/");
    }
    return NULL;
}

int OpenWindowByName (char *WindowName)
{
    char WindowSectionPath[INI_MAX_LINE_LENGTH];
    char WindowType[INI_MAX_LINE_LENGTH];
    char *SectionName;
    int Ret = -1;

    StringCopyMaxCharTruncate(WindowSectionPath, "GUI/Widgets/", sizeof(WindowSectionPath));
    STRING_APPEND_TO_ARRAY(WindowSectionPath, WindowName);

    if (!IsWindowOpenFromOtherThread(WindowName)) {
        if (IniFileDataBaseReadString (WindowSectionPath, OPN_WND_TYP_TEXT,
                                       "", WindowType, sizeof(WindowType),
                                       GetMainFileDescriptor()) > 0) {
            if (GetSection_AllWindowsOfType (WindowType, &SectionName) == 0) {
                int XPos;
                int YPos;
                int Width;
                int Height;
                XPos = IniFileDataBaseReadInt (WindowSectionPath, "left", 0, GetMainFileDescriptor());
                YPos = IniFileDataBaseReadInt (WindowSectionPath, "top", 0, GetMainFileDescriptor());
                Width = IniFileDataBaseReadInt (WindowSectionPath, "width", 0, GetMainFileDescriptor());
                Height = IniFileDataBaseReadInt (WindowSectionPath, "height", 0, GetMainFileDescriptor());
                if (OpenWindowByNameFromOtherThread (WindowName, 1, XPos, YPos, 1, Width, Height) == 0) {
                    Ret = 0;
                }
            }
        }
    }
    return Ret;
}

int OpenWindowByFilter (char *WindowNameFilter)
{
    int Idx;
    char *pWindowList;
    char Filter[INI_MAX_LINE_LENGTH];
    char WindowSectionPath[INI_MAX_SECTION_LENGTH];
    char WindowType[32];

    StringCopyMaxCharTruncate(Filter, "GUI/Widgets/", sizeof(WindowSectionPath));
    STRING_APPEND_TO_ARRAY(Filter, WindowNameFilter);
    Idx = 0;
    while ((Idx = IniFileDataBaseFindNextSectionNameRegExp (Idx, Filter, 1, WindowSectionPath, sizeof(WindowSectionPath),
                                                           GetMainFileDescriptor())) >= 0) {
        if (IniFileDataBaseReadString (WindowSectionPath, OPN_WND_TYP_TEXT,
                                       "", WindowType, sizeof(WindowType),
                                       GetMainFileDescriptor()) > 0) {
            if (GetSection_AllWindowsOfType (WindowType, &pWindowList) == 0) {
                char *WindowName = GetWindowNameFromSectionPath(WindowSectionPath);
                if (WindowName != NULL) {
                    if (IsIn_AllWindowOfTypeList (GetMainFileDescriptor(), pWindowList, WindowName, 0) >= 0) {
                        OpenWindowByName (WindowName);
                    }
                }
            }
        }
    }
    return 0;
}

int CloseWindowByName (char *WindowName)
{
    return CloseWindowByNameFromOtherThread(WindowName);
}

int CloseWindowByFilter (char *WindowNameFilter)
{
    return CloseWindowByNameFromOtherThread(WindowNameFilter);
}

int ExportWindowByFilter (char *SheetFilter, char *WindowNameFilter, char *FileName)
{
    char *WindowList;
    int Idx;
    char Text[64];
    char WindowType[32];
    char WindowSectionPath[INI_MAX_LINE_LENGTH];
    char Filter[INI_MAX_LINE_LENGTH];

    int Fd = IniFileDataBaseOpen(FileName);
    if (Fd <= 0) {
        // File not exists
        Fd = IniFileDataBaseCreateAndOpenNewIniFile(FileName);
        if (Fd <= 0) {
            return -1;
        }
        // Insert a section/entry marker
        IniFileDataBaseWriteString ("ListOfExportedWindowsIdentifingSection",
                                     "ListOfExportedWindowsIdentifingEntry",
                                     "ListOfExportedWindowsIdentifingString", Fd);
    } else {
        // If  file exist check if the file has the section/entry marker
        IniFileDataBaseReadString ("ListOfExportedWindowsIdentifingSection",
                                    "ListOfExportedWindowsIdentifingEntry",
                                    "", Text, sizeof (Text), Fd);
        if (strcmp ("ListOfExportedWindowsIdentifingString", Text)) {
            return -1;
        }
    }

    // guarantee that all window information are stored into the INI DB
    SaveAllConfigToIniDataBaseFromOtherThread();

    StringCopyMaxCharTruncate(Filter, "GUI/Widgets/", sizeof(WindowSectionPath));
    STRING_APPEND_TO_ARRAY(Filter, WindowNameFilter);

    Idx = 0;
    while ((Idx = IniFileDataBaseFindNextSectionNameRegExp (Idx, Filter, 1, WindowSectionPath, sizeof(WindowSectionPath),
                                                           GetMainFileDescriptor())) >= 0) {
        if (IniFileDataBaseReadString (WindowSectionPath, OPN_WND_TYP_TEXT,
                                        "", WindowType, sizeof(WindowType),
                                        GetMainFileDescriptor()) > 0) {
            if (GetSection_AllWindowsOfType (WindowType, &WindowList) == 0) {
                char *WindowName = GetWindowNameFromSectionPath(WindowSectionPath);
                if (WindowName != NULL) {
                    if (IsIn_AllWindowOfTypeList (GetMainFileDescriptor(), WindowList, WindowName, 0) >= 0) {
                        if ((SheetFilter == NULL) || IsIn_Sheet (GetMainFileDescriptor(), SheetFilter, WindowName)) {
                            // Export window section into file
                            IniFileDataBaseCopySection (Fd, GetMainFileDescriptor(), WindowSectionPath, WindowSectionPath);
                        }
                    }
                }
            }
        }
    }
    IniFileDataBaseClose(Fd);
    return 0;
}

int ImportWindowByFilter (char *WindowNameFilter, char *FileName)
{
    FILE *fh;
    char *Help;
    int Idx;
    char WindowType[32];
    char Filter[INI_MAX_SECTION_LENGTH];
    char WindowSectionPath[INI_MAX_SECTION_LENGTH];

    SaveAllConfigToIniDataBaseFromOtherThread();

    int Fd = IniFileDataBaseOpen(FileName);
    if (Fd <= 0) {
        return -1;
    }

    StringCopyMaxCharTruncate(Filter, "GUI/Widgets/", sizeof(WindowSectionPath));
    STRING_APPEND_TO_ARRAY(Filter, WindowNameFilter);

    Idx = 0;
    while ((Idx = IniFileDataBaseFindNextSectionNameRegExp (Idx, Filter, 1, WindowSectionPath, sizeof(WindowSectionPath), Fd)) >= 0) {
        if (IniFileDataBaseReadString (WindowSectionPath, OPN_WND_TYP_TEXT,
                                       "", WindowType, sizeof(WindowType),
                                       Fd) > 0) {
            if (GetSection_AllWindowsOfType (WindowType, &Help) == 0) {
                char *WindowName = GetWindowNameFromSectionPath(WindowSectionPath);
                if (WindowName != NULL) {
                    int XPos;
                    int YPos;
                    int Width;
                    int Height;
                    IniFileDataBaseCopySection (GetMainFileDescriptor(), Fd, WindowSectionPath, WindowSectionPath);
                    XPos = IniFileDataBaseReadInt (WindowSectionPath, "left", 0, Fd);
                    YPos = IniFileDataBaseReadInt (WindowSectionPath, "top", 0, Fd);
                    Width = IniFileDataBaseReadInt (WindowSectionPath, "width", 0, Fd);
                    Height = IniFileDataBaseReadInt (WindowSectionPath, "height", 0, Fd);
                    if (OpenWindowByNameFromOtherThread (WindowName, 1, XPos, YPos, 1, Width, Height)) {
                        IniFileDataBaseWriteString (WindowSectionPath, NULL, NULL, GetMainFileDescriptor());
                        IniFileDataBaseClose(Fd);
                        return -1;
                    }
                }
            }
        }
    }
    IniFileDataBaseClose(Fd);
    return 0;
}


int DeleteWindowByFilter (char *WindowNameFilter)
{
    int Idx;
    char *pWindowList;
    char WindowSectionPath[INI_MAX_SECTION_LENGTH];
    char Filter[INI_MAX_SECTION_LENGTH];
    char WindowType[32];

    SaveAllConfigToIniDataBaseFromOtherThread();

    CloseWindowByNameFromOtherThread(WindowNameFilter);

    StringCopyMaxCharTruncate(Filter, "GUI/Widgets/", sizeof(WindowSectionPath));
    STRING_APPEND_TO_ARRAY(Filter, WindowNameFilter);
    Idx = 0;
    while ((Idx = IniFileDataBaseFindNextSectionNameRegExp(Idx, Filter, 1, WindowSectionPath, sizeof(WindowSectionPath),
                                                           GetMainFileDescriptor())) >= 0) {
        if (IniFileDataBaseReadString (WindowSectionPath, OPN_WND_TYP_TEXT,
                                       "", WindowType, sizeof(WindowType),
                                       GetMainFileDescriptor()) > 0) {
            if (GetSection_AllWindowsOfType (WindowType, &pWindowList) == 0) {
                char *WindowName = GetWindowNameFromSectionPath(WindowSectionPath);
                if (WindowName != NULL) {
                    if (IsIn_AllWindowOfTypeList (GetMainFileDescriptor(), pWindowList, WindowName, 0) >= 0) {
                        DeleteWindowFromIniFile (WindowName, pWindowList);
                    }
                }
            }
        }
    }
    return 0;
}

int ScriptOpenWindowByFilter (char *WindowNameFilter)
{
    return OpenWindowByFilter(WindowNameFilter);
}

int ScriptCloseWindowByFilter (char *WindowNameFilter)
{
    return CloseWindowByFilter(WindowNameFilter);
}

int ScriptExportWindowByFilter (char *SheetFilter, char *WindowNameFilter, char *FileName)
{
    return ExportWindowByFilter(SheetFilter, WindowNameFilter, FileName);
}

int ScriptImportWindowByFilter (char *WindowNameFilter, char *FileName)
{
    return ImportWindowByFilter(WindowNameFilter, FileName);
}

int ScriptDeleteWindowByFilter (char *WindowNameFilter)
{
    return DeleteWindowByFilter(WindowNameFilter);
}

