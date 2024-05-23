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


#ifndef INTERFACETOSCRIPT_H
#define INTERFACETOSCRIPT_H

#include "tcb.h"

/* For status flag states */
#define STOP      0
#define START     1
#define SLEEP     2
#define RUNNING   3
#define READY     4
#define FINISHED  5
#define ERROR_SCT 6

#define MAX_PATH_LAENGE MAX_PATH

#define MAX_ZEILENLAENGE  8192

#ifdef __cplusplus
extern "C" {
#endif
int StartScript(const char *par_Script);
int GetScriptState(void);
int StopScript(void);

void ScriptIdentifyPath (char *par_Filename);
int copy_file(const char *quelldateiname, const char *zieldateiname);

extern char script_filename[];

extern unsigned char  script_status_flag;

void OpenScriptDebugWindow (void);
void CloseScriptDebugWindow (void);

void ScriptErrorMsgDlg_GuiThread (void* ptr_ErrMsgStruct);

// Script debug window communication between GUI thread and scheduler thread
void ScriptDebugWindowGui_GuiThread (void *ptr_Data);
void ReceiveScriptDebugWindowRequest_SchedThread (void *ptr_Data);

const char *GetRunningScriptPath (void);
int GetRunningScriptLine (void);

int SetScriptOutputFilenamesPrefix (char *par_Prefix);


extern TASK_CONTROL_BLOCK script_tcb;

#ifdef __cplusplus
}
#endif
#endif
