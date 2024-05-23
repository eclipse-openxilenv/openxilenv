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


#ifndef _PROC_EQU_H
#define _PROC_EQU_H

#include "tcb.h"

void ClacBeforeProcessEquations(TASK_CONTROL_BLOCK *tcb);
void ClacBehindProcessEquations(TASK_CONTROL_BLOCK *tcb);
int AddBeforeProcessEquation(int Nr, int Pid, void *ExecStack);
int AddBehindProcessEquation(int Nr, int Pid, void *ExecStack);
int DelBeforeProcessEquations(int Nr, int Pid);
int DelBehindProcessEquations(int Nr, int Pid);

#endif