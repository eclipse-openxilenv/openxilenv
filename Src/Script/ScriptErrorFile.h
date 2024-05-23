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


#ifndef SC_ERRORFILE_H
#define SC_ERRORFILE_H

#include <stdint.h>

int GenerateNewErrorFile(void);

void CloseScriptErrorFile(void);

#define NO_COUNTER   0
#define WARN_COUNTER 1
#define ERR_COUNTER  2
void AddScriptError(char *szText, char counter_increment);

uint32_t GetErrorCounter(void);

#define DONT_TERMINATE_XILENV_ON_SCRIPT_ERROR       0
#define TERMINATE_XILENV_ON_SCRIPT_EXECUTION_ERROR  1
#define TERMINATE_XILENV_ON_SCRIPT_SYNTAX_ERROR     2
void SetScriptErrExit (int par_Flags);
int ExitIfErrorInScript (void);
int ExitIfSyntaxErrorInScript (void);

#endif
