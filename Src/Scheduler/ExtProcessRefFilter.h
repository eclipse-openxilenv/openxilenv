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


#ifndef EXTPROCESSREFFILTER_H
#define EXTPROCESSREFFILTER_H

#define MAX_EXT_PROCESS_REF_FILTERS   32



int BuildExternProcessReferenceFilter(int par_Pid, const char *par_ProcessPath);

void FreeExternProcessReferenceFilter(int par_Pid);

int ExternProcessReferenceMatchFilter (int par_Filter, const char *par_RefName);

void InitExternProcessReferenceFilters(void);

#endif // EXTPROCESSREFFIILTER_H
