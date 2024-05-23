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


#ifndef COMPILERTREE_H
#define COMPILERTREE_H

#include <stdint.h>

class cParser;

// wird beim Uebersetzen verwendet um IF WHILE ATOMIC mit den entsprechenden ENDIF, ENDWHILE, ENDATOMIC
// zu verknuepfen
class cCompilerTree {
private:
    struct TREE_ELEM {
        int Cmd;
        #define COMP_TREE_IF_CMD         1 
        #define COMP_TREE_ELSE_CMD       2 
        #define COMP_TREE_WHILE_CMD      3 
        #define COMP_TREE_ATOMIC_CMD     4 
        #define COMP_TREE_DEF_PROC_CMD   5
        #define COMP_TREE_DEF_LOCALS_CMD 6
        #define COMP_TREE_BREAK          7
        #define COMP_TREE_ELSEIF_CMD     8 
        uint32_t Ip;
        int LineNr;
        uint32_t FileOffset;

        int Next;
        int Prev;
        int FirstBranch;
        int LastBranch;

    } *Tree;
    int SizeOfTree;
    int FirstFreePos;
    //int TopOfBole;
    int TopOfTree;
    int BaseOfTree;

    int StrictAtomic;

    void FreeOne (int par_Pos)
    {
        Tree[par_Pos].Next = FirstFreePos;
        FirstFreePos = par_Pos;
    }

    int AllocOne (void)
    {
        if (FirstFreePos < 0) {   // keine mehr frei 
            int OldSizeOfTree = SizeOfTree;
            SizeOfTree += 64;
            Tree = (struct TREE_ELEM*)my_realloc (Tree, sizeof (struct TREE_ELEM) * (SizeOfTree));
            if (Tree == NULL) return -1;
            for (int x = OldSizeOfTree; x < SizeOfTree; x++) {
                FreeOne (x);
            }
        }
        int Ret = FirstFreePos;
        FirstFreePos = Tree[FirstFreePos].Next;
        return Ret;
    }

    int Add (int par_Cmd, uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset);
    int AddBranch (int ParentPtr, int par_Cmd, uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset);

    int Remove (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, int *ret_Branch);

    int GetNextBranch (int par_Branch, uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser);

public:
    cCompilerTree (void);
    ~cCompilerTree (void);

    int AddIf (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset);
    int AddElse (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset,
                 uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser);
    int AddElseIf (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset,
                   uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser);
    int RemoveEndIf (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser);

    int AddWhile (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset);
    int AddBreak (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset, cParser *par_Parser);
    int RemoveEndWhile (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser);
    int RemoveBreakFromWhile (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser);

    int AddAtomic (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset);
    int RemoveEndAtomic (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser);

    int AddDefProc (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset);
    int RemoveEndDefProc (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser);

    int AddDefLocals (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset);
    int RemoveEndDefLocals (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser);

    void Reset (cParser *par_Parser);

    int EndOfFile (cParser *par_Parser);

    void SetStrictAtomic (int par_StrictAtomic)
    {
        if (par_StrictAtomic) StrictAtomic = 1;
        else StrictAtomic = 0;
    }
};

#endif
