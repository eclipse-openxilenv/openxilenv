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


#define RPC_CONTROL_C
#include "Config.h"
#include "Scheduler.h"
#include "Message.h"
#include "EquationParser.h"
#include "ExecutionStack.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "CanRecorder.h"
#include "RpcControlProcess.h"

#define MAX_CONTROL_WAIT 8

struct CONTROL_WAIT {
    volatile int InUseFlag;
    volatile int ValidFlag;
    void *Connection;
    uint32_t DelayCycleCounter;
    int MessageIDWaitingFor;
    void (*FuncPtrCallBehindDelay) (int TimeoutFlag, uint32_t Remainder, void* Param);
    void (*FuncPtrCallBeforeDelay) (void* Param);
    void *ParamCallBehindDelay;
    void *ParamCallBeforeDelay;
    char *WaitForEquation;
    struct EXEC_STACK_ELEM *WaitForExecStack;
};

static struct CONTROL_WAIT cws[MAX_CONTROL_WAIT];

static CRITICAL_SECTION CriticalSection;


static struct CONTROL_WAIT* GetFreeControlWait (void)
{
    int x;
    for (x = 0; x < MAX_CONTROL_WAIT; x++) {
        if (!cws[x].InUseFlag) {
            //RpCDebugPrint("GetFreeControlWait() x = %i\n", x);
            cws[x].InUseFlag = 1;
            return &(cws[x]);
        }
    }
    return NULL;
}

void SetDelayFunc (void *par_Connection, uint32_t par_Delay,
                   void (*par_BeforeFuncPtr) (void* Param), void *par_BeforeParm,
                   void (*par_BehindFuncPtr) (int TimeoutFlag, uint32_t Remainder, void* Param), void *par_BehindParm,
                   int par_WaitingForMsgId, char *Equation)
{
    struct CONTROL_WAIT *cw;
    ENTER_CS (&CriticalSection);

    cw = GetFreeControlWait ();
    if (cw == NULL) {
        LEAVE_CS (&CriticalSection);
        ThrowError (1, "not enough RPC slots max. (%i)", MAX_CONTROL_WAIT);
        return;
    }
    cw->Connection = par_Connection;
    cw->FuncPtrCallBehindDelay = par_BehindFuncPtr;
    cw->FuncPtrCallBeforeDelay = par_BeforeFuncPtr;
    cw->ParamCallBehindDelay = par_BehindParm;
    cw->ParamCallBeforeDelay = par_BeforeParm;
    cw->DelayCycleCounter = GetCycleCounter () + par_Delay - 1;

    cw->MessageIDWaitingFor = par_WaitingForMsgId;
    if (Equation != NULL) {
        char *Help;
        int Len = (int)strlen (Equation) + 1;
        Help = (char*)my_malloc (Len);
        if (Help != NULL) {
            MEMCPY (Help, Equation, (size_t)Len);
        }
        cw->WaitForEquation = Help;
    }
    cw->ValidFlag = 1;
    LEAVE_CS (&CriticalSection);
}

void CleanUpWaitFuncForConnection(void *par_Connection)
{
    int i;
    ENTER_CS (&CriticalSection);
    for (i = 0; i < MAX_CONTROL_WAIT; i++) {
        if (cws[i].ValidFlag && cws[i].InUseFlag) {
            if (cws[i].Connection == par_Connection) {
                cws[i].ValidFlag = cws[i].InUseFlag = 0;
            }
        }
    }
    LEAVE_CS (&CriticalSection);
}

static void ExitWaitFor (int Index, int Flag, unsigned int Remainder) 
{
    cws[Index].ValidFlag = 0;
    if (cws[Index].WaitForExecStack != NULL) remove_exec_stack (cws[Index].WaitForExecStack);
    cws[Index].WaitForExecStack = NULL;
    if (cws[Index].FuncPtrCallBehindDelay) {
        cws[Index].FuncPtrCallBehindDelay (Flag, Remainder, cws[Index].ParamCallBehindDelay);
        //RpCDebugPrint("callback FuncPtrCallBehindDelay(%i, %u, %p)\n", Flag, Remainder, cws[Index].ParamCallBehindDelay);
    }
    cws[Index].FuncPtrCallBehindDelay = NULL;
    cws[Index].MessageIDWaitingFor = 0;
    cws[Index].DelayCycleCounter = 0;
    cws[Index].InUseFlag = 0;
}


static int CanRecorderCount;
static struct CAN_RECORDER *CanRecorders[16];

int AddCanRecorder(struct CAN_RECORDER *par_CanRecorders)
{
    int Ret;
    ENTER_CS (&CriticalSection);
    if (CanRecorderCount < 16) {
        CanRecorders[CanRecorderCount] = par_CanRecorders;
        CanRecorderCount++;
        Ret = 0;
    } else {
        Ret = -1;
    }
    LEAVE_CS (&CriticalSection);
    return Ret;
}

static int RemoveCanRecorderAlreadyLocked(struct CAN_RECORDER *par_CanRecorders)
{
    int Ret = -1;
    //ENTER_CS (&CriticalSection);
    for (int x = 0; x < CanRecorderCount; x++) {
        if (CanRecorders[x] == par_CanRecorders) {
            for (x++; x < CanRecorderCount; x++) {
                CanRecorders[x - 1] = CanRecorders[x];
            }
            CanRecorderCount--;
            Ret = 0;
            break;
        }
    }
    //LEAVE_CS (&CriticalSection);
    return Ret;
}

static void CyclicRPCControl (void)
{
    int i;
    uint32_t TimeCounter;
    ENTER_CS (&CriticalSection);
    for (i = 0; i < MAX_CONTROL_WAIT; i++) {
        if (cws[i].ValidFlag && cws[i].InUseFlag) {
            if (cws[i].FuncPtrCallBeforeDelay != NULL) {
                cws[i].FuncPtrCallBeforeDelay(cws[i].ParamCallBeforeDelay);
                cws[i].FuncPtrCallBeforeDelay = NULL;
            } else {
                if (cws[i].WaitForEquation != NULL) {
                    cws[i].WaitForExecStack = solve_equation (cws[i].WaitForEquation);
                    my_free (cws[i].WaitForEquation);
                    cws[i].WaitForEquation = NULL;
                }
                // First check for timeouts
                if (cws[i].DelayCycleCounter) {
                    TimeCounter = GetCycleCounter ();
                    if (TimeCounter >= cws[i].DelayCycleCounter) {
                        ExitWaitFor (i, 1, 0);
                    } else {
                        // remaining timeout
                        TimeCounter = cws[i].DelayCycleCounter - TimeCounter;
                    }
                } else {
                    TimeCounter = 0;
                }

                // WaitUntil is active
                if (cws[i].WaitForExecStack != NULL) {
                    double Erg;
                    Erg = execute_stack (cws[i].WaitForExecStack);
                    if (!((Erg > -0.5) && (Erg < 0.5))) {
                        ExitWaitFor (i, 0, TimeCounter);
                    }
                } else if (cws[i].MessageIDWaitingFor) {
                    MESSAGE_HEAD mhead;
                    if (test_message (&mhead)) {
                        if (mhead.mid == cws[i].MessageIDWaitingFor) {
                            ExitWaitFor (i, 0, TimeCounter);
                        }
                        remove_message();
                    }
                }
            }
        }
    }

    for (int x = 0; x < CanRecorderCount; x++) {
        if (CanRecorderReadAndProcessMessages(CanRecorders[x])) {
            RemoveCanRecorderAlreadyLocked(CanRecorders[x]);
            FreeCanRecorder(CanRecorders[x]);
        }
    }
    LEAVE_CS (&CriticalSection);
}

static int RPCControlPid;

int GetRPCControlPid (void)
{
    return RPCControlPid;
}

static int InitRPCControl (void)
{
    RPCControlPid = get_pid_by_name ("RPC_Control");
    return 0;
}

static void TerminateRPCControl (void)
{
    int x;
    for (x = 0;  x < MAX_CONTROL_WAIT; x++) {
        cws[x].InUseFlag = 0;
        cws[x].ValidFlag = 0;
        cws[x].Connection = NULL;
        cws[x].MessageIDWaitingFor = 0;
        cws[x].ParamCallBehindDelay = NULL;
        cws[x].ParamCallBeforeDelay = NULL;
        cws[x].FuncPtrCallBeforeDelay = NULL;
        cws[x].FuncPtrCallBehindDelay = NULL;
        cws[x].DelayCycleCounter = 0;
    }
    RPCControlPid = -1;
}


TASK_CONTROL_BLOCK rpc_controll_tcb
    = INIT_TASK_COTROL_BLOCK("RPC_Control", INTERN_ASYNC, 2000, CyclicRPCControl, InitRPCControl, TerminateRPCControl, 32*1024);


void InitRPCControlWaitStruct(void)
{
    int x;

    for (x = 0;  x < MAX_CONTROL_WAIT; x++) {
        cws[x].InUseFlag = 0;
        cws[x].ValidFlag = 0;
    }
    INIT_CS (&CriticalSection);
}
