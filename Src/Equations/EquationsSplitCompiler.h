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


#ifndef EQUATIONSPLITCOMPILER_H
#define EQUATIONSPLITCOMPILER_H

#include <stdlib.h>
#include "Config.h"
#include "tcb.h"
#include "Blackboard.h"
#include "ExecutionStack.h"

void rm_CyclicEquationCompiler (void);
void rm_TerminateEquationCompiler (void);
int rm_InitEquationCompiler (void);

extern TASK_CONTROL_BLOCK rm_EquationCompilerTcb;

#undef extern
#endif

