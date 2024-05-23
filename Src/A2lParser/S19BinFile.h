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

#include <stdint.h>
#include "A2LValue.h"

typedef struct {
    uint64_t Size;
    uint64_t Offset;
    char *Filename;
    unsigned char *Image;
}  BIN_FILE_IMAGE;

BIN_FILE_IMAGE *LoadBinFile(const char *par_FileName);
BIN_FILE_IMAGE *LoadS19File(const char *par_FileName, uint64_t par_Offset, uint64_t par_Size);

int ReadValueFromImage(BIN_FILE_IMAGE *par_Image, uint64_t par_Address, uint32_t par_Flags, int par_Type, A2L_SINGLE_VALUE *ret_Value);

void CloseBinImage(BIN_FILE_IMAGE *par_Image);

