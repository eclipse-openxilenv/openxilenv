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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "MyMemory.h"
#include "PrintFormatToString.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "IniDataBase.h"
#include "Scheduler.h"
#include "Blackboard.h"
#include "A2LBuffer.h"
#include "A2LParser.h"
#include "A2LTokenizer.h"
#include "A2LAccess.h"
#include "A2LAccessData.h"
#include "A2LUpdate.h"
#ifndef NO_GUI
#include "A2LCalWidgetSync.h"
#endif
#include "A2LLink.h"

#define UNUSED(x) (void)(x)

#define LINK_NR_OFFSET 0xA200
#define LINK_NR_MASK 0xFF
#define MAX_A2L_LINKS  32
static A2L_LINK Links[MAX_A2L_LINKS];
static int LinkNrCount;

static CRITICAL_SECTION CriticalSection;

#if 1
#define InitializeCriticalSectionX(x) InitializeCriticalSection(&Links[x].CriticalSection)
#define EnterCriticalSectionX(x) EnterCriticalSection(&Links[x].CriticalSection)
#define LeaveCriticalSectionX(x) LeaveCriticalSection(&Links[x].CriticalSection)
#else
#define InitializeCriticalSectionX(x)
#define EnterCriticalSectionX(x)
#define LeaveCriticalSectionX(x)
#endif

void A2LLinkInitCriticalSection (void)
{
    InitializeCriticalSection (&CriticalSection);
    LinkNrCount = LINK_NR_OFFSET;
}

int GetBaseAddressOf(int par_Pid, int par_Flags, uint64_t *ret_BaseAddress)
{
    if ((par_Flags & (A2L_LINK_ADDRESS_TRANSLATION_DLL_FLAG | A2L_LINK_ADDRESS_TRANSLATION_MULTI_DLL_FLAG)) != 0x0) {
        char DllName[MAX_PATH];
        char PocessName[MAX_PATH];

        if (get_name_by_pid(par_Pid, PocessName, sizeof(PocessName))) {
            return -1;
        }
        if (!IsExternProcessLinuxExecutable(par_Pid) &&     // This will work only for 32bit windows executable
            lives_process_inside_dll(PocessName, DllName, sizeof(DllName)) > 0) {
            uint64_t ModuleBaseAddress;
            int Index;
            if (GetExternProcessBaseAddress(par_Pid, &ModuleBaseAddress, DllName) != 0) {
                ThrowError (1, "cannot get base address of process \"%s\" useing DLL \"%s\"", PocessName, DllName);
                return -1;
            }
            if ((par_Flags & A2L_LINK_ADDRESS_TRANSLATION_MULTI_DLL_FLAG) != 0x0) {
                Index = GetExternProcessIndexInsideExecutable (par_Pid);
                if (Index < 0) {
                    ThrowError (1, "cannot get index of process \"%s\" inside executable", PocessName);
                    return -1;
                }
                *ret_BaseAddress = (int64_t)0x80000000 + (int64_t)(Index << 28) - (int64_t)ModuleBaseAddress;
            } else if ((par_Flags & A2L_LINK_ADDRESS_TRANSLATION_DLL_FLAG) != 0x0) {
                *ret_BaseAddress = ModuleBaseAddress;
            }
        }
    }
    return 0;
}

static void FreeLink(int par_Idx)
{
    Links[par_Idx].Type = LINK_TYPE_EMPTY;
    FreeAsap2Database(Links[par_Idx].Database);
    my_free(Links[par_Idx].FileName);
    Links[par_Idx].LoadedFlag = 0;
}

int A2LLinkToExternProcess(const char *par_A2LFileName, int par_Pid, int par_Flags)
{
    int x;
    int Ret = -1;
    int LinkExists = 0;
    uint64_t BaseAddress = 0;

    if (GetBaseAddressOf(par_Pid, par_Flags, &BaseAddress)) {
        return -1;
    }

    // check if some links marked as should be removed can be freed
    for (x = 0; x < MAX_A2L_LINKS; x++) {
        if (Links[x].InUseFlag) {
            int Freed = 0;
            EnterCriticalSectionX(x);
            if (Links[x].ShouldBeRemoved) {
                if (Links[x].InUseCounter <= 0) {
                    FreeLink(x);
                    Freed = 1;
                }
            }
            LeaveCriticalSectionX(x);
            if (Freed) {
                EnterCriticalSection(&CriticalSection);
                Links[x].InUseFlag = 0;
                LeaveCriticalSection(&CriticalSection);
            }
        }
    }

    EnterCriticalSection(&CriticalSection);
    // check if a link exists?
    for (x = 0; x < MAX_A2L_LINKS; x++) {
        if (Links[x].InUseFlag) {
            if (Links[x].To.Pid == par_Pid) {
                LinkExists = 1;
            }
        }
    }

    for (x = 0; x < MAX_A2L_LINKS; x++) {
        if (!Links[x].InUseFlag) {
            Links[x].InUseFlag = 1;
            Links[x].InUseCounter = 0;
            Links[x].LoadedFlag = 0;
            Links[x].Type = LINK_TYPE_EMPTY;
            if (!Links[x].CriticalSectionIsInit) {
                InitializeCriticalSectionX(x);
                Links[x].CriticalSectionIsInit = 1;
            }
            break;  // for(;;)
        }
    }
    LeaveCriticalSection(&CriticalSection);

    if (x < MAX_A2L_LINKS) {
#define MAX_ERR_STRING_LEN (8*1024)
        char *ErrString = my_malloc(MAX_ERR_STRING_LEN);
        int LineNr;
        StringCopyMaxCharTruncate (ErrString, "during loading file: ", MAX_ERR_STRING_LEN);
        const char *s = par_A2LFileName;
        char *p = ErrString + strlen(ErrString);
        while (*s != 0) {
            if (*s == '%') *p++ = '%';
             *p++ = *s++;
        }
        *p++ = ':';
        *p++ = ' ';
        ASAP2_DATABASE *Database = LoadAsapFile(par_A2LFileName, 0, p, MAX_ERR_STRING_LEN - (int)(p - ErrString) , &LineNr);
        if (Database != NULL) {
            if ((par_Flags & A2L_LINK_UPDATE_FLAG) == A2L_LINK_UPDATE_FLAG) {
                char ProcessName[MAX_PATH];
                if (get_name_by_pid(par_Pid, ProcessName, sizeof(ProcessName)) == 0) {
                    const char *Error;
                    if (A2LUpdate(Database, NULL, "PROCESS", par_Flags, ProcessName, 0, 0, NULL, &Error)) {
                        ThrowError (1, "Updating \"%s\" with addresses of \"%s\" (%s)", par_A2LFileName, ProcessName, Error);
                    }
                }
            }
            EnterCriticalSectionX(x);
            Links[x].LinkNr = (LinkNrCount << 8) | x;
            LinkNrCount++;
            Links[x].Type = LINK_TYPE_EXT_PROCESS;
            Links[x].To.Pid = par_Pid;
            Links[x].Database = Database;
            Links[x].FileName = StringMalloc(par_A2LFileName);
            Links[x].BaseAddress = BaseAddress;
            Links[x].Flags = par_Flags;
            Links[x].LoadedFlag = 1;
            LeaveCriticalSectionX(x);
            Ret = Links[x].LinkNr;
            // rereference symbols
            if ((par_Flags & A2L_LINK_REMEMBER_REFERENCED_LABELS_FLAG) == A2L_LINK_REMEMBER_REFERENCED_LABELS_FLAG) {
                char ProcessShortName[MAX_PATH];
                if (GetProcessShortName(par_Pid, ProcessShortName, sizeof(ProcessShortName)) == 0) {
                    A2LLoadAllReferencesForProcess(Ret, ProcessShortName);
                }
            }
        } else {
            Links[x].InUseFlag = 0;
            ThrowError(1, ErrString);
        }
        my_free(ErrString);
    }
#ifndef NO_GUI
    if ((Ret > 0) && (LinkExists == 0)) {
        // tells all open A2LCalMapWidget that a process is started with an assigned A2L file.
        char ShortProcessName[MAX_PATH];
        if (GetProcessShortName(par_Pid, ShortProcessName, sizeof(ShortProcessName)) == 0) {
            GlobalNotifiyA2LCalMapWidgetFuncSchedThread(ShortProcessName, 0, 1);
        }
    }
#endif
    return Ret;
}


int A2LSetupLinkToExternProcess(const char *par_A2LFileName, const char *par_ProcessName, int par_UpdateFlag)
{
    int Pid;
    Pid = get_pid_by_name(par_ProcessName);
    if (Pid > 0) {
        int LinkNr = A2LLinkToExternProcess(par_A2LFileName, Pid, par_UpdateFlag);
        if (LinkNr > 0) {
            TASK_CONTROL_BLOCK *Tcb = get_process_info(Pid);
            if (Tcb != NULL) {
                Tcb->A2LLinkNr = LinkNr;
                return 0;
            } else {
                A2LCloseLink(LinkNr, par_ProcessName);
            }
        }
    }
    return -1;
}

int A2LGetLinkToExternProcess(const char *par_ProcessName)
{
    int x, Pid;
    Pid = get_pid_by_name(par_ProcessName);
    if (Pid > 0) {
        EnterCriticalSection(&CriticalSection);
        for (x = 0; x < MAX_A2L_LINKS; x++) {
            if (Links[x].InUseFlag) {
                if (Links[x].Type == LINK_TYPE_EXT_PROCESS) {
                    if (Links[x].To.Pid == Pid) {
                        LeaveCriticalSection(&CriticalSection);
                        return Links[x].LinkNr;
                    }
                }
            }
        }
        LeaveCriticalSection(&CriticalSection);
    }
    return -1;
}

int A2LCloseLink(int par_LinkNr, const char *par_ProcessName)
{
    int Idx = par_LinkNr & LINK_NR_MASK;
    if ((Idx < 0) || (Idx >= MAX_A2L_LINKS) || !Links[Idx].LoadedFlag || (Links[Idx].LinkNr != par_LinkNr)) return -1;

#ifndef NO_GUI
    // tells all open A2LCalMapWidget that a process is terminated with an assigned A2L file.
    GlobalNotifiyA2LCalMapWidgetFuncSchedThread(par_ProcessName, 0, 0);
#endif
    int Pid = get_pid_by_name(par_ProcessName);
    // store reference symbols
    if ((Links[Idx].Flags & A2L_LINK_REMEMBER_REFERENCED_LABELS_FLAG) == A2L_LINK_REMEMBER_REFERENCED_LABELS_FLAG) {
        char ShortProcessName[MAX_PATH];
        if (Pid > 0) {
            GetProcessShortName(Pid, ShortProcessName, sizeof(ShortProcessName));
            A2LStoreAllReferencesForProcess(par_LinkNr, ShortProcessName);
        }
    }
    DisconnectA2LFromProcess(Pid);
    int Freed;
    EnterCriticalSectionX(Idx);
    if (Links[Idx].InUseCounter <= 0) {
        FreeLink(Idx);
        Freed = 1;
    } else {
        Links[Idx].ShouldBeRemoved = 1;
        Freed = 0;
    }
    LeaveCriticalSectionX(Idx);
    if (Freed) {
        EnterCriticalSection(&CriticalSection);
        Links[Idx].InUseFlag = 0;
        LeaveCriticalSection(&CriticalSection);
    }
    return 0;
}


int A2LSetupLinkToS19File(const char *par_A2LFileName, const char *par_S19FileName)
{
    UNUSED(par_A2LFileName);
    UNUSED(par_S19FileName);
    return 0;
}

void *A2LGetDataFromLinkState(int par_LinkNr, int par_Index, void *par_Reuse, int par_Flags,
                              int par_State, int par_DimGroupNo, int par_DataGroupNo, const char **ret_Error)

{
    void *Ret = NULL;

    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            if (par_Index >= 0x60000000) {
                Ret = GetAxisData(&Links[LinkIdx], Links[LinkIdx].Database, (A2L_DATA*)par_Reuse, 0, -1, par_Index - 0x60000000, par_Flags,
                                  par_State, par_DimGroupNo, par_DataGroupNo, ret_Error, 1);
            } else if (par_Index >= 0x40000000) {
                Ret = GetCharacteristicData(&Links[LinkIdx], Links[LinkIdx].Database, (A2L_DATA*)par_Reuse, par_Index - 0x40000000, par_Flags,
                                            par_State, par_DimGroupNo, par_DataGroupNo, ret_Error);
            } else if (par_Index >= 0) {
                Ret = GetMeasurementData(&Links[LinkIdx], Links[LinkIdx].Database, (A2L_DATA*)par_Reuse, par_Index, par_Flags,
                                         par_State, par_DataGroupNo, ret_Error);
            }
            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        }
        LeaveCriticalSectionX(LinkIdx);
    }
    return Ret;
}

int A2LSetDataToLinkState(int par_LinkNr, int par_Index, void *par_Data,
                          int par_State, int par_DataGroupMaskNo, int par_DataGroupNo, const char **ret_Error)
{
    int Ret;
    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            if (par_Index >= 0x60000000) {
                Ret = SetAxisData(&Links[LinkIdx], Links[LinkIdx].Database, (A2L_DATA*)par_Data, 0, par_Index - 0x60000000,
                                  par_State, par_DataGroupMaskNo, par_DataGroupNo, ret_Error);
            } else if (par_Index >= 0x40000000) {
                Ret = SetCharacteristicData(&Links[LinkIdx], Links[LinkIdx].Database, (A2L_DATA*)par_Data, par_Index - 0x40000000,
                                            par_State, par_DataGroupMaskNo, par_DataGroupNo, ret_Error);
            } else {
                Ret = -1;
                if (ret_Error != NULL) *ret_Error = "cannot set measurements";
            }
            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        } else {
            Ret = -1;
            if (ret_Error != NULL) *ret_Error = "wrong index";
        }
        LeaveCriticalSectionX(LinkIdx);
    } else {
        Ret = -1;
        if (ret_Error != NULL) *ret_Error = "wrong index";
    }
    return Ret;
}


int A2LGetIndexFromLink(int par_LinkNr, const char *par_Label, int par_TypeMask)
{
    int Idx = -1;
    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            if ((par_TypeMask & A2L_LABEL_TYPE_MEASUREMENT) != 0) {
                Idx = GetMeasurementIndex(Links[LinkIdx].Database, par_Label);
            }
            if ((Idx < 0) && ((par_TypeMask & A2L_LABEL_TYPE_CALIBRATION) != 0)) {
                Idx = GetCharacteristicIndex(Links[LinkIdx].Database, par_Label);
                if (Idx >= 0) Idx |= 0x40000000;
            }
            if ((Idx < 0) && ((par_TypeMask & A2L_LABEL_TYPE_AXIS_CALIBRATION) != 0)) {
                Idx = GetCharacteristicAxisIndex(Links[LinkIdx].Database, par_Label);
                if (Idx >= 0) Idx |= 0x60000000;
            }
            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        }
        LeaveCriticalSectionX(LinkIdx);
    }
    return Idx;
}

int A2LGetNextSymbolFromLink(int par_LinkNr, int par_Index, int par_TypeMask, const char *par_Filter, char *ret_Name, int par_MaxChar)
{
    int Idx = -1;
    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            if ((par_Index < 0x40000000) &&
                ((par_TypeMask & A2L_LABEL_TYPE_MEASUREMENT) != 0)) {
                unsigned int Flags = par_TypeMask & 0xFF;
                Idx = GetNextMeasurement(Links[LinkIdx].Database, par_Index, par_Filter, Flags, ret_Name, par_MaxChar);
                if (Idx < 0) {
                    par_Index = -1;  // switch to characteristics
                }
            }
            if (Idx < 0) {  // Characteristics
                if ((par_Index < 0x60000000) &&
                    ((par_TypeMask & A2L_LABEL_TYPE_CALIBRATION) != 0)) {
                    unsigned int Flags = par_TypeMask & 0xFF00;
                    if (par_Index >= 0x40000000) par_Index -= 0x40000000;
                    Idx = GetNextCharacteristic(Links[LinkIdx].Database, par_Index, par_Filter, Flags, ret_Name, par_MaxChar);
                    if (Idx >= 0) {
                        Idx |= 0x40000000;
                    } else {
                        par_Index = -1;  // switch to axis
                    }
                }
            }
            if (Idx < 0) {  // Axis
                if ((par_TypeMask & A2L_LABEL_TYPE_AXIS_CALIBRATION) != 0) {
                    unsigned int Flags = par_TypeMask & 0x10000;
                    if (par_Index >= 0x60000000) par_Index -= 0x60000000;
                    Idx = GetNextCharacteristicAxis(Links[LinkIdx].Database, par_Index, par_Filter, Flags, ret_Name, par_MaxChar);
                    if (Idx >= 0) {
                        Idx |= 0x60000000;
                    }
                }
            }
            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        }
        LeaveCriticalSectionX(LinkIdx);
    }
    return Idx;
}

int A2LGetNextFunktionFromLink(int par_LinkNr, int par_Index, const char *par_Filter, char *ret_Name, int par_MaxChar)
{
    int Idx = -1;
    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            Idx = GetNextFunction(Links[LinkIdx].Database, par_Index, par_Filter, ret_Name, par_MaxChar);
            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        }
        LeaveCriticalSectionX(LinkIdx);
    }
    return Idx;
}


int A2LIncDecGetReferenceToDataFromLink(int par_LinkNr, int par_Index, int par_Offset, int par_DirFlags, int par_Vid)
{
    int Ret = -1;
    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            ASAP2_PARSER* Parser = (ASAP2_PARSER*)Links[LinkIdx].Database->Asap2Ptr;
            ASAP2_MODULE_DATA* Module = Parser->Data.Modules[Links[LinkIdx].Database->SelectedModul];
            if (par_Index < 0x40000000) {
                if (par_Index < Module->MeasurementCounter) {  // measurements
                    if ((Module->Measurements[par_Index]->ReferenceCounter + par_Offset) < 0) {
                        Module->Measurements[par_Index]->ReferenceCounter = 0;
                    } else {
                        Module->Measurements[par_Index]->ReferenceCounter += par_Offset;
                    }
                    Ret = Module->Measurements[par_Index]->ReferenceCounter;
                    Module->Measurements[par_Index]->DirFlags = par_DirFlags;
                    if (par_Vid > 0) {
                        Module->Measurements[par_Index]->Vid = par_Vid;
                    }
                }
            } else if (par_Index < 0x60000000) {
                par_Index -= 0x40000000;
                if (par_Index < Module->MeasurementCounter) {  // characteristics
                    if ((Module->Characteristics[par_Index]->ReferenceCounter + par_Offset) < 0) {
                        Module->Characteristics[par_Index]->ReferenceCounter = 0;
                    } else {
                        Module->Characteristics[par_Index]->ReferenceCounter += par_Offset;
                    }
                    Ret = Module->Characteristics[par_Index]->ReferenceCounter;
                    Module->Characteristics[par_Index]->DirFlags = par_DirFlags;
                    if (par_Vid > 0) {
                        Module->Characteristics[par_Index]->Vid = par_Vid;
                    }
                }
            } else {
                par_Index -= 0x60000000;
                if (par_Index < Module->MeasurementCounter) {  // characteristics axis
                    if ((Module->AxisPtss[par_Index]->ReferenceCounter + par_Offset) < 0) {
                        Module->AxisPtss[par_Index]->ReferenceCounter = 0;
                    } else {
                        Module->AxisPtss[par_Index]->ReferenceCounter += par_Offset;
                    }
                    Ret = Module->AxisPtss[par_Index]->ReferenceCounter;
                    Module->AxisPtss[par_Index]->DirFlags = par_DirFlags;
                    if (par_Vid > 0) {
                        Module->AxisPtss[par_Index]->Vid = par_Vid;
                    }
                }
            }
            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        }
        LeaveCriticalSectionX(LinkIdx);
    }
    return Ret;
}

int A2LGetReadWritablFlagsFromLink(int par_LinkNr, int par_Index, int par_UserDefFlag)
{
    int Ret = 0x2;
    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            ASAP2_PARSER* Parser = (ASAP2_PARSER*)Links[LinkIdx].Database->Asap2Ptr;
            ASAP2_MODULE_DATA* Module = Parser->Data.Modules[Links[LinkIdx].Database->SelectedModul];
            if (par_Index >= 0x40000000) {
                int Index = par_Index - 0x40000000;
                if (Index < Module->CharacteristicCounter) {  // measurements
                    if (par_UserDefFlag) {
                        Ret = 0;  // defaults from A2L file
                    } else {
                        if (CheckIfFlagSetPos (Module->Characteristics[Index]->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_READ_ONLY)) {
                            Ret = 0x2;   // it is only readable
                        } else {
                            Ret = 0x3;   // it is read writable
                        }
                    }
                }
            } else {
                if (par_Index < Module->MeasurementCounter) {  // measurements
                    if (par_UserDefFlag) {
                        Ret = Module->Measurements[par_Index]->DirFlags;
                    } else {
                        if (CheckIfFlagSetPos (Module->Measurements[par_Index]->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_WRITABLE_FLAG)) {
                            Ret = 0x3;   // it is read writable
                        } else {
                            Ret = 0x2;   // it is only readable
                        }
                    }
                }
            }
            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        }
        LeaveCriticalSectionX(LinkIdx);
    }
    return Ret;
}

int A2lGetMeasurementInfos (int par_LinkNr, int par_Index,
                           char *ret_Name, int par_MaxSizeName,
                           int *ret_Type, uint64_t *ret_Address,
                           int *ret_ConvType, char *ret_Conv, int par_MaxSizeConv,
                           char *ret_Unit, int par_MaxSizeUnit,
                           int *ret_FormatLength, int *ret_FormatLayout,
                           int *ret_XDim, int *ret_YDim, int *ret_ZDim,
                           double *ret_Min, double *ret_Max, int *ret_IsWritable)
{
    int Ret = -1;
    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            ASAP2_PARSER* Parser = (ASAP2_PARSER*)Links[LinkIdx].Database->Asap2Ptr;
            ASAP2_MODULE_DATA* Module = Parser->Data.Modules[Links[LinkIdx].Database->SelectedModul];
            if (par_Index < 0x40000000) {
                if (par_Index < Module->MeasurementCounter) {  // measurements
                    Ret = GetMeasurementInfos ((ASAP2_DATABASE*)Links[LinkIdx].Database, par_Index,
                                              ret_Name, par_MaxSizeName,
                                              ret_Type, ret_Address,
                                              ret_ConvType, ret_Conv, par_MaxSizeConv,
                                              ret_Unit, par_MaxSizeUnit,
                                              ret_FormatLength, ret_FormatLayout,
                                              ret_XDim, ret_YDim, ret_ZDim,
                                              ret_Min, ret_Max, ret_IsWritable);
                }
            } else if (par_Index < 0x60000000) {
                par_Index -= 0x40000000;
                if (par_Index < Module->CharacteristicCounter) {  // characteristics
                    Ret = GetValueCharacteristicInfos ((ASAP2_DATABASE*)Links[LinkIdx].Database, par_Index,
                                                      ret_Name, par_MaxSizeName,
                                                      ret_Type, ret_Address,
                                                      ret_ConvType, ret_Conv, par_MaxSizeConv,
                                                      ret_Unit, par_MaxSizeUnit,
                                                      ret_FormatLength, ret_FormatLayout,
                                                      ret_XDim, ret_YDim, ret_ZDim,
                                                      ret_Min, ret_Max, ret_IsWritable);
                }
            }

            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        }
        LeaveCriticalSectionX(LinkIdx);
    }
    return Ret;
}

int A2LGetFunctionIndexFromLink(int par_LinkNr, const char *par_Function)
{
    int Idx = -1;
    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            Idx = GetFunctionIndex(Links[LinkIdx].Database, par_Function);
            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        }
        LeaveCriticalSectionX(LinkIdx);
    }
    return Idx;
}

int A2LGetNextFunctionMemberFromLink (int par_LinkNr, int par_FuncIdx, int par_MemberIdx, const char *par_Filter, unsigned int par_Flags,
                                      char *ret_Name, int par_MaxSize, int *ret_Type)
{
    int Idx = -1;
    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            Idx = GetNextFunctionMember (Links[LinkIdx].Database, par_FuncIdx, par_MemberIdx, par_Filter, par_Flags,
                                         ret_Name, par_MaxSize, ret_Type);
            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        }
        LeaveCriticalSectionX(LinkIdx);
    }
    return Idx;
}


int A2LGetCharacteristicInfoType(int par_LinkNr, int par_Index)
{
    int Ret = -1;
    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            if (par_Index >= 0x40000000) {
                Ret = GetCharacteristicInfoType ((ASAP2_DATABASE*)Links[LinkIdx].Database, par_Index - 0x40000000);
            }
            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        }
        LeaveCriticalSectionX(LinkIdx);
    }
    return Ret;
}

int A2LGetMeasurementOrCharacteristicAddress(int par_LinkNr, int par_Index, uint64_t *ret_Address)
{
    int Ret = -1;
    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            if (par_Index >= 0x40000000) {
                Ret = GetCharacteristicAddress ((ASAP2_DATABASE*)Links[LinkIdx].Database, par_Index - 0x40000000, ret_Address);
            } else {
                Ret = GetMeasurementAddress ((ASAP2_DATABASE*)Links[LinkIdx].Database, par_Index - 0x40000000, ret_Address);

            }
            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        }
        LeaveCriticalSectionX(LinkIdx);
    }
    return Ret;
}

int A2LGetCharacteristicAxisInfos(int par_LinkNr, int par_Index, int par_AxisNo,
                                  CHARACTERISTIC_AXIS_INFO *ret_AxisInfos)
{
    int Ret = -1;
    int LinkIdx = par_LinkNr & LINK_NR_MASK;
    if ((LinkIdx >= 0) && (LinkIdx < MAX_A2L_LINKS)) {
        EnterCriticalSectionX(LinkIdx);
        if (Links[LinkIdx].LoadedFlag && (Links[LinkIdx].LinkNr == par_LinkNr)) {
            Links[LinkIdx].InUseCounter++;
            LeaveCriticalSectionX(LinkIdx);
            if (par_Index >= 0x40000000) {
                char Unit[64];
                char Input[512];
                char *Conversion = my_malloc(INI_MAX_LONGLINE_LENGTH);
                Ret = GetCharacteristicAxisInfos ((ASAP2_DATABASE*)Links[LinkIdx].Database,
                                                  par_Index - 0x40000000, par_AxisNo,
                                                  &ret_AxisInfos->m_Min, &ret_AxisInfos->m_Max,
                                                  Input, 512,
                                                  &ret_AxisInfos->m_ConvType, Conversion, INI_MAX_LONGLINE_LENGTH,
                                                  Unit, 64,
                                                  &ret_AxisInfos->m_FormatLength,
                                                  &ret_AxisInfos->m_FormatLayout);
                ret_AxisInfos->m_Conversion = StringMalloc(Conversion);
                my_free(Conversion);
                ret_AxisInfos->m_Input = StringMalloc(Input);
                ret_AxisInfos->m_Unit = StringMalloc(Unit);
            }
            EnterCriticalSectionX(LinkIdx);
            Links[LinkIdx].InUseCounter--;
        }
        LeaveCriticalSectionX(LinkIdx);
    }
    return Ret;
}


static int ReferenceVariable(int par_LinkNr, int par_Index,
                             char *par_Name, char *par_Unit, int par_ConvType, char *par_Conv,
                             int par_FormatLength, int par_FormatLayout,
                             uint64_t par_Address, int par_Pid, int par_Type, int par_Dir,
                             int par_ReferenceFlag)
{
    int Vid;
    if (par_ReferenceFlag) {
        if (scm_ref_vari (par_Address, par_Pid, par_Name, par_Type, par_Dir) ||
            ((Vid = get_bbvarivid_by_name(par_Name)) <= 0)) {
            ThrowError (1, "cannot reference \"%s\"", par_Name);
            return -1;
        } else {
            if (Vid > 0) {
                set_bbvari_unit(Vid, par_Unit);
                set_bbvari_conversion(Vid, par_ConvType, par_Conv);
                set_bbvari_format(Vid, par_FormatLength, par_FormatLayout);
                A2LIncDecGetReferenceToDataFromLink(par_LinkNr, par_Index, +1, par_Dir, Vid);
            }
        }
    } else {
        if (scm_unref_vari(par_Address, par_Pid, par_Name, par_Type)) {
            ThrowError (1, "cannot unreference \"%s\"", par_Name);
            return -1;
        }
        A2LIncDecGetReferenceToDataFromLink(par_LinkNr, par_Index, -1, par_Dir, 0);
    }
    return 0;
}


int A2LReferenceMeasurementToBlackboard(int par_LinkNr, int par_Idx, int par_Dir, int par_ReferenceFlag)
{
    char Name[512];
    int Type;
    uint64_t Address;
    int ConvType;
    char Unit[64];
    int FormatLength;
    int FormatLayout;
    int XDim, YDim, ZDim;
    double Min, Max;
    int IsWritable;
    int Pid;
    int Ret = 0;
    char *Conv = (char*)my_malloc(128*1024);
    if (Conv == NULL) {
        ThrowError(1, "out of memory");
        Ret = -1;
    } else {
        Pid = A2LGetLinkedToPid(par_LinkNr);

        if ((A2LIncDecGetReferenceToDataFromLink(par_LinkNr, par_Idx, 0, 0, 0) != 0) ^ par_ReferenceFlag) {
            if (A2lGetMeasurementInfos (par_LinkNr, par_Idx,
                                        Name, sizeof(Name),
                                        &Type, &Address,
                                        &ConvType, Conv, 128*1024,
                                        Unit, sizeof(Unit),
                                        &FormatLength, &FormatLayout,
                                        &XDim, &YDim, &ZDim,
                                        &Min, &Max, &IsWritable) == 0) {
                if ((XDim <= 1) && (YDim <= 1) && (ZDim <= 1)) {
                    if (ReferenceVariable(par_LinkNr, par_Idx,
                                          Name, Unit, ConvType, Conv,
                                          FormatLength, FormatLayout,
                                          Address, Pid, Type, par_Dir, par_ReferenceFlag)) {
                        Ret--;
                    }
                } else if ((YDim <= 1) && (ZDim <= 1)) {  // one dimensional array
                    int i;
                    char *p = Name + strlen(Name);
                    for (i = 0; i < XDim; i++) {
                        PrintFormatToString(p, sizeof(Name) - (p - Name), "[%i]", i);
                        if (ReferenceVariable(par_LinkNr, par_Idx,
                                              Name, Unit, ConvType, Conv,
                                              FormatLength, FormatLayout,
                                              Address, Pid, Type, par_Dir, par_ReferenceFlag)) {
                            Ret--;
                        }
                        Address += GetDataTypeByteSize(Type);
                    }
                } else if (ZDim <= 1) { // two dimensional array
                    int i, j;
                    char *p = Name + strlen(Name);
                    for (j = 0; j < YDim; j++) {
                        for (i = 0; i < XDim; i++) {
                            PrintFormatToString(p, sizeof(Name) - (p - Name), "[%i][%i]", j, i);
                            if (ReferenceVariable(par_LinkNr, par_Idx,
                                                  Name, Unit, ConvType, Conv,
                                                  FormatLength, FormatLayout,
                                                  Address, Pid, Type, par_Dir, par_ReferenceFlag)) {
                                Ret--;
                            }
                            Address += GetDataTypeByteSize(Type);
                        }
                    }
                }
            }
        }
        my_free(Conv);
    }
    return Ret;
}

int A2LStoreAllReferencesForProcess(int par_LinkNr, const char *par_ProcessName)
{
    int Index = -1;
    char LabelName[512];
    char SectionName[MAX_PATH + 100];
    PrintFormatToString (SectionName, sizeof(SectionName), "Signal referenced from A2L for Process %s", par_ProcessName);
    IniFileDataBaseWriteString(SectionName, NULL, NULL, GetMainFileDescriptor());
    while ((Index = A2LGetNextSymbolFromLink(par_LinkNr, Index, A2L_LABEL_TYPE_REFERENCED_MEASUREMENT, "*", LabelName, sizeof(LabelName))) >= 0) {
        const char *UserDir;
        switch(A2LGetReadWritablFlagsFromLink(par_LinkNr, Index, 1)) {
        case 0x1:
            UserDir = "bb2ep";
            break;
        case 0x2:
            UserDir = "ep2bb";
            break;
        case 0x3:
            UserDir = "readwrite";
            break;
        default:
            UserDir = "default";
            break;
        }
        IniFileDataBaseWriteString(SectionName, LabelName, UserDir, GetMainFileDescriptor());
    }
    return 0;
}

int A2LLoadAllReferencesForProcess(int par_LinkNr, const char *par_ProcessName)
{
    int Index = 0;
    int WhatToDo = 0;
    char LabelName[512];
    char SectionName[MAX_PATH + 100];
    char Text[512];
    PrintFormatToString (SectionName, sizeof(SectionName), "Signal referenced from A2L for Process %s", par_ProcessName);

    while ((Index = IniFileDataBaseGetNextEntryName(Index, SectionName, LabelName, sizeof(LabelName), GetMainFileDescriptor())) >= 0) {
        int ErrFlag = -1;
        int A2LIndex = A2LGetIndexFromLink(par_LinkNr, LabelName, A2L_LABEL_TYPE_MEASUREMENT | A2L_LABEL_TYPE_SINGLE_VALUE_CALIBRATION);
        if (A2LIndex >= 0) {
            int Dir = 0x2;
            IniFileDataBaseReadString(SectionName, LabelName, "default", Text, sizeof(Text), GetMainFileDescriptor());
            if (!strcmp(Text, "bb2ep")) {
                Dir = 0x1;
            } else if (!strcmp(Text, "ep2bb")) {
                Dir = 0x2;
            } else if (!strcmp(Text, "readwrite")) {
                Dir = 0x3;
            } else {  // default
                Dir = A2LGetReadWritablFlagsFromLink(par_LinkNr, A2LIndex, 0);
            }
            ErrFlag = A2LReferenceMeasurementToBlackboard(par_LinkNr, A2LIndex, Dir, 1);
        }
        if (ErrFlag) {
            switch (WhatToDo) {
            case 0:
                switch (ThrowError (ERROR_OK_OKALL_CANCEL_CANCELALL, "the label \"%s\" not exists inside the A2L description.\n"
                               "should it remove?", LabelName)) {
                case IDOK:
                    WhatToDo = 0;  // delete only this
                    IniFileDataBaseWriteString(SectionName, NULL, NULL, GetMainFileDescriptor());
                    Index--;
                    break;
                case IDOKALL:
                    WhatToDo = 1;  // delete all
                    IniFileDataBaseWriteString(SectionName, NULL, NULL, GetMainFileDescriptor());
                    Index--;
                    break;
                case IDCANCEL:
                    WhatToDo = 0;  // ignore this
                    break;
                case IDCANCELALL:
                    WhatToDo = -1;  // ignore all
                    break;
                }
                break;
            case 1:
                IniFileDataBaseWriteString(SectionName, NULL, NULL, GetMainFileDescriptor());
                Index--;
                break;
            case -1:
                break;
            }
        }
    }
    return 0;
}

int A2LRemoveOneReferencesForProcess(int par_LinkNr, int par_Pid, const char *par_ProcessName, int par_Index)
{
    char Name[512];
    int Type;
    uint64_t Address;
    int ConvType;
    char Conv[512];
    char Unit[64];
    int FormatLength;
    int FormatLayout;
    int XDim, YDim, ZDim;
    double Min, Max;
    int IsWritable;

    if (A2lGetMeasurementInfos (par_LinkNr, par_Index,
                               Name, sizeof(Name),
                               &Type, &Address,
                               &ConvType, Conv, sizeof(Conv),
                               Unit, sizeof(Unit),
                               &FormatLength, &FormatLayout,
                               &XDim, &YDim, &ZDim,
                               &Min, &Max, &IsWritable) == 0) {
        if ((XDim <= 1) && (YDim <= 1) && (ZDim <= 1)) {
            scm_unref_vari(Address, par_Pid, Name, Type);
            A2LIncDecGetReferenceToDataFromLink(par_LinkNr, par_Index, -1, 0, 0);
        } else if ((YDim <= 1) && (ZDim <= 1)) {  // one dimensional array
            int i;
            char *p = Name + strlen(Name);
            for (i = 0; i < XDim; i++) {
                PrintFormatToString(p, p - Name, "[%i]", i);
                scm_unref_vari(Address, par_Pid, Name, Type);
                A2LIncDecGetReferenceToDataFromLink(par_LinkNr, par_Index, -1, 0, 0);
                Address += GetDataTypeByteSize(Type);
            }
        } else if (ZDim <= 1) { // two dimensional array
            int i, j;
            char *p = Name + strlen(Name);
            for (j = 0; j < YDim; j++) {
                for (i = 0; i < XDim; i++) {
                    PrintFormatToString(p, p- Name, "[%i][%i]", j, i);
                    scm_unref_vari(Address, par_Pid, Name, Type);
                    A2LIncDecGetReferenceToDataFromLink(par_LinkNr, par_Index, -1, 0, 0);
                    Address += GetDataTypeByteSize(Type);
                }
            }
        }
        return 0;
    } else {
        return -1;
    }
}


int A2LRemoveAllReferencesForProcess(int par_Pid, const char *par_ProcessName, int par_LinkNr)
{
    int Index = -1;
    char LabelName[512];
    char SectionName[MAX_PATH + 100];
    PrintFormatToString (SectionName, sizeof(SectionName), "Signal referenced from A2L for Process %s", par_ProcessName);
    IniFileDataBaseWriteString(SectionName, NULL, NULL, GetMainFileDescriptor());
    while ((Index = A2LGetNextSymbolFromLink(par_LinkNr, Index,
                                             A2L_LABEL_TYPE_REFERENCED_MEASUREMENT |
                                                 A2L_LABEL_TYPE_MEASUREMENT |
                                                 A2L_LABEL_TYPE_CALIBRATION |
                                                 A2L_LABEL_TYPE_AXIS_CALIBRATION,
                                             "*", LabelName, sizeof(LabelName))) >= 0) {
        A2LRemoveOneReferencesForProcess(par_LinkNr, par_Pid, par_ProcessName, Index);
    }
    return 0;
}


int ImportMeasurementReferencesListForProcess (int pid, char *FileName)
{
    char ProcessNameWithoutPath[MAX_PATH];
    char Section[INI_MAX_SECTION_LENGTH];
    char ProcessName[MAX_PATH];
    int Ret = 0;
    int Idx = 0;
    int LinkNr;
    int Fd;

    get_name_by_pid (pid, ProcessName, sizeof(ProcessName));
    if (GetProcessNameWithoutPath (pid, ProcessNameWithoutPath, sizeof(ProcessNameWithoutPath))) {
        return -1;
    }
    if (_access (FileName, 4) != 0) {  // Read permission?
        return -1;
    }
    LinkNr = A2LGetLinkToExternProcess(ProcessNameWithoutPath);
    if (LinkNr < 0) {
        return -1;
    }
    A2LRemoveAllReferencesForProcess (pid, ProcessNameWithoutPath, LinkNr);
    // first delete all sections for this process
    PrintFormatToString (Section, sizeof(Section), "Signal referenced from A2L for Process *%s", ProcessNameWithoutPath);
    while ((Idx = IniFileDataBaseFindNextSectionNameRegExp (Idx, Section, 0, Section, sizeof(Section), GetMainFileDescriptor())) >= 0) {
        IniFileDataBaseWriteString(Section, NULL, NULL, GetMainFileDescriptor());
    }
    Fd = IniFileDataBaseOpen (FileName);
    PrintFormatToString (Section, sizeof(Section), "Signal referenced from A2L for Process %s", ProcessNameWithoutPath);
    Ret = IniFileDataBaseCopySection (GetMainFileDescriptor(), Fd, Section, "Signal referenced from A2L for Process");
    IniFileDataBaseClose (Fd);
    A2LLoadAllReferencesForProcess(LinkNr, ProcessNameWithoutPath);
    return Ret;
}

int ExportMeasurementReferencesListForProcess (int pid, char *FileName)
{
    char ProcessNameWithoutPath[MAX_PATH];
    char Section[INI_MAX_SECTION_LENGTH];
    char ProcessName[MAX_PATH];
    int Ret = 0;
    int LinkNr;
    int Fd;

    get_name_by_pid (pid, ProcessName, sizeof(ProcessName));
    if (GetProcessNameWithoutPath (pid, ProcessNameWithoutPath, sizeof(ProcessNameWithoutPath))) {
        return -1;
    }
    LinkNr = A2LGetLinkToExternProcess(ProcessNameWithoutPath);
    if (LinkNr < 0) {
        return -1;
    }
    if (CreateNewEmptyIniFile(FileName)) {
        return -1;
    }

    A2LStoreAllReferencesForProcess(LinkNr, ProcessNameWithoutPath);
    Fd = CreateNewEmptyIniFile (FileName);
    PrintFormatToString (Section, sizeof(Section), "Signal referenced from A2L for Process %s", ProcessNameWithoutPath);
    Ret = IniFileDataBaseCopySection (Fd, GetMainFileDescriptor(), "Signal referenced from A2L for Process", Section);
    IniFileDataBaseSave (Fd, FileName, INIFILE_DATABAE_OPERATION_REMOVE);
    return Ret;
}


int A2LGetLinkedToPid(int par_LinkNr)
{
    int Pid = -1;
    int Idx = par_LinkNr & LINK_NR_MASK;
    if ((Idx < 0) || (Idx >= MAX_A2L_LINKS) || !Links[Idx].LoadedFlag || (Links[Idx].LinkNr != par_LinkNr)) return -1;
    EnterCriticalSectionX(Idx);
    if (Links[Idx].Type == LINK_TYPE_EXT_PROCESS) {
        Pid = Links[Idx].To.Pid;
    }
    LeaveCriticalSectionX(Idx);
    return Pid;
}
