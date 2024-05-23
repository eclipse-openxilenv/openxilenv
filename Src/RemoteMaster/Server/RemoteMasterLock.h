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

#include <stdint.h>
#include <pthread.h>

typedef struct {
    pthread_mutex_t Mutex;
    int ThreadId;
	int DebugLineNr;
    const char *DebugFileName;
	uint64_t MaxWaitTime;
	int CpuNr;
	int LockCounter;
} REMOTE_MASTER_LOCK;

void RemoteMasterLock(REMOTE_MASTER_LOCK *Lock, int LineNr, const char *File);

void RemoteMasterUnlock(REMOTE_MASTER_LOCK *Lock);

void RemoteMasterInitLock(REMOTE_MASTER_LOCK *Lock);

void RemoteMasterDestroyLock(REMOTE_MASTER_LOCK *Lock);
