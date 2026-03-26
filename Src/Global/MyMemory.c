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


#define MYMEMORY_C
#include "Platform.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "Config.h"
#include "MyMemory.h"
#include "MemZeroAndCopy.h"
#include "PrintFormatToString.h"
#include "ThrowError.h"
#include "Scheduler.h"
#include "MainValues.h"

// If this is defined the file mem_leak.txt or rt_mem_leak.txt will be written during
// termination. This will list all not freed memory blocks
//#define DEBUG_MEM_LEAKS

#define OVERWRITE_END_OF_BLOCK_DETECTION   4

#define UNUSED(x) (void)(x)

static CRITICAL_SECTION MallocCriticalSection;

#if 0
static void DebugPrintBlock (char *Prefix, struct memory_cblock *cblock)
{
    FILE *fh;

    fh = fopen ("c:\\tmp\\memprot.txt", "at");
    if (fh != NULL) {
        fprintf (fh, "%s: ", Prefix);
        fprintf (fh, "(%i) %p[%u] %s[%i]\n", cblock->pid, cblock->block, cblock->size, cblock->file, cblock->line);
        fclose (fh);
    }
}
#else
#define DebugPrintBlock(Prefix, cblock) 
#endif

static int IsCriticalSectionInitialized;
static void CheckInitCriticalSection (void)
{
    if (!IsCriticalSectionInitialized) {
        InitializeCriticalSection (&MallocCriticalSection);
        IsCriticalSectionInitialized = 1;
    }
}


void *__my_malloc (const char * const file, int line, size_t size)
{
    struct memory_cblock *cblock;
    int x;
    unsigned char *p;

    size_t NeededSize = size + sizeof (struct memory_cblock) + OVERWRITE_END_OF_BLOCK_DETECTION;
    CheckInitCriticalSection ();
    EnterCriticalSection (&MallocCriticalSection);
    if ((cblock = (struct memory_cblock*)malloc (NeededSize)) == NULL) {
        ThrowError (1, "cannot alloc memory (%liByte)", (long)size);
        LeaveCriticalSection (&MallocCriticalSection);
        return NULL;
    }
    p = (unsigned char*)cblock + sizeof (struct memory_cblock) + size;
    for (x = 0; x < OVERWRITE_END_OF_BLOCK_DETECTION; x++) {
        p[x] = 0xA5;
    }
    cblock->next = memory_block_list;
    cblock->block = (void*)cblock;
    cblock->size = (unsigned int)size;
    cblock->pid = GetCurrentPid ();
    cblock->file = file;
    cblock->line = line;
    // Add to double linked list
    cblock->prev = NULL;
    if (memory_block_list == NULL) {
        cblock->next = NULL;
    } else {
        cblock->next = memory_block_list;
        memory_block_list->prev = cblock;
    }
    memory_block_list = cblock;

    DebugPrintBlock ("my_malloc", cblock);

    LeaveCriticalSection (&MallocCriticalSection);
    return (++cblock);
}

void *__my_calloc (const char * const file, int line, size_t nitems, size_t size)
{
    struct memory_cblock *cblock;
    int x;
    unsigned char *p;
    size_t byte_size;

    byte_size = nitems * size;
     CheckInitCriticalSection ();
    EnterCriticalSection (&MallocCriticalSection);
    if ((cblock = (struct memory_cblock*)malloc ((size_t)byte_size + sizeof (struct memory_cblock) + OVERWRITE_END_OF_BLOCK_DETECTION)) == NULL) {
        ThrowError (1, "cannot alloc memory (%iByte)", byte_size);
        LeaveCriticalSection (&MallocCriticalSection);
        return NULL;
    }
    MEMSET ((void*)cblock, 0, byte_size + sizeof (struct memory_cblock));
    p = (unsigned char*)cblock + sizeof (struct memory_cblock) + byte_size;
    for (x = 0; x < OVERWRITE_END_OF_BLOCK_DETECTION; x++) {
        p[x] = 0xA5;
    }

    cblock->next = memory_block_list;
    cblock->block = (void*)cblock;
    cblock->size = (unsigned int)byte_size;
    cblock->pid = GetCurrentPid ();
    cblock->file = file;
    cblock->line = line;
    // Add to double linked list
    cblock->prev = NULL;
    if (memory_block_list == NULL) {
        cblock->next = NULL;
    } else {
        cblock->next = memory_block_list;
        memory_block_list->prev = cblock;
    }
    memory_block_list = cblock;

    DebugPrintBlock ("my_calloc", cblock);

    LeaveCriticalSection (&MallocCriticalSection);

    return (++cblock);
}

void my_free (const void *block)
{
    union {
        const void *cv;
        struct memory_cblock *s;
    } hblock;
    int x;
    unsigned char *p;

    CheckInitCriticalSection ();
    EnterCriticalSection (&MallocCriticalSection);

    hblock.cv = block;
    hblock.s--;

    if (memory_block_list != NULL) {
        if (hblock.s != hblock.s->block) {
            LeaveCriticalSection (&MallocCriticalSection);
            ThrowError (1, "free unknown memory %p", block);
            return;
        } else {
            DebugPrintBlock ("my_free", hblock);
            hblock.s->block = NULL;
            p = (unsigned char*)hblock.s + sizeof (struct memory_cblock) + hblock.s->size;
            for (x = 0; x < OVERWRITE_END_OF_BLOCK_DETECTION; x++) {
                if (p[x] != 0xA5) {
                    ThrowError (1, "memory corruption at %p "
                              "(%i) %p[%u] %s[%i]", block, hblock.s->pid, hblock.s->block, hblock.s->size, hblock.s->file, hblock.s->line);
                    break;
                }
            }
            if (hblock.s->prev != NULL) {
                (hblock.s->prev)->next = hblock.s->next;
            }
            if (hblock.s->next != NULL) {
                (hblock.s->next)->prev = hblock.s->prev;
            }
            if (hblock.s == memory_block_list) {
                memory_block_list = hblock.s->next;
            }
            free (hblock.s);
        }
    }
    LeaveCriticalSection (&MallocCriticalSection);
}

void *__my_realloc(const char *const file, int line, void *block, size_t size)
{
    struct memory_cblock *hblock, *sblock;
    int x;
    unsigned char *p;

    if (size == 0) {
        my_free (block);
        return NULL;
    }
    if (block == NULL) return __my_malloc (file, line, size);

    CheckInitCriticalSection ();
    EnterCriticalSection (&MallocCriticalSection);

    hblock = (struct memory_cblock*)block;
    hblock--;

    p = (unsigned char*)hblock + sizeof (struct memory_cblock) + hblock->size;
    for (x = 0; x < OVERWRITE_END_OF_BLOCK_DETECTION; x++) {
        if (p[x] != 0xA5) {
            ThrowError (1, "memory corruption at %p "
                        "(%i) %p[%u] %s[%i]", block, hblock->pid, hblock->block, hblock->size, hblock->file, hblock->line);
            break;
        }
    }

    if (memory_block_list != NULL) {
        if (hblock->block != hblock) {
            ThrowError (1, "realloc unknown memory %p != %p", block, hblock->block);
            LeaveCriticalSection (&MallocCriticalSection);
            return NULL;
        }

        sblock = hblock;
        hblock = (struct memory_cblock*)realloc (hblock, size + sizeof(struct memory_cblock) + OVERWRITE_END_OF_BLOCK_DETECTION);
        if (hblock == NULL) {
            LeaveCriticalSection (&MallocCriticalSection);
            return NULL;
        } else {
            hblock->size = (unsigned int)size;
            p = (unsigned char*)hblock + sizeof (struct memory_cblock) + size;
            for (x = 0; x < OVERWRITE_END_OF_BLOCK_DETECTION; x++) {
                p[x] = 0xA5;
            }
            hblock->file = file;
            hblock->line = line;
            hblock->block = hblock;
            if (hblock->prev != NULL) (hblock->prev)->next = hblock;
            if (hblock->next != NULL) (hblock->next)->prev = hblock;
            if (sblock == memory_block_list) memory_block_list = hblock;

            DebugPrintBlock ("my_realloc", hblock);

            LeaveCriticalSection (&MallocCriticalSection);
            return (++hblock);
        }
    }
    LeaveCriticalSection (&MallocCriticalSection);
    ThrowError (1, "realloc unknown memory %p", block);

    return NULL;
}

/* Free all memory of a internal process  */
int free_process_used_memory (int pid)
{
    struct memory_cblock *cblock, *hblock, *nextblock, *lastblock = NULL;
    int memblock_count = 0;

    for (cblock = memory_block_list; cblock != NULL; cblock = nextblock) {
        nextblock = cblock->next;
        hblock = cblock;
        if (cblock->pid == pid) {
            memblock_count++;
            if (cblock == memory_block_list)
                memory_block_list = cblock->next;
            else lastblock->next = cblock->next;
            free (cblock);
        } else lastblock = hblock;
    }
    return memblock_count;
}

typedef struct {
    const char *FileName;
    int LineNr;
    int Counter;
} LINE_FILE_COUNTER;

static LINE_FILE_COUNTER *FileLineNrCounter = NULL;
static int SizeOfFileLineNrCounter = 0;

static void IncFileLineCounter(const char *FileName, int LineNr)
{
    int x;
    for (x = 0; x < SizeOfFileLineNrCounter; x++) {
        if (FileLineNrCounter[x].LineNr == LineNr) {
            if (!strcmp(FileLineNrCounter[x].FileName, FileName)) {
                 FileLineNrCounter[x].Counter++;
                 return;
            }
        }
    }
    // New entry
    SizeOfFileLineNrCounter++;
    FileLineNrCounter = (LINE_FILE_COUNTER*)realloc(FileLineNrCounter, sizeof(LINE_FILE_COUNTER) * (size_t)SizeOfFileLineNrCounter);
    FileLineNrCounter[x].LineNr = LineNr;
    FileLineNrCounter[x].FileName = FileName;
    FileLineNrCounter[x].Counter = 1;
}

static int SearchSortCompareFunction (void const* a, void const* b)
{
    int Ret = strcmp(((const LINE_FILE_COUNTER*)a)->FileName,
                     ((const LINE_FILE_COUNTER*)b)->FileName);
    if (Ret == 0) {
        return ((const LINE_FILE_COUNTER*)a)->LineNr - ((const LINE_FILE_COUNTER*)b)->LineNr;
    }
    return Ret;
}

void check_memory_corrupted (int LineNr, char *Filename)
{
    struct memory_cblock *cblock, *nextblock;
    int x;
    unsigned char *p;
    int Counter = 0;

    for (cblock = memory_block_list; cblock != NULL; cblock = nextblock) {
        Counter++;
        nextblock = cblock->next;
        p = (unsigned char*)cblock + sizeof (struct memory_cblock) + cblock->size;
        for (x = 0; x < OVERWRITE_END_OF_BLOCK_DETECTION; x++) {
            if (p[x] != 0xA5) {
                ThrowError (1, "memory corruption at block number %i detectet in %s (%i)"
                            "(%i) %p[%u] %s[%i]", Counter, Filename, LineNr, cblock->pid, cblock->block, cblock->size, cblock->file, cblock->line);
                return;
            }
        }
    }
}

int init_memory (void)
{
    CheckInitCriticalSection ();
    return 0;
}

#define MEMORY_LEAK_FILE "c:\\temp\\xilenv_mem_leak.txt"
uint64_t terminate_memory (void)
{
#ifdef DEBUG_MEM_LEAKS
    s_main_ini_val.WriteMemoryLeakFile = 1;
#endif
    struct memory_cblock *cblock, *nextblock;
    uint64_t total_mem = 0;
    FILE *fh;
    if (s_main_ini_val.WriteMemoryLeakFile) {
        if (memory_block_list != NULL) {
            fh = fopen (MEMORY_LEAK_FILE, "wt");
            if (fh != NULL) {
                fprintf (fh, "Memory leaks in XilEnv:\n");
                for (cblock = memory_block_list; cblock != NULL; cblock = nextblock) {
                    nextblock = cblock->next;
                    fprintf (fh, "(%i) %p[%u] %s[%i]\n", cblock->pid, cblock->block, cblock->size, cblock->file, cblock->line);
                    total_mem += cblock->size;
                }
                fprintf (fh, "total = %" PRIu64 "Byte\n", total_mem);
                fclose (fh);
            }
        } else {
            remove (MEMORY_LEAK_FILE);
        }
    }
    return total_mem;
}


int DebugCheckIfIsHeapx (void *par_Address)
{
#ifdef _WIN32
    _HEAPINFO hinfo;
    int heapstatus;
    int Ret = 0;
    union {
        const void *cv;
        struct memory_cblock *s;
    } hblock;

    hblock.cv = par_Address;
    hblock.s--;

    hinfo._pentry = NULL;
    while ((heapstatus = _heapwalk (&hinfo)) == _HEAPOK) {
        if (hinfo._useflag == _USEDENTRY) {
            if (((char*)hinfo._pentry + 0x30) == (char*)hblock.s) {
                if ((uint32_t)hinfo._size < hblock.s->size) {
                    ThrowError(1, "Internal error: %s (%i)", __FILE__, __LINE__);
                }
                Ret = 1;
            }
        }
    }
    switch (heapstatus) {
    case _HEAPEMPTY:
        printf ("OK - empty heap\n" );
        break;
    case _HEAPEND:
        printf ("OK - end of heap\n" );
        break;
    case _HEAPBADPTR:
        printf ("ERROR - bad pointer to heap\n" );
        Ret = -1;
        break;
    case _HEAPBADBEGIN:
        printf ("ERROR - bad start of heap\n" );
        Ret = -1;
        break;
    case _HEAPBADNODE:
        printf ("ERROR - bad node in heap\n" );
        Ret = -1;
        break;
    }
    return Ret;
#else
    UNUSED(par_Address);
    return -1;
#endif
}
