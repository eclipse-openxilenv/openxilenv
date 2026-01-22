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


#ifndef PRINTTOSTRING_H
#define PRINTTOSTRING_H

#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif
int PrintFormatToString(char *ret_DestBuffer, int par_SizeOfDestBuffer, const char *par_Format, ...);
int VariableArgumentsListPrintFormatToString(char *ret_DestBuffer, int par_SizeOfDestBuffer, const char *par_Format,  va_list vlist);
#ifdef __cplusplus
}
#endif

#endif // PRINTTOSTRING_H
