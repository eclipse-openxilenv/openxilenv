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


#ifndef EXTERNLOGINTIMEOUTCONTROL_H
#define EXTERNLOGINTIMEOUTCONTROL_H

extern volatile int WaitForAllLoginCompleteFlag;

void InitExternProcessTimeouts (void);

int AddExecutableToTimeoutControl (const char *par_ExecutableName, int par_Timeout);
int RemoveExecutableFromTimeoutControl (const char *par_ExecutableName);

int LoginProcess (char *par_ExecutableName, int par_Number, int par_Count);


#define WAIT_FOR_ALL_LOGIN_COMPLETE() if (WaitForAllLoginCompleteFlag) WaitForAllLoginComplete();

void WaitForAllLoginComplete(void);

#endif
