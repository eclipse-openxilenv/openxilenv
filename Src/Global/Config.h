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


#ifndef _CONFIG_H
#define _CONFIG_H

#define XILENV_VERSION               0
#define XILENV_MINOR_VERSION         8
#define XILENV_PATCH_VERSION         16
#define XILENV_INIFILE_MIN_VERSION           0
#define XILENV_INIFILE_MIN_MINOR_VERSION     8
#define XILENV_INIFILE_MIN_PATCH_VERSION     0

#define BLACKBOARD_VERSION          2
#define RPC_COMMUNICATION_VERSION   1
#define REMOTE_MASTER_LIBRARY_API_VERSION   1

#ifdef UNICODE
#error UNICODE is defined
#endif

#ifndef M_PI
#define M_PI        3.14159265358979323846
#endif

#define TIMERCLKFRQ   1000000000.0
#define TIMERCLKPERIODE  (1 / TIMERCLKFRQ)

#define SIGNAL_GENERATOR_COMPILER    "SignalGeneratorCompiler"
#define SIGNAL_GENERATOR_CALCULATOR  "SignalGeneratorCalculator"
#define EQUATION_COMPILER            "EquationCompiler"
#define EQUATION_CALCULATOR          "EquationCalculator"
#define STIMULI_PLAYER               "StimuliPlayer"
#define STIMULI_QUEUE                "StimuliQueue"
#define TRACE_RECORDER               "TraceRecorder"
#define TRACE_QUEUE                  "TraceQueue"
#define SCRIPT_PN                    "Script"
#define OSCILLOSCOPE                 "Oscilloscope"

#define PN_SIGNAL_GENERATOR_COMPILER      pn_signal_generator_compiler
#define PN_SIGNAL_GENERATOR_CALCULATOR    pn_signal_generator_calculator
#define PN_EQUATION_COMPILER              pn_equation_compiler
#define PN_EQUATION_CALCULATOR            pn_equation_calculator
#define PN_STIMULI_PLAYER                 pn_stimuli_player
#define PN_STIMULI_QUEUE                  pn_stimuli_queue
#define PN_TRACE_RECORDER                 pn_trace_recorder
#define PN_TRACE_QUEUE                    pn_trace_queue
#define PN_SCRIPT                         pn_script
#define PN_OSCILLOSCOPE                   pn_oscilloscope


extern char pn_signal_generator_compiler[];
extern char pn_signal_generator_calculator[];
extern char pn_equation_compiler[];
extern char pn_equation_calculator[];
extern char pn_stimuli_player[];
extern char pn_stimuli_queue[];
extern char pn_trace_recorder[];
extern char pn_trace_queue[];
extern char pn_script[];
extern char pn_oscilloscope[];


#define ENTER_CS(ps) EnterCriticalSection (ps)
#define LEAVE_CS(ps) LeaveCriticalSection (ps)
#define INIT_CS(ps) InitializeCriticalSection (ps)
#define DELETE_CS(ps) DeleteCriticalSection (ps)
#define ENTER_GCS() EnterGlobalCriticalSection ()
#define LEAVE_GCS() LeaveGlobalCriticalSection ()
extern void EnterGlobalCriticalSection (void);
extern void LeaveGlobalCriticalSection (void);

//#define USE_OLD_SOFTCAR_NAME
#ifdef USE_OLD_SOFTCAR_NAME
#define DEFAULT_PREFIX_TYPE_PROGRAM_NAME     "Softcar"
#define DEFAULT_PREFIX_TYPE_SHORT_BLACKBOARD ""
#define DEFAULT_PREFIX_TYPE_LONG_BLACKBOARD  "Softcar"
#define DEFAULT_PREFIX_TYPE_LONG2_BLACKBOARD "Softcar"
#define DEFAULT_PREFIX_TYPE_ERROR_FILE       "softcar.err"
#define DEFAULT_PREFIX_TYPE_MESSAGE_FILE     "softcar.msg"
#define DEFAULT_PREFIX_TYPE_CAN_NAMES        "SoftcarRT"
#define DEFAULT_PREFIX_TYPE_FLEXRAY_NAMES    "SoftcarRT"

#define DEFAULT_PREFIX_TYPE_CYCLE_COUNTER        "SoftcarCycleCounter"
#define DEFAULT_PREFIX_TYPE_SAMPLE_TIME          "abt_per"
#define DEFAULT_PREFIX_TYPE_SAMPLE_FREQUENCY     "abt_frq"
#define DEFAULT_PREFIX_TYPE_OWN_EXIT_CODE        "SoftcarExitCode"

#define DEFAULT_PREFIX_TYPE_SCRIPT               "Script"
#define DEFAULT_PREFIX_TYPE_GENERATOR            "Generator"
#define DEFAULT_PREFIX_TYPE_TRACE_RECORDER       "HD_Rec"
#define DEFAULT_PREFIX_TYPE_STIMULUS_PLAYER      "HD_Play"
#define DEFAULT_PREFIX_TYPE_EQUATION_CALCULATOR  "Trigger"

#else
#define DEFAULT_PREFIX_TYPE_PROGRAM_NAME     "XilEnv"
#define DEFAULT_PREFIX_TYPE_SHORT_BLACKBOARD "XilEnv."
#define DEFAULT_PREFIX_TYPE_LONG_BLACKBOARD  "XilEnv"
#define DEFAULT_PREFIX_TYPE_LONG2_BLACKBOARD "XilEnv."
#define DEFAULT_PREFIX_TYPE_ERROR_FILE       "XilEnv.err"
#define DEFAULT_PREFIX_TYPE_MESSAGE_FILE     "XilEnv.msg"
#define DEFAULT_PREFIX_TYPE_CAN_NAMES        "XilEnv"
#define DEFAULT_PREFIX_TYPE_FLEXRAY_NAMES    "XilEnv"

#define DEFAULT_PREFIX_TYPE_CYCLE_COUNTER        "XilEnv.CycleCounter"
#define DEFAULT_PREFIX_TYPE_SAMPLE_TIME          "XilEnv.SampleTime"
#define DEFAULT_PREFIX_TYPE_SAMPLE_FREQUENCY     "XilEnv.SampleFrequency"
#define DEFAULT_PREFIX_TYPE_OWN_EXIT_CODE        "XilEnv.OwnExitCode"

#define DEFAULT_PREFIX_TYPE_SCRIPT               "XilEnv.Script"
#define DEFAULT_PREFIX_TYPE_GENERATOR            "XilEnv.Generator"
#define DEFAULT_PREFIX_TYPE_TRACE_RECORDER       "XilEnv.TraceRecorder"
#define DEFAULT_PREFIX_TYPE_STIMULUS_PLAYER      "XilEnv.StimulusPlayer"
#define DEFAULT_PREFIX_TYPE_EQUATION_CALCULATOR  "XilEnv.EquationCalculator"

#endif

#endif
