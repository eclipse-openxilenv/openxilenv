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

#define A2LCONVERT2XCP_WRITABLE_MEASUREMENTS_AS_PARAMETER    0x1

int A2LConvertToXcpOrCpp(const char *par_A2LFile, const char *par_XcpOrCppFile, int par_XcpOrCpp, int par_Flags, char *ErrString, int MaxErrSize);
