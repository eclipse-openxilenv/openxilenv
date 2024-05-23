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


#ifndef __XCPCTRL_H
#define __XCPCTRL_H

#include "tcb.h"

int Start_XCP (int ConnectionNo, int CalibOrMeasurment, int Count, char **Variables);
#define START_MEASSUREMENT  0
#define START_CALIBRATION   1
int Stop_XCP (int ConnectionNo, int CalibOrMeasurment);
#define STOP_MEASSUREMENT  0
#define STOP_CALIBRATION   1
int Is_XCP_AllCommandDone (int *pErrorCode);      
int Is_XCP_CommandDone (int Connection, int *pErrorCode, char **pErrText);      
int SaveConfig_XCP (int ConnectionNo, char *fname);
int LoadConfig_XCP (int ConnectionNo, char *fname);
int GetECUString_XCP (int ConnectionNo, char *String, int maxc);

extern TASK_CONTROL_BLOCK xcp_control_tcb;

/* ------------------------------------------------------------ *\
 *    Task Control Block                                     *
\* ------------------------------------------------------------ */

extern TASK_CONTROL_BLOCK xcp_control_tcb;

#endif
