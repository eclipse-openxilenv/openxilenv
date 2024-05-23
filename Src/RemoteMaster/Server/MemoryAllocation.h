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

int InitMemoryAllocator(int SizeOfMemory);

#define my_malloc(s) __my_malloc (__FILE__, __LINE__, s)
#define my_malloc_pid(pid, s) __my_malloc_pid (pid, __FILE__, __LINE__, s)
#define my_realloc(b, s) __my_realloc (__FILE__, __LINE__, b, s)
#define my_realloc_pid(pid, b, s) __my_realloc_pid (pid, __FILE__, __LINE__, b, s)
#define my_calloc(n, s) __my_calloc (__FILE__, __LINE__, n, s)
#define my_calloc_pid(pid, n, s) __my_calloc_pid (pid, __FILE__, __LINE__, n, s)

void* __my_malloc(char *file, int line, long size);
void* __my_malloc_pid(int pid, char *file, int line, long size);
void  my_free(void * block_ptr);
void  my_free_pid(int pid, void * block_ptr);
void* __my_realloc(char *file, int line, void* old_ptr, long new_size);
void* __my_realloc_pid(int pid, char *file, int line, void* old_ptr, long new_size);
void* __my_calloc(char *file, int line, long num, long size);
void* __my_calloc_pid(int pid, char *file, int line, long num, long size);

//int get_next_allocated_rt_memory_block(int flag, int *pid, void **ptr, int *size, char *file, int *line);
