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


#include "Platform.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

extern "C" {
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "ThrowError.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
}

#include "Parser.h"
#include "Stack.h"

#define UNUSED(x) (void)(x)

int cStack::CheckSize (int AddNumOfBytes)
{
    if ((StackPointerToScriptStack + AddNumOfBytes) >= SizeOfScriptStack) {
        if (AddNumOfBytes > 64*1024) {
            SizeOfScriptStack += AddNumOfBytes;  // should not happen
        }
        SizeOfScriptStack += 64*1024;
        ScriptStack = static_cast<char*>(my_realloc (ScriptStack, static_cast<size_t>(SizeOfScriptStack)));
        if (ScriptStack == nullptr) {
            ThrowError (ERROR_CRITICAL, "Out of memory");
            return -1;
        }
    }
    return 0;
}

STACK_ELEM *cStack::AddElemToStack (STACK_ELEM_TYPE Type, const char * const Name)
{
    int LenName, LenStruct;
    STACK_ELEM *Sp, *SpNew;
    int SaveStackPointer;

    if (Name == nullptr) {
        LenName = 0;
    } else {
        LenName = static_cast<int>(strlen (Name)) + 1;
    }
    LenStruct = static_cast<int>(sizeof (STACK_ELEM)) + LenName;

    if (CheckSize (LenStruct)) {
        return nullptr;
    }
    Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack));
    SaveStackPointer = StackPointerToScriptStack;
    StackPointerToScriptStack += Sp->SizeOf;
    SpNew = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack));
    if (LenName) MEMCPY (SpNew->Name, Name, static_cast<size_t>(LenName));
    SpNew->SizeOf = LenStruct;
    SpNew->PosOfPrev = SaveStackPointer;
    SpNew->Type = Type;
    SpNew->Data.Value = 0.0;
    StackChangedCounter++;  // Stack has changed
    return SpNew;
}

int cStack::AddRunReturnToScript (int par_RunReturnToIp, const char *par_Fimename)
{
    STACK_ELEM *Sp = AddElemToStack (SCRIPT_STACK_BACK_TO_FILE, par_Fimename);
    if (Sp == nullptr) return -1;
    Sp->Data.RunReturnToIp = par_RunReturnToIp;
    return 0;
}

int cStack::RemoveRunReturnToScript (cParser *par_Parser, int *ret_RunReturnToIp, char *ret_Fimename)
{
    STACK_ELEM *Sp;

    for (Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack));
         StackPointerToScriptStack >= 0; 
         StackPointerToScriptStack = Sp->PosOfPrev, Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack))) {
        switch (Sp->Type) {
        case SCRIPT_STACK_ENDOF:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot remove return to file from stack because end of stack reached");
            return -1;
        case SCRIPT_STACK_LOCAL_VARIABLE:
        case SCRIPT_STACK_REF_TO_LOCAL_VARIABLE:
            break;
        case SCRIPT_STACK_PROC_CALL:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error: this should never happen RemoveRunReturnToScript() %s (%i)", __FILE__, __LINE__);
            break;
        case SCRIPT_STACK_BACK_TO_FILE:
            *ret_RunReturnToIp = Sp->Data.RunReturnToIp;
            if (ret_Fimename != nullptr) strcpy (ret_Fimename, Sp->Name);
            StackPointerToScriptStack = Sp->PosOfPrev;
            StackChangedCounter++;  // Stack has changed
            return 0;
        case SCRIPT_STACK_GOSUB:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "missing RETURN for GOSUB(%s) at end of script file", Sp->Name);
            break;
        case SCRIPT_STACK_END_OF_BLOCK:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error: this should never happen RemoveRunReturnToScript() %s (%i)", __FILE__, __LINE__);
            return -1;
        }
    }
    par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error: this should never happen RemoveRunReturnToScript() %s (%i)", __FILE__, __LINE__);
    return -1;
}


cStack::cStack (void)
{
    STACK_ELEM *Sp;

    SizeOfScriptStack = 64*1024;
    StackPointerToScriptStack = 0;
    ScriptStack = static_cast<char*>(my_calloc (static_cast<size_t>(SizeOfScriptStack), 1));
    if (ScriptStack != nullptr) {
        // First entry is the "end of the Stack"
        Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack));
        Sp->Type = SCRIPT_STACK_ENDOF;
        Sp->SizeOf = sizeof (STACK_ELEM);
        Sp->PosOfPrev = -1; // doesn't exist!
        AddElemToStack (SCRIPT_STACK_ENDOF, "EndOfStack");
    }
    StackChangedCounter = 0;
}

cStack::~cStack (void)
{
    if (ScriptStack != nullptr) my_free (ScriptStack);
    ScriptStack = nullptr;
}

// GOSUB
int cStack::AddGosubToStack (char *par_GosubName, int par_Ip, int par_AtomicDepth)
{
    STACK_ELEM *Sp;

    if ((Sp = AddElemToStack (SCRIPT_STACK_GOSUB, par_GosubName)) == nullptr) {
        return -1;
    }
    Sp->Data.RetFromGosub.Ip = par_Ip;
    Sp->Data.RetFromGosub.AtomicDepth = par_AtomicDepth;
    return 0;
}

int cStack::RemoveGosubFromStack (cParser *par_Parser, int *ret_ReturnToIp, int *ret_ReturnToAtomicDepth)
{
    STACK_ELEM *Sp;

    for (Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack));
         StackPointerToScriptStack >= 0; 
         StackPointerToScriptStack = Sp->PosOfPrev, Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack))) {
        switch (Sp->Type) {
        case SCRIPT_STACK_ENDOF:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot RETURN to GOSUB because not called by GOSUB");
            return -1;
        case SCRIPT_STACK_REF_TO_LOCAL_VARIABLE:
        case SCRIPT_STACK_LOCAL_VARIABLE:
            break;
        case SCRIPT_STACK_PROC_CALL:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot RETURN to GOSUB because not called by GOSUB");
            return -1;
        case SCRIPT_STACK_GOSUB:
            *ret_ReturnToIp = Sp->Data.RetFromGosub.Ip;
            *ret_ReturnToAtomicDepth = Sp->Data.RetFromGosub.AtomicDepth;
            StackPointerToScriptStack = Sp->PosOfPrev;
            StackChangedCounter++;  // Stack has changed
            return 0;
        case SCRIPT_STACK_BACK_TO_FILE:
        case SCRIPT_STACK_END_OF_BLOCK:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot RETURN to GOSUB because not called by GOSUB");
            return -1;
        }
    }
    par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error: this should never happen RemoveProcFromStack() %s (%i)", __FILE__, __LINE__);
    return -1;
}


int cStack::AddBlockToStack (void)
{
    if (AddElemToStack (SCRIPT_STACK_END_OF_BLOCK, nullptr) == nullptr) {
        return -1;
    }
    return 0;
}

int cStack::AddLocalVariableToStack (char *par_Name, double par_Value)
{
    STACK_ELEM *Sp;

    if ((Sp = AddElemToStack (SCRIPT_STACK_LOCAL_VARIABLE, par_Name)) == nullptr) {
        return -1;
    }
    Sp->Data.Value = par_Value;
    return 0;
}


int cStack::AddProcToStack (int par_ProcIdx, int par_Ip, char *par_ProcName)
{
    STACK_ELEM *Sp;

    // Put on the stack the return jump address
    if ((Sp = AddElemToStack (SCRIPT_STACK_PROC_CALL, par_ProcName)) == nullptr) {
        return -1;
    }
    Sp->Data.RetFromProc.RetToProcIdx = par_ProcIdx;
    Sp->Data.RetFromProc.RetToIp = par_Ip;
    return 0;
}

int cStack::AddRefToLocalVariableToStack (char *par_Name, int64_t par_Ref)
{
    STACK_ELEM *Sp;

    if ((Sp = AddElemToStack (SCRIPT_STACK_REF_TO_LOCAL_VARIABLE, par_Name)) == nullptr) {
        return -1;
    }
    Sp->Data.Ref = par_Ref;
    return 0;
}

void cStack::DebugPrintStackToFile (char *Filename)
{
    int Pos;
    STACK_ELEM *Sp;
    FILE *fh;

    fh = fopen (Filename, "wt");
    if (fh != nullptr) {
        Pos = StackPointerToScriptStack;
        for (Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + Pos));
            Pos >= 0; 
            Pos = Sp->PosOfPrev, Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + Pos))) {
            switch (Sp->Type) {
            case SCRIPT_STACK_ENDOF:
                fprintf (fh, "SCRIPT_STACK_ENDOF \"%s\"\n", Sp->Name);
                goto __EXIT;
            case SCRIPT_STACK_LOCAL_VARIABLE:
                fprintf (fh, "SCRIPT_STACK_LOCAL_VARIABLE Name = \"%s\" = %f\n", Sp->Name, Sp->Data.Value);
                break;
            case SCRIPT_STACK_REF_TO_LOCAL_VARIABLE:
                fprintf (fh, "SCRIPT_STACK_REF_TO_LOCAL_VARIABLE RefName = \"%s\" poits to %" PRIi64 "\n", Sp->Name, Sp->Data.Ref);
                break;
            case SCRIPT_STACK_PROC_CALL:
                fprintf (fh, "SCRIPT_STACK_PROC_CALL Return to proc = \"%s\" ProcIdx %i\n", 
                         Sp->Name, Sp->Data.RetFromProc.RetToProcIdx);
                break;
            case SCRIPT_STACK_BACK_TO_FILE:
                fprintf (fh, "SCRIPT_STACK_BACK_TO_FILE Return to file = \"%s\"\n", Sp->Name);
                break;
            case SCRIPT_STACK_END_OF_BLOCK:
                fprintf (fh, "SCRIPT_STACK_END_OF_BLOCK\n");
                break;
            case SCRIPT_STACK_GOSUB:
                fprintf (fh, "SCRIPT_STACK_GOSUB to \"%s\" Ip = %i, AtomicDepth = %i\n", Sp->Name, Sp->Data.RetFromGosub.Ip, Sp->Data.RetFromGosub.AtomicDepth);
                break;
            }
        }
        __EXIT:
        fclose (fh);
    }
}


int cStack::PrintOneStackToString (int par_Index, char *par_String, int par_MaxChar)
{
    UNUSED(par_MaxChar);
    int x;
    int Pos;
    STACK_ELEM *Sp;

    Pos = StackPointerToScriptStack;
    for (Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + Pos)), x = 0;
        Pos >= 0;
        Pos = Sp->PosOfPrev, Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + Pos)), x++) {
        if (x == par_Index) {
            switch (Sp->Type) {
            case SCRIPT_STACK_ENDOF:
                goto END_OF_STACK;
            case SCRIPT_STACK_LOCAL_VARIABLE:
                sprintf (par_String, "SCRIPT_STACK_LOCAL_VARIABLE Name = \"%s\" = %f", Sp->Name, Sp->Data.Value);
                break;
            case SCRIPT_STACK_REF_TO_LOCAL_VARIABLE:
                if ((Sp->Data.Ref >> 32) == 1) {   // ist eine VID
                    sprintf (par_String, "SCRIPT_STACK_REF_TO_LOCAL_VARIABLE RefName = \"%s\" poits to (VID) %i", Sp->Name, static_cast<int>(Sp->Data.Ref));
                } else {
                    sprintf (par_String, "SCRIPT_STACK_REF_TO_LOCAL_VARIABLE RefName = \"%s\" poits to %i", Sp->Name, static_cast<int>(Sp->Data.Ref));
                }
                break;
            case SCRIPT_STACK_PROC_CALL:
                sprintf (par_String, "SCRIPT_STACK_PROC_CALL Return to proc = \"%s\" ProcIdx %i",
                         Sp->Name, Sp->Data.RetFromProc.RetToProcIdx);
                break;
            case SCRIPT_STACK_BACK_TO_FILE:
                sprintf (par_String, "SCRIPT_STACK_BACK_TO_FILE Return to file = \"%s\"", Sp->Name);
                break;
            case SCRIPT_STACK_END_OF_BLOCK:
                sprintf (par_String, "SCRIPT_STACK_END_OF_BLOCK");
                break;
            case SCRIPT_STACK_GOSUB:
                sprintf (par_String, "SCRIPT_STACK_GOSUB to \"%s\" Ip = %i, AtomicDepth = %i", Sp->Name, Sp->Data.RetFromGosub.Ip, Sp->Data.RetFromGosub.AtomicDepth);
                break;
            }
            return 1;
        }
    }
END_OF_STACK:
    return 0;
}

int cStack::RemoveLocalVariablesFromStack (cParser *par_Parser)
{
    STACK_ELEM *Sp;

    for (Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack));
         StackPointerToScriptStack >= 0; 
         StackPointerToScriptStack = Sp->PosOfPrev, Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack))) {
        switch (Sp->Type) {
        case SCRIPT_STACK_ENDOF:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot remove local variables from stack because end of stack reached");
            return -1;
        case SCRIPT_STACK_LOCAL_VARIABLE:
        case SCRIPT_STACK_REF_TO_LOCAL_VARIABLE:
            break;
        case SCRIPT_STACK_PROC_CALL:
        case SCRIPT_STACK_BACK_TO_FILE:
        case SCRIPT_STACK_GOSUB:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error: this should never happen RemoveLocalVariablesFromStack() %s (%i)", __FILE__, __LINE__);
            break;
        case SCRIPT_STACK_END_OF_BLOCK:
            StackPointerToScriptStack = Sp->PosOfPrev;
            StackChangedCounter++;  // Stack has changed
            return 0;
        }
    }
    par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error: this should never happen RemoveLocalVariablesFromStack() %s (%i)", __FILE__, __LINE__);
    return -1;
}

int cStack::RemoveProcFromStack (cParser *par_Parser, int *ret_ReturnToProcIdx, int *ret_ReturnToIp)
{
    STACK_ELEM *Sp;

    for (Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack));
         StackPointerToScriptStack >= 0; 
         StackPointerToScriptStack = Sp->PosOfPrev, Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack))) {
        switch (Sp->Type) {
        case SCRIPT_STACK_ENDOF:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "cannot remove local variables from stack because end of stack reached");
            return -1;
        case SCRIPT_STACK_REF_TO_LOCAL_VARIABLE:
        case SCRIPT_STACK_LOCAL_VARIABLE:
            break;
        case SCRIPT_STACK_PROC_CALL:
            *ret_ReturnToProcIdx = Sp->Data.RetFromProc.RetToProcIdx;
            *ret_ReturnToIp = Sp->Data.RetFromProc.RetToIp;
            StackPointerToScriptStack = Sp->PosOfPrev;
            StackChangedCounter++;  // Stack has changed
            return 0;
        case SCRIPT_STACK_GOSUB:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "there was a gosub without a return");
            return -1;
        case SCRIPT_STACK_BACK_TO_FILE:
        case SCRIPT_STACK_END_OF_BLOCK:
            par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error: this should never happen RemoveProcFromStack() %s (%i)", __FILE__, __LINE__);
            return -1;
        }
    }
    par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error: this should never happen RemoveProcFromStack() %s (%i)", __FILE__, __LINE__);
    return -1;
}

int cStack::GetLocalVariableValue (char *Name, double *pRet, int StackPos)
{
    STACK_ELEM *Sp;

    if (StackPos < 0) {
        StackPos = StackPointerToScriptStack;
    }
    if (StackPos == 0) return -1;
    for (Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPos));
         Sp->PosOfPrev >= 0; 
         Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + Sp->PosOfPrev))) {
        switch (Sp->Type) {
        case SCRIPT_STACK_LOCAL_VARIABLE:
            if (!strcmp (Sp->Name, Name)) {
                *pRet = Sp->Data.Value;
                return 0;
            }
            break;
        case SCRIPT_STACK_ENDOF:
        case SCRIPT_STACK_PROC_CALL:
        case SCRIPT_STACK_BACK_TO_FILE:
            // Locale variable not found
            return -1;
        case SCRIPT_STACK_REF_TO_LOCAL_VARIABLE:
            break;
        case SCRIPT_STACK_END_OF_BLOCK:
            break;
        case SCRIPT_STACK_GOSUB:
            break;
        }
    }
    //printf ("This should never reached  GetLocalVariableValue()");
    return -1;
}

int cStack::GetValueOfRefToLocalVariable (char *RefName, double *pRet, int StackPos, int Cmd)
{
    int Ref;
    STACK_ELEM *Sp;

    if (StackPos < 0) {
        StackPos = StackPointerToScriptStack;
    }
    for (Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPos));
         Sp->PosOfPrev >= 0; 
         Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + Sp->PosOfPrev))) {
        switch (Sp->Type) {
        case SCRIPT_STACK_LOCAL_VARIABLE:
            break;
        case SCRIPT_STACK_ENDOF:
        case SCRIPT_STACK_PROC_CALL:
        case SCRIPT_STACK_BACK_TO_FILE:
            // Locale variable not found
            return -1;
        case SCRIPT_STACK_REF_TO_LOCAL_VARIABLE:
            if (!strcmp (Sp->Name + 1, RefName)) {
                Ref = static_cast<int>(Sp->Data.Ref);
                if ((Sp->Data.Ref >> 32) == 1) {
                    union FloatOrInt64 Value;
                    int ByteWidth;
                    int Type = read_bbvari_by_id(Ref, &Value, &ByteWidth, Cmd);
                    if (Type == FLOAT_OR_INT_64_TYPE_INVALID) {
                        return -1;
                    } else {
                        *pRet = to_double_FloatOrInt64(Value, Type);
                    }
                } else {
                    Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + Ref));
                    *pRet = Sp->Data.Value;
                }
                return 0;
            }
            break;
        case SCRIPT_STACK_END_OF_BLOCK:
            break;
        case SCRIPT_STACK_GOSUB:
            break;
        }
    }
    //printf ("This should never reached GetValueOfRefToLocalVariable()");
    return -1;
}

int64_t cStack::GetRefToLocalVariable (char *RefName, int StackPos)
{
    STACK_ELEM *Sp;

    if (StackPos < 0) {
        StackPos = StackPointerToScriptStack;
    }
    for (Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPos));
         Sp->PosOfPrev >= 0; 
         Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + Sp->PosOfPrev))) {
        switch (Sp->Type) {
        case SCRIPT_STACK_LOCAL_VARIABLE:
            if (!strcmp (Sp->Name, RefName + 1)) {
                return reinterpret_cast<char*>(Sp) - ScriptStack;
            }
            break;
        case SCRIPT_STACK_ENDOF:
        case SCRIPT_STACK_PROC_CALL:
        case SCRIPT_STACK_BACK_TO_FILE:
            // Locale variable not found
            return -1;
        case SCRIPT_STACK_REF_TO_LOCAL_VARIABLE:
            if (!strcmp (Sp->Name + 1, RefName + 1)) {
                return Sp->Data.Ref;
            }
            break;
        case SCRIPT_STACK_END_OF_BLOCK:
            break;
        case SCRIPT_STACK_GOSUB:
            break;
        }
    }
    //printf ("This should never reached  GetRefToLocalVariable()");
    return -1;
}



int cStack::SetLocalVariableValue (char *Name, double Value)
{
    STACK_ELEM *Sp;

    if (StackPointerToScriptStack == 0) return -1;
    for (Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack));
         Sp->PosOfPrev >= 0; 
         Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + Sp->PosOfPrev))) {
        switch (Sp->Type) {
        case SCRIPT_STACK_LOCAL_VARIABLE:
            if (!strcmp (Sp->Name, Name)) {
                Sp->Data.Value = Value;
                StackChangedCounter++;  // Stack has changed
                return 0;
            }
            break;
        case SCRIPT_STACK_ENDOF:
        case SCRIPT_STACK_PROC_CALL:
        case SCRIPT_STACK_BACK_TO_FILE:
            // Locale variable not found
            return -1;
        case SCRIPT_STACK_REF_TO_LOCAL_VARIABLE:
            break;
        case SCRIPT_STACK_END_OF_BLOCK:
            break;
        case SCRIPT_STACK_GOSUB:
            break;
        }
    }
    //printf ("This should never reached  SetLocalVariableValue()");
    return -1;
}

int cStack::SetValueOfLocalVariableRefTo (char *RefName, double Value, int Cmd)
{
    int Ref;
    STACK_ELEM *Sp;

    for (Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack));
         Sp->PosOfPrev >= 0; 
         Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + Sp->PosOfPrev))) {
        switch (Sp->Type) {
        case SCRIPT_STACK_LOCAL_VARIABLE:
            break;
        case SCRIPT_STACK_ENDOF:
        case SCRIPT_STACK_PROC_CALL:
        case SCRIPT_STACK_BACK_TO_FILE:
            // Locale variable not found
            return -1;
        case SCRIPT_STACK_REF_TO_LOCAL_VARIABLE:
            if (!strcmp (Sp->Name + 1, RefName)) {
                Ref = static_cast<int>(Sp->Data.Ref);
                if ((Sp->Data.Ref >> 32) == 1) {
                    union FloatOrInt64 v;
                    v.d = Value;
                    write_bbvari_by_id(Ref, v, FLOAT_OR_INT_64_TYPE_F64,BB_DOUBLE, 0, Cmd);
                } else {
                    Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + Ref));
                    Sp->Data.Value = Value;
                }
                StackChangedCounter++;  // Stack has changed
                return 0;
            }
            break;
        case SCRIPT_STACK_END_OF_BLOCK:
            break;
        case SCRIPT_STACK_GOSUB:
            break;
        }
    }
    //printf ("This should never reached SetValueOfLocalVariableRefTo()");
    return -1;
}

int cStack::GetFirstReturnIp (void)
{
    STACK_ELEM *Sp;

    for (Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + StackPointerToScriptStack));
         Sp->PosOfPrev >= 0; 
         Sp = static_cast<STACK_ELEM*>(static_cast<void*>(ScriptStack + Sp->PosOfPrev))) {
        switch (Sp->Type) {
        case SCRIPT_STACK_ENDOF:
            return -1;    // not found
        case SCRIPT_STACK_LOCAL_VARIABLE:
            break;
        case SCRIPT_STACK_REF_TO_LOCAL_VARIABLE:
            break;
        case SCRIPT_STACK_PROC_CALL:
            return Sp->Data.RetFromProc.RetToIp;
        case SCRIPT_STACK_BACK_TO_FILE:
            return Sp->Data.RunReturnToIp + 1;
        case SCRIPT_STACK_GOSUB:
            return Sp->Data.RetFromGosub.Ip;
        default:
            break;
        }
    }
    return -1;
}

