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


#ifndef STIMULUSPLAYER_H
#define STIMULUSPLAYER_H

#include <stdlib.h>
#include <stdio.h>
#include "tcb.h"
#include "Blackboard.h"
#include "ReadFromBlackboardPipe.h"


#define HDPLAY_MAX_VARIS  (8*1024)

/* HD-Recorder states */
#define HDPLAY_SLEEP      1
#define HDPLAY_FILE_NAME  2
#define HDPLAY_PLAY       3

/* ------------------------------------------------------------ *\
 *    Task Control Block                                     *
\* ------------------------------------------------------------ */

extern TASK_CONTROL_BLOCK hdplay_tcb;
#endif
