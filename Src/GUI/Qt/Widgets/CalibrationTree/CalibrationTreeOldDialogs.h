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


#ifndef CALLIBRATIONTREEOLDDIALOGS_H
#define CALLIBRATIONTREEOLDDIALOGS_H

#include "Wildcards.h"
#include "DebugInfos.h"

typedef struct {
    char *process_name;
    int pid;
    char *winname;
    char *filter;
    int ConstOnly;
    int show_values;
    int hex_or_dec;
    INCLUDE_EXCLUDE_FILTER *IncExcFilter;

    DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata;
    int UniqueNumber;

} CALLIBRATION_CONFIG;

int parameter_reference_dlg (CALLIBRATION_CONFIG *papplwin);

#endif // CALLIBRATIONTREEOLDDIALOGS_H

