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


#ifndef CANRECORDER_H
#define CANRECORDER_H

#include "CanFifo.h"

struct CAN_RECORDER;

int CanRecorderReadAndProcessMessages (struct CAN_RECORDER *par_Rec);

struct CAN_RECORDER *SetupCanRecorder(const char *par_FileName,
                               const char *par_TriggerEqu,
                               int par_DisplayColumnCounterFlag,
                               int par_DisplayColumnTimeAbsoluteFlag,
                               int par_DisplayColumnTimeDiffFlag,
                               int par_DisplayColumnTimeDiffMinMaxFlag,
                               CAN_ACCEPT_MASK *par_AcceptanceMasks,   // the last must be with channel = -1
                               int par_AcceptanceMaskCount);


void StopCanRecorder(struct CAN_RECORDER *par_Rec);
void FreeCanRecorder(struct CAN_RECORDER *par_Rec);

#endif // CANRECORDER_H
