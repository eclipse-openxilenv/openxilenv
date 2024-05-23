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


#include <alloca.h>
#include "RealtimeScheduler.h"
#include "ExecutionStack.h"
#include "MyMemory.h"
#include "ThrowError.h"

#include "RemoteMasterLock.h"
#include "RealtimeProcessEquations.h"


//#define __AcquireSpinlock(SpinlockAddress) 
//#define __ReleaseSpinlock(SpinlockAddress) 


extern REMOTE_MASTER_LOCK SchedMessageGlobalSpinlock;
#define __AcquireSpinlock(s,l,f) RemoteMasterLock (s,l,f);
#define __ReleaseSpinlock(s) RemoteMasterUnlock (s);


/*
static void DebugPrintEquations(TASK_CONTROL_BLOCK *tcb)
{
    int x;
    error(1, "Process Name: %s", tcb->name);
    for (x = 0; x < tcb->BeforeProcessEquationCounter; x++) {
        if (tcb->BeforeProcessEquations[x].ExecStack == NULL) break;
        error(1, "  Equation Nr %i, (0x%p)", (int)tcb->BeforeProcessEquations[x].Nr,
            tcb->BeforeProcessEquations[x].ExecStack);
        print_exec_stack(tcb->BeforeProcessEquations[x].ExecStack);
    }
}*/

void ClacBeforeProcessEquations(TASK_CONTROL_BLOCK *tcb)
{
    int x;

    //DebugPrintEquations (tcb);
    for (x = 0; x < tcb->BeforeProcessEquationCounter; x++) {
        if (tcb->BeforeProcessEquations[x].ExecStack == NULL) break;
        execute_stack((struct EXEC_STACK_ELEM *)tcb->BeforeProcessEquations[x].ExecStack);
    }
}

void ClacBehindProcessEquations(TASK_CONTROL_BLOCK *tcb)
{
    int x;

    for (x = 0; x < tcb->BehindProcessEquationCounter; x++) {
        if (tcb->BehindProcessEquations[x].ExecStack == NULL) break;
        execute_stack((struct EXEC_STACK_ELEM *)tcb->BehindProcessEquations[x].ExecStack);
    }
}


int AddBeforeProcessEquation(int Nr, int Pid, void *ExecStack)
{
    int x, y;
    TASK_CONTROL_BLOCK *tcb;
    struct EXEC_STACK_ELEM *NewExecStack;
    int Ret;

    NewExecStack = my_malloc(sizeof_exec_stack((struct EXEC_STACK_ELEM*)ExecStack));
    if (NewExecStack == NULL) {
        return -102;
    }
    copy_exec_stack(NewExecStack,
        (struct EXEC_STACK_ELEM*)ExecStack);
    attach_exec_stack_by_pid(Pid, NewExecStack);

    __AcquireSpinlock(&SchedMessageGlobalSpinlock, __LINE__, __FILE__);
    tcb = GetPointerToTaskControlBlock(Pid);
    if (tcb == NULL) {
        __ReleaseSpinlock(&SchedMessageGlobalSpinlock);
        remove_exec_stack_for_process(NewExecStack, Pid);
        my_free(NewExecStack);
        return -101;
    }

    x = 0;
    while (1) {
        for (; x < tcb->BeforeProcessEquationCounter; x++) {
            if (tcb->BeforeProcessEquations[x].ExecStack == NULL) {
                tcb->BeforeProcessEquations[x].Nr = Nr;
                tcb->BeforeProcessEquations[x].ExecStack = NewExecStack;
                // DebugPrintEquations (tcb);
                Ret = 0;
                goto __RETURN;
            }
        }
        tcb->BeforeProcessEquationCounter += 10;
        tcb->BeforeProcessEquations = (PROC_EXEC_STACK*)my_realloc(tcb->BeforeProcessEquations,
            (size_t)tcb->BeforeProcessEquationCounter * sizeof(PROC_EXEC_STACK));
        if (tcb->BeforeProcessEquations == NULL) {
            tcb->BeforeProcessEquationCounter = 0;
            Ret = -103;
            goto __RETURN;
        }
        for (y = x; y < tcb->BeforeProcessEquationCounter; y++) {
            tcb->BeforeProcessEquations[y].ExecStack = NULL;
        }
    }
__RETURN:
    __ReleaseSpinlock(&SchedMessageGlobalSpinlock);
    return Ret;
}


int AddBehindProcessEquation(int Nr, int Pid, void *ExecStack)
{
    int x, y;
    TASK_CONTROL_BLOCK *tcb;
    struct EXEC_STACK_ELEM *NewExecStack;
    int Ret;

    NewExecStack = my_malloc(sizeof_exec_stack((struct EXEC_STACK_ELEM*)ExecStack));
    if (NewExecStack == NULL) {
        return -102;
    }
    copy_exec_stack(NewExecStack,
        (struct EXEC_STACK_ELEM*)ExecStack);
    attach_exec_stack_by_pid(Pid, NewExecStack);

    __AcquireSpinlock(&SchedMessageGlobalSpinlock, __LINE__, __FILE__);
    tcb = GetPointerToTaskControlBlock(Pid);
    if (tcb == NULL) {
        __ReleaseSpinlock(&SchedMessageGlobalSpinlock);
        remove_exec_stack_for_process(NewExecStack, Pid);
        my_free(NewExecStack);
        return -101;
    }

    x = 0;
    while (1) {
        for (; x < tcb->BehindProcessEquationCounter; x++) {
            if (tcb->BehindProcessEquations[x].ExecStack == NULL) {
                tcb->BehindProcessEquations[x].Nr = Nr;
                tcb->BehindProcessEquations[x].ExecStack = NewExecStack;
                //DebugPrintEquations (tcb);
                Ret = 0;
                goto __RETURN;
            }
        }
        tcb->BehindProcessEquationCounter += 10;
        tcb->BehindProcessEquations = (PROC_EXEC_STACK*)my_realloc(tcb->BehindProcessEquations,
            (size_t)tcb->BehindProcessEquationCounter * sizeof(PROC_EXEC_STACK));
        if (tcb->BehindProcessEquations == NULL) {
            tcb->BehindProcessEquationCounter = 0;
            Ret = -103;
            goto __RETURN;
        }
        for (y = x; y < tcb->BehindProcessEquationCounter; y++) {
            tcb->BehindProcessEquations[y].ExecStack = NULL;
        }
    }
__RETURN:
    __ReleaseSpinlock(&SchedMessageGlobalSpinlock);
    return Ret;
}


int DelBeforeProcessEquations(int Nr, int Pid)
{
    int x, y;
    PROC_EXEC_STACK Help;
    TASK_CONTROL_BLOCK *tcb;
    struct EXEC_STACK_ELEM **ExecStacks;
    int c;

    __AcquireSpinlock(&SchedMessageGlobalSpinlock, __LINE__, __FILE__);
    tcb = GetPointerToTaskControlBlock(Pid);
    if (tcb == NULL) {
        __ReleaseSpinlock(&SchedMessageGlobalSpinlock);
        return -1;
    }
    ExecStacks = alloca((size_t)tcb->BeforeProcessEquationCounter * sizeof(struct EXEC_STACK_ELEM *));
    if (ExecStacks == NULL) {
        ThrowError(1, "cannot delete equation(s) before process because not enough realtime memory");
        __ReleaseSpinlock(&SchedMessageGlobalSpinlock);
        return -1;
    }
    for (c = y = x = 0; x < tcb->BeforeProcessEquationCounter; x++) {
        if (tcb->BeforeProcessEquations[x].ExecStack == NULL) break;
        if ((Nr == -1) || (tcb->BeforeProcessEquations[x].Nr == Nr)) {
            ExecStacks[c] = (struct EXEC_STACK_ELEM *)tcb->BeforeProcessEquations[x].ExecStack;
            tcb->BeforeProcessEquations[x].ExecStack = NULL;
            c++;
        }
        else {
            Help = tcb->BeforeProcessEquations[x];
            tcb->BeforeProcessEquations[x].ExecStack = NULL;
            tcb->BeforeProcessEquations[y] = Help;
            y++;
        }
    }
    __ReleaseSpinlock(&SchedMessageGlobalSpinlock);
    // das muss ausserhalb von SchedMessageGlobalSpinlock geschehen da write_message aufgerufen wird
    for (x = 0; x < c; x++) {
        remove_exec_stack_for_process(ExecStacks[x], Pid);
    }
    //my_free (ExecStacks); 
    return 0;
}

int DelBehindProcessEquations(int Nr, int Pid)
{
    int x, y;
    PROC_EXEC_STACK Help;
    TASK_CONTROL_BLOCK *tcb;
    struct EXEC_STACK_ELEM **ExecStacks;
    int c;

    __AcquireSpinlock(&SchedMessageGlobalSpinlock, __LINE__, __FILE__);
    tcb = GetPointerToTaskControlBlock(Pid);
    if (tcb == NULL) {
        __ReleaseSpinlock(&SchedMessageGlobalSpinlock);
        return -1 ;
    }
    ExecStacks = alloca((size_t)tcb->BehindProcessEquationCounter * sizeof(struct EXEC_STACK_ELEM *));
    if (ExecStacks == NULL) {
        ThrowError(1, "cannot delete equation(s) before process because not enough realtime memory");
        __ReleaseSpinlock(&SchedMessageGlobalSpinlock);
        return -1;
    }
    for (c = y = x = 0; x < tcb->BehindProcessEquationCounter; x++) {
        if (tcb->BehindProcessEquations[x].ExecStack == NULL) break;
        if ((Nr == -1) || (tcb->BehindProcessEquations[x].Nr == Nr)) {
            ExecStacks[c] = (struct EXEC_STACK_ELEM *)tcb->BehindProcessEquations[x].ExecStack;
            tcb->BehindProcessEquations[x].ExecStack = NULL;
            c++;
        }
        else {
            Help = tcb->BehindProcessEquations[x];
            tcb->BehindProcessEquations[x].ExecStack = NULL;
            tcb->BehindProcessEquations[y] = Help;
            y++;
        }
    }
    __ReleaseSpinlock(&SchedMessageGlobalSpinlock);
    // das muss ausserhalb von SchedMessageGlobalSpinlock geschehen da write_message aufgerufen wird
    for (x = 0; x < c; x++) {
        remove_exec_stack_for_process(ExecStacks[x], Pid);
    }
    //my_free (ExecStacks); 
    return 0;
}


