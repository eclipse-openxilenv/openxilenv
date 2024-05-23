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
#include <string.h>


extern "C" {
#include "MyMemory.h"
}
#include "Executor.h"
#include "Parser.h"
#include "DefLocalsTree.h"


cDefLocalsTree::cDefLocalsTree (void)
{
    Tree = nullptr;
    MaxSizeOfTree = 0;

    PosLocalsTreeLast = -1;
    PosLocalsTreeStart = -1;
    LocalsTreeDepth = 0;

    PointerToDefLocalsTree = 0;
}

cDefLocalsTree::~cDefLocalsTree (void)
{
    if (Tree != nullptr) my_free (Tree);
}


int cDefLocalsTree::CheckTreeSize (void)
{
    if (MaxSizeOfTree >= PointerToDefLocalsTree) {
        MaxSizeOfTree += 2;
        Tree = static_cast<DEF_LOCALS_TREE_ELEM*>(my_realloc (Tree, sizeof (DEF_LOCALS_TREE_ELEM) * static_cast<size_t>(MaxSizeOfTree)));
    }
    return ((Tree == nullptr) ? -1 : 0);
}



int cDefLocalsTree::AddDefLocals (int Ip)
{
    DEF_LOCALS_TREE_ELEM *TpNew, *TpPrev, *TpParent;
    int PosToParent;
    int PosToNew;

    if (CheckTreeSize ()) return -2;

    if (LocalsTreeDepth) {
        // there exist one parent
        PosToParent = PosLocalsTreeLast;
        TpParent = &Tree[PosToParent];
        if (TpParent->PosToLastChild < 0) {  // there don't exist a previous
            TpPrev = nullptr;
        } else {
            TpPrev = &Tree[TpParent->PosToLastChild];
        }
    } else {
        PosToParent = -1;
        TpParent = nullptr; // no parent
        if (PosLocalsTreeStart < 0) {
            // This is the first entry
            PosLocalsTreeStart = PointerToDefLocalsTree;
            TpPrev = nullptr;
        } else {
            if (PosLocalsTreeLast < 0) {
                TpPrev = &Tree[PosLocalsTreeStart];
            } else {
                TpPrev = &Tree[PosLocalsTreeLast];
            }
        }
    }
    // add new tree element
    PosToNew = PointerToDefLocalsTree;
    PosLocalsTreeLast = PosToNew;
    TpNew = &Tree[PosToNew];
    PointerToDefLocalsTree++;
    
    // add new element to the tree
    TpNew->PosToNext = -1;  // End of the list
    TpNew->PosToFirstChild = -1;  // No childs
    TpNew->PosToLastChild = -1;
    TpNew->PosToLastSubChild = -1;
    TpNew->PosToParent = PosToParent;
    if (TpParent != nullptr) {
        TpParent->PosToLastChild = PosToNew;
        if (TpParent->PosToFirstChild < 0) {
            TpParent->PosToFirstChild = PosToNew;
        }
    }
    if (TpPrev != nullptr) {
        TpPrev->PosToNext = PosToNew;
    }
    TpNew->IpStart = Ip;

    LocalsTreeDepth++;
    
    return PosToNew;
}

int cDefLocalsTree::EndDefLocals (int Ip)
{
    DEF_LOCALS_TREE_ELEM *Tp, *TpParent;

    if (PosLocalsTreeLast < 0) return -1;
    // Current DEF_LOCALS block
    Tp = &Tree[PosLocalsTreeLast];
    Tp->IpEnd = Ip;

    if (Tp->PosToParent >= 0) {
        TpParent = &Tree[Tp->PosToParent];
        if (Tp->PosToLastSubChild < 0) { // Has no own Childs
            TpParent->PosToLastSubChild = PosLocalsTreeLast;
        } else {
            TpParent->PosToLastSubChild = Tp->PosToLastSubChild;
        }
    }

    PosLocalsTreeLast = Tp->PosToParent;
    LocalsTreeDepth--;

    return 0;
}



int cDefLocalsTree::AddLocalVariablesGotoInsideABlock (DEF_LOCALS_TREE_ELEM *Tp,
                                                       cParser *par_Parser, cExecutor *par_Executor)
{
    int IpSave = par_Executor->GetCurrentIp ();
    // Parse DEF_LOCALS command and execute
    par_Executor->SetNextIp (Tp->IpStart);
    int Ret = par_Executor->ExecuteNextCmd (par_Parser);
    par_Executor->SetNextIp (IpSave);

    return Ret;
}


int cDefLocalsTree::RemoveLocalVariablesFromStack (cParser *par_Parser, cExecutor *par_Executor)
{
    return par_Executor->Stack.RemoveLocalVariablesFromStack (par_Parser);
}


int cDefLocalsTree::GoUpToChildFrom (int PosToFirstChild, int GotoPosInTree,
                                     cParser *par_Parser, cExecutor *par_Executor)
{
    int Pos;
    DEF_LOCALS_TREE_ELEM *TpChild;

    for (Pos = PosToFirstChild, TpChild = &Tree[Pos];
        Pos >= 0; 
        Pos = TpChild->PosToNext, TpChild = &Tree[Pos]) {
        if (Pos == GotoPosInTree) {
            if (AddLocalVariablesGotoInsideABlock (TpChild, par_Parser, par_Executor)) return -1;
            LocalTreePointer = Pos;
            return 0;   // Target reached
        }
        if ((GotoPosInTree >= TpChild->PosToFirstChild) && (GotoPosInTree <= TpChild->PosToLastSubChild)) {
            if (AddLocalVariablesGotoInsideABlock (TpChild, par_Parser, par_Executor)) return -1;
            return GoUpToChildFrom (TpChild->PosToFirstChild, GotoPosInTree, par_Parser, par_Executor);
        }
    }
    par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error: this should never happend GoUpToChildFrom() %s (%i)", __FILE__, __LINE__);
    return -1;
}


int cDefLocalsTree::GoToParentOf (DEF_LOCALS_TREE_ELEM *Tp, int GotoPosInTree,
                                  cParser *par_Parser, cExecutor *par_Executor)
{
    int Pos;
    DEF_LOCALS_TREE_ELEM *TpNext;

    if (Tp->PosToParent < 0) {
        par_Parser->Error (SCRIPT_PARSER_FATAL_ERROR, "internal error: this should never happend GoToParentOf() %s (%i)", __FILE__, __LINE__);
        return -1;
    }

    if (RemoveLocalVariablesFromStack (par_Parser, par_Executor)) return -1;

    for (Pos = Tp->PosToParent, TpNext = &Tree[Pos];
        Pos >= 0; 
        Pos = TpNext->PosToNext, TpNext = &Tree[Pos]) {
        if (Pos == GotoPosInTree) {
            LocalTreePointer = Pos;
            return 0;   // Target reached
        }
        if ((GotoPosInTree >= TpNext->PosToFirstChild) && (GotoPosInTree <= TpNext->PosToLastSubChild)) {
            return GoUpToChildFrom (TpNext->PosToFirstChild, GotoPosInTree, par_Parser, par_Executor);
        }
    }
    return GoToParentOf (&Tree[Tp->PosToParent], GotoPosInTree, par_Parser, par_Executor);
}

int cDefLocalsTree::GotoDefLocals (int GotoPosInTree, cParser *par_Parser, cExecutor *par_Executor)
{
    DEF_LOCALS_TREE_ELEM *Tp;

    if (LocalTreePointer == GotoPosInTree) {
        // inside same block -> do nothing
        return 0;  
    }
    if (LocalTreePointer < 0) {  
        // start at the bottom
        return GoUpToChildFrom (PosLocalsTreeStart, GotoPosInTree, par_Parser, par_Executor); 
    } else {
        Tp = &Tree[LocalTreePointer];
        if ((GotoPosInTree >= Tp->PosToFirstChild) && (GotoPosInTree <= Tp->PosToLastSubChild)) {
            // It is a sub block of the current blocks
            return GoUpToChildFrom (Tp->PosToFirstChild, GotoPosInTree, par_Parser, par_Executor);
        }
        return GoToParentOf (Tp, GotoPosInTree, par_Parser, par_Executor);
    }
}
