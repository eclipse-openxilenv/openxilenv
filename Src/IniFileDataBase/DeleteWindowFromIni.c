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

#include "Config.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "IniDataBase.h"
#include "DeleteWindowFromIni.h"

int DeleteWindowFromIniFile (const char *WindowName, const char *WindowList)
{
    int s, x, Found;
    char Entry[64];
    char Section[INI_MAX_SECTION_LENGTH];
    char NamePath[INI_MAX_SECTION_LENGTH];
    char SheetName[INI_MAX_SECTION_LENGTH];
    int Fd = GetMainFileDescriptor();

    STRING_COPY_TO_ARRAY(NamePath, "GUI/Widgets/");
    char *Name = NamePath + strlen(NamePath);
    int MaxLen = sizeof(NamePath) - (Name - NamePath);
    if (WindowList != NULL) {
        Found = 0;
        for (x = 0;;x++) {
            PrintFormatToString (Entry, sizeof(Entry), "W%i", x);
            if (IniFileDataBaseReadString (WindowList, Entry, "", Name, MaxLen, Fd) == 0)
                break;   // End of list
            if (!strcmp (WindowName, Name)) {
                IniFileDataBaseWriteString (WindowList, Entry, NULL, Fd); // delete from INI list
                // delete the window from INI file
                IniFileDataBaseWriteString (NamePath, NULL, NULL, Fd);
                Found++;
            } else {
                IniFileDataBaseWriteString (WindowList, Entry, NULL, Fd); // delete from INI list
                PrintFormatToString (Entry, sizeof(Entry), "W%i", x - Found);
                IniFileDataBaseWriteString (WindowList, Entry, Name, Fd); // write it back to INI list
            }
        }
    }
    for (s = 0;; s++) {
        if (s == 0) {  // default Sheet
            PrintFormatToString (Section, sizeof(Section), "GUI/OpenWindowsForSheet%i", s);
            STRING_COPY_TO_ARRAY (SheetName, "default");
        } else {
            PrintFormatToString (Section, sizeof(Section), "GUI/OpenWindowsForSheet%i", s);
            if (IniFileDataBaseReadString (Section, "SheetName", "",
                                           SheetName, sizeof(SheetName), Fd) == 0) {
                break; // End of list
            }
        }

        Found = 0;
        for (x = 1;;x++) {
            PrintFormatToString (Entry, sizeof(Entry), "W%i", x);
            if (IniFileDataBaseReadString (Section, Entry, "", NamePath, sizeof (NamePath), Fd) == 0)
                break;   // End of list
            if (!strcmp (WindowName, Name)) {
                IniFileDataBaseWriteString (Section, Entry, NULL, Fd); // delete from INI list
                PrintFormatToString (Entry, sizeof(Entry), "WP%i", x);
                IniFileDataBaseWriteString (Section, Entry, NULL, Fd); // delete from INI list
                Found++;
            } else {
                IniFileDataBaseWriteString (Section, Entry, NULL, Fd); // delete from INI list
                PrintFormatToString (Entry, sizeof(Entry), "W%i", x-Found);
                IniFileDataBaseWriteString (Section, Entry, NamePath, Fd); // write it back to INI list
                PrintFormatToString (Entry, sizeof(Entry), "WP%i", x);
                IniFileDataBaseReadString (Section, Entry, "", NamePath, sizeof (NamePath), Fd);
                IniFileDataBaseWriteString (Section, Entry, NULL, Fd); // delete from INI list
                PrintFormatToString (Entry, sizeof(Entry), "WP%i", x-Found);
                IniFileDataBaseWriteString (Section, Entry, NamePath, Fd); // write it back to INI list
            }
        }
    }
    return 0;
}

