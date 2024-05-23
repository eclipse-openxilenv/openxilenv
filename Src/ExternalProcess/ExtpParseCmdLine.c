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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "Platform.h"
#include "XilEnvExtProc.h"
#include "ExtpParseCmdLine.h"

int XilEnvInternal_ParseCmdLine (char *par_Line,
                                 char *ret_CallFrom, int par_MaxCharCallFrom,
                                 char *ret_Instance, int par_MaxCharInstance,
                                 char *ret_ServerName, int par_MaxCharServerName,
                                 int *ret_Priority, int *ret_CycleDivider, int *ret_Delay,
                                 int *ret_Quiet, int *ret_MaxWait,
                                 char ** ret_StartExePath, char **ret_StartExeCmdLine,
                                 int *ret_DontBreakOutOfJob,
                                 int *ret_NoGui,
                                 int *ret_Err2Msg,
                                 int *ret_NoXcp,
                                 char *ret_WriteBackExeToDir, int par_MaxCharWriteBackExeToDir)
{
    char *s, *d;
    int i;
    int quotation_mark;

    if (ret_CallFrom != NULL) ret_CallFrom[0] = 0;
    if (ret_Instance != NULL) ret_Instance[0] = 0;
    if (ret_ServerName != NULL) ret_ServerName[0] = 0;
    if (ret_Priority != NULL) *ret_Priority = -1;
    if (ret_CycleDivider != NULL) *ret_CycleDivider = -1;
    if (ret_Delay != NULL) *ret_Delay = 0;
    if (ret_Quiet != NULL) *ret_Quiet = 0;
    if (ret_MaxWait != NULL) *ret_MaxWait = 10;   // wait max. 10s till XilEnv will be start
    if (ret_StartExePath != NULL) *ret_StartExePath = NULL;
    if (ret_StartExeCmdLine != NULL) *ret_StartExeCmdLine = NULL;
    if (ret_DontBreakOutOfJob != NULL) *ret_DontBreakOutOfJob = 0;
    if (ret_NoGui != NULL) *ret_NoGui = 0;
    if (ret_Err2Msg != NULL) *ret_Err2Msg = 0;
    if (ret_NoXcp != NULL) *ret_NoXcp = 0;
    if (ret_WriteBackExeToDir != NULL) ret_WriteBackExeToDir[0] = 0;

    s = (char*)malloc (strlen (par_Line) + 1);  // this memory will not been freed
    if (s == NULL) {
        ThrowError (1, "out of memory");
        return -1;
    }
    strcpy(s, par_Line);
    while (*s != 0) {
        while (isspace (*s)) s++;
    	quotation_mark = 0;
        while (*s == '\"') {
        	s++;
            while (isspace (*s)) s++;
        	quotation_mark++;
        }
        if (*s == '-') {
            s++;
            if (*s == 'w') {
                int MaxWait;
                if (ret_Quiet != NULL) *ret_Quiet = 0;
                s++;
                MaxWait = strtol (s, &d, 10);
                if ((d == NULL) || (d == s)) MaxWait = 10;
                if (ret_MaxWait != NULL) *ret_MaxWait = MaxWait;
                s = d;
                while ((quotation_mark-- > 0) && (*s == '\"')) {
                    *s = 0;
                	s++;
                    while (isspace (*s)) s++;
                }
                while (isspace (*s)) s++;
            	quotation_mark = 0;
                while (*s == '\"') {
                	s++;
                    while (isspace (*s)) s++;
                	quotation_mark++;
                }
                if (ret_StartExePath != NULL) *ret_StartExePath = s;
                while ((*s != 0) &&    // end of line
                	   (*s != '\"') &&
                	   !((quotation_mark == 0) && isspace (*s))) {    // whitechar outside ""
                    s++;
                }
                if (quotation_mark > 0) {
					while ((quotation_mark-- > 0) && (*s == '\"')) {
						*s = 0;
						s++;
						while (isspace (*s)) s++;
					}
                } else {
					*s = 0;
					while (isspace (*s)) s++;
                }
                if (*s != 0) {
                    if (ret_StartExeCmdLine != NULL) *ret_StartExeCmdLine = s;
                } else {
                    if (ret_StartExeCmdLine != NULL) *ret_StartExeCmdLine = "";
                }
                return 0;          // After -w XX the XilEnv call must be followed (all behind will be ignored here)
            } else if (*s == 'q') {
                int MaxWait;
                if (ret_Quiet != NULL) *ret_Quiet = 1;
                s++;
                MaxWait = strtol (s, &d, 10);
                if ((d == NULL) || (d == s)) MaxWait = 10;
                if (ret_MaxWait != NULL) *ret_MaxWait = MaxWait;
                s = d;
                while ((quotation_mark-- > 0) && (*s == '\"')) {
                    *s = 0;
                	s++;
                    while (isspace (*s)) s++;
                }
                while (isspace (*s)) s++;
            	quotation_mark = 0;
                while (*s == '\"') {
                	s++;
                    while (isspace (*s)) s++;
                	quotation_mark++;
                }
                if (ret_StartExePath != NULL) *ret_StartExePath = s;
                while ((*s != 0) &&    // end of line
                	   (*s != '\"') &&
                	   !((quotation_mark == 0) && isspace (*s))) {    // whitechar outside ""
                    s++;
                }
                if (quotation_mark > 0) {
					while ((quotation_mark-- > 0) && (*s == '\"')) {
						*s = 0;
						s++;
						while (isspace (*s)) s++;
					}
                } else {
					*s = 0;
				    s++;
					while (isspace (*s)) s++;
                }
                if (*s != 0) {
                    if (ret_StartExeCmdLine != NULL) *ret_StartExeCmdLine = s;
                } else {
                    if (ret_StartExeCmdLine != NULL) *ret_StartExeCmdLine = "";
                }
                return 0;       // After -q XX the XilEnv call must be followed (all behind will be ignored here)
            } else if (*s == 'c') {
                int CycleDivider, Delay, Prio;
                s++;
                CycleDivider = strtol (s, &d, 10);
                s = d;
                if (d == NULL) CycleDivider = -1;
                else if (CycleDivider < 1) CycleDivider = 1;
                else if (CycleDivider > 0x7FFF) CycleDivider = 0x7FFF;
                if (*s == ':') {  // additional a delay?
                    s++;
                    Delay = strtol (s, &d, 10);
                    s = d;
                    if (d == NULL) Delay = -1;
                    else if (Delay < 0) Delay = 0;
                    else if (Delay > 0x7FFF) Delay = 0x7FFF;
                }
                s = d;
                if (*s == ':') {  // additional a priority?
                    s++;
                    Prio = strtol (s, &d, 10);
                    s = d;
                    if (d == NULL) Prio = -1;
                    else if (Prio < 1) Prio = 1;
                    else if (Prio > 0x7FFF) Prio = 0x7FFF;
                }
                if (ret_CycleDivider != NULL) *ret_CycleDivider = CycleDivider;
                if (ret_Delay != NULL) *ret_Delay = Delay;
                if (ret_Priority != NULL) *ret_Priority = Prio;
            } else if ((*s == 'i') || !_strnicmp ("Instance", s, strlen ("Instance"))) {    // Instance
                if (*s == 'i') s++;
                else s += strlen ("Instance");
                while (isspace (*s)) s++;
                if (ret_Instance == NULL) {
                    while ((*s != 0) && !isspace (*s)) s++;
                } else {
                    d = ret_Instance;
                    while ((*s != 0) && !isspace (*s)) {
                        if (d - ret_Instance >= par_MaxCharInstance) {
                            ThrowError (1, "the name of the instance can be only %i characters long", par_MaxCharInstance);
                            return -1;
                        }
                        *d++ = *s++;
                    }
                    *d = 0;
                }
            } else if ((*s == 's') || !_strnicmp ("Server", s, strlen ("Server"))) {    // Port
                if (*s == 's') s++;
                else s += strlen ("Server");
                while (isspace (*s)) s++;
                if (ret_ServerName == NULL) {
                    while ((*s != 0) && !isspace (*s)) s++;
                } else {
                    d = ret_ServerName;
                    while ((*s != 0) && !isspace (*s)) {
                        if (d - ret_ServerName >= par_MaxCharInstance) {
                            ThrowError (1, "the name of the server can be only %i characters long", par_MaxCharServerName);
                            return -1;
                        }
                        *d++ = *s++;
                    }
                    *d = 0;
                }
            } else if (!_strnicmp ("CallFrom", s, strlen ("CallFrom"))) {    // pass CallFrom Parameter
                s += strlen ("CallFrom");
                while (isspace (*s)) s++;
                if (ret_CallFrom == NULL) {
                    while ((*s != 0) && !isspace (*s)) s++;
                } else {
                    d = ret_CallFrom;
                    while ((*s != 0) && !isspace (*s)) {
                        if (d - ret_CallFrom >= par_MaxCharCallFrom) {
                            ThrowError (1, "the name of the call from process can be only %i characters long", par_MaxCharInstance);
                            return -1;
                        }
                        *d++ = *s++;
                    }
                    *d = 0;
                }
            } else if (!_strnicmp ("DontBreakOutOfJob", s, strlen ("DontBreakOutOfJob"))) {    // try to break out of an Eclipse Ddbug jo
                s += strlen ("DontBreakOutOfJob");
                while (isspace (*s)) s++;
                if (ret_DontBreakOutOfJob != NULL) *ret_DontBreakOutOfJob = 1;
            } else if (!_strnicmp ("nogui", s, strlen ("nogui"))) {
                s += strlen ("nogui");
                while (isspace (*s)) s++;
                if (ret_NoGui != NULL) *ret_NoGui = 1;
            } else if (!_strnicmp ("err2msg", s, strlen ("err2msg"))) {
                s += strlen ("err2msg");
                while (isspace (*s)) s++;
                if (ret_Err2Msg != NULL) *ret_Err2Msg = 1;
            } else if (!_strnicmp ("NoXcp", s, strlen ("NoXcp"))) {
                s += strlen ("NoXcp");
                while (isspace (*s)) s++;
                if (ret_NoXcp != NULL) *ret_NoXcp = 1;
            } else if (!_strnicmp ("WriteBackExeToDir", s, strlen ("WriteBackExeToDir"))) {
                s += strlen ("WriteBackExeToDir");
                while (isspace (*s)) s++;
                i = 0;
                if (ret_WriteBackExeToDir == NULL) {
                    while ((*s != 0) && ((!isspace (*s) || (i > 0)))) {
                        if (*s == '\"') i = !i;
                        s++;
                    }
                } else {
                    d = ret_WriteBackExeToDir;
                    while ((*s != 0) && ((!isspace (*s) || (i > 0)))) {
                        if (*s == '\"') {
                            i = !i;
                            s++;
                        } else {
                            if (d - ret_WriteBackExeToDir >= par_MaxCharWriteBackExeToDir) {
                                ThrowError (1, "the name of the write back to executabele path can be only %i characters long", par_MaxCharWriteBackExeToDir);
                                return -1;
                            }
                            *d++ = *s++;
                        }
                    }
                    *d = 0;
                }
            } else { // that is not for XilEnv -> ignore
                while ((*s != 0) && !isspace (*s)) s++;
            }
            while ((quotation_mark-- > 0) && (*s == '\"')) {
                *s = 0;
            	s++;
                while (isspace (*s)) s++;
            }
        } else { // that is not for XilEnv -> ignore
            while ((*s != 0) && !isspace (*s)) s++;
        }
    }
    return 0;
}
