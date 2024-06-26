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


#include <stdint.h>
#include <stdio.h>
#include <float.h>

#include "Config.h"
#include "Scheduler.h"
#include "OscilloscopeCyclic.h"


void cyclic_oszi(void)
{
}

int init_oszi(void)
{
    return 0;
}

void terminate_oszi(void)
{
}

TASK_CONTROL_BLOCK oszi_tcb =
    INIT_TASK_COTROL_BLOCK(OSCILLOSCOPE, INTERN_ASYNC, 212, cyclic_oszi, init_oszi, terminate_oszi, 4*1024*1024);

void InitOsciCycleCriticalSection (void)
{
}
