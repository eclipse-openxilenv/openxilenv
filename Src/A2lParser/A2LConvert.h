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


#pragma once
#include "A2LBuffer.h"
#include "A2LValue.h"

int ConvertPhysToRaw(ASAP2_MODULE_DATA* Module, const char *par_ConvertName, A2L_SINGLE_VALUE *par_Phys, A2L_SINGLE_VALUE *ret_Raw);
int ConvertRawToPhys(ASAP2_MODULE_DATA* Module, const char *par_ConvertName, int par_Flags, A2L_SINGLE_VALUE *par_Raw, A2L_SINGLE_VALUE *ret_Phys);
