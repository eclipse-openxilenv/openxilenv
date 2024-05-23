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


#ifndef PIPESCHED_H
#define PIPESCHED_H

#include "tcb.h"
#include "BaseMessages.h"
#include "SchedEnableDisable.h"

#define MAX_SCHEDULERS  16
#define MAX_EXTERN_PROCESSES 32
#define TIME_TO_WAIT_FOR_TERMINATE_ALL_SCHEUDLERS   30

#define MAX_SCHEDULER_NAME_LENGTH   64

#define SCHED_INI_SECTION     "Scheduler"
#define PERIODE_INI_ENTRY     "Period"

typedef struct SCHEDULER_DATA_STRUCT {
    int Number;
    int IsMainScheduler;
    int State;
#define SCHED_OFF_STATE            0
#define SCHED_RUNNING_STATE        1
#define SCHED_STOPPING_STATE       2 
#define SCHED_IS_STOPPED_STATE     3 
#define SCHED_TERMINATING_STATE    4
#define SCHED_IS_TERMINATED_STATE  5

#define SCHED_EXTERNAL_ONLY_INFO   6

    int Prio;

    TASK_CONTROL_BLOCK *FirstTcb;
    TASK_CONTROL_BLOCK *LastTcb;
    TASK_CONTROL_BLOCK *CurrentTcb;
    double SchedPeriod;
    int64_t SchedPeriodInNanoSecond;
    uint64_t Cycle;

    // This time will be set during start, afterwars it will never changed
    uint64_t SimulatedStartTimeInNanoSecond;

    uint64_t SimulatedTimeInNanoSecond;

    // The simulated time elapsed since started
    uint64_t SimulatedTimeSinceStartedInNanoSecond;

    uint64_t TimeOffsetBetweenSimualtedAndRealTimeInNanoSecond;
    int64_t TimeDiffBetweenSymAndSystemInNanoSecond;   // Tis can be also negativ
    int SchedWaitNotFasterState;

    uint64_t SystemTimeInNanoSecond;

    double LastSchedCallTime;
    double SchedPeriodNotFiltered;
    double SchedPeriodFiltered;
    double RealtimeFactorFiltered;

    int  not_faster_than_real_time_old;


    SCHEDULER_STOP_REQ StopReq;

    int SchedulerNr;
    DWORD SchedulerThreadId;
    char SchedulerName[MAX_SCHEDULER_NAME_LENGTH];

    PIPE_API_BASE_CMD_MESSAGE* pTransmitMessageBuffer;
    PIPE_API_BASE_CMD_MESSAGE_ACK* pReceiveMessageBuffer;

    int CycleCounterVid;

    // This will be used inside  RegisterSchedFunc
    int user_sched_func_flag;      // If 1 the function pointer is valid
    void (*user_sched_func) (void);

    int BarrierBehindCount;
    int BarrierBehindNr[8];
    int PosInsideBarrierBehind[8];

    int BarrierBeforeCount;
    int BarrierBeforeNr[8];
    int PosInsideBarrierBefore[8];

    uint32_t BarrierMask;

    CRITICAL_SECTION BreakPointCriticalSection;
    char *BreakPointString;
    int SizeofBreakPointBuffer;
    int BreakPointActive;
    int BreakPointHitFlag;

    int SchedulerHaveRecogizedTerminationRequeFlag;

} SCHEDULER_DATA;

int GetSchedulerCount (void);
SCHEDULER_DATA *GetSchedulerInfos (int par_SchedulerNr);

#define GET_PID() GetCurrentPid()
#define GET_TCB() GetCurrentTcb()

int IsGuiThread(void);
int GetCurrentPid (void);
TASK_CONTROL_BLOCK *GetCurrentTcb (void);
SCHEDULER_DATA *GetCurrentScheduler (void);

void InitPipeSchedulerCriticalSections (void);

void SetCallFromProcess (char *par_CallFromProcess);
int InitPipeSchedulers (char *par_Prefix);
void SchedulersStartingShot (void);
int AddPipeSchedulers (char *par_SchedulerName, int par_IsMainScheduler);
SCHEDULER_DATA *RegisterScheduler  (char *SchedulerName);

PID start_process (const char *ProcessName);
PID start_process_err_msg (char *ProcessName, char **ret_ErrMsg);
PID start_process_env_var (char *ProcessName);
PID start_process_timeout (const char *par_ProcessName, int par_Timeout, char **ret_NoErrMsg);
int get_process_timeout (PID par_Pid);

TASK_CONTROL_BLOCK * GetPointerToTaskControlBlock (PID pid);

int Compare2ProcessNames (const char *pn1, const char *pn2);
int Compare2ExecutableNames (const char *par_Name1, const char *par_Name2);

int TruncatePathFromProcessName (char *DestProcessName, const char *SourceProcessName);
int TruncatePathAndTaskNameFromProcessName (char *DestProcessName, char *SourceProcessName);
int TruncateTaskNameFromProcessName (char *DestProcessName, char *SourceProcessName);

int AddExternPorcessToScheduler (char *ProcessName, char *ExecutableName, int ProcessNumber, int NumberOfProcesses,
                                 DWORD ProcessId, HANDLE hPipe, int SocketOrPipe,
                                 int Priority, int CycleDivider, int Delay,
                                 int IsStartedInDebugger, int MachineType,
                                 HANDLE hAsyncAlivePingReqEvent, HANDLE hAsyncAlivePingAckEvent, char *DllName,
                                 DWORD (*ReadFromConnection) (HANDLE Handle, void *Buffer, DWORD BufferSize, LPDWORD BytesRead),
                                 DWORD (*WriteToConnection) (HANDLE Handle, void *Buffer, int nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten),
                                 void  (*CloseConnectionToExternProcess) (HANDLE Handle),
                                 FILE *ConnectionLoggingFile, uint64_t ExecutableBaseAddress, uint64_t DllBaseAddress, int ExternProcessComunicationProtocolVersion);
void TerminatePipeScheduler (void);

// This wil define the number of cycles the scheduler should run.
// Afterwards it should call the callback function Callback(CallbackParameter)
int make_n_next_cycles (int FromUser, uint64_t nextcyclecount,
                        void (*Callback)(void*), void *CallbackParameter);

SCHEDULER_DATA *GetSchedulerProcessIsRunning (int Pid);
int GetSchedulerNameProcessIsRunning (char *ProcessName, char *ret_SchedulerName, int par_MaxChars);

int GetProsessExeFilename (char *ProcessName, char *ret_ExeFilename, int par_MaxChars);
int GetProcessNameWithoutPath (int pid, char *pname);

int WaitUntilProcessIsNotActiveAndThanLockItEx (int par_Pid, int par_MaxWaitTime, int par_ErrorBehavior, const char *par_OperationDescription,
                                                int *ret_PidsSameExe, int *ret_PidsSameExeCount, int par_SizeOfPidsSameExe,
                                                const char *par_FileName, int par_LineNr);
int WaitUntilProcessIsNotActiveAndThanLockIt (int par_Pid, int par_MaxWaitTime, int par_ErrorBehavior, const char *par_OperationDescription,
                                              const char *par_FileName, int par_LineNr);
#define ERROR_BEHAVIOR_NO_ERROR_MESSAGE  0
#define ERROR_BEHAVIOR_ERROR_MESSAGE     1

int UnLockProcess (int par_Pid);

// following 4 functions will be used only inside Scheduler.c
int WaitUntilProcessIsNotLocked (TASK_CONTROL_BLOCK *pTcb, char *par_FileName, int par_LineNr);
void EndOfProcessCycle (TASK_CONTROL_BLOCK *pTcb);
void InitWaitUntilProcessIsNotActive (TASK_CONTROL_BLOCK *pTcb);
void TerminatWaitUntilProcessIsNotActive (TASK_CONTROL_BLOCK *pTcb);

int get_process_state (PID pid);
int GetProcessLongName (PID par_Pid, char *ret_Name);
int GetProcessShortName (PID par_Pid, char *ret_Name);
int GetProcessExecutableName (PID par_Pid, char *ret_Name);
int GetProcessInsideExecutableName (PID par_Pid, char *ret_Name);
int GetProcessPidAndExecutableAndDllName (char *par_Name, char *ret_ExecutableName, char *ret_DllName, int *ret_ProcessInsideExecutableNumber);
int GetProsessLongExeFilename (const char *par_ProcessName, char *ret_ExeFilename, int par_MaxChars);
int GetProsessShortExeFilename (const char *par_ProcessName, char *ret_ExeFilename, int par_MaxChars);
int GetShortDllFilename (const char *par_DllLongName, char *ret_DllShortName, int par_MaxChars);

// If the return value is not NULL the returned memory must be give free with my_free
char *GetAllProcessNamesSemicolonSeparated (void);
// If the return value is not NULL the returned memory must be give free with my_free
char *GetProcessInsideExecutionFunctionSemicolonSeparated (void);

int read_extprocinfos_from_ini (int par_Fd,
                                const char *ext_proc_name,
                                int *priority,
                                int *time_steps,
                                int *delay,
                                int *timeout,
                                char *RangeErrorCounter,
                                char *RangeControl,
                                unsigned int *RangeControlFlags,
                                char *BBPrefix,
                                char *Scheduler,
                                char *BarriersBeforeOnlySignal,
                                char *BarriersBeforeSignalAndWait,
                                char *BarriersBehindOnlySignal,
                                char *BarriersBehindSignalAndWait,
                                char *BarriersLoopOutBeforeOnlySignal,
                                char *BarriersLoopOutBeforeSignalAndWait,
                                char *BarriersLoopOutBehindOnlySignal,
                                char *BarriersLoopOutBehindSignalAndWait);

int write_extprocinfos_to_ini (int par_Fd,
                               const char *ext_proc_name,
                               int priority,
                               int time_steps,
                               int delay,
                               int timeout,
                               int RangeControlBeforeActiveFlags,
                               int RangeControlBehindActiveFlags,
                               int RangeControlStopSchedFlag,
                               int RangeControlOutput,
                               int RangeErrorCounterFlag,
                               const char *RangeErrorCounter,
                               int RangeControlFlag,
                               const char *RangeControl,
                               int RangeControlPhysFlag,
                               int RangeControlLimitValues,
                               const char *BBPrefix,
                               const char *Scheduler,
                               const char *BarriersBeforeOnlySignal,
                               const char *BarriersBeforeSignalAndWait,
                               const char *BarriersBehindOnlySignal,
                               const char *BarriersBehindSignalAndWait,
                               const char *BarriersLoopOutBeforeOnlySignal,
                               const char *BarriersLoopOutBeforeSignalAndWait,
                               const char *BarriersLoopOutBehindOnlySignal,
                               const char *BarriersLoopOutBehindSignalAndWait);

char *GetNameOfAllSchedulers (void);

uint64_t get_timestamp_counter (void);
uint64_t GetSimulatedTimeInNanoSecond (void);
double GetSimulatedTimeSinceStartedInSecond (void);
uint64_t GetSimulatedTimeSinceStartedInNanoSecond(void);
uint64_t GetSimulatedStartTimeInNanoSecond (void);

uint32_t get_sched_periode_timer_clocks();
uint64_t get_sched_periode_timer_clocks64();

void DelBeforeProcessEquationFile (const char *ProcessName);
void DelBehindProcessEquationFile (const char *ProcessName);

void SetSVLFileLoadedBeforeInitProcessFileName (const char *ProcessName,
                                                const char *SVLFileName, int INIFlag);
int GetSVLFileLoadedBeforeInitProcessFileName (const char *ProcessName,
                                               char *SVLFileName);

int CheckIfAllSchedulerAreTerminated (void);
void TerminateAllSchedulerRequest (void);
void SetTerminateAllSchedulerTimeout (int par_Timeout);
int CheckTerminateAllSchedulerTimeout (void);

int GetSchedulingInformation (int32_t *ret_SeparateCyclesForRefAndInitFunction,
                              uint64_t *ret_SchedulerCycleCounter,
                              int64_t *ret_SchedulerPeriod,
                              uint64_t *ret_MainSchedulerCycleCounter,
                              uint64_t *ret_ProcessCycleCounter,
                              int32_t *ret_ProcessPerid,
                              int32_t *ret_ProcessDelay);

int SchedulerLoopOut (char *par_BarrierBefore, char *par_BarrierBehind, char *SnapShotDataIn, int SnapShotSizeIn, char *SnapShotDataOut);
int SchedulerLogout (int par_ImmediatelyFlag);

#define SCHEDULER_CONTROLED_BY_RPC      2
#define SCHEDULER_CONTROLED_BY_USER     1
#define SCHEDULER_CONTROLED_BY_SYSTEM   0
int disable_scheduler_at_end_of_cycle (int FromUser, void (*Callback)(void*), void *CallbackParameter);

int get_scheduler_state (void);

void enable_scheduler (int FromUser);
void enable_scheduler_with_callback (int FromUser, void (*Callback)(void*), void *CallbackParameter);

void GetStopRequestCounters (int *ret_System, int *ret_User, int *ret_Rpc);

int SetSchedulerBreakPoint (const char *BreakPoint);
int CheckBreakPoint (void);

HANDLE get_extern_process_handle_by_pid (int pid);
DWORD get_extern_process_windows_id_by_pid (int pid);

int lives_process_inside_dll (const char *ProcessName, char *ret_DllName);

int GetExternProcessIndexInsideExecutable (int Pid);
int IsExternProcess64BitExecutable (int Pid);
int IsExternProcessLinuxExecutable (int Pid);
int GetExternProcessMachineType (int Pid);

char *GetSchedulerName (int par_Index);
int GetSchedulerIndex (SCHEDULER_DATA *par_Scheduler);


int get_process_info_ex (PID pid, char *ret_Name, int maxc_Name, int *ret_Type, int *ret_Prio, int *ret_Cycles, int *ret_Delay, int *ret_MessageSize,
                         char *ret_AssignedScheduler, int maxc_AssignedScheduler, uint64_t *ret_bb_access_mask,
                         int *ret_State, uint64_t *ret_CyclesCounter, uint64_t *ret_CyclesTime, uint64_t *ret_CyclesTimeMax, uint64_t *ret_CyclesTimeMin);

int get_process_info_internal_debug (PID pid,
                                     HANDLE *ret_hPipe,

                                     int *ret_SocketOrPipe,

                                     int *ret_IsStartedInDebugger,  // This is 1 if the process are started inside a debugger (remark the process can be attached afterwards, this will not recognized)
                                     int *ret_MachineType,          // If 0 -> 32Bit Windows 1 -> 64Bit Windows, 2 -> 32Bit Linux, 3 -> 64Bit Linux

                                     int *ret_UseDebugInterfaceForWrite,
                                     int *ret_UseDebugInterfaceForRead,

                                     int *ret_wait_on,             // This is 1 if the scheduler waits on this process


                                     int *ret_WorkingFlag,   // Process is inside cyclic/init/terminate/reference Function
                                     int *ret_LockedFlag,    // Process cannot enter its cyclic/init/terminate/reference Function the scheduler have to wait
                                     int *ret_SchedWaitForUnlocking,  // Scheduler waits that he can call cyclic/init/terminate/reference function of the process
                                     int *ret_WaitForEndOfCycleFlag,  // Somebody is waiting that cyclic/init/terminate/reference Function will be returned

                                     DWORD *ret_LockedByThreadId,
                                     char *ret_SrcFileName,
                                     int maxc_SrcFileName,
                                     int *ret_SrcLineNr,
                                     char *ret_DllName,     // If the process is a *.DLL/*.so this will be the file name otherwise this is NULL
                                     int maxc_DllName,
                                     char *ret_InsideExecutableName,
                                     int maxc_InsideExecutableName,
                                     int *ret_NumberInsideExecutable,


                                     uint64_t *ret_ExecutableBaseAddress,
                                     uint64_t *ret_DllBaseAddress,

                                     int *ret_ExternProcessComunicationProtocolVersion);


TASK_CONTROL_BLOCK *get_process_info (PID pid);

PID get_pid_by_name (const char *name);
int get_name_by_pid (PID pid, char *name);


double GetRealtimeFactor(void);

uint32_t GetCycleCounter (void);
uint64_t GetCycleCounter64 (void);

void RegisterSchedFunc (void (*user_sched_func) (void));

void UnRegisterSchedFunc (void);

int activate_extern_process (const char *name, int timeout, char **ret_NoErrMsg);
void activate_extern_process_free_err_msg (char *par_ErrMsg);

PID start_process_ex (const char *name, int Prio, int Cycle, int Delay, int Timeout,
                      const char *SVLFile,
                      const char *A2LFile, int A2LFlags,
                      const char *BBPrefix,
                      int UseRangeControl,  // If this is 0 ignore all following "RangeControl*" parameters
                      int RangeControlBeforeActiveFlags,
                      int RangeControlBehindActiveFlags,
                      int RangeControlStopSchedFlag,
                      int RangeControlOutput,
                      int RangeErrorCounterFlag,
                      const char *RangeErrorCounter,
                      int RangeControlVarFlag,
                      const char *RangeControl,
                      int RangeControlPhysFlag,
                      int RangeControlLimitValues,
                      char **ret_ErrMsg);

int terminate_process (PID pid);

int GetExternProcessBaseAddress (int Pid, uint64_t *ret_Address, char *ExecutableName);

int get_real_running_process_name (char *pname);

typedef struct {
    char *Ptr;
    char *BasePtr;
    int RunOrSleep;
    int First;
} READ_NEXT_PROCESS_NAME;

READ_NEXT_PROCESS_NAME *init_read_next_process_name (int run_sleep);
char *read_next_process_name (READ_NEXT_PROCESS_NAME *buffer);
void close_read_next_process_name(READ_NEXT_PROCESS_NAME *buffer);

// This function checks it it is an internal process name
// It will return 1 for an internal running process
// It will return 2 for an internal not running process
// If it is not an internal process the return value is 0
int IsInternalProcess (char *ProcessName);

int SetPriority(char *txt);

void SetSVLFileLoadedBeforeInitProcessFileName (const char *ProcessName,
                                                const char *SVLFileName, int INIFlag);
int GetSVLFileLoadedBeforeInitProcessFileName (const char *ProcessName,
                                               char *SVLFileName);
void SetA2LFileAssociatedToProcessFileName (const char *ProcessName,
                                            const char *A2LFileName, int UpdateAddrFlag, int INIFlag);
int GetA2LFileAssociatedProcessFileName (const char *ProcessName,
                                         char *A2LFileName,
                                         int *UpdateAddrFlag);
int DisconnectA2LFromProcess(int par_Pid);


void SetBeforeProcessEquationFileName (const char *ProcessName,
                                       const char *EquFileName);
void SetBehindProcessEquationFileName (const char *ProcessName,
                                       const char *EquFileName);
int GetBeforeProcessEquationFileName (const char *ProcessName,
                                      char *EquFileName);
int GetBehindProcessEquationFileName (const char *ProcessName,
                                      char *EquFileName);
void DelBeforeProcessEquationFile (const char *ProcessName);
void DelBehindProcessEquationFile (const char *ProcessName);


int GetToHostPCFifoHandle(void);

unsigned int GetProcessRangeControlFlags(int par_Pid);

extern TASK_CONTROL_BLOCK GuiTcb;

int GetGuiPid(void);

void RemoveAllPendingStartStopSchedulerRequestsOfThisThread(int par_DisableCounter);

#endif
