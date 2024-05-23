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
#include "Parser.h"
#include "CompilerTree.h"

cCompilerTree::cCompilerTree (void)
{
    Tree = nullptr;
    SizeOfTree = 0;
    FirstFreePos = -1;
    BaseOfTree = TopOfTree = -1;
    StrictAtomic = 0;   // Only Warning if ATOMIC and END_ATOMIC don't match
}

cCompilerTree::~cCompilerTree (void)
{
    if (Tree != nullptr) my_free (Tree);
}


int cCompilerTree::Add (int par_Cmd, uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset)
{
    int TreePtr = AllocOne ();
    if (TreePtr < 0) return -2;
    Tree[TreePtr].Cmd = par_Cmd;
    Tree[TreePtr].Ip = par_Ip;
    Tree[TreePtr].LineNr = par_LineNr;
    Tree[TreePtr].FileOffset = par_FileOffset;
    Tree[TreePtr].Next = -1;
    Tree[TreePtr].FirstBranch = -1;
    Tree[TreePtr].LastBranch = -1;
    Tree[TreePtr].Prev = TopOfTree;
    if (TopOfTree < 0) {
        BaseOfTree = TreePtr;
    } else {
        Tree[TopOfTree].Next = TreePtr;
    }
    TopOfTree = TreePtr;
    return 0;
}

int cCompilerTree::AddBranch (int ParentPtr, int par_Cmd, uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset)
{
    int TreePtr = AllocOne ();
    if (TreePtr < 0) return -2;
    Tree[TreePtr].Cmd = par_Cmd;
    Tree[TreePtr].Ip = par_Ip;
    Tree[TreePtr].LineNr = par_LineNr;
    Tree[TreePtr].FileOffset = par_FileOffset;
    Tree[TreePtr].Next = -1;
    Tree[TreePtr].Prev = Tree[ParentPtr].LastBranch;
    if (Tree[ParentPtr].LastBranch != -1) {
        Tree[Tree[ParentPtr].LastBranch].Next = TreePtr;
    }
    Tree[ParentPtr].LastBranch = TreePtr;
    if (Tree[ParentPtr].FirstBranch == -1) Tree[ParentPtr].FirstBranch = TreePtr;
    return 0;
}


int cCompilerTree::Remove (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, int *ret_Branch)
{
    if (TopOfTree < 0) return -1;
    *ret_Ip = Tree[TopOfTree].Ip;
    *ret_LineNr = Tree[TopOfTree].LineNr;
    *ret_FileOffset = Tree[TopOfTree].FileOffset;
    if (ret_Branch != nullptr) *ret_Branch = Tree[TopOfTree].FirstBranch;
    int Ret = Tree[TopOfTree].Cmd;
    int Prev = Tree[TopOfTree].Prev;
    FreeOne (TopOfTree);
    TopOfTree = Prev;
    return Ret;
}


int cCompilerTree::RemoveBreakFromWhile (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser)
{
    if ((TopOfTree < 0)  ||
        (Tree[TopOfTree].Cmd != COMP_TREE_WHILE_CMD)) {
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "no coresponding WHILE found for that BREAK");
        return -1;
    }
    int Pos = Tree[TopOfTree].FirstBranch;
    if (Pos < 0) return 0;   // no more BREAKs
    *ret_Ip = Tree[Pos].Ip;
    *ret_LineNr = Tree[Pos].LineNr;
    *ret_FileOffset = Tree[Pos].FileOffset;
    Tree[TopOfTree].FirstBranch = Tree[Pos].Next;
    FreeOne (Pos);
    return 1;
}


int cCompilerTree::AddIf (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset)
{
    return Add (COMP_TREE_IF_CMD, par_Ip, par_LineNr, par_FileOffset);
}

int cCompilerTree::AddElse (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset,
                            uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser)
{
    switch (Remove (ret_Ip, ret_LineNr, ret_FileOffset, nullptr)) {
    case COMP_TREE_IF_CMD:
    case COMP_TREE_ELSEIF_CMD:
        return Add (COMP_TREE_ELSE_CMD, par_Ip, par_LineNr, par_FileOffset);
    default:
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "missing IF or ELSEIF for that ELSE");
        return -1;
    }
}


int cCompilerTree::AddElseIf (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset,
                              uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser)
{
    switch (Remove (ret_Ip, ret_LineNr, ret_FileOffset, nullptr)) {
    case COMP_TREE_IF_CMD:
    case COMP_TREE_ELSEIF_CMD:
        return Add (COMP_TREE_ELSEIF_CMD, par_Ip, par_LineNr, par_FileOffset);
    default:
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "missing IF or ELSEIF for that ELSEIF");
        return -1;
    }
}


int cCompilerTree::RemoveEndIf (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser)
{
    switch (Remove (ret_Ip, ret_LineNr, ret_FileOffset, nullptr)) {
    case COMP_TREE_IF_CMD:
        // no break
    case COMP_TREE_ELSE_CMD:
        // no break
    case COMP_TREE_ELSEIF_CMD:
        // no break
        return 0;
    case COMP_TREE_WHILE_CMD: 
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an ENDWHILE and not an ENDIF");
        return -1;
    case COMP_TREE_ATOMIC_CMD: 
        if (StrictAtomic) {
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_ATOMIC and not an ENDIF");
            return -1;
        } else {
            par_Parser->Error (SCRIPT_PARSER_WARNING, "expecting an END_ATOMIC and not an ENDIF");
            return RemoveEndIf (ret_Ip, ret_LineNr, ret_FileOffset, par_Parser);
        }
    case COMP_TREE_DEF_PROC_CMD: 
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_DEF_PROC and not an ENDIF");
        return -1;
    case COMP_TREE_DEF_LOCALS_CMD: 
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_DEF_LOCALS and not an ENDIF");
        return -1;
    default:
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "there is no corresponding IF or ELSE");
        return -1;
    }
}

int cCompilerTree::AddWhile (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset)
{
    return Add (COMP_TREE_WHILE_CMD, par_Ip, par_LineNr, par_FileOffset);
}

int cCompilerTree::AddBreak (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset, cParser *par_Parser)
{
    int x;

    for (x = TopOfTree; x >= 0; x = Tree[x].Prev) {
        switch (Tree[x].Cmd) {
        case COMP_TREE_WHILE_CMD:
            return AddBranch (x, COMP_TREE_BREAK, par_Ip, par_LineNr, par_FileOffset);
        case COMP_TREE_DEF_PROC_CMD:
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "no corresponding WHILE for this BREAK found");
            return -1;
        default:
            break;
        }
    }
    par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "no corresponding WHILE for this BREAK found");
    return -1;
}

int cCompilerTree::RemoveEndWhile (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser)
{
    switch (Remove (ret_Ip, ret_LineNr, ret_FileOffset, nullptr)) {
    case COMP_TREE_IF_CMD:
    case COMP_TREE_ELSE_CMD:
    case COMP_TREE_ELSEIF_CMD:
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an ENDIF and not an ENDWHILE");
        return -1;
    case COMP_TREE_WHILE_CMD: 
        return 0;
    case COMP_TREE_ATOMIC_CMD: 
        if (StrictAtomic) {
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_ATOMIC and not an ENDWHILE");
            return -1;
        } else {
            return RemoveEndWhile (ret_Ip, ret_LineNr, ret_FileOffset, par_Parser);
        }
    case COMP_TREE_DEF_PROC_CMD: 
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_DEF_PROC and not an ENDWHILE");
        return -1;
    case COMP_TREE_DEF_LOCALS_CMD: 
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_DEF_LOCALS and not an ENDWHILE");
        return -1;
    default:
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "there is no corresponding WHILE");
        return -1;
    }
}

int cCompilerTree::AddAtomic (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset)
{
    return Add (COMP_TREE_ATOMIC_CMD, par_Ip, par_LineNr, par_FileOffset);
}

int cCompilerTree::RemoveEndAtomic (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser)
{
    int Cmd;
    switch (Cmd = Remove (ret_Ip, ret_LineNr, ret_FileOffset, nullptr)) {
    case COMP_TREE_IF_CMD:
    case COMP_TREE_ELSE_CMD:
    case COMP_TREE_ELSEIF_CMD:
        if (StrictAtomic) {
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an ENDIF or ELSE or ELSEIF and not an END_ATOMIC");
            return -1;
        } else {
            par_Parser->Error (SCRIPT_PARSER_WARNING, "expecting an ENDIF or ELSE or ELSEIF and not an END_ATOMIC");
            Add (Cmd, *ret_Ip, *ret_LineNr, *ret_FileOffset);
            return 0;
        }
    case COMP_TREE_WHILE_CMD: 
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an ENDWHILE and not an END_ATOMIC");
        return -1;
    case COMP_TREE_ATOMIC_CMD: 
        return 0;
    case COMP_TREE_DEF_PROC_CMD: 
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_DEF_PROC and not an END_ATOMIC");
        return -1;
    case COMP_TREE_DEF_LOCALS_CMD: 
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_DEF_LOCALS and not an END_ATOMIC");
        return -1;
    default:
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "there is no corresponding ATOMIC");
        return -1;
    }
}

int cCompilerTree::AddDefProc (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset)
{
    return Add (COMP_TREE_DEF_PROC_CMD, par_Ip, par_LineNr, par_FileOffset);
}


int cCompilerTree::RemoveEndDefProc (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser)
{
    switch (Remove (ret_Ip, ret_LineNr, ret_FileOffset, nullptr)) {
    case COMP_TREE_IF_CMD:
    case COMP_TREE_ELSE_CMD:
    case COMP_TREE_ELSEIF_CMD:
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an ENDIF and not an END_DEF_PROC");
        return -1;
    case COMP_TREE_WHILE_CMD: 
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an ENDWHILE and not an END_DEF_PROC");
        return -1;
    case COMP_TREE_ATOMIC_CMD: 
        if (StrictAtomic) {
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_ATOMIC and not an END_DEF_PROC");
            return -1;
        } else {
            return RemoveEndDefProc (ret_Ip, ret_LineNr, ret_FileOffset, par_Parser);
        }
    case COMP_TREE_DEF_PROC_CMD: 
        return 0;
    case COMP_TREE_DEF_LOCALS_CMD: 
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_DEF_LOCALS and not an END_DEF_PROC");
        return -1;
    default:
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "there is no corresponding DEF_PROC");
        return -1;
    }
}

int cCompilerTree::AddDefLocals (uint32_t par_Ip, int par_LineNr, uint32_t par_FileOffset)
{
    return Add (COMP_TREE_DEF_LOCALS_CMD, par_Ip, par_LineNr, par_FileOffset);
}


int cCompilerTree::RemoveEndDefLocals (uint32_t *ret_Ip, int *ret_LineNr, uint32_t *ret_FileOffset, cParser *par_Parser)
{
    switch (Remove (ret_Ip, ret_LineNr, ret_FileOffset, nullptr)) {
    case COMP_TREE_IF_CMD:
    case COMP_TREE_ELSE_CMD:
    case COMP_TREE_ELSEIF_CMD:
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an ENDIF and not an END_DEF_LOCALS");
        return -1;
    case COMP_TREE_WHILE_CMD: 
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an ENDWHILE and not an END_DEF_LOCALS");
        return -1;
    case COMP_TREE_ATOMIC_CMD: 
        if (StrictAtomic) {
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_ATOMIC and not an END_DEF_LOCALS");
            return -1;
        } else {
            return RemoveEndDefLocals (ret_Ip, ret_LineNr, ret_FileOffset, par_Parser);
        }
    case COMP_TREE_DEF_PROC_CMD: 
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_DEF_PROC and not an END_DEF_LOCALS");
        return -1;
    case COMP_TREE_DEF_LOCALS_CMD: 
        return 0;
    default:
        par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "there is no corresponding DEF_LOCALS");
        return -1;
    }
}


void cCompilerTree::Reset (cParser *par_Parser)
{
    uint32_t Ip;
    int LineNr;
    uint32_t FileOffset;
    int Branch;
    while (Remove (&Ip, &LineNr, &FileOffset, &Branch) >= 0) {
        if (Branch >= 0) {
            while (RemoveBreakFromWhile (&Ip, &LineNr, &FileOffset, par_Parser) >= 0);
        }
    }
}

int cCompilerTree::EndOfFile (cParser *par_Parser)
{
    uint32_t Ip;
    int LineNr;
    uint32_t FileOffset;
    int Branch;
    int Cmd;

    while ((Cmd = Remove (&Ip, &LineNr, &FileOffset, &Branch)) >= 0) {
        switch (Cmd) {
        case COMP_TREE_IF_CMD:
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an ENDIF and not end of file (IF started at line %i)", LineNr);
            return -1;
        case COMP_TREE_ELSE_CMD:
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an ENDIF and not end of file (ELSE started at line %i)", LineNr);
            return -1;
        case COMP_TREE_ELSEIF_CMD:
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an ENDIF and not end of file (ELSEIF started at line %i)", LineNr);
            return -1;
        case COMP_TREE_WHILE_CMD: 
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an ENDWHILE and not end of file (WHILE started at line %i)", LineNr);
            return -1;
        case COMP_TREE_ATOMIC_CMD: 
            if (StrictAtomic) {
                par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_ATOMIC and not end of file (ATOMIC started at line %i)", LineNr);
                return -1;
            }
            break;
        case COMP_TREE_DEF_PROC_CMD: 
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_DEF_PROC and not end of file (DEF_PROC started at line %i)", LineNr);
            return -1;
        case COMP_TREE_DEF_LOCALS_CMD: 
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "expecting an END_DEF_LOCALS and not end of file (DEF_LOCALS started at line %i)", LineNr);
            return -1;
        default:
            par_Parser->Error (SCRIPT_PARSER_ERROR_CONTINUE, "this should never happen");
            return -1;
        }
    }
    return 0;
}
