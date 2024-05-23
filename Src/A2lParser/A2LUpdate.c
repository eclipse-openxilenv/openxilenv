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


#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "ThrowError.h"
#include "MyMemory.h"
#include "Files.h"
#include "Blackboard.h"
#include "Scheduler.h"
#include "DebugInfoDB.h"
#include "DebugInfoAccessExpression.h"
#include "DwarfReader.h"
#include "UniqueNumber.h"
#include "A2LParser.h"
#include "A2LAccess.h"
#include "A2LLink.h"
#include "A2LUpdate.h"

#define UNUSED(x) (void)(x)

// must be Sorted by FileOffset.
typedef struct {
    uint32_t FileOffset;
    uint32_t Fill;
    uint64_t Address;
} A2L_ADDRESS_ELEMENT;

typedef struct {
    A2L_ADDRESS_ELEMENT *Elements;
    int Size;
    int Pos;
} A2L_ADDRESS_LIST;


static int CmpAddressListElemnets (const void *a, const void *b)
{
    if (((const A2L_ADDRESS_ELEMENT*)a)->FileOffset < ((const A2L_ADDRESS_ELEMENT*)b)->FileOffset) return -1;
    else if (((const A2L_ADDRESS_ELEMENT*)a)->FileOffset == ((const A2L_ADDRESS_ELEMENT*)b)->FileOffset) return 0;
    else return 1;
}

static void SortAddressList(A2L_ADDRESS_LIST *par_List)
{
    qsort (par_List->Elements, (size_t)par_List->Pos, sizeof (A2L_ADDRESS_ELEMENT), CmpAddressListElemnets);
}

static int SetupAddressList(A2L_ADDRESS_LIST *par_List)
{
    par_List->Pos = 0;
    par_List->Size = 1024;
    par_List->Elements = (A2L_ADDRESS_ELEMENT*)my_malloc(par_List->Size * sizeof(A2L_ADDRESS_ELEMENT));
    if (par_List->Elements == NULL)  return -1;
    else return 0;
}

static void RemoveAddressList(A2L_ADDRESS_LIST *par_List)
{
    par_List->Pos = 0;
    par_List->Size = 0;
    if (par_List->Elements != NULL) {
        my_free(par_List->Elements);
        par_List->Elements = NULL;
    }
}

static int AddAddressElement(A2L_ADDRESS_LIST *par_List, uint64_t par_Address, uint32_t par_FileOffset)
{
    if (par_List->Pos >= par_List->Size) {
        par_List->Size += 1024;
        par_List->Elements = (A2L_ADDRESS_ELEMENT*)my_realloc(par_List->Elements, par_List->Size * sizeof(A2L_ADDRESS_ELEMENT));
        if (par_List->Elements == NULL)  return -1;
    }
    par_List->Elements[par_List->Pos].Address = par_Address;
    par_List->Elements[par_List->Pos].FileOffset = par_FileOffset;
    par_List->Pos++;
    return 0;
}

static uint64_t GetAddressByA2LLabelFromExtProc(DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata, const char *par_A2LLabel)
{
    char Label[1024];
    const char *s = par_A2LLabel;
    char *d = Label;
    uint64_t Ret;
    int32_t Typenr;

    while (*s != 0) {
        switch(*s) {
        case '_':
            if (s > par_A2LLabel) {
                if (*(s-1) == '.') {
                    if (*(s+1) == '.') {
                        d--;  // replace "._." with "::"
                        *d = ':';
                        d++;
                        *d = ':';
                        d++;
                        s+=2;
                        continue;
                    } else {
                        unsigned int Idx;
                        char *End = NULL;
                        Idx = strtoul(s+1, &End, 10);
                        if (End > (s+1)) {
                            if (*End == '_') {
                                // now we are sure we have found an array
                                // replace "._X_." with "[X]"
                                s = End+1;
                                d--;  // '.'
                                sprintf(d, "[%i]", Idx);
                                d += strlen(d);
                                continue;
                            }
                        }
                    }
                }
            }
            *d++ = *s++;
            break;
        default:
            *d++ = *s++;
            break;
        }
    }
    *d = 0;
    if (appl_label_already_locked (pappldata, pappldata->AssociatedProcess->Pid, Label, &Ret, &Typenr, 1, 0)) {
        Ret = 0;  // not found
    }
    return Ret;
}


// must be Sorted by Name
typedef struct {
    char *Name;
    uint64_t Address;
} SYM_ELEMENT;

typedef struct {
    char *Buffer;
    int BufferSize;
    int BufferPos;
    SYM_ELEMENT *Elements;
    int Size;
    int Pos;
} SYM_FILE;

typedef struct {
    int NotUpdateFileValid;
    char FileName[MAX_PATH];
    FILE *Handle;
    uint64_t BaseAddress;
    uint64_t MinusOffset;
    uint64_t PlusOffset;
} NOT_UPDATED_LABEL_FILE;

static uint64_t GetAddressByA2LLabelFromSymFile(SYM_FILE *par_SymFile, const char *par_A2LLabel)
{
    UNUSED(par_SymFile);
    UNUSED(par_A2LLabel);
    return 0;
}

static uint64_t GetAddressByA2LLabel(SYM_FILE *par_SymFile, DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata, const char *par_A2LLabel, NOT_UPDATED_LABEL_FILE *par_NotUpdatedLabelFile)
{
    uint64_t Ret;
    if (pappldata != NULL) Ret = GetAddressByA2LLabelFromExtProc(pappldata, par_A2LLabel);
    else if (par_SymFile != NULL) Ret = GetAddressByA2LLabelFromSymFile(par_SymFile, par_A2LLabel);
    else Ret = 0;
    if (Ret == 0) {
        if ((par_NotUpdatedLabelFile != NULL) && par_NotUpdatedLabelFile->NotUpdateFileValid) {
            if (par_NotUpdatedLabelFile->Handle == NULL) {
                par_NotUpdatedLabelFile->Handle = open_file(par_NotUpdatedLabelFile->FileName, "wt");
            }
            if (par_NotUpdatedLabelFile->Handle != NULL) {
                fprintf (par_NotUpdatedLabelFile->Handle, "%s\n", par_A2LLabel);
            } else {
                par_NotUpdatedLabelFile->NotUpdateFileValid = 0;
            }
        }
    } else {
        Ret = (Ret - par_NotUpdatedLabelFile->MinusOffset) + par_NotUpdatedLabelFile->PlusOffset;
        // TODO
        Ret = Ret - par_NotUpdatedLabelFile->BaseAddress;
    }
    return Ret;
}

// return == 0 -> new address is replaced and fit inside old address
//         > 0 -> new address is replaced and don't fit in the old address space and extend the file with n bytes.
//         < 0 -> Address is not replaced.
static int ReplaceAddress(FILE_CACHE *par_Cache, unsigned int par_Offset, uint64_t par_Address,
                          char *ret_ReplaceBuffer, unsigned int par_MaxChar,
                          unsigned int *ret_CutStart, unsigned int *ret_CutSize, unsigned int *ret_ReplaceSize)
{
    char NewAddressString[32];
    unsigned int NeededSpace, OldSpace, CutSpace;
    uint64_t OldAddress;
    char *EndPtr = NULL;
    unsigned int i, x;

    sprintf (NewAddressString, "0x%" PRIX64 "", par_Address);
    NeededSpace = (unsigned int)strlen(NewAddressString);
    OldAddress = strtoull((char*)par_Cache->Buffer + par_Offset, &EndPtr, 0);
    if (EndPtr <= (char*)par_Cache->Buffer) return -1;
    OldSpace = (unsigned int)(EndPtr - (char*)par_Cache->Buffer) - par_Offset;

    if (NeededSpace == OldSpace) {
        for (i = 0; (i < NeededSpace) && (i < par_MaxChar); i++) {
            ret_ReplaceBuffer[i] = NewAddressString[i];
        }
        *ret_CutStart = par_Offset;
        *ret_CutSize = OldSpace;
        *ret_ReplaceSize = OldSpace;
        return 0;
    } else if (NeededSpace < OldSpace) {
        for (x = 0; (x < (OldSpace - NeededSpace)) && (x < par_MaxChar); x++) {
            ret_ReplaceBuffer[x] = ' ';
        }
        for (i = 0; (i < NeededSpace) && (x < par_MaxChar); i++, x++) {
            ret_ReplaceBuffer[x] = NewAddressString[i];
        }
        *ret_CutStart = par_Offset;
        *ret_CutSize = OldSpace;
        *ret_ReplaceSize = OldSpace;
        return 0;
    } else /*if (NeededSpace > OldSpace)*/ {
        // look if some whitespace are infront of.
        int ws_before = 0;
        int ws_behind = 0;
        int diff = NeededSpace - OldSpace;
        for (x = par_Offset; (x > 1) && (par_Cache->Buffer[x-1] == ' '); x--) {
            ws_before++;
        }
        for (x = par_Offset + OldSpace; (x < par_Cache->Size) && (par_Cache->Buffer[x] == ' '); x++) {
            ws_behind++;
        }
        for (i = 0; (i < NeededSpace) && (i < par_MaxChar); i++) {
            ret_ReplaceBuffer[i] = NewAddressString[i];
        }

        CutSpace = OldSpace;
        // try to remove the whitespaces before
        if (ws_before >= 2) {
            if (diff <= (ws_before)) {
                par_Offset -= diff;      // fit all before
                *ret_CutStart = par_Offset;
                *ret_CutSize = NeededSpace;
                *ret_ReplaceSize = NeededSpace;
                return 0;
            } else {
                CutSpace += ws_before - 1;
                par_Offset -= ws_before - 1;      // fit not all before
            }
        }

        // try to remove the whitespaces behind
        if (ws_behind >= 2) {
            if (diff <= (ws_behind)) {
                par_Offset -= diff;      // fit all before
                *ret_CutStart = par_Offset;
                *ret_CutSize = NeededSpace;
                *ret_ReplaceSize = NeededSpace;
                return 0;
            } else {
                CutSpace += ws_before - 1;
                par_Offset -= ws_behind - 1;      // fit not all behind
            }
        }

        // now we have to extend the file!
        *ret_CutStart = par_Offset;
        *ret_CutSize = CutSpace;
        *ret_ReplaceSize = NeededSpace;
        return NeededSpace - CutSpace;
    }
}


static int WriteA2LWithUpdateAddresses(ASAP2_DATABASE *Database, const char *par_OutA2LFile, A2L_ADDRESS_LIST *par_AddrList, const char **ret_ErrString)
{
    ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
    unsigned int FilePos = 0;
    unsigned int BlockSize;
    int AddressListPos = 0;
    HANDLE hFile;
    DWORD Written;
    int Ret = 0;

#ifdef _WIN32
        hFile = CreateFile(par_OutA2LFile,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        if (hFile == (HANDLE)(HFILE_ERROR)) {
            *ret_ErrString = "cannot open file";
            return -1;
        }
#else
        hFile = open(par_OutA2LFile, O_CREAT | O_WRONLY);
        if (hFile < 0) {
            *ret_ErrString = "cannot open file";
            return -1;
        }
#endif

    while (FilePos < Parser->Cache->Size) {
        if (AddressListPos < par_AddrList->Pos) {
            char ReplaceBuffer[256];
            unsigned int CutStart;
            unsigned int CutSize;
            unsigned int ReplaceSize;
            int Code = ReplaceAddress(Parser->Cache,
                                      par_AddrList->Elements[AddressListPos].FileOffset,
                                      par_AddrList->Elements[AddressListPos].Address,
                                      ReplaceBuffer, sizeof(ReplaceBuffer),
                                      &CutStart, &CutSize, &ReplaceSize);
            if(Code < 0) return -1;
            BlockSize = CutStart - FilePos;
            if (!WriteFile(hFile, Parser->Cache->Buffer + FilePos, BlockSize, &Written, NULL) ||
                (BlockSize != Written)) {
                *ret_ErrString = "cannot write to file";
                Ret =  -1;
                break;
            }
            FilePos += BlockSize;
            if (!WriteFile(hFile, ReplaceBuffer, ReplaceSize, &Written, NULL) ||
                (ReplaceSize != Written)) {
                *ret_ErrString = "cannot write to file";
                Ret =  -1;
                break;
            }
            FilePos += CutSize;
            AddressListPos++;
        } else {
            // at the end the file tail
            BlockSize = Parser->Cache->Size - FilePos;
            if (!WriteFile(hFile, Parser->Cache->Buffer + FilePos, BlockSize, &Written, NULL) ||
                (BlockSize != Written)) {
                *ret_ErrString = "cannot write to file";
                Ret =  -1;
                break;
            }
            FilePos += BlockSize;
        }
    }
#ifdef _WIN32
    CloseHandle(hFile);
#else
    close(hFile);
#endif
    return Ret;
}

static ASAP2_MODULE_DATA * GetModule(ASAP2_DATABASE *Database)
{
    if (Database == NULL) return NULL;
    ASAP2_PARSER* Parser = (ASAP2_PARSER*)Database->Asap2Ptr;
    if (Parser == NULL) return NULL;
    if (Parser->Data.Modules == NULL) return NULL;
    return Parser->Data.Modules[Database->SelectedModul];
}

int A2LUpdate(ASAP2_DATABASE *Database, const char *par_OutA2LFile, const char *par_SourceType, int par_Flags, const char *par_Source,
              uint64_t par_MinusOffset, uint64_t par_PlusOffset,
              const char *par_NotUpdatedLabelFile, const char **ret_ErrString)
{
    int Ret = 0;
    A2L_ADDRESS_LIST AddrList;
    int i, x;
    DEBUG_INFOS_ASSOCIATED_CONNECTION *pappldata = NULL;
    SYM_FILE *SymFile = NULL;
    int Pid = -1;
    int UniqueId;
    int LoadedDebugInfosFlag = 0;
    NOT_UPDATED_LABEL_FILE NotUpdatedLabelFile;

    AddrList.Elements = NULL;

    memset (&NotUpdatedLabelFile, 0, sizeof(NotUpdatedLabelFile));
    if (par_NotUpdatedLabelFile != NULL) {
        NotUpdatedLabelFile.NotUpdateFileValid = 1;
        strncpy(NotUpdatedLabelFile.FileName, par_NotUpdatedLabelFile, sizeof(NotUpdatedLabelFile.FileName)-1);
    }
    NotUpdatedLabelFile.MinusOffset = par_MinusOffset;
    NotUpdatedLabelFile.PlusOffset = par_PlusOffset;

    if ((Database != NULL) && (par_Source != NULL)) {
        ASAP2_MODULE_DATA* Module = GetModule(Database);
        if (Module != NULL) {
            if (strcmpi(par_SourceType, "PROCESS") == 0) {
                Pid = get_pid_by_name(par_Source);
                if (Pid > 0) {
                    uint64_t BaseAddress = 0;
                    if (GetBaseAddressOf(Pid, par_Flags, &BaseAddress)) {
                        *ret_ErrString = "cannot determine base address";
                        return -1;
                    }
                    NotUpdatedLabelFile.BaseAddress = BaseAddress;

                    if (WaitUntilProcessIsNotActiveAndThanLockIt (Pid, 5000, ERROR_BEHAVIOR_ERROR_MESSAGE, "cannot lock process", __FILE__, __LINE__) == 0) {

                        UniqueId = AquireUniqueNumber();
                        if (UniqueId < 0) {
                            ThrowError (ERROR_SYSTEM_CRITICAL, "out of unique ids stop!");
                            return -1;
                        }
                        pappldata = ConnectToProcessDebugInfos (UniqueId,
                                                                par_Source, &Pid,
                                                                NULL,
                                                                NULL,
                                                                NULL,
                                                                0);
                        if (pappldata == NULL) {
                            FreeUniqueNumber (UniqueId);
                            ThrowError (ERROR_SYSTEM_CRITICAL, "no debug infos for process \"%s\"", par_Source);
                            return -1;
                        }
                    } else {
                        return -1;
                    }
                } else {
                    return -1;
                }
            } else if ((strcmpi(par_SourceType, "EXE") == 0) ||
                       (strcmpi(par_SourceType, "ELF") == 0)) {
                pappldata = ReadDebufInfosNotConnectingToProcess(par_SourceType, par_Source);
                if (pappldata == NULL) {
                    ThrowError (ERROR_SYSTEM_CRITICAL, "cannot read debug infos from \"%s\"", par_Source);
                    return -1;
                }
                LoadedDebugInfosFlag = 1;
            } else {
                *ret_ErrString = "unknown source type (PROCESS, EXE or ELF allowed)";
                return -1;
            }

            if (SetupAddressList(&AddrList)) {
                *ret_ErrString = "out of memory";
                goto __ERROUT;
            }
            // go through all measurements
            for (x = 0; x < Module->MeasurementCounter; x++) {
                uint64_t Address = 0;
                ASAP2_MEASUREMENT *Measurement = Module->Measurements[x];
                if (CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_SYMBOL_LINK)) {
                    Address = GetAddressByA2LLabel(SymFile, pappldata, Measurement->OptionalParameter.SymbolLink, &NotUpdatedLabelFile);
                } else {
                    if (Measurement->OptionalParameter.IfDataCanapeExtCount == 0) {
                        Address = GetAddressByA2LLabel(SymFile, pappldata, Measurement->Ident, &NotUpdatedLabelFile);
                    }
                }
                for (i = 0; i < Measurement->OptionalParameter.IfDataCanapeExtCount; i++) {
                    ASAP2_IF_DATA_CANAPE_EXT *IfDataCanapeExt = Measurement->OptionalParameter.IfDataCanapeExt[i];
                    if (CheckIfFlagSetPos(IfDataCanapeExt->OptionalParameter.Flags, OPTPARAM_IF_DATA_CANAPE_EXT_LINK_MAP)) {
                        Address = GetAddressByA2LLabel(SymFile, pappldata, IfDataCanapeExt->OptionalParameter.Label, &NotUpdatedLabelFile);
                        if (((par_Flags & A2L_LINK_UPDATE_IGNORE_FLAG) == 0) || (Address != 0)) {
                            IfDataCanapeExt->OptionalParameter.Address = Address;
                            if (AddAddressElement(&AddrList, Address, IfDataCanapeExt->OptionalParameter.FileOffsetAddress)) {
                                *ret_ErrString = "out of memory";
                                goto __ERROUT;
                            }
                        }
                    }
                }
                if (CheckIfFlagSetPos(Measurement->OptionalParameter.Flags, OPTPARAM_MEASUREMENT_ADDRESS)) {
                    if (((par_Flags & A2L_LINK_UPDATE_IGNORE_FLAG) == 0) || (Address != 0)) {
                        Measurement->OptionalParameter.Address = Address;
                        if (AddAddressElement(&AddrList, Address, Measurement->OptionalParameter.FileOffsetAddress)) {
                            *ret_ErrString = "out of memory";
                            goto __ERROUT;
                        }
                    }
                }
            }
            // loop through all characteristics
            for (x = 0; x < Module->CharacteristicCounter; x++) {
                uint64_t Address = 0;
                int HasValidLinkMap = 0;
                int HasValidSymbolLink = 0;
                ASAP2_CHARACTERISTIC *Characteristic = Module->Characteristics[x];
                if (CheckIfFlagSetPos(Characteristic->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_SYMBOL_LINK)) {
                    Address = GetAddressByA2LLabel(SymFile, pappldata, Characteristic->OptionalParameter.SymbolLink, &NotUpdatedLabelFile);
                    HasValidSymbolLink = 1;
                } else {
                    Address = 0;
                }
                for (i = 0; i < Characteristic->OptionalParameter.IfDataCanapeExtCount; i++) {
                    ASAP2_IF_DATA_CANAPE_EXT *IfDataCanapeExt = Characteristic->OptionalParameter.IfDataCanapeExt[i];
                    if (CheckIfFlagSetPos(IfDataCanapeExt->OptionalParameter.Flags, OPTPARAM_IF_DATA_CANAPE_EXT_LINK_MAP)) {
                        Address = GetAddressByA2LLabel(SymFile, pappldata, IfDataCanapeExt->OptionalParameter.Label, &NotUpdatedLabelFile);
                        if (((par_Flags & A2L_LINK_UPDATE_IGNORE_FLAG) == 0) || (Address != 0)) {
                            IfDataCanapeExt->OptionalParameter.Address = Address;
                            if (AddAddressElement(&AddrList, Address, IfDataCanapeExt->OptionalParameter.FileOffsetAddress)) {
                                *ret_ErrString = "out of memory";
                                goto __ERROUT;
                            }
                            HasValidLinkMap = 1;
                        }
                    }
                }
                if (!HasValidSymbolLink && !HasValidLinkMap) {
                    Address = GetAddressByA2LLabel(SymFile, pappldata, Characteristic->Name, &NotUpdatedLabelFile);
                }
                if (((par_Flags & A2L_LINK_UPDATE_IGNORE_FLAG) == 0) || (Address != 0)) {
                    Characteristic->Address = Address;
                    if (AddAddressElement(&AddrList, Address, Characteristic->FileOffsetAddress)) {
                        *ret_ErrString = "out of memory";
                        goto __ERROUT;
                    }
                }
            }
            // loop through all axis characteristics
            for (x = 0; x < Module->AxisPtsCounter; x++) {
                uint64_t Address = 0;
                ASAP2_AXIS_PTS *AxisPts = Module->AxisPtss[x];
                if (CheckIfFlagSetPos(AxisPts->OptionalParameter.Flags, OPTPARAM_CHARACTERISTIC_SYMBOL_LINK)) {
                    Address = GetAddressByA2LLabel(SymFile, pappldata, AxisPts->OptionalParameter.SymbolLink, &NotUpdatedLabelFile);
                } else {
                    if (AxisPts->OptionalParameter.IfDataCanapeExtCount == 0) {
                        Address = GetAddressByA2LLabel(SymFile, pappldata, AxisPts->Name, &NotUpdatedLabelFile);
                    }
                }
                for (i = 0; i < AxisPts->OptionalParameter.IfDataCanapeExtCount; i++) {
                    ASAP2_IF_DATA_CANAPE_EXT *IfDataCanapeExt = AxisPts->OptionalParameter.IfDataCanapeExt[i];
                    if (CheckIfFlagSetPos(IfDataCanapeExt->OptionalParameter.Flags, OPTPARAM_IF_DATA_CANAPE_EXT_LINK_MAP)) {
                        Address = GetAddressByA2LLabel(SymFile, pappldata, IfDataCanapeExt->OptionalParameter.Label, &NotUpdatedLabelFile);
                        IfDataCanapeExt->OptionalParameter.Address = Address;
                        if (((par_Flags & A2L_LINK_UPDATE_IGNORE_FLAG) == 0) || (Address != 0)) {
                            if (AddAddressElement(&AddrList, Address, IfDataCanapeExt->OptionalParameter.FileOffsetAddress)) {
                                *ret_ErrString = "out of memory";
                                goto __ERROUT;
                            }
                        }
                    }
                }
                if (((par_Flags & A2L_LINK_UPDATE_IGNORE_FLAG) == 0) || (Address != 0)) {
                    AxisPts->Address = Address;
                    if (AddAddressElement(&AddrList, Address, AxisPts->FileOffsetAddress)) {
                        *ret_ErrString = "out of memory";
                        goto __ERROUT;
                    }
                }
            }

            if (par_OutA2LFile != NULL) {
                SortAddressList(&AddrList);
                Ret = WriteA2LWithUpdateAddresses(Database, par_OutA2LFile, &AddrList, ret_ErrString);
            }
        }
    } else {
        *ret_ErrString = "no data found";
        Ret = -1;
    }
__ERROUT:
    if (!LoadedDebugInfosFlag && (pappldata != NULL)) {
        RemoveConnectFromProcessDebugInfos(UniqueId);
        FreeUniqueNumber (UniqueId);
        UnLockProcess (Pid);
    }
    if (LoadedDebugInfosFlag) {
        DeleteDebufInfosNotConnectingToProcess(pappldata);
    }
    if (NotUpdatedLabelFile.Handle != NULL) close_file(NotUpdatedLabelFile.Handle);

    RemoveAddressList(&AddrList);
    return Ret;
}
