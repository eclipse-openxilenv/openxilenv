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


#ifndef _EXTPREFVAR_H
#define _EXTPREFVAR_H

int IsCallingFromCStartup (void);
void XilEnvInternal_CStartupIsFinished (void);
void InitAddReferenceToBlackboardAtEndOfFunction (void);
void SetReferenceForProcessPostfix (char *par_ProcessPostfix);
void XilEnvInternal_DelayReferenceAllCStartupReferences (char *par_ProcessNamePostfix);
int XilEnvInternal_ReferenceVariable (void *Address, char *Name, int Type, int Dir);
int XilEnvInternal_DereferenceVariable (void *ptr, int vid);

#endif
