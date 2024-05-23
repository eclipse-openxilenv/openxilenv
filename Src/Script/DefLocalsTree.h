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


#ifndef DEFLOCALSTREE_H
#define DEFLOCALSTREE_H


// DEF_LOCALS tree is necessary to cleanup the stack for a GOTOcommand

class cParser;
class cExecutor;

// Remark: main script procedure have the name "" and must exist
class cDefLocalsTree {
private:
    typedef struct {
        int PosToNext;
        int PosToFirstChild;
        int PosToLastChild;
        int PosToLastSubChild;
        int PosToParent;
        int IpStart;
        int IpEnd;
        char Filler[4];
    } DEF_LOCALS_TREE_ELEM;  // 7*4 + 4 = 32

    DEF_LOCALS_TREE_ELEM *Tree;
    int MaxSizeOfTree;

    int PosLocalsTreeLast;
    int PosLocalsTreeStart;
    int LocalsTreeDepth;

    int TreeDepth;  
    int PointerToDefLocalsTree;

    int LocalTreePointer;  // This have to be set to -1  if a procedure are called

    int CheckTreeSize (void);
    int AddLocalVariablesGotoInsideABlock (DEF_LOCALS_TREE_ELEM *Tp,
                                           cParser *par_Parser, cExecutor *par_Executor);
    int RemoveLocalVariablesFromStack (cParser *par_Parser, cExecutor *par_Executor);
    int GoUpToChildFrom (int PosToFirstChild, int GotoPosInTree,
                         cParser *par_Parser, cExecutor *par_Executor);
    int GoToParentOf (DEF_LOCALS_TREE_ELEM *Tp, int GotoPosInTree,
                      cParser *par_Parser, cExecutor *par_Executor);
    int GotoDefLocals (int GotoPosInTree, cParser *par_Parser, cExecutor *par_Executor);

public:
    cDefLocalsTree (void);
    ~cDefLocalsTree (void);

    int AddDefLocals (int Ip);
    int EndDefLocals (int Ip);

    void EnterProc (void)
    {
        LocalTreePointer = -1;
    }

    int GotoFromToDefLocalsPos (int par_FromPos, int par_ToPos, cParser *par_Parser, cExecutor *par_Executor)
    {
        LocalTreePointer = par_FromPos;
        return GotoDefLocals (par_ToPos, par_Parser, par_Executor);
    }
};

#endif
