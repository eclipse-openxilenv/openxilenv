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


#ifndef STACH_H
#define STACH_H

#include <stdint.h>

class cParser;

typedef enum {
    SCRIPT_STACK_ENDOF = 0,
    SCRIPT_STACK_LOCAL_VARIABLE = 1,
    SCRIPT_STACK_REF_TO_LOCAL_VARIABLE = 2,
    //SCRIPT_STACK_REF_TO_BB_VARIABLE = 3,
    SCRIPT_STACK_END_OF_BLOCK = 3,
    SCRIPT_STACK_PROC_CALL = 4,
    SCRIPT_STACK_GOSUB = 5,
    SCRIPT_STACK_BACK_TO_FILE = 6
} STACK_ELEM_TYPE;

typedef struct {
    int SizeOf;
    int PosOfPrev;
    STACK_ELEM_TYPE Type;
    union {
        double Value;
        int64_t Ref;
        struct {
            int RetToProcIdx;
            int RetToIp;
        } RetFromProc;
        int RunReturnToIp;
        struct {
            int Ip;
            int AtomicDepth;
        } RetFromGosub;
    } Data;
    char Name[1];   // ist eigentlch ein Array mit flexibler Laenge
} STACK_ELEM;


class cStack {
private:

    char *ScriptStack;
    int SizeOfScriptStack;
    int StackPointerToScriptStack;

    int CheckSize (int AddNumOfBytes);
    STACK_ELEM *AddElemToStack (STACK_ELEM_TYPE Type, const char * const Name);

    int StackChangedCounter;
public:
    cStack (void);

    ~cStack (void);

    int AddRunReturnToScript (int par_RunReturnToIp, const char *Fimename);
    int RemoveRunReturnToScript (cParser *par_Parser, int *ret_RunReturnToIp, char *ret_Fimename, int par_Maxc);

    // DEL_LOCALS
    int AddBlockToStack (void);
    int AddLocalVariableToStack (char *par_Name, double par_Value);
    int AddRefToLocalVariableToStack (char *par_Name, int64_t par_Ref);
    //int AddRefBBVariableToStack (char *par_Name, int par_Vid);
    int RemoveLocalVariablesFromStack (cParser *par_Parser);

    // CALL
    int AddProcToStack (int par_ProcIdx, int par_Ip, char *par_ProcName);
    int RemoveProcFromStack (cParser *par_Parser, int *ret_ReturnToProcIdx, int *ret_ReturnToIp);

    // GOSUB
    int AddGosubToStack (char *par_GosubName, int par_Ip, int par_AtomicDepth);
    int RemoveGosubFromStack (cParser *par_Parser, int *ret_ReturnToIp, int *ret_ReturnToAtomicDepth);


    void DebugPrintStackToFile (char *Filename);

    int GetLocalVariableValue (char *Name, double *pRet, int StackPos);

    int GetValueOfRefToLocalVariable (char *RefName, double *pRet, int StackPos, int Cmd);

    int64_t GetRefToLocalVariable (char *RefName, int StackPos);

    int SetLocalVariableValue (char *Name, double Value);

    int SetValueOfLocalVariableRefTo (char *RefName, double Value, int Cmd);

    int PrintOneStackToString (int par_Index, char *par_String, int par_MaxChar);

    int GetCurrentStackPos (void)
    {
        return StackPointerToScriptStack;
    }
    void Reset (void)
    {
        StackPointerToScriptStack = 0;
    }

    int GetFirstReturnIp (void);

    int GetStackChangedCounter(void)
    {
        return StackChangedCounter;
    }


};

#endif
