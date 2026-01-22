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


#ifndef EXECUTIONSTACK_H
#define EXECUTIONSTACK_H

#include <stdint.h>

#include "Blackboard.h"
#include "ReadCanCfg.h"

typedef struct  {
    int Pointer;
    struct STACK_ENTRY {
        union FloatOrInt64 V;
        uint32_t Type;  // 0 -> 64Bit Integer, 1 -> Unsigned Interger,  2 -> 64Bit Float 3 -> undefined (error)
        uint32_t ByteWidth;  // 1-> int8_t, 2-> int16_t, 4 -> int32_t, 8 -> int64_t
    } Data[64];
    struct STACK_ENTRY Parameter;
    int cs;    // if this is 0 than the thread already has hold the BlackboardCriticalSection
    NEW_CAN_SERVER_OBJECT *CanObject;
    int VidReplaceWithFixedValue;
    int NoBBSpinlockFlag;
    int WriteVariableEnable;
} EXECUTION_STACK;

union OP_PARAM {
    struct {
        int vid;
    } variref;
    double value;
    uint64_t unique_number;
    uint64_t value_ui64;
    int64_t value_i64;
};

struct EXEC_STACK_ELEM {
    short op_pos;
    short Fill1;
    int Fill2;
    union OP_PARAM param;
};    /* 16 bytes */

typedef int (*UserDefinedBuildinFunctionExecuteType) (EXECUTION_STACK *pStack, struct EXEC_STACK_ELEM *param);

void init_stack(EXECUTION_STACK *Stack, double Parameter);
int direct_execute_one_command (int Cmd, EXECUTION_STACK *Stack, union OP_PARAM Value);
double get_current_stack_value(EXECUTION_STACK *Stack);
int get_current_stack_union(EXECUTION_STACK *Stack, union BB_VARI *Ret, int *ret_BBType);
int get_current_stack_FloatOrInt64 (EXECUTION_STACK *Stack, union FloatOrInt64 *ret_Value);
int add_stack_element_FloatOrInt64 (EXECUTION_STACK *Stack, union FloatOrInt64 Value, int Type, int ByteWidth);

int attach_exec_stack_by_pid(int pid, struct EXEC_STACK_ELEM *exec_stack);
int attach_exec_stack_cs (struct EXEC_STACK_ELEM *exec_stack, int cs);
int attach_exec_stack (struct EXEC_STACK_ELEM *exec_stack);
void detach_exec_stack_cs (struct EXEC_STACK_ELEM *start_exec_stack, int cs);
void detach_exec_stack (struct EXEC_STACK_ELEM *start_exec_stack);
int copy_exec_stack (struct EXEC_STACK_ELEM *dest_exec_stack,
                     struct EXEC_STACK_ELEM *src_exec_stack);
int sizeof_exec_stack (struct EXEC_STACK_ELEM *start_exec_stack);
double execute_stack_cs (struct EXEC_STACK_ELEM *start_exec_stack, int cs);
double execute_stack (struct EXEC_STACK_ELEM *start_exec_stack);
int execute_stack_ret_err_code_cs (struct EXEC_STACK_ELEM *start_exec_stack, double *pRet, int cs);
int execute_stack_ret_err_code (struct EXEC_STACK_ELEM *start_exec_stack, double *pRet);
double execute_stack_whith_parameter_cs (struct EXEC_STACK_ELEM *start_exec_stack, double parameter, int cs);
double execute_stack_whith_parameter (struct EXEC_STACK_ELEM *start_exec_stack, double parameter);
int execute_stack_whith_can_parameter_cs (struct EXEC_STACK_ELEM *start_exec_stack, union FloatOrInt64 parameter, int parameter_type, NEW_CAN_SERVER_OBJECT *CanObject, union FloatOrInt64 *ret_value, int cs);
int execute_stack_whith_can_parameter (struct EXEC_STACK_ELEM *start_exec_stack, union FloatOrInt64 parameter, int parameter_type, NEW_CAN_SERVER_OBJECT *CanObject, union FloatOrInt64 *ret_value);
int add_exec_stack_elem (int command, char *name, union OP_PARAM value,
                         struct EXEC_STACK_ELEM **pExecStack, int cs, int Pid);
void remove_exec_stack_cs (struct EXEC_STACK_ELEM *start_exec_stack, int cs);
void remove_exec_stack (struct EXEC_STACK_ELEM *start_exec_stack);
void remove_exec_stack_for_process_cs (struct EXEC_STACK_ELEM *start_exec_stack, int cs, int pid);
void remove_exec_stack_for_process (struct EXEC_STACK_ELEM *start_exec_stack, int pid);

double execute_stack_replace_variable_with_parameter_cs (struct EXEC_STACK_ELEM *start_exec_stack, int vid, double parameter, int cs);
double execute_stack_replace_variable_with_parameter (struct EXEC_STACK_ELEM *start_exec_stack, int vid, double parameter);

int FindFunctionByKey (int Key);

int convert_to_stack_entry (union BB_VARI Value, int BBType, struct STACK_ENTRY *Ret);

int AddUserDefinedBuildinFunctionToExecutor(int par_Number, UserDefinedBuildinFunctionExecuteType par_UserDefinedBuildinFunctionExecute);

#endif
