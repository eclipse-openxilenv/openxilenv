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


#include <string.h>
#include "XilEnvExtProc.h"
#include "Platform.h"
#include <malloc.h>
#include "ExtpMemoryAllocation.h"


int XilEnvInternal_InitMemoryAllocator(void)
{
    return 0;
}

void* XilEnvInternal_malloc(int size)
{
    return malloc(size);
}

void  XilEnvInternal_free(void * block_ptr)
{
    free(block_ptr);
}

void* XilEnvInternal_realloc(void* old_ptr, int new_size)
{
    return realloc(old_ptr, new_size);
}

void* XilEnvInternal_calloc(int num, int size)
{
    return calloc(num, size);
}

#define UNUSED(x) (void)(x)

void *__my_malloc (const char * const file, int line, size_t size)
{
    UNUSED(file);
    UNUSED(line);
    return malloc (size);
}

void *__my_calloc (const char * const file, int line, size_t nitems, size_t size)
{
    UNUSED(file);
    UNUSED(line);
    return calloc (nitems, size);
}

void my_free (void* block)
{
    free (block);
}

void *__my_realloc(const char * const file, int line, void *block, size_t size)
{
    UNUSED(file);
    UNUSED(line);
    return realloc (block, size);
}
