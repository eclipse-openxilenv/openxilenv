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

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "RemoteMasterLock.h"

#define gettid() syscall(__NR_gettid)

static uint64_t my_rdtsc(void) {
	uint64_t a, d;
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
	return (d << 32) | a;
}

void RemoteMasterLock(REMOTE_MASTER_LOCK *Lock, int LineNr, const char *File)
{
	uint64_t diff;
	uint64_t start;
	int CpuNr;
    int ThreadId;

    ThreadId = gettid();
	CpuNr = sched_getcpu();
	start = my_rdtsc();
    pthread_mutex_lock(&(Lock->Mutex));
	if (Lock->LockCounter != 0) {
		printf("LockCounter = %i\n", Lock->LockCounter);
	}
	Lock->LockCounter++;
	diff = my_rdtsc() - start;
	if (diff > Lock->MaxWaitTime) {
		Lock->MaxWaitTime = diff;
		if (diff > 30000) {
            printf("max mait time = %llu %s(%i)\n", Lock->MaxWaitTime, File, LineNr);
		}
	}
	Lock->DebugLineNr = LineNr;
	Lock->DebugFileName = File;
	Lock->CpuNr = CpuNr;
    Lock->ThreadId = ThreadId;
}

void RemoteMasterUnlock(REMOTE_MASTER_LOCK *Lock)
{
	Lock->CpuNr = -1;
	Lock->LockCounter--;
    pthread_mutex_unlock(&(Lock->Mutex));
}

void RemoteMasterInitLock(REMOTE_MASTER_LOCK *Lock)
{
    pthread_mutex_init(&(Lock->Mutex), NULL);
	Lock->DebugLineNr = 0;
	Lock->DebugFileName = NULL;
	Lock->MaxWaitTime = 0;
	Lock->LockCounter = 0;
	Lock->CpuNr = -1;
}

void RemoteMasterDestroyLock(REMOTE_MASTER_LOCK *Lock)
{
    pthread_mutex_destroy(&(Lock->Mutex));
}
