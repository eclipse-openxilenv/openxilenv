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
#include <stdlib.h>
#include <windows.h>

#include "memory2.h"
#include "error2.h"
#include "levensht.h"
#include "p_tdump.h"
#include "sched.h"
#include "msgscm_.h" 

#include "WriteSection2Exe.h"
#include "WriteSection2Exe.rh"

#if 0
static void AddSectionToListBox (HWND Listbox, char *ProcessName)
{
    PROCESS_APPL_DATA *pappldata;
    int x;

    if (ProcessName != NULL) {
        pappldata = appl_param_check (NULL, NULL, (char*)ProcessName, 1);
        //pappldata = get_appldata_ptr (ProcessName, 0);
        if (pappldata != NULL) {
            if ((pappldata->SectionHeaders != NULL) || (pappldata->SectionNames != NULL) || (pappldata->NumOfSections > 0)) {
                for (x = 0; x < pappldata->NumOfSections; x++) {
					if (strcmp (pappldata->SectionNames[x], ".data") != 0) {
						if (SendMessage (Listbox, LB_FINDSTRINGEXACT, -1, (LPARAM)pappldata->SectionNames[x]) == LB_ERR) {
							ListBox_AddString (Listbox, pappldata->SectionNames[x]);
						}
					}
                }
            }
        }
    }
}

static long FAR PASCAL WriteSection2Exe_DlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
    char *p, *pname;
    int i;
    HWND hItem;

    switch (message) {
    case WM_INITDIALOG:
        EnableWindow (GetDlgItem (hwnd, ID_SAVE_AND_RESTART), FALSE);  // das geht nicht deshalb ausschalten!
        hItem = GetDlgItem (hwnd, ID_PROCESS_NAME);
        i = 1;
        while ((pname = read_next_process_name (i, 2)) != NULL) {
            if (i == 1) i = 0;
            p = pname;
            while (*p != '\0') p++;
            if ((*(p-4) == '.') && (*(p-3) == 'E') && (*(p-2) == 'X') && (*(p-1) == 'E')) {
                ListBox_AddString (hItem, GetProcessNameFromPath (pname));
            }
        }
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case 1:    /* OK */
        case ID_SAVE_AND_RESTART:
            {
                int x;
                char Section[256];  // eigentlich max 9 Zeichen
                short pid;
                char ProcessName[MAX_PATH];

                hItem = GetDlgItem (hwnd, ID_PROCESS_NAME);
                if (ListBox_GetCurSel (hItem) < 0) {
                    error (1, "process \"%s\" not running", ProcessName);
                    return TRUE;
                }
                ListBox_GetText (hItem, ListBox_GetCurSel (hItem), ProcessName);

                pid = get_pid_by_name (ProcessName);
                if (pid <= 0) {
                    error (1, "process \"%s\" not running", ProcessName);
                    return TRUE;
                }
                for (x = 0; x < ListBox_GetCount (GetDlgItem (hwnd, ID_SELECTED_SECTS)); x++) {
                    ListBox_GetText (GetDlgItem (hwnd, ID_SELECTED_SECTS), x, Section);
                    if (scm_write_section_to_exe (pid, Section)) {
                        error (1, "cannot write section \"%s\" to \"%s\"", Section, ProcessName);
                    }
                }
                // das geht nicht /fuehrt zum Haenger) deshalb ausschalten!
                /*if (LOWORD (wParam) == ID_SAVE_AND_RESTART) {
                    get_name_by_pid (pid, ProcessName);
                    terminate_process (pid);
                    while (get_pid_by_name (ProcessName) > 0) {
                        scheduler ();
                    }
                    aktivate_process (ProcessName); 
                }*/
            }
            EndDialog (hwnd, 0);
            return TRUE;
        case 2:    /* Cancel */
            EndDialog (hwnd, 0);
            return TRUE;
        case 101:    /* Process Combobox */
            if (HIWORD (wParam) == LBN_SELCHANGE) {
                char ProcessName[MAX_PATH];
                // neuer Prozess ausgewaehlt
                hItem = GetDlgItem (hwnd, ID_PROCESS_NAME);
                if (ListBox_GetCurSel (hItem) >= 0) {
                    ListBox_GetText (hItem, ListBox_GetCurSel (hItem), ProcessName);
                    ListBox_ResetContent (GetDlgItem (hwnd, ID_AVAIL_SECTS));
                    AddSectionToListBox (GetDlgItem (hwnd, ID_AVAIL_SECTS), ProcessName);
                    ListBox_ResetContent (GetDlgItem (hwnd, ID_SELECTED_SECTS));
                }
            }
            return TRUE;
        case 201: // Add
            {
                int Idx;
                char Section[32];
                if ((Idx = SendMessage (GetDlgItem (hwnd, 204), LB_GETCURSEL, 0, 0)) != LB_ERR) {
                    SendMessage (GetDlgItem (hwnd, 204), LB_GETTEXT, Idx, (LPARAM)Section);
                    SendMessage (GetDlgItem (hwnd, 200), LB_ADDSTRING, 0, (LPARAM)Section);
                }
            }
            return TRUE;
        case 202: // Del
            {
                int Idx;
                if ((Idx = SendMessage (GetDlgItem (hwnd, 200), LB_GETCURSEL, 0, 0)) != LB_ERR) {
                    SendMessage (GetDlgItem (hwnd, 200), LB_DELETESTRING, Idx, 0);
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

extern HINSTANCE  hInst;


void WriteSection2ExeDialog (HWND Hwnd)
{
    DialogBoxParam (hInst, "WRITE_SECTION_TO_EXE_DLG", Hwnd, (DLGPROC)WriteSection2Exe_DlgProc, (LPARAM)0);
}

#endif
