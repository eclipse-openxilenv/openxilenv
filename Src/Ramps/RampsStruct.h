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


#ifndef RAMPEN_H
#define RAMPEN_H

#include "Blackboard.h"

typedef struct {
    char name[BBVARI_NAME_SIZE];
    int type;
    #define RAMPE_NORMAL   0
    #define RAMPE_ADDIEREN 1
    struct EXEC_STACK_ELEM **ampl_exec_stack;
    struct EXEC_STACK_ELEM **time_exec_stack;
    double *ampl;
    double *time;
    double steig;
    double dt;
    int count;
    int size;
    int state;
#define RAMPE_WARTET       0
#define RAMPE_LAEUFT       1
#define RAMPE_WIRD_BEENDET 3
#define RAMPE_BEENDET      4
} RAMPEN_ELEM;        /* 38 Bytes */

#define MAX_RAMPEN 100

typedef struct {
    int anz_rampen;
    int trigger_flag;
    int trigger_state;
    char trigger_var[BBVARI_NAME_SIZE];
    double trigger_schw;
    RAMPEN_ELEM rampen[MAX_RAMPEN];     /* max 100 Rampen */
    long vids[MAX_RAMPEN+3];
    unsigned long max_loops;
    unsigned long loop_count;
} RAMPEN_INFO;             /* mit MAX_RAMPEN == 50  1912 Bytes */


#endif