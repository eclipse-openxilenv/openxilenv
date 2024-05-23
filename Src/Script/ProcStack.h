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


#ifndef PROCSTACK_H
#define PROCSTACK_H

#include "Proc.h"

class cProcStack {
private:
    struct STACK_ELEM {
        cProc *Proc;
    } *Stack;  
    unsigned long SizeOfStack;
    unsigned long PosInStack;

    int CheckSizeOf (void)
    {
        if (PosInStack >= SizeOfStack) {
            SizeOfStack += 64 + (SizeOfStack >> 3);
            Stack = (struct STACK_ELEM*)my_realloc (Stack, sizeof (struct STACK_ELEM) * SizeOfStack);
        }
        return ((Stack == NULL) ? -1 : 0);
    }

public:
    cProcStack (void)
    {
        SizeOfStack = 0;
        PosInStack = 0;
        Stack = NULL;
        CheckSizeOf ();
    }

    ~cProcStack (void)
    {
        if (Stack != NULL) my_free (Stack);
        Stack = NULL;
        SizeOfStack = 0;
        PosInStack = 0;
    }

    int Push (cProc *par_Proc)
    {
        if (CheckSizeOf ()) return -1;
        Stack[PosInStack].Proc = par_Proc;
        PosInStack++;
        return 0;
    }

    int Pop (cProc **ret_Proc)
    {
        if (PosInStack <= 0) return -1;
        PosInStack--;
        *ret_Proc = Stack[PosInStack].Proc;
        return PosInStack;
    }

    int GetNext (int par_Pos, cProc **ret_Proc)
    {
        if (par_Pos < 0)  par_Pos = PosInStack - 1;
        *ret_Proc = Stack[par_Pos].Proc;
        return par_Pos - 1;
    }

    void Reset (void)
    {
        PosInStack = 0;
    }

};

#endif