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


#ifndef OSCILLOSCOPECYCLIC_H
#define OSCILLOSCOPECYCLIC_H

#include "tcb.h"
#include "OscilloscopeData.h"

#ifdef __cplusplus
extern "C" {
#endif

int RegisterOscilloscopeToCyclic (OSCILLOSCOPE_DATA *poszidata);
int UnregisterOscilloscopeFromCyclic (OSCILLOSCOPE_DATA *poszidata);

void SwitchOscilloscopeOnline (OSCILLOSCOPE_DATA *poszidata, uint64_t par_StartTime, int par_EnterCS);

void SwitchAllSyncronOffline(void);

void EnterOsziCycleCS (void);
void LeaveOsziCycleCS (void);

int AllocOsciWidgetCriticalSection (void);
void EnterOsciWidgetCriticalSection (int par_Number);
void LeaveOsciWidgetCriticalSection (int par_Number);
void DeleteOsciWidgetCriticalSection (int par_Number);

extern void cyclic_oszi (void);

extern int init_oszi (void);

extern void terminate_oszi (void);

extern int update_vid_owc (OSCILLOSCOPE_DATA *poszidata);

void InitOsciCycleCriticalSection (void);

extern TASK_CONTROL_BLOCK oszi_tcb;

#ifdef __cplusplus
}
#endif


#endif // OSCILLOSCOPECYCLIC_H

