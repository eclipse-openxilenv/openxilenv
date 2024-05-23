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


#ifndef SC_START_EXE_H
#define SC_START_EXE_H


#include "Platform.h"

HANDLE start_exe(char *par_CommandString, char **ret_ErrMsg);
int wait_for_end_of_exe (HANDLE hProcess, int par_Timeout, DWORD *ret_ExitCode);

DWORD get_start_exe_ExitCode(void);

void FreeStartExeErrorMsgBuffer(char *par_ErrMsg);
void StartExeFreeProcessHandle(HANDLE hProcess);


#endif
