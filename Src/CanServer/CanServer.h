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


#ifndef __NEWCANS2_H
#define __NEWCANS2_H

#include "tcb.h"

int SendCanObjectForOtherProcesses (int Channel, unsigned int Id, int Ext, int Size, unsigned char *Data);
int Mixed11And29BitIdsAllowed (int Channel);
uint32_t GetCanServerCycleTime_ms(void);

void DecodeJ1939RxMultiPackageFrame(NEW_CAN_SERVER_CONFIG *csc, int Channel, int ObjectPos);


extern TASK_CONTROL_BLOCK new_canserv_tcb;

#endif
