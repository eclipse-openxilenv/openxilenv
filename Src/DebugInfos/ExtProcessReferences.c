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
#include "Config.h"
#include "Scheduler.h"
#include "PipeMessages.h"
#include "IniDataBase.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "Scheduler.h"
#include "DebugInfos.h"
#include "Blackboard.h"
#include "MainValues.h"
#include "UniqueNumber.h"
#include "DebugInfoDB.h"
#include "DebugInfoAccessExpression.h"
#include "BlackboardConvertFromTo.h"
#include "ExtProcessReferences.h"

// This will store a list of referenced addressesand names of variables of a process
typedef struct {
    char *Name;
    uint64_t Addr;
    int Type;
    int Filler;
} NAME_ADDR;

typedef struct {
    int Pid;
    int SizeOfArray;
    int LevelArray;
    NAME_ADDR *NameAddr;
} PROCESS_REF_ADDR_ARRAY;

// List of all manuell referenced variables
// with addresse and data type
static PROCESS_REF_ADDR_ARRAY ProcessRefAddrArrays[MAX_EXTERN_PROCESSES];
static CRITICAL_SECTION ProcessRefAddrArrayCriticalSection;

int InitProcessRefAddrCriticalSection (void)
{
    INIT_CS (&ProcessRefAddrArrayCriticalSection);
    return 0;
}

int SearchProcessInArray (int Pid)
{
    int x;
    if (!Pid) return -1;
    for (x = 0; x < MAX_EXTERN_PROCESSES; x++) {
        if (ProcessRefAddrArrays[x].Pid == Pid) return x;
    }
    return -1;
}

int CheckProcessInArray (int Pid)
{
    int x;
    if ((x = SearchProcessInArray (Pid)) >= 0) return x;   // Process already inside the array
    for (x = 0; x < MAX_EXTERN_PROCESSES; x++) {
        if (ProcessRefAddrArrays[x].Pid == 0) {
            ProcessRefAddrArrays[x].Pid = Pid;
            return x;
        }
    }
    return -1;
}

// This will call from the scheduler fuction kill_process()
int DelProcessFromArray (int Pid)
{
    int IdxProcess, IdxVari;
    int Ret = -1;

    ENTER_CS (&ProcessRefAddrArrayCriticalSection);
    IdxProcess = SearchProcessInArray (Pid);
    if (IdxProcess < 0) {
        Ret = -1;   // Process is not inside the array
    } else {
        for (IdxVari = 0; IdxVari < ProcessRefAddrArrays[IdxProcess].LevelArray; IdxVari++) {
            my_free (ProcessRefAddrArrays[IdxProcess].NameAddr[IdxVari].Name);
        }
        my_free (ProcessRefAddrArrays[IdxProcess].NameAddr);
        ProcessRefAddrArrays[IdxProcess].Pid = 0;
        ProcessRefAddrArrays[IdxProcess].LevelArray = 0;
        ProcessRefAddrArrays[IdxProcess].SizeOfArray = 0;
        ProcessRefAddrArrays[IdxProcess].NameAddr = NULL;
        Ret = 0;
    }
    LEAVE_CS (&ProcessRefAddrArrayCriticalSection);
    return Ret;
}

int SearchVariableInArrayByName (int ProcessIdx, const char *Variablename)
{
    int x;
    for (x = 0; x < ProcessRefAddrArrays[ProcessIdx].LevelArray; x++) {
        if (!CmpVariName (Variablename, ProcessRefAddrArrays[ProcessIdx].NameAddr[x].Name)) return x;
    }
    return -1;
}

int AddNewVariableToArray (int ProcessIdx, const char *Variablename, uint64_t Addr, int Type)
{
    NAME_ADDR *NameAddr;

    if (ProcessRefAddrArrays[ProcessIdx].LevelArray >= ProcessRefAddrArrays[ProcessIdx].SizeOfArray) {
        ProcessRefAddrArrays[ProcessIdx].SizeOfArray += 100;
        ProcessRefAddrArrays[ProcessIdx].NameAddr = (NAME_ADDR*)my_realloc (ProcessRefAddrArrays[ProcessIdx].NameAddr,
                                                                            sizeof (NAME_ADDR) * (size_t)ProcessRefAddrArrays[ProcessIdx].SizeOfArray);
        if (ProcessRefAddrArrays[ProcessIdx].NameAddr == NULL) {
            ThrowError (1, "cannot reference variable '%s; out of memory", Variablename);
            return -1;
        }
    }
    NameAddr = &ProcessRefAddrArrays[ProcessIdx].NameAddr[ProcessRefAddrArrays[ProcessIdx].LevelArray];
    NameAddr->Name = StringMalloc (Variablename);
    if (NameAddr->Name == NULL) {
        ThrowError (1, "cannot reference variable '%s; out of memory", Variablename);
        return -1;
    }
    NameAddr->Addr = Addr;
    NameAddr->Type = Type;
    ProcessRefAddrArrays[ProcessIdx].LevelArray++;
    return 0;
}


int GetVarAddrFromProcessRefList (int Pid, const char *Variablename, uint64_t *pAddr, int *pType)
{
    int IdxProcess, IdxVari;

    IdxProcess = SearchProcessInArray (Pid);
    if (IdxProcess < 0) return -1;  // Process not exist
    IdxVari = SearchVariableInArrayByName (IdxProcess, Variablename);
    if (IdxVari < 0) return -2;   // Variable not exist
    *pAddr = ProcessRefAddrArrays[IdxProcess].NameAddr[IdxVari].Addr;
    *pType = ProcessRefAddrArrays[IdxProcess].NameAddr[IdxVari].Type;
    return 0;
}

int AddVarToProcessRefList (int Pid, const char *Variablename, uint64_t Address, int Type)
{
    int IdxProcess;
    int Ret = -1;

    ENTER_CS (&ProcessRefAddrArrayCriticalSection);
    IdxProcess = CheckProcessInArray (Pid);
    if (IdxProcess < 0) {
        Ret = -1;  // Process not exist
    } else {
        if (SearchVariableInArrayByName (IdxProcess, Variablename) >= 0) {
            Ret = -2;   // Variable already exist
        } else {
            Ret = AddNewVariableToArray (IdxProcess, Variablename, Address, Type);
        }
    }
    LEAVE_CS (&ProcessRefAddrArrayCriticalSection);
    return Ret;
}

int DelVarFromProcessRefList (int Pid, const char *Variablename)
{
    int IdxProcess, IdxVari;
    int Ret = -1;

    ENTER_CS (&ProcessRefAddrArrayCriticalSection);
    IdxProcess = SearchProcessInArray (Pid);
    if (IdxProcess < 0) {
        Ret = -1;  // Process not exist
    } else {
        IdxVari = SearchVariableInArrayByName (IdxProcess, Variablename);
        if (IdxVari < 0) {
            return -2;   // Variable not exist
        } else {
            my_free (ProcessRefAddrArrays[IdxProcess].NameAddr[IdxVari].Name);
            IdxVari++;
            if (IdxVari < ProcessRefAddrArrays[IdxProcess].LevelArray) {
                memmove (&ProcessRefAddrArrays[IdxProcess].NameAddr[IdxVari-1],     // Destination
                        &ProcessRefAddrArrays[IdxProcess].NameAddr[IdxVari],   // Source
                        sizeof (NAME_ADDR) * (size_t)(ProcessRefAddrArrays[IdxProcess].LevelArray - IdxVari));
            }
            ProcessRefAddrArrays[IdxProcess].LevelArray--;
            Ret = 0;
        }
    }
    LEAVE_CS (&ProcessRefAddrArrayCriticalSection);
    return Ret;
}

int rereference_all_vari_from_ini (int pid, int IgnoreBasicSettings, int StartIdx)
{
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char pname[MAX_PATH];
    char entry[32];
    char variname[BBVARI_NAME_SIZE];
    char DisplayName[BBVARI_NAME_SIZE];
    int x;
    uint64_t address;
    int32_t typenr;
    int type;
    int DispayNameFlag;
    int WhatToDo = 0;
    PROCESS_APPL_DATA *pappldata = NULL;
    int UniqueId = -1;
    int Pid;
    char *p;
    int Fd = GetMainFileDescriptor();

    get_name_by_pid (pid, pname);
    if (GetProcessNameWithoutPath (pid, pname_withoutpath)) {
        return -1;
    }
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    if (IniFileDataBaseFindNextSectionNameRegExp(0, section_filter, 0, section, sizeof(section), Fd) < 0) {
        return -1;
    }

    // Basic Settings
    if (!IgnoreBasicSettings) {
        if (!s_main_ini_val.RememberReferencedLabels) {
            // Delete complete reference list from INI file
            IniFileDataBaseWriteString(section, NULL, NULL, Fd);
            return 0;
        }
    }

    if (WaitUntilProcessIsNotActiveAndThanLockIt (pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "cannot reference variable", __FILE__, __LINE__) == 0) {
        // Iterate through complete variable list from INI file
        for (x = StartIdx;;x++) {
            sprintf (entry, "Ref%i", x);
            if (IniFileDataBaseReadString(section, entry, "", variname, sizeof (variname), Fd) == 0) break;

            p = variname + strlen(variname);
            // Delete whitespaces from line end
            while (isascii(*(p-1)) && isspace(*(p-1)) && (p > variname)) p--;
            *p = 0;

            if (GetDisplayName (section, x, variname, DisplayName, sizeof (DisplayName)) == 0) {
                DispayNameFlag = 1;
            } else {
                DispayNameFlag = 0;
            }

            if (pappldata == NULL) {
                UniqueId = AquireUniqueNumber ();
                pappldata = ConnectToProcessDebugInfos (UniqueId,
                                                        pname, &Pid,
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        0);  // Whith error messages active
            }
            ConvertLabelAsapCombatible (variname, sizeof(variname), 2);   // ._. will always replaced with :: , appl_label cannot handle ._.
            if (appl_label_already_locked (pappldata, pid, variname, &address, &typenr, 1, 1)) {
                ConvertLabelAsapCombatible (variname, sizeof(variname), 0);   // replace :: wiht ._.  if necessary
                switch (WhatToDo) {
                case 0:
                    switch (ThrowError (ERROR_OK_OKALL_CANCEL_CANCELALL, "try to rerefrence unknown variable %s in process %s\n delete it ?", variname, pname)) {
                    case IDOK:
                        WhatToDo = 0;  // only delete this one
                        remove_referenced_vari_ini (pid, variname);
                        x--;
                        break;
                    case IDOKALL:
                        WhatToDo = 1;  // delete all
                        remove_referenced_vari_ini (pid, variname);
                        x--;
                        break;
                    case IDCANCEL:
                        WhatToDo = 0;  // jump over this one
                        break;
                    case IDCANCELALL:
                        WhatToDo = -1;  // jump over all
                        break;
                    }
                    break;
                case 1:
                    remove_referenced_vari_ini (pid, variname);
                    x--;
                    break;
                case -1:
                    break;
                }
            } else if ((type = get_base_type_bb_type_ex (typenr, pappldata)) < 0) {
                ConvertLabelAsapCombatible (variname, sizeof(variname), 0);   // replace :: with ._. if necessary
                ThrowError (1, "rerefrence variable %s with unknown type in process %s", variname, pname);
            } else {
                if (!scm_ref_vari_lock_flag (address, pid, (DispayNameFlag) ? DisplayName : variname, type, 0x3, 0)) {
                    AddVarToProcessRefList (pid, variname, address, type);
                }
            }
        }
        if (pappldata != NULL) {
            RemoveConnectFromProcessDebugInfos(UniqueId);
            FreeUniqueNumber(UniqueId);
        }
        UnLockProcess (pid);
    }
    return 0;   // OK
}


int unreference_all_vari_from_ini (int pid)
{
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char pname[MAX_PATH];
    char entry[32];
    char variname[BBVARI_NAME_SIZE];
    char DisplayName[BBVARI_NAME_SIZE];
    int x;
    uint64_t address;
    int type;
    int DispayNameFlag;
    int Fd = GetMainFileDescriptor();

    get_name_by_pid (pid, pname);
    if (GetProcessNameWithoutPath (pid, pname_withoutpath)) {
        return -1;
    }
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    if (IniFileDataBaseFindNextSectionNameRegExp (0, section_filter, 0, section, sizeof(section), Fd) < 0) {
        return -1;
    }
    if (WaitUntilProcessIsNotActiveAndThanLockIt (pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "cannot reference variable", __FILE__, __LINE__) == 0) {
        // Iterate through complete variable list from INI file
        for (x = 0;;x++) {
            sprintf (entry, "Ref%i", x);
            if (IniFileDataBaseReadString(section, entry, "", variname, sizeof (variname), Fd) == 0) break;
            if (GetDisplayName (section, x, variname, DisplayName, sizeof (DisplayName)) == 0) {
                DispayNameFlag = 1;
            } else {
                DispayNameFlag = 0;
            }

            ConvertLabelAsapCombatible (variname, sizeof(variname), 2);   // ._. will always replaced with :: , appl_label cannot handle ._.
            if (GetVarAddrFromProcessRefList (pid, variname, &address, &type)) {
                ConvertLabelAsapCombatible (variname, sizeof(variname), 0);   //  replace :: wiht ._.  if necessary
            } else {
                ConvertLabelAsapCombatible (variname, sizeof(variname), 0);   //  replace :: wiht ._.  if necessary
                if (!scm_unref_vari_lock_flag (address, (short)pid, (DispayNameFlag) ? DisplayName : variname, type, 0)) {
                    DelVarFromProcessRefList (pid, variname);
                }
            }
        }
        UnLockProcess (pid);
    }
    // Delete section
    IniFileDataBaseWriteString (section, NULL, NULL, Fd);
    DelProcessFromArray (pid);
    return 0;   // OK
}


int look_if_referenced (int pid, char *lname, char *DisplayName, int Maxc)
{
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char pname[MAX_PATH];
    char entry[32];
    char variname[BBVARI_NAME_SIZE];
    int x;
    int Fd = GetMainFileDescriptor();

    get_name_by_pid (pid, pname);
    if (GetProcessNameWithoutPath (pid, pname_withoutpath)) {
        return 0;
    }
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    if (IniFileDataBaseFindNextSectionNameRegExp (0, section_filter, 0, section, sizeof(section), Fd) < 0) {
        return 0;
    }
    // Search inside variable list
    for (x = 0;;x++) {
        sprintf (entry, "Ref%i", x);
        if (IniFileDataBaseReadString(section, entry, "", variname, sizeof (variname), Fd) == 0) break;
        if (!CmpVariName (variname, lname)) {
            if (GetDisplayName (section, x, variname, DisplayName, Maxc) == 0) {
                return 2;   // refernced and with display name
            } else {
                return 1;   // refernced
            }
        }
    }
    return 0;   // not referenced
}

int write_referenced_vari_ini (int pid, const char *lname, const char *dname)
{
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char pname[MAX_PATH];
    char entry[32];
    char variname[BBVARI_NAME_SIZE];
    char ref_dsp_name[2*BBVARI_NAME_SIZE+1];
    int x;
    int Fd = GetMainFileDescriptor();

    get_name_by_pid (pid, pname);
    if (GetProcessNameWithoutPath (pid, pname_withoutpath)) {
        return -1;
    }
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    if (IniFileDataBaseFindNextSectionNameRegExp (0, section_filter, 0, section, sizeof(section), Fd) < 0) {
        sprintf (section, "referenced variables %s", pname);
    }
    // Search inside variable list
    for (x = 0;;x++) {
        sprintf (entry, "Ref%i", x);
        if (IniFileDataBaseReadString(section, entry, "", variname, sizeof (variname), Fd) == 0) break;
        ConvertLabelAsapCombatible (variname, sizeof(variname), 0);   // replace :: wiht ._.  if necessary
        if (!CmpVariName (variname, lname)) return -1;   // Is already inside the list
    }
    IniFileDataBaseWriteString (section, entry, lname, Fd);
    if (CmpVariName (lname, dname)) {
        sprintf (entry, "Dsp%i", x);
        StringCopyMaxCharTruncate (ref_dsp_name, lname, sizeof(ref_dsp_name));
        strcat (ref_dsp_name, " ");
        strcat (ref_dsp_name, dname);
        IniFileDataBaseWriteString (section, entry, ref_dsp_name, Fd);
    } else {
        // Display name equal source name
        sprintf (entry, "Dsp%i", x);
        IniFileDataBaseWriteString (section, entry, NULL, Fd);
    }
    return 0;   // OK
}


int remove_referenced_vari_ini (int pid, const char *lname)
{
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char pname[MAX_PATH];
    char entry[32];
    char variname[BBVARI_NAME_SIZE];
    char DisplayName[2*BBVARI_NAME_SIZE+1];
    int x, found = 0;
    int Fd = GetMainFileDescriptor();

    get_name_by_pid (pid, pname);
    if (GetProcessNameWithoutPath (pid, pname_withoutpath)) {
        return -1;
    }
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    if (IniFileDataBaseFindNextSectionNameRegExp (0, section_filter, 0, section, sizeof(section), Fd) < 0) {
        return -1;
    }
    // Search inside variable list
    for (x = 0;;x++) {
        sprintf (entry, "Ref%i", x);
        if (IniFileDataBaseReadString(section, entry, "", variname, sizeof (variname), Fd) == 0) break;
        ConvertLabelAsapCombatible (variname, sizeof(variname), 0);   // replace :: wiht ._.  if necessary
        if (!CmpVariName (variname, lname)) {
            IniFileDataBaseWriteString(section, entry, NULL, Fd);
            sprintf (entry, "Dsp%i", x);
            IniFileDataBaseWriteString(section, entry, NULL, Fd);
            found++;
        } else if (found) {
            IniFileDataBaseWriteString (section, entry, NULL, Fd);
            sprintf (entry, "Ref%i", x - found);
            IniFileDataBaseWriteString (section, entry, variname, Fd);

            sprintf (entry, "Dsp%i", x);
            if (IniFileDataBaseReadString (section, entry, "", DisplayName, sizeof (DisplayName), Fd) > 0) {
                IniFileDataBaseWriteString (section, entry, NULL, Fd);
                sprintf (entry, "Dsp%i", x - found);
                IniFileDataBaseWriteString (section, entry, DisplayName, Fd);
            }
        }
    }
    return -1;   // Variable not found
}


static int CopyRefListAndConvertFormatOldToNew (int DstFile, int SrcFile, char *DstSection, char *SrcSection)
{
    char Entry[32];
    char Variname[BBVARI_NAME_SIZE];
    char DisplayName[BBVARI_NAME_SIZE];
    int x;

    IniFileDataBaseWriteString(DstSection, NULL, NULL, DstFile);
    for (x = 0;;x++) {
        sprintf (Entry, "Ref%i", x);
        if (IniFileDataBaseReadString(SrcSection, Entry, "", Variname, sizeof (Variname), SrcFile) == 0) break;
        if (GetDisplayName (SrcSection, x, Variname, DisplayName, sizeof (DisplayName)) != 0) {
            StringCopyMaxCharTruncate (DisplayName, Variname, sizeof(DisplayName));
        }
        IniFileDataBaseWriteString (DstSection, Variname, DisplayName, DstFile);
    }
    return 0;   // OK
}

static int AddRefListAndConvertFormatNewToOld (int DstFile, int SrcFile, char *DstSection, char *SrcSection, int DstStartIdx)
{
    char Entry[32];
    char Variname[BBVARI_NAME_SIZE];
    char DisplayName[BBVARI_NAME_SIZE];
    char Help[INI_MAX_LINE_LENGTH];
    int EntryIdx = 0;
    int RefIdx = DstStartIdx;

    if (DstStartIdx == 0) IniFileDataBaseWriteString(DstSection, NULL, NULL, DstFile);
    while ((EntryIdx = IniFileDataBaseGetNextEntryName (EntryIdx, SrcSection, Variname, sizeof(Variname), SrcFile)) >= 0) {
        sprintf (Entry, "Ref%i", RefIdx);
        IniFileDataBaseWriteString (DstSection, Entry, Variname, DstFile);
        IniFileDataBaseReadString (SrcSection, Variname, "", DisplayName, sizeof (DisplayName), SrcFile);
        // If Display name not equal variable name?
        if (strcmp (Variname, DisplayName)) {
            sprintf (Entry, "Dsp%i", RefIdx);
            sprintf (Help, "%s %s", Variname, DisplayName);
            IniFileDataBaseWriteString (DstSection, Entry, Help, DstFile);
        }
        RefIdx++;
    }
    return 0;   // OK
}


static int CopyRefListAndConvertFormatNewToOld (int DstFile, int SrcFile, char *DstSection, char *SrcSection)
{
    return AddRefListAndConvertFormatNewToOld (DstFile, SrcFile, DstSection, SrcSection, 0);
}


/* Export a reference list of an external process
   The process specific section "referenced variables <process name>"
   will be renamed into a process neutral section "referenced variables".
   In case of error the function will return a negative value otherwise 0.*/
int ExportVariableReferenceList (int pid, const char *FileName, int OutputFormat)
{
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char pname[MAX_PATH];
    int ret;
    int Fd;

    if (get_name_by_pid (pid, pname)) {
        return -1;
    }
    if (GetProcessNameWithoutPath (pid, pname_withoutpath)) {
        return -1;
    }
    Fd = IniFileDataBaseCreateAndOpenNewIniFile(FileName);
    if (Fd <= 0) {
        return -1;
    }
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    if (IniFileDataBaseFindNextSectionNameRegExp (0, section_filter, 0, section, sizeof(section), GetMainFileDescriptor()) < 0) {
        // Setup only an empty INI file
        ret = 0;
    } else {
        if (OutputFormat) {
            ret = CopyRefListAndConvertFormatOldToNew (Fd,  GetMainFileDescriptor(), "referenced variables for external process", section);
        } else {
            ret = IniFileDataBaseCopySection (Fd,  GetMainFileDescriptor(), "referenced variables", section);
        }
    }
    IniFileDataBaseSave(Fd, NULL, INIFILE_DATABAE_OPERATION_REMOVE);
    return ret;
}

/* Importiert a refeence list of an external process
   The process neutral section  "referenced variables" section will be renamed
   to a process specific section "referenced variables <process name>"
   In case of error the function will return a negative value otherwise 0. */
int ImportVariableReferenceList (int pid, const char *FileName)
{
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char pname[MAX_PATH];
    int ret = 0;
    int idx = 0;
    int Fd;

    get_name_by_pid (pid, pname);
    if (GetProcessNameWithoutPath (pid, pname_withoutpath)) {
        return -1;
    }
    Fd = IniFileDataBaseOpen(FileName);
    if (Fd <= 0) {
        return -1;
    }
    // Dereference all before referenced variables
    unreference_all_vari_from_ini (pid);
    // First delete all reference section with the process name inside
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    while ((idx = IniFileDataBaseFindNextSectionNameRegExp (idx, section_filter, 0, section, sizeof(section), GetMainFileDescriptor())) >= 0) {
        IniFileDataBaseWriteString(section, NULL, NULL, GetMainFileDescriptor());
    }
    sprintf (section, "referenced variables %s", pname);
    if (IniFileDataBaseLookIfSectionExist ("referenced variables for external process", Fd)) {
        ret = CopyRefListAndConvertFormatNewToOld (GetMainFileDescriptor(), Fd, section, "referenced variables for external process");
    } else {
        IniFileDataBaseCopySection (GetMainFileDescriptor(), Fd, section, "referenced variables");
    }
    IniFileDataBaseClose(Fd);
    rereference_all_vari_from_ini (pid, 1, 0);
    return ret;
}

int AddVariableReferenceList (int pid, char *FileName)
{
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char pname[MAX_PATH];
    char entry[32];
    char help[INI_MAX_LINE_LENGTH];
    int ret = 0;
    int x, end;
    int Fd;

    get_name_by_pid (pid, pname);
    if (GetProcessNameWithoutPath (pid, pname_withoutpath)) {
        return -1;
    }
    Fd = IniFileDataBaseOpen(FileName);
    if (Fd <= 0) {
        return -1;
    }
    // Search the matching reference section
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    if (IniFileDataBaseFindNextSectionNameRegExp(0, section_filter, 0, section, sizeof(section), GetMainFileDescriptor()) >= 0) {
    } else {
        sprintf (section, "referenced variables %s", pname);
    }
    // Go to the end of the reference list
    for (x = 0; ; x++) {
        sprintf (entry, "Ref%i", x);
        if (IniFileDataBaseReadString (section, entry, "", help, sizeof (help), GetMainFileDescriptor()) == 0) break;
    }
    end = x;

    // Add the new element
    if (IniFileDataBaseLookIfSectionExist ("referenced variables for external process", Fd)) {
        AddRefListAndConvertFormatNewToOld (GetMainFileDescriptor(), Fd, section, "referenced variables for external process", end);
    } else {
        for (x = 0; ; x++) {
            sprintf (entry, "Ref%i", x);
            if (IniFileDataBaseReadString ("referenced variables", entry, "", help, sizeof (help), Fd) == 0) break;
            sprintf (entry, "Ref%i", x+end);
            IniFileDataBaseWriteString (section, entry, help, GetMainFileDescriptor());
            sprintf (entry, "Dsp%i", x);
            // If a display name exist copy it also
            if (IniFileDataBaseReadString ("referenced variables", entry, "", help, sizeof (help), Fd) > 0) {
                sprintf (entry, "Dsp%i", x+end);
                IniFileDataBaseWriteString (section, entry, help, GetMainFileDescriptor());
            }
        }
    }
    IniFileDataBaseClose(Fd);
    rereference_all_vari_from_ini (pid, 1, end);
    return ret;
}

int GetDisplayName (char *Section, int Index, const char *ReferenceName, char *ret_DispayName, int Maxc)
{
    char Entry[32];
    char IniLine[INI_MAX_LINE_LENGTH];
    char *p, *pr, *pd;
    int c;

    sprintf (Entry, "Dsp%i", Index);
    if (IniFileDataBaseReadString (Section, Entry, "", IniLine, sizeof (IniLine), GetMainFileDescriptor()) != 0) {
        p = IniLine;
        while ((*p != 0) && isspace (*p)) p++;  // jump over whitespaces
        pr = p;
        while ((*p != 0) && !isspace (*p)) p++;
        if (!strncmp (pr, ReferenceName, (size_t)(p - pr))) {   // Reference name is correct
            while ((*p != 0) && isspace (*p)) p++;  // jump over whitespaces
            pd = ret_DispayName;
            c = 1;
            while ((*p != 0) && !isspace (*p) && (c < Maxc)) {
                *pd++ = *p++;
                c++;
            }
            *pd = 0;
            return 0;
        } else return -2;
    } else return -1;
}

int GetDisplayNameByLabelName (int pid, const char *lname, char *ret_DisplayName, int Maxc)
{
    char pname_withoutpath[MAX_PATH];
    char section[INI_MAX_SECTION_LENGTH];
    char section_filter[INI_MAX_SECTION_LENGTH];
    char pname[MAX_PATH];
    char entry[32];
    char variname[BBVARI_NAME_SIZE];
    int x;

    get_name_by_pid (pid, pname);
    if (GetProcessNameWithoutPath (pid, pname_withoutpath)) {
        return -1;
    }
    sprintf (section_filter, "referenced variables *%s", pname_withoutpath);
    if (IniFileDataBaseFindNextSectionNameRegExp(0, section_filter, 0, section, sizeof(section), GetMainFileDescriptor()) < 0) {
        return -1;
    }
    // Search inside variabale list
    for (x = 0;;x++) {
        sprintf (entry, "Ref%i", x);
        if (IniFileDataBaseReadString(section, entry, "", variname, sizeof (variname), GetMainFileDescriptor()) == 0) break;
        ConvertLabelAsapCombatible (variname, sizeof(variname), 0);   // replace :: wiht ._.  if necessary
        if (!CmpVariName (variname, lname)) {
            return GetDisplayName (section, x, lname, ret_DisplayName, Maxc);
        }
    }
    return -1;   // variable not found
}

static int IsNotANumber (double v)
{
    return isnan (v);
}

int ReferenceOneSymbol(const char* Symbol, const char *DisplayName, const char* Process, const char *Unit, int ConversionType, const char *Conversion,
                       double Min, double Max, int Color, int Width, int Precision, int Flags)
{
    char Variable[512];
    const char *Name;
    int Pid;
    uint64_t Address;
    int TypeNo;
    int Type;
    PROCESS_APPL_DATA *pappldata;
    int Dir = 0;;
    int Ret = -1;
    int UniqueId = AquireUniqueNumber ();
    pappldata = ConnectToProcessDebugInfos (UniqueId,
                                            Process, &Pid,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0);
    if (pappldata != NULL) {
        if (WaitUntilProcessIsNotActiveAndThanLockIt (Pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "cannot reference variable", __FILE__, __LINE__) == 0) {
            memset(Variable, 0, sizeof(Variable));
            strncpy (Variable, Symbol, sizeof(Variable)-1);
            ConvertLabelAsapCombatible (Variable, sizeof(Variable), 2);   // ._. will always replaced with :: , appl_label cannot handle ._.
            if (appl_label_already_locked (pappldata, Pid, Variable, &Address, &TypeNo, 1, 1)) {
                if ((Flags & 0x10000) != 0x10000) {
                    ConvertLabelAsapCombatible (Variable, sizeof(Variable), 0);   // replace :: wiht ._.  if necessary
                    ThrowError (1, "try to refrence unknown variable %s in process %s", Variable, Process);
                }
            } else if ((Type = get_base_type_bb_type_ex (TypeNo, pappldata)) < 0) {
                if ((Flags & 0x10000) != 0x10000) {
                    ConvertLabelAsapCombatible (Variable, sizeof(Variable), 0);   // replace :: wiht ._.  if necessary
                    ThrowError (1, "refrence variable %s with unknown type in process %s", Variable, Process);
                }
            } else {
                if ((DisplayName == NULL) || (DisplayName[0] == 0)) {
                    Name = Symbol;
                } else {
                    Name = DisplayName;
                }
                if ((Flags & 0x1) == 0x1) Dir |= PIPE_API_REFERENCE_VARIABLE_DIR_READ;
                if ((Flags & 0x2) == 0x2) Dir |= PIPE_API_REFERENCE_VARIABLE_DIR_WRITE;

                if (AddVarToProcessRefList (Pid, Variable, Address, Type) == 0) {
                    if (scm_ref_vari_lock_flag (Address, Pid, Name, Type, Dir, 0) == 0) {
                        if ((Flags & 0x4) == 0x4) write_referenced_vari_ini (Pid, Symbol ,Name);
                        int Vid = get_bbvarivid_by_name(Name);
                        if (Vid > 0) {
                            if (Unit[0] > 0) set_bbvari_unit(Vid, Unit);
                            if (!IsNotANumber(Min)) set_bbvari_min(Vid, Min);
                            if (!IsNotANumber(Max)) set_bbvari_max(Vid, Max);
                            if ((Width != 255) && (Precision != 255)) set_bbvari_format(Vid, Width, Precision);
                            if (ConversionType != 255) set_bbvari_conversion(Vid, ConversionType, Conversion);
                            if (Color <= 0xFFFFFF) set_bbvari_color(Vid, Color);
                            Ret = Vid;
                        }
                    } else {
                        DelVarFromProcessRefList(Pid, Variable);
                    }
                } else {
                    Ret = VARIABLE_ALREADY_REFERENCED;
                }
            }
            UnLockProcess (Pid);
        }
    }
    if (pappldata != NULL) {
        RemoveConnectFromProcessDebugInfos(UniqueId);
        FreeUniqueNumber(UniqueId);
    }
    return Ret;
}

int DereferenceOneSymbol(const char* Symbol, const char* Process, int Flags)
{
    char Variable[512];
    int Pid;
    uint64_t Address;
    int TypeNo;
    int Type;
    PROCESS_APPL_DATA *pappldata;
    int Ret = -1;
    int UniqueId = AquireUniqueNumber ();
    pappldata = ConnectToProcessDebugInfos (UniqueId,
                                            Process, &Pid,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0);
    if (pappldata != NULL) {
        if (WaitUntilProcessIsNotActiveAndThanLockIt (Pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "cannot reference variable", __FILE__, __LINE__) == 0) {
            memset(Variable, 0, sizeof(Variable));
            strncpy (Variable, Symbol, sizeof(Variable)-1);
            ConvertLabelAsapCombatible (Variable, sizeof(Variable), 2);   // ._. will always replaced with :: , appl_label cannot handle ._.
            if (appl_label_already_locked (pappldata, Pid, Variable, &Address, &TypeNo, 1, 1)) {
                if ((Flags & 0x10000) != 0x10000) {
                    ConvertLabelAsapCombatible (Variable, sizeof(Variable), 0);   // replace :: wiht ._.  if necessary
                    ThrowError (1, "try to derefrence unknown variable %s in process %s", Variable, Process);
                }
            } else if ((Type = get_base_type_bb_type_ex (TypeNo, pappldata)) < 0) {
                if ((Flags & 0x10000) != 0x10000) {
                    ConvertLabelAsapCombatible (Variable, sizeof(Variable), 0);   // replace :: wiht ._.  if necessary
                    ThrowError (1, "derefrence variable %s with unknown type in process %s", Variable, Process);
                }
            } else {
                if (!scm_unref_vari_lock_flag (Address, Pid, Variable, Type, 0)) {
                    Ret = DelVarFromProcessRefList(Pid, Variable);
                    if ((Flags & 0x4) == 0x4) Ret |= remove_referenced_vari_ini (Pid, Variable);

                }
            }
            UnLockProcess (Pid);
        }
    }
    if (pappldata != NULL) {
        RemoveConnectFromProcessDebugInfos(UniqueId);
        FreeUniqueNumber(UniqueId);
    }
    return Ret;
}

enum BB_DATA_TYPES GetRawValueOfOneSymbol(const char* Symbol, const char* Process, int Flags, union BB_VARI *ret_Value)
{
    char Variable[512];
    int Pid;
    uint64_t Address;
    int TypeNo;
    int Type;
    PROCESS_APPL_DATA *pappldata;
    int Ret = -1;
    int UniqueId = AquireUniqueNumber ();
    pappldata = ConnectToProcessDebugInfos (UniqueId,
                                            Process, &Pid,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0);
    if (pappldata != NULL) {
        if (WaitUntilProcessIsNotActiveAndThanLockIt (Pid, 5000, ERROR_BEHAVIOR_NO_ERROR_MESSAGE, "", __FILE__, __LINE__) == 0) {
            memset(Variable, 0, sizeof(Variable));
            strncpy (Variable, Symbol, sizeof(Variable)-1);
            ConvertLabelAsapCombatible (Variable, sizeof(Variable), 2);   // ._. will always replaced with :: , appl_label cannot handle ._.
            if (appl_label_already_locked (pappldata, Pid, Variable, &Address, &TypeNo, 1, 1)) {
                if ((Flags & 0x1000) != 0x1000) {
                    ConvertLabelAsapCombatible (Variable, sizeof(Variable), 0);   // replace :: wiht ._.  if necessary
                    ThrowError (1, "try to read from unknown variable %s in process %s", Variable, Process);
                }
                Ret = UNKNOWN_VARIABLE;
            } else if ((Type = get_base_type_bb_type_ex (TypeNo, pappldata)) < 0) {
                if ((Flags & 0x1000) != 0x1000) {
                    ConvertLabelAsapCombatible (Variable, sizeof(Variable), 0);   // replace :: wiht ._.  if necessary
                    ThrowError (1, "try to read from variable %s with unknown type in process %s", Variable, Process);
                }
                Ret = UNKNOWN_DTYPE;
            } else {
                int Size = GetDataTypeByteSize(Type);
                if ((Size > 0) && (Size <= 8)) {
                    if (scm_read_bytes (Address, Pid, (char*)ret_Value, Size) == Size) {
                        Ret = Type;
                    } else {
                        Ret = ADDRESS_NO_VALID_LABEL;
                    }
                } else {
                    Ret = UNKNOWN_DTYPE;
                }
            }
            UnLockProcess (Pid);
        }
    } else {
        if ((Flags & 0x1000) != 0x1000) {
            ConvertLabelAsapCombatible (Variable, sizeof(Variable), 0);   // replace :: wiht ._.  if necessary
            ThrowError (1, "try to read from variable %s from unknown process %s", Variable, Process);
        }
        Ret = UNKNOWN_PROCESS;
    }
    if (pappldata != NULL) {
        RemoveConnectFromProcessDebugInfos(UniqueId);
        FreeUniqueNumber(UniqueId);
    }
    return Ret;
}

int SetRawValueOfOneSymbol(const char* Symbol, const char* Process, int Flags, enum BB_DATA_TYPES DataType, union BB_VARI Value)
{
    char Variable[512];
    int Pid;
    uint64_t Address;
    int TypeNo;
    int Type;
    PROCESS_APPL_DATA *pappldata;
    int Ret = -1;
    int UniqueId = AquireUniqueNumber ();
    pappldata = ConnectToProcessDebugInfos (UniqueId,
                                            Process, &Pid,
                                            NULL,
                                            NULL,
                                            NULL,
                                            0);
    if (pappldata != NULL) {
        if (WaitUntilProcessIsNotActiveAndThanLockIt (Pid, 5000, ERROR_BEHAVIOR_NO_ERROR_MESSAGE, "", __FILE__, __LINE__) == 0) {
            memset(Variable, 0, sizeof(Variable));
            strncpy (Variable, Symbol, sizeof(Variable)-1);
            ConvertLabelAsapCombatible (Variable, sizeof(Variable), 2);   // ._. will always replaced with :: , appl_label cannot handle ._.
            if (appl_label_already_locked (pappldata, Pid, Variable, &Address, &TypeNo, 1, 1)) {
                if ((Flags & 0x1000) != 0x1000) {
                    ConvertLabelAsapCombatible (Variable, sizeof(Variable), 0);   // replace :: wiht ._.  if necessary
                    ThrowError (1, "try to write to unknown variable %s in process %s", Variable, Process);
                }
                Ret = UNKNOWN_VARIABLE;
            } else if ((Type = get_base_type_bb_type_ex (TypeNo, pappldata)) < 0) {
                if ((Flags & 0x1000) != 0x1000) {
                    ConvertLabelAsapCombatible (Variable, sizeof(Variable), 0);   // replace :: wiht ._.  if necessary
                    ThrowError (1, "try to write to variable %s with unknown type in process %s", Variable, Process);
                }
            } else {
                union BB_VARI ValueConverted;
                if (sc_convert_from_to_0(DataType, &Value, Type, &ValueConverted) > 0) {
                    int Size = GetDataTypeByteSize(Type);
                    if ((Size > 0) && (Size <= 8)) {
                        if (scm_write_bytes (Address, Pid, (unsigned char*)&ValueConverted, Size) == Size) {
                            Ret = 0;   // all OK
                        } else {
                            Ret = ADDRESS_NO_VALID_LABEL;
                        }
                    } else {
                        Ret = UNKNOWN_DTYPE;
                    }
                } else {
                    if ((Flags & 0x1000) != 0x1000) {
                        ConvertLabelAsapCombatible (Variable, sizeof(Variable), 0);   // replace :: wiht ._.  if necessary
                        ThrowError (1, "try to write to variable %s to unknown data type %i", Variable, DataType);
                    }
                    Ret = UNKNOWN_DTYPE;
                }
            }
            UnLockProcess (Pid);
        } else {
            if ((Flags & 0x1000) != 0x1000) {
                ConvertLabelAsapCombatible (Variable, sizeof(Variable), 0);   // replace :: wiht ._.  if necessary
                ThrowError (1, "try to write to variable %s to unknown process %s", Variable, Process);
            }
            Ret = UNKNOWN_PROCESS;
        }
    }
    if (pappldata != NULL) {
        RemoveConnectFromProcessDebugInfos(UniqueId);
        FreeUniqueNumber(UniqueId);
    }
    return Ret;
}
