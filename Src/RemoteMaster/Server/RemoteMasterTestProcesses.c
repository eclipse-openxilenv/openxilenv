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


#include "rtextp.h"
#include "wsc_10.h"

#if 0

static int counter;

void test1_cyclic(void)
{
    RT_FILE *fh;

    counter++;
    if ((counter % 500) == 0) {
        sc_error(1, "1 = %d, 2 = %d, 0xFFFFFFFF = 0x%X", 1, 2, 0xFFFFFFFFUL);
        sc_error(1, "test1_cycle() called %i times", counter);
    }
    if (counter == 1000) {
        printf("request the file handle\n");
        rt_fopen("c:\\tmp\\schrott.txt");
    }
    if (fh = get_rt_file_stream()) {
        printf("get the file handle\n");
    }
}

int test1_init (void)
{
    sc_error(1, "test1_init() called");
    return 0;
}

void test1_terminate(void)
{
}

TASK_CONTROL_BLOCK test1_tcb
= {
    -1,                       /* Prozess nicht aktiv */
    "Test1",             /* Prozess-Name */
    1,
    0,                       /* Prozess Schlaeft = 0 */
    0,
    111,                      /* Prioritaet */
    test1_cyclic,
    test1_init,
    test1_terminate,
    0,                       /* N.C. */
    0,                       /* timecounter zu Anfang 0 */
    1,                       /* Prozess in jedem Zeittakt aufrufen */
    16 * 1024               /* 16Kytes Message queue */
};

void test2_cyclic(void)
{
}

int test2_init(void)
{
    return 0;
}

void test2_terminate(void)
{
}

TASK_CONTROL_BLOCK test2_tcb
= {
    -1,                       /* Prozess nicht aktiv */
    "Test2",             /* Prozess-Name */
    1,
    0,                       /* Prozess Schlaeft = 0 */
    0,
    222,                      /* Prioritaet */
    test2_cyclic,
    test2_init,
    test2_terminate,
    0,                       /* N.C. */
    0,                       /* timecounter zu Anfang 0 */
    1,                       /* Prozess in jedem Zeittakt aufrufen */
    16 * 1024               /* 16Kytes Message queue */
};

void test3_cyclic(void)
{
}

int test3_init(void)
{
    return 0;
}

void test3_terminate(void)
{
}

TASK_CONTROL_BLOCK test3_tcb
= {
    -1,                       /* Prozess nicht aktiv */
    "Test3",             /* Prozess-Name */
    1,
    0,                       /* Prozess Schlaeft = 0 */
    0,
    333,                      /* Prioritaet */
    test3_cyclic,
    test3_init,
    test3_terminate,
    0,                       /* N.C. */
    0,                       /* timecounter zu Anfang 0 */
    1,                       /* Prozess in jedem Zeittakt aufrufen */
    16 * 1024               /* 16Kytes Message queue */
};

#endif
