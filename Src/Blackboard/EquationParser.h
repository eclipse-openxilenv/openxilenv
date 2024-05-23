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


#ifndef EQUATIONPARSER_H
#define EQUATIONPARSER_H

#include "Blackboard.h"

#define BUILDIN_FUNC_MAX_PARAMETER  32

typedef enum {
    EQU_OP_NAME, EQU_OP_NUMBER, EQU_OP_END, EQU_OP_COMMAND, EQU_OP_EQUAL,
    EQU_OP_NOT_EQUAL, EQU_OP_LOGICAL_AND, EQU_OP_LOGICAL_OR, EQU_OP_SMALLER_EQUAL, EQU_OP_LARGER_EQUAL,
    EQU_OP_READ_BBVARI, EQU_OP_WRITE_BBVARI, EQU_OP_NEG,
    EQU_OP_SINUS, EQU_OP_COSINUS, EQU_OP_TANGENS, EQU_OP_ARC_SINUS, EQU_OP_ARC_COSINUS, EQU_OP_ARC_TANGENS,
    EQU_OP_EXP, EQU_OP_POW, EQU_OP_SQRT, EQU_OP_LOG, EQU_OP_LOG10, EQU_OP_GET_PARAM,
    EQU_OP_GETBITS, EQU_OP_SETBITS, EQU_OP_ANDBITS, EQU_OP_ORBITS, EQU_OP_XORBITS,

    EQU_OP_BITWISE_AND, EQU_OP_BITWISE_OR, EQU_OP_BITWISE_XOR, EQU_OP_BITWISE_INVERT,
    EQU_OP_PLUS, EQU_OP_MINUS, EQU_OP_MUL, EQU_OP_DIV,
    EQU_OP_ASSIGNMENT, EQU_OP_OPEN_BRACKETS, EQU_OP_CLOSE_BRACKETS, EQU_OP_SMALLER, EQU_OP_LARGER, EQU_OP_NOT,

    EQU_OP_CANBYTE, EQU_OP_CANWORD, EQU_OP_CANDWORD,
    EQU_OP_ABS, EQU_OP_ROUND, EQU_OP_ROUND_UP, EQU_OP_ROUND_DOWN,
    EQU_OP_CAN_CYCLIC, EQU_OP_CAN_DATA_CHANGED,
    EQU_OP_MODULO, EQU_OP_ADD_MSB_LSB, EQU_OP_OVERFLOW, EQU_OP_UNDERFLOW,
    EQU_OP_MIN, EQU_OP_MAX, EQU_OP_SWAP16, EQU_OP_SWAP32, EQU_OP_SHIFT_LEFT, EQU_OP_SHIFT_RIGHT,
    EQU_OP_SINUS_HYP, EQU_OP_COSINUS_HYP, EQU_OP_TANGENS_HYP,
    EQU_OP_ONES_COMPLEMENT_BBVARI, EQU_OP_GET_BINARY_BBVARI, EQU_OP_SET_BINARY_BBVARI,
    EQU_OP_READ_BBVARI_PHYS, EQU_OP_WRITE_BBVARI_PHYS,
    EQU_OP_HAS_CHANGED, EQU_OP_HAS_CHANGED_NEXT,
    EQU_OP_SLOPE_UP, EQU_OP_SLOPE_UP_NEXT, EQU_OP_SLOPE_DOWN, EQU_OP_SLOPE_DOWN_NEXT, EQU_OP_CRC8_USER_POLY,
    EQU_OP_CAN_ID, EQU_OP_CAN_SIZE, EQU_OP_ADD_MSN_LSN,
    EQU_OP_CAN_CRC8_REV_IN_OUT,
    EQU_OP_CRC16_USER_POLY, EQU_OP_EQUAL_DECIMALS,
    EQU_OP_SWAP_ON_STACK,
    EQU_OP_TO_DOUBLE, EQU_OP_TO_INT8, EQU_OP_TO_INT16, EQU_OP_TO_INT32, EQU_OP_TO_INT64,
    EQU_OP_TO_UINT8, EQU_OP_TO_UINT16, EQU_OP_TO_UINT32, EQU_OP_TO_UINT64,
    EQU_OP_NUMBER_INT8, EQU_OP_NUMBER_INT16, EQU_OP_NUMBER_INT32, EQU_OP_NUMBER_INT64,
    EQU_OP_NUMBER_UINT8, EQU_OP_NUMBER_UINT16, EQU_OP_NUMBER_UINT32, EQU_OP_NUMBER_UINT64,
    EQU_OP_GET_CALC_DATA_TYPE, EQU_OP_GET_CALC_DATA_WIDTH,
    EQU_OP_CRC8_USER_POLY_REFLECT, EQU_OP_CRC16_USER_POLY_REFLECT, EQU_OP_CRC32_USER_POLY_REFLECT,
    EQU_OP_CAN_CYCLES,
    EQU_OP_PLUS_EQUAL=1000, EQU_OP_MINUS_EQUAL=1001, EQU_OP_MUL_EQUAL=1002, EQU_OP_DIV_EQUAL=1003,
    EQU_OP_NAME_PHYS=1004, EQU_OP_NAME_PHYS_REF=1005
} EQU_TOKEN;


/* Translate a equation string to an binary execution stack */
struct EXEC_STACK_ELEM *solve_equation (const char *equation);
struct EXEC_STACK_ELEM *solve_equation_err_string (const char *equation, char **errstring);
struct EXEC_STACK_ELEM *solve_equation_no_add_bbvari (const char *equation, char **errstring, int AddNotExistingVars);
//struct EXEC_STACK_ELEM *solve_equation_no_add_bbvari_err_string (const char *equation, char **errstring);
int solve_equation_for_process(const char *equation, int pid, int AddNotExistingVars,
                               struct EXEC_STACK_ELEM **ret_ExecStack, char **errstring);

/* Calculate a equation string immediately to a resulting double value */
double direct_solve_equation (const char *equation);
double direct_solve_equation_script_stack (const char *equation, int StackPos);
double direct_solve_equation_no_error_message (const char *equation);

/* Calculate a equation string immediately to a resulting double value
   But this functions will not automatic add variable if they don't exists */
double direct_solve_equation_no_add_bbvari (const char *equation);
double direct_solve_equation_no_add_bbvari_pid (const char *equation, int pid);
double direct_solve_equation_script_stack_no_add_bbvari (const char *equation, int StackPos);

/* Calculate a equation string immediately to a resulting double value
   But this functions will not automatic add variable if they don't exists
   and give back an error state */
int direct_solve_equation_err_sate (const char *equation, double *erg);
int direct_solve_equation_script_stack_err_sate (char *equation, double *erg, int StackPos);

/* Calculate a equation string immediately to a resulting double value
   But this functions will not automatic add variable if they don't exists
   and give back an error state and an error string */
int direct_solve_equation_err_string (const char *equation, double *erg, char **errstring);
int direct_solve_equation_script_stack_err_string (const char *equation, double *erg, char **errstring, int StackPos,
                                                  int AddNotExistingVars);
int direct_solve_equation_script_stack_err_string_float_or_int64 (const char *equation, union FloatOrInt64 *ret_ResultValue,
                                                                 int *ret_ResultType, char **errstring, int StackPos,
                                                                 int AddNotExistingVars);

struct EXEC_STACK_ELEM *solve_equation_replace_parameter (const char *equation);
struct EXEC_STACK_ELEM *solve_equation_replace_parameter_with_can (const char *equation, int *pUseCANData);
struct EXEC_STACK_ELEM *solve_equation_replace_parameter_with_flexray (const char *equation, int *pUseFlexrayData);
int direct_solve_equation_err_state_replace_value (const char *equation, double value, double *erg);


// This will replace # or $ with a fixed value
int direct_solve_equation_err_string_replace_value (char *equation, double value, double *erg, char **errstring);
int direct_solve_equation_err_string_replace_union (const char *equation, union BB_VARI value, int value_bbtype, union BB_VARI *erg, int *ret_bbtype, char **errstring);

//  This will replace defined variable name with a fixed value
int direct_solve_equation_err_string_replace_var_value (const char *equation, const char *variable, double value, double *erg, char **errstring);
struct EXEC_STACK_ELEM *solve_equation_replace_variable (char *equation, char *name);

// This will do only a syntax check of an equation
int CheckEquSyntax (char *Equation, int CANCommandAllowed, int CheckIfVarsExists, int *pUseCANData);

// This function will give free the memory that will be allocated be all functions with parameter "errstring"
// if they have detected an error.
void FreeErrStringBuffer (char **errstring);

int calc_raw_value_for_phys_value (const char *equation, double phys_value, const char *variable_name, int type, double *ret_raw_value, double *ret_phys_value, char **errstring);

#endif
