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
#include <stdlib.h>
#include <stdio.h>
#ifndef _WIN32
#include <sys/mman.h>
#endif
#include "XilEnvExtProc.h"
#include "StringMaxChar.h"
#include "ExtpMemoryAllocation.h"
#include "ExtpMemoryAccess.h"
#include "PipeMessagesShared.h"
#include "ExtpProcessAndTaskInfos.h"
#include "ExtpBaseMessages.h"
#include "ExtpBlackboard.h"
#include "ExtpError.h"
#include "ExtpBlackboardCopyLists.h"


int XilEnvInternal_GetBbVarTypeSize (int Type)
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
    case BB_DOUBLE:
	case BB_QWORD:
	case BB_UQWORD:
        return 8;
    default:
        return 0;
    }    
}

unsigned long long XilEnvInternal_BuildRefUniqueId (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo)
{
    TaskInfo->CopyLists.List.RefUniqueIdCounter++;   // This should never overflow 64 bit
    return TaskInfo->CopyLists.List.RefUniqueIdCounter;
}

static int XilEnvInternal_InsertVariableToCopyListsRdOrWr (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, PIPE_MESSAGE_REF_COPY_LIST *pList, const char *Name, int Vid, unsigned long long Addr, int TypeBB, int TypePipe, int Dir, unsigned long long UniqueId)
{
    if (pList->ElemCount >= pList->ElemCountMax) {
        pList->ElemCountMax += 128;
        pList->pElems = (PIPE_MESSAGE_REF_COPY_LIST_ELEM*)XilEnvInternal_realloc (pList->pElems, pList->ElemCountMax * sizeof (PIPE_MESSAGE_REF_COPY_LIST_ELEM));
        if (pList->pElems == NULL) {
            XilEnvInternal_ThrowError (TaskInfos, 1, "out of memory cannot build copy lists BB <-> reference");
            return -1;
        }
    }
    pList->pElems[pList->ElemCount].Dir = 0x0000;
    if (Name == NULL) {
        pList->pElems[pList->ElemCount].UniqueIdOrNamneAndUniqueId.UniqueId = UniqueId;
    } else {
        int Len = strlen(Name) + 1;
        pList->pElems[pList->ElemCount].UniqueIdOrNamneAndUniqueId.NameAndUniqueId = (NAME_AND_UNIQUE_ID*)XilEnvInternal_malloc(sizeof(NAME_AND_UNIQUE_ID) + Len);
        if (pList->pElems[pList->ElemCount].UniqueIdOrNamneAndUniqueId.NameAndUniqueId != NULL) {
            pList->pElems[pList->ElemCount].Dir = 0x4000;
            pList->pElems[pList->ElemCount].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->UniqueId = UniqueId;
            pList->pElems[pList->ElemCount].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->AttachCounter = 1;
            StringCopyMaxCharTruncate(pList->pElems[pList->ElemCount].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->Name, Name, Len);
        }
    }
    pList->pElems[pList->ElemCount].Vid = Vid;
    pList->pElems[pList->ElemCount].Addr = Addr;
    pList->pElems[pList->ElemCount].TypeBB = (short)TypeBB;
    pList->pElems[pList->ElemCount].TypePipe = (short)TypePipe;
    pList->pElems[pList->ElemCount].Dir |= (short)Dir;
    pList->SizeInBytes += XilEnvInternal_GetBbVarTypeSize (TypePipe);
    pList->ElemCount++;
    return 0;
}

static int XilEnvInternal_CheckIfVariableIsInsideCopyLists (PIPE_MESSAGE_REF_COPY_LIST *pList, const char *Name, unsigned long long Addr, int TypeBB)
{
    int x;

    for (x = 0; x < pList->ElemCount; x++) {
        if (pList->pElems[x].Addr == Addr) {
            if ((pList->pElems[x].Dir & 0x4000) == 0x4000) {
                if (!strcmp(pList->pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->Name, Name)) {
                    if ((TypeBB == BB_GET_DATA_TYPE_FOM_DEBUGINFO) || (pList->pElems[x].TypeBB == TypeBB)) {
                        pList->pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->AttachCounter++;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

int XilEnvInternal_CheckIfVariableAreAlreadyInsideCopyLists (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfo, const char *Name, unsigned long long Addr, int TypeBB)
{
    return XilEnvInternal_CheckIfVariableIsInsideCopyLists (&(TaskInfo->CopyLists.List), Name, Addr, TypeBB);
}

int XilEnvInternal_InsertVariableToCopyLists (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, const char *Name, int Vid, void *Addr, int TypeBB, int TypePipe, int Dir, unsigned long long UniqueId)
{
    if (XilEnvInternal_InsertVariableToCopyListsRdOrWr (TaskInfos, &(TaskInfos->CopyLists.List), Name, Vid, (unsigned long long)Addr, TypeBB, TypePipe, Dir, UniqueId)) {
        return -1;
    }
    if ((Dir & PIPE_API_REFERENCE_VARIABLE_DIR_READ) == PIPE_API_REFERENCE_VARIABLE_DIR_READ) {
    	switch (XilEnvInternal_GetBbVarTypeSize(TypePipe)) {
    	case 1:
			if (XilEnvInternal_InsertVariableToCopyListsRdOrWr (TaskInfos, &(TaskInfos->CopyLists.RdList1), NULL, Vid, (unsigned long long)Addr, TypeBB, TypePipe, Dir, UniqueId)) {
				return -1;
			}
			break;
    	case 2:
			if (XilEnvInternal_InsertVariableToCopyListsRdOrWr (TaskInfos, &(TaskInfos->CopyLists.RdList2), NULL, Vid, (unsigned long long)Addr, TypeBB, TypePipe, Dir, UniqueId)) {
				return -1;
			}
			break;
    	case 4:
			if (XilEnvInternal_InsertVariableToCopyListsRdOrWr (TaskInfos, &(TaskInfos->CopyLists.RdList4), NULL, Vid, (unsigned long long)Addr, TypeBB, TypePipe, Dir, UniqueId)) {
				return -1;
			}
			break;
    	case 8:
			if (XilEnvInternal_InsertVariableToCopyListsRdOrWr (TaskInfos, &(TaskInfos->CopyLists.RdList8), NULL, Vid, (unsigned long long)Addr, TypeBB, TypePipe, Dir, UniqueId)) {
				return -1;
			}
			break;
    	default:
    		return -1;
		}
    }
    if ((Dir & PIPE_API_REFERENCE_VARIABLE_DIR_WRITE) == PIPE_API_REFERENCE_VARIABLE_DIR_WRITE) {
        int Size = XilEnvInternal_GetBbVarTypeSize(TypePipe);
        if (XilEnvInternal_EnableWriteAccressToMemory(Addr, Size)) {
            return -1;
        }
        switch (Size) {
    	case 1:
			if (XilEnvInternal_InsertVariableToCopyListsRdOrWr (TaskInfos, &(TaskInfos->CopyLists.WrList1), NULL, Vid, (unsigned long long)Addr, TypeBB, TypePipe, Dir, UniqueId)) {
				return -1;
			}
			break;
    	case 2:
			if (XilEnvInternal_InsertVariableToCopyListsRdOrWr (TaskInfos, &(TaskInfos->CopyLists.WrList2), NULL, Vid, (unsigned long long)Addr, TypeBB, TypePipe, Dir, UniqueId)) {
				return -1;
			}
			break;
    	case 4:
			if (XilEnvInternal_InsertVariableToCopyListsRdOrWr (TaskInfos, &(TaskInfos->CopyLists.WrList4), NULL, Vid, (unsigned long long)Addr, TypeBB, TypePipe, Dir, UniqueId)) {
				return -1;
			}
			break;
    	case 8:
			if (XilEnvInternal_InsertVariableToCopyListsRdOrWr (TaskInfos, &(TaskInfos->CopyLists.WrList8), NULL, Vid, (unsigned long long)Addr, TypeBB, TypePipe, Dir, UniqueId)) {
				return -1;
			}
			break;
    	default:
    		return -1;
    	}
    }
    return 0;
}

// If Name == NULL search by Vid otherwise search by Name.
// returns the Vid if reference was deleted.
// returns == 0 only the attach counter was decremented.
// return -1 the references doesn't exists.
static int XilEnvInternal_RemoveVariableFromCopyListsRdOrWr (PIPE_MESSAGE_REF_COPY_LIST *pList, const char *Name, int Vid, void *Addr, int *ret_TypeBB, int *ret_TypePipe, int *ret_RangeControlFlag, unsigned long long *ret_UniqueId)
{
    int x;
    int Ret = -1;

    for (x = 0; x < pList->ElemCount; x++) {
        if (pList->pElems[x].Addr == (unsigned long long)Addr) {
            if ((pList->pElems[x].Dir & 0x4000) == 0x4000) { // it is an extended (with name) element
                if (Name != NULL) {
                    // search by name
                    if (!strcmp(pList->pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->Name, Name)) {
                        if (ret_UniqueId != NULL) *ret_UniqueId = pList->pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->UniqueId;
                        pList->pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->AttachCounter--;
                        if (pList->pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->AttachCounter == 0) {
                            XilEnvInternal_free (pList->pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId);
                            Ret = pList->pElems[x].Vid;
                        } else {
                            Ret = 0;
                        }
                        break;   // for (;;)  a matchting reference found
                    }
                } else {
                    // search by vid
                    if (pList->pElems[x].Vid == Vid) {
                        if (ret_UniqueId != NULL) *ret_UniqueId = pList->pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->UniqueId;
                        Ret = pList->pElems[x].Vid;
                        break;   // for (;;)  a matchting reference found
                    }
                }
            } else {
                if (pList->pElems[x].Vid == Vid) {
                    if (ret_UniqueId != NULL) *ret_UniqueId = pList->pElems[x].UniqueIdOrNamneAndUniqueId.UniqueId;
                    Ret = pList->pElems[x].Vid;
                    break;   // for (;;)  a matchting reference found
                } 
            }
        }
    }
    if (Ret >= 0) {
        // a reference was found
        if (ret_TypeBB != NULL) *ret_TypeBB = pList->pElems[x].TypeBB;
        if (ret_TypePipe != NULL) *ret_TypePipe = pList->pElems[x].TypePipe;
        if (ret_RangeControlFlag != NULL) *ret_RangeControlFlag = pList->pElems[x].RangeControlFlag;
        if (Ret > 0) {
            // a reference was deleted
            pList->SizeInBytes -= XilEnvInternal_GetBbVarTypeSize (pList->pElems[x].TypePipe);
            for (x = x + 1; x < pList->ElemCount; x++) {
                pList->pElems[x - 1] = pList->pElems[x];
            }
            pList->ElemCount--;
        }
    }
    return Ret;
}

static int XilEnvInternal_RemoveVariableFromCopyListsRdOrWrByUniqueId (PIPE_MESSAGE_REF_COPY_LIST *pList, unsigned long long UniqueId, void *Addr)
{
    int x;
    int Ret = -1;

    for (x = 0; x < pList->ElemCount; x++) {
        if (pList->pElems[x].Addr == (unsigned long long)Addr) {
            if ((pList->pElems[x].Dir & 0x4000) == 0x4000) { // it is an extended (with name) element
                if (UniqueId == pList->pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->UniqueId) {
                    pList->pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->AttachCounter--;
                    if (pList->pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->AttachCounter == 0) {
                        XilEnvInternal_free (pList->pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId);
                        Ret = pList->pElems[x].Vid;
                    } else {
                        Ret = 0;
                    }
                    break;   // for (;;)  a matchting reference found
                }
            } else {
                if (UniqueId == pList->pElems[x].UniqueIdOrNamneAndUniqueId.UniqueId) {
                    Ret = pList->pElems[x].Vid;
                    break;   // for (;;)  a matchting reference found
                }
            }
        }
    }
    if (Ret > 0) {
        // a reference was deleted
        pList->SizeInBytes -= XilEnvInternal_GetBbVarTypeSize (pList->pElems[x].TypePipe);
        for (x = x + 1; x < pList->ElemCount; x++) {
            pList->pElems[x - 1] = pList->pElems[x];
        }
        pList->ElemCount--;
    }
    return Ret;
}


// returns the Vid if reference was deleted.
// returns == 0 only the attach counter was decremented.
// return -1 the references doesn't exists.
int XilEnvInternal_RemoveVariableFromCopyLists (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, const char *Name, int Vid, void *Addr, int *ret_Dir, int *ret_TypeBB, int *ret_TypePipe, int *ret_RangeControlFlag, unsigned long long *ret_UniqueId)
{
    int Dir = 0;
    int Ret;
    Ret = XilEnvInternal_RemoveVariableFromCopyListsRdOrWr (&(TaskInfos->CopyLists.List), Name, Vid, Addr, ret_TypeBB, ret_TypePipe, ret_RangeControlFlag, ret_UniqueId);
    if (Ret > 0) {  // the reference was deleted?
        if ((XilEnvInternal_RemoveVariableFromCopyListsRdOrWrByUniqueId (&(TaskInfos->CopyLists.RdList1), *ret_UniqueId, Addr) == 0) ||
    	    (XilEnvInternal_RemoveVariableFromCopyListsRdOrWrByUniqueId (&(TaskInfos->CopyLists.RdList2), *ret_UniqueId, Addr) == 0) ||
		    (XilEnvInternal_RemoveVariableFromCopyListsRdOrWrByUniqueId (&(TaskInfos->CopyLists.RdList4), *ret_UniqueId, Addr) == 0) ||
		    (XilEnvInternal_RemoveVariableFromCopyListsRdOrWrByUniqueId (&(TaskInfos->CopyLists.RdList8), *ret_UniqueId, Addr) == 0)) {
            Dir |= PIPE_API_REFERENCE_VARIABLE_DIR_READ;
        }
        if ((XilEnvInternal_RemoveVariableFromCopyListsRdOrWrByUniqueId (&(TaskInfos->CopyLists.WrList1), *ret_UniqueId, Addr) == 0) ||
    	    (XilEnvInternal_RemoveVariableFromCopyListsRdOrWrByUniqueId (&(TaskInfos->CopyLists.WrList2), *ret_UniqueId, Addr) == 0) ||
		    (XilEnvInternal_RemoveVariableFromCopyListsRdOrWrByUniqueId (&(TaskInfos->CopyLists.WrList4), *ret_UniqueId, Addr) == 0) ||
		    (XilEnvInternal_RemoveVariableFromCopyListsRdOrWrByUniqueId (&(TaskInfos->CopyLists.WrList8), *ret_UniqueId, Addr) == 0)) {
            Dir |= PIPE_API_REFERENCE_VARIABLE_DIR_WRITE;
        }
    }
    *ret_Dir = Dir;

    return Ret;
}

void XilEnvInternal_CopyWithCheckOverlayBuffer (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, void *Address, int Size, void *Src)
{
    DWORD OldFlags;
    int b, x, found;

    for (b = 0; b < Size; b++) {   // each to copy byte alone
        found = 0;
        for (x = 0; x < TaskInfos->OverlayBuffers.Count; x++) { // check if it is inside a memory region
            if ((((char*)Address + b) >= (char*)TaskInfos->OverlayBuffers.Entrys[x].FirstAddress) &&  
                (((char*)Address + b) <= (char*)TaskInfos->OverlayBuffers.Entrys[x].LastAddress)) {
                found = 1;
                break;
            }
        }
        if (!found) {
            if (((char*)Address)[b] != ((char*)Src)[b]) {
                ((char*)Address)[b] = ((char*)Src)[b];
            }
        }
    }
}

int XilEnvInternal_CopyFromPipeToRefWithOverlayCheck (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, char *SnapShotData, int SnapShotSize)
{
    int x, pos, size;
    
    size = TaskInfos->CopyLists.RdList8.SizeInBytes + TaskInfos->CopyLists.RdList4.SizeInBytes + TaskInfos->CopyLists.RdList2.SizeInBytes + TaskInfos->CopyLists.RdList1.SizeInBytes;
    if (SnapShotSize != size) {
        XilEnvInternal_ThrowError (TaskInfos, 1, "wrong size of pipe snapshot %i != %i", SnapShotSize, size);
        return -1;
    }
    for (pos = x = 0; x < TaskInfos->CopyLists.RdList8.ElemCount; x++) {
        XilEnvInternal_CopyWithCheckOverlayBuffer (TaskInfos, (void*)TaskInfos->CopyLists.RdList8.pElems[x].Addr, 8, SnapShotData + pos);
        pos+=8;
    }
    for (x = 0; x < TaskInfos->CopyLists.RdList4.ElemCount; x++) {
        XilEnvInternal_CopyWithCheckOverlayBuffer (TaskInfos, (void*)TaskInfos->CopyLists.RdList4.pElems[x].Addr, 4, SnapShotData + pos);
        pos+=4;
    }
    for (x = 0; x < TaskInfos->CopyLists.RdList2.ElemCount; x++) {
        XilEnvInternal_CopyWithCheckOverlayBuffer (TaskInfos, (void*)TaskInfos->CopyLists.RdList2.pElems[x].Addr, 2, SnapShotData + pos);
        pos+=2;
    }
    for (x = 0; x < TaskInfos->CopyLists.RdList1.ElemCount; x++) {
        XilEnvInternal_CopyWithCheckOverlayBuffer (TaskInfos, (void*)TaskInfos->CopyLists.RdList1.pElems[x].Addr, 1, SnapShotData + pos);
        pos++;
    }
    TaskInfos->OverlayBuffers.Count = 0;    // reset overlay buffer
    if (SnapShotSize != pos) {
        XilEnvInternal_ThrowError (TaskInfos, 1, "wrong size of pipe snapshot %i != %i", SnapShotSize, pos);
        return -1;
    }
    return pos;
}


int XilEnvInternal_CopyFromPipeToRef (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, char *SnapShotData, int SnapShotSize)
{
    int x, pos, size;

    size = TaskInfos->CopyLists.RdList8.SizeInBytes + TaskInfos->CopyLists.RdList4.SizeInBytes + TaskInfos->CopyLists.RdList2.SizeInBytes + TaskInfos->CopyLists.RdList1.SizeInBytes;
    if (SnapShotSize != size) {
        XilEnvInternal_ThrowError (TaskInfos, 1, "wrong size of pipe snapshot %i != %i", SnapShotSize, size);
        return -1;
    }
    for (pos = x = 0; x < TaskInfos->CopyLists.RdList8.ElemCount; x++) {
        if ( *(int64_t*)(TaskInfos->CopyLists.RdList8.pElems[x].Addr) != *(int64_t*)(SnapShotData + pos)) {
            *(int64_t*)TaskInfos->CopyLists.RdList8.pElems[x].Addr = *(int64_t*)(SnapShotData + pos);
        }
        pos+=8;
    }
    for (x = 0; x < TaskInfos->CopyLists.RdList4.ElemCount; x++) {
        if ( *(int32_t*)(TaskInfos->CopyLists.RdList4.pElems[x].Addr) != *(int32_t*)(SnapShotData + pos)) {
            *(int32_t*)TaskInfos->CopyLists.RdList4.pElems[x].Addr = *(int32_t*)(SnapShotData + pos);
        }
        pos+=4;
    }
    for (x = 0; x < TaskInfos->CopyLists.RdList2.ElemCount; x++) {
        if ( *(signed short*)(TaskInfos->CopyLists.RdList2.pElems[x].Addr) != *(signed short*)(SnapShotData + pos)) {
            *(signed short*)TaskInfos->CopyLists.RdList2.pElems[x].Addr = *(signed short*)(SnapShotData + pos);
        }
        pos+=2;
    }
    for (x = 0; x < TaskInfos->CopyLists.RdList1.ElemCount; x++) {
        if ( *(signed char*)(TaskInfos->CopyLists.RdList1.pElems[x].Addr) != *(signed char*)(SnapShotData + pos)) {
            *(signed char*)TaskInfos->CopyLists.RdList1.pElems[x].Addr = *(signed char*)(SnapShotData + pos);
        }
        pos++;
    }
    if (SnapShotSize != pos) {
        XilEnvInternal_ThrowError (TaskInfos, 1, "wrong size of pipe snapshot %i != %i", SnapShotSize, pos);
        return -1;
    }
    return pos;
}


int XilEnvInternal_CopyFromRefToPipe (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, char *SnapShotData) //, int SnapShotSize)
{
    int x, pos;

    for (pos = x = 0; x < TaskInfos->CopyLists.WrList8.ElemCount; x++) {
        *(int64_t*)(SnapShotData + pos) = *(int64_t*)TaskInfos->CopyLists.WrList8.pElems[x].Addr;
        pos += 8;
    }
    for (x = 0; x < TaskInfos->CopyLists.WrList4.ElemCount; x++) {
        *(int32_t*)(SnapShotData + pos) = *(int32_t*)TaskInfos->CopyLists.WrList4.pElems[x].Addr;
        pos += 4;
    }
    for (x = 0; x < TaskInfos->CopyLists.WrList2.ElemCount; x++) {
        *(int16_t*)(SnapShotData + pos) = *(int16_t*)TaskInfos->CopyLists.WrList2.pElems[x].Addr;
        pos += 2;
    }
    for (x = 0; x < TaskInfos->CopyLists.WrList1.ElemCount; x++) {
        *(int8_t*)(SnapShotData + pos) = *(int8_t*)TaskInfos->CopyLists.WrList1.pElems[x].Addr;
        pos++;
    }
    return pos;
}


int XilEnvInternal_DeReferenceAllVariables (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos)
{
    int x;

    for (x = TaskInfos->CopyLists.List.ElemCount-1; x >= 0; x--) {  // stating from the end (that is faster)
        unsigned long long UniqueId;
        if ((TaskInfos->CopyLists.List.pElems[x].Dir & 0x4000) == 0x4000) {
            UniqueId = TaskInfos->CopyLists.List.pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId->UniqueId;
            XilEnvInternal_free(TaskInfos->CopyLists.List.pElems[x].UniqueIdOrNamneAndUniqueId.NameAndUniqueId);
        } else {
            UniqueId = TaskInfos->CopyLists.List.pElems[x].UniqueIdOrNamneAndUniqueId.UniqueId;
        }
        XilEnvInternal_PipeRemoveBlackboardVariable (TaskInfos, TaskInfos->CopyLists.List.pElems[x].Vid, TaskInfos->CopyLists.List.pElems[x].RangeControlFlag, TaskInfos->CopyLists.List.pElems[x].TypeBB,
                                                      TaskInfos->CopyLists.List.pElems[x].Addr, UniqueId);
    }
    TaskInfos->CopyLists.RdList8.ElemCount = 0;
    TaskInfos->CopyLists.WrList8.ElemCount = 0;
    TaskInfos->CopyLists.RdList4.ElemCount = 0;
    TaskInfos->CopyLists.WrList4.ElemCount = 0;
    TaskInfos->CopyLists.RdList2.ElemCount = 0;
    TaskInfos->CopyLists.WrList2.ElemCount = 0;
    TaskInfos->CopyLists.RdList1.ElemCount = 0;
    TaskInfos->CopyLists.WrList1.ElemCount = 0;
    TaskInfos->CopyLists.List.ElemCount = 0;
    return 0;
}

int XilEnvInternal_GetReferencedVariableIdentifierNameByAddress (void *Address)
{
    int x;
    EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos;
    int Ret = -1;

    TaskInfos = XilEnvInternal_GetTaskPtr ();
    if (TaskInfos == NULL) return -1;
    for (x = 0; x < TaskInfos->CopyLists.List.ElemCount; x++) {
        if (TaskInfos->CopyLists.List.pElems[x].Addr == (unsigned long long)Address) {
            Ret = TaskInfos->CopyLists.List.pElems[x].Vid;
            break;
        }
    }
    XilEnvInternal_ReleaseTaskPtr(TaskInfos);
    return Ret;
}

int XilEnvInternal_AddAddressToOverlayBuffer (EXTERN_PROCESS_TASK_INFOS_STRUCT *TaskInfos, void *Address, int Size)
{
    if (TaskInfos->OverlayBuffers.Size <= TaskInfos->OverlayBuffers.Count) {
        TaskInfos->OverlayBuffers.Size += 1024;
        TaskInfos->OverlayBuffers.Entrys = (REFERENCE_OVERLAY_BUFFER*)XilEnvInternal_realloc (TaskInfos->OverlayBuffers.Entrys, TaskInfos->OverlayBuffers.Size * sizeof (REFERENCE_OVERLAY_BUFFER));
        if (TaskInfos->OverlayBuffers.Entrys == NULL) {
            TaskInfos->OverlayBuffers.Size = 0;
            XilEnvInternal_ThrowError (TaskInfos, 1, "out of memory");
            return -1;
        }
    }
    TaskInfos->OverlayBuffers.Entrys[TaskInfos->OverlayBuffers.Count].FirstAddress = Address;
    TaskInfos->OverlayBuffers.Entrys[TaskInfos->OverlayBuffers.Count].LastAddress = (char*)Address + (Size - 1);
    TaskInfos->OverlayBuffers.Count++;
    return 0;
}

EXPORT_OR_IMPORT int  __FUNC_CALL_CONVETION__  get_vid_by_reference_address (void *ptr)
{
    return XilEnvInternal_GetReferencedVariableIdentifierNameByAddress (ptr);
}
