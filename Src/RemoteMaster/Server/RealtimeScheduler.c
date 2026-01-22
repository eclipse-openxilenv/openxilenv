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
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <linux/unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "Config.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "ConfigurablePrefix.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Message.h"
#include "RemoteMasterReqToClient.h"
#include "RemoteMasterLock.h"
#include "RealtimeProcessEquations.h"

#include "SearchAndInitHardware.h"

#include "RealtimeScheduler.h"

#include "tcb.h"


#define UNUSED(x) (void)(x)

typedef struct SCB {
    int State;
    pthread_t ThreadId;
    char name[512];                // Scheduler name
    double CyclePeriod;
    uint64_t CyclePeriodInNanoSecond;
    int SyncWithFlexray;

    TASK_CONTROL_BLOCK *ToAddSchedulingProcesses[MAX_REALTIME_PROCESSES];
    int WritePos;
    int ReadPos;
    TASK_CONTROL_BLOCK *StartSchedulingProcesses;
    TASK_CONTROL_BLOCK *CurrentRunningProcess;

    uint64_t CycleCounter;
    uint64_t Cycle;
    // The simulated time that has eluded is since XilEnv is started
    uint64_t SimulatedTimeSinceStartedInNanoSecond;

    int CycleCounterVid;
    int CyclePeriodVid;
    int CycleMaxPeriodVid;
    int CycleMinPeriodVid;
    int MainCycleCounterVid;
	int TimeResetVid;
	int CycleCountMaxPeriodVid;
	int CycleCountMinPeriodVid;

    __uint64_t CycleMaxTime;
    __uint64_t CycleMinTime;

    struct SCB *next;                // Pointer to next scheduler control block
    struct SCB *prev;                // Pointer to previous scheduler control block

    uint64_t TimerTicks;

} SCHEDULER_CONTROL_BLOCK;


static SCHEDULER_CONTROL_BLOCK Schedulers[MAX_SCHEDULERS];

static void set_latency_target(void)
{
    struct stat s;
    int err;
    int latency_target_fd;
    static int32_t latency_target_value = 0;

    errno = 0;
    err = stat("/dev/cpu_dma_latency", &s);
    if (err == -1) {
        ThrowError(1, "WARN: stat /dev/cpu_dma_latency failed");
        return;
    }

    errno = 0;
    latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);
    if (latency_target_fd == -1) {
        ThrowError(1, "WARN: open /dev/cpu_dma_latency");
        return;
    }

    errno = 0;
    err = write(latency_target_fd, &latency_target_value, 4);
    if (err < 1) {
        ThrowError(1, "error setting cpu_dma_latency to %d!", latency_target_value);
        close(latency_target_fd);
        return;
    }
    printf("/dev/cpu_dma_latency set to %dus\n", latency_target_value);
}

REMOTE_MASTER_LOCK SchedMessageGlobalSpinlock;
static REMOTE_MASTER_LOCK PidNameArrayCriticalSection;
#define InitializeCriticalSection(x) RemoteMasterInitLock(x)
#define EnterCriticalSection(x)  RemoteMasterLock(x, __LINE__, __FILE__)
#define LeaveCriticalSection(x)  RemoteMasterUnlock(x)


void PrintProcessList(SCHEDULER_CONTROL_BLOCK *Scheduler)
{
    TASK_CONTROL_BLOCK *Process;

    for (Process = Scheduler->StartSchedulingProcesses; Process != NULL;  Process = Process->next) {
        printf("  %s\n", Process->name);
    }
}

static void RemoveProcessFromList(SCHEDULER_CONTROL_BLOCK *Scheduler, TASK_CONTROL_BLOCK *Process)
{
    // If this is the current active process
    if (Scheduler->CurrentRunningProcess == Process) {
        Scheduler->CurrentRunningProcess = Process->next;
    }

    // Remove process from the task list
    if (Process->next != NULL) {
        TASK_CONTROL_BLOCK *NextProcess = Process->next;
        NextProcess->prev = Process->prev;
    }
    if (Process->prev != NULL) {
        TASK_CONTROL_BLOCK *PrevProcess = Process->prev;
        PrevProcess->next = Process->next;
    }

    // If first process change the base
    if (Scheduler->StartSchedulingProcesses == Process) {
        Scheduler->StartSchedulingProcesses = Process->next;
    }
}

static void InsertProcessToList(SCHEDULER_CONTROL_BLOCK *Scheduler, TASK_CONTROL_BLOCK *Process)
{
    // There are no process inside the list
    if (Scheduler->StartSchedulingProcesses == NULL) {
        Scheduler->StartSchedulingProcesses = Process;
        Process->next = NULL;
        Process->prev = NULL;
    } else {
        TASK_CONTROL_BLOCK *pTcb, *LastTcb = NULL;
        // Walk through the process list (sorted by priority)
        for (pTcb = Scheduler->StartSchedulingProcesses; pTcb != NULL;
            pTcb = pTcb->next) {
            if (Process->prio < pTcb->prio) {
                break;
            }
            LastTcb = pTcb;
        }

        // Insert process
        if (pTcb == NULL) {   // At the end
            Process->prev = LastTcb;
            Process->next = NULL;
            Process->prev->next = Process;
        }
        else if (pTcb->prev == NULL) {  // At the beginning
            Process->prev = NULL;
            Process->next = pTcb;
            Scheduler->StartSchedulingProcesses = pTcb->prev = Process;
        }
        else {                       // Somewhere in the middle
            Process->next = pTcb;
            Process->prev = pTcb->prev;
            Process->prev->next = Process->next->prev = Process;
        }
    }
}

static int AddProcessToScheduler(char *par_Scheduler, TASK_CONTROL_BLOCK *Process)
{
    int s;

    for (s = 0; s < MAX_SCHEDULERS; s++) {
        char *p = Schedulers[s].name;
        if (!strcmp(par_Scheduler, p)) {
            if (Schedulers[s].ToAddSchedulingProcesses[Schedulers[s].WritePos] != 0) {
                ThrowError(1, "too many added processes on a time to scheduler \"%s\" the process \"%s\" will be ignored", Schedulers[s].name, Process->name);
                return -1;
            }
            Schedulers[s].ToAddSchedulingProcesses[Schedulers[s].WritePos] = Process;
            Schedulers[s].WritePos++;
            if (Schedulers[s].WritePos >= MAX_REALTIME_PROCESSES) {
                Schedulers[s].WritePos = 0;
            }
            AddProcessToPidNameArray(Process->pid, Process->name, Process->message_size);
            get_bb_accessmask(Process->pid, &(Process->bb_mask), "");
            Process->active = 1;
            Process->assigned_scheduler = s;
            return 0;
        }
    }
    return -1;
}


static void CheckIfProceeesShouldBeAdded(SCHEDULER_CONTROL_BLOCK *Scheduler)
{
    if (Scheduler->ToAddSchedulingProcesses[Scheduler->ReadPos] != 0) {
        TASK_CONTROL_BLOCK *Tcb;
        Tcb = Scheduler->ToAddSchedulingProcesses[Scheduler->ReadPos];
        Scheduler->ToAddSchedulingProcesses[Scheduler->ReadPos] = 0;
        Scheduler->ReadPos++;
        if (Scheduler->ReadPos >= MAX_REALTIME_PROCESSES) {
            Scheduler->ReadPos = 0;
        }
        InsertProcessToList(Scheduler, Tcb);
        RealtimeProcessStarted(Tcb->pid, Tcb->name);
    }
}


static void schedulers_internal_cyclic(void)
{
}

static int schedulers_internal_init(void)
{
    return 0;
}

static void schedulers_internal_terminate(void)
{
}

TASK_CONTROL_BLOCK schedulers_internal_tcb
= INIT_TASK_COTROL_BLOCK("schedulers_internal", 1, 10000, schedulers_internal_cyclic, schedulers_internal_init, schedulers_internal_terminate, 0);

static pthread_cond_t SignalCyclicEvent_ConditionalVariable;
static pthread_mutex_t SignalCyclicEvent_Mutex;
static int SignalCyclicEvent_Flag;
static uint64_t SignalCyclicEvent_CycleCounter;
static uint64_t SignalCyclicEvent_SimulatedTimeSinceStartedInNanoSecond;

#define USEC_PER_SEC		1000000
static inline __uint64_t calcdiff(struct timespec t1, struct timespec t2)
{
    __uint64_t diff;
    diff = USEC_PER_SEC * (unsigned long long)((int)t1.tv_sec - (int)t2.tv_sec);
    diff += ((int)t1.tv_nsec - (int)t2.tv_nsec) / 1000;
    return diff;
}

#define CLOCK_MONOTONIC 1

#define gettid() syscall(__NR_gettid)
#define sigev_notify_thread_id _sigev_un._tid


static void *RealtimeSchedulerTreadFunction(void* par_Data)
{
    cpu_set_t mask;
    pthread_t thread;

    sigevent_t sigev;
    sigset_t sigset;
    int signum = SIGALRM;
    timer_t timer;
    struct itimerspec tspec;
    int sigs;
    struct sched_param schedp;

    struct timespec now, last;
    __uint64_t diff;

    SCHEDULER_CONTROL_BLOCK *Scheduler = (SCHEDULER_CONTROL_BLOCK*)par_Data;
    MEMSET(Scheduler->ToAddSchedulingProcesses, 0, sizeof(Scheduler->ToAddSchedulingProcesses));
    Scheduler->WritePos = 0;
    Scheduler->ReadPos = 0;
    Scheduler->StartSchedulingProcesses = 0;
    Scheduler->CurrentRunningProcess = 0;
    Scheduler->CycleMaxTime = 0;
    Scheduler->CycleMinTime = 0x7FFFFFFFFFFFFFFFLL;
    Scheduler->CycleCounter = 0;
    {   // This variable must be added inside the scheduler task
        char VariableName[BBVARI_NAME_SIZE];
        const char *Prefix = GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG2_BLACKBOARD);
        Scheduler->CurrentRunningProcess = &schedulers_internal_tcb;

        PrintFormatToString (VariableName, sizeof(VariableName), "%sCycleCounter", Prefix);
        Scheduler->MainCycleCounterVid = add_bbvari(VariableName, BB_UDWORD, "");
        Prefix = GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_LONG_BLACKBOARD);
        PrintFormatToString (VariableName, sizeof(VariableName), "%s.%s.CyleCounter", Prefix, Scheduler->name);
        Scheduler->CycleCounterVid = add_bbvari(VariableName, BB_UDWORD, "");
        PrintFormatToString (VariableName, sizeof(VariableName), "%s.%s.CyclePeriod", Prefix, Scheduler->name);
        Scheduler->CyclePeriodVid = add_bbvari(VariableName, BB_DOUBLE, "");
        PrintFormatToString (VariableName, sizeof(VariableName), "%s.%s.CycleMinPeriod", Prefix, Scheduler->name);
        Scheduler->CycleMinPeriodVid = add_bbvari(VariableName, BB_DOUBLE, "");
        PrintFormatToString (VariableName, sizeof(VariableName), "%s.%s.CycleMaxPeriod", Prefix, Scheduler->name);
        Scheduler->CycleMaxPeriodVid = add_bbvari(VariableName, BB_DOUBLE, "");


        PrintFormatToString (VariableName, sizeof(VariableName), "%s.%s.TimeReset", Prefix, Scheduler->name);
		Scheduler->TimeResetVid = add_bbvari(VariableName, BB_UDWORD, "");
        PrintFormatToString (VariableName, sizeof(VariableName), "%s.%s.CycleCountMinPeriod", Prefix, Scheduler->name);
		Scheduler->CycleCountMinPeriodVid = add_bbvari(VariableName, BB_UDWORD, "");
        PrintFormatToString (VariableName, sizeof(VariableName), "%s.%s.CycleCountMaxPeriod", Prefix, Scheduler->name);
		Scheduler->CycleCountMaxPeriodVid = add_bbvari(VariableName, BB_UDWORD, "");
        // only debug
        //PrintFormatToString (VariableName, sizeof(VariableName), "%s.%s.FreeRunningCycleCounter_Debug", Scheduler->name);
        //SignalCyclicEvent_FreeRunningCycleCounter_Debug_Vid = add_bbvari(VariableName, BB_UDWORD, "");
	}

    Scheduler->ThreadId = pthread_self();

    CPU_ZERO(&mask);
    CPU_SET(2, &mask);  // CPU2
    thread = pthread_self();
    if (pthread_setaffinity_np(thread, sizeof(mask), &mask) != 0) {
        printf("Could not set CPU affinity to CPU %d\n", 1);
    }

    MEMSET(&schedp, 0, sizeof(schedp));
    schedp.sched_priority = 99;
    if (sched_setscheduler(0, SCHED_FIFO, &schedp)) {
        printf("failed to set priority to %d\n", 99);
    }
    if (1) {
        sigemptyset(&sigset);
        sigaddset(&sigset, signum);
        sigprocmask(SIG_BLOCK, &sigset, NULL);

        sigev.sigev_notify = SIGEV_THREAD_ID | SIGEV_SIGNAL;
        sigev.sigev_signo = signum;
        sigev.sigev_notify_thread_id = gettid();
        timer_create(CLOCK_MONOTONIC, &sigev, &timer);
        tspec.it_interval.tv_sec = 0;
        tspec.it_interval.tv_nsec = Scheduler->CyclePeriod * 1000000000.0;

        tspec.it_value = tspec.it_interval;
        timer_settime(timer, TIMER_ABSTIME, &tspec, NULL);
    }

	printf("RealtimeSchedulerTreadFunction() running on cpu %i\n", sched_getcpu());

    while (Scheduler->State) {
		if (sched_getcpu() != 2) {
			printf("Scheduler should be running on CPU2\n");
		}

        sigwait(&sigset, &sigs);

        last = now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        Scheduler->CurrentRunningProcess = &schedulers_internal_tcb;
        if (Scheduler->CycleCounter) {
            diff = calcdiff(now, last);
            if (diff > Scheduler->CycleMaxTime) {
                Scheduler->CycleMaxTime = diff;
                write_bbvari_double_without_check(Scheduler->CycleMaxPeriodVid, (double)Scheduler->CycleMaxTime * 0.000001);
				write_bbvari_udword_without_check(Scheduler->CycleCountMaxPeriodVid, Scheduler->CycleCounter);
            } else if (diff < Scheduler->CycleMinTime) {
                Scheduler->CycleMinTime = diff;
                write_bbvari_double_without_check(Scheduler->CycleMinPeriodVid, (double)Scheduler->CycleMinTime * 0.000001);
				write_bbvari_udword_without_check(Scheduler->CycleCountMinPeriodVid, Scheduler->CycleCounter);
            }
        }
        // The scheduler have own function for writing to blackboard variables (no checking about access)
        write_bbvari_udword_without_check(Scheduler->CycleCounterVid, read_bbvari_udword(Scheduler->CycleCounterVid) + 1);
        write_bbvari_double_without_check(Scheduler->CyclePeriodVid, (double)diff * 0.000001);
        write_bbvari_udword_without_check(Scheduler->MainCycleCounterVid, read_bbvari_udword(Scheduler->MainCycleCounterVid) + 1);

		if (read_bbvari_udword(Scheduler->TimeResetVid)) {
			write_bbvari_udword_without_check(Scheduler->TimeResetVid, 0);
			Scheduler->CycleMaxTime = 0;
			Scheduler->CycleMinTime = 0x7FFFFFFFFFFFFFFFLL;
		}

        Scheduler->CycleCounter++;
        Scheduler->SimulatedTimeSinceStartedInNanoSecond += Scheduler->CyclePeriodInNanoSecond;

		pthread_mutex_lock(&SignalCyclicEvent_Mutex);
		SignalCyclicEvent_CycleCounter = Scheduler->CycleCounter; 
		SignalCyclicEvent_SimulatedTimeSinceStartedInNanoSecond = Scheduler->SimulatedTimeSinceStartedInNanoSecond;
		SignalCyclicEvent_Flag = 1;
		pthread_mutex_unlock(&SignalCyclicEvent_Mutex);
		pthread_cond_signal(&SignalCyclicEvent_ConditionalVariable);

        EnterCriticalSection(&SchedMessageGlobalSpinlock);
        Scheduler->CurrentRunningProcess = Scheduler->StartSchedulingProcesses;
        while (Scheduler->CurrentRunningProcess != 0) {
            switch (Scheduler->CurrentRunningProcess->state) {
            case 0:  // Call Init
                if (Scheduler->CurrentRunningProcess->init()) {
                    Scheduler->CurrentRunningProcess->state = 3;
                }
                else {
                    char VariableName[BBVARI_NAME_SIZE];
                    const char *Prefix = GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME);
                    PrintFormatToString (VariableName, sizeof(VariableName), "%s.Runtime.%s", Prefix, Scheduler->CurrentRunningProcess->name);
                    Scheduler->CurrentRunningProcess->state = 1;
                    Scheduler->CurrentRunningProcess->VidRuntime = add_bbvari(VariableName, BB_UDWORD, "");
                }
                break;
            case 1:  // Call Cyclic
                if (!Scheduler->CurrentRunningProcess->should_be_stopped) {
                    Scheduler->CurrentRunningProcess->time_counter++;
                    if (Scheduler->CurrentRunningProcess->time_counter >= Scheduler->CurrentRunningProcess->time_steps) {
                        Scheduler->CurrentRunningProcess->time_counter = 0;
                        Scheduler->CurrentRunningProcess->call_count++;

                        struct timespec start, stop;
                        clock_gettime(CLOCK_MONOTONIC, &start);

                        ClacBeforeProcessEquations(Scheduler->CurrentRunningProcess);
                        Scheduler->CurrentRunningProcess->cyclic();
                        ClacBehindProcessEquations(Scheduler->CurrentRunningProcess);

                        clock_gettime(CLOCK_MONOTONIC, &stop);
                        diff = calcdiff(stop, start);
                        Scheduler->CurrentRunningProcess->CycleTime = (__uint64_t)diff;
                        write_bbvari_udword_without_check(Scheduler->CurrentRunningProcess->VidRuntime, (uint32_t)diff);
                        if (diff > Scheduler->CurrentRunningProcess->CycleTimeMax) Scheduler->CurrentRunningProcess->CycleTimeMax = diff;
                        if (diff < Scheduler->CurrentRunningProcess->CycleTimeMin) Scheduler->CurrentRunningProcess->CycleTimeMin = diff;
                    }
                    break;
                }
                // no break;
            case 2:  // Call Terminate
                Scheduler->CurrentRunningProcess->terminat();
                Scheduler->CurrentRunningProcess->state = 3;
                break;
            case 3:  // Killed
                remove_bbvari(Scheduler->CurrentRunningProcess->VidRuntime);
                // Report to the client that the process was terminated
                RealtimeProcessStopped(Scheduler->CurrentRunningProcess->pid, Scheduler->CurrentRunningProcess->name);
                free_bb_accessmask(Scheduler->CurrentRunningProcess->pid, Scheduler->CurrentRunningProcess->bb_mask);
                RemoveProcessFromPidNameArray(Scheduler->CurrentRunningProcess->pid);
                GetOrFreeUniquePid(FREE_UNIQUE_PID_COMMAND, Scheduler->CurrentRunningProcess->pid, Scheduler->CurrentRunningProcess->name);
                Scheduler->CurrentRunningProcess->pid = -1;
                Scheduler->CurrentRunningProcess->state = 0;
                Scheduler->CurrentRunningProcess->active = 0;
                RemoveProcessFromList(Scheduler, Scheduler->CurrentRunningProcess);
                if (Scheduler->CurrentRunningProcess == NULL) continue;   // If last procees will be terminate
                break;
            }
            Scheduler->CurrentRunningProcess = Scheduler->CurrentRunningProcess->next;
        }
        CheckIfProceeesShouldBeAdded(Scheduler);
        LeaveCriticalSection(&SchedMessageGlobalSpinlock);
    }

    // Scheduler will be terminate (Call for all running processes the terminate() function)
    EnterCriticalSection(&SchedMessageGlobalSpinlock);
    Scheduler->CurrentRunningProcess = Scheduler->StartSchedulingProcesses;
    while (Scheduler->CurrentRunningProcess != 0) {
        switch (Scheduler->CurrentRunningProcess->state) {
        case 1:  // Call Cyclic
            Scheduler->CurrentRunningProcess->terminat();
            Scheduler->CurrentRunningProcess->state = 3;
            break;
        }
        Scheduler->CurrentRunningProcess = Scheduler->CurrentRunningProcess->next;
    }
    LeaveCriticalSection(&SchedMessageGlobalSpinlock);
    EnterCriticalSection(&SchedMessageGlobalSpinlock);
    Scheduler->CurrentRunningProcess = Scheduler->StartSchedulingProcesses;
    while (Scheduler->CurrentRunningProcess != 0) {
        switch (Scheduler->CurrentRunningProcess->state) {
        case 3:  // Killed
            remove_bbvari(Scheduler->CurrentRunningProcess->VidRuntime);
            // Report to the client that the process was terminated
            RealtimeProcessStopped(Scheduler->CurrentRunningProcess->pid, Scheduler->CurrentRunningProcess->name);
            free_bb_accessmask(Scheduler->CurrentRunningProcess->pid, Scheduler->CurrentRunningProcess->bb_mask);
            RemoveProcessFromPidNameArray(Scheduler->CurrentRunningProcess->pid);
            GetOrFreeUniquePid(FREE_UNIQUE_PID_COMMAND, Scheduler->CurrentRunningProcess->pid, Scheduler->CurrentRunningProcess->name);
            Scheduler->CurrentRunningProcess->pid = -1;
            Scheduler->CurrentRunningProcess->state = 0;
            Scheduler->CurrentRunningProcess->active = 0;
            RemoveProcessFromList(Scheduler, Scheduler->CurrentRunningProcess);
            if (Scheduler->CurrentRunningProcess == NULL) continue;   // If last procees will be terminate
            break;
        }
        Scheduler->CurrentRunningProcess = Scheduler->CurrentRunningProcess->next;
    }
    LeaveCriticalSection(&SchedMessageGlobalSpinlock);
    return 0;
}

static void *TransmitCyclicEventTreadFunction(void* par_Data)
{
    UNUSED(par_Data);
	while (1) {
		pthread_mutex_lock(&SignalCyclicEvent_Mutex);
		while (!SignalCyclicEvent_Flag) {
			pthread_cond_wait(&SignalCyclicEvent_ConditionalVariable, &SignalCyclicEvent_Mutex);
	    }
	    SignalCyclicEvent_Flag = 0;
	    pthread_mutex_unlock(&SignalCyclicEvent_Mutex);

        // Transmit cyclic event to the client
		rm_SchedulerCyclicEvent(0, SignalCyclicEvent_CycleCounter, SignalCyclicEvent_SimulatedTimeSinceStartedInNanoSecond);
	}
}

int AddRealtimeScheduler(char *par_Name, double par_CyclePeriod, int par_SyncWithFlexray, uint64_t *ret_CyclePeriodInNanoSecond)
{
    int s;
    struct sched_param param;
    pthread_attr_t attr;
    int ret;
    static int call_set_latency_target = 1;

    if (call_set_latency_target) {
        call_set_latency_target = 0;
        set_latency_target();
    }
    for (s = 0; s < MAX_SCHEDULERS; s++) {
        if (Schedulers[s].State == 0) {
            pthread_t ThreadId;
            MEMSET(&(Schedulers[s]), 0, sizeof(Schedulers[0]));
            STRING_COPY_TO_ARRAY(Schedulers[s].name, par_Name);
            Schedulers[s].State = 1;
            Schedulers[s].CyclePeriod = par_CyclePeriod;
            Schedulers[s].SyncWithFlexray = par_SyncWithFlexray;
            Schedulers[s].CyclePeriodInNanoSecond = (uint64_t)(par_CyclePeriod * (1000.0 * 1000.0 * 1000.0));   // Nano seconds
            Schedulers[s].TimerTicks = (uint64_t)(par_CyclePeriod * (1000.0 * 1000.0));   // Micro seconds

            *ret_CyclePeriodInNanoSecond = Schedulers[s].CyclePeriodInNanoSecond;

            /* Lock memory */
            if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
                printf("mlockall failed: %m\n");
                //return -1; this should not be a hard error
            }

            /* Initialize pthread attributes (default values) */
            ret = pthread_attr_init(&attr);
            if (ret) {
                printf("init pthread attributes failed\n");
                return -1;
            }

            /* Set a specific stack size  */
            ret = pthread_attr_setstacksize(&attr, 256*1024);
            if (ret) {
                printf("pthread setstacksize failed\n");
                return -1;
            }

            /* Set scheduler policy and priority of pthread */
            ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
            if (ret) {
                printf("pthread setschedpolicy failed\n");
                return -1;
            }
            param.sched_priority = 99;
            ret = pthread_attr_setschedparam(&attr, &param);
            if (ret) {
                printf("pthread setschedparam failed\n");
                return -1;
            }
            /* Use scheduling parameters of attr */
            ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
            if (ret) {
                printf("pthread setinheritsched failed\n");
                return -1;
            }

            if (pthread_create(&ThreadId, &attr, RealtimeSchedulerTreadFunction, (void*)&(Schedulers[s])) < 0) {
                perror("could not create scheduler thread");
                return -1;
            }
            pthread_attr_destroy(&attr);

			pthread_setname_np(ThreadId, "rt_scheduler");

            Schedulers[s].ThreadId = ThreadId;

			if (pthread_create(&ThreadId, NULL, TransmitCyclicEventTreadFunction, (void*)&(Schedulers[s])) < 0) {
				perror("could not create transmit cyclic event thread");
				return -1;
			}

			return 0;
        }
    }
    return -1;
}

int StopRealtimeScheduler(char *par_Name)
{
    UNUSED(par_Name);
    return 0;
}

int StopAllRealtimeScheduler(void)
{
    int s, x;
    for (s = 0; s < MAX_SCHEDULERS; s++) {
        if (Schedulers[s].State != 0) {
            Schedulers[s].State = 0;
        }
    }
    for (x = 0; x < 3; x++) {
        int AllTerminated = 1;
        for (s = 0; s < MAX_SCHEDULERS; s++) {
            if (Schedulers[s].State != -1) {
                AllTerminated = 0;
            }
        }
        if (AllTerminated) return 0;
        sleep(1);  // maybe to long?
    }
    ThrowError(1, "cannot terminate all realtime schedulers");
    return -1;   // 
}

extern TASK_CONTROL_BLOCK ModelIntegration_tcb;
extern TASK_CONTROL_BLOCK rdpipe_tcb;
extern TASK_CONTROL_BLOCK wrpipe_tcb;
extern TASK_CONTROL_BLOCK new_canserv_tcb;
extern TASK_CONTROL_BLOCK sync_rampen_tcb;
extern TASK_CONTROL_BLOCK sync_trigger_tcb;

TASK_CONTROL_BLOCK *AvailableRealtimeProcesses[] = { &ModelIntegration_tcb,
                                                     &rdpipe_tcb,
                                                     &wrpipe_tcb,
                                                     &new_canserv_tcb,
                                                     &sync_rampen_tcb,
                                                     &sync_trigger_tcb,
};

#define AvailableRealtimeProcessCount  (sizeof(AvailableRealtimeProcesses) / sizeof(AvailableRealtimeProcesses[0]))



int StartRealtimeProcess(int par_Pid, char *par_Name, char *par_Scheduler, int par_Prio, int par_Cycles, int par_Delay)
{
    size_t x;
    for (x = 0; x < AvailableRealtimeProcessCount; x++) {
        if (!AvailableRealtimeProcesses[x]->active &&
            !strcmp(AvailableRealtimeProcesses[x]->name, par_Name)) {  // todo: maxc
            AvailableRealtimeProcesses[x]->Type = REALTIME_PROCESS;
            AvailableRealtimeProcesses[x]->pid = par_Pid;
            AvailableRealtimeProcesses[x]->prio = par_Prio;
            AvailableRealtimeProcesses[x]->time_steps = par_Cycles;
            AvailableRealtimeProcesses[x]->delay = par_Delay;
            AvailableRealtimeProcesses[x]->time_counter = 0;
            AvailableRealtimeProcesses[x]->call_count = 0;
            AvailableRealtimeProcesses[x]->should_be_stopped = 0;
            AvailableRealtimeProcesses[x]->CycleTime = 0;
            AvailableRealtimeProcesses[x]->CycleTimeMax = 0;
            AvailableRealtimeProcesses[x]->CycleTimeMin = 0xFFFFFFFFFFFFFFFFULL;

            AvailableRealtimeProcesses[x]->BeforeProcessEquationCounter = 0;
            AvailableRealtimeProcesses[x]->BeforeProcessEquations = NULL;
            AvailableRealtimeProcesses[x]->BehindProcessEquationCounter = 0;
            AvailableRealtimeProcesses[x]->BehindProcessEquations = NULL;
            return AddProcessToScheduler(par_Scheduler, AvailableRealtimeProcesses[x]);
        }
    }
    return -1;
}

int StopRealtimeProcess(int par_Pid)
{
    size_t x;
    for (x = 0; x < AvailableRealtimeProcessCount; x++) {
        int LocalPid = AvailableRealtimeProcesses[x]->pid;
        if (LocalPid == par_Pid) {
            AvailableRealtimeProcesses[x]->should_be_stopped = 1;
            return 0;
        }
    }
    return -1;
}

int ResetProcessMinMaxRuntimeMeasurement(int par_Pid)
{
    size_t x;
    EnterCriticalSection(&PidNameArrayCriticalSection);
    for (x = 0; x < AvailableRealtimeProcessCount; x++) {
        int LocalPid = AvailableRealtimeProcesses[x]->pid;
        if (LocalPid == par_Pid) {
            AvailableRealtimeProcesses[x]->CycleTimeMax = 0;
            AvailableRealtimeProcesses[x]->CycleTimeMin = 0xFFFFFFFFFFFFFFFFULL;
            LeaveCriticalSection(&PidNameArrayCriticalSection);
            return 0;
        }
    }
    LeaveCriticalSection(&PidNameArrayCriticalSection);
    return -1;
}


int GetNextRealtimeProcess(int par_Index, int par_Flags, char *ret_Buffer, int par_maxc)
{
    UNUSED(par_maxc);
    size_t x;
    for (x = par_Index; x < AvailableRealtimeProcessCount; x++) {
        if (par_Flags) {
            if (AvailableRealtimeProcesses[x]->active) {
                StringCopyMaxCharTruncate(ret_Buffer, AvailableRealtimeProcesses[x]->name, par_maxc);
                return x + 1;
            }
        }
        else {
            if (!AvailableRealtimeProcesses[x]->active) {
                StringCopyMaxCharTruncate(ret_Buffer, AvailableRealtimeProcesses[x]->name, par_maxc);
                return x + 1;
            }
        }
    }
    return -1;
}

int IsRealtimeProcess(char *par_Name)
{
    size_t x;
    for (x = 0; x < AvailableRealtimeProcessCount; x++) {
        if (!strcmp(AvailableRealtimeProcesses[x]->name, par_Name)) {
            return 1;
        }
    }
    return 0;
}

int IsRealtimeProcessPid(int par_Pid)
{
    TASK_CONTROL_BLOCK *tcb;

    // Serach process list for process ID
    for (tcb = Schedulers[0].StartSchedulingProcesses; tcb != NULL; tcb = tcb->next) {
        // If pid found than takeover status
        if (tcb->pid == par_Pid) {
            return 1;
        }
    }
    return 0;
}


int GET_PID()
{
    int s;
    pthread_t ThreadId = pthread_self();

    for (s = 0; s < MAX_SCHEDULERS; s++) {
        if (pthread_equal(Schedulers[s].ThreadId, ThreadId)) {
            if (Schedulers[s].CurrentRunningProcess != 0) {
                return Schedulers[s].CurrentRunningProcess->pid;
            }
        }
    }
    return 1;
}


static struct {
    int Pid;
    MESSAGE_BUFFER *message_buffer;  // Pointer to message queue data structure
    char Name[260];
} PidNameArray[64];

int AddProcessToPidNameArray(int par_Pid, char *par_Name, int MessageQueuSize)
{
    int x;
    int Ret = -1;

    EnterCriticalSection(&PidNameArrayCriticalSection);
    for (x = 0; x < 64; x++) {
        if (PidNameArray[x].Pid == 0) {
            PidNameArray[x].Pid = par_Pid;
            STRING_COPY_TO_ARRAY (PidNameArray[x].Name, par_Name);
			if ((par_Pid  & RT_PID_BIT_MASK) == RT_PID_BIT_MASK) {
				PidNameArray[x].message_buffer = build_message_queue(MessageQueuSize);
			} else {
				PidNameArray[x].message_buffer = NULL;
			}
            Ret = 0;
            break;
        }
    }
    LeaveCriticalSection(&PidNameArrayCriticalSection);
    return Ret;
}

int RemoveProcessFromPidNameArray(int par_Pid)
{
    int x;
    int Ret = -1;

    EnterCriticalSection(&PidNameArrayCriticalSection);
    for (x = 0; x < 64; x++) {
        if (PidNameArray[x].Pid == par_Pid) {
            PidNameArray[x].Pid = 0;
            STRING_COPY_TO_ARRAY(PidNameArray[x].Name, "");
            remove_message_queue(PidNameArray[x].message_buffer);
            PidNameArray[x].message_buffer = NULL;
            Ret = 0;
            break;
        }
    }
    LeaveCriticalSection(&PidNameArrayCriticalSection);
    return Ret;
}

int get_pid_by_name(char *Name)
{
    return GetOrFreeUniquePid(GET_UNIQUE_PID_BY_NAME_COMMAND, 0, Name);
}

MESSAGE_BUFFER *GetMessageQueueForProcess(int par_Pid)
{
    int x;
    MESSAGE_BUFFER *Ret = 0;

    EnterCriticalSection(&PidNameArrayCriticalSection);
    for (x = 0; x < 64; x++) {
        if (PidNameArray[x].Pid == par_Pid) {
            Ret = PidNameArray[x].message_buffer;
            break;
        }
    }
    LeaveCriticalSection(&PidNameArrayCriticalSection);
    return Ret;
}


uint64_t get_timestamp_counter(void)
{
    // Always scheduler 0
    return  (uint64_t)(Schedulers[0].SimulatedTimeSinceStartedInNanoSecond);
}

uint64_t get_rt_cycle_counter(void)
{
    return Schedulers[0].CycleCounter;
}

uint64_t GetCycleCounter64(void)
{
    return Schedulers[0].CycleCounter;
}

uint64_t get_sched_periode_timer_clocks(void)
{
    return Schedulers[0].TimerTicks;
}


uint64_t get_sched_periode_timer_clocks64(void)
{
    return Schedulers[0].TimerTicks;
}

void InitSchedulers(int par_PidForSchedulersHelperProcess)
{
    InitializeCriticalSection(&PidNameArrayCriticalSection);
	pthread_cond_init(&SignalCyclicEvent_ConditionalVariable, NULL);
	pthread_mutex_init(&SignalCyclicEvent_Mutex, NULL);
	schedulers_internal_tcb.pid = par_PidForSchedulersHelperProcess;
}

static void strcpy_maxc(char *dst, char *src, int max_c)
{
    if (max_c > 0) {
        int len = strlen(src) + 1;
        if (len <= max_c) {
            MEMCPY(dst, src, len);
        }
        else {
            MEMCPY(dst, src, max_c - 1);
            dst[max_c - 1] = 0;
        }
    }
}

int get_process_info_ex(int pid, char *ret_Name, int maxc_Name, int *ret_Type, int *ret_Prio, int *ret_Cycles, int *ret_Delay, int *ret_MessageSize,
                        char *ret_AssignedScheduler, int maxc_AssignedScheduler, uint64_t *ret_bb_access_mask,
                        int *ret_State, uint64_t *ret_CyclesCounter, uint64_t *ret_CyclesTime, uint64_t *ret_CyclesTimeMax, uint64_t *ret_CyclesTimeMin)
{
    size_t x;

    for (x = 0; x < AvailableRealtimeProcessCount; x++) {
        if (AvailableRealtimeProcesses[x]->pid == pid) {
            if ((ret_Name != NULL) && (maxc_Name > 0)) {
                strcpy_maxc(ret_Name, AvailableRealtimeProcesses[x]->name, maxc_Name);
            }
            if (ret_Type != NULL) *ret_Type = AvailableRealtimeProcesses[x]->Type;
            if (ret_Prio != NULL) *ret_Prio = AvailableRealtimeProcesses[x]->prio;
            if (ret_Cycles != NULL) *ret_Cycles = AvailableRealtimeProcesses[x]->time_steps;
            if (ret_Delay != NULL) *ret_Delay = AvailableRealtimeProcesses[x]->delay;
            if (ret_MessageSize != NULL) *ret_MessageSize = AvailableRealtimeProcesses[x]->message_size;
            if ((ret_AssignedScheduler != NULL) && (maxc_AssignedScheduler > 0)) {
                strcpy_maxc(ret_AssignedScheduler, "RealtimeScheduler", maxc_AssignedScheduler);
            }
            if (ret_bb_access_mask != NULL) *ret_bb_access_mask = AvailableRealtimeProcesses[x]->bb_mask;
            if (ret_State != NULL) *ret_State = AvailableRealtimeProcesses[x]->state;
            if (ret_CyclesCounter != NULL) *ret_CyclesCounter = AvailableRealtimeProcesses[x]->call_count;
            if (ret_CyclesTime != NULL) *ret_CyclesTime = AvailableRealtimeProcesses[x]->CycleTime;
            if (ret_CyclesTimeMax != NULL) *ret_CyclesTimeMax = AvailableRealtimeProcesses[x]->CycleTimeMax;
            if (ret_CyclesTimeMin != NULL) *ret_CyclesTimeMin = AvailableRealtimeProcesses[x]->CycleTimeMin;
            return 0;
        }
    }
    return -1;
}


/* If us != 0 the timeout will be set
   afterwards this function have to be called with us = 0
   till the return value != 0. Than the timeout occured
   Remark this function is not re-entrant
*/
int MyTimeout(int us)
{
    static int timeout;
    int64_t diff;
    static struct timespec start;
    struct timespec now;

    if (!us) {
        clock_gettime(CLOCK_MONOTONIC, &now);
        diff = calcdiff(now, start);
        return (diff > timeout);
    } else {
        clock_gettime(CLOCK_MONOTONIC, &start);
        timeout = us;
        return 0;
    }
}


int Compare2ProcessNames(char *par_Name1, char *par_Name2)
{
    char *p1, *p2;
    char *po1, *po2;   // Last '.' char
    char *bs1, *bs2;   // Last '\\' char
    int ret;

    p1 = par_Name1;
    p2 = par_Name2;
    bs1 = po1 = NULL;
    bs2 = po2 = NULL;
    while (*p1 != 0) {
        if (*p1 == '.') po1 = p1 + 1;
        else if (*p1 == '\\') bs1 = p1 + 1;
        p1++;
    }
    while (*p2 != 0) {
        if (*p2 == '.') po2 = p2 + 1;
        else if (*p2 == '\\') bs2 = p2 + 1;
        p2++;
    }

    if ((po1 != NULL) && (po2 != NULL)) {  // Compare two EXE- Processe (This must include a '.' inside its name)
        int Len1, Len2;
        char *pp1, *pp2;
        if (bs1 != NULL) {  // If folder exist ignore it
            pp1 = bs1;
        }
        else {
            pp1 = par_Name1;
        }
        if (bs2 != NULL) {  // If folder exist ignore it
            pp2 = bs2;
        }
        else {
            pp2 = par_Name2;
        }
        Len1 = p1 - pp1;
        Len2 = p2 - pp2;
        if ((Len1 > 0) && (Len1 == Len2)) {
            return strncasecmp(pp1, pp2, Len1);  // Name not equal?
        }
        return 1;
    }
    else if ((po1 != NULL) || (po2 != NULL)) { // A EXE ans a internal process
        return 1;

    }
    else {   // otherwise it are two internal
        int Len1, Len2;
        Len1 = p1 - par_Name1;
        Len2 = p2 - par_Name2;
        if ((Len1 > 0) && (Len1 == Len2)) {
            return strncasecmp(par_Name1, par_Name2, Len1);  // Name not equal?
        }
        return 1;
    }
    return ret;
}


//#define GET_FREE_UNIQUE_PID_DEBUGGING

int GetOrFreeUniquePid(int par_Command, int par_Pid, char *par_Name)
{
    /* ProcessIds consists a 16Bit variable the last 6 significant bit correspond with the bit position inside the WR mask of the blackboard */
#ifdef GET_FREE_UNIQUE_PID_DEBUGGING
    static FILE *fh = NULL;
#endif
    static int Pid[MAX_PIDS];
    static char *Name[MAX_PIDS];
    static char ValidFlag[MAX_PIDS];
    static int PidCounter;
    int Ret;
    int x;

    Ret = UNKNOWN_PROCESS;
    EnterCriticalSection(&PidNameArrayCriticalSection);

#ifdef GET_FREE_UNIQUE_PID_DEBUGGING
    if (fh == NULL) {
        fh = fopen("/tmp/pid_logging.txt", "wt");
    }
    if (fh != NULL) {
        fprintf(fh, "%u:\n", GetCycleCounter());
        fprintf(fh, "GetOrFreeUniquePid (par_Command=%i, par_Pid=%i, par_Name=%s)\n", par_Command, par_Pid, par_Name);
        for (x = 0; x < MAX_PIDS; x++) {
            fprintf(fh, " %i", Pid[x]);
        }
        fprintf(fh, "\n");
    }
#endif
    switch (par_Command) {
	case GENERATE_UNIQUE_RT_PID_COMMAND:
    case GENERATE_UNIQUE_PID_COMMAND:
        Ret = NO_FREE_PID;
        for (x = 0; x < MAX_PIDS; x++) {
            if (Pid[x] == 0) {
                PidCounter++;
                if (PidCounter >(0x7FFF / MAX_PIDS)) {
                    PidCounter = 1;
                }
				if (par_Command == GENERATE_UNIQUE_RT_PID_COMMAND) {
					Ret = Pid[x] = (PidCounter * MAX_PIDS + x) | RT_PID_BIT_MASK;
				} else {
					Ret = Pid[x] = PidCounter * MAX_PIDS + x;
				}
                if (par_Name != NULL) {
                    int len = strlen(par_Name) + 1;
                    Name[x] = (char*)my_malloc(len);
                    if (Name[x] != NULL) MEMCPY(Name[x], par_Name, len);
                }
                ValidFlag[x] = 1;
                break;
            }
        }
        break;
    case INVALIDATE_UNIQUE_PID_COMMAND:
        Ret = UNKNOWN_PROCESS;
        for (x = 0; x < MAX_PIDS; x++) {
            if (Pid[x] == par_Pid) {
                ValidFlag[x] = 0;
                Ret = Pid[x];
                break;
            }
        }
        break;
    case FREE_UNIQUE_PID_COMMAND:
        Ret = UNKNOWN_PROCESS;
        for (x = 0; x < MAX_PIDS; x++) {
            if (par_Pid == Pid[x]) {
                ValidFlag[x] = 0;
                Ret = Pid[x];
                Pid[x] = 0;
                if (Name[x] != NULL) my_free(Name[x]);
                Name[x] = NULL;
                break;
            }
        }
        break;
    case GET_UNIQUE_PID_BY_NAME_COMMAND:
        Ret = UNKNOWN_PROCESS;
        if (par_Name != NULL) {
            for (x = 0; x < MAX_PIDS; x++) {
                if (ValidFlag[x] && (Pid[x] > 0) && (Name[x] != NULL)) {
                    if (!Compare2ProcessNames(Name[x], par_Name)) {
                        Ret = Pid[x];
                        break;
                    }
                }
            }
        }
        break;
    default:
        break;
    }
#ifdef GET_FREE_UNIQUE_PID_DEBUGGING
    if (fh != NULL) {
        fprintf(fh, "Ret = %i\n", Ret);
        fflush(fh);
    }
#endif
    LeaveCriticalSection(&PidNameArrayCriticalSection);
    return Ret;
}


TASK_CONTROL_BLOCK * GetPointerToTaskControlBlock(int pid)
{
    TASK_CONTROL_BLOCK *tcb;

    // Walk through the process list
    for (tcb = Schedulers[0].StartSchedulingProcesses; tcb != NULL; tcb = tcb->next) {
        // If pid found
        if (tcb->pid == pid) {
            return tcb;
        }
    }
    return NULL;
}
