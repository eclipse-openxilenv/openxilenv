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


#ifndef FILESTACK_H
#define FILESTACK_H

#include "FileCache.h"

class cFileStack {
private:
    struct STACK_ELEM {
        int FileNr;
        cFileCache *FileCache;
        int Ip;
        char Fill[4];
    } *Stack;  
    int SizeOfStack;
    int PosInStack;

    int CheckSizeOf (void)
    {
        if (PosInStack >= SizeOfStack) {
            SizeOfStack += 64 + (SizeOfStack >> 3);
            Stack = (struct STACK_ELEM*)my_realloc (Stack, sizeof (struct STACK_ELEM) * SizeOfStack);
        }
        return ((Stack == NULL) ? -1 : 0);
    }

public:
    cFileStack (void)
    {
        SizeOfStack = 0;
        PosInStack = 0;
        Stack = NULL;
        CheckSizeOf ();
    }

    ~cFileStack (void)
    {
        if (Stack != NULL) my_free (Stack);
        Stack = NULL;
        SizeOfStack = 0;
        PosInStack = 0;
    }

    int IsEmpty (void)
    {
        return (PosInStack == 0);
    }

    int Push (int par_FileNr, cFileCache *par_FileCache, int par_Ip) 
    {
        if (CheckSizeOf ()) return -1;
        Stack[PosInStack].FileNr = par_FileNr;
        Stack[PosInStack].FileCache = par_FileCache;
        Stack[PosInStack].Ip = par_Ip;
        PosInStack++;
        return 0;
    }

    int Pop (int *ret_FileNr, cFileCache **ret_FileCache, int *ret_Ip)
    {
        if (PosInStack <= 0) return -1;
        PosInStack--;
        *ret_FileNr = Stack[PosInStack].FileNr;
        *ret_FileCache = Stack[PosInStack].FileCache;
        *ret_Ip = Stack[PosInStack].Ip;
        return PosInStack;
    }

    int GetNext (int par_Pos, int *ret_FileNr, cFileCache **ret_FileCache, int *ret_Ip)
    {
        if (par_Pos < 0)  par_Pos = PosInStack - 1;
        *ret_FileNr = Stack[par_Pos].FileNr;
        *ret_FileCache = Stack[par_Pos].FileCache;
         *ret_Ip = Stack[PosInStack].Ip;
        return par_Pos - 1;
    }

    int IsInStack (int par_FileNr)
    {
        for (int x = 0; x < PosInStack; x++) {
            if (Stack[x].FileNr == par_FileNr) {
                return 1;
            }
        }
        return 0;
    }

    void Reset (void)
    {
        PosInStack = 0;
    }

};

#endif