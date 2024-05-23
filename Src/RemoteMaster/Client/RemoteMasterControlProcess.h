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


#ifndef REMOTEMASTERCONTROLPROCESS_H
#define REMOTEMASTERCONTROLPROCESS_H

#include "tcb.h"
#include "ReadCanCfg.h"

int GetCanCardType (int Channel);
char *GetCanCardName (int Channel);
char *GetFlexrayCardName (int Channel);

extern TASK_CONTROL_BLOCK RemoteMasterControl_Tcb;

#endif // REMOTEMASTERCONTROLPROCESS_H
