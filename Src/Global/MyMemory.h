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


#ifndef MYMEMORY_H
#define MYMEMORY_H

#include <stdint.h>
#include <stdio.h>

#ifdef MYMEMORY_C
#define extern
#endif

struct memory_cblock {
    void *block;                   /* Pointer to this memory block */
    unsigned int size;             /* Size of this memory block */
    int pid;                       /* Process Id of the process which have requested this memory block */
    const char *file;
    int line;
    struct memory_cblock *next;    /* next memory block */
    struct memory_cblock *prev;    /* previous memory block */
};

/* ------------------------------------------------------------ *\
 *    Global variables 	                    		        *
\* ------------------------------------------------------------ */

/* Start of the memory block list */
extern struct memory_cblock *memory_block_list
#ifdef MEMORY_C
= NULL
#endif
;

/* ------------------------------------------------------------ *\
 *    Global Functions                                          *
\* ------------------------------------------------------------ */

#ifdef __cplusplus
#define my_malloc(s) __my_malloc (__FILE__, __LINE__, (s))
#else
#define my_malloc(s) __my_malloc (__FILE__, __LINE__, (size_t)(s))
#endif
extern void *__my_malloc (const char * const file, int line, size_t size);
#ifdef __cplusplus
#define my_realloc(b, s) __my_realloc (__FILE__, __LINE__, b, (s))
#else
#define my_realloc(b, s) __my_realloc (__FILE__, __LINE__, b, (size_t)(s))
#endif
extern void *__my_realloc(const char * const file, int line, void *block, size_t size);

#ifdef __cplusplus
#define my_calloc(i, s) __my_calloc (__FILE__, __LINE__, i, (s))
#else
#define my_calloc(i, s) __my_calloc (__FILE__, __LINE__, (size_t)i, (size_t)(s))
#endif
extern void *__my_calloc (const char * const file, int line, size_t nitems, size_t size);

extern void my_free (const void *block);

extern int test_memory (void);

extern int init_memory (void);
extern uint64_t terminate_memory(void);

extern int free_process_used_memory (int pid);

void check_memory_corrupted (int LineNr, char *Filename);
#define CHECK_MEMORY_CORRUPTED() check_memory_corrupted (__LINE__, (char*)__FILE__)

uint64_t write_memory_infos_to_file(int Idx);

int DebugCheckIfIsHeap (void *par_Address);

#undef extern
#endif
