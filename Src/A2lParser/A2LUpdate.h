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
#include "A2LAccess.h"

int A2LUpdate(ASAP2_DATABASE *Database, const char *par_OutA2LFile, const char *par_SourceType, int par_Flags, const char *par_Source,
              uint64_t par_MinusOffset, uint64_t par_PlusOffset,
              const char *par_NotUpdatedLabelFile, const char **ret_ErrString);
