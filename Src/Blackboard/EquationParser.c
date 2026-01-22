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


#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>      /* for is.. functions */
#include <string.h>
#include <math.h>
#include <float.h>
#include <malloc.h>
#include <stdarg.h>
#ifndef _WIN32
#include <dirent.h>
#include <dlfcn.h>
#include "Platform.h"
#endif
#include "ThrowError.h"
#include "StringMaxChar.h"
#include "ExecutionStack.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "MyMemory.h"
#include "MainValues.h"
#include "CanDataBase.h"
#ifdef REMOTE_MASTER
#include <strings.h>
#include "RemoteMasterReqToClient.h"
#include "RealtimeScheduler.h"
//#define _alloca(x) alloca(x)
//#define stricmp(s1,s2) strcasecmp(s1,s2)
//#define strnicmp(s1,s2,n) strncasecmp(s1,s2,n)
#else
#include "Files.h"
#include "Scheduler.h"
#include "EnvironmentVariables.h"
#include "Script.h"
#endif
#include "EquationParser.h"

#define UNUSED(x) (void)(x)

#ifndef MAXDOUBLE
#define MAXDOUBLE 1.7976931348623158e+308
#endif

#define EQU_PARSER_ERROR_STOP     -1
#define EQU_PARSER_ERROR_CONTINUE  1

typedef struct {
    EQU_TOKEN Token;  // This will always the value of the last call
    union OP_PARAM Value;
    char NameString[BBVARI_NAME_SIZE];
    const char *NextChar;      // Pointer to the next character to be evaluated
    struct EXEC_STACK_ELEM *ExecStack;
    char ErrorInEquationStop;
    char ErrorInEquationContinue;
    const char *Equation;                // Pointer to the equation string (necessary for error messages)
    char *SharpReplaceVariable;
    struct STACK_ENTRY SharpReplaceValue;
    const char *ReplaceVariableFixedValue;  // If EQU_PARSER_REPLACE_VARIABLE_WITH_FIXED_VALUE is set this must be also set.
    char **ErrorString;
    int ErrStringLength;
    int UseCanOrFlexrayDataFlag;
    int Pid;   // if EQU_PARSER_ADD_BB_VARI_FOR_PROCESS is set this must be also set
    int StackPos;
    int Flags;
#define EQU_PARSER_READ_WRITE_FROM_NOT_EXITING_VARS_STOP 0x10000
#define EQU_PARSER_DONT_READ_FROM_NOT_EXITING_VARS        0x8000
#define EQU_PARSER_REPLACE_VARIABLE_WITH_FIXED_VALUE      0x4000
#define EQU_PARSER_ADD_BB_VARI_FOR_PROCESS                0x2000
#define EQU_PARSER_FLEXRAY_COMMANDS_ALLOWED               0x1000
#define EQU_PARSER_REMOVE_VARS_AFTER_WRITING               0x800
#define EQU_PARSER_SCRIPT_LOCAL_VARIABLES                  0x400
#define EQU_PARSER_SYNTAX_CHECK_ONLY_VARIABLES_EXIST       0x200
#define EQU_PARSER_SYNTAX_CHECK_ONLY                       0x100
#define EQU_PARSER_CAN_COMMANDS_ALLOWED                     0x80
#define EQU_PARSER_SHARP_REPLACE_PARAMETER                  0x40
#define EQU_PARSER_SHARP_REPLACE_VALUE                      0x20
#define EQU_PARSER_SHARP_REPLACE_VARIABLE                   0x10
#define EQU_PARSER_DONT_ADD_NOT_EXITING_VARS                 0x8
#define EQU_PARSER_NO_ERR_MSG                                0x4
#define EQU_PARSER_TRANSLATE_TO_BYTECODE                     0x2
#define EQU_PARSER_SOLVE_DIRECT                              0x1

#define READ_WRITE_FROM_NOT_EXITING_VARS_STOP(p) ((p->Flags & EQU_PARSER_READ_WRITE_FROM_NOT_EXITING_VARS_STOP) ==  EQU_PARSER_READ_WRITE_FROM_NOT_EXITING_VARS_STOP)
#define DONT_READ_FROM_NOT_EXITING_VARS(p) ((p->Flags & EQU_PARSER_DONT_READ_FROM_NOT_EXITING_VARS) ==  EQU_PARSER_DONT_READ_FROM_NOT_EXITING_VARS)
#define REPLACE_VARIABLE_WITH_FIXED_VALUE(p) ((p->Flags & EQU_PARSER_REPLACE_VARIABLE_WITH_FIXED_VALUE) ==  EQU_PARSER_REPLACE_VARIABLE_WITH_FIXED_VALUE)
#define ADD_BB_VARI_FOR_PROCESS(p) ((p->Flags & EQU_PARSER_ADD_BB_VARI_FOR_PROCESS) ==  EQU_PARSER_ADD_BB_VARI_FOR_PROCESS)
#define REMOVE_VARS_AFTER_WRITING(p) ((p->Flags & EQU_PARSER_REMOVE_VARS_AFTER_WRITING) ==  EQU_PARSER_REMOVE_VARS_AFTER_WRITING)
#define CAN_COMMANDS_ALLOWED(p) ((p->Flags & EQU_PARSER_CAN_COMMANDS_ALLOWED) ==  EQU_PARSER_CAN_COMMANDS_ALLOWED)
#define FLEXRAY_COMMANDS_ALLOWED(p) ((p->Flags & EQU_PARSER_FLEXRAY_COMMANDS_ALLOWED) ==  EQU_PARSER_FLEXRAY_COMMANDS_ALLOWED)
#define SHARP_REPLACE_PARAMETER(p) ((p->Flags & EQU_PARSER_SHARP_REPLACE_PARAMETER) ==  EQU_PARSER_SHARP_REPLACE_PARAMETER)
#define SHARP_REPLACE_VALUE(p) ((p->Flags & EQU_PARSER_SHARP_REPLACE_VALUE) ==  EQU_PARSER_SHARP_REPLACE_VALUE)
#define SHARP_REPLACE_VARIABLE(p) ((p->Flags & EQU_PARSER_SHARP_REPLACE_VARIABLE) ==  EQU_PARSER_SHARP_REPLACE_VARIABLE)
#define DONT_ADD_NOT_EXITING_VARS(p) ((p->Flags & EQU_PARSER_DONT_ADD_NOT_EXITING_VARS) ==  EQU_PARSER_DONT_ADD_NOT_EXITING_VARS)
#define NO_ERR_MSG(p) ((p->Flags & EQU_PARSER_NO_ERR_MSG) ==  EQU_PARSER_NO_ERR_MSG)
#define TRANSLATE_BYTE_CODE(p) ((p->Flags & EQU_PARSER_TRANSLATE_TO_BYTECODE) ==  EQU_PARSER_TRANSLATE_TO_BYTECODE)
#define SOLVE_DIRECT(p) ((p->Flags & EQU_PARSER_SOLVE_DIRECT) ==  EQU_PARSER_SOLVE_DIRECT)
#define SYNTAX_CHECK_ONLY(p) ((p->Flags & EQU_PARSER_SYNTAX_CHECK_ONLY) ==  EQU_PARSER_SYNTAX_CHECK_ONLY)
#define SCRIPT_LOCAL_VARIS_ALLOWED(p) ((p->Flags & EQU_PARSER_SCRIPT_LOCAL_VARIABLES) ==  EQU_PARSER_SCRIPT_LOCAL_VARIABLES)

   int ParamCount;
   int cs;  // If 0 than the thread has already locked BlackboardCriticalSection

   // If the equation will be calculated immediately this will be hold the execution stack
   EXECUTION_STACK Stack;

} EQUATION_PARSER_INFOS;


typedef const char* (*UserDefinedBuildinFunctionGetNameType) (int *ret_Version, int *ret_MaxParameters, int *ret_MinParameters, int *ret_Flags);
struct {
    char *FunctionName;
    int MaxParameters;
    int MinParameters;
    int Flags;
} *UserDefinedBuildinFunctionTable;
int UserDefinedBuildinFunctionTableSize;


static int or_logic (EQUATION_PARSER_INFOS *pParser);


static void EquationError (EQUATION_PARSER_INFOS *pParser, int Level, char *Format, ...)
{
   va_list args;
   int Len;
   char *Buffer;

   if (!NO_ERR_MSG (pParser)) {
       if (pParser->ErrorString != NULL) {
           va_start (args, Format);
#ifdef _WIN32
           Len = _vscprintf (Format, args) + 1;
#else
           Len = vsnprintf (NULL, 0, Format, args) + 1;
#endif
           va_end(args);
           if (pParser->ErrStringLength == 0) {
               // new error
               *pParser->ErrorString = my_malloc(Len);
               va_start (args, Format);
               vsnprintf (*pParser->ErrorString, Len, Format, args);
               va_end(args);
               pParser->ErrStringLength = Len;
           } else {
               // add error
               *pParser->ErrorString = my_realloc(*pParser->ErrorString, pParser->ErrStringLength + Len);
               (*pParser->ErrorString)[pParser->ErrStringLength - 1] = '\n';
               va_start (args, Format);
               vsnprintf (*pParser->ErrorString + pParser->ErrStringLength, Len, Format, args);
               va_end(args);
               pParser->ErrStringLength += Len;
           }
       } else {
           va_start (args, Format);
#ifdef _WIN32
           Len = _vscprintf (Format, args) + 1;
#else
           Len = vsnprintf (NULL, 0, Format, args) + 1;
#endif
           va_end(args);
           Buffer = _alloca ((size_t)Len);
           va_start (args, Format);
           vsnprintf (Buffer, Len, Format, args);
           va_end(args);
           ThrowError (1, "%s", Buffer);
       }
   }
   if (Level < 0) {
       pParser->ErrorInEquationStop++;
       pParser->Token = EQU_OP_END;
   } else if (Level > 0) {
       pParser->ErrorInEquationContinue++;
   }
}

static void CheckAddNotExistingVars(EQUATION_PARSER_INFOS *Parser, int AddNotExistingVars)
{


   switch(AddNotExistingVars) {
   case 0:   // ADD_BBVARI_AUTOMATIC_OFF
       Parser->Flags |= EQU_PARSER_DONT_ADD_NOT_EXITING_VARS; // if variable on the right side of = don't exist do not add.
       break;
   case 3:   // ADD_BBVARI_AUTOMATIC_STOP
       Parser->Flags |= EQU_PARSER_DONT_ADD_NOT_EXITING_VARS | // if variable on the right side of = don't exist do not add.
                        EQU_PARSER_READ_WRITE_FROM_NOT_EXITING_VARS_STOP;
       break;
   case 1:   // ADD_BBVARI_AUTOMATIC_ON
       break;
   case 2:   // ADD_BBVARI_AUTOMATIC_REMOVE
       Parser->Flags |= EQU_PARSER_REMOVE_VARS_AFTER_WRITING;
       break;
   case 4:   // ADD_BBVARI_AUTOMATIC_OFF_EX
       Parser->Flags |= EQU_PARSER_DONT_READ_FROM_NOT_EXITING_VARS |
                        EQU_PARSER_DONT_ADD_NOT_EXITING_VARS; //  if variable on the right and left side of = don't exist do not add.
       break;
   case 5:   // ADD_BBVARI_AUTOMATIC_STOP_EX
       Parser->Flags |= EQU_PARSER_DONT_READ_FROM_NOT_EXITING_VARS |
                        EQU_PARSER_DONT_ADD_NOT_EXITING_VARS | //  if variable on the right and left side of = don't exist do not add.
                        EQU_PARSER_READ_WRITE_FROM_NOT_EXITING_VARS_STOP;
       break;
   }
}


#ifndef REMOTE_MASTER
static double ReadValueOfLocalVariableRefTo (EQUATION_PARSER_INFOS *pParser, char *RefName, int Cmd)
{
    double value;

    if (SYNTAX_CHECK_ONLY (pParser)) return 0.0;

    if (GetValueOfRefToLocalVariable (RefName, &value, -1, Cmd)) {
        EquationError (pParser, EQU_PARSER_ERROR_STOP, "no reference with the name \"%s\" defined in equation \"%s\"", RefName, pParser->Equation);
        return 0.0;
    }
    return value;
}

static double WriteValueOfLocalVariableRefTo (EQUATION_PARSER_INFOS *pParser, char *RefName, double value, int Cmd)
{
    if (SYNTAX_CHECK_ONLY (pParser)) return 0.0;

    if (SetValueOfLocalVariableRefTo (RefName, value, Cmd)) {
        EquationError (pParser, EQU_PARSER_ERROR_STOP, "no reference with the name \"%s\" defined in equation \"%s\"", RefName, pParser->Equation);
        return 0.0;
    }

    return value;
}
#endif

static int CheckParamterCount (EQUATION_PARSER_INFOS *pParser, char *FuncName, int soll, int ist)
{
    int ret;

    if (soll != ist) {
        EquationError (pParser, EQU_PARSER_ERROR_STOP, "too few or less parameters in function '%s' in equation \"%s\"", FuncName, pParser->Equation);
        ret = -1;
    } else ret = 0;
    return ret;
}

static int ParseUserDefinedBuildinFunctions(EQUATION_PARSER_INFOS *pParser, char *par_FuncName, int par_NrArgs, int *ret,
                                            EQU_TOKEN *ret_Token, int *ret_UseCanOrFlexrayDataFlag)
{
    int x;
    for (x = 0; x < UserDefinedBuildinFunctionTableSize; x++) {
        if (!strcmp(UserDefinedBuildinFunctionTable[x].FunctionName, par_FuncName)) {
            *ret_Token = x + USER_DEFINED_BUILDIN_FUNCTION_OFFSET;
            *ret = CheckParamterCount (pParser, par_FuncName, UserDefinedBuildinFunctionTable[x].MaxParameters, par_NrArgs);
            *ret_UseCanOrFlexrayDataFlag = UserDefinedBuildinFunctionTable[x].Flags;
            return 0;
        }
    }
    return -1;
}

static int CheckIfCANCommandsAllowed (EQUATION_PARSER_INFOS *pParser, char *FuncName)
{
    if (!((TRANSLATE_BYTE_CODE (pParser) || SYNTAX_CHECK_ONLY (pParser)) && CAN_COMMANDS_ALLOWED (pParser))) {
        pParser->Token = EQU_OP_END;
        EquationError (pParser, EQU_PARSER_ERROR_STOP, "buildin function '%s' in equation \"%s\" only allowed in CAN configuration", FuncName, pParser->Equation);
        return -1;  // no
    }
    return 0;  // yes
}

static int EquBuildInFunctions (EQUATION_PARSER_INFOS *pParser, char *FuncName, int NrArgs)
{
    int ret;
    if (!strcmp (FuncName, "sin")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_SINUS;
    } else if (!strcmp (FuncName, "cos")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_COSINUS;
    } else if (!strcmp (FuncName, "tan")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_TANGENS;
    } else if (!strcmp (FuncName, "sinh")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_SINUS_HYP;
    } else if (!strcmp (FuncName, "cosh")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_COSINUS_HYP;
    } else if (!strcmp (FuncName, "tanh")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_TANGENS_HYP;
    } else if (!strcmp (FuncName, "asin")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_ARC_SINUS;
    } else if (!strcmp (FuncName, "acos")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_ARC_COSINUS;
    } else if (!strcmp (FuncName, "atan")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_ARC_TANGENS;
    } else if (!strcmp (FuncName, "exp")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_EXP;
    } else if (!strcmp (FuncName, "pow")) {
        ret = CheckParamterCount (pParser, FuncName, 2, NrArgs);
        pParser->Token = EQU_OP_POW;
    } else if (!strcmp (FuncName, "sqrt")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_SQRT;
    } else if (!strcmp (FuncName, "log")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_LOG;
    } else if (!strcmp (FuncName, "log10")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_LOG10;
    } else if (!strcmp (FuncName, "getbits")) {
        ret = CheckParamterCount (pParser, FuncName, 3, NrArgs);
        pParser->Token = EQU_OP_GETBITS;
    } else if (!strcmp (FuncName, "setbits")) {
        ret = CheckParamterCount (pParser, FuncName, 4, NrArgs);
        pParser->Token = EQU_OP_SETBITS;
    } else if (!strcmp (FuncName, "andbits")) {
        ret = CheckParamterCount (pParser, FuncName, 4, NrArgs);
        pParser->Token = EQU_OP_ANDBITS;
    } else if (!strcmp (FuncName, "orbits")) {
        ret = CheckParamterCount (pParser, FuncName, 4, NrArgs);
        pParser->Token = EQU_OP_ORBITS;
    } else if (!strcmp (FuncName, "xorbits")) {
        ret = CheckParamterCount (pParser, FuncName, 4, NrArgs);
        pParser->Token = EQU_OP_XORBITS;
    } else if (!strcmp (FuncName, "and")) {
        ret = CheckParamterCount (pParser, FuncName, 2, NrArgs);
        pParser->Token = EQU_OP_BITWISE_AND;
    } else if (!strcmp (FuncName, "or")) {
        ret = CheckParamterCount (pParser, FuncName, 2, NrArgs);
        pParser->Token = EQU_OP_BITWISE_OR;
    } else if (!strcmp (FuncName, "xor")) {
        ret = CheckParamterCount (pParser, FuncName, 2, NrArgs);
        pParser->Token = EQU_OP_BITWISE_XOR;
    } else if (!strcmp (FuncName, "invert")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_BITWISE_INVERT;
    } else if (!strcmp (FuncName, "canbyte")) {
        ret = CheckIfCANCommandsAllowed (pParser, FuncName);
        ret = ret || CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_CANBYTE;
        pParser->UseCanOrFlexrayDataFlag = 1;
    } else if (!strcmp (FuncName, "canword")) {
        ret = CheckIfCANCommandsAllowed (pParser, FuncName);
        ret = ret || CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_CANWORD;
        pParser->UseCanOrFlexrayDataFlag = 1;
    } else if (!strcmp (FuncName, "candword")) {
        ret = CheckIfCANCommandsAllowed (pParser, FuncName);
        ret = ret || CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_CANDWORD;
        pParser->UseCanOrFlexrayDataFlag = 1;
    } else if (!strcmp (FuncName, "can_cyclic")) {
        ret = CheckIfCANCommandsAllowed (pParser, FuncName);
        ret = ret || CheckParamterCount (pParser, FuncName, 0, NrArgs);
        pParser->Token = EQU_OP_CAN_CYCLIC;
    } else if (!strcmp (FuncName, "can_data_changed")) {
        ret = CheckIfCANCommandsAllowed (pParser, FuncName);
        ret = ret || CheckParamterCount (pParser, FuncName, 2, NrArgs);
        pParser->Token = EQU_OP_CAN_DATA_CHANGED;
        pParser->UseCanOrFlexrayDataFlag = 1;
    } else if (!strcmp (FuncName, "abs")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_ABS;
    } else if (!strcmp (FuncName, "equal")) {
        pParser->Token = EQU_OP_EQUAL_DECIMALS;
        ret = 0;
        if (NrArgs == 2) {  // If there is no Number of decimal places defined assume 0
            NrArgs = 3;
            pParser->Value.value = 0.0;
            if (TRANSLATE_BYTE_CODE (pParser)) {
                ret = add_exec_stack_elem (EQU_OP_NUMBER, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid);
            } else {
                ret = direct_execute_one_command (EQU_OP_NUMBER, &(pParser->Stack), pParser->Value);
            }
        }
        ret = ret || CheckParamterCount (pParser, FuncName, 3, NrArgs);
    } else if (!strcmp (FuncName, "round")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_ROUND;
    } else if (!strcmp (FuncName, "round_up")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_ROUND_UP;
    } else if (!strcmp (FuncName, "round_down")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_ROUND_DOWN;
    } else if (!strcmp (FuncName, "modulo")) {
        ret = CheckParamterCount (pParser, FuncName, 2, NrArgs);
        pParser->Token = EQU_OP_MODULO;
    } else if (!strcmp (FuncName, "add_msb_lsb")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_ADD_MSB_LSB;
    } else if (!strcmp (FuncName, "overflow")) {
        ret = CheckParamterCount (pParser, FuncName, 3, NrArgs);
        pParser->Token = EQU_OP_OVERFLOW;
    } else if (!strcmp (FuncName, "underflow")) {
        ret = CheckParamterCount (pParser, FuncName, 3, NrArgs);
        pParser->Token = EQU_OP_UNDERFLOW;
    } else if (!strcmp (FuncName, "min")) {
        ret = CheckParamterCount (pParser, FuncName, 2, NrArgs);
        pParser->Token = EQU_OP_MIN;
    } else if (!strcmp (FuncName, "max")) {
        ret = CheckParamterCount (pParser, FuncName, 2, NrArgs);
        pParser->Token = EQU_OP_MAX;
    } else if (!strcmp (FuncName, "swap16")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_SWAP16;
    } else if (!strcmp (FuncName, "swap32")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_SWAP32;
    } else if (!strcmp (FuncName, "shift_left")) {
        ret = CheckParamterCount (pParser, FuncName, 2, NrArgs);
        pParser->Token = EQU_OP_SHIFT_LEFT;
    } else if (!strcmp (FuncName, "shift_right")) {
        ret = CheckParamterCount (pParser, FuncName, 2, NrArgs);
        pParser->Token = EQU_OP_SHIFT_RIGHT;
    } else if (!strcmp (FuncName, "crc8_user_poly")) {
        pParser->ParamCount = NrArgs; 
        pParser->Token = EQU_OP_CRC8_USER_POLY;
        ret = 0;
    } else if (!strcmp (FuncName, "crc16_user_poly")) {
        pParser->ParamCount = NrArgs;
        pParser->Token = EQU_OP_CRC16_USER_POLY;
        ret = 0;
    } else if (!strcmp (FuncName, "crc8_user_poly_reflect")) {
        pParser->ParamCount = NrArgs;
        pParser->Token = EQU_OP_CRC8_USER_POLY_REFLECT;
        ret = 0;
    } else if (!strcmp (FuncName, "crc16_user_poly_reflect")) {
        pParser->ParamCount = NrArgs;
        pParser->Token = EQU_OP_CRC16_USER_POLY_REFLECT;
        ret = 0;
    } else if (!strcmp (FuncName, "crc32_user_poly_reflect")) {
        pParser->ParamCount = NrArgs;
        pParser->Token = EQU_OP_CRC32_USER_POLY_REFLECT;
        ret = 0;
    } else if (!strcmp (FuncName, "has_changed")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        if (!TRANSLATE_BYTE_CODE (pParser)) {
            EquationError (pParser, EQU_PARSER_ERROR_STOP, "function has_changed() not allowed in this context in equation \"%s\"", pParser->Equation);
            pParser->Token = EQU_OP_END;
            ret = -1;
        } else {
            pParser->Token = EQU_OP_HAS_CHANGED;
        }
    } else if (!strcmp (FuncName, "slope_up")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        if (!TRANSLATE_BYTE_CODE (pParser)) {
            EquationError (pParser, EQU_PARSER_ERROR_STOP, "function slope_up() not allowed in this context in equation \"%s\"", pParser->Equation);
            pParser->Token = EQU_OP_END;
            ret = -1;
        } else {
            pParser->Token = EQU_OP_SLOPE_UP;
        }
    } else if (!strcmp (FuncName, "slope_down")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        if (!TRANSLATE_BYTE_CODE (pParser)) {
            EquationError (pParser, EQU_PARSER_ERROR_STOP, "function slope_down() not allowed in this context in equation \"%s\"", pParser->Equation);
            pParser->Token = EQU_OP_END;
            ret = -1;
        } else {
            pParser->Token = EQU_OP_SLOPE_DOWN;
        }
    } else if (!strcmp (FuncName, "can_id")) {
        ret = CheckIfCANCommandsAllowed (pParser, FuncName);
        ret = ret || CheckParamterCount (pParser, FuncName, 0, NrArgs);
        pParser->Token = EQU_OP_CAN_ID;
        pParser->UseCanOrFlexrayDataFlag = 1;
    } else if (!strcmp (FuncName, "can_cycles")) {
        ret = CheckIfCANCommandsAllowed (pParser, FuncName);
        ret = ret || CheckParamterCount (pParser, FuncName, 0, NrArgs);
        pParser->Token = EQU_OP_CAN_CYCLES;
        pParser->UseCanOrFlexrayDataFlag = 1;
    } else if (!strcmp (FuncName, "can_size")) {
        ret = CheckIfCANCommandsAllowed (pParser, FuncName);
        ret = ret || CheckParamterCount (pParser, FuncName, 0, NrArgs);
        pParser->Token = EQU_OP_CAN_SIZE;
        pParser->UseCanOrFlexrayDataFlag = 1;
    } else if (!strcmp (FuncName, "add_msn_lsn")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_ADD_MSN_LSN;
    } else if (!strcmp (FuncName, "can_crc8_rev_in_out")) {
        ret = CheckIfCANCommandsAllowed (pParser, FuncName);
        ret = ret || CheckParamterCount (pParser, FuncName, 4, NrArgs);
        pParser->Token = EQU_OP_CAN_CRC8_REV_IN_OUT;
        pParser->UseCanOrFlexrayDataFlag = 1;
    } else if (!strcmp (FuncName, "double")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_TO_DOUBLE;
    } else if (!strcmp (FuncName, "int8")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_TO_INT8;
    } else if (!strcmp (FuncName, "int16")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_TO_INT16;
    } else if (!strcmp (FuncName, "int32")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_TO_INT32;
    } else if (!strcmp (FuncName, "int64")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_TO_INT64;
    } else if (!strcmp (FuncName, "uint8")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_TO_UINT8;
    } else if (!strcmp (FuncName, "uint16")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_TO_UINT16;
    } else if (!strcmp (FuncName, "uint32")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_TO_UINT32;
    } else if (!strcmp (FuncName, "uint64")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_TO_UINT64;
    } else if (!strcmp (FuncName, "get_calc_data_type")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_GET_CALC_DATA_TYPE;
    } else if (!strcmp (FuncName, "get_calc_data_width")) {
        ret = CheckParamterCount (pParser, FuncName, 1, NrArgs);
        pParser->Token = EQU_OP_GET_CALC_DATA_WIDTH;
    } else if (!ParseUserDefinedBuildinFunctions(pParser, FuncName, NrArgs, &ret, &pParser->Token, &pParser->UseCanOrFlexrayDataFlag)) {
        // are there any user defined buildin functions?
    } else {
        pParser->Token = EQU_OP_END;
        EquationError (pParser, EQU_PARSER_ERROR_STOP, "unknown buildin function '%s' in equation \"%s\"", FuncName, pParser->Equation);
        ret = -1;
    }
    return ret;
}

#ifndef REMOTE_MASTER
static int CheckFileExist(const char *par_Filename)
{
    char Filename[1024];
    SearchAndReplaceEnvironmentStrings(par_Filename, Filename, sizeof(Filename));
    if (access(Filename, 0) == 0) {
        return 1;
    } else {
        return 0;
    }
}
#endif

static int FindDelimiterOfParameter(EQUATION_PARSER_INFOS *pParser,
                                    const char *par_FunctionName,
                                    const char **ret_ParStart, const char **ret_ParEnd)
{
    const char *p = pParser->NextChar + 1;
    const char *ParEnd;
    const char *ParStart;
    int RecursiveCounter = 0;

    while (isascii (*p) && isspace (*p)) p++; // ignore whitespaces behind "("
    ParStart = p;
    while ((*p != ')') || (RecursiveCounter > 0)) {
        if (*p == '\0') {
            EquationError (pParser, EQU_PARSER_ERROR_STOP, "expect a ')' in %s() inside equation \"%s\"", par_FunctionName, pParser->Equation);
            return -1;
        } else if (*p == '(') {
            RecursiveCounter++;
        } else if (*p == ')') {
            RecursiveCounter--;
        }
        p++;
    }
    ParEnd = p;
    pParser->NextChar = p + 1;
    while (isascii (*(ParEnd-1)) && isspace (*(ParEnd-1))) ParEnd--; // ignore whitespaces before ")"
    *ret_ParStart = ParStart;
    *ret_ParEnd = ParEnd;
    return (int)(ParEnd - ParStart); // return the number of characters
}

static char *GetNextTrimmedParameter(EQUATION_PARSER_INFOS *pParser, const char *par_FunctionName,
                                     char *par_ParameterListString, char **ret_Parameter)
{
    char *ep;
    char *p = par_ParameterListString;
    int RecursiveCounter = 0;
    if (p != NULL)  {
        while (isascii (*p) && isspace (*p)) p++; // ignore whitespaces before
        *ret_Parameter = p;
        while (((*p != ',') && (*p != 0)) || (RecursiveCounter > 0)) {
            if (*p == '(') {
                RecursiveCounter++;
            } else if (*p == ')') {
                RecursiveCounter--;
            } if (*p == 0) {
                EquationError (pParser, EQU_PARSER_ERROR_STOP, "expect a ')' in %s() inside equation \"%s\"", par_FunctionName, pParser->Equation);
                return NULL;
            }
            p++;
        }
        ep = p;
        if (*p == ',') p++;
        while ((ep > p) && isascii (*(ep-1)) && isspace (*(ep-1))) ep--; // ignore whitespaces behind
        *ep = 0;
    }
    return p;
}

static int CheckIfFunctionsIsAllowed(EQUATION_PARSER_INFOS *pParser, const char *par_FuntionName)
{
    if (TRANSLATE_BYTE_CODE (pParser)) {
        EquationError (pParser, EQU_PARSER_ERROR_STOP, "function %s() not allowed in this context in equation \"%s\"", par_FuntionName, pParser->Equation);
        pParser->Token = EQU_OP_END;
        return 0;
    } else {
        return 1;
    }
}

static int ReadToken (EQUATION_PARSER_INFOS *pParser);

static int BuildinFunctions(EQUATION_PARSER_INFOS *pParser)
{
    int i;
    char FuncName[32];
    char *p;
    const char *ParEnd;
    const char *ParStart;
    int Size;

    StringCopyMaxCharTruncate (FuncName, pParser->NameString, sizeof (FuncName));
#ifdef REMOTE_MASTER
    if(0) {
        ;
#else
    if (!strcmp (FuncName, "get_process_state")) {
        if (CheckIfFunctionsIsAllowed(pParser, FuncName)) {
            Size = FindDelimiterOfParameter(pParser, FuncName, &ParStart, &ParEnd);
            if (Size >= 0) {
                char *Parameter = alloca(Size+1);
                StringCopyMaxCharTruncate(Parameter, ParStart, Size+1);
                switch (get_process_state (get_pid_by_name (RenameProcessByBasicSettingsTable(Parameter)))) {
                case EX_PROCESS_REFERENCE:
                    pParser->Value.value = 1;
                    break;
                case EX_PROCESS_INIT:
                    pParser->Value.value = 2;
                    break;
                case PROCESS_AKTIV:
                case PROCESS_SLEEP:
                case PROCESS_NONSLEEP:
                    pParser->Value.value = 3;
                    break;
                case EX_PROCESS_TERMINATE:
                    pParser->Value.value = 4;
                    break;
                default:
                    pParser->Value.value = 0;
                }
                return pParser->Token = EQU_OP_NUMBER;
            } else {
                return EQU_OP_END;
            }
        } else {
            return EQU_OP_END;
        }
    } else if (!strcmp (FuncName, "exist")) {
        if (CheckIfFunctionsIsAllowed(pParser, FuncName)) {
            Size = FindDelimiterOfParameter(pParser, FuncName, &ParStart, &ParEnd);
            if (Size >= 0) {
                char *Parameter = alloca(Size+1);
                StringCopyMaxCharTruncate(Parameter, ParStart, Size+1);
                if (get_bbvarivid_by_name (Parameter) > 0) {
                    if (get_bbvaritype_by_name (Parameter) < BB_UNKNOWN) {
                        pParser->Value.value = 1;
                    } else {
                        pParser->Value.value = 0;
                    }
                } else {
                    pParser->Value.value = 0;
                }
                return pParser->Token = EQU_OP_NUMBER;
            } else {
                return EQU_OP_END;
            }
        } else {
            return EQU_OP_END;
        }
    } else if (!strcmp (FuncName, "env_exist")) {
        if (CheckIfFunctionsIsAllowed(pParser, FuncName)) {
            Size = FindDelimiterOfParameter(pParser, FuncName, &ParStart, &ParEnd);
            if (Size >= 0) {
                char *Parameter = alloca(Size+1);
                StringCopyMaxCharTruncate(Parameter, ParStart, Size+1);
                switch (CheckIfEnvironmentVariableExist (Parameter)) {
                case 1:
                    pParser->Value.value = 1;
                    break;
                case 0:
                    pParser->Value.value = 0;
                    break;
                case -1:
                    EquationError (pParser, EQU_PARSER_ERROR_STOP, "environment variable too long in env_exist() in equation \"%s\"", pParser->Equation);
                    pParser->Value.value = 0;
                    break;
                case -2:
                    EquationError (pParser, EQU_PARSER_ERROR_STOP, "environment variable cannot include '%%' char in env_exist() in equation \"%s\"", pParser->Equation);
                    pParser->Value.value = 0;
                    break;
                default:
                    EquationError (pParser, EQU_PARSER_ERROR_STOP, "internel error in env_exist() in equation \"%s\"", pParser->Equation);
                    pParser->Value.value = 0;
                    break;
                }
                return pParser->Token = EQU_OP_NUMBER;
            } else {
                return EQU_OP_END;
            }
        } else {
            return EQU_OP_END;
        }
    } else if (!strcmp (FuncName, "file_exist")) {
        if (CheckIfFunctionsIsAllowed(pParser, FuncName)) {
            Size = FindDelimiterOfParameter(pParser, FuncName, &ParStart, &ParEnd);
            if (Size >= 0) {
                char *Parameter = alloca(Size+1);
                StringCopyMaxCharTruncate(Parameter, ParStart, Size+1);
                pParser->Value.value = CheckFileExist(Parameter);
                return pParser->Token = EQU_OP_NUMBER;
            } else {
                return EQU_OP_END;
            }
        } else {
            return EQU_OP_END;
        }
#endif
    } else if (!strcmp (FuncName, "enum")) {
        Size = FindDelimiterOfParameter(pParser, FuncName, &ParStart, &ParEnd);
        if (Size >= 0) {
            int Vid;
            int64_t from, to;
            char *p;
            char *Parameter = alloca(Size+1);
            StringCopyMaxCharTruncate(Parameter, ParStart, Size+1);
            p = Parameter;

            // enum(blackboardname.ENUM-Text) or enum(blackboardname@ENUM-Text)
            // search from the end because . characters can also be included inside a variable name
            while (*p != 0) p++;
            if (p > Parameter) p--;
            while ((*p != '.') && (*p != '@')) {
                if (p <= Parameter) {
                    EquationError (pParser, EQU_PARSER_ERROR_STOP, "Missing separator ('.' or '@') in enum() inside equation \"%s\"", pParser->Equation);
                    return pParser->Token = EQU_OP_END;
                }
                p--;
            }
            *p = 0;
            p++;
            Vid = add_bbvari (Parameter, BB_UNKNOWN, "");
            if (Vid <= 0) {
                EquationError (pParser, EQU_PARSER_ERROR_STOP, "variable \"%s\" doesn't exist in enum() in equation \"%s\"", Parameter, pParser->Equation);
                return pParser->Token = EQU_OP_END;
            }
            if (convert_textreplace_value (Vid, p, &from, &to)) {
                remove_bbvari (Vid);
                EquationError (pParser, EQU_PARSER_ERROR_STOP, "variable \"%s\" doesn't have an enum \"%s\" in enum() in equation \"%s\"", Parameter, p, pParser->Equation);
                return pParser->Token = EQU_OP_END;
            }
            remove_bbvari (Vid);
            pParser->Value.value = (double)from;
            return pParser->Token = EQU_OP_NUMBER;
        } else {
            return EQU_OP_END;
        }
    } else if (!strcmp (FuncName, "phys")) {
        int IsRef = 0;
        Size = FindDelimiterOfParameter(pParser, FuncName, &ParStart, &ParEnd);
        if (Size >= 0) {
            char *Dst, *Src;
            char *Parameter = alloca(Size+1);
            StringCopyMaxCharTruncate(Parameter, ParStart, Size+1);
            Src = Parameter;
            if (*Src == '*') {  // seems to be a reference
                IsRef = 1;
                Src++;
                while (isascii (*Src) && isspace (*Src)) Src++;
            }
            Dst = pParser->NameString;
            while (isalnum (*Src) || (*Src== '_') ||
                   (*Src == '[') || (*Src == ']') ||
                   (*Src == '.') || (*Src == ':') ||
                   (*Src == '{') || (*Src == '}') ||
                   (*Src== '@')) {
                if ((Dst - pParser->NameString) >= ((int)sizeof (pParser->NameString) - 1)) {
                    EquationError (pParser, EQU_PARSER_ERROR_STOP, "label size exeeds %i chars in phys() in equation \"%s\"", sizeof (pParser->NameString) - 1, pParser->Equation);
                    return pParser->Token = EQU_OP_END;
                }
                *Dst++ = *Src++;
            }
            *Dst = 0;
            if (IsRef) {
                return pParser->Token = EQU_OP_NAME_PHYS_REF;
            } else {
                return pParser->Token = EQU_OP_NAME_PHYS;
            }
        } else {
            return EQU_OP_END;
        }
    } else if (!strcmp (FuncName, "strcmp") ||
               !strcmp (FuncName, "stricmp") ||
               !strcmp (FuncName, "strncmp") ||
               !strcmp (FuncName, "strnicmp")) {
        if (CheckIfFunctionsIsAllowed(pParser, FuncName)) {
            Size = FindDelimiterOfParameter(pParser, FuncName, &ParStart, &ParEnd);
            if (Size >= 0) {
                char *String1, *String2, *Parameter3;
                char *Parameter = alloca(Size+1);
                StringCopyMaxCharTruncate(Parameter, ParStart, Size+1);
                p = GetNextTrimmedParameter(pParser, FuncName, Parameter, &String1);
                p = GetNextTrimmedParameter(pParser, FuncName, p, &String2);
                if (!strcmp (FuncName, "strncmp") ||
                    !strcmp (FuncName, "strnicmp")) {
                     p = GetNextTrimmedParameter(pParser, FuncName, p, &Parameter3);
                } else {
                    Parameter3 = NULL;
                }
                if (p == NULL) {
                    EquationError (pParser, EQU_PARSER_ERROR_STOP, "Missing parameter in %s() inside equation \"%s\"", FuncName, pParser->Equation);
                    return pParser->Token = EQU_OP_END;
                }
                if (!strcmp (FuncName, "strcmp")) pParser->Value.value = strcmp (String1, String2);
                else if (!strcmp (FuncName, "stricmp")) pParser->Value.value = stricmp (String1, String2);
                else if (!strcmp (FuncName, "strncmp") && (Parameter3 != NULL)) {
                    const char *SaveEquation;
                    const char *SaveNextChar;
                    double LenD;
                    SaveEquation = pParser->Equation;
                    SaveNextChar = pParser->NextChar;
                    pParser->Equation = pParser->NextChar = Parameter3;
                    ReadToken (pParser);
                    or_logic (pParser);
                    pParser->Equation = SaveEquation;
                    pParser->NextChar = SaveNextChar;
                    LenD = get_current_stack_value(&(pParser->Stack));
                    pParser->Value.value = strncmp (String1, String2, (size_t)LenD);
                } else if (!strcmp (FuncName, "strnicmp")) {
                    const char *SaveEquation;
                    const char *SaveNextChar;
                    double LenD;
                    SaveEquation = pParser->Equation;
                    SaveNextChar = pParser->NextChar;
                    pParser->Equation = pParser->NextChar = Parameter3;
                    ReadToken (pParser);
                    or_logic (pParser);
                    pParser->Equation = SaveEquation;
                    pParser->NextChar = SaveNextChar;
                    LenD = get_current_stack_value(&(pParser->Stack));
                    pParser->Value.value = strnicmp (String1, String2, (size_t)LenD);
                }
                return pParser->Token = EQU_OP_NUMBER;
            } else {
                return EQU_OP_END;
            }
        } else {
            return EQU_OP_END;
        }
#ifndef REMOTE_MASTER
    } else if (!strcmp (FuncName, "can_transmit_cycle_rate") ||
               !strcmp (FuncName, "can_object_id")) {
        if (CheckIfFunctionsIsAllowed(pParser, FuncName)) {
            Size = FindDelimiterOfParameter(pParser, FuncName, &ParStart, &ParEnd);
            if (Size >= 0) {
                double ChannelD;
                const char *SaveEquation;
                const char *SaveNextChar;
                char *String1, *String2;
                char *Parameter = alloca(Size+1);
                StringCopyMaxCharTruncate(Parameter, ParStart, Size+1);
                p = GetNextTrimmedParameter(pParser, FuncName, Parameter, &String1);
                p = GetNextTrimmedParameter(pParser, FuncName, p, &String2);
                if (p == NULL) {
                    EquationError (pParser, EQU_PARSER_ERROR_STOP, "Missing parameter in %s() inside equation \"%s\"", FuncName, pParser->Equation);
                    return pParser->Token = EQU_OP_END;
                }
                SaveEquation = pParser->Equation;
                SaveNextChar = pParser->NextChar;
                pParser->Equation = pParser->NextChar = String1;
                ReadToken (pParser);
                or_logic (pParser);
                pParser->Equation = SaveEquation;
                pParser->NextChar = SaveNextChar;
                ChannelD = get_current_stack_value(&(pParser->Stack));
                pParser->Value.value = ScriptGetCANTransmitCycleRateOrId  ((int)ChannelD, String2, !strcmp (FuncName, "can_object_id"));
                return pParser->Token = EQU_OP_NUMBER;
            } else {
                return EQU_OP_END;
            }
        } else {
            return EQU_OP_END;
        }
#endif
    } else if (!strcmp (FuncName, "ones_complement")) {
        Size = FindDelimiterOfParameter(pParser, FuncName, &ParStart, &ParEnd);
        if (Size >= 0) {
            if (Size >= ((int)sizeof(pParser->NameString) + 1)) {
                EquationError (pParser, EQU_PARSER_ERROR_STOP, "variable name are longer than %i chars in ones_complement() in equation \"%s\"", sizeof(pParser->NameString), pParser->Equation);
                return pParser->Token = EQU_OP_END;
            }
            StringCopyMaxCharTruncate(pParser->NameString, ParStart, Size + 1);
            return pParser->Token = EQU_OP_ONES_COMPLEMENT_BBVARI;
        } else {
            return EQU_OP_END;
        }
    } else if (!strcmp (FuncName, "get_binary")) {
        Size = FindDelimiterOfParameter(pParser, FuncName, &ParStart, &ParEnd);
        if (Size >= 0) {
            if (Size >= ((int)sizeof(pParser->NameString) + 1)) {
                EquationError (pParser, EQU_PARSER_ERROR_STOP, "variable name are longer than %i chars in get_binary() in equation \"%s\"", sizeof(pParser->NameString), pParser->Equation);
                return pParser->Token = EQU_OP_END;
            }
            StringCopyMaxCharTruncate(pParser->NameString, ParStart, Size + 1);
            return pParser->Token = EQU_OP_GET_BINARY_BBVARI;
        } else {
            return EQU_OP_END;
        }
    } else if (!strcmp (FuncName, "set_binary")) {
        Size = FindDelimiterOfParameter(pParser, FuncName, &ParStart, &ParEnd);
        if (Size >= 0) {
            const char *SaveEquation;
            const char *SaveNextChar;
            char *String1, *String2;
            char *Parameter = alloca(Size+1);
            StringCopyMaxCharTruncate(Parameter, ParStart, Size+1);
            p = GetNextTrimmedParameter(pParser, FuncName, Parameter, &String1);
            p = GetNextTrimmedParameter(pParser, FuncName, p, &String2);
            if (p == NULL) {
                EquationError (pParser, EQU_PARSER_ERROR_STOP, "Missing parameter in %s() inside equation \"%s\"", FuncName, pParser->Equation);
                return pParser->Token = EQU_OP_END;
            }
            SaveEquation = pParser->Equation;
            SaveNextChar = pParser->NextChar;
            pParser->Equation = pParser->NextChar = String2;
            ReadToken (pParser);
            or_logic (pParser);
            pParser->Equation = SaveEquation;
            pParser->NextChar = SaveNextChar;
            StringCopyMaxCharTruncate (pParser->NameString, String1, sizeof(pParser->NameString));
            return pParser->Token = EQU_OP_SET_BINARY_BBVARI;
        } else {
            return EQU_OP_END;
        }
    } else {  // "normal" buildin functions
        Size = FindDelimiterOfParameter(pParser, FuncName, &ParStart, &ParEnd);
        if (Size >= 0) {
            const char *SaveEquation;
            const char *SaveNextChar;
            char *Parameter;
            char *ParameterList = alloca(Size+1);
            StringCopyMaxCharTruncate(ParameterList, ParStart, Size+1);
            p = ParameterList;
            for (i = 0; (p != NULL) && (*p != 0); i++) {
                if (i >= BUILDIN_FUNC_MAX_PARAMETER) {
                    EquationError (pParser, EQU_PARSER_ERROR_STOP, "not more than %i parameter allowd in buildin functions (equation \"%s\")", (int)BUILDIN_FUNC_MAX_PARAMETER, pParser->Equation);
                    return pParser->Token = EQU_OP_END;
                }
                p = GetNextTrimmedParameter(pParser, FuncName, p, &Parameter);
                SaveEquation = pParser->Equation;
                SaveNextChar = pParser->NextChar;
                pParser->Equation = pParser->NextChar = Parameter;
                ReadToken (pParser);         // Read first token
                or_logic (pParser);          // Argument
                pParser->Equation = SaveEquation;
                pParser->NextChar = SaveNextChar;
            }
            pParser->Value.value = EquBuildInFunctions (pParser, FuncName, i);
            return pParser->Token;
        } else {
            return EQU_OP_END;
        }
    }
}

static int ReadToken (EQUATION_PARSER_INFOS *pParser)
{
    char *p;
    char ch;

    while (isspace (*pParser->NextChar)) ++pParser->NextChar;
    ch = *pParser->NextChar;

    switch (ch) {
        case ',':  // Buildin function
             //EquationError (pParser, EQU_PARSER_ERROR_STOP, "unexpected ',' in equation \"%s\"", pParser->Equation);
        case '\n':
        case 0:
            return pParser->Token = EQU_OP_END;
        case '*':                    /* function */
            if (*(pParser->NextChar+1) == '=') {
                pParser->NextChar += 2;
                return pParser->Token = EQU_OP_MUL_EQUAL;
            } else {
                pParser->NextChar++;
                return pParser->Token = EQU_OP_MUL;
            }
        case '/':
            if (*(pParser->NextChar+1) == '=') {
                pParser->NextChar += 2;
                return pParser->Token = EQU_OP_DIV_EQUAL;
            } else {
                pParser->NextChar++;
                return pParser->Token = EQU_OP_DIV;
            }
        case '+':
            if (*(pParser->NextChar+1) == '=') {
                pParser->NextChar += 2;
                return pParser->Token = EQU_OP_PLUS_EQUAL;
            } else {
                pParser->NextChar++;
                return pParser->Token = EQU_OP_PLUS;
            }
        case '-':
            if (*(pParser->NextChar+1) == '=') {
                pParser->NextChar += 2;
                return pParser->Token = EQU_OP_MINUS_EQUAL;
            } else {
                pParser->NextChar++;
                return pParser->Token = EQU_OP_MINUS;
            }
        case '(':
            pParser->NextChar++;
            return pParser->Token = EQU_OP_OPEN_BRACKETS;
        case ')':
            pParser->NextChar++;
            return pParser->Token = EQU_OP_CLOSE_BRACKETS;
        case '=':
            if (*(pParser->NextChar+1) == '=') {
                pParser->NextChar += 2;
                return pParser->Token = EQU_OP_EQUAL;
            } else {
                pParser->NextChar++;
                return pParser->Token = EQU_OP_ASSIGNMENT;
            }
        case '!':
            if (*(pParser->NextChar+1) == '=') {
                pParser->NextChar += 2;
                return pParser->Token = EQU_OP_NOT_EQUAL;
            } else {
                pParser->NextChar++;
                return pParser->Token = EQU_OP_NOT;
            }
        case '|':
            if (*(pParser->NextChar+1) == '|') {
                pParser->NextChar += 2;
                return pParser->Token = EQU_OP_LOGICAL_OR;
            } else {
                pParser->NextChar++;
                return pParser->Token = EQU_OP_BITWISE_OR;
            }
        case '^':
            pParser->NextChar++;
            return pParser->Token = EQU_OP_BITWISE_XOR;
        case '&':
            if (*(pParser->NextChar+1) == '&') {
                pParser->NextChar += 2;
                return pParser->Token = EQU_OP_LOGICAL_AND;
            } else {
                pParser->NextChar++;
                return pParser->Token = EQU_OP_BITWISE_AND;
            }
        case '<':
            if (*(pParser->NextChar+1) == '=') {
                pParser->NextChar += 2;
                return pParser->Token = EQU_OP_SMALLER_EQUAL;
			} else if (*(pParser->NextChar+1) == '<') {
                pParser->NextChar += 2;
                return pParser->Token = EQU_OP_SHIFT_LEFT;
            } else {
                pParser->NextChar++;
                return pParser->Token = EQU_OP_SMALLER;
            }
        case '>':
            if (*(pParser->NextChar+1) == '=') {
                pParser->NextChar += 2;
                return pParser->Token = EQU_OP_LARGER_EQUAL;
			} else if (*(pParser->NextChar+1) == '>') {
                pParser->NextChar += 2;
                return pParser->Token = EQU_OP_SHIFT_RIGHT;
            } else {
                pParser->NextChar++;
                return pParser->Token = EQU_OP_LARGER;
            }
        case '0':
        case '1':                    /* Value */
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '.':
            pParser->Value.value_ui64 = strtoull(pParser->NextChar, &p, 0);
            if ((*p == '.') || (*p == 'e') || (*p == 'E')) {
                pParser->Value.value = strtod (pParser->NextChar, &p);
                pParser->Token = EQU_OP_NUMBER;
            } else {
                if ((p[0] == 'u') && (p[1] == 'i')) {
                    if (p[2] == '8') { pParser->Token = EQU_OP_NUMBER_UINT8; p += 3; }
                    else if ((p[2] == '1') && (p[3] == '6')) { pParser->Token = EQU_OP_NUMBER_UINT16; p += 4; }
                    else if ((p[2] == '3') && (p[3] == '2')) { pParser->Token = EQU_OP_NUMBER_UINT32; p += 4; }
                    else if ((p[2] == '6') && (p[3] == '4')) { pParser->Token = EQU_OP_NUMBER_UINT64; p += 4; }
                } else if (p[0] == 'i') {
                    if (p[1] == '8') { pParser->Token = EQU_OP_NUMBER_INT8; p += 2; }
                    else if ((p[1] == '1') && (p[2] == '6')) { pParser->Token = EQU_OP_NUMBER_INT16; p += 3; }
                    else if ((p[1] == '3') && (p[2] == '2')) { pParser->Token = EQU_OP_NUMBER_INT32; p += 3; }
                    else if ((p[1] == '6') && (p[2] == '4')) { pParser->Token = EQU_OP_NUMBER_INT64; p += 3; }
                } else {
                    if (pParser->Value.value_ui64 <= 0x7FFFFFFFllu) {
                        pParser->Token = EQU_OP_NUMBER_INT32;
                    } else if (pParser->Value.value_ui64 <= 0x7FFFFFFFFFFFFFFFllu) {
                        pParser->Token = EQU_OP_NUMBER_INT64;
                    } else {
                        pParser->Token = EQU_OP_NUMBER_UINT64;
                    }
                }
            }
            pParser->NextChar = p;
            return pParser->Token;
        case '#':           // Variable name was set with mit 'set_SharpReplaceVariable' before
        case '$':
            if (SHARP_REPLACE_PARAMETER (pParser)) {
                pParser->NextChar++;
                return pParser->Token = EQU_OP_GET_PARAM;
            } else if (SHARP_REPLACE_VALUE (pParser)) {
                switch (pParser->SharpReplaceValue.Type) {
                case FLOAT_OR_INT_64_TYPE_INT64:
                    pParser->Token = EQU_OP_NUMBER_INT64;
                    pParser->Value.value_i64 = pParser->SharpReplaceValue.V.qw;
                    break;
                case FLOAT_OR_INT_64_TYPE_UINT64:
                    pParser->Token = EQU_OP_NUMBER_UINT64;
                    pParser->Value.value_ui64 = pParser->SharpReplaceValue.V.uqw;
                    break;
                case FLOAT_OR_INT_64_TYPE_F64:
                    pParser->Token = EQU_OP_NUMBER;
                    pParser->Value.value = pParser->SharpReplaceValue.V.d;
                    break;
                default:
                    ThrowError(1, "internal error %s (%i)", __FILE__, __LINE__);
                    pParser->Token = EQU_OP_NUMBER;
                     pParser->Value.value = 0.0;
                    break;
                }
                pParser->NextChar++;
                return pParser->Token;
            } else if (SHARP_REPLACE_VARIABLE (pParser)) {
                StringCopyMaxCharTruncate (pParser->NameString, pParser->SharpReplaceVariable, sizeof(pParser->NameString));
                pParser->NextChar++;
                return pParser->Token = EQU_OP_NAME;
            } else {
                EquationError (pParser, EQU_PARSER_ERROR_STOP, "'#' or '$' not allowed in this contest in equation \"%s\"", pParser->Equation);
                return pParser->Token = EQU_OP_END;
            }
            return pParser->Token = EQU_OP_END;
        default:                     /* Variable name */
            if (isalpha (ch) || (ch == '_')) {
                p = pParser->NameString;
                while (isalnum (*pParser->NextChar) || (*pParser->NextChar == '_') ||
                       (*pParser->NextChar == '[') || (*pParser->NextChar == ']') ||
                       (*pParser->NextChar == '.') || (*pParser->NextChar == ':') ||
                       (*pParser->NextChar == '{') || (*pParser->NextChar == '}') ||
                       (*pParser->NextChar == '@')) {
                    if ((p - pParser->NameString) >= ((int)sizeof (pParser->NameString) - 1)) {
                        EquationError (pParser, EQU_PARSER_ERROR_STOP, "label size exeeds %i chars in equation \"%s\"", sizeof (pParser->NameString) - 1, pParser->Equation);
                        return pParser->Token = EQU_OP_END;
                    }
                    *p++ = *pParser->NextChar++;
                }
                *p = 0;
                while (isspace (*pParser->NextChar)) pParser->NextChar++;
                if (*pParser->NextChar == '(') {
                    // Buildin function
                    return BuildinFunctions(pParser);
                } else {
                    // Variable
                    return pParser->Token = EQU_OP_NAME;
                }
            }
            EquationError (pParser, EQU_PARSER_ERROR_STOP, "illegal character %c (0x%i) in equation \"%s\"", ch, (int)ch, pParser->Equation);
            return pParser->Token = EQU_OP_END;
    }  // switch
}



static int direct_read_write_variable (EQUATION_PARSER_INFOS *pParser, char *name, int RefFlag, int Cmd,
                                       int ReadOrWrite,   // 1 -> Read, 0 -> Write
                                       int ReplacePosible)
{
    union OP_PARAM Value;
    union FloatOrInt64 Value2;

    if (SYNTAX_CHECK_ONLY (pParser)) {
        if (ReadOrWrite) {
            Value.value = 1.0;
            return direct_execute_one_command (EQU_OP_NUMBER, &pParser->Stack, Value);
        }
        return 0;
    }
    if (REPLACE_VARIABLE_WITH_FIXED_VALUE(pParser)) {
        if (!strcmp (name, pParser->ReplaceVariableFixedValue)) {
            if (ReplacePosible) {
                EquationError (pParser, EQU_PARSER_ERROR_STOP, "not more than %i parameter allowd in buildin functions (equation \"%s\")", (int)BUILDIN_FUNC_MAX_PARAMETER, pParser->Equation);
                return pParser->Token = EQU_OP_END;
            } else {
                switch (pParser->SharpReplaceValue.Type) {
                case FLOAT_OR_INT_64_TYPE_INT64:
                    Value.value_i64 = pParser->SharpReplaceValue.V.qw;
                    return direct_execute_one_command (EQU_OP_NUMBER_INT64, &pParser->Stack, Value);
                case FLOAT_OR_INT_64_TYPE_UINT64:
                    Value.value_ui64 = pParser->SharpReplaceValue.V.uqw;
                    return direct_execute_one_command (EQU_OP_NUMBER_UINT64, &pParser->Stack, Value);
                case FLOAT_OR_INT_64_TYPE_F64:
                    Value.value = pParser->SharpReplaceValue.V.d;
                    return direct_execute_one_command (EQU_OP_NUMBER, &pParser->Stack, Value);
                default:
                    ThrowError(1, "internal error %s (%i)", __FILE__, __LINE__);
                    Value.value = 0.0;
                    return direct_execute_one_command (EQU_OP_NUMBER, &pParser->Stack, Value);
                }
            }
        }
    }
#ifndef REMOTE_MASTER
    if (SCRIPT_LOCAL_VARIS_ALLOWED (pParser)) {
        if (ReadOrWrite) {
            if (RefFlag) {
                if (GetValueOfRefToLocalVariable (name, &Value.value, pParser->StackPos, Cmd) == 0) {
                    return direct_execute_one_command (EQU_OP_NUMBER, &pParser->Stack, Value);
                }
            } else {
                if (GetLocalVariableValue (name, &Value.value, pParser->StackPos) == 0) {
                    return direct_execute_one_command (EQU_OP_NUMBER, &pParser->Stack, Value);
                }
            }
        } else {
            Value.value = get_current_stack_value(&pParser->Stack);
            if (RefFlag) {
                if (SetValueOfLocalVariableRefTo (name, Value.value, Cmd) == 0) {
                    return pParser->Token = EQU_OP_END;
                }
            } else {
                if (SetLocalVariableValue (name, Value.value) == 0) {
                    return pParser->Token = EQU_OP_END;
                }
            }
        }
    }
#endif
    if (ReadOrWrite) {
        int ByteWidth;
        switch(read_bbvari_by_name(name, &Value2, &ByteWidth, Cmd)) {
        case FLOAT_OR_INT_64_TYPE_INT64:
            return add_stack_element_FloatOrInt64(&pParser->Stack, Value2, FLOAT_OR_INT_64_TYPE_INT64, ByteWidth);
        case FLOAT_OR_INT_64_TYPE_UINT64:
            return add_stack_element_FloatOrInt64(&pParser->Stack, Value2, FLOAT_OR_INT_64_TYPE_UINT64, ByteWidth);
        case FLOAT_OR_INT_64_TYPE_F64:
            return add_stack_element_FloatOrInt64(&pParser->Stack, Value2, FLOAT_OR_INT_64_TYPE_F64, ByteWidth);
        default:
            EquationError (pParser, EQU_PARSER_ERROR_STOP, "cannot read from variable '%s' in equation \"%s\"", name, pParser->Equation);
            return -1;
        }
    } else {
        int Ret;
        int BBType;
        int Pid;
        int Type = get_current_stack_FloatOrInt64(&pParser->Stack, &Value2);
        if (DONT_ADD_NOT_EXITING_VARS (pParser)) {
            BBType = BB_UNKNOWN;
        } else {
            BBType = BB_UNKNOWN_DOUBLE;
        }
        if (pParser->Pid < 0) {
            Pid = GET_PID();
        } else {
            Pid = pParser->Pid;
        }
        // add the variable if not exist
        Ret = write_bbvari_by_name(Pid, name, Value2, Type, BBType, !DONT_ADD_NOT_EXITING_VARS (pParser), Cmd, pParser->cs);
        if (Ret) EquationError (pParser, EQU_PARSER_ERROR_STOP, "cannot write to variable '%s' in equation \"%s\"", name, pParser->Equation);
        return Ret;
    }
}

static int prim (EQUATION_PARSER_INFOS *pParser)
{
    int ret = 0;
    int len;
    char *sv_puffer;
    int akt_baustein_save = pParser->Token;
    int ParamCount_save = pParser->ParamCount;

    switch (pParser->Token) {
        case EQU_OP_SINUS:   // Buildin functions with fixed parameter count
        case EQU_OP_COSINUS:
        case EQU_OP_TANGENS:
        case EQU_OP_SINUS_HYP:
        case EQU_OP_COSINUS_HYP:
        case EQU_OP_TANGENS_HYP:
        case EQU_OP_ARC_SINUS:
        case EQU_OP_ARC_COSINUS:
        case EQU_OP_ARC_TANGENS:
        case EQU_OP_EXP:
        case EQU_OP_POW:
        case EQU_OP_SQRT:
        case EQU_OP_LOG:
        case EQU_OP_LOG10:
        case EQU_OP_GETBITS:
        case EQU_OP_SETBITS:
        case EQU_OP_ANDBITS:
        case EQU_OP_ORBITS:
        case EQU_OP_XORBITS:
        case EQU_OP_BITWISE_AND:
        case EQU_OP_BITWISE_OR:
        case EQU_OP_BITWISE_XOR:
        case EQU_OP_BITWISE_INVERT:
        case EQU_OP_CANBYTE:
        case EQU_OP_CANWORD:
        case EQU_OP_CANDWORD:
        case EQU_OP_CAN_CYCLIC:
        case EQU_OP_CAN_DATA_CHANGED:
        case EQU_OP_ABS:
        case EQU_OP_EQUAL_DECIMALS:
        case EQU_OP_ROUND:
        case EQU_OP_ROUND_UP:
        case EQU_OP_ROUND_DOWN:
        case EQU_OP_MODULO:
        case EQU_OP_ADD_MSB_LSB:
        case EQU_OP_OVERFLOW:
        case EQU_OP_UNDERFLOW:
        case EQU_OP_MIN:
        case EQU_OP_MAX:
        case EQU_OP_SWAP16:
        case EQU_OP_SWAP32:
        case EQU_OP_SHIFT_LEFT:
        case EQU_OP_SHIFT_RIGHT:
        case EQU_OP_HAS_CHANGED:
        case EQU_OP_SLOPE_UP:
        case EQU_OP_SLOPE_DOWN:
        case EQU_OP_CAN_ID:
        case EQU_OP_CAN_CYCLES:
        case EQU_OP_CAN_SIZE:
        case EQU_OP_ADD_MSN_LSN:
        case EQU_OP_CAN_CRC8_REV_IN_OUT:
        case EQU_OP_TO_DOUBLE:
        case EQU_OP_TO_INT8:
        case EQU_OP_TO_INT16:
        case EQU_OP_TO_INT32:
        case EQU_OP_TO_INT64:
        case EQU_OP_TO_UINT8:
        case EQU_OP_TO_UINT16:
        case EQU_OP_TO_UINT32:
        case EQU_OP_TO_UINT64:
        case EQU_OP_GET_CALC_DATA_TYPE:
        case EQU_OP_GET_CALC_DATA_WIDTH:
            ReadToken (pParser);
            pParser->Value.value = 0.0;
            if (TRANSLATE_BYTE_CODE (pParser)) {
                return add_exec_stack_elem (akt_baustein_save, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid);
            } else {
                return direct_execute_one_command (akt_baustein_save, &(pParser->Stack), pParser->Value);
            }
        case EQU_OP_CRC8_USER_POLY:
        case EQU_OP_CRC16_USER_POLY:
        case EQU_OP_CRC8_USER_POLY_REFLECT:
        case EQU_OP_CRC16_USER_POLY_REFLECT:
        case EQU_OP_CRC32_USER_POLY_REFLECT:
            if (ReadToken (pParser)) ret = -1;
            if (TRANSLATE_BYTE_CODE (pParser)) {
                pParser->Value.value = ParamCount_save;
                if (add_exec_stack_elem (EQU_OP_NUMBER, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid)) ret = -1;
                pParser->Value.value = 0.0;
                if (add_exec_stack_elem (akt_baustein_save, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid)) ret = -1;
                return ret;
            } else {
                union OP_PARAM Value;
                Value.value = (double)ParamCount_save;
                if (direct_execute_one_command (EQU_OP_NUMBER, &(pParser->Stack), Value)) ret = -1;
                Value.value = 0.0;
                if (direct_execute_one_command (akt_baustein_save, &(pParser->Stack), Value)) ret = -1;
                return ret;
            }
        case EQU_OP_GET_PARAM:
            ReadToken (pParser);
            pParser->Value.value = 0.0;
            if (TRANSLATE_BYTE_CODE (pParser)) {
                return add_exec_stack_elem (EQU_OP_GET_PARAM, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid);
            } else {
                return direct_execute_one_command (EQU_OP_GET_PARAM, &(pParser->Stack), pParser->Value);
            }
        case EQU_OP_NUMBER:
        case EQU_OP_NUMBER_INT8:
        case EQU_OP_NUMBER_INT16:
        case EQU_OP_NUMBER_INT32:
        case EQU_OP_NUMBER_INT64:
        case EQU_OP_NUMBER_UINT8:
        case EQU_OP_NUMBER_UINT16:
        case EQU_OP_NUMBER_UINT32:
        case EQU_OP_NUMBER_UINT64:
            ReadToken (pParser);
            if (TRANSLATE_BYTE_CODE (pParser)) {
                return add_exec_stack_elem (akt_baustein_save, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid);
            } else {
                return direct_execute_one_command (akt_baustein_save, &(pParser->Stack), pParser->Value);
            }

        case EQU_OP_ONES_COMPLEMENT_BBVARI:
            len = (int)strlen (pParser->NameString) + 1;
            sv_puffer = _alloca (len);
            StringCopyMaxCharTruncate (sv_puffer, pParser->NameString, len);
            ReadToken (pParser);
            if (TRANSLATE_BYTE_CODE (pParser)) {
                pParser->Value.value = 0.0;
                return add_exec_stack_elem (EQU_OP_ONES_COMPLEMENT_BBVARI, sv_puffer, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid);
            } else {
                return direct_read_write_variable(pParser, sv_puffer, 0, READ_TYPE_ONS_COMPLEMENT, 1, 0); // Read, Replace not posible
            }
        case EQU_OP_GET_BINARY_BBVARI:
            len = (int)strlen (pParser->NameString) + 1;
            sv_puffer = _alloca (len);
            StringCopyMaxCharTruncate (sv_puffer, pParser->NameString, len);
            ReadToken (pParser);
            if (TRANSLATE_BYTE_CODE (pParser)) {
                pParser->Value.value = 0.0;
                return add_exec_stack_elem (EQU_OP_GET_BINARY_BBVARI, sv_puffer, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid);
            } else {
                return direct_read_write_variable(pParser, sv_puffer, 0, READ_TYPE_RAW_BINARY, 1, 0); // Read, Replace not posible
            }
        case EQU_OP_SET_BINARY_BBVARI:
            len = (int)strlen (pParser->NameString) + 1;
            sv_puffer = _alloca (len);
            StringCopyMaxCharTruncate (sv_puffer, pParser->NameString, len);
            ReadToken (pParser);
            if (TRANSLATE_BYTE_CODE (pParser)) {
                pParser->Value.value = 0.0;
                return add_exec_stack_elem (EQU_OP_SET_BINARY_BBVARI, sv_puffer, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid);
            } else {
                return direct_read_write_variable(pParser, sv_puffer, 0, WRITE_TYPE_RAW_BINARY, 0, 0); // Write, Replace not posible
            }
        case EQU_OP_NAME:
        case EQU_OP_NAME_PHYS:
        case EQU_OP_NAME_PHYS_REF:
            {
                int PhysFlag = (pParser->Token == EQU_OP_NAME_PHYS) || (pParser->Token == EQU_OP_NAME_PHYS_REF);
                int RefFlag = (pParser->Token == EQU_OP_NAME_PHYS_REF);
                int help = ReadToken (pParser);
                if ((help == EQU_OP_ASSIGNMENT) ||                     // on the left side of =
                    (help == EQU_OP_PLUS_EQUAL) ||
                    (help == EQU_OP_MINUS_EQUAL) ||
                    (help == EQU_OP_MUL_EQUAL) ||
                    (help == EQU_OP_DIV_EQUAL)) {
                    len = (int)strlen (pParser->NameString) + 1;
                    sv_puffer = _alloca (len);
                    StringCopyMaxCharTruncate (sv_puffer, pParser->NameString, len);
                    ReadToken (pParser);
                    if (TRANSLATE_BYTE_CODE (pParser)) {
                        if (help != EQU_OP_ASSIGNMENT) {
                            or_logic (pParser);

                            if (DONT_READ_FROM_NOT_EXITING_VARS(pParser)) {
                                if (READ_WRITE_FROM_NOT_EXITING_VARS_STOP(pParser)) {
                                    pParser->Value.value = 2.0;  // 2.0 means variable must be exist
                                } else {
                                    pParser->Value.value = 1.0;  // 1.0 means variable should exist
                                }
                            } else {
                                pParser->Value.value = 0.0;  // 0.0 means variable must not exist
                            }
                            ret = add_exec_stack_elem (PhysFlag ? EQU_OP_READ_BBVARI_PHYS : EQU_OP_READ_BBVARI,
                                                      sv_puffer, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid);
                            if (ret != 0) {
                                EquationError (pParser, (ret < 0) ? EQU_PARSER_ERROR_STOP : EQU_PARSER_ERROR_CONTINUE,
                                              "cannot read from variable \"%s\" in equation \"%s\"", sv_puffer, pParser->Equation);
                            }
                            if (ret < 0) {
                                ret = -1;
                            } else {
                                ret = 0;
                            }
                            if (add_exec_stack_elem (EQU_OP_SWAP_ON_STACK,
                                                     NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid)) ret = -1;
                            switch (help) {
                            case EQU_OP_PLUS_EQUAL:
                                if (add_exec_stack_elem (EQU_OP_PLUS, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid)) ret = -1;
                                break;
                            case EQU_OP_MINUS_EQUAL:
                                if (add_exec_stack_elem (EQU_OP_MINUS, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid)) ret = -1;
                                break;
                            case EQU_OP_MUL_EQUAL:
                                if (add_exec_stack_elem (EQU_OP_MUL, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid)) ret = -1;
                                break;
                            case EQU_OP_DIV_EQUAL:
                                if (add_exec_stack_elem (EQU_OP_DIV, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid)) ret = -1;
                                break;
                            }
                        } else {
                            if (or_logic (pParser)) ret = -1;
                        }
                        if (DONT_ADD_NOT_EXITING_VARS (pParser)) {
                            if (READ_WRITE_FROM_NOT_EXITING_VARS_STOP(pParser)) {
                                pParser->Value.value = 2.0;  // 2.0 means variable must be exist
                            } else {
                                pParser->Value.value = 1.0;  // 1.0 means variable should exist
                            }
                        } else {
                            pParser->Value.value = 0.0;  // 0.0 means variable must not exist
                        }
                        if (add_exec_stack_elem (PhysFlag ? EQU_OP_WRITE_BBVARI_PHYS : EQU_OP_WRITE_BBVARI,
                                                sv_puffer, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid)) ret = -1;
                        if (ret != 0) {
                            EquationError (pParser,  (ret < 0) ? EQU_PARSER_ERROR_STOP : EQU_PARSER_ERROR_CONTINUE,
                                          "cannot write to variable \"%s\" in equation \"%s\"", sv_puffer, pParser->Equation);
                        }
                        if (ret < 0) {
                            ret = -1;
                        } else {
                            ret = 0;
                        }
                    } else {
                        or_logic (pParser);
                        if (help != EQU_OP_ASSIGNMENT) {
                            if (direct_read_write_variable(pParser, sv_puffer, 0, READ_TYPE_RAW_VALUE, 1, 1)) ret = -1; // Read, Replace posible
                            pParser->Value.value = 0.0;
                            if (direct_execute_one_command (EQU_OP_SWAP_ON_STACK, &(pParser->Stack), pParser->Value)) ret = -1;
                            switch (help) {
                            case EQU_OP_PLUS_EQUAL:
                                if (direct_execute_one_command (EQU_OP_PLUS, &(pParser->Stack), pParser->Value)) ret = -1;
                                break;
                            case EQU_OP_MINUS_EQUAL:
                                if (direct_execute_one_command (EQU_OP_MINUS, &(pParser->Stack), pParser->Value)) ret = -1;
                                break;
                            case EQU_OP_MUL_EQUAL:
                                if (direct_execute_one_command (EQU_OP_MUL, &(pParser->Stack), pParser->Value)) ret = -1;
                                break;
                            case EQU_OP_DIV_EQUAL:
                                if (direct_execute_one_command (EQU_OP_DIV, &(pParser->Stack), pParser->Value)) ret = -1;
                                break;
                            }
                        }
                        if (direct_read_write_variable(pParser, sv_puffer, RefFlag, PhysFlag ? WRITE_TYPE_PHYS : WRITE_TYPE_RAW_VALUE, 0, 1)) ret = -1; // Write, Replace posible
                        return ret;
                    }
                    return ret;
                } else {    // on the right side of =
                    if (TRANSLATE_BYTE_CODE (pParser)) {
                        if (RefFlag) {
                            EquationError (pParser, EQU_PARSER_ERROR_STOP, "no dereference \"*p\" allowed in equation \"%s\"", pParser->Equation);
                            return -1;
                        }
                        if (DONT_READ_FROM_NOT_EXITING_VARS(pParser)) {
                            if (READ_WRITE_FROM_NOT_EXITING_VARS_STOP(pParser)) {
                                pParser->Value.value = 2.0;  // 2.0 means variable must be exist
                            } else {
                                pParser->Value.value = 1.0;  // 1.0 means variable should exist
                            }
                        } else {
                            pParser->Value.value = 0.0;  // 0.0 means variable must not exist
                        }
                        ret = add_exec_stack_elem (PhysFlag ? EQU_OP_READ_BBVARI_PHYS : EQU_OP_READ_BBVARI, pParser->NameString, pParser->Value,
                                                  &pParser->ExecStack, pParser->cs, pParser->Pid);
                        if (ret != 0) {
                            EquationError (pParser,  (ret < 0) ? EQU_PARSER_ERROR_STOP : EQU_PARSER_ERROR_CONTINUE,
                                          "cannot read from variable \"%s\" in equation \"%s\"", pParser->NameString, pParser->Equation);
                        }
                        if (ret < 0) {
                            ret = -1;
                        } else {
                            ret = 0;
                        }
                        return ret;
                    } else {
                        if (direct_read_write_variable(pParser, pParser->NameString, RefFlag, PhysFlag ? READ_TYPE_PHYS :  READ_TYPE_RAW_VALUE, 1, 1)) ret = -1; // Write, Replace posible
                        return ret;
                    }
                }
            }
#ifndef REMOTE_MASTER
        case EQU_OP_MUL:                         // Dereferenzing is only possible with local variables inside a script
            if (!SCRIPT_LOCAL_VARIS_ALLOWED (pParser)) {
                EquationError (pParser, EQU_PARSER_ERROR_STOP, "no dereference \"*p\" allowed in equation \"%s\"", pParser->Equation);
                return -1;
            } else {
                if (ReadToken (pParser) == EQU_OP_NAME) {
                    int help = ReadToken (pParser);
                    if ((help == EQU_OP_ASSIGNMENT) ||
                        (help == EQU_OP_PLUS_EQUAL) ||
                        (help == EQU_OP_MINUS_EQUAL) ||
                        (help == EQU_OP_MUL_EQUAL) ||
                        (help == EQU_OP_DIV_EQUAL)) {
                        len = (int)strlen (pParser->NameString) + 1;
                        sv_puffer = _alloca (len);
                        StringCopyMaxCharTruncate (sv_puffer, pParser->NameString, len);
                        ReadToken (pParser);
                        or_logic (pParser);
                        if (help != EQU_OP_ASSIGNMENT) {
                            union OP_PARAM Value;
                            Value.value = ReadValueOfLocalVariableRefTo (pParser, sv_puffer, READ_TYPE_RAW_VALUE);
                            if (direct_execute_one_command (EQU_OP_NUMBER, &(pParser->Stack), Value)) ret = -1;
                            switch (help) {
                            case EQU_OP_PLUS_EQUAL:
                                if (direct_execute_one_command (EQU_OP_PLUS, &(pParser->Stack), Value)) ret = -1;
                                break;
                            case EQU_OP_MINUS_EQUAL:
                                if (direct_execute_one_command (EQU_OP_MINUS, &(pParser->Stack), Value)) ret = -1;
                                break;
                            case EQU_OP_MUL_EQUAL:
                                if (direct_execute_one_command (EQU_OP_MUL, &(pParser->Stack), Value)) ret = -1;
                                break;
                            case EQU_OP_DIV_EQUAL:
                                if (direct_execute_one_command (EQU_OP_DIV, &(pParser->Stack), Value)) ret = -1;
                                break;
                            }
                        } else {
                            ret = 0;
                        }
                        WriteValueOfLocalVariableRefTo (pParser, sv_puffer, get_current_stack_value(&pParser->Stack), WRITE_TYPE_RAW_VALUE);
                        return ret;
                    } else {
                        union OP_PARAM Value;
                        Value.value = ReadValueOfLocalVariableRefTo (pParser, pParser->NameString, READ_TYPE_RAW_VALUE);
                        if (direct_execute_one_command (EQU_OP_NUMBER, &(pParser->Stack), Value)) ret = -1;
                        return ret;
                    }
                } else {
                    EquationError (pParser, EQU_PARSER_ERROR_STOP, "need a name for dereference \"*p\" in equation \"%s\"", pParser->Equation);
                    return -1;
                }
            }
#endif
        case EQU_OP_MINUS:                       // unary minus
            ReadToken (pParser);
            prim (pParser);
            if (TRANSLATE_BYTE_CODE (pParser)) {
                pParser->Value.value = 0.0;
                return add_exec_stack_elem (EQU_OP_NEG, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid);
            } else {
                union OP_PARAM Value;
                Value.value = 0.0;
                return direct_execute_one_command (EQU_OP_NEG, &(pParser->Stack), Value);
            }
        case EQU_OP_PLUS:                       // unary plus
            ReadToken (pParser);
            prim (pParser);
            if (TRANSLATE_BYTE_CODE (pParser)) {
                return 0;
            } else {
                return 0;
            }
        case EQU_OP_NOT:
            ReadToken (pParser);
            prim (pParser);
            if (TRANSLATE_BYTE_CODE (pParser)) {
                pParser->Value.value = 0.0;
                return add_exec_stack_elem (EQU_OP_NOT, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid);
            } else {
                union OP_PARAM Value;
                Value.value = 0.0;
                return direct_execute_one_command (EQU_OP_NOT, &(pParser->Stack), Value);
            }
        case EQU_OP_OPEN_BRACKETS:
            ReadToken (pParser);
            or_logic (pParser);
            if (pParser->Token != EQU_OP_CLOSE_BRACKETS) {
                EquationError (pParser, EQU_PARSER_ERROR_STOP, "missing ')' in equation \"%s\"", pParser->Equation);
                return -1;
            }
            ReadToken (pParser);
            return 0;
        case EQU_OP_END:
            //return 1;
        default:
            if ((pParser->Token >= USER_DEFINED_BUILDIN_FUNCTION_OFFSET) &&
                (pParser->Token < (USER_DEFINED_BUILDIN_FUNCTION_OFFSET + UserDefinedBuildinFunctionTableSize))) {
                // User defined buildin functions
                ReadToken (pParser);
                pParser->Value.value = 0.0;
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    return add_exec_stack_elem (akt_baustein_save, NULL, pParser->Value, &pParser->ExecStack, pParser->cs, pParser->Pid);
                } else {
                    return direct_execute_one_command (akt_baustein_save, &(pParser->Stack), pParser->Value);
                }
            } else {
                EquationError (pParser, EQU_PARSER_ERROR_STOP, "expecting a primary (value, variable, (), or a command) in equation \"%s\"", pParser->Equation);
            }

    }
    return 0;
}

static int or_call (int x1, int x2)
{
    return x1 | x2;
}

/* * / */
static int term (EQUATION_PARSER_INFOS *pParser)
{
    int ret;

    ret = prim (pParser);

    while (!pParser->ErrorInEquationStop) {
        switch (pParser->Token) {
            case EQU_OP_MUL:
            ReadToken (pParser);
                prim (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_MUL, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_MUL, &(pParser->Stack), Value), ret);
                }
                break;
            case EQU_OP_DIV:
            ReadToken (pParser);
                prim (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_DIV, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_DIV, &(pParser->Stack), Value), ret);
                }
                break;
            default:
                return ret;
        }
    }
    return ret;
}


/* + - */
static int add_sub (EQUATION_PARSER_INFOS *pParser)
{
    int  ret;

    ret = term (pParser);
    while (!pParser->ErrorInEquationStop) {
        switch (pParser->Token) {
            case EQU_OP_PLUS:
            ReadToken (pParser);
                term (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call (add_exec_stack_elem (EQU_OP_PLUS, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call (direct_execute_one_command (EQU_OP_PLUS, &(pParser->Stack), Value), ret);
                }
                break;
            case EQU_OP_MINUS:
            ReadToken (pParser);
                term (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_MINUS, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_MINUS, &(pParser->Stack), Value), ret);
                }
                break;
            case EQU_OP_NOT:
                EquationError (pParser, EQU_PARSER_ERROR_STOP, "illegal '!' in equation \"%s\"", pParser->Equation);
                return -1;
            default:
                return ret;
        }
    }
    return ret;
}

/* >> << */
static int shift (EQUATION_PARSER_INFOS *pParser)
{
    int ret;

    ret = add_sub (pParser);
    while (!pParser->ErrorInEquationStop) {
        switch (pParser->Token) {
            case EQU_OP_SHIFT_LEFT:
            ReadToken (pParser);
                add_sub (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_SHIFT_LEFT, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_SHIFT_LEFT, &(pParser->Stack), Value), ret);
                }
                break;
            case EQU_OP_SHIFT_RIGHT:
            ReadToken (pParser);
                add_sub (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_SHIFT_RIGHT, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_SHIFT_RIGHT, &(pParser->Stack), Value), ret);
                }
                break;
            default:
                return ret;
        }
    }
    return ret;
}

/* < <= > >= */
static int compare (EQUATION_PARSER_INFOS *pParser)
{
    int ret;

    ret = shift (pParser);
    while (!pParser->ErrorInEquationStop) {
        switch (pParser->Token) {
            case EQU_OP_LARGER:
            ReadToken (pParser);
                shift (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_LARGER, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_LARGER, &(pParser->Stack), Value), ret);
                }
                break;
            case EQU_OP_LARGER_EQUAL:
            ReadToken (pParser);
                shift (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_LARGER_EQUAL, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_LARGER_EQUAL, &(pParser->Stack), Value), ret);
                }
                break;
            case EQU_OP_SMALLER:
            ReadToken (pParser);
                shift (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_SMALLER, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_SMALLER, &(pParser->Stack), Value), ret);
                }
                break;
            case EQU_OP_SMALLER_EQUAL:
            ReadToken (pParser);
                shift (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_SMALLER_EQUAL, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_SMALLER_EQUAL, &(pParser->Stack), Value), ret);
                }
                break;
            default:
                return ret;
        }
    }
    return ret;
}

/* == != */
static int equal (EQUATION_PARSER_INFOS *pParser)
{
    int ret;

    ret = compare (pParser);
    while (!pParser->ErrorInEquationStop) {
        switch (pParser->Token) {
            case EQU_OP_EQUAL:
            ReadToken (pParser);
                compare (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_EQUAL, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_EQUAL, &(pParser->Stack), Value), ret);
                }
                break;
            case EQU_OP_NOT_EQUAL:
            ReadToken (pParser);
                compare (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_NOT_EQUAL, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_NOT_EQUAL, &(pParser->Stack), Value), ret);
                }
                break;
            default:
                return ret;
        }
    }
    return ret;
}

/* & */
static int and_bits (EQUATION_PARSER_INFOS *pParser)
{
    int ret;

    ret = equal (pParser);
    while(!pParser->ErrorInEquationStop) {
        switch (pParser->Token) {
            case EQU_OP_BITWISE_AND:
            ReadToken (pParser);
                equal (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_LOGICAL_AND, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_LOGICAL_AND, &(pParser->Stack), Value), ret);
                }
                break;
            default:
                return ret;
        }
    }
    return ret;
}

/* ^ */
static int xor_bits (EQUATION_PARSER_INFOS *pParser)
{
    int ret;

    ret = and_bits (pParser);
    while(!pParser->ErrorInEquationStop) {
        switch (pParser->Token) {
            case EQU_OP_BITWISE_XOR:
            ReadToken (pParser);
                and_bits (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_BITWISE_XOR, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_BITWISE_XOR, &(pParser->Stack), Value), ret);
                }
                break;
            default:
                return ret;
        }
    }
    return ret;
}

/* | */
static int or_bits (EQUATION_PARSER_INFOS *pParser)
{
    int ret;

    ret = xor_bits (pParser);
    while(!pParser->ErrorInEquationStop) {
        switch (pParser->Token) {
            case EQU_OP_BITWISE_OR:
            ReadToken (pParser);
                xor_bits (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_BITWISE_OR, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_BITWISE_OR, &(pParser->Stack), Value), ret);
                }
                break;
            default:
                return ret;
        }
    }
    return ret;
}

/* && */
static int and_logic (EQUATION_PARSER_INFOS *pParser)
{
    int ret;

    ret = or_bits (pParser);
    while(!pParser->ErrorInEquationStop) {
        switch (pParser->Token) {
            case EQU_OP_LOGICAL_AND:
            ReadToken (pParser);
                or_bits (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_LOGICAL_AND, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_LOGICAL_AND, &(pParser->Stack), Value), ret);
                }
                break;
            default:
                return ret;
        }
    }
    return ret;
}

/* || */
static int or_logic (EQUATION_PARSER_INFOS *pParser)
{
    int ret;

    ret = and_logic (pParser);
    while (!pParser->ErrorInEquationStop) {
        switch (pParser->Token) {
            case EQU_OP_LOGICAL_OR:
            ReadToken (pParser);
                and_logic (pParser);
                if (TRANSLATE_BYTE_CODE (pParser)) {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(add_exec_stack_elem (EQU_OP_LOGICAL_OR, NULL, Value, &pParser->ExecStack, pParser->cs, pParser->Pid), ret);
                } else {
                    union OP_PARAM Value;
                    Value.value = 0.0;
                    ret = or_call(direct_execute_one_command (EQU_OP_LOGICAL_OR, &(pParser->Stack), Value), ret);
                }
                break;
            default:
                return ret;
        }
    }
    return ret;
}


// This will translate an equation string into an byte code (execution stack)
struct EXEC_STACK_ELEM *solve_equation (const char *equation)
{
    struct EXEC_STACK_ELEM *ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_TRANSLATE_TO_BYTECODE;    // Equation should be translated
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;
    Parser.ExecStack = NULL;      // New execution stack

    Parser.Equation = Parser.NextChar = equation;   // Init. pointers to equation
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);                 //  Translate the rest of the equation

    if (Parser.ErrorInEquationStop) {
        if (Parser.ExecStack != NULL) remove_exec_stack (Parser.ExecStack);
        ret = NULL;
    } else {
        ret = Parser.ExecStack;
    }
    return ret;
}

// This will translate an equation string into an byte code (execution stack)
// And give back an error string if an error ocures
struct EXEC_STACK_ELEM *solve_equation_err_string (const char *equation, char **errstring)
{
    struct EXEC_STACK_ELEM *ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_TRANSLATE_TO_BYTECODE;    // Equation should be translated
    Parser.Pid = -1;
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    if (errstring != NULL) *errstring = NULL;
    Parser.ErrorString = errstring;
    Parser.ErrStringLength = 0;

    Parser.ExecStack = NULL;  // New execution stack

    Parser.Equation = Parser.NextChar = equation;   // Init. pointers to equation
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);                 //  Translate the rest of the equation

    if (Parser.ErrorInEquationStop) {
        if (Parser.ExecStack != NULL) remove_exec_stack (Parser.ExecStack);
        ret = NULL;
    } else {
        ret = Parser.ExecStack;
    }
    return ret;
}

// This will translate an equation string into an byte code (execution stack)
// But don't automatic add none existing variables to the blackboard
struct EXEC_STACK_ELEM *solve_equation_no_add_bbvari (const char *equation, char **errstring, int AddNotExistingVars)
{
    struct EXEC_STACK_ELEM *ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_TRANSLATE_TO_BYTECODE;    // Equation should be translated

    CheckAddNotExistingVars(&Parser, AddNotExistingVars);

    CheckAddNotExistingVars(&Parser, AddNotExistingVars);
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = errstring;
    Parser.ErrStringLength = 0;
    Parser.ExecStack = NULL;  // New execution stack

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation

    if (Parser.ErrorInEquationStop) {
        if (Parser.ExecStack != NULL) remove_exec_stack (Parser.ExecStack);
        ret = NULL;
    } else {
        ret = Parser.ExecStack;
    }
    return ret;
}

#if 0
struct EXEC_STACK_ELEM *solve_equation_no_add_bbvari_err_string (const char *equation, char **errstring)
{
    struct EXEC_STACK_ELEM *ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_TRANSLATE_TO_BYTECODE    // Equation should be translated
                   | EQU_PARSER_DONT_ADD_NOT_EXITING_VARS;  // If a variable not exist, don't automatic add none existing variables to the blackboard

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    if (errstring != NULL) *errstring = NULL;
    Parser.ErrorString = errstring;
    Parser.ErrStringLength = 0;
    Parser.ExecStack = NULL;  // New execution stack

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation

    if (Parser.ErrorInEquationStop) {
        if (Parser.ExecStack != NULL) remove_exec_stack (Parser.ExecStack);
        ret = NULL;
    } else {
        ret = Parser.ExecStack;
    }
    return ret;
}
#endif

int solve_equation_for_process(const char *equation, int pid, int AddNotExistingVars,
                               struct EXEC_STACK_ELEM **ret_ExecStack, char **errstring)
{
    int ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_TRANSLATE_TO_BYTECODE       // Equation should be translated
                   | EQU_PARSER_ADD_BB_VARI_FOR_PROCESS;  // If a variable don't exist add the variable for an defined process

    CheckAddNotExistingVars(&Parser, AddNotExistingVars);

    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = pid;
    if (errstring != NULL) *errstring = NULL;
    Parser.ErrorString = errstring;
    Parser.ErrStringLength = 0;
    Parser.ExecStack = NULL;  // New execution stack

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation

    if (Parser.ErrorInEquationStop) {
        if (Parser.ExecStack != NULL) remove_exec_stack (Parser.ExecStack);
        *ret_ExecStack = NULL;
        ret = -2;   // error stop
    } else if (Parser.ErrorInEquationContinue) {
        *ret_ExecStack = Parser.ExecStack;
        ret = -1;   // error continue;
    } else {
        *ret_ExecStack = Parser.ExecStack;
        ret = 0;   // no error;
    }
    return ret;
}


// Direct calculate an eqiation
double direct_solve_equation (const char *equation)
{
    double erg;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT;     // directly calculate the equation
    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation

    erg = get_current_stack_value(&(Parser.Stack));

    return erg;
}

double direct_solve_equation_script_stack (const char *equation, int StackPos)
{
    double erg;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT     // directly calculate the equation
                 | EQU_PARSER_SCRIPT_LOCAL_VARIABLES;   // Use the local stack of the script interpreter

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;
    Parser.StackPos = StackPos;

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    erg = get_current_stack_value(&(Parser.Stack));

    return erg;
}

double direct_solve_equation_no_error_message (const char *equation)
{
    double erg;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT     // directly calculate the equation
                   | EQU_PARSER_NO_ERR_MSG;   // If an error occur no error message

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    erg = get_current_stack_value(&(Parser.Stack));

    return erg;
}

double direct_solve_equation_no_add_bbvari (const char *equation)
{
    double erg;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT     // directly calculate the equation
                   | EQU_PARSER_DONT_ADD_NOT_EXITING_VARS;  // If a variable not exist, don't automatic add none existing variables to the blackboard

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    erg = get_current_stack_value(&(Parser.Stack));

    return erg;
}

double direct_solve_equation_no_add_bbvari_pid (const char *equation, int pid)
{
    double erg;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT     // directly calculate the equation
                   | EQU_PARSER_DONT_ADD_NOT_EXITING_VARS;  // If a variable not exist, don't automatic add none existing variables to the blackboard

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = pid;
    Parser.ErrorString = NULL;

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    erg = get_current_stack_value(&(Parser.Stack));

    return erg;
}

double direct_solve_equation_script_stack_no_add_bbvari (const char *equation, int StackPos)
{
    double erg;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT     // directly calculate the equation
                   | EQU_PARSER_DONT_ADD_NOT_EXITING_VARS  // If a variable not exist, don't automatic add none existing variables to the blackboard
                   | EQU_PARSER_SCRIPT_LOCAL_VARIABLES;   // Use the local stack of the script interpreter

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;
    Parser.StackPos = StackPos;

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    erg = get_current_stack_value(&(Parser.Stack));

    return erg;
}

// Directly calculate an equation and give back an error state
int direct_solve_equation_err_sate (const char *equation, double *erg)
{
    int ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT;     // directly calculate the equation

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    *erg = get_current_stack_value(&(Parser.Stack));

    ret = Parser.ErrorInEquationStop;
    return ret;
}

int direct_solve_equation_script_stack_err_sate (char *equation, double *erg, int StackPos)
{
    int ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT     // directly calculate the equation
                 | EQU_PARSER_SCRIPT_LOCAL_VARIABLES;   // Use the local stack of the script interpreter

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;
    Parser.StackPos = StackPos;

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    *erg = get_current_stack_value(&(Parser.Stack));

    ret = Parser.ErrorInEquationStop;
    return ret;
}

int direct_solve_equation_err_string (const char *equation, double *erg, char **errstring)
{
    int ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT;     // directly calculate the equation

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    if (errstring != NULL) *errstring = NULL;
    Parser.ErrorString = errstring;
    Parser.ErrStringLength = 0;

    
    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    *erg = get_current_stack_value(&(Parser.Stack));

    ret = Parser.ErrorInEquationStop;
    return ret;
}

int direct_solve_equation_script_stack_err_string (const char *equation, double *erg, char **errstring, int StackPos, int AddNotExistingVars)
{
    int ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT     // directly calculate the equation
                 | EQU_PARSER_SCRIPT_LOCAL_VARIABLES;   // Use the local stack of the script interpreter

    CheckAddNotExistingVars(&Parser, AddNotExistingVars);

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    if (errstring != NULL) *errstring = NULL;
    Parser.ErrorString = errstring;
    Parser.ErrStringLength = 0;

    Parser.StackPos = StackPos;
    
    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    *erg = get_current_stack_value(&(Parser.Stack));

    ret = Parser.ErrorInEquationStop;
    return ret;
}


int direct_solve_equation_script_stack_err_string_float_or_int64 (const char *equation, union FloatOrInt64 *ret_ResultValue, int *ret_ResultType, char **errstring, int StackPos, int AddNotExistingVars)
{
    int ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT     // directly calculate the equation
                 | EQU_PARSER_SCRIPT_LOCAL_VARIABLES;   // Use the local stack of the script interpreter

    CheckAddNotExistingVars(&Parser, AddNotExistingVars);

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    if (errstring != NULL) *errstring = NULL;
    Parser.ErrorString = errstring;
    Parser.ErrStringLength = 0;

    Parser.StackPos = StackPos;

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    *ret_ResultType = get_current_stack_FloatOrInt64(&(Parser.Stack), ret_ResultValue);

    ret = Parser.ErrorInEquationStop;
    return ret;
}

struct EXEC_STACK_ELEM *solve_equation_replace_variable (char *equation, char *name)
{
    struct EXEC_STACK_ELEM *ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_TRANSLATE_TO_BYTECODE    // Equation should be translated
                   | EQU_PARSER_SHARP_REPLACE_VARIABLE;   // the token # should be replaced with a variable

    Parser.SharpReplaceVariable = name;

    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;

    Parser.ExecStack = NULL;  // New execution stack

    Parser.Equation = Parser.NextChar = equation;   // Init. pointers to equation
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);                 //  Translate the rest of the equation

    if (Parser.ErrorInEquationStop) {
        if (Parser.ExecStack != NULL) remove_exec_stack (Parser.ExecStack);
        ret = NULL;
    } else {
        ret = Parser.ExecStack;
    }
    return ret;
}


int direct_solve_equation_err_state_replace_value (const char *equation, double value, double *erg)
{
    int ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), value);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT   // directly calculate the equation
                   | EQU_PARSER_SHARP_REPLACE_VALUE;   // replace all #/$ token with a fixed value

    // The fixed value that should replace the #/$ token
    Parser.SharpReplaceValue.V.d = value;
    Parser.SharpReplaceValue.Type  = FLOAT_OR_INT_64_TYPE_F64;
    Parser.SharpReplaceValue.ByteWidth = 8;
    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;

    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    *erg = get_current_stack_value(&(Parser.Stack));

    ret = Parser.ErrorInEquationStop;
    return ret;
}

int direct_solve_equation_err_string_replace_value (char *equation, double value, double *erg, char **errstring)
{
    int ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), value);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT   // directly calculate the equation
                   | EQU_PARSER_SHARP_REPLACE_VALUE;   // replace all #/$ token with a fixed value

    // The fixed value that should replace the #/$ token
    Parser.SharpReplaceValue.V.d = value;
    Parser.SharpReplaceValue.Type  = FLOAT_OR_INT_64_TYPE_F64;
    Parser.SharpReplaceValue.ByteWidth = 8;

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    if (errstring != NULL) *errstring = NULL;
    Parser.ErrorString = errstring;
    Parser.ErrStringLength = 0;


    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    *erg = get_current_stack_value(&(Parser.Stack));

    ret = Parser.ErrorInEquationStop;
    return ret;
}

int direct_solve_equation_err_string_replace_union (const char *equation, union BB_VARI value, int value_bbtype, union BB_VARI *erg, int *ret_bbtype, char **errstring)  // wird in 3dview.cpp(294/320/346)  direct_solve_equation_err_sate
{
    int ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);

    convert_to_stack_entry (value, value_bbtype, &Parser.Stack.Parameter);

    // The fixed value that should replace the #/$ token
    Parser.SharpReplaceValue = Parser.Stack.Parameter;

    Parser.Flags = EQU_PARSER_SOLVE_DIRECT   // directly calculate the equation
                   | EQU_PARSER_SHARP_REPLACE_VALUE;   // replace all #/$ token with a fixed value

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    if (errstring != NULL) *errstring = NULL;
    Parser.ErrorString = errstring;
    Parser.ErrStringLength = 0;
    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    get_current_stack_union(&(Parser.Stack), erg, ret_bbtype);

    ret = Parser.ErrorInEquationStop;
    return ret;
}


// The # and/or one variable should be replaced by a fixed value
int direct_solve_equation_err_string_replace_var_value (const char *equation, const char *variable, double value, double *erg, char **errstring)
{
    int ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), value);
    Parser.Flags = EQU_PARSER_SOLVE_DIRECT   // directly calculate the equation
                   | EQU_PARSER_SHARP_REPLACE_VALUE                  // replace all #/$ token with a fixed value
                   | EQU_PARSER_REPLACE_VARIABLE_WITH_FIXED_VALUE;   // ersetze eine Variable mit dem gleichen Festwert
    // The fixed value that should replace the #/$ token
    Parser.SharpReplaceValue.V.d = value;
    Parser.SharpReplaceValue.Type  = FLOAT_OR_INT_64_TYPE_F64;
    Parser.SharpReplaceValue.ByteWidth = 8;
    Parser.ReplaceVariableFixedValue = variable;

    // do not translate only direct calculate
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    if (errstring != NULL) *errstring = NULL;
    Parser.ErrorString = errstring;
    Parser.ErrStringLength = 0;
    Parser.Equation = Parser.NextChar = equation;
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);           //  Translate the rest of the equation
    *erg = get_current_stack_value(&(Parser.Stack));

    ret = Parser.ErrorInEquationStop;
    return ret;
}

struct EXEC_STACK_ELEM *solve_equation_replace_parameter (const char *equation)
{
    struct EXEC_STACK_ELEM *ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_TRANSLATE_TO_BYTECODE  // Equation should be translated
                   | EQU_PARSER_SHARP_REPLACE_PARAMETER;   // replace all #/$ token with a fixed value

    // replace all #/$ token with a fixed value
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;
    Parser.ExecStack = NULL;  // New execution stack

    Parser.Equation = Parser.NextChar = equation;   // Init. pointers to equation
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);                 //  Translate the rest of the equation

    if (Parser.ErrorInEquationStop) {
        if (Parser.ExecStack != NULL) remove_exec_stack (Parser.ExecStack);
        ret = NULL;
    } else {
        ret = Parser.ExecStack;
    }
    return ret;
}

// replace all #/$ token with a fixed value
struct EXEC_STACK_ELEM *solve_equation_replace_parameter_with_can (const char *equation, int *pUseCANData)
{
    struct EXEC_STACK_ELEM *ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_CAN_COMMANDS_ALLOWED    // CAN buildin function should be enabeled
                   | EQU_PARSER_TRANSLATE_TO_BYTECODE  // Equation should be translated
                   | EQU_PARSER_SHARP_REPLACE_PARAMETER;   // replace all #/$ token with a fixed value

    // replace all #/$ token with a fixed value
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;
    Parser.ExecStack = NULL;  // New execution stack
    Parser.UseCanOrFlexrayDataFlag = 0;

    Parser.Equation = Parser.NextChar = equation;   // Init. pointers to equation
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);                 //  Translate the rest of the equation

    if (pUseCANData != NULL) *pUseCANData = Parser.UseCanOrFlexrayDataFlag;

    if (Parser.ErrorInEquationStop) {
        if (Parser.ExecStack != NULL) remove_exec_stack (Parser.ExecStack);
        ret = NULL;
    } else {
        ret = Parser.ExecStack;
    }
    return ret;
}

struct EXEC_STACK_ELEM *solve_equation_replace_parameter_with_flexray (const char *equation, int *pUseFlexrayData)  // wird in rdcancfg.c(130) verwendet
{
    struct EXEC_STACK_ELEM *ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags = EQU_PARSER_FLEXRAY_COMMANDS_ALLOWED    // Flexray buildin function should be enabeled
                   | EQU_PARSER_TRANSLATE_TO_BYTECODE  // Equation should be translated
                   | EQU_PARSER_SHARP_REPLACE_PARAMETER;   // replace all #/$ token with a fixed value

    // replace all #/$ token with a fixed value
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;
    Parser.ExecStack = NULL;  // New execution stack
    Parser.UseCanOrFlexrayDataFlag = 0;

    Parser.Equation = Parser.NextChar = equation;   // Init. pointers to equation
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);                 //  Translate the rest of the equation

    if (pUseFlexrayData != NULL) *pUseFlexrayData = Parser.UseCanOrFlexrayDataFlag;

    if (Parser.ErrorInEquationStop) {
        if (Parser.ExecStack != NULL) remove_exec_stack (Parser.ExecStack);
        ret = NULL;
    } else {
        ret = Parser.ExecStack;
    }
    return ret;
}

int CheckEquSyntax (char *Equation, int CANCommandAllowed, int CheckIfVarsExists, int *pUseCANData)
{
    int ret;
    EQUATION_PARSER_INFOS Parser;

    Parser.cs = 1;
    init_stack(&(Parser.Stack), 0.0);
    Parser.Flags =  EQU_PARSER_SOLVE_DIRECT
                  | EQU_PARSER_SYNTAX_CHECK_ONLY
                  | EQU_PARSER_SHARP_REPLACE_PARAMETER;   // replace all #/$ token with a fixed value
    if (CANCommandAllowed) Parser.Flags |= EQU_PARSER_CAN_COMMANDS_ALLOWED;
    if (CheckIfVarsExists) Parser.Flags |= EQU_PARSER_SYNTAX_CHECK_ONLY_VARIABLES_EXIST;

    // replace all #/$ token with a fixed value
    Parser.ErrorInEquationStop = 0;
    Parser.ErrorInEquationContinue = 0;
    Parser.Pid = -1;
    Parser.ErrorString = NULL;
    Parser.ExecStack = NULL;  // New execution stack
    Parser.UseCanOrFlexrayDataFlag = 0;

    Parser.Equation = Parser.NextChar = Equation;   // Init. pointers to equation
    ReadToken (&Parser);         // read first tocken
    or_logic (&Parser);                 //  Translate the rest of the equation

    if (pUseCANData != NULL) *pUseCANData = Parser.UseCanOrFlexrayDataFlag;

    if (Parser.ErrorInEquationStop) {
        ret = 0;
    } else {
        ret = 1;
    }
    return ret;
}

void FreeErrStringBuffer (char **errstring)
{
    if (*errstring != NULL) {
        my_free (*errstring);
        *errstring = NULL;
    }
}


static char *malloc_message_string(char *message)
{
    char *ret;
    ret = StringMalloc (message);
    return ret;
}

static int CompareDoubleEqual(double a, double b)
{
    double diff = a - b;
    if ((diff <= 0.0) && (diff >= 0.0)) return 1;
    else return 0;
}

// sehr viel doppelt im bb_funcs.c
static int write_bbvari_phys_minmax_check_iteration (const char *equation, const char *variable_name, int type, double new_phys_value, double x1, double y1, double min, double max, double delta,
                                                     double *ret_new_dec_value, double *ret_m, char **errstring)
{
    UNUSED(min);
    double x2, y2;
    union BB_VARI Value;
    double Help;
    double m, b;
    double new_dec_value;

    x2 = x1 + delta;
    if (x2 > max) {
        x2 = x1 - delta;
    }
    ConvertDoubleToUnion(type, x2, &Value);
    if (bbvari_to_double (type,
                          Value,
                          &Help) == -1) {
        if (errstring != NULL) *errstring = malloc_message_string ("unknown data type");
        return -1;
    }

    if (CompareDoubleEqual(x1,x2)) {
        if (errstring != NULL) *errstring = malloc_message_string ("cannot calculate 2 raw values");
        return -1;
    }

    if (direct_solve_equation_err_string_replace_var_value (equation, variable_name, Help, &y2, errstring) != 0) {
        if (errstring != NULL) *errstring = malloc_message_string ("cannot calculate physical value");
        return -1;
    }

    m = (y2 - y1) / (x2 - x1);
    b = y1 - m * x1;

    if (m == 0.0) {
        return -2;
    }

    *ret_m = m;

    new_dec_value = (new_phys_value - b) / m;

    ConvertDoubleToUnion(type, new_dec_value, &Value);

    if (bbvari_to_double (type,
                          Value,
                          ret_new_dec_value) == -1) {
        if (errstring != NULL) *errstring = malloc_message_string ("unknown data type");
        return -1;
    }
    if (IsMinOrMaxLimit (type, Value)) {
        return 1;  // Value is at the limit
    } else {
         // Only if the min./max. boarders of the data type are not reached
        return 0;
    }
}

static int write_bbvari_phys_minmax_check_diff (const char *equation, const char *variable_name, double new_phys_value, double raw_value, double *ret_diff, double *ret_phys_value, char **errstring)
{
    double phys;
    if (direct_solve_equation_err_string_replace_var_value (equation, variable_name, raw_value, &phys, errstring) != 0) {
        if (errstring != NULL) *errstring = malloc_message_string ("cannot calculate physical value");
        return -1;
    }
    *ret_phys_value = phys;
    *ret_diff = fabs(phys - new_phys_value);
    return 0;
}

static double mantissa_plus_or_minus_one_digit_double (double value, int plus_or_minus)
{
    double ret;
    int exp;
    double man = frexp (value, &exp);
    if (plus_or_minus) {
        man += DBL_EPSILON;
    } else {
        man -= DBL_EPSILON;
    }
    ret = ldexp(man, exp);
    return ret;
}

static float mantissa_plus_or_minus_one_digit_float (float value, int plus_or_minus)
{
    float ret;
    int exp;
    float man = frexpf (value, &exp);
    if (plus_or_minus) {
        man += FLT_EPSILON;
    } else {
        man -= FLT_EPSILON;
    }
    ret = ldexpf(man, exp);
    return ret;
}


static void calc_min_max_target_value_float (double target_value, double *ret_min, double *ret_max)
{
    int exp;
    float man2;
    float man = frexpf ((float)target_value, &exp);
    man2 = man - 8 * FLT_EPSILON;
    *ret_min = (double)ldexpf(man2, exp);
    man2 = man + 8 * FLT_EPSILON;
    *ret_max = (double)ldexpf(man2, exp);
}

static void calc_min_max_target_value_double (double target_value, double *ret_min, double *ret_max)
{
    int exp;
    double man2;
    double man = frexp (target_value, &exp);
    man2 = man - 8 * DBL_EPSILON;
    *ret_min = ldexp(man2, exp);
    man2 = man + 8 * DBL_EPSILON;
    *ret_max = ldexp(man2, exp);
}


static int calc_raw_value_for_phys_value_inner(const char *equation, const char *variable_name, int type, double new_phys_value, union BB_VARI *ret_Value, double *ret_phys_value, char **errstring)
{
    double old_raw_value;
    double old_phys_value;
    double new_raw_value;
    double new_raw_value_help;

    double min_diff;
    int dir, x;
    double delta;

    double min;
    double max;
    double range_min;
    double range_max;
    double m;

    if (type == BB_FLOAT) {
        calc_min_max_target_value_float (new_phys_value, &range_min, &range_max);
    } else if (type == BB_DOUBLE) {
        calc_min_max_target_value_double (new_phys_value, &range_min, &range_max);
    } else {
        range_max = DBL_MAX;  //only to avoid warning
        range_min = -DBL_MAX;
    }
    get_datatype_min_max_value (type, &min, &max);

    old_raw_value = 0.0;

    // First approach
    delta = fabs(old_raw_value * 0.01) + 1.0;   // 1% + 1
    if (direct_solve_equation_err_string_replace_var_value (equation, variable_name, old_raw_value, &old_phys_value, errstring) != 0) {
        return -1;
    }
    if (write_bbvari_phys_minmax_check_iteration (equation, variable_name, type, new_phys_value, old_raw_value, old_phys_value, min, max, delta, &new_raw_value, &m, errstring) < 0) {
        return -1;
    }
    // following approach steps (but max. 100)
    for (x = 0; x < 100; x++) {
        double phys;
        if (direct_solve_equation_err_string_replace_var_value (equation, variable_name, new_raw_value, &phys, errstring) != 0) {
            return -1;
        }
        if ((type == BB_FLOAT) ||
            (type == BB_DOUBLE)) {
            if ((phys < range_max) && (phys > range_min)) {
                break;
            }
        } else {
            if (fabs(phys - new_phys_value) <= fabs(m)) {
                break;
            }
        }
        delta = fabs(new_raw_value * 0.01) + 1.0;   // 1% + 1
        if (write_bbvari_phys_minmax_check_iteration (equation, variable_name, type, new_phys_value, new_raw_value, phys, min, max, delta, &new_raw_value, &m, errstring) < 0) {
            return -1;
        }
    }
    if (x == 100) {
        return -2;
    }
    // And now do max. single steps in both directions (but max 4)
    if (write_bbvari_phys_minmax_check_diff (equation, variable_name, new_phys_value, new_raw_value, &min_diff, ret_phys_value, errstring)) {
        return -1;
    }
    if (min_diff != 0.0) {
        new_raw_value_help = new_raw_value;
        for (dir = 0; dir < 2; dir++) {
            for (x = 0; x < 4; x++) {
                double help;
                if (type == BB_FLOAT) {
                    new_raw_value_help = (double)mantissa_plus_or_minus_one_digit_float ((float)new_raw_value_help, dir);
                } else if (type == BB_DOUBLE) {
                    new_raw_value_help = mantissa_plus_or_minus_one_digit_double (new_raw_value_help, dir);
                } else {
                    // Integer data types
                    if (dir) {
                        new_raw_value_help = round(new_raw_value_help) + 1.0;
                        if (new_raw_value_help > max) {
                            break;
                        }
                    } else {
                        new_raw_value_help = round(new_raw_value_help) - 1.0;
                        if (new_raw_value_help < min) {
                            break;
                        }
                    }
                }

                if (write_bbvari_phys_minmax_check_diff (equation, variable_name, new_phys_value, new_raw_value_help, &help, ret_phys_value, errstring)) {
                    return -1;
                }
                if (min_diff < help) {
                    break;   //
                } else {
                    new_raw_value = new_raw_value_help;
                }
            }
        }
    }
    // Not outside the data type value range
    if (new_raw_value > max) {
        new_raw_value = max;
    }
    if (new_raw_value < min) {
        new_raw_value = min;
    }
    ConvertDoubleToUnion(type, new_raw_value, ret_Value);
    return 0;
}


int calc_raw_value_for_phys_value (const char *equation, double phys_value, const char *variable_name, int type, double *ret_raw_value, double *ret_phys_value, char **errstring)
{
    union BB_VARI Value;
    int ret = calc_raw_value_for_phys_value_inner(equation, variable_name, type, phys_value, &Value, ret_phys_value, errstring);
    if (ret == 0) {
        if (bbvari_to_double (type,
                              Value,
                              ret_raw_value) == -1) {
            if (errstring != NULL) *errstring = malloc_message_string ("unknown data type");
            return -1;
        }
    }
    return ret;
}

#ifndef REMOTE_MASTER
int DirectSolveEquationFile (const char *EquationFile, const char *EquationBuffer, char **ErrString)
{
    FILE *fh = NULL;
    char *Buffer;
    char *p;
    char *Line;
    int Size, Pos;
    int c;
    int Ret = 0;

    Size = Pos = 0;
    Buffer = Line = NULL;
    if (EquationBuffer == NULL) {
        fh = open_file(EquationFile, "rt");
        if (fh == NULL) {
            *ErrString = StringMalloc("cannot open file");
            Ret = -1;
            goto __OUT;
        }
        // read the file into the buffer
        while((c = fgetc (fh)) != EOF) {
            if (Size <= (Pos + 1)) {  // + 1 because of tailing termination
                Size += 1024 + Size / 2;
                Buffer = my_realloc(Buffer, Size);
                if (Buffer == NULL) {
                    *ErrString = NULL;
                    Ret = -1;
                    goto __OUT;
                }
            }
            Buffer[Pos] = c;
            Pos++;
        }
        if (Buffer == NULL) {
            *ErrString = StringMalloc("empty file");
            Ret = -1;
            goto __OUT;
        } else {
            Buffer[Pos] = 0;
        }
    } else {
        Buffer = (char*)EquationBuffer;
    }
    Pos = Size = 0;
    p = Buffer;
    do {
        c = *p++;
        while (isspace (c)) c = *p++;
        if (c == ';') {    // it is a comment line
            if (Pos > 0) {
                double Value;
                Line[Pos] = 0;
                // there is something to solve
                if (direct_solve_equation_err_string (Line, &Value, ErrString)) {
                    Ret = -1;
                    goto __OUT;
                }
                Pos = 0;
            }
            // ignore all behind the ; character in this line
            do {
                c = *p++;
            } while ((c != 0) && (c != '\n'));
        } else {
            if (Size <= (Pos + 1)) {  // + 1 because of tailing termination
                Size += 1024 + Size / 2;
                Line = my_realloc(Line, Size);
                if (Line == NULL) {
                    *ErrString = NULL;
                    Ret = -1;
                    goto __OUT;
                }
            }
            Line[Pos] = c;
            Pos++;
        }
    } while (c != 0);
__OUT:
    if (Line != NULL) my_free(Line);
    if ((EquationBuffer == NULL) && (Buffer != NULL)) my_free(Buffer);
    if (fh != NULL) close_file(fh);
    return Ret;
}
#endif

int AddUserDefinedBuildinFunctionToParser(const char *par_FunctionName, int par_MaxParameters, int par_MinParameters, int par_Flags)
{
    int Pos = UserDefinedBuildinFunctionTableSize;
    UserDefinedBuildinFunctionTableSize++;
    UserDefinedBuildinFunctionTable = my_realloc(UserDefinedBuildinFunctionTable, UserDefinedBuildinFunctionTableSize*sizeof(UserDefinedBuildinFunctionTable[0]));
    UserDefinedBuildinFunctionTable[Pos].FunctionName = StringMalloc(par_FunctionName);
    UserDefinedBuildinFunctionTable[Pos].MaxParameters = par_MaxParameters;
    UserDefinedBuildinFunctionTable[Pos].MinParameters = par_MinParameters;
    UserDefinedBuildinFunctionTable[Pos].Flags = par_Flags;
    return Pos + USER_DEFINED_BUILDIN_FUNCTION_OFFSET;
}


// List of all found user defined buildin functions
char **ListOfUserDefinedBuildinFunctionDlls;
int ListOfUserDefinedBuildinFunctionDllsSize;

int AddUserDefinedBuildinFunctionDllToList(char *par_Name)
{
    int Pos = ListOfUserDefinedBuildinFunctionDllsSize;
    ListOfUserDefinedBuildinFunctionDllsSize++;
    ListOfUserDefinedBuildinFunctionDlls = (char**)my_realloc(ListOfUserDefinedBuildinFunctionDlls, sizeof(char*) * ListOfUserDefinedBuildinFunctionDllsSize);
    if(ListOfUserDefinedBuildinFunctionDlls != NULL) {
        ListOfUserDefinedBuildinFunctionDlls[Pos] = StringMalloc(par_Name);
        return 0;
    } else {
        return -1;
    }
}

static int CompareFunction(const void *a, const void *b)
{
    return strcmp((char*)a, (char*)b);
}

static void SortedAndLoadAllFoundUserBuildinFunction(char *par_Folder)
{
    int x;
    qsort(ListOfUserDefinedBuildinFunctionDlls, ListOfUserDefinedBuildinFunctionDllsSize, sizeof(char*), CompareFunction);
    for (x = 0; x < ListOfUserDefinedBuildinFunctionDllsSize; x++) {
        char DllPath[MAX_PATH];
        StringCopyMaxCharTruncate(DllPath, par_Folder, sizeof(DllPath));
#ifdef _WIN32
        STRING_APPEND_TO_ARRAY(DllPath, "\\");
#else
        STRING_APPEND_TO_ARRAY(DllPath, "/");
#endif
        STRING_APPEND_TO_ARRAY(DllPath, ListOfUserDefinedBuildinFunctionDlls[x]);

#ifdef _WIN32
        HMODULE hModulDll = LoadLibraryA (DllPath);
        if (hModulDll == NULL) {
            char *MsgBuf = NULL;
            DWORD dw = GetLastError ();
            FormatMessageA (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL,
                           dw,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPSTR)&MsgBuf,
                           0, NULL);
#else
        void *SharedLibHandle = dlopen(DllPath, RTLD_LAZY); // RTLD_NOW | RTLD_GLOBAL);
        if (SharedLibHandle == NULL) {
            char *MsgBuf = dlerror();
#endif
            ThrowError(1, "cannot load user defined buildin function DLL \"%s\" because \"%s\"", DllPath, MsgBuf);
        } else {
#ifdef _WIN32
            UserDefinedBuildinFunctionGetNameType UserDefinedBuildinFunctionGetName = (UserDefinedBuildinFunctionGetNameType)GetProcAddress(hModulDll, "UserDefinedBuildinFunctionGetName");
            UserDefinedBuildinFunctionExecuteType UserDefinedBuildinFunctionExecute = (UserDefinedBuildinFunctionExecuteType)GetProcAddress(hModulDll, "UserDefinedBuildinFunctionExecute");
#else
            UserDefinedBuildinFunctionGetNameType UserDefinedBuildinFunctionGetName = (UserDefinedBuildinFunctionGetNameType)dlsym(SharedLibHandle, "UserDefinedBuildinFunctionGetName");
            UserDefinedBuildinFunctionExecuteType UserDefinedBuildinFunctionExecute = (UserDefinedBuildinFunctionExecuteType)dlsym(SharedLibHandle, "UserDefinedBuildinFunctionExecute");
#endif
            if (UserDefinedBuildinFunctionGetName == NULL) {
                ThrowError (1, "cannot find function \"UserDefinedBuildinFunctionGetName\" inside DLL \"%s\"", DllPath);
            } else if (UserDefinedBuildinFunctionExecute == NULL) {
                ThrowError (1, "cannot find function \"UserDefinedBuildinFunctionExecute\" inside DLL \"%s\"", DllPath);
            } else {
                int Version, MaxParameters, MinParameters, Flags;
                const char *FunctionName = UserDefinedBuildinFunctionGetName(&Version, &MaxParameters, &MinParameters, &Flags);
                if (Version != USER_DEFINED_BUILDIN_FUNCTION_INTERFACE_VERSION) {
                    ThrowError (1, "Interface version %i of DLL \"%s\" dosen't match to %i", Version, DllPath, USER_DEFINED_BUILDIN_FUNCTION_INTERFACE_VERSION);
                } else {
                    int Number = AddUserDefinedBuildinFunctionToParser(FunctionName, MaxParameters, MinParameters, Flags);
                    AddUserDefinedBuildinFunctionToExecutor(Number, UserDefinedBuildinFunctionExecute);
                }
            }
        }
    }
}


#ifdef _WIN32
static void LoadUserBuildinFunctions(void)
{
    int Ret = 0;
    HMODULE hModulDll;
    char Folder[MAX_PATH];

    Folder[0] = 0;
    if (GetEnvironmentVariableA("XILENV_BUILDINFUNC_DLL_PATH", Folder, MAX_PATH) <= 0) {
        // inside the same path as the XilEnv executable
        GetModuleFileNameA(NULL, Folder, MAX_PATH);
        // remove the executable name
        char *p = Folder;
        while (*p != 0) p++;
        if (p > Folder) {
            do {
                p--;
            } while((p > Folder) && (*p != '\\') && (*p != '/'));
            *p = 0;
        }
        STRING_APPEND_TO_ARRAY(Folder, "\\user_buildin_func");
    }
    char Pattern[MAX_PATH];
    intptr_t Handle;
    struct _finddata_t FindData;
    DWORD Attributes;

    if ((Attributes = GetFileAttributesA(Folder)) == INVALID_FILE_ATTRIBUTES) {
        return;
    }
    if ((Attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
        StringCopyMaxCharTruncate(Pattern, Folder, sizeof(Pattern));
        STRING_APPEND_TO_ARRAY(Pattern, "\\*.DLL");
        if((Handle = _findfirst(Pattern, &FindData)) > 0) {
            do {
                AddUserDefinedBuildinFunctionDllToList(FindData.name);
            } while(_findnext(Handle, &FindData) == 0);
            _findclose(Handle);
        }
    }
    SortedAndLoadAllFoundUserBuildinFunction(Folder);
}
#else
static void LoadUserBuildinFunctions(void)
{
    char Folder[MAX_PATH];

    Folder[0] = 0;
    if (GetEnvironmentVariable("XILENV_BUILDINFUNC_DLL_PATH", Folder, sizeof(Folder)) <= 0) {
        // inside the same path as the XilEnv executable
        GetModuleFileName(NULL, Folder, MAX_PATH);
        // remove the executable name
        char *p = Folder;
        while (*p != 0) p++;
        if (p > Folder) {
            do {
                p--;
            } while((p > Folder) && (*p != '\\') && (*p != '/'));
            *p = 0;
        }
        STRING_APPEND_TO_ARRAY(Folder, "/user_buildin_func");
    }
    struct dirent *dp;
    DIR *Dir = opendir(Folder);
    if (Dir != NULL) {
        while ((dp = readdir(Dir)) != NULL) {
            char *p = dp->d_name;
            while (*p != 0) p++;
            if (((p  - dp->d_name) > 4) &&  // File name ends with ".so"
                !strcmp(p - 3, ".so")) {
                AddUserDefinedBuildinFunctionDllToList(dp->d_name);
            }
        }
        closedir(Dir);
    }
    SortedAndLoadAllFoundUserBuildinFunction(Folder);
}
#endif

int InitEquatuinParser(void)
{
    LoadUserBuildinFunctions();
    return 0;
}
