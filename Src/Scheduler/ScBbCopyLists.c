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
#include "Blackboard.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "PrintFormatToString.h"
#include "MemZeroAndCopy.h"
#include "BlackboardAccess.h"
#include "ExecutionStack.h"
#include "ScriptMessageFile.h"

#include "PipeMessagesShared.h"
#include "Scheduler.h"
#include "ScBbCopyLists.h"
#include "BlackboardConvertFromTo.h"
#include "Wildcards.h"

#define UNUSED(x) (void)(x)

static int GetBbVarTypeSize (int Type)
{
    switch (Type) {
    case BB_BYTE:
    case BB_UBYTE:
        return 1;
    case BB_WORD:
    case BB_UWORD:
        return 2;
    case BB_DWORD:
    case BB_UDWORD:
    case BB_FLOAT:
        return 4;
    case BB_QWORD:
    case BB_UQWORD:
    case BB_DOUBLE:
        return 8;
    default:
        return 0;
    }    
}

static int InsertVariableToCopyListsRdOrWr (PIPE_MESSAGE_REF_COPY_LIST *pList, int Vid, unsigned long long Addr, unsigned long long UniqueId, int TypeBB, int TypePipe, int Dir, int RangeControlFlag)
{
    if (pList->ElemCount >= pList->ElemCountMax) {
        pList->ElemCountMax += 128;
        pList->pElems = (PIPE_MESSAGE_REF_COPY_LIST_ELEM*)my_realloc (pList->pElems, (size_t)pList->ElemCountMax * sizeof (PIPE_MESSAGE_REF_COPY_LIST_ELEM));
        if (pList->pElems == NULL) {
            ThrowError (1, "out of memory cannot build copy lists BB <-> reference");
            return -1;
        }
    }
    pList->pElems[pList->ElemCount].UniqueIdOrNamneAndUniqueId.UniqueId = UniqueId;
    pList->pElems[pList->ElemCount].Vid = Vid;
    pList->pElems[pList->ElemCount].Addr = Addr;
    pList->pElems[pList->ElemCount].TypeBB = (short)TypeBB;
    pList->pElems[pList->ElemCount].TypePipe = (short)TypePipe;
    pList->pElems[pList->ElemCount].RangeControlState = 0;
    pList->pElems[pList->ElemCount].RangeControlFlag = (short)RangeControlFlag;
    pList->pElems[pList->ElemCount].Dir = Dir;
    pList->SizeInBytes += GetBbVarTypeSize (TypePipe);
    pList->ElemCount++;
    return 0;
}

int InsertVariableToCopyLists (int Pid, int Vid, unsigned long long Addr, int TypeBB, int TypePipe, int Dir, unsigned long long UniqueId)
{
    TASK_CONTROL_BLOCK *Tcb;
    short RangeControlFlag;

    Tcb = GetPointerToTaskControlBlock (Pid);
    if (Tcb == NULL) {
        return -1;
    }

    // check if it would fit into the list
    int Size = GetBbVarTypeSize(TypePipe);
    if ((Dir & PIPE_API_REFERENCE_VARIABLE_DIR_READ) == PIPE_API_REFERENCE_VARIABLE_DIR_READ) {
        int SumSize = Tcb->CopyLists.RdList1.SizeInBytes
                      + Tcb->CopyLists.RdList2.SizeInBytes
                      + Tcb->CopyLists.RdList4.SizeInBytes
                      + Tcb->CopyLists.RdList8.SizeInBytes;
        // only the half of th max. massage siz should be used as variabls the rest shold b left for the virtual network
        if ((SumSize + Size) > (PIPE_MESSAGE_BUFSIZE / 2)) {
            return VARIABLE_REF_LIST_MAX_SIZE_REACHED;
        }
    }
    if ((Dir & PIPE_API_REFERENCE_VARIABLE_DIR_WRITE) == PIPE_API_REFERENCE_VARIABLE_DIR_WRITE) {
        int SumSize = Tcb->CopyLists.WrList1.SizeInBytes
                      + Tcb->CopyLists.WrList2.SizeInBytes
                      + Tcb->CopyLists.WrList4.SizeInBytes
                      + Tcb->CopyLists.WrList8.SizeInBytes;
        // only the half of th max. massage siz should be used as variabls the rest shold b left for the virtual network
        if ((SumSize + Size) > (PIPE_MESSAGE_BUFSIZE / 2)) {
            return VARIABLE_REF_LIST_MAX_SIZE_REACHED;
        }
    }

    if (InsertVariableToCopyListsRdOrWr (&(Tcb->CopyLists.List), Vid, Addr, UniqueId, TypeBB, TypePipe, Dir, 0)) {
        return -1;
    }
    RangeControlFlag = ((Tcb->RangeControlFlags & RANGE_CONTROL_BEFORE_ACTIVE_FLAG) == RANGE_CONTROL_BEFORE_ACTIVE_FLAG) &&
                       !((Tcb->RangeControlFlags & RANGE_CONTROL_BEFORE_INIT_AS_DEACTIVE) == RANGE_CONTROL_BEFORE_INIT_AS_DEACTIVE);
    if ((Dir & PIPE_API_REFERENCE_VARIABLE_DIR_READ) == PIPE_API_REFERENCE_VARIABLE_DIR_READ) {
        switch (Size) {
        case 1:
            if (InsertVariableToCopyListsRdOrWr (&(Tcb->CopyLists.RdList1), Vid, Addr, UniqueId, TypeBB, TypePipe, Dir, RangeControlFlag)) {
                return -1;
            }
            break;
        case 2:
            if (InsertVariableToCopyListsRdOrWr (&(Tcb->CopyLists.RdList2), Vid, Addr, UniqueId, TypeBB, TypePipe, Dir, RangeControlFlag)) {
                return -1;
            }
            break;
        case 4:
            if (InsertVariableToCopyListsRdOrWr (&(Tcb->CopyLists.RdList4), Vid, Addr, UniqueId, TypeBB, TypePipe, Dir, RangeControlFlag)) {
                return -1;
            }
            break;
        case 8:
            if (InsertVariableToCopyListsRdOrWr (&(Tcb->CopyLists.RdList8), Vid, Addr, UniqueId, TypeBB, TypePipe, Dir, RangeControlFlag)) {
                return -1;
            }
            break;
        default:
            return -1;
        }
    }
    RangeControlFlag = ((Tcb->RangeControlFlags & RANGE_CONTROL_BEHIND_ACTIVE_FLAG) == RANGE_CONTROL_BEHIND_ACTIVE_FLAG) &&
                       !((Tcb->RangeControlFlags & RANGE_CONTROL_BEHIND_INIT_AS_DEACTIVE) == RANGE_CONTROL_BEHIND_INIT_AS_DEACTIVE);
    if ((Dir & PIPE_API_REFERENCE_VARIABLE_DIR_WRITE) == PIPE_API_REFERENCE_VARIABLE_DIR_WRITE) {
        switch (Size) {
        case 1:
            if (InsertVariableToCopyListsRdOrWr (&(Tcb->CopyLists.WrList1), Vid, Addr, UniqueId, TypeBB, TypePipe, Dir, RangeControlFlag)) {
                return -1;
            }
            break;
        case 2:
            if (InsertVariableToCopyListsRdOrWr (&(Tcb->CopyLists.WrList2), Vid, Addr, UniqueId, TypeBB, TypePipe, Dir, RangeControlFlag)) {
                return -1;
            }
            break;
        case 4:
            if (InsertVariableToCopyListsRdOrWr (&(Tcb->CopyLists.WrList4), Vid, Addr, UniqueId, TypeBB, TypePipe, Dir, RangeControlFlag)) {
                return -1;
            }
            break;
        case 8:
            if (InsertVariableToCopyListsRdOrWr (&(Tcb->CopyLists.WrList8), Vid, Addr, UniqueId, TypeBB, TypePipe, Dir, RangeControlFlag)) {
                return -1;
            }
            break;
        default:
            return -1;
        }
    }
    return 0;
}

static int RemoveVariableFromCopyListsRdOrWr (PIPE_MESSAGE_REF_COPY_LIST *pList, int Vid, unsigned long long Addr, unsigned long long UniqueId, int *ret_Dir)
{
    UNUSED(Addr);
    int x;

    for (x = pList->ElemCount - 1; x >= 0; x--) {
        if ((pList->pElems[x].UniqueIdOrNamneAndUniqueId.UniqueId == UniqueId) &&   // Unique ID and Vid must be equal (Vids can be occur several times)!
            (pList->pElems[x].Vid == Vid)) {
            if (ret_Dir != NULL) *ret_Dir =  pList->pElems[x].Dir;
            pList->SizeInBytes -= GetBbVarTypeSize (pList->pElems[x].TypePipe);
            pList->ElemCount--;
            pList->pElems[x] = pList->pElems[pList->ElemCount];  // move the last one to the deleted
            return 0;
        }
    }
    return -1;
}


static int RemoveAllVariableFromCopyListsRdOrWr (PIPE_MESSAGE_REF_COPY_LIST *pList)
{
    pList->ElemCount = 0;
    pList->SizeInBytes = 0;
    return 0;
}

int RemoveVariableFromCopyLists (int Pid, int Vid, unsigned long long UniqueId, unsigned long long Addr)
{
    int Ret = 0;
    TASK_CONTROL_BLOCK *Tcb;
    int Dir = 0;

    Tcb = GetPointerToTaskControlBlock (Pid);
    if (Tcb == NULL) {
        return -1;
    }
    if (RemoveVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.List), Vid, Addr, UniqueId, &Dir) != 0) {
        Ret = -1;
    }
    if ((Dir & PIPE_API_REFERENCE_VARIABLE_DIR_READ) == PIPE_API_REFERENCE_VARIABLE_DIR_READ) {
        if ((RemoveVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.RdList1), Vid, Addr, UniqueId, NULL) != 0) &&
            (RemoveVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.RdList2), Vid, Addr, UniqueId, NULL) != 0) &&
            (RemoveVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.RdList4), Vid, Addr, UniqueId, NULL) != 0) &&
            (RemoveVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.RdList8), Vid, Addr, UniqueId, NULL) != 0)) {
            Ret = -1;
        }
    }
    if ((Dir & PIPE_API_REFERENCE_VARIABLE_DIR_WRITE) == PIPE_API_REFERENCE_VARIABLE_DIR_WRITE) {
        if ((RemoveVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.WrList1), Vid, Addr, UniqueId, NULL) != 0) &&
            (RemoveVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.WrList2), Vid, Addr, UniqueId, NULL) != 0) &&
            (RemoveVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.WrList4), Vid, Addr, UniqueId, NULL) != 0) &&
            (RemoveVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.WrList8), Vid, Addr, UniqueId, NULL) != 0)) {
            Ret = -1;
        }
    }
    return Ret;
}


int RemoveAllVariableFromCopyLists (int Pid)
{
    int Ret = 0;
    TASK_CONTROL_BLOCK *Tcb;

    Tcb = GetPointerToTaskControlBlock (Pid);
    if (Tcb == NULL) {
        return -1;
    }
    if (RemoveAllVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.List)) != 0) {
        Ret = -1;
    }
    if ((RemoveAllVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.RdList1)) != 0) ||
        (RemoveAllVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.RdList2)) != 0) ||
        (RemoveAllVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.RdList4)) != 0) ||
        (RemoveAllVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.RdList8)) != 0)) {
        Ret = -1;
    }
    if ((RemoveAllVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.WrList1)) != 0) ||
        (RemoveAllVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.WrList2)) != 0) ||
        (RemoveAllVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.WrList4)) != 0) ||
        (RemoveAllVariableFromCopyListsRdOrWr (&(Tcb->CopyLists.WrList8)) != 0)) {
        Ret = -1;
    }
    return Ret;
}


static int ChangeVariableRangeControlInsideCopyListsRdOrWr (PIPE_MESSAGE_REF_COPY_LIST *pList, int Vid, int Flag)
{
    int x, Ret = -1;

    for (x = 0; x < pList->ElemCount; x++) {
        if (pList->pElems[x].Vid == Vid) {   //  Remark: Vids can be occur several times
            pList->pElems[x].RangeControlFlag = (short)Flag;
            pList->pElems[x].RangeControlState = 0;
            Ret = 0;
        }
    }
    return Ret;
}

int ChangeVariableRangeControlInsideCopyLists (int Pid, int Vid, int Flag)
{
    int Ret = -1;
    TASK_CONTROL_BLOCK *Tcb;

    Tcb = GetPointerToTaskControlBlock (Pid);
    if (Tcb == NULL) {
        return -1;
    }
    if ((ChangeVariableRangeControlInsideCopyListsRdOrWr (&(Tcb->CopyLists.RdList1), Vid, Flag) == 0) &&
        (ChangeVariableRangeControlInsideCopyListsRdOrWr (&(Tcb->CopyLists.RdList2), Vid, Flag) == 0) &&
        (ChangeVariableRangeControlInsideCopyListsRdOrWr (&(Tcb->CopyLists.RdList4), Vid, Flag) == 0) &&
        (ChangeVariableRangeControlInsideCopyListsRdOrWr (&(Tcb->CopyLists.RdList8), Vid, Flag) == 0)) {
        Ret = 0;
    }

    if ((ChangeVariableRangeControlInsideCopyListsRdOrWr (&(Tcb->CopyLists.WrList1), Vid, Flag) == 0) &&
        (ChangeVariableRangeControlInsideCopyListsRdOrWr (&(Tcb->CopyLists.WrList2), Vid, Flag) == 0) &&
        (ChangeVariableRangeControlInsideCopyListsRdOrWr (&(Tcb->CopyLists.WrList4), Vid, Flag) == 0) &&
        (ChangeVariableRangeControlInsideCopyListsRdOrWr (&(Tcb->CopyLists.WrList8), Vid, Flag) == 0)) {
        Ret = 0;
    }
    return Ret;
}


static int ChangeVariableRangeControlFilteredInsideCopyListsRdOrWr (PIPE_MESSAGE_REF_COPY_LIST *pList, char *NameFilter, int Flag)
{
    int x, Ret = -1;
    char Name[BBVARI_NAME_SIZE];

    for (x = 0; x < pList->ElemCount; x++) {
        if (GetBlackboardVariableName (pList->pElems[x].Vid, Name, sizeof(Name)) == 0) {
            if (!Compare2StringsWithWildcardsCaseSensitive (Name, NameFilter, 0)) {
                pList->pElems[x].RangeControlFlag = (short)Flag;
                pList->pElems[x].RangeControlState = 0;
            }  // Remark: Vids can be occur several times therefor no break
            Ret = 0;
        }
    }
    return Ret;
}

int ChangeVariableRangeControlFilteredInsideCopyLists (int Pid, char *NameFilter, int Flag)
{
    int Ret = -1;
    TASK_CONTROL_BLOCK *Tcb;

    Tcb = GetPointerToTaskControlBlock (Pid);
    if (Tcb == NULL) {
        return -1;
    }
    if ((ChangeVariableRangeControlFilteredInsideCopyListsRdOrWr (&(Tcb->CopyLists.RdList1), NameFilter, Flag) == 0) &&
        (ChangeVariableRangeControlFilteredInsideCopyListsRdOrWr (&(Tcb->CopyLists.RdList2), NameFilter, Flag) == 0) &&
        (ChangeVariableRangeControlFilteredInsideCopyListsRdOrWr (&(Tcb->CopyLists.RdList4), NameFilter, Flag) == 0) &&
        (ChangeVariableRangeControlFilteredInsideCopyListsRdOrWr (&(Tcb->CopyLists.RdList8), NameFilter, Flag) == 0)) {
        Ret = 0;
    }

    if ((ChangeVariableRangeControlFilteredInsideCopyListsRdOrWr (&(Tcb->CopyLists.WrList1), NameFilter, Flag) == 0) &&
        (ChangeVariableRangeControlFilteredInsideCopyListsRdOrWr (&(Tcb->CopyLists.WrList2), NameFilter, Flag) == 0) &&
        (ChangeVariableRangeControlFilteredInsideCopyListsRdOrWr (&(Tcb->CopyLists.WrList4), NameFilter, Flag) == 0) &&
        (ChangeVariableRangeControlFilteredInsideCopyListsRdOrWr (&(Tcb->CopyLists.WrList8), NameFilter, Flag) == 0)) {
        Ret = 0;
    }
    return Ret;
}

static int RangeControlCheck (TASK_CONTROL_BLOCK *Tcb, PIPE_MESSAGE_REF_COPY_LIST_ELEM *CopyListElem, int Phys, int Limit, int Output)
{
    int vid_index;
    double Value;
    int Ret = 0;

    vid_index = get_variable_index (CopyListElem->Vid);
    if (vid_index < 0) return 0;

    if (!CopyListElem->RangeControlFlag) {
        return 0;
    }
    if (Phys && (blackboard[vid_index].pAdditionalInfos->Conversion.Type != BB_CONV_FORMULA)) {
        Value = execute_stack ((struct EXEC_STACK_ELEM *)blackboard[vid_index].pAdditionalInfos->Conversion.Conv.Formula.FormulaByteCode);
    } else {
        bbvari_to_double (blackboard[vid_index].Type,
                          blackboard[vid_index].Value,
                          &Value);
    }

    if (Value < blackboard[vid_index].pAdditionalInfos->Min) {
        switch (CopyListElem->RangeControlState) {
        case -1:  // was already inside the negative invalid range
            break;
        case 0:   // was inside valid range
            // no break;
        case 1:   // was inside positive invalid range
            CopyListElem->RangeControlState = -1;  // now it is inside the negative invalid range
            Ret = 1;
            if (Tcb->RangeCounterVid > 0) {
                write_bbvari_uqword (Tcb->RangeCounterVid, read_bbvari_uqword (Tcb->RangeCounterVid) + 1);
            }
            if (Output) {
                char Text[1024];
                PrintFormatToString (Text, sizeof(Text), "Variable %s = %g%s become smaler than it's min value %g",
                         blackboard[vid_index].pAdditionalInfos->Name, Value, Phys ? " (Phys)": "", blackboard[vid_index].pAdditionalInfos->Min);
                switch (Output) {
                case 1:
                    ThrowError (WARNING_NO_STOP, Text);
                    break;
                case 2:
                    AddScriptMessage (Text);
                    break;
                }
            }
            break;
        }
        if (Limit) {
            if (Phys && (blackboard[vid_index].pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA)) {
                write_bbvari_phys_minmax_check (blackboard[vid_index].Vid, blackboard[vid_index].pAdditionalInfos->Min);
            } else {
                write_bbvari_minmax_check (blackboard[vid_index].Vid, blackboard[vid_index].pAdditionalInfos->Min);
            }
        }
    } else if (Value > blackboard[vid_index].pAdditionalInfos->Max) {
        switch (CopyListElem->RangeControlState) {
        case 1:  //was already inside the positive invalid range
            break;
        case 0:   // was inside valid range
            // no break;
        case -1:   // was inside negative invalid range
            Ret = 1;
            CopyListElem->RangeControlState = 1;  // now it is inside the positive invalid range
            if (Tcb->RangeCounterVid > 0) {
                write_bbvari_uqword (Tcb->RangeCounterVid, read_bbvari_uqword (Tcb->RangeCounterVid) + 1);
            }
            if (Output) {
                char Text[1024];
                PrintFormatToString (Text, sizeof(Text), "Variable %s = %g%s become larger than it's max value %g",
                         blackboard[vid_index].pAdditionalInfos->Name, Value, Phys ? " (Phys)": "", blackboard[vid_index].pAdditionalInfos->Max);
                switch (Output) {
                case 1:
                    ThrowError (WARNING_NO_STOP, Text);
                    break;
                case 2:
                    AddScriptMessage (Text);
                    break;
                }
            }
            break;
        }
        if (Limit) {
            if (Phys && (blackboard[vid_index].pAdditionalInfos->Conversion.Type == BB_CONV_FORMULA)) {
                write_bbvari_phys_minmax_check (blackboard[vid_index].Vid, blackboard[vid_index].pAdditionalInfos->Max);
            } else {
                write_bbvari_minmax_check (blackboard[vid_index].Vid, blackboard[vid_index].pAdditionalInfos->Max);
            }
        }
    } else {
        CopyListElem->RangeControlState = 0; // it is inside valid range
    }

    return Ret;
}

static inline void UnlockBBErrorMessage(PIPE_MESSAGE_REF_COPY_LIST_ELEM *par_Elems, int par_LineNr)
{
    char Name[BBVARI_NAME_SIZE];
    Name[0] = 0;
    LeaveBlackboardCriticalSection();
    GetBlackboardVariableName(par_Elems->Vid, Name, sizeof(Name));
    ThrowError (1, "internal error in copy pipe to blackboard list vid=%i \"%s\" (unknown data type %i %i) %s [%i]",
           par_Elems->Vid, Name, par_Elems->TypeBB, par_Elems->TypePipe,
           __FILE__, par_LineNr);
    EnterBlackboardCriticalSection();
}


static int CopyFromBlackbardToPipeRC (TASK_CONTROL_BLOCK *Tcb, char *SnapShotData)
{
    int x, pos = 0;
    int Phys;
    int Limit;
    int Output;
    int RangeErrorCounter = 0;

    if (blackboard == NULL) {
        return 0;
    }

    Phys = (Tcb->RangeControlFlags & RANGE_CONTROL_PHYSICAL_FLAG) == RANGE_CONTROL_PHYSICAL_FLAG;
    Limit = (Tcb->RangeControlFlags & RANGE_CONTROL_LIMIT_VALUES_FLAG) == RANGE_CONTROL_LIMIT_VALUES_FLAG;
    Output = Tcb->RangeControlFlags & RANGE_CONTROL_OUTPUT_MASK;
    EnterBlackboardCriticalSection();
    for (x = 0; x < Tcb->CopyLists.WrList8.ElemCount; x++) {
        reset_wrflag (Tcb->CopyLists.WrList8.pElems[x].Vid, Tcb->wr_flag_mask);
    }
    for (x = 0; x < Tcb->CopyLists.WrList4.ElemCount; x++) {
        reset_wrflag (Tcb->CopyLists.WrList4.pElems[x].Vid, Tcb->wr_flag_mask);
    }
    for (x = 0; x < Tcb->CopyLists.WrList2.ElemCount; x++) {
        reset_wrflag (Tcb->CopyLists.WrList2.pElems[x].Vid, Tcb->wr_flag_mask);
    }
    for (x = 0; x < Tcb->CopyLists.WrList1.ElemCount; x++) {
        reset_wrflag (Tcb->CopyLists.WrList1.pElems[x].Vid, Tcb->wr_flag_mask);
    }
    for (x = 0; x < Tcb->CopyLists.RdList8.ElemCount; x++) {
        int size;
        // Range Control
        RangeErrorCounter += RangeControlCheck (Tcb, &(Tcb->CopyLists.RdList8.pElems[x]), Phys, Limit, Output);

        size = read_bbvari_convert_to (Tcb->CopyLists.RdList8.pElems[x].Vid, Tcb->CopyLists.RdList8.pElems[x].TypePipe,  (union BB_VARI*)(void*)(SnapShotData + pos));
        if (size <= 0) {
            UnlockBBErrorMessage(&Tcb->CopyLists.RdList8.pElems[x], __LINE__);
            break;
        }
        pos += size;
    }
    for (x = 0; x < Tcb->CopyLists.RdList4.ElemCount; x++) {
        int size;
        // Range Control
        RangeErrorCounter += RangeControlCheck (Tcb, &(Tcb->CopyLists.RdList4.pElems[x]), Phys, Limit, Output);

        size = read_bbvari_convert_to (Tcb->CopyLists.RdList4.pElems[x].Vid, Tcb->CopyLists.RdList4.pElems[x].TypePipe,  (union BB_VARI*)(void*)(SnapShotData + pos));
        if (size <= 0) {
            UnlockBBErrorMessage(&Tcb->CopyLists.RdList4.pElems[x], __LINE__);
            break;
        }
        pos += size;
    }
    for (x = 0; x < Tcb->CopyLists.RdList2.ElemCount; x++) {
        int size;
        // Range Control
        RangeErrorCounter += RangeControlCheck (Tcb, &(Tcb->CopyLists.RdList2.pElems[x]), Phys, Limit, Output);

        size = read_bbvari_convert_to (Tcb->CopyLists.RdList2.pElems[x].Vid, Tcb->CopyLists.RdList2.pElems[x].TypePipe,  (union BB_VARI*)(void*)(SnapShotData + pos));
        if (size <= 0) {
            UnlockBBErrorMessage(&Tcb->CopyLists.RdList2.pElems[x], __LINE__);
            break;
        }
        pos += size;
    }
    for (x = 0; x < Tcb->CopyLists.RdList1.ElemCount; x++) {
        int size;
        // Range Control
        RangeErrorCounter += RangeControlCheck (Tcb, &(Tcb->CopyLists.RdList1.pElems[x]), Phys, Limit, Output);

        size = read_bbvari_convert_to (Tcb->CopyLists.RdList1.pElems[x].Vid, Tcb->CopyLists.RdList1.pElems[x].TypePipe,  (union BB_VARI*)(void*)(SnapShotData + pos));
        if (size <= 0) {
            UnlockBBErrorMessage(&Tcb->CopyLists.RdList1.pElems[x], __LINE__);
            break;
        }
        pos += size;
    }
    LeaveBlackboardCriticalSection();

    if (RangeErrorCounter > 0) {
         if ((Tcb->RangeControlFlags & RANGE_CONTROL_STOP_SCHEDULER_FLAG) == RANGE_CONTROL_STOP_SCHEDULER_FLAG) {
            disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_USER, NULL, NULL);
        }
    }

    return pos;
}


int CopyFromBlackbardToPipe (TASK_CONTROL_BLOCK *Tcb, char *SnapShotData)
{
    int x, pos = 0;

    if ((Tcb->RangeControlFlags & RANGE_CONTROL_BEFORE_ACTIVE_FLAG) == RANGE_CONTROL_BEFORE_ACTIVE_FLAG) {
        switch (read_bbvari_udword(Tcb->RangeControlVid)) {
        case 1:
        case 3:
            return CopyFromBlackbardToPipeRC (Tcb, SnapShotData);
        }
    }

    EnterBlackboardCriticalSection();
    for (x = 0; x < Tcb->CopyLists.WrList8.ElemCount; x++) {
        reset_wrflag (Tcb->CopyLists.WrList8.pElems[x].Vid, Tcb->wr_flag_mask);
    }
    for (x = 0; x < Tcb->CopyLists.WrList4.ElemCount; x++) {
        reset_wrflag (Tcb->CopyLists.WrList4.pElems[x].Vid, Tcb->wr_flag_mask);
    }
    for (x = 0; x < Tcb->CopyLists.WrList2.ElemCount; x++) {
        reset_wrflag (Tcb->CopyLists.WrList2.pElems[x].Vid, Tcb->wr_flag_mask);
    }
    for (x = 0; x < Tcb->CopyLists.WrList1.ElemCount; x++) {
        reset_wrflag (Tcb->CopyLists.WrList1.pElems[x].Vid, Tcb->wr_flag_mask);
    }
    for (x = 0; x < Tcb->CopyLists.RdList8.ElemCount; x++) {
        int size = read_bbvari_convert_to (Tcb->CopyLists.RdList8.pElems[x].Vid, Tcb->CopyLists.RdList8.pElems[x].TypePipe, (union BB_VARI*)(void*)(SnapShotData + pos));
        if (size <= 0) {
            UnlockBBErrorMessage(&Tcb->CopyLists.RdList8.pElems[x], __LINE__);
            break;
        }
        pos += size;
    }
    for (x = 0; x < Tcb->CopyLists.RdList4.ElemCount; x++) {
        int size = read_bbvari_convert_to (Tcb->CopyLists.RdList4.pElems[x].Vid, Tcb->CopyLists.RdList4.pElems[x].TypePipe, (union BB_VARI*)(void*)(SnapShotData + pos));
        if (size <= 0) {
            UnlockBBErrorMessage(&Tcb->CopyLists.RdList4.pElems[x], __LINE__);
            break;
        }
        pos += size;
    }
    for (x = 0; x < Tcb->CopyLists.RdList2.ElemCount; x++) {
        int size = read_bbvari_convert_to (Tcb->CopyLists.RdList2.pElems[x].Vid, Tcb->CopyLists.RdList2.pElems[x].TypePipe, (union BB_VARI*)(void*)(SnapShotData + pos));
        if (size <= 0) {
            UnlockBBErrorMessage(&Tcb->CopyLists.RdList2.pElems[x], __LINE__);
            break;
        }
        pos += size;
    }
    for (x = 0; x < Tcb->CopyLists.RdList1.ElemCount; x++) {
        int size = read_bbvari_convert_to (Tcb->CopyLists.RdList1.pElems[x].Vid, Tcb->CopyLists.RdList1.pElems[x].TypePipe, (union BB_VARI*)(void*)(SnapShotData + pos));
        if (size <= 0) {
            UnlockBBErrorMessage(&Tcb->CopyLists.RdList1.pElems[x], __LINE__);
            break;
        }
        pos += size;
    }
    LeaveBlackboardCriticalSection();
    return pos;
}

static int CopyOneWriteListRC(int *pos, char *SnapShotData, PIPE_MESSAGE_REF_COPY_LIST *par_WrList, TASK_CONTROL_BLOCK *Tcb)
{
    int x;
    int Phys;
    int Limit;
    int Output;
    int RangeErrorCounter = 0;

    Phys = (Tcb->RangeControlFlags & RANGE_CONTROL_PHYSICAL_FLAG) == RANGE_CONTROL_PHYSICAL_FLAG;
    Limit = (Tcb->RangeControlFlags & RANGE_CONTROL_LIMIT_VALUES_FLAG) == RANGE_CONTROL_LIMIT_VALUES_FLAG;
    Output = Tcb->RangeControlFlags & RANGE_CONTROL_OUTPUT_MASK;
    for (x = 0; x < par_WrList->ElemCount; x++) {
        if (!test_wrflag (par_WrList->pElems[x].Vid, Tcb->wr_flag_mask)) {
            int size = write_bbvari_convert_to (Tcb->pid, par_WrList->pElems[x].Vid,
                                                par_WrList->pElems[x].TypePipe, SnapShotData + *pos);
            if (size <= 0) {
                UnlockBBErrorMessage(&par_WrList->pElems[x], __LINE__);
            } else {
                *pos += size;
            }
            RangeErrorCounter += RangeControlCheck (Tcb, &(par_WrList->pElems[x]), Phys, Limit, Output);
        } else {
            int size = size_of_bbvari (par_WrList->pElems[x].TypePipe);
            if (size <= 0) {
                UnlockBBErrorMessage(&par_WrList->pElems[x], __LINE__);
            } else {
                *pos += size;
            }
        }
    }
    return RangeErrorCounter;
}

static int CopyFromPipeToBlackboardRC (TASK_CONTROL_BLOCK *Tcb, char *SnapShotData, int SnapShotSize)
{
    int pos = 0;
    int RangeErrorCounter = 0;
    int Size;

    if (blackboard == NULL) {
        return 0;
    }

    Size = Tcb->CopyLists.WrList8.SizeInBytes + Tcb->CopyLists.WrList4.SizeInBytes + Tcb->CopyLists.WrList2.SizeInBytes + Tcb->CopyLists.WrList1.SizeInBytes;
    if (SnapShotSize != Size) {
        ThrowError (1, "wrong size of pipe snapshot %i != %i", SnapShotSize, Size);
        return 0;
    }
    EnterBlackboardCriticalSection();
    RangeErrorCounter = CopyOneWriteListRC(&pos, SnapShotData, &(Tcb->CopyLists.WrList8), Tcb);
    RangeErrorCounter += CopyOneWriteListRC(&pos, SnapShotData, &(Tcb->CopyLists.WrList4), Tcb);
    RangeErrorCounter += CopyOneWriteListRC(&pos, SnapShotData, &(Tcb->CopyLists.WrList2), Tcb);
    RangeErrorCounter += CopyOneWriteListRC(&pos, SnapShotData, &(Tcb->CopyLists.WrList1), Tcb);
    LeaveBlackboardCriticalSection();
    if (SnapShotSize != pos) {
        ThrowError (1, "wrong size of pipe snapshot %i != %i", SnapShotSize, pos);
        return 0;
    }

    if (RangeErrorCounter > 0) {
        if ((Tcb->RangeControlFlags & RANGE_CONTROL_STOP_SCHEDULER_FLAG) == RANGE_CONTROL_STOP_SCHEDULER_FLAG) {
            disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_USER, NULL, NULL);
        }
    }

    return pos;
}

static int CopyOneWriteList(int pos, char *SnapShotData, PIPE_MESSAGE_REF_COPY_LIST *par_WrList, TASK_CONTROL_BLOCK *Tcb)
{
    int x;
    for (x = 0; x < par_WrList->ElemCount; x++) {
        if (!test_wrflag (par_WrList->pElems[x].Vid, Tcb->wr_flag_mask)) {
            int size = write_bbvari_convert_to (Tcb->pid, par_WrList->pElems[x].Vid,
                                                par_WrList->pElems[x].TypePipe, SnapShotData + pos);
            if (size <= 0) {
                UnlockBBErrorMessage(&par_WrList->pElems[x], __LINE__);
            } else {
                pos += size;
            }
        } else {
            int size = size_of_bbvari (par_WrList->pElems[x].TypePipe);
            if (size <= 0) {
                UnlockBBErrorMessage(&par_WrList->pElems[x], __LINE__);
            } else {
                pos += size;
            }
        }
    }
    return pos;
}

int CopyFromPipeToBlackboard (TASK_CONTROL_BLOCK *Tcb, char *SnapShotData, int SnapShotSize) 
{
    int pos, size;

    if ((Tcb->RangeControlFlags & RANGE_CONTROL_BEHIND_ACTIVE_FLAG) == RANGE_CONTROL_BEHIND_ACTIVE_FLAG) {
        switch (read_bbvari_udword(Tcb->RangeControlVid)) {
        case 1:
        case 2:
            return CopyFromPipeToBlackboardRC (Tcb, SnapShotData, SnapShotSize);
        }
    }
    size = Tcb->CopyLists.WrList8.SizeInBytes + Tcb->CopyLists.WrList4.SizeInBytes + Tcb->CopyLists.WrList2.SizeInBytes+ Tcb->CopyLists.WrList1.SizeInBytes;

    if (SnapShotSize != size) {
        ThrowError (1, "wrong size of pipe snapshot %i != %i", SnapShotSize, size);
        return 0;
    }
    EnterBlackboardCriticalSection();
    pos = CopyOneWriteList(0, SnapShotData, &(Tcb->CopyLists.WrList8), Tcb);
    pos = CopyOneWriteList(pos, SnapShotData, &(Tcb->CopyLists.WrList4), Tcb);
    pos = CopyOneWriteList(pos, SnapShotData, &(Tcb->CopyLists.WrList2), Tcb);
    pos = CopyOneWriteList(pos, SnapShotData, &(Tcb->CopyLists.WrList1), Tcb);
    LeaveBlackboardCriticalSection();
    if (SnapShotSize != pos) {
        ThrowError (1, "wrong size of pipe snapshot %i != %i", SnapShotSize, pos);
        return 0;
    }
    return pos;
}


int DeReferenceAllVariables (TASK_CONTROL_BLOCK *Tcb)
{
    int x;

    for (x = 0; x < Tcb->CopyLists.List.ElemCount; x++) {
        if (Tcb->CopyLists.List.pElems[x].TypeBB == BB_UNKNOWN_WAIT) {
            remove_bbvari_unknown_wait (Tcb->CopyLists.List.pElems[x].Vid);
        } else {
            remove_bbvari (Tcb->CopyLists.List.pElems[x].Vid);
        }
    }
    Tcb->CopyLists.List.ElemCount = 0;
    return 0;
}


int GetReferencedLabelNameByVid (int Pid, int Vid, char *LabelName, int MaxChar)
{
    TASK_CONTROL_BLOCK *Tcb;
    int x;

    Tcb = GetPointerToTaskControlBlock (Pid);
    if (Tcb == NULL) {
        return -1;
    }
    for (x = 0; x <  Tcb->CopyLists.List.ElemCount; x++) {
        if (Tcb->CopyLists.List.pElems[x].Vid == Vid) {
            if (GetBlackboardVariableName (Tcb->CopyLists.List.pElems[x].Vid, LabelName, MaxChar)) {
                return -1;
            }
            return 0;
        }
    }
    return -1;
}

int GetCopyListForProcess (int par_Pid, int par_ListType, unsigned long long **ret_Addresses, int **ret_Vids, short **ret_TypeBB, short **ret_TypePipe)
{
    TASK_CONTROL_BLOCK *Tcb;
    PIPE_MESSAGE_REF_COPY_LIST *List1, *List2, *List4, *List8;
    int Size;

    if ((ret_Addresses == NULL) || (ret_Vids == NULL)) {
        return -1;
    }
    if (WaitUntilProcessIsNotActiveAndThanLockIt (par_Pid, 1000, ERROR_BEHAVIOR_ERROR_MESSAGE, "cannot lock process", __FILE__, __LINE__) == 0) {
        Tcb = GetPointerToTaskControlBlock (par_Pid);
        if (Tcb == NULL) {
            UnLockProcess(par_Pid);
            return -1;
        }
        switch (par_ListType) {
        case 0:
        default:  // main List
            List1 = &(Tcb->CopyLists.List);
            List2 = NULL;
            List4 = NULL;
            List8 = NULL;
            Size = List1->ElemCount;
            break;
        case 1:   // BB -> Pipe
            List1 = &(Tcb->CopyLists.RdList1);
            List2 = &(Tcb->CopyLists.RdList2);
            List4 = &(Tcb->CopyLists.RdList4);
            List8 = &(Tcb->CopyLists.RdList8);
            Size = List1->ElemCount + List2->ElemCount + List4->ElemCount + List8->ElemCount;
            break;
        case 2:   // Pipe -> BB
            List1 = &(Tcb->CopyLists.WrList1);
            List2 = &(Tcb->CopyLists.WrList2);
            List4 = &(Tcb->CopyLists.WrList4);
            List8 = &(Tcb->CopyLists.WrList8);
            Size = List1->ElemCount + List2->ElemCount + List4->ElemCount + List8->ElemCount;
            break;
        }
        *ret_Addresses = (unsigned long long*)my_malloc ((size_t)Size * sizeof (unsigned long long));
        *ret_Vids = (int*)my_malloc ((size_t)Size * sizeof (int));
        *ret_TypeBB = (short*)my_malloc ((size_t)Size * sizeof (short));
        *ret_TypePipe = (short*)my_malloc ((size_t)Size * sizeof (short));
        if ((*ret_Addresses == NULL) || (*ret_Vids == NULL) ||(*ret_TypeBB == NULL) || (*ret_TypePipe == NULL)) {
            if (*ret_Addresses != NULL) my_free (*ret_Addresses);
            if (*ret_Vids != NULL) my_free (*ret_Vids);
            if (*ret_TypeBB != NULL) my_free (*ret_TypeBB);
            if (*ret_TypePipe != NULL) my_free (*ret_TypePipe);
            ThrowError (1, "out of memory");
            UnLockProcess(par_Pid);
            return -1;
        } else {
            int x, y = 0;
            for (x = 0; x < List1->ElemCount; x++, y++) {
                (*ret_Addresses)[y] = List1->pElems[x].Addr;
                (*ret_Vids)[y] = List1->pElems[x].Vid;
                (*ret_TypeBB)[y] = List1->pElems[x].TypeBB;
                (*ret_TypePipe)[y] = List1->pElems[x].TypePipe;
            }
            if (List2 != NULL) {
                for (x = 0; x < List2->ElemCount; x++, y++) {
                    (*ret_Addresses)[y] = List2->pElems[x].Addr;
                    (*ret_Vids)[y] = List2->pElems[x].Vid;
                    (*ret_TypeBB)[y] = List2->pElems[x].TypeBB;
                    (*ret_TypePipe)[y] = List2->pElems[x].TypePipe;
                }
            }
            if (List4 != NULL) {
                for (x = 0; x < List4->ElemCount; x++, y++) {
                    (*ret_Addresses)[y] = List4->pElems[x].Addr;
                    (*ret_Vids)[y] = List4->pElems[x].Vid;
                    (*ret_TypeBB)[y] = List4->pElems[x].TypeBB;
                    (*ret_TypePipe)[y] = List4->pElems[x].TypePipe;
                }
            }
            if (List8 != NULL) {
                for (x = 0; x < List8->ElemCount; x++, y++) {
                    (*ret_Addresses)[y] = List8->pElems[x].Addr;
                    (*ret_Vids)[y] = List8->pElems[x].Vid;
                    (*ret_TypeBB)[y] = List8->pElems[x].TypeBB;
                    (*ret_TypePipe)[y] = List8->pElems[x].TypePipe;
                }
            }
        }
        UnLockProcess(par_Pid);
        return Size;
    }
    return -1;
}

static void FreeOneCopyList(PIPE_MESSAGE_REF_COPY_LIST *par_CopyList)
{
    if (par_CopyList->pElems != NULL) {
        my_free (par_CopyList->pElems);
    }
    STRUCT_ZERO_INIT(*par_CopyList, PIPE_MESSAGE_REF_COPY_LIST);
}

void FreeCopyLists (PIPE_MESSAGE_REF_COPY_LISTS *par_CopyLists)
{
    FreeOneCopyList(&par_CopyLists->List);
    FreeOneCopyList(&par_CopyLists->RdList1);
    FreeOneCopyList(&par_CopyLists->RdList2);
    FreeOneCopyList(&par_CopyLists->RdList4);
    FreeOneCopyList(&par_CopyLists->RdList8);
    FreeOneCopyList(&par_CopyLists->WrList1);
    FreeOneCopyList(&par_CopyLists->WrList2);
    FreeOneCopyList(&par_CopyLists->WrList4);
    FreeOneCopyList(&par_CopyLists->WrList8);
}
