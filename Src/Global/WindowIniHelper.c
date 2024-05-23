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
#include <stdlib.h>
#include "IniDataBase.h"
#include "DebugInfos.h"
#include "MainValues.h"
#include "IniSectionEntryDefines.h"
#include "Wildcards.h"
#include "WindowIniHelper.h"


int GetSection_AllWindowsOfType (char *WindowType, char **pSectionName)
{
    int Ret = 0;

    if (!strcmp (WindowType, "Text")) {
        *pSectionName = "GUI/AllTextWindows";
    } else if (!strcmp (WindowType, "Oscilloscope")) {
        *pSectionName = "GUI/AllOscilloscopeWindows";
    } else if (!strcmp (WindowType, "Tachometer")) {
        *pSectionName = "GUI/AllTachometerWindows";
    } else if (!strcmp (WindowType, "Slider")) {
        *pSectionName = "GUI/AllSliderWindows";
    } else if (!strcmp (WindowType, "Bargraph")) {
        *pSectionName = "GUI/AllBargraphWindows";
    } else if (!strcmp (WindowType, "Knob")) {
        *pSectionName = "GUI/AllKnobWindows";
    } else if (!strcmp (WindowType, "CalibrationTree")) {
        *pSectionName = "GUI/AllCalibrationTreeWindows";
    } else if (!strcmp (WindowType, "CalibrationMap")) {
        *pSectionName = "GUI/AllCalibrationMapViewWindows";
    } else if (!strcmp (WindowType, "Enum")) {
        *pSectionName = "GUI/AllEnumWindows";
    } else if (!strcmp (WindowType, "CANMessages")) {
        *pSectionName = "GUI/AllCanMessageWindows";
    } else if (!strcmp (WindowType, "FlexrayMessages")) {
        *pSectionName = "GUI/AllFlexrayMessageWindows";
    } else if (!strcmp (WindowType, "ControlLamps")) {
        *pSectionName = "GUI/AllControlLampsViewWindows";
    } else if (!strcmp (WindowType, "UserDraw")) {
        *pSectionName = "GUI/AllUserDrawWindows";
    } else if (!strcmp (WindowType, "UserControl")) {
        *pSectionName = "GUI/AllUserControlWindows";
    } else if (!strcmp (WindowType, "A2lMapView")) {
        *pSectionName = "GUI/AllA2lMapViewWindows";
    } else if (!strcmp (WindowType, "A2lSingleView")) {
        *pSectionName = "GUI/AllA2lSingleViewWindows";
    } else {
        *pSectionName = NULL;
        Ret = -1;
    }
    return Ret;
}


int IsIn_AllWindowOfTypeList (int par_Fd, char *WindowList, const char *WindowName, int StartIdx)
{
    int x, ret = -1;
    char Entry[64];
    char RefName[INI_MAX_SECTION_LENGTH];

    if (WindowList == NULL) return 0;

    for (x = StartIdx;;x++) {
        sprintf (Entry, "W%i", x);
        if (IniFileDataBaseReadString (WindowList, Entry, "", RefName, sizeof (RefName), par_Fd) == 0) {
            ret = -1;
            break;
        }
        if (!strcmp (WindowName, RefName)) {
            ret = x;
            break;
        }
    }
    return x;
}

int IsIn_Sheet (int par_Fd, const char *SheetFilter, const char *WindowName)
{
    int s, x;
    char Entry[64];
    char Section[INI_MAX_SECTION_LENGTH];
    char RefWindowName[INI_MAX_SECTION_LENGTH];
    char SheetName[INI_MAX_SECTION_LENGTH];

    for (s = 0;; s++) {
        sprintf (Section, "GUI/OpenWindowsForSheet%i", s);
        if (s == 0) {  // default sheet
            strcpy (SheetName, "default");
        } else {
            if (IniFileDataBaseReadString (Section, "SheetName", "",
                                           SheetName, sizeof(SheetName), par_Fd) == 0) {
                break; // End of the list
            }
        }
        if (!Compare2StringsWithWildcardsCaseSensitive (SheetName, SheetFilter, 1)) {
            for (x = 1;;x++) {
                sprintf (Entry, "W%i", x);
                if (IniFileDataBaseReadString(Section, Entry, "", RefWindowName, sizeof (RefWindowName), par_Fd) == 0)
                    break;   // End of the list
                if (!strcmp (WindowName, RefWindowName)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}
