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
#include <malloc.h>
#include "ThrowError.h"
#include "StringMaxChar.h"
#include "MemoryAllocation.h"

// only for debugging
//static void PrintMemoryInfos(char *Postfix);
//static void CheckMemoryList(void);


typedef struct MCB {
    //unsigned char OverwriteCheckBlock[8];
    struct MCB* next;
    struct MCB* prev;
    struct MCB* free_list_next;
    struct MCB* free_list_prev;
    char *file;
    int line;
    long length;  // enthaelt die Laenge ohne sizeof(struct MCB)
    int pid;
    short used;
} Memory_Control_Block;

#include "RemoteMasterLock.h"

static REMOTE_MASTER_LOCK MallocCriticalSection;
#define InitializeCriticalSection(x) RemoteMasterInitLock(x)
#define EnterCriticalSection(x)  RemoteMasterLock(x, __LINE__, __FILE__)
#define LeaveCriticalSection(x)  RemoteMasterUnlock(x)


static int process_identifier;  // damit es mal compiliert

                                //	char mem_block[BLOCK_SIZE];
//static struct MCB* first_mcb_ptr = NULL;
//static unsigned long anzahl_mcbs = 1;
//static long RealtimeHeapMemorySize;

static struct MCB* FreeLists[32];


/*
static void FillOverwriteCheckBlock(struct MCB *MemBlock)
{
    int x;
    for (x = 0; x < 8; x++) {
        MemBlock->OverwriteCheckBlock[x] = 0xA5;
    }
}

static int CheckOverwriteCheckBlock(struct MCB *MemBlock)
{
    int x;
    for (x = 0; x < 8; x++) {
        if (MemBlock->OverwriteCheckBlock[x] != 0xA5) {
            error(1, "memory curruption!!!");
            return 1;
        }
    }
    return 0;
}
*/

static struct MCB * GetMemoryBlockBeforeFree(struct MCB *MemBlock)
{
    if (MemBlock->prev != NULL) {
        if (!MemBlock->prev->used) {
            return MemBlock->prev;
        }
    }
    return NULL;
}

static struct MCB *GetMemoryBlockBehindFree(struct MCB *MemBlock)
{
    if (MemBlock->next != NULL) {
        if (!MemBlock->next->used) {
            return MemBlock->next;
        }
    }
    return NULL;
}

void CutMemoryBlockOutOfFreeList(struct MCB *MemBlock)
{
    if (MemBlock->free_list_next != NULL) {
        MemBlock->free_list_next->free_list_prev = MemBlock->free_list_prev;
    }
    if (MemBlock->free_list_prev != NULL) {
        MemBlock->free_list_prev->free_list_next = MemBlock->free_list_next;
    } else {
        // Letzes Element in einer Free-Liste
        int x;
        for (x = 0; x < 31; x++) {
            if (FreeLists[x] == MemBlock) {
                FreeLists[x] = NULL;   // Free-List ist jetzt leer
                break;
            }
        }
    }
}

static struct MCB *StickTogetherMemoryBlock(struct MCB *MemBlock1, struct MCB *MemBlock2)
{
    if ((MemBlock1 >= MemBlock2) ||
        (MemBlock1->used != 0) || 
        (MemBlock2->used != 0)) {
        ThrowError(1, "Internal alloc error %s(%i)", __FILE__, __LINE__);
    } else {
        MemBlock1->length += MemBlock2->length + sizeof(struct MCB);
        MemBlock1->next = MemBlock2->next;
        if (MemBlock1->next != NULL) {
            MemBlock1->next->prev = MemBlock1;
        }
    }
    return MemBlock1;
}

static void AddMemoryBlockToFreeList(struct MCB *MemBlock)
{
    int x;
    MemBlock->used = 0;
    for (x = 0; x < 31; x++) {
        if (MemBlock->length <= (1 << (x+1))) { // 8, 16, 32, ...
            MemBlock->free_list_prev = NULL;
            MemBlock->free_list_next = FreeLists[x];
            if (FreeLists[x] != NULL) FreeLists[x]->free_list_prev = MemBlock;
            FreeLists[x] = MemBlock;
            break;
        }
    }
}

static struct MCB* FetchFreeMemoryBlock(int Size)
{
    int x;
    for (x = 0; x < 31; x++) {
        if (Size <= (1 << x)) {   // 8, 16, 32, ...
            if (FreeLists[x] != NULL) {
                struct MCB* Ret = FreeLists[x];
                FreeLists[x] = FreeLists[x]->free_list_next;
                if (FreeLists[x] != NULL) {
                    FreeLists[x]->free_list_prev = NULL;
                }
                Ret->used = 1;
                return Ret;
            }
        }
    }
    return NULL;
}


static void TrimmAndSplitMemoryBlock(struct MCB* MemBlock, int Size)
{
    if ((MemBlock->length - Size) >= (8 + sizeof(struct MCB))) {
        struct MCB* NewMemBlock = (struct MCB*)((char*)(MemBlock + 1) + Size);
        NewMemBlock->length = MemBlock->length - Size - sizeof(struct MCB);
        NewMemBlock->used = 0;
        MemBlock->length = Size;

        // in Liste einhaengen
        NewMemBlock->prev = MemBlock;
        NewMemBlock->next = MemBlock->next;
        if (NewMemBlock->next != NULL) NewMemBlock->next->prev = NewMemBlock;
        MemBlock->next = NewMemBlock;

        //FillOverwriteCheckBlock(NewMemBlock);
        AddMemoryBlockToFreeList(NewMemBlock);
    }
}


static char *BaseAddressOfRealtimeMemory;
uint64_t SizeOfRealtimeMemory;

#if 0
static void *(*old_malloc_hook)(size_t size, const void *caller);

static void *my_malloc_hook(size_t size, const void *caller)
{
    return my_malloc(size);
}

static void *(*old_realloc_hook)(void *pointer, size_t size, const void *caller);

static void *my_realloc_hook(void *pointer, size_t size, const void *caller)
{
    if ((pointer < BaseAddressOfRealtimeMemory) || (pointer >= (BaseAddressOfRealtimeMemory + SizeOfRealtimeMemory))) {
        void *ret;
        // ist ein Speicher Block der auf dem normalen Heap liegt!
        printf("realloc a none realtime memory block\n");
        __realloc_hook = old_realloc_hook;
        ret = old_realloc_hook(pointer, size, caller);
        __realloc_hook = my_realloc_hook;
    }
    else {
        return my_realloc(pointer, size);
    }
}

static void (*old_free_hook)(void *pointer);

static void my_free_hook(void *pointer)
{
    if ((pointer < BaseAddressOfRealtimeMemory) || (pointer >= (BaseAddressOfRealtimeMemory + SizeOfRealtimeMemory))) {
        // ist ein Speicher Block der auf dem normalen Heap liegt!
        /*printf("free a none realtime memory block\n");
         __free_hook = old_free_hook;
         free(pointer);
        __free_hook = my_free_hook;*/
    } else {
        my_free(pointer);
    }
}
#endif

int InitMemoryAllocator(int SizeOfMemory)
{
    InitializeCriticalSection(&MallocCriticalSection);
    struct MCB* FirstMemBlock = malloc(SizeOfMemory);

    if (FirstMemBlock != NULL) {
        BaseAddressOfRealtimeMemory = (char*)FirstMemBlock;
        SizeOfRealtimeMemory = SizeOfMemory;

        FirstMemBlock->length = SizeOfMemory - sizeof(struct MCB);
        FirstMemBlock->used = 0;
        FirstMemBlock->free_list_next = NULL;
        FirstMemBlock->free_list_prev = NULL;
        FirstMemBlock->next = NULL;
        FirstMemBlock->prev = NULL;

        //FillOverwriteCheckBlock(FirstMemBlock);

        AddMemoryBlockToFreeList(FirstMemBlock);
        // nun setze die malloc, realloc und free Funktionen
#if 0
        old_malloc_hook = __malloc_hook;
        __malloc_hook = my_malloc_hook;
        old_realloc_hook = __realloc_hook;
        __realloc_hook = my_realloc_hook;
        old_free_hook = __free_hook;
        __free_hook = my_free_hook;
#endif
        return 0;
    } else {
        return -1;
    }
}



static uint64_t Get_RDTSC(void)
{
    unsigned int hi, lo;
    __asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

static uint64_t malloc_start_time;
uint64_t malloc_last_runtime;
int malloc_loop_counter;

/*************************************************************************
**
**    Function    : my_malloc
**
**    Description : simplified malloc-function
**    Parameter   : size
**    Returnvalues: pointer to the allocated memory block
**
******************************************/
void* __my_malloc_pid(int pid, char *file, int line, long size)
{
    struct MCB* mcb;

    if (size == 0) return NULL;

    // 8Byte alligned
    size += 7;
    size &= 0xfffffffffffffff8LLU;

    EnterCriticalSection(&MallocCriticalSection);
    malloc_start_time = Get_RDTSC();
    mcb = FetchFreeMemoryBlock(size);
    if (mcb == NULL) {
        LeaveCriticalSection(&MallocCriticalSection);
        return NULL;
    }
    TrimmAndSplitMemoryBlock(mcb, size);
    mcb->file = file;
    mcb->line = line;
    LeaveCriticalSection(&MallocCriticalSection);
    return (void*)(mcb + 1);
}

void* __my_malloc(char *file, int line, long size)
{
    //return malloc(size);
    return __my_malloc_pid(process_identifier, file, line, size);
}

/*************************************************************************
**
**    Function    : my_free
**
**    Description : simplified free-function
**    Parameter   : pointer of the block to free
**    Returnvalues: -
**
******************************************/
void my_free_pid(int pid, void* block_ptr)
{
    struct MCB* mcb;
    struct MCB* mcb_before;
    struct MCB* mcb_behind;

    if (block_ptr == NULL) return;

    EnterCriticalSection(&MallocCriticalSection);

    mcb = (struct MCB*)block_ptr - 1;
    mcb->used = 0;
    mcb_before = GetMemoryBlockBeforeFree(mcb);
    if (mcb_before != NULL) {
        CutMemoryBlockOutOfFreeList(mcb_before);
        mcb = StickTogetherMemoryBlock(mcb_before, mcb);
    }
    mcb_behind = GetMemoryBlockBehindFree(mcb);
    if (mcb_behind != NULL) {
        CutMemoryBlockOutOfFreeList(mcb_behind);
        mcb = StickTogetherMemoryBlock(mcb, mcb_behind);
    }

    AddMemoryBlockToFreeList(mcb);

    LeaveCriticalSection(&MallocCriticalSection);
}

void my_free(void* block_ptr)
{
    //free(block_ptr);
    my_free_pid(process_identifier, block_ptr);
}

/*************************************************************************
**
**    Function    : my_realloc
**
**    Description : simplified realloc-function
**    Parameter   : new size
**    Returnvalues: pointer to the allocated memory block
**
******************************************/
void* __my_realloc(char *file, int line, void* old_ptr, long new_size)
{
    /*void * new_ptr = NULL;
    new_ptr = __my_malloc(file, line, new_size);
    MEMSET(new_ptr, 0, new_size);
    if (old_ptr != NULL) {
        MEMCPY(new_ptr, old_ptr, new_size);
        my_free(old_ptr);
    }
    return new_ptr;*/
    return __my_realloc_pid(process_identifier, file, line, old_ptr, new_size);
}

void* __my_realloc_pid(int pid, char *file, int line, void* old_ptr, long new_size)
{
    //return (realloc(old_ptr, new_size));
    void * new_ptr = NULL;
    new_ptr = __my_malloc_pid(pid, file, line, new_size);
    MEMSET(new_ptr, 0, new_size);
    if (old_ptr != NULL) {
		struct MCB* mcb_ptr = NULL;
		mcb_ptr = (struct MCB*)old_ptr - 1;
		if (mcb_ptr->length <= new_size) {
			MEMCPY(new_ptr, old_ptr, mcb_ptr->length);
		}
		else {
			MEMCPY(new_ptr, old_ptr, new_size);
		}
		my_free_pid(pid, old_ptr);
    }
    return new_ptr;
}

/*************************************************************************
**
**    Function    : my_calloc
**
**    Description : simplified calloc-function
**    Parameter   : num, size
**    Returnvalues: pointer to the allocated memory block
**
******************************************/
void* __my_calloc(char *file, int line, long num, long size)
{
    //return calloc(num, size);
    void * block_ptr = NULL;
    block_ptr = __my_malloc(file, line, num * size);
    if (block_ptr != NULL) {
        MEMSET(block_ptr, 0, num * size);
    }
    return block_ptr;
}

void* __my_calloc_pid(int pid, char *file, int line, long num, long size)
{
    //return calloc(num, size);
    void * block_ptr = NULL;
    block_ptr = __my_malloc_pid(pid, file, line, num * size);
    if (block_ptr != NULL) {
        MEMSET(block_ptr, 0, num * size);
    }
    return block_ptr;
}

