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


#ifndef XCP_OVER_ETH_H
#define XCP_OVER_ETH_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif
void ReAllInitXCPOverEthLinks (void);
int ExternProcessTerminatedCloseLink (short Pid);
int XCPWriteSectionToExeSyncCallFromMainThread (void *Ptr);
#if defined(__cplusplus)
}
#endif

typedef struct {
    int LoadedFlag;
    int EnableFlag;
    char Identification[MAX_PATH];
    char AssociatedProcessName[MAX_PATH];
    char CalibrationDataSegmentName[64];
    char CodeSegmentName[64];
    unsigned short Port;
    int DebugFileFlag;
    char DebugFileName[MAX_PATH];
    int VarListCount;
    int VarListCountOld;
    int VarListSize;
    int VarListPos;
    char *VarList;
    int VarListHasChanged;
} XCP_OVER_ETH_CFGDLG_SHEET_DATA;



int ConnectToProcess (int LinkNr, int *ret_Pid, char *ret_ProcessName);
int DisconnectFronProcess (int LinkNr);

int XCPReadMemoryFromExternProcess (int LinkNr, uint64_t address, void *dest, int len);

int XCPWriteMemoryToExternProcess (int LinkNr, uint64_t address, void *src, int len);

int XCPWriteSectionBackToExeFile (short Pid, char *Section);

extern "C" {
#include "tcb.h"
extern TASK_CONTROL_BLOCK xcp_over_eth_tcb;
}

#endif
