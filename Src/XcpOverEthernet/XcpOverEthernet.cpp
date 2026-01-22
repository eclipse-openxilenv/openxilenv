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
#include <string.h>
#include <malloc.h>
#include "Platform.h"

extern "C" {
#include "tcb.h"
#include "Scheduler.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "Files.h"

#define IDM_WRITE_SECTION_TO_EXE_CMD        0xABF1

#include "MainValues.h"
#include "IniDataBase.h"
}
#include "XcpConnector.h"
#include "XcpWrapper.h"
#include "XcpOverEthernet.h"


// Connection between port and process name
typedef struct {
    int ActiveFlag;
    unsigned short Port;
    char ProcessName[MAX_PATH];
    char Identification[MAX_PATH];
    char *DataSegment;
    char *CodeSegment;
    int LoggingActive;
    char LogFile[MAX_PATH];
    int NumOfEvents;
    XCP_EVENT_DESC EventChannelSettings[MAXEVENTCHANNELS];
    unsigned int EventCycleCounters[MAXEVENTCHANNELS];

    cXCPWrapper *XCPWrapper; 

    int ConnectedFlag;
    int Pid;
    HANDLE ProcHandle;

    bool AddressTranslationForProcessesInsideDll;

} XCP_LINK_CONFIG;


static XCP_LINK_CONFIG XcpLinkConfigs[MAX_XCP_LINKS];

typedef struct {
    short Pid;
    char Section[64];
} IDM_WRITE_SECTION_TO_EXE_DATA;

int XCPWriteSectionBackToExeFile (short Pid, char *Section)
{
    IDM_WRITE_SECTION_TO_EXE_DATA Data;

    Data.Pid = Pid;
    STRING_COPY_TO_ARRAY (Data.Section, Section);
    ThrowError (1, "todo!");
    return 0;
}

extern "C" {
int XCPWriteSectionToExeSyncCallFromMainThread (void *Ptr)
{
    return scm_write_section_to_exe ((static_cast<IDM_WRITE_SECTION_TO_EXE_DATA*>(Ptr))->Pid, (static_cast<IDM_WRITE_SECTION_TO_EXE_DATA*>(Ptr))->Section);
}
}

void ReadXcpConfigsForOneLinkFromIni (int l)
{
    char Section[INI_MAX_SECTION_LENGTH];
    char Entry[INI_MAX_ENTRYNAME_LENGTH];
    char Txt[INI_MAX_LINE_LENGTH];
    int Fd = GetMainFileDescriptor();

    STRUCT_ZERO_INIT (XcpLinkConfigs[l], XCP_LINK_CONFIG);

    PrintFormatToString (Section, sizeof(Section), "XCP over Ethernet Configuration for Target %i", l);
    XcpLinkConfigs[l].ActiveFlag = IniFileDataBaseReadYesNo(Section, "Enable", 0, Fd);
    IniFileDataBaseReadString(Section, "Identification", "", XcpLinkConfigs[l].Identification, sizeof(XcpLinkConfigs[l].Identification), Fd);
    IniFileDataBaseReadString(Section, "AssociatedProcessName", "", XcpLinkConfigs[l].ProcessName, sizeof(XcpLinkConfigs[l].ProcessName), Fd);
    IniFileDataBaseReadString(Section, "CalibrationDataSegmentName", "", Txt, sizeof(Txt), Fd);
    XcpLinkConfigs[l].DataSegment = StringMalloc(Txt);
    IniFileDataBaseReadString(Section, "CodeSegmentName", "", Txt, sizeof(Txt), Fd);
    XcpLinkConfigs[l].CodeSegment = StringMalloc(Txt);
    XcpLinkConfigs[l].Port = static_cast<unsigned short>(IniFileDataBaseReadUInt(Section, "Port", 1802, Fd));
    XcpLinkConfigs[l].LoggingActive = IniFileDataBaseReadYesNo(Section, "DebugFile", 0, Fd);
    IniFileDataBaseReadString(Section, "DebugFileName", "", XcpLinkConfigs[l].LogFile, sizeof(XcpLinkConfigs[l].LogFile), Fd);
    XcpLinkConfigs[l].AddressTranslationForProcessesInsideDll = IniFileDataBaseReadYesNo(Section, "AddressTranslationForProcessesInsideDll", 0, Fd) != 0;

    int EventCount;
    for (EventCount = 0; EventCount < MAXEVENTCHANNELS; EventCount++) {
        PrintFormatToString (Entry, sizeof(Entry), "daq_%i", EventCount);
        if (IniFileDataBaseReadString (Section, Entry, "", Txt, sizeof (Txt), Fd) <= 0) {
            break;
        }
        char *t, *c, *pr, *n;
        if (StringCommaSeparate (Txt, &t, &c, &pr, &n, nullptr) != 4) continue;
        if (!strcmp (t, "1ns")) {
            XcpLinkConfigs[l].EventChannelSettings[EventCount].timebase = 0;
        } else if (!strcmp (t, "10ns")) {
            XcpLinkConfigs[l].EventChannelSettings[EventCount].timebase = 1;
        } else if (!strcmp (t, "100ns")) {
            XcpLinkConfigs[l].EventChannelSettings[EventCount].timebase = 2;
        } else if (!strcmp (t, "1us")) {
            XcpLinkConfigs[l].EventChannelSettings[EventCount].timebase = 3;
        } else if (!strcmp (t, "10us")) {
            XcpLinkConfigs[l].EventChannelSettings[EventCount].timebase = 4;
        } else if (!strcmp (t, "100us")) {
            XcpLinkConfigs[l].EventChannelSettings[EventCount].timebase = 5;
        } else if (!strcmp (t, "1ms")) {
            XcpLinkConfigs[l].EventChannelSettings[EventCount].timebase = 6;
        } else if (!strcmp (t, "10ms")) {
            XcpLinkConfigs[l].EventChannelSettings[EventCount].timebase = 7;
        } else if (!strcmp (t, "100ms")) {
            XcpLinkConfigs[l].EventChannelSettings[EventCount].timebase = 8;
        } else if (!strcmp (t, "1s")) {
            XcpLinkConfigs[l].EventChannelSettings[EventCount].timebase = 9;
        } else {
            XcpLinkConfigs[l].EventChannelSettings[EventCount].timebase = 7;
        }
        XcpLinkConfigs[l].EventChannelSettings[EventCount].timeCycle = static_cast<unsigned char>(atol (c));
        XcpLinkConfigs[l].EventChannelSettings[EventCount].Prio = static_cast<unsigned char>(atol (pr));
        XcpLinkConfigs[l].EventChannelSettings[EventCount].EventName = StringMalloc (n);
    }
    XcpLinkConfigs[l].NumOfEvents = EventCount;

    if (XcpLinkConfigs[l].ActiveFlag) {
        XcpLinkConfigs[l].XCPWrapper = new cXCPWrapper;
        if (XcpLinkConfigs[l].XCPWrapper != nullptr) {
            XcpLinkConfigs[l].XCPWrapper->Init (l,
                                                XcpLinkConfigs[l].Port,
                                                XcpLinkConfigs[l].Identification,
                                                XcpLinkConfigs[l].ProcessName,
                                                static_cast<unsigned char>(XcpLinkConfigs[l].NumOfEvents),
                                                XcpLinkConfigs[l].EventChannelSettings,
                                                XcpLinkConfigs[l].DataSegment, XcpLinkConfigs[l].CodeSegment,
                                                XcpLinkConfigs[l].LoggingActive, XcpLinkConfigs[l].LogFile);
            if (XcpLinkConfigs[l].AddressTranslationForProcessesInsideDll) {
                XcpLinkConfigs[l].XCPWrapper->ActivateDllAddressTranslation();
            } else {
                XcpLinkConfigs[l].XCPWrapper->DeactivateDllAddressTranslation();
            }
            //XcpLinkConfigs[l].XCPWrapper->SetLogfile (XcpLinkConfigs[l].LogFile);
        } else {
            ThrowError (1, "new cXCPWrapper");
        }
    }
}


void CleanUpXcpOneLinkLinkConfigs (int l)
{
#ifdef _WIN32
    int x;

    if (XcpLinkConfigs[l].ActiveFlag) {
        if (XcpLinkConfigs[l].XCPWrapper != nullptr) {
            delete XcpLinkConfigs[l].XCPWrapper;
            XcpLinkConfigs[l].XCPWrapper = nullptr;
        }
        if (XcpLinkConfigs[l].ProcHandle != nullptr) {
            CloseHandle (XcpLinkConfigs[l].ProcHandle);
            XcpLinkConfigs[l].ProcHandle = nullptr;
        }
        for (x = 0; x < XcpLinkConfigs[l].NumOfEvents; x++) {
            if (XcpLinkConfigs[l].EventChannelSettings[x].EventName != nullptr) {
                my_free (XcpLinkConfigs[l].EventChannelSettings[x].EventName);
            }
        }
        if (XcpLinkConfigs[l].DataSegment != nullptr) my_free (XcpLinkConfigs[l].DataSegment);
        if (XcpLinkConfigs[l].CodeSegment != nullptr) my_free (XcpLinkConfigs[l].CodeSegment);
        STRUCT_ZERO_INIT (XcpLinkConfigs[l], XCP_LINK_CONFIG);
    }
#else
#endif
}


int ConnectToProcess (int LinkNr, int *ret_Pid, char *ret_ProcessName, int MaxChars)
{
#ifdef _WIN32
    if ((LinkNr < MAX_XCP_LINKS) && (LinkNr >= 0)) {
        if (XcpLinkConfigs[LinkNr].ActiveFlag) {
            PID Pid = -1; 
            Pid = get_pid_by_name (XcpLinkConfigs[LinkNr].ProcessName);
            if (Pid > 0) {
                DWORD ProcId = get_extern_process_windows_id_by_pid(Pid);
                if (ProcId > 0) {
                    HANDLE ProcHandle = OpenProcess (PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, ProcId);
                    if (ProcHandle != INVALID_HANDLE_VALUE) {
                        XcpLinkConfigs[LinkNr].ConnectedFlag = 1;
                        XcpLinkConfigs[LinkNr].ProcHandle = ProcHandle;
                        *ret_Pid = XcpLinkConfigs[LinkNr].Pid = Pid;
                        StringCopyMaxCharTruncate(ret_ProcessName, XcpLinkConfigs[LinkNr].ProcessName, MaxChars);
                    }
                    return 0;
                }
            }
        }
    }
#else
#endif
    return -1;
}

int DisconnectFronProcess (int LinkNr)
{
#ifdef _WIN32
    if ((LinkNr < MAX_XCP_LINKS) && (LinkNr >= 0)) {
        if (XcpLinkConfigs[LinkNr].ActiveFlag) {
            if ( XcpLinkConfigs[LinkNr].ConnectedFlag) {
                XcpLinkConfigs[LinkNr].ConnectedFlag = 0;
                if (XcpLinkConfigs[LinkNr].ProcHandle != nullptr) {
                    CloseHandle (XcpLinkConfigs[LinkNr].ProcHandle);
                    XcpLinkConfigs[LinkNr].ProcHandle = nullptr;
                }
                return 0;
            }
        }
    }
#else
#endif
    return -1;
}


int XCPReadMemoryFromExternProcess (int LinkNr, uint64_t address, void *dest, int len)
{
#ifdef _WIN32
    SIZE_T ret = 0;

    if ((LinkNr < MAX_XCP_LINKS) && (LinkNr >= 0)) {
        if (XcpLinkConfigs[LinkNr].ActiveFlag) {
            if (XcpLinkConfigs[LinkNr].Pid > 0) {
                if (!ReadProcessMemory (XcpLinkConfigs[LinkNr].ProcHandle, reinterpret_cast<void*>(address), dest, static_cast<size_t>(len), &ret)) {
                    char *ErrString = GetLastSysErrorString ();
                    ThrowError (1, "cannot write to process \"%s\" memory location 0x%08X  (%i) \"%s\"", XcpLinkConfigs[LinkNr].ProcessName, address, GetLastError (), ErrString);
                    FreeLastSysErrorString (ErrString);
                }
            }
        }
    }
    return static_cast<int>(ret);
#else
    return -1;
#endif
}

int XCPWriteMemoryToExternProcess (int LinkNr, uint64_t address, void *src, int len)
{
#ifdef _WIN32
    SIZE_T ret = 0;

    if ((LinkNr < MAX_XCP_LINKS) && (LinkNr >= 0)) {
        if (XcpLinkConfigs[LinkNr].ActiveFlag) {
            if (XcpLinkConfigs[LinkNr].Pid > 0) {
                if (!WriteProcessMemory (XcpLinkConfigs[LinkNr].ProcHandle, reinterpret_cast<void*>(address), src, static_cast<size_t>(len), &ret)) {
                    char *ErrString = GetLastSysErrorString ();
                    ThrowError (1, "cannot write to process \"%s\" memory location 0x%08X  (%i) \"%s\"", XcpLinkConfigs[LinkNr].ProcessName, address, GetLastError (), ErrString);
                    FreeLastSysErrorString (ErrString);
                }
            }
        }
    }
    return static_cast<int>(ret);
#else
    return -1;
#endif
}

extern "C" {
void ReAllInitXCPOverEthLinks (void)
{
    int l;
    for (l = 0; l < MAX_XCP_LINKS; l++) {
        CleanUpXcpOneLinkLinkConfigs (l);
    }
    for (l = 0; l < MAX_XCP_LINKS; l++) {
        ReadXcpConfigsForOneLinkFromIni (l);
    }
}
}

extern "C" {
int ExternProcessTerminatedCloseLink (short Pid)
{
    int l;

    for (l = 0; l < MAX_XCP_LINKS; l++) {
        if (XcpLinkConfigs[l].ActiveFlag && (XcpLinkConfigs[l].Pid == Pid)) {
            CleanUpXcpOneLinkLinkConfigs (l);
            ReadXcpConfigsForOneLinkFromIni (l);
            return 0;
        }
    }
    return -1;
}
}

int xcp_over_eth_init (void)
{
    int l;
#ifdef _WIN32
    if (_heapchk () != _HEAPOK) {
        printf ("Stop");
    }
#endif
    for (l = 0; l < MAX_XCP_LINKS; l++) {
        ReadXcpConfigsForOneLinkFromIni (l);
    }
    return 0;
}

void xcp_over_eth_cyclic (void)
{
    int l;

    for (l = 0; l < MAX_XCP_LINKS; l++) {
        if (XcpLinkConfigs[l].ActiveFlag) {
            XcpLinkConfigs[l].XCPWrapper->IncrementCycleCounter ();
        }
    }
}

void xcp_over_eth_terminate (void)
{
    int l;
    for (l = 0; l < MAX_XCP_LINKS; l++) {
        CleanUpXcpOneLinkLinkConfigs (l);
    }
}

extern "C" {
TASK_CONTROL_BLOCK xcp_over_eth_tcb
    = INIT_TASK_COTROL_BLOCK("XCP_over_Eth", INTERN_ASYNC, 333, xcp_over_eth_cyclic, xcp_over_eth_init, xcp_over_eth_terminate, 16*1024);
}
