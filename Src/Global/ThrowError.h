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


#ifndef ERRORFUNCTION_H
#define ERRORFUNCTION_H

#include <stdint.h>

/*************************************************************************
**    constants and macros
*************************************************************************/

// error type defines
#define MESSAGE_ONLY              0
#define MESSAGE_STOP	          1
#define ERROR_NO_CRITICAL    	  2
#define ERROR_NO_CRITICAL_STOP	  3
#define ERROR_CRITICAL	          4
#define ERROR_SYSTEM_CRITICAL	  5
#define ERROR_OKCANCEL	          6
#define RT_ERROR                  7
#define ERROR_OK_OKALL_CANCEL     8
#define INFO_NO_STOP              9
#define WARNING_NO_STOP          10
#define ERROR_NO_STOP            11
#define RT_INFO_MESSAGE          12
#define ERROR_OK_OKALL_CANCEL_CANCELALL   13
#define INFO_NOT_WRITEN_TO_MSG_FILE       14
#define ERROR_OK_CANCEL_CANCELALL   15
#define QUESTION_OKCANCEL           16
#define MESSAGE_OKCANCEL            17
#define QUESTION_SAVE_IGNORE_CANCLE 18

// special defines
#define IDOKALL                 100
#define IDCANCELALL             101
#define IDSAVE                  102
//#define IDIGNORE                103

/*************************************************************************
**    function prototypes
*************************************************************************/

void SetError2Message (int flag);
int GetError2MessageFlag (void);

#ifndef NO_ERROR_FUNCTION_DECLARATION
extern int ThrowError (int level, const char *txt, ...);
extern int ThrowErrorString (int level, uint64_t Cycle, const char *MessageBuffer);
#endif

int ThrowErrorWithCycle (int level, uint64_t Cycle, const char *format, ...);

char *GetLastSysErrorString (void);
void FreeLastSysErrorString (char *String);

#endif
