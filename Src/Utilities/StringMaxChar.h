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

#include <string.h>

void StringCopyMaxCharTruncate(char *par_Dst, const char *par_Src, int par_MaxChar);

void StringAppendMaxCharTruncate(char *par_Dst, const char *par_Src, int par_MaxChar);

char *StringMalloc(const char *par_Src);

char *StringRealloc(char *par_Dst, const char *par_Src);

void StringFree(const char *par_Src);

#ifdef USE_OWN_MEMCPY_ZEROMEM
inline void *MEMCPY(void *dst, const void *src, size_t size)
{
     volatile unsigned char *sp = (unsigned char*) src;
     volatile unsigned char *dp = (unsigned char*) dst;
     while (size--){
         *dp++ = *sp++;
     }
    return dst;
}

inline void *ZEROMEM(void *addr, size_t size)
{
     volatile unsigned char *p = (unsigned char*)addr;
     while (size--){
         *p++ = 0;
     }
     return addr;
}

#else
#ifdef __STDC_WANT_LIB_EXT1__
#define MEMCPY(dst, src, size) memcpy_s(dst, size, src, size)
#define ZEROMEM(addr, size) memset_s(addr, 0, size)
#else
#define MEMCPY(dst, src, size) memcpy(dst, src, size)
#define ZEROMEM(addr, size) memset(addr, 0, size)
#endif
#endif

#if (__STDC_VERSION__ >= 201112L)
// is C11 standard
#define STRUCT_ZERO_INIT(name, type) name = {}
#define POINT_TO_STRUCT_ZERO_INIT(pointer, type) {type HelpZeroStruct = {}; *(pointer) = HelpZeroStruct; }
#else
#ifdef _WIN32
#define STRUCT_ZERO_INIT(name, type) ZeroMemory(&name, sizeof(type))
#define POINT_TO_STRUCT_ZERO_INIT(pointer, type) ZeroMemory(pointer, sizeof(type))
#else
#define STRUCT_ZERO_INIT(name, type) memset(&name, 0, sizeof(type))
#define POINT_TO_STRUCT_ZERO_INIT(pointer, type)  memset(pointer, 0, sizeof(type))
#endif
#endif

#endif // STRINGMAXCHAR_H
