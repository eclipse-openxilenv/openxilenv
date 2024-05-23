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


#ifndef _PARSECMDLINE_H
#define _PARSECMDLINE_H

int  XilEnvInternal_ParseCmdLine (char *par_Line,
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
                                  char *ret_XcpExeWriteBackPath, int par_MaxCharXcpExeWriteBackPath);

#endif
