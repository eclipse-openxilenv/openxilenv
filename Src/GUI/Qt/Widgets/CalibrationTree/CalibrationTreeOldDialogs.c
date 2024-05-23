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


#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>

#include "windows_help_func.h"
#include "sched.h"
#include "ini_db.h"
#include "memory2.h"
#include "error2.h"
#include "appl.h"
#include "p_tdump.h"
#include "blackboard.h"
#include "ExtProcessReferences.h"
#include "CallibrationTreeOldDialogs.h"

#if 0
#if 0
// ist auch in ExtProcessReferences.c
int remove_referenced_vari_ini (int pid, char *lname)
{
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char pname[MAX_PATH];
    char entry[32];
    char variname[BBVARI_NAME_SIZE];
    char DisplayName[2*BBVARI_NAME_SIZE+1];
    int x, found = 0;

    get_name_by_pid ((unsigned short)pid, pname);
    if (GetProcessNameWithoutPath (pid, pname_withoutpath)) {
        return -1;
    }
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    if (DBFindNextSectionNameRegExp (0, section_filter, 0, section, sizeof(section), INIFILE) < 0) {
        return -1;
    }
    // suche in Variabalenliste in INI-File
    for (x = 0;;x++) {
        sprintf (entry, "Ref%i", x);
        if (DBGetPrivateProfileString(section, entry, "", variname, sizeof (variname), INIFILE) == 0) break;
        ConvertLabelAsapCombatible (variname, 0);   // :: durch ._. ersetzen falls ntig
        if (!CmpVariName (variname, lname)) {
            DBWritePrivateProfileString(section, entry, NULL, INIFILE);
            sprintf (entry, "Dsp%i", x);
            DBWritePrivateProfileString(section, entry, NULL, INIFILE);
            found++;
        } else if (found) {
            DBWritePrivateProfileString (section, entry, NULL, INIFILE);
            sprintf (entry, "Ref%i", x - found);
            DBWritePrivateProfileString (section, entry, variname, INIFILE);

            sprintf (entry, "Dsp%i", x);
            if (DBGetPrivateProfileString (section, entry, "", DisplayName, sizeof (DisplayName), INIFILE) > 0) {
                DBWritePrivateProfileString (section, entry, NULL, INIFILE);
                sprintf (entry, "Dsp%i", x - found);
                DBWritePrivateProfileString (section, entry, DisplayName, INIFILE);
            }
        }
    }
    return -1;   // Variable nicht gefunden
}
#endif

static int ExtractNames (char *ListboxString, char *VariName, char *DisplayName, int DisplayNameFirstFlag)
{
    char *p;
    char *p1s, *p1e, *p2s, *p2e;
    int HaveDispayName = 0;
    int l1, l2;

    p = ListboxString;
    while ((*p != 0) && isspace (*p)) p++;  // Leerzeichen entfernen
    p1s = p;
    while ((*p != 0) && !isspace (*p)) p++;
    p1e = p;
    l1 = p1e - p1s;
    while ((*p != 0) && isspace (*p)) p++;  // Leerzeichen entfernen
    if (*p == '(') {
        HaveDispayName = 1;
        p++;
        p2s = p;
        while ((*p != 0) && !isspace (*p) && (*p != ')')) p++;
        p2e = p;
        l2 = p2e - p2s;
    }
    if (HaveDispayName) {
        if (DisplayNameFirstFlag) {
            MEMCPY (VariName, p2s, l2);
            VariName[l2] = 0;
            MEMCPY (DisplayName, p1s, l1);
            DisplayName[l1] = 0;
        } else {
            MEMCPY (VariName, p1s, l1);
            VariName[l1] = 0;
            MEMCPY (DisplayName, p2s, l2);
            DisplayName[l2] = 0;
        }
    } else {
        MEMCPY (VariName, p1s, l1);
        VariName[l1] = 0;
        MEMCPY (DisplayName, p1s, l1);
        DisplayName[l1] = 0;
    }
    return 0;
}




static int RefillListBox (HWND hwnd, int pid, int DisplayNameFirstFlag)
{
    int x;
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char entry[32];
    char variname[BBVARI_NAME_SIZE];
    char displayname[BBVARI_NAME_SIZE];
    char ListboxString[2*BBVARI_NAME_SIZE + 16];

    SendDlgItemMessage (hwnd, 102, (UINT)LB_RESETCONTENT, 0, 0);
    if (GetProcessNameWithoutPath (pid, pname_withoutpath)) {
        return -1;
    }
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    if (DBFindNextSectionNameRegExp (0, section_filter, 0, section, sizeof(section), INIFILE) >= 0) {
        // komplette Variabalenliste in INI-File lesen
        for (x = 0;;x++) {
            sprintf (entry, "Ref%i", x);
            if (DBGetPrivateProfileString (section, entry, "", variname, BBVARI_NAME_SIZE, INIFILE) == 0) break;
            if (GetDisplayName (section, x, variname, displayname, sizeof (displayname)) == 0) {
                ConvertLabelAsapCombatible (variname, 0);   // :: durch ._. ersetzen falls ntig
                if (DisplayNameFirstFlag) strcpy (ListboxString, displayname);
                else strcpy (ListboxString, variname);
                strcat (ListboxString, "    (");
                if (DisplayNameFirstFlag) strcat (ListboxString, variname);
                else strcat (ListboxString, displayname);
                strcat (ListboxString, ")");
            } else {
                ConvertLabelAsapCombatible (variname, 0);   // :: durch ._. ersetzen falls ntig
                strcpy (ListboxString, variname);
            }
            SendDlgItemMessage (hwnd, 102, LB_ADDSTRING, 0, (LPARAM)((LPSTR)ListboxString));
        }
    }
    return 0;
}


static long FAR PASCAL show_referenced_labels_WndProc(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
    int i;
    int *sel_list;
    int items;
    unsigned long address;
    long typenr;
    int type;
    static char *pname;
    static short pid;
    static int DisplayNameFirstFlag;
    PROCESS_APPL_DATA *pappldata;

    switch (message) {
    case WM_INITDIALOG:
        DisplayNameFirstFlag = 0;
        SendMessage(GetDlgItem(hwnd, 102), LB_SETHORIZONTALEXTENT, 1000, 0);
        pname = (char*)lParam;
        if ((pid = get_pid_by_name (pname)) <= 0)
            EndDialog (hwnd, 0);
        if (RefillListBox (hwnd, pid, DisplayNameFirstFlag))
            EndDialog (hwnd, 0);
        SetDlgItemText (hwnd, 101, pname);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1:    /* Close */
            EndDialog (hwnd, 0);
            return TRUE;
        case 105:  // edit label

            return TRUE;
        case 104:
            if (IsDlgButtonChecked (hwnd, 104) != DisplayNameFirstFlag) {
                DisplayNameFirstFlag = IsDlgButtonChecked (hwnd, 104);
                RefillListBox (hwnd, pid, DisplayNameFirstFlag);
            }
            return TRUE;
        case 103:  /* dereference Label */
            // Applikationsparameter neu lesen falls noch nicht geschehen (Debug-Infos)
            pappldata = appl_param_check (NULL, NULL, pname, 0);
            if (((items = (int)SendDlgItemMessage (hwnd, 102, LB_GETSELCOUNT, 0, 0L)) != LB_ERR) &&
                 ((sel_list = (int*)my_malloc (items * sizeof (int))) != NULL)) {
                SendDlgItemMessage (hwnd, 102, LB_GETSELITEMS, items, (LPARAM)sel_list);
                for (i = items-1; i >= 0; i--) {
                    char ListboxString[2*BBVARI_NAME_SIZE + 16];
                    char variname[BBVARI_NAME_SIZE];
                    char DispayNname[BBVARI_NAME_SIZE];
                    SendDlgItemMessage (hwnd, 102, LB_GETTEXT, (WPARAM)sel_list[i], (LPARAM)ListboxString);
                    ExtractNames (ListboxString, variname, DispayNname, DisplayNameFirstFlag);
                    remove_referenced_vari_ini (pid, variname);
                    ConvertLabelAsapCombatible (variname, 2);   // ._. immer durch :: ersetzen, appl_label kennt ._. nicht!
                    if (appl_label (get_pid_by_name (pname), variname, &address, &typenr)) {
                        error (1, "try to derefrence unknown variable %s", variname);
                    } else if ((type = get_base_type_bb_type_ex (typenr, pappldata)) < 0) {
                        error (1, "derefrence variable %s with unknown type", variname);
                    } else {
                        // loeschen Label aus Refernce-Liste im INI-File
                         ConvertLabelAsapCombatible (variname, 0);   // :: durch ._. ersetzen falls ntig
                        if (scm_unref_vari (address, pid, DispayNname, type)) {
                            error (1, "cannot dereference variable (extern process not running or halted in debugger)");
                        } else {
                            DelVarFromProcessRefList (pid, variname);
                        }
                    }
                }
                for (i = items-1; i >= 0; i--) {
                    SendDlgItemMessage (hwnd, 102, LB_DELETESTRING, (WPARAM)sel_list[i], (LPARAM)0);
                }
                my_free (sel_list);
            }
            return TRUE;;
        }
        break;
    }
    return FALSE;
}


int schow_referenced_labels_dlg (HWND hwnd, char *pname)
{
    int rt;

    rt = DialogBoxParam (NULL, "REFERENCED_LABEL", hwnd, (DLGPROC)show_referenced_labels_WndProc, (LPARAM)(pname));
    return rt;
}


static void EnableDisableHexDecButtoms (HWND hwnd)
{
    if (IsDlgButtonChecked (hwnd, 118) == BST_CHECKED) {
        EnableWindow (GetDlgItem(hwnd, 120), TRUE);
        EnableWindow (GetDlgItem(hwnd, 121), TRUE);
    } else {
        EnableWindow (GetDlgItem(hwnd, 120), FALSE);
        EnableWindow (GetDlgItem(hwnd, 121), FALSE);
    }
}


static long __stdcall appl_reference_param_WndProc(HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
    char txt[1024+3];
    int index, i;
    HWND hItem;
    static HWND hwndLV;
    static CALLIBRATION_CONFIG *papplwin;
    char *pname, *p;
    static int OldShowValuesFlag;

    switch (message) {
    case WM_INITDIALOG:
        papplwin = (CALLIBRATION_CONFIG*)lParam;
        // Prozesslist fuellen
        hItem = GetDlgItem(hwnd, 101);

        // alles Prozesse durchlaufen
        i = 1;
        while ((pname = read_next_process_name(i, 2)) != NULL) {
            i = 0;
            if (!IsInternalProcess (pname)) {
                ListBox_AddString(hItem, pname);
            }
        }
        ResizeListboxSliderSize (hItem);

        // Zuletzt ausgewaehlten Prozess selektieren
        if ((index = ListBox_FindString(hItem, -1, papplwin->process_name)) != LB_ERR) {
            ListBox_SetCurSel(hItem, index);
        }
        SetDlgItemText (hwnd, 105, papplwin->filter);
        SetDlgItemText (hwnd, 117, papplwin->winname);
        CheckDlgButton (hwnd, 118, papplwin->show_values);
        if (papplwin->hex_or_dec)
            CheckRadioButton (hwnd, 120, 121, 121);
        else CheckRadioButton (hwnd, 120, 121, 120);
        EnableDisableHexDecButtoms (hwnd);
        if (papplwin->IncExcFilter != NULL)
            FillDialogIncludeExcludeLists (papplwin->IncExcFilter, GetDlgItem (hwnd, 200), GetDlgItem (hwnd, 300));

        OldShowValuesFlag = papplwin->show_values;
        return TRUE;
    case WM_DESTROY:
        DestroyWindow (hwndLV);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 201:
            AddElementToIncludeOrExcludeDialogList (hwnd, GetDlgItem (hwnd, 200));
            break;
        case 202:
            DelElementFromIncludeOrExcludeDialogList (GetDlgItem (hwnd, 200));
            break;
        case 301:
            AddElementToIncludeOrExcludeDialogList (hwnd, GetDlgItem (hwnd, 300));
            break;
        case 302:
            DelElementFromIncludeOrExcludeDialogList (GetDlgItem (hwnd, 300));
            break;
        case 1111:   /* show referenced labels */
            index = (int)SendDlgItemMessage (hwnd, 101, LB_GETCURSEL, 0, 0);
            if (index == LB_ERR) return TRUE;
            SendDlgItemMessage (hwnd, 101, LB_GETTEXT, index, (LPARAM)((LPSTR)txt));
            schow_referenced_labels_dlg (hwnd, txt);
            break;
        case 1:    /* OK */
            GetDlgItemText (hwnd, 105, papplwin->filter, 512);
            GetDlgItemText (hwnd, 117, papplwin->winname, MAX_PATH);
            if (IsDlgButtonChecked (hwnd, 118) == BST_CHECKED) papplwin->show_values = 1;
            else papplwin->show_values = 0;

            //if (OldShowValuesFlag != papplwin->show_values) ResizeChildWindows (papplwin->hwnd, papplwin);
            index = (int)SendDlgItemMessage (hwnd, 101, LB_GETCURSEL, 0, 0);
            if (index == LB_ERR) return TRUE;
            SendDlgItemMessage (hwnd, 101, LB_GETTEXT, index, (LPARAM)((LPSTR)papplwin->process_name));
            strupr (papplwin->process_name);
            papplwin->pid = get_pid_by_name (papplwin->process_name);
            sprintf (txt, "%s (%s)", papplwin->winname, papplwin->process_name);
            //SetWindowText (papplwin->hwnd, txt);
            if (IsDlgButtonChecked (hwnd, 121) > 0) {
                papplwin->hex_or_dec = 1;
            } else {
                papplwin->hex_or_dec = 0;
            }
            // alter Filter loeschen falls vorhanden
            if (papplwin->IncExcFilter != NULL)
                FreeIncludeExcludeFilter (papplwin->IncExcFilter);
            papplwin->IncExcFilter = BuildIncludeExcludeFilterFromDlg (GetDlgItem (hwnd, 105), GetDlgItem (hwnd, 200), GetDlgItem (hwnd, 300));

            // Aenderungen gleich ins INI schreiben
            //write_applwin_ini (papplwin);

            //papplwin->pappldata = AttachOrLoadDebugInfos (papplwin->UniqueNumber, papplwin->process_name, 0);

            EndDialog (hwnd, 1);
            return TRUE;
        case 118:
            EnableDisableHexDecButtoms (hwnd);
            return TRUE;
        case 2:    /* Cancel */
            EndDialog (hwnd, 0);
            return TRUE;
        }
    }
    return FALSE;
}

int parameter_reference_dlg (CALLIBRATION_CONFIG *papplwin)
{
    int rt;

    rt = DialogBoxParam (NULL, "APPL_DLG", NULL, (DLGPROC)appl_reference_param_WndProc, (LPARAM)papplwin);
    return rt;
}

#endif
