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


#ifndef PROCESS_DEF
#define PROCESS_DEF
#define PROCESS_V10

#include "Config.h"
#include "tcb.h"


#ifndef SCHEDULER_C
extern TASK_CONTROL_BLOCK *all_known_tasks[];
#else

// Task Control Bloecke der internen Prozesse bekanntgeben
extern TASK_CONTROL_BLOCK init_tcb;
extern TASK_CONTROL_BLOCK hdrec_tcb;
extern TASK_CONTROL_BLOCK hdplay_tcb;
extern TASK_CONTROL_BLOCK wrpipe_tcb;
extern TASK_CONTROL_BLOCK rdpipe_tcb;
extern TASK_CONTROL_BLOCK async_rampen_tcb;
extern TASK_CONTROL_BLOCK sync_rampen_tcb;
extern TASK_CONTROL_BLOCK EquationCompilerTcb;
extern TASK_CONTROL_BLOCK EquationExecutionTcb;
extern TASK_CONTROL_BLOCK script_tcb;
extern TASK_CONTROL_BLOCK oszi_tcb;
extern TASK_CONTROL_BLOCK ccp_control_tcb;
extern TASK_CONTROL_BLOCK xcp_control_tcb;
extern TASK_CONTROL_BLOCK new_canserv_tcb;
extern TASK_CONTROL_BLOCK rpc_controll_tcb;
extern TASK_CONTROL_BLOCK TimeTcb;
extern TASK_CONTROL_BLOCK xcp_over_eth_tcb;
extern TASK_CONTROL_BLOCK RemoteMasterControl_Tcb;
extern TASK_CONTROL_BLOCK sync_rampen_tcb;
extern TASK_CONTROL_BLOCK rm_async_rampen_tcb;
extern TASK_CONTROL_BLOCK rm_EquationCompilerTcb;


TASK_CONTROL_BLOCK *all_known_tasks[] = {
    &init_tcb,
    &hdrec_tcb,
    &hdplay_tcb,
    &script_tcb,
    &oszi_tcb,
    &async_rampen_tcb,
    &EquationCompilerTcb,
    &ccp_control_tcb,
    &xcp_control_tcb,
    &sync_rampen_tcb,
    &EquationExecutionTcb,
    &wrpipe_tcb,
    &rdpipe_tcb,
    &new_canserv_tcb,
    &TimeTcb,
    &xcp_over_eth_tcb,
    &rpc_controll_tcb,
    &RemoteMasterControl_Tcb,
    &rm_async_rampen_tcb,
    &rm_EquationCompilerTcb,
    NULL};
#endif

#ifdef SCHEDULER_C
char pn_signal_generator_compiler[] =   SIGNAL_GENERATOR_COMPILER;
char pn_signal_generator_calculator[] = SIGNAL_GENERATOR_CALCULATOR;
char pn_equation_compiler[] =           EQUATION_COMPILER;
char pn_equation_calculator[] =         EQUATION_CALCULATOR;
char pn_stimuli_player[] =              STIMULI_PLAYER;
char pn_stimuli_queue[] =               STIMULI_QUEUE;
char pn_trace_recorder[] =              TRACE_RECORDER;
char pn_trace_queue[] =                 TRACE_QUEUE;
char pn_script[] =                      SCRIPT_PN;
char pn_oscilloscope[] =                OSCILLOSCOPE;
#endif

/*************************************************************************
**    function prototypes
*************************************************************************/

#endif
