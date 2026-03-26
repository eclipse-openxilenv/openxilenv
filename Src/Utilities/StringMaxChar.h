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


#ifndef STRINGMAXCHAR_H
#define STRINGMAXCHAR_H
#include <assert.h>
#include "MemZeroAndCopy.h"

#define STRING_COPY_TO_ARRAY(Dst, Src) static_assert(sizeof(Dst) > sizeof(void*), "this should be an array that is larger than a pointer");\
                                  StringCopyMaxCharTruncate(Dst, Src, sizeof(Dst))
#define STRING_COPY_TO_ARRAY_OFSET(Dst, Offset, Src) static_assert(sizeof(Dst) > sizeof(void*), "this should be an array that is larger than a pointer");\
                                  StringCopyMaxCharTruncate((Dst) + (Offset), Src, sizeof(Dst) - (Offset))

#define STRING_APPEND_TO_ARRAY(Dst, Src) static_assert(sizeof(Dst) > sizeof(void*), "this should be an array that is larger than a pointer");\
                                StringAppendMaxCharTruncate(Dst, Src, sizeof(Dst))

void StringCopyMaxCharTruncate(char *par_Dst, const char *par_Src, int par_MaxChar);

void StringAppendMaxCharTruncate(char *par_Dst, const char *par_Src, int par_MaxChar);

char *StringMalloc(const char *par_Src);

char *StringRealloc(char *par_Dst, const char *par_Src);

void StringFree(const char *par_Src);

#endif // STRINGMAXCHAR_H
