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


#ifndef __CCPCTRL_H
#define __CCPCTRL_H

#include "tcb.h"

int Start_CCP (int ConnectionNo, int CalibOrMeasurment, int Count, char **Variables);
#define START_MEASSUREMENT  0
#define START_CALIBRATION   1
int Stop_CCP (int ConnectionNo, int CalibOrMeasurment);
#define STOP_MEASSUREMENT  0
#define STOP_CALIBRATION   1
int Is_CCP_CommandDone (int ConnectionNo, int *pErrorCode, char **pErrText);      
int Is_CCP_AllCommandDone (int *pErrorCode);
int SaveConfig_CCP (int ConnectionNo, char *fname);
int LoadConfig_CCP (int ConnectionNo, const char *fname);

int GetECUInfos_CCP (int ConnectionNo, char *String, int maxc);
int GetECUString_CCP (int ConnectionNo, char *String, int maxc);

/* ------------------------------------------------------------ *\
 *    Task Control Block                                     *
\* ------------------------------------------------------------ */

extern TASK_CONTROL_BLOCK ccp_control_tcb;

#endif
