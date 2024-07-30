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


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "Platform.h"

#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "IniDataBase.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "Message.h"
#include "Scheduler.h"
#include "Files.h"
#include "EquationParser.h"
#include "ExecutionStack.h"
#include "CanDataBase.h"
#include "EquationList.h"
#include "MainValues.h"
#include "ConfigurablePrefix.h"
#include "ReadCanCfg.h"

#define UNUSED(x) (void)(x)

#define REGISTER_EQU_PID  -1

static uint64_t SwapBytes (uint64_t v, int size)
{
    int x;
    uint64_t ret = 0;
    char *s, *d;

    s = (char*)&v;
    d = (char*)&ret + size -1;
    for (x = 0; x < size; x++) {
        *d-- = *s++;
    }
    return ret;
}


static void DetachAdditionalVariabales (NEW_CAN_SERVER_CONFIG *csc, int Idx)
{
    int *pIdx = (int*)(void*)&csc->Symbols[Idx];
    do {
        remove_bbvari (*pIdx);
        pIdx++;
    } while (*pIdx != 0);
}

static void DetachEquationArray (NEW_CAN_SERVER_CONFIG *csc, int Idx)
{
    int *pIdx = (int*)(void*)&csc->Symbols[Idx];
    do {
        detach_exec_stack ((struct EXEC_STACK_ELEM*)&csc->Symbols[*pIdx]);
        pIdx++;
    } while (*pIdx != 0);
}

int detach_canserver_config (NEW_CAN_SERVER_CONFIG *csc)
{
    int c_pos, s_pos, o_pos;

    if (csc != NULL) {
        for (c_pos = 0; c_pos < MAX_CAN_CHANNELS; c_pos++) {
            if (csc->channels[c_pos].global_tx_enable_vid > 0) remove_bbvari (csc->channels[c_pos].global_tx_enable_vid );
        }
        for (o_pos = 0; o_pos < MAX_OBJECTS_ALL_CHANNELS; o_pos++) {
            if (csc->objects[o_pos].EventOrCyclic >=1) {   // 2 -> translate event to byte code
                detach_exec_stack ((struct EXEC_STACK_ELEM*)GET_CAN_EVENT_BYTECODE(csc, o_pos));
            }
            if (csc->objects[o_pos].AdditionalVariablesIdx) {
                DetachAdditionalVariabales (csc, csc->objects[o_pos].AdditionalVariablesIdx);
            }
            if (csc->objects[o_pos].EquationBeforeIdx) {
                DetachEquationArray (csc, csc->objects[o_pos].EquationBeforeIdx);
            }
            if (csc->objects[o_pos].EquationBehindIdx) {
                DetachEquationArray (csc, csc->objects[o_pos].EquationBehindIdx);
            }
            if (csc->objects[o_pos].vid > 0) remove_bbvari (csc->objects[o_pos].vid);

            if (csc->objects[o_pos].cycle_is_execst) {
                if (csc->objects[o_pos].cycles > 0)  detach_exec_stack ((struct EXEC_STACK_ELEM*)GET_CAN_CYCLES_BYTECODE(csc, o_pos));
            }
        }
        // If byte code contains conversion delte it (detach blackboard variable)
        for (s_pos = 0; s_pos < MAX_SIGNALS_ALL_CHANNELS; s_pos++) {
            if (csc->bb_signals[s_pos].vid > 0) {
                remove_bbvari (csc->bb_signals[s_pos].vid);
            }
            if (csc->bb_signals[s_pos].ConvType == CAN_CONV_EQU) {
                detach_exec_stack ((struct EXEC_STACK_ELEM *)GET_CAN_SIG_BYTECODE(csc, s_pos));
            }
            if (csc->bb_signals[s_pos].type == MUX_BY_SIGNAL) {
                if (csc->bb_signals[s_pos].mux_startbit > 0) {
                    remove_bbvari_unknown_wait (csc->bb_signals[s_pos].mux_startbit);
                }
            }
        }
    }
    return 0;
}

int remove_canserver_config (NEW_CAN_SERVER_CONFIG *csc)
{
    int ret = 0;

    if (csc != NULL) {
        ret = detach_canserver_config (csc);
        my_free (csc);
    }
    return ret;
}


NEW_CAN_SERVER_CONFIG *LoadCanConfigAndStart (int par_Fd)
{
    if (s_main_ini_val.ConnectToRemoteMaster) {
        NEW_CAN_SERVER_CONFIG *ret = NULL;
        static NEW_CAN_SERVER_CONFIG *csc;

        // This must be locked
        IniFileDataBaseEnterCriticalSectionUser(__FILE__, __LINE__);

        // If ther is already an old configuration loaded -> remove this
        if (csc != NULL) remove_canserver_config (csc);
        csc = NULL;
        if (par_Fd <= 0) {
            IniFileDataBaseLeaveCriticalSectionUser();
            return NULL;
        }

        if ((csc = ReadCanConfig (par_Fd)) != NULL) {
            if (sizeof (csc->channels[0]) != 8192) {
                ThrowError (1, "(sizeof (csc->channels[0]) != 8192)   %i != 8192", sizeof (csc->channels[0]));
            }
            if ((int32_t)sizeof (NEW_CAN_SERVER_CONFIG) + csc->SymSize > MESSAGE_MAX_SIZE) {
                ThrowError (1, "sizeof (NEW_CAN_SERVER_CONFIG) + csc->SymSize %i > MESSAGE_MAX_SIZE % i",
                       (int32_t)sizeof (NEW_CAN_SERVER_CONFIG) + csc->SymSize, MESSAGE_MAX_SIZE);
                ret = NULL;
            } else {
                write_message (get_pid_by_name ("CANServer"),
                                                NEW_CANSERVER_STOP,
                                                0,
                                                NULL);
                write_message (get_pid_by_name ("CANServer"),
                                                NEW_CANSERVER_INIT,
                                                (int)sizeof (NEW_CAN_SERVER_CONFIG) + csc->SymSize,
                                                (char*)csc);

                ret = csc;
            }
        } else {
            ThrowError (1, "cannot read CAN config!?");
            ret = NULL;
        }
        IniFileDataBaseLeaveCriticalSectionUser();
        return ret;
    } else {
        write_message (get_pid_by_name ("CANServer"),
                                        NEW_CANSERVER_STOP,
                                        0,
                                        NULL);
        return NULL;
    }
}


static NEW_CAN_SERVER_CONFIG *ResizeCanConfigStruct (NEW_CAN_SERVER_CONFIG *Old, int NewSize)
{
    return (NEW_CAN_SERVER_CONFIG*)my_realloc (Old, (size_t)NewSize + sizeof(NEW_CAN_SERVER_CONFIG));
}


static NEW_CAN_SERVER_CONFIG *AllocMemInsideCanCfg (NEW_CAN_SERVER_CONFIG *Cfg, int *pIndex, int ByteSize)
{
    int NewSize;

    if ((int)(Cfg->SymPos + ByteSize) >= Cfg->SymSize) {
        NewSize = Cfg->SymSize + 64 * 1024;    // Increase by 64Kbyte
        Cfg = ResizeCanConfigStruct (Cfg, NewSize);
        if (Cfg == NULL) return NULL;
        Cfg->SymSize = NewSize;
    }
    *pIndex = Cfg->SymPos;
    Cfg->SymPos += ByteSize;
    return Cfg;
}

static NEW_CAN_SERVER_CONFIG *AddSymbol2CanCfg (NEW_CAN_SERVER_CONFIG *Cfg, int SigPos, char *Symbol, char *Unit, 
                                                char *Conv, struct EXEC_STACK_ELEM *ExecStack, int ConvSize)
{
    int NewSize;
    int SymbolLen, UnitLen;

    SymbolLen = (int)strlen(Symbol) + 1;
    UnitLen = (int)strlen(Unit) + 1;
    if ((Cfg->SymPos + SymbolLen + UnitLen + ConvSize + 2) >= Cfg->SymSize) {
        NewSize = Cfg->SymSize + 64 * 1024;    // Increase by 64Kbyte
        Cfg = ResizeCanConfigStruct (Cfg, NewSize);
        Cfg->SymSize = NewSize;
    }
    StringCopyMaxCharTruncate (&Cfg->Symbols[Cfg->SymPos], Symbol, SymbolLen);
    Cfg->bb_signals[SigPos].NameIdx = Cfg->SymPos;
    Cfg->SymPos += (int32_t)strlen (Symbol) + 1;
    StringCopyMaxCharTruncate (&Cfg->Symbols[Cfg->SymPos], Unit, UnitLen);
    Cfg->bb_signals[SigPos].UnitIdx = Cfg->SymPos;
    Cfg->SymPos += (int32_t)strlen (Unit) + 1;
    if ((ConvSize > 0) && (ExecStack != NULL)) {
        MEMCPY (&Cfg->Symbols[Cfg->SymPos], ExecStack, (size_t)ConvSize);
        RegisterEquation (REGISTER_EQU_PID, Conv, (struct EXEC_STACK_ELEM *)&Cfg->Symbols[Cfg->SymPos], "", EQU_TYPE_CAN);
        Cfg->bb_signals[SigPos].ConvIdx = Cfg->SymPos;
        Cfg->SymPos += ConvSize;
    }
    return Cfg;
}

static NEW_CAN_SERVER_CONFIG *AddByteCode2CanCfg (NEW_CAN_SERVER_CONFIG *Cfg, int *pIndex, char *ByteCode, int ByteCodeSize)
{
    int NewSize;

    if ((int)(Cfg->SymPos + ByteCodeSize) >= Cfg->SymSize) {
        NewSize = Cfg->SymSize + 64 * 1024;    // Increase by 64Kbyte
        Cfg = ResizeCanConfigStruct (Cfg, NewSize);
        Cfg->SymSize = NewSize;
    }
    if (ByteCodeSize > 0) {
        MEMCPY (&Cfg->Symbols[Cfg->SymPos], ByteCode, (size_t)ByteCodeSize);
        *pIndex = Cfg->SymPos;
        Cfg->SymPos += ByteCodeSize;
    }
    return Cfg;
}

struct EXEC_STACK_ELEM *TranslateString2ByteCode (char *equ, int maxc, int *pSize, int *pUseCANData)
{
    UNUSED(maxc);
    struct EXEC_STACK_ELEM *Ret;

    *pSize = 0;
    Ret = solve_equation_replace_parameter_with_can (equ, pUseCANData);
    if (Ret != NULL) {
        *pSize = sizeof_exec_stack (Ret);
    }
    return Ret;
}

int TranslateString2Curve (char *txt, int maxc, int *pSize)
{
    double v[INI_MAX_LINE_LENGTH/8];   // max 256 interpolated sample points
    int x;
    char *p;

    p = txt;

    *pSize = 0;
    x = 2;  // 0 is dimension 1 is empty
    while (*p != 0) {
        if (x >= INI_MAX_LINE_LENGTH/8) {
            ThrowError (1, "too many point (255) in curve definition \"%s\"", txt);
            return -1;
        }
        while (isspace(*p)) p++;
        if (!strncmp ("0x", p, 2)) {   // Hex value
            v[x] = strtoul (p, &p, 16);
        } else {
            v[x] = strtod (p, &p);
        }
        x++;
        while (isspace(*p)) p++;
        if (*p != '/') {
            ThrowError (1, "missing / in curve definition (%i) \"%s\"", p-txt, txt);
            return -1;
        }
        p++;
        while (isspace(*p)) p++;
        if (!strncmp ("0x", p, 2)) {   // Hex value
            v[x] = strtoul (p, &p, 16);
        } else {
            v[x] = strtod (p, &p);
        } 
        x++;
        if ((*p != 0) && !isspace (*p)) {
            ThrowError (1, "missing seperator in curve definition (%i) \"%s\"", p-txt, txt);
            return -1;
        }
    }
    if ((x * (int)sizeof(double)) >= maxc) {
        ThrowError (1, "too many point in curve definition \"%s\"", txt);
        return -1;
    }
    v[0] = x / 2 - 1;
    MEMCPY (txt, v, (size_t)x * sizeof(double));
    *pSize = x * (int)sizeof(double);
 
    return 0;
}


/*  *elem1  < *elem2        fcmp returns an integer < 0
    *elem1 == *elem2       fcmp returns 0
    *elem1  > *elem2        fcmp returns an integer > 0 */
static int qsort_function( const void *a, const void *b)
{
    const NEW_CAN_HASH_ARRAY_ELEM *aa = (const NEW_CAN_HASH_ARRAY_ELEM *)a;
    const NEW_CAN_HASH_ARRAY_ELEM *bb = (const NEW_CAN_HASH_ARRAY_ELEM *)b;
    if (bb->id > aa->id) return -1;
    if (bb->id < aa->id) return 1;
    return 0;
}

typedef struct {
    NEW_CAN_HASH_ARRAY_ELEM h;
    NEW_CAN_SERVER_CONFIG *csc;
} EX_NEW_CAN_HASH_ARRAY_ELEM;

static int qsort_ex_function( const void *a, const void *b)
{
    uint32_t ta, tb;
    const EX_NEW_CAN_HASH_ARRAY_ELEM *aa = (const EX_NEW_CAN_HASH_ARRAY_ELEM *)a;
    const EX_NEW_CAN_HASH_ARRAY_ELEM *bb = (const EX_NEW_CAN_HASH_ARRAY_ELEM *)b;
    ta = aa->h.id;
    if (ta > 0) {
        if (aa->csc->objects[aa->h.pos].type == J1939_22_MULTI_C_PG) {
            ta |= 0x80000000UL;
        }
    } else {
        ta = 0;
    }
    tb = bb->h.id;
    if (tb > 0) {
        if (bb->csc->objects[bb->h.pos].type == J1939_22_MULTI_C_PG) {
            tb |= 0x80000000UL;
        }
    } else {
        tb = 0;
    }
    if (tb > ta) return -1;
    if (tb < ta) return 1;
    return 0;
}

// this sort function sorts the mutli C_PG object to the last positions
static void Sort_Multi_C_PG_Tx(NEW_CAN_SERVER_CONFIG *csc, int c)
{
    EX_NEW_CAN_HASH_ARRAY_ELEM ExtHash[MAX_TX_OBJECTS_ONE_CHANNEL];
    int x;
    for (x = 0; x < MAX_TX_OBJECTS_ONE_CHANNEL; x++) {
        ExtHash[x].h = csc->channels[c].hash_tx[x];
        ExtHash[x].csc = csc;
    }
    qsort((void *)ExtHash, MAX_TX_OBJECTS_ONE_CHANNEL, sizeof(EX_NEW_CAN_HASH_ARRAY_ELEM), qsort_ex_function);
    for (x = 0; x < MAX_TX_OBJECTS_ONE_CHANNEL; x++) {
        csc->channels[c].hash_tx[x] = ExtHash[x].h;
    }
}


int SortMuxSignals (NEW_CAN_SERVER_CONFIG *csc, int o_p)
{
    int s;
    int FirstMuxSig;
    int p1, p2, p3, p1o, p2o;
    int16_t NewSigs[MAX_SIGNALS_ONE_OBJECT];
    int NewSigsIdx;
    int s_p;
    short *new_ptr;

    // this must be recalculate each time it is used! csc can be moved!
    new_ptr = (short*)(void*)&(csc->Symbols[csc->objects[o_p].SignalsArrayIdx]);
    if (csc->objects[o_p].signals_ptr != new_ptr) {
        csc->objects[o_p].signals_ptr = new_ptr;
    }

    // MUX signale with same MUX start bit inide the MUX martix sort
    FirstMuxSig = -1;
    NewSigsIdx = 0;
    for (s = 0; s < csc->objects[o_p].signal_count; s++) {
        s_p = csc->objects[o_p].signals_ptr[s];
        if (csc->bb_signals[s_p].type == MUX_SIGNAL) {
            csc->bb_signals[s_p].mux_next = -1;
            csc->bb_signals[s_p].mux_equ_sb_childs = -1;
            csc->bb_signals[s_p].mux_equ_v_childs = -1;
            if (FirstMuxSig == -1) {  // Insert the first MUX signal into the new signal list
                NewSigs[NewSigsIdx++] = (int16_t)s_p;
                FirstMuxSig = s_p;
            } else  {  // not the first MUX signal
                for (p1o = p1 = FirstMuxSig; p1 >= 0; p1o = p1, p1 = csc->bb_signals[p1].mux_next) {
                    if (csc->bb_signals[s_p].mux_startbit == csc->bb_signals[p1].mux_startbit) {
                        // there exist a MUX signal with this MUX start bit
                        for (p2o = p2 = p1; p2 >= 0; p2o = p2, p2 = csc->bb_signals[p2].mux_equ_sb_childs) {
                            if (csc->bb_signals[s_p].mux_value == csc->bb_signals[p2].mux_value) {
                                // Ther exist a MUX with this MUX value
                                for (p3 = p2; csc->bb_signals[p3].mux_equ_v_childs >= 0; p3 = csc->bb_signals[p3].mux_equ_v_childs);
                                csc->bb_signals[p3].mux_equ_v_childs = s_p;
                                goto __NEXT_SIGNAL;
                            }
                        }
                        // There is no MUX signal with that MUX value
                        csc->bb_signals[p2o].mux_equ_sb_childs = s_p;
                        goto __NEXT_SIGNAL;
                    }
                    // There is no MUX signal with that MUX start bit
                }           
                csc->bb_signals[p1o].mux_next = s_p;
                // Insert only the master MUX signale into the new signal liste
                NewSigs[NewSigsIdx++] = (int16_t)s_p;
            }
        } else {
            // Always insert none MUX-Signale into the new signal liste
            NewSigs[NewSigsIdx++] = (int16_t)s_p;
        }
__NEXT_SIGNAL:
        continue;
    }

    // Takeover new signal list
    for (s = 0; s < NewSigsIdx; s++) {
        csc->objects[o_p].signals_ptr[s] = NewSigs[s];
    }
    csc->objects[o_p].signal_count = NewSigsIdx;
    return 0;
}

// Remark ObjectData Arrays have to be  3 bytes than neccessary!
static __inline void ResetDataBitsValue (int StartBit,
                                         int BitLen,
                                         int ByteOrder,
                                         int SizeOfObject,
                                         char *ObjectData)
{
    uint64_t Mask;
    uint32_t PosBit;
    uint64_t Help64;
    unsigned char Help8;

    if (BitLen < 64) {
        Mask = 0xFFFFFFFFFFFFFFFFULL << BitLen;
    } else {
        Mask = 0;
    }
    Mask = ~Mask;
    if (ByteOrder == MSB_FIRST_BYTEORDER) {  // 1 -> MSB_FISRT
        int32_t PosByte;   // this must be signed because the value can be negative
        PosBit = StartBit & 0x7;
        PosByte = SizeOfObject - 8 - (StartBit >> 3);
        Help64 = Mask << PosBit;
#ifdef _WIN32
        Help64 = _byteswap_uint64 (Help64);
#else
        Help64 = __builtin_bswap64(Help64);
#endif
        Help64 = ~Help64;
        *(uint64_t*)(void*)(ObjectData + PosByte) &= Help64;
        Help8 = (Mask >> 56) >> (8 - PosBit);
        Help8 = ~Help8;
        *(unsigned char *)(ObjectData + PosByte - 1) &= Help8;
    } else {  // 0 -> LSB first
        uint32_t PosByte;
        PosBit = StartBit & 0x7;
        PosByte = (uint32_t)StartBit >> 3;
        Help64 = (uint64_t)Mask << PosBit;
        Help64 = ~Help64;
        *(uint64_t*)(void*)(ObjectData + PosByte) &= Help64;
        Help8 = (Mask >> 56) >> (8 - PosBit);
        Help8 = ~Help8;
        *(unsigned char *)(ObjectData + PosByte + 8) &= Help8;
    }
}

static NEW_CAN_SERVER_CONFIG *SetInitDataOfCANObject (NEW_CAN_SERVER_CONFIG *csc,
                                                      int o_pos,
                                                      char *InitDataString)
{
    unsigned char *InitBytes;
    int HelpIdx;
    int Size;
    uint64_t Help_u64;
    unsigned char Help_u8;
    int x;

    Size = csc->objects[o_pos].size;
    if (Size < 8) Size = 8;

    InitBytes = _alloca ((size_t)Size);
    memset (InitBytes, 0, (size_t)Size);

#ifdef _WIN32
    Help_u64  = _strtoui64 (InitDataString, NULL, 0);
#else
    Help_u64  = strtoull (InitDataString, NULL, 0);
#endif
    if (Size <= 8) {
        if (csc->objects[o_pos].byteorder == MSB_FIRST_BYTEORDER) {
            Help_u64 = SwapBytes (Help_u64, csc->objects[o_pos].size);
        }
        *(uint64_t*)(void*)InitBytes = Help_u64;
    } else {
        Help_u8 = (unsigned char)Help_u64;
        for (x = 0; x < Size; x++) {
            InitBytes[x] = Help_u8;
        }
    }
    csc = AddByteCode2CanCfg (csc, &HelpIdx, (char*)InitBytes, Size + 4);  // Always 3 bytelarger as neccessary because of ResetDataBitsValue()
    csc->objects[o_pos].InitDataIdx = HelpIdx;

    return csc;
}

static uint32_t BuildMask32 (int Size)
{
    uint32_t Ret;

    if (Size >= 32) Ret = 0xFFFFFFFF;
    else {
        Ret = 0xFFFFFFFFUL;
        Ret <<= Size;
        Ret = ~Ret;
    }
    return Ret;
}


static uint64_t BuildMask64 (int Size)
{
    uint64_t Ret;

    if (Size >= 64) Ret = 0xFFFFFFFFFFFFFFFFULL;
    else {
        Ret = 0xFFFFFFFFFFFFFFFFULL;
        Ret <<= Size;
        Ret = ~Ret;
    }
    return Ret;
}

#define MAX_ADDITIONAL_EQUS 100

static NEW_CAN_SERVER_CONFIG* Add_C_PGs(NEW_CAN_SERVER_CONFIG *csc, int o_pos, const char *section, char *txt, int par_Fd)
{
    IniFileDataBaseReadString (section, "C_PGs", "", txt, INI_MAX_LINE_LENGTH, par_Fd);
    if (1) {  // must be called even the string is empty!
        int run;
        int count = 0;
        uint32_t *Ids;
        for (run = 0; run < 2; run++) {
            char *p = txt;
            for (int x = 1; ; x++) { // starts with 1 because first enry is the size
                char *h = NULL;
                int Id = strtoul(p, &h, 0);
                if ((h == NULL) || (h == p)) break;
                if (run == 0) count++;
                else if (run == 1) {
                    int Idx = csc->objects[o_pos].Protocol.J1939.C_PGs_ArrayIdx;
                    Ids = (uint32_t*)&(csc->Symbols[Idx]);
                    Ids = GET_CAN_J1939_MULTI_C_PG_OBJECT(csc, o_pos);
                    Ids[x] = Id;
                }
                p = h;
                while(isascii(*p) && isspace(*p)) p++;
                if (*p != ',') break;
                p++;
            }
            if (run == 0) {
                int32_t HelpInt;
                csc = AllocMemInsideCanCfg(csc, &HelpInt, (count + 1) * sizeof(int32_t));
                if (csc == NULL) return NULL;
                csc->objects[o_pos].Protocol.J1939.C_PGs_ArrayIdx = HelpInt;
                Ids = GET_CAN_J1939_MULTI_C_PG_OBJECT(csc, o_pos);
                Ids[0] = count;  // first enry is the size
            }
        }
    }
    return csc;
}

static void C_PGs_Translate(NEW_CAN_SERVER_CONFIG *csc, int c, int o_pos)
{
    int Count;
    uint32_t *Ids = GET_CAN_J1939_MULTI_C_PG_OBJECT(csc, o_pos);
    Count = Ids[0] + 1;

    for (int x = 1; x < Count; x++) { // starts with 1 because first enry is the size
        int o;
        for (o = 0; o < csc->channels[c].object_count; o++) {
            int o_p = csc->channels[c].objects[o];
            if (csc->objects[o_p].id == Ids[x]) {
                if (csc->objects[o_p].dir != csc->objects[o_pos].dir) {
                    ThrowError (1, "C_PG Id 0x%X has not the same direction inside multi PG Id = 0x%X on channel %i", Ids[x], csc->objects[o_pos].id, c);
                    Ids[x] = (uint32_t)-1;
                } else {
                    Ids[x] = o_p;
                }
                break;
            }
        }
        if (o == csc->channels[c].object_count) {
            // not found!
            ThrowError (1, "C_PG Id 0x%X not exists inside multi PG Id = 0x%X on channel %i", Ids[x], csc->objects[o_pos].id, c);
            Ids[x] = (uint32_t)-1;
        }
    }
}


int CheckCanFdPossibleMessageSize(int par_Size)
{
    if (par_Size <= 8) {
        return par_Size;
    } else if (par_Size <= 12) {
        return 12;
    } else if (par_Size <= 16) {
        return 16;
    } else if (par_Size <= 20) {
        return 20;
    } else if (par_Size <= 24) {
        return 24;
    } else if (par_Size <= 32) {
        return 32;
    } else if (par_Size <= 48) {
        return 48;
    } else {
        return 64;
    }
}

NEW_CAN_SERVER_CONFIG *ReadCanConfig (int par_Fd)
{
    NEW_CAN_SERVER_CONFIG *csc;
    char txt[INI_MAX_LINE_LENGTH];
    char txt2[INI_MAX_LINE_LENGTH];
    char section[INI_MAX_SECTION_LENGTH];
    char entry[256];
    int varianten;
    int can_varianten_count;
    int object_per_node;
    int o_pos, s_pos;
    int c, o, oo, s;
    char Name[512];
    char Unit[64];
    int Size;
    int x, xx, o_p;
    int txc, rxc;
    int HelpIntArray[MAX_ADDITIONAL_EQUS + 1];
    int HelpInt;
    double HelpDouble;
    char *p;
    struct EXEC_STACK_ELEM *ExecStack;

    o_pos = 0;
    s_pos = 0;
    
    csc = (NEW_CAN_SERVER_CONFIG*)my_calloc (1, sizeof(NEW_CAN_SERVER_CONFIG));
    if (csc == NULL) return NULL;

    // index should never be 0 so we add a not used string at the beginning
    csc = AddByteCode2CanCfg (csc, &HelpInt, "CAN-Config", 11);

    sprintf (section, "CAN/Global");
    // do not use units in signal definitions for blackboard variable descriptions
    csc->dont_use_units_for_bb = IniFileDataBaseReadInt (section, "dont use units for blackboard", 0, par_Fd);
    csc->dont_use_init_values_for_existing_variables = s_main_ini_val.DontUseInitValuesForExistingCanVariables;
    //csc->channel_count = IniFileDataBaseReadInt (section, "can_controller_count", 0, par_Fd);
    csc->channel_count = GetCanControllerCountFromIni (section, par_Fd);
    if ((csc->channel_count < 1) && (csc->channel_count > MAX_CAN_CHANNELS)) {
        ThrowError (1, "expected 1...%d CAN-Controller not %i", MAX_CAN_CHANNELS, csc->channel_count);
        remove_canserver_config (csc);
        return NULL;
    }
    can_varianten_count = IniFileDataBaseReadInt (section, "can_varianten_count", 0, par_Fd);
    if (can_varianten_count <= 0) {
        ThrowError (1, "no CAN configuration please configure your CAN", can_varianten_count);
        remove_canserver_config (csc);
        return NULL;
    }
    IniFileDataBaseReadString (section, "can_card_type", "", txt, sizeof (txt), par_Fd);
    if (!strcmp ("iPC-I 165/PCI 2*i527", txt)) {
        csc->channel_type = iPC_I_165_PCI_2_527;
    } else {
        csc->channel_type = iPC_I_165_PCI_2_527;
        //ThrowError (1, "not supported CAN-Controller %s", txt);
    }
    csc->EnableGatewayDeviceDriver = IniFileDataBaseReadYesNo (section, "enable gateway device driver", 0, par_Fd);
    if (csc->EnableGatewayDeviceDriver) {
        double Value;
        csc->EnableGatewayDeviceDriver = 0;
        switch (IniFileDataBaseReadUInt (section, "enable gateway virtual device driver", 0x1, par_Fd))  {
        default:
        case 1:
            csc->EnableGatewayDeviceDriver |= 1;
            break;
        case 0:
            break;
        case 2:
            IniFileDataBaseReadString (section, "enable gateway virtual device driver equ", "", txt, sizeof(txt), par_Fd);
            Value = direct_solve_equation(txt);
            csc->EnableGatewayDeviceDriver |= (Value > 0.5) ? 1 : 0;
            break;
        }
        switch (IniFileDataBaseReadUInt (section, "enable gateway real device driver", 0x1, par_Fd))  {
        default:
        case 1:
            csc->EnableGatewayDeviceDriver |= 2;
            break;
        case 0:
            break;
        case 2:
            IniFileDataBaseReadString (section, "enable gateway real device driver equ", "", txt, sizeof(txt), par_Fd);
            Value = direct_solve_equation(txt);
            csc->EnableGatewayDeviceDriver |= (Value > 0.5) ? 2 : 0;
            break;
        }
    }
    for (c = 0; c < csc->channel_count; c++) {
        oo = 0;
        csc->channels[c].object_count = 0;
        rxc = txc = 0;
        sprintf (section, "CAN/Global");
        csc->channels[c].enable = 0x0;
        sprintf (entry, "can_controller%i_startup_state", c+1);
        csc->channels[c].StartupState =  IniFileDataBaseReadInt ("CAN/Global", entry, 1, par_Fd);
        sprintf (entry, "can_controller%i_variante", c+1);
        IniFileDataBaseReadString ("CAN/Global", entry, "", txt2, sizeof (txt2), par_Fd);
        p = txt2;
        csc->channels[c].j1939_flag = 0;
        for (x = 0; x < J1939TP_MP_MAX_SRC_ADDRESSES; x++) {
            csc->channels[c].J1939_src_addr[x] = (unsigned char)0xFF;
        }
        while (isdigit (*p)) { 
            enum {PrefixId, PrefixObjName, NoPrefixObjName, PrefixVarObjName, NoPrefixVarObjName} ControlBlackboardName;
            char VarianteName[512];
            varianten = strtol (p, &p, 0);
            if (*p == ',') p++;
            if ((varianten < 0) || (varianten >= can_varianten_count))  {
                ThrowError (1, "wrong variante number %i (0...%i)", varianten, can_varianten_count);
                remove_canserver_config (csc);
                return NULL;
            }

            sprintf (section, "CAN/Variante_%i", varianten);
            csc->channels[c].bus_frq = IniFileDataBaseReadInt (section, "baud rate", 0, par_Fd);
            csc->channels[c].can_fd_enabled = IniFileDataBaseReadYesNo (section, "can fd", 0, par_Fd);
            csc->channels[c].sample_point = IniFileDataBaseReadFloat (section, "sample point", 0.75, par_Fd);
            csc->channels[c].data_sample_point = IniFileDataBaseReadFloat (section, "sample point", 0.8, par_Fd);
            csc->channels[c].data_baud_rate = IniFileDataBaseReadInt (section, "data baud rate", 4000, par_Fd);
            IniFileDataBaseReadString (section, "name", "", VarianteName, sizeof (VarianteName), par_Fd);

            IniFileDataBaseReadString (section, "ControlBbName", "PrefixId", txt, sizeof (txt), par_Fd);
            if (!strcmpi ("NoPrefixObjName", txt)) {
                ControlBlackboardName = NoPrefixObjName;
            } else if (!strcmpi ("PrefixObjName", txt)) {
                ControlBlackboardName = PrefixObjName;
            } else if (!strcmpi ("NoPrefixVarObjName", txt)) {
                ControlBlackboardName = NoPrefixVarObjName;
            } else if (!strcmpi ("PrefixVarObjName", txt)) {
                ControlBlackboardName = PrefixVarObjName;
            } else {
                ControlBlackboardName = PrefixId;
            }

            IniFileDataBaseReadString (section, "j1939", "no", txt, sizeof (txt), par_Fd);
            csc->channels[c].j1939_flag |= !strcmpi ("yes", txt);
            csc->j1939_flag |= csc->channels[c].j1939_flag;

            for (x = 0; x < 4; x++) {
                unsigned char Addr;
                int i;
                sprintf (entry, "j1939 src addr %i", x);
                Addr = (unsigned char)IniFileDataBaseReadInt (section, entry, 0xFF, par_Fd);
                if (Addr != 0xFF) {
                    for (i = 0; i < J1939TP_MP_MAX_SRC_ADDRESSES; i++) {
                        if (csc->channels[c].J1939_src_addr[i] == 0xFF) {
                            csc->channels[c].J1939_src_addr[i] = Addr;
                            Addr = 0xFF;
                            break;
                        }
                    }
                    if (Addr != 0xFF) {
                        ThrowError (1, "only 4 j1939 source addresses allowed for one channel"); 
                    }
                }
            }

            object_per_node = IniFileDataBaseReadInt (section, "can_object_count", 0, par_Fd);
            csc->channels[c].object_count += object_per_node;

            // All Objects of a variant
            for (o = 0; o < object_per_node; o++, oo++) {
                int SignalIndexListOffset;
                if (oo >= MAX_OBJECTS_ONE_CHANNEL) {
                    ThrowError (1, "max. %i objects for one channals allowed", (int)MAX_OBJECTS_ONE_CHANNEL);
                    remove_canserver_config (csc);
                    return NULL;
                }
                if (o_pos >= MAX_OBJECTS_ALL_CHANNELS) {
                    ThrowError (1, "max. %i objects for all channals allowed", (int)MAX_OBJECTS_ALL_CHANNELS);
                    remove_canserver_config (csc);
                    return NULL;
                }
                sprintf (section, "CAN/Variante_%i/Object_%i", varianten, o);

                IniFileDataBaseReadString (section, "direction", "", txt, sizeof (txt), par_Fd);
                if (!strcmpi ("write", txt) || !strcmpi ("write_variable_id", txt)) {
                    if (!strcmpi ("write_variable_id", txt)) csc->objects[o_pos].dir = WRITE_VARIABLE_ID_OBJECT;
                    else csc->objects[o_pos].dir = WRITE_OBJECT;
                    if (txc >= (MAX_TX_OBJECTS_ONE_CHANNEL-1)) {
                        ThrowError (1, "max. %i TX objects for one channals allowed", (int)(MAX_TX_OBJECTS_ONE_CHANNEL-1));
                        remove_canserver_config (csc);
                        return NULL;
                    }
                    txc++;
                } else {
                    csc->objects[o_pos].dir = READ_OBJECT;
                    if (rxc >= (MAX_RX_OBJECTS_ONE_CHANNEL-1)) {
                        ThrowError (1, "max. %i RX objects for one channals allowed", (int)(MAX_RX_OBJECTS_ONE_CHANNEL-1));
                        remove_canserver_config (csc);
                        return NULL;
                    }
                    rxc++;
                }
                IniFileDataBaseReadString (section, "id", "", txt, sizeof (txt), par_Fd);
                csc->objects[o_pos].id = strtoul (txt, NULL, 0);

__TRY_AGAIN:
                switch (ControlBlackboardName) {
                case NoPrefixObjName:
                    IniFileDataBaseReadString (section, "name", "", txt, sizeof (txt), par_Fd);
                    break;
                case PrefixObjName:
                    sprintf (txt, "%s.CAN%i.", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES), c);
                    IniFileDataBaseReadString (section, "name", "", txt+strlen(txt), sizeof (txt)-(DWORD)strlen(txt), par_Fd);
                    break;
                case NoPrefixVarObjName:
                    sprintf (txt, "%s.", VarianteName);
                    IniFileDataBaseReadString (section, "name", "", txt+strlen(txt), sizeof (txt)-(DWORD)strlen(txt), par_Fd);
                    break;
                case PrefixVarObjName:
                    sprintf (txt, "%s.CAN%i.%s.", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES), c, VarianteName);
                    IniFileDataBaseReadString (section, "name", "", txt+strlen(txt), sizeof (txt)-(DWORD)strlen(txt), par_Fd);
                    break;
                case PrefixId:
                //default:
                    if (csc->objects[o_pos].dir == WRITE_VARIABLE_ID_OBJECT) {
                        sprintf (txt, "%s.CAN%i.%s.", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES), c, VarianteName);
                        IniFileDataBaseReadString (section, "name", "", txt+strlen(txt), sizeof (txt)-(DWORD)strlen(txt), par_Fd);
                    } else {
                        sprintf (txt, "%s.CAN%i.0x%X", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES), c, csc->objects[o_pos].id);
                    }
                    break;
                }
                csc->objects[o_pos].vid =  add_bbvari (txt, BB_UDWORD, "[]");
                if ((csc->objects[o_pos].vid == LABEL_NAME_NOT_VALID) &&
                    (ControlBlackboardName != PrefixId)) {
                    ThrowError(1, "the name \"%s\" is no valid blackboard variable name", txt);
                    ControlBlackboardName = PrefixId;
                    goto __TRY_AGAIN;
                }
                if (csc->objects[o_pos].vid <= 0) {
                    ThrowError(1, "cannot add blackboard variable \"%s\" (%i)", txt, csc->objects[o_pos].vid);
                }
                IniFileDataBaseReadString (section, "extended", "", txt, sizeof (txt), par_Fd);
                if (!strcmpi ("yes", txt)) csc->objects[o_pos].ext = EXTENDED_ID;
                else csc->objects[o_pos].ext = STANDARD_ID;
                csc->objects[o_pos].size = (int16_t)IniFileDataBaseReadInt (section, "size", 0, par_Fd);
                if (csc->channels[c].can_fd_enabled) {
                    if (IniFileDataBaseReadYesNo (section, "bit_rate_switch", 0, par_Fd) > 0) csc->objects[o_pos].ext |= CANFD_BTS_MASK;
                    if (IniFileDataBaseReadYesNo (section, "fd_frame_format_switch", 0, par_Fd) > 0) csc->objects[o_pos].ext |= CANFD_FDF_MASK;
                }
                IniFileDataBaseReadString (section, "type", "", txt, sizeof (txt), par_Fd);
                if (!strcmpi ("mux", txt)) csc->objects[o_pos].type = MUX_OBJECT;
                else if (!strcmpi ("j1939", txt)) csc->objects[o_pos].type = J1939_OBJECT;
                else if (!strcmpi ("j1939_multi_c_pg", txt)) csc->objects[o_pos].type = J1939_22_MULTI_C_PG;
                else if (!strcmpi ("j1939_c_pg", txt)) csc->objects[o_pos].type = J1939_22_C_PG;
                else csc->objects[o_pos].type = NORMAL_OBJECT;
                if (csc->objects[o_pos].size < 1) csc->objects[o_pos].size = 1;
                if (csc->objects[o_pos].type == J1939_OBJECT) {
                    if (csc->objects[o_pos].size > 1785) csc->objects[o_pos].size = 1785;
                    if (IniFileDataBaseReadString (section, "variable_dlc", "", Name, 512, par_Fd)) {
                        csc->objects[o_pos].Protocol.J1939.vid_dlc = add_bbvari (Name, BB_UWORD, "");
                        write_bbvari_uword (csc->objects[o_pos].Protocol.J1939.vid_dlc, (uint16_t)csc->objects[o_pos].size);
                    } else csc->objects[o_pos].Protocol.J1939.vid_dlc = 0;
                    if (IniFileDataBaseReadString (section, "variable_dst_addr", "", Name, 512, par_Fd)) {
                        csc->objects[o_pos].Protocol.J1939.dst_addr_vid = add_bbvari (Name, BB_UBYTE, "");
                        csc->objects[o_pos].Protocol.J1939.dst_addr = (unsigned char)IniFileDataBaseReadInt (section, "init_dst_addr", 0xFF, par_Fd);
                        write_bbvari_ubyte (csc->objects[o_pos].Protocol.J1939.dst_addr_vid, csc->objects[o_pos].Protocol.J1939.dst_addr);
                    } else {
                        csc->objects[o_pos].Protocol.J1939.dst_addr_vid = 0;
                        csc->objects[o_pos].Protocol.J1939.dst_addr = (unsigned char)IniFileDataBaseReadInt (section, "fixed_dst_addr", 0xFF, par_Fd);
                    }
                } else if (csc->objects[o_pos].type == J1939_22_C_PG) {
                    if (csc->objects[o_pos].size > 64) csc->objects[o_pos].size = 64;
                } else if (csc->objects[o_pos].type == J1939_22_MULTI_C_PG) {
                    if (csc->objects[o_pos].size > 64) csc->objects[o_pos].size = 64;
                } else {
                    if ((csc->objects[o_pos].ext & CANFD_FDF_MASK) == CANFD_FDF_MASK) {
                        if (csc->objects[o_pos].size > 64) csc->objects[o_pos].size = 64;
                        csc->objects[o_pos].size = CheckCanFdPossibleMessageSize(csc->objects[o_pos].size);
                    } else {
                        if (csc->objects[o_pos].size > 8) csc->objects[o_pos].size = 8;
                    }
                }
                csc->objects[o_pos].max_size = csc->objects[o_pos].size;
                switch (csc->objects[o_pos].type) {
                case J1939_22_MULTI_C_PG:
                    csc = Add_C_PGs(csc, o_pos, section, txt, par_Fd);
                    if (csc == NULL) return NULL;
                    break;
                case J1939_22_C_PG:
                    // ? TODO: ?
                    break;
                default:
                    break;
                }
                // LSB_FIRST or MSB_FISRT format
                IniFileDataBaseReadString (section, "byteorder", "", txt, sizeof (txt), par_Fd);
                if (!strcmpi ("msb_first", txt)) csc->objects[o_pos].byteorder = MSB_FIRST_BYTEORDER;
                else csc->objects[o_pos].byteorder = LSB_FISRT_BYTEORDER;

                // CAN object init data
                if (IniFileDataBaseReadString (section, "InitData", "", txt, sizeof (txt), par_Fd)) {
                    csc = SetInitDataOfCANObject (csc, o_pos, txt);
                } else {
                    csc = SetInitDataOfCANObject (csc, o_pos, "0x00");
                }
                if (csc->objects[o_pos].type == MUX_OBJECT) {
                    csc->objects[o_pos].Protocol.Mux.BitPos = (int16_t)IniFileDataBaseReadInt (section, "mux_startbit", 0, par_Fd);
                    csc->objects[o_pos].Protocol.Mux.Size = (int8_t)IniFileDataBaseReadInt (section, "mux_bitsize", 0, par_Fd);
                    csc->objects[o_pos].Protocol.Mux.Value = IniFileDataBaseReadInt (section, "mux_value", 0, par_Fd);
                    ResetDataBitsValue (csc->objects[o_pos].Protocol.Mux.BitPos,
                                        csc->objects[o_pos].Protocol.Mux.Size,
                                        csc->objects[o_pos].byteorder,
                                        csc->objects[o_pos].size,
                                        GET_CAN_INIT_DATA_OBJECT(csc, o_pos));
                    csc->objects[o_pos].Protocol.Mux.Mask = BuildMask32 (csc->objects[o_pos].Protocol.Mux.Size);
                    csc->objects[o_pos].Protocol.Mux.master = -2;
                }
                
                IniFileDataBaseReadString (section, "cycles", "1", txt, sizeof (txt), par_Fd);
                {
                    char *endp;
                    csc->objects[o_pos].cycles = strtol (txt, &endp, 0);
                    csc->objects[o_pos].cycle_is_execst = 0;
                    if (*endp != 0) {  // is it a formula?
                        int UseCANData = 0;
                        if ((ExecStack = TranslateString2ByteCode (txt, sizeof (txt), &Size, &UseCANData)) != NULL) {
                            csc = AddByteCode2CanCfg (csc, &HelpInt, (char*)ExecStack, Size);
                            // Not remmove_exec_stack but only a free (attach counter wil not be decremented)
                            my_free (ExecStack);
                            RegisterEquation (REGISTER_EQU_PID, txt, (struct EXEC_STACK_ELEM *)&csc->Symbols[HelpInt], "", EQU_TYPE_CAN);
                            csc->objects[o_pos].cycles = HelpInt;
                            csc->objects[o_pos].cycle_is_execst = 1;
                        } else csc->objects[o_pos].cycles = 1;
                    }
                }
                IniFileDataBaseReadString (section, "delay", "0", txt, sizeof (txt), par_Fd);
                HelpDouble = direct_solve_equation (txt);
                csc->objects[o_pos].delay = (int)HelpDouble;
                csc->objects[o_pos].cycles_counter = 0;
                csc->objects[o_pos].signal_count = IniFileDataBaseReadInt (section, "signal_count", 0, par_Fd);

                csc = AllocMemInsideCanCfg (csc, &SignalIndexListOffset, csc->objects[o_pos].signal_count * (int)sizeof (short));
                if (csc == NULL) {
                    return NULL;
                }
                csc->objects[o_pos].SignalsArrayIdx = SignalIndexListOffset;
                // This addresse muss must be calculated each time it will be used!
                csc->objects[o_pos].signals_ptr = (short*)(void*)&(csc->Symbols[SignalIndexListOffset]);

                IniFileDataBaseReadString (section, "CyclicOrEvent", "Cyclic", txt, sizeof (txt), par_Fd);
                if (!strcmpi ("Event", txt)) {
                    int UseCANData = 0;
                    csc->objects[o_pos].EventOrCyclic = 1;   // Transmit-Event
                    IniFileDataBaseReadString (section, "EventEquation", "", txt, sizeof (txt), par_Fd);
                    if ((ExecStack = TranslateString2ByteCode (txt, sizeof (txt), &Size, &UseCANData)) == NULL) {
                        csc->objects[o_pos].EventOrCyclic = 0;
                    } else {
                        csc = AddByteCode2CanCfg (csc, &HelpInt, (char*)ExecStack, Size);
                        // Not remmove_exec_stack but only a free (attach counter wil not be decremented)
                        my_free (ExecStack);
                        RegisterEquation (REGISTER_EQU_PID, txt, (struct EXEC_STACK_ELEM *)&csc->Symbols[HelpInt], "", EQU_TYPE_CAN);
                        csc->objects[o_pos].EventIdx = HelpInt;
                        if (UseCANData) csc->objects[o_pos].EventOrCyclic = 2;
                    }
                } else {
                    csc->objects[o_pos].EventOrCyclic = 0;   // Transmit-Cyclic
                }

                // additional blackboard variables in CAN message
                for (xx = x = 0; x < MAX_ADDITIONAL_EQUS; x++) {  // max. 100 equations
                    int dtype;
                    char *p, *n, *d;
                    sprintf (entry, "additional_variable_%i", x);
                    if (IniFileDataBaseReadString (section, entry, "", txt, sizeof (txt), par_Fd) <= 0) {
                        break;
                    }
                    p = txt;
                    if (GetDtypeString ("BYTE", &p, &d, &n)) dtype = BB_BYTE;
                    else if (GetDtypeString ("UBYTE", &p, &d, &n)) dtype = BB_UBYTE;
                    else if (GetDtypeString ("WORD", &p, &d, &n)) dtype = BB_WORD;
                    else if (GetDtypeString ("UWORD", &p, &d, &n)) dtype = BB_UWORD;
                    else if (GetDtypeString ("DWORD", &p, &d, &n)) dtype = BB_DWORD;
                    else if (GetDtypeString ("UDWORD", &p, &d, &n)) dtype = BB_UDWORD;
                    else if (GetDtypeString ("QWORD", &p, &d, &n)) dtype = BB_QWORD;
                    else if (GetDtypeString ("UQWORD", &p, &d, &n)) dtype = BB_UQWORD;
                    else if (GetDtypeString ("FLOAT", &p, &d, &n)) dtype = BB_FLOAT;
                    else if (GetDtypeString ("DOUBLE", &p, &d, &n)) dtype = BB_DOUBLE;
                    else dtype = -1;
                    if (dtype != -1) {
                        HelpIntArray[xx++] = add_bbvari (n, dtype, "");
                    }
                }
                if (xx) {
                    HelpIntArray[xx++] = 0;  // End of the list
                    csc = AddByteCode2CanCfg (csc, &HelpInt, (char*)HelpIntArray, (int)sizeof (int) * xx);
                    csc->objects[o_pos].AdditionalVariablesIdx = HelpInt;
                } else {
                    csc->objects[o_pos].AdditionalVariablesIdx = 0; // there are no
                }
                // additional equations before transmit/receive CAN message
                for (xx = x = 0; x < MAX_ADDITIONAL_EQUS; x++) {  // max. 100 Equations
                    int UseCANData;
                    sprintf (entry, "equ_before_%i", x);
                    if (IniFileDataBaseReadString (section, entry, "", txt, sizeof (txt), par_Fd) <= 0) {
                        break;
                    }
                    if ((ExecStack = TranslateString2ByteCode (txt, sizeof (txt), &Size, &UseCANData)) != NULL) {
                        csc = AddByteCode2CanCfg (csc, &HelpInt, (char*)ExecStack, Size);
                        // Not remmove_exec_stack but only a free (attach counter wil not be decremented)
                        my_free (ExecStack);
                        RegisterEquation (REGISTER_EQU_PID, txt, (struct EXEC_STACK_ELEM *)&csc->Symbols[HelpInt], "", EQU_TYPE_CAN);
                        HelpIntArray[xx++] = HelpInt;
                    }
                }
                if (xx) {
                    HelpIntArray[xx++] = 0;  // End of the liste
                    csc = AddByteCode2CanCfg (csc, &HelpInt, (char*)HelpIntArray, (int)sizeof (int) * xx);
                    csc->objects[o_pos].EquationBeforeIdx = HelpInt;
                } else {
                    csc->objects[o_pos].EquationBeforeIdx = 0;  // not existing
                }
                // additional equations behind transmit/receive CAN message
                for (xx = x = 0; x < MAX_ADDITIONAL_EQUS; x++) {  // max. 100 equations
                    int UseCANData;
                    sprintf (entry, "equ_behind_%i", x);
                    if (IniFileDataBaseReadString (section, entry, "", txt, sizeof (txt), par_Fd) <= 0) {
                        break;
                    }
                    if ((ExecStack = TranslateString2ByteCode (txt, sizeof (txt), &Size, &UseCANData)) != NULL) {
                        csc = AddByteCode2CanCfg (csc, &HelpInt, (char*)ExecStack, Size);
                        // Not remmove_exec_stack but only a free (attach counter wil not be decremented)
                        my_free (ExecStack);
                        RegisterEquation (REGISTER_EQU_PID, txt, (struct EXEC_STACK_ELEM *)&csc->Symbols[HelpInt], "", EQU_TYPE_CAN);
                        HelpIntArray[xx++] = HelpInt;
                    }
                }
                if (xx) {
                    HelpIntArray[xx++] = 0;  // End of the list
                    csc = AddByteCode2CanCfg (csc, &HelpInt, (char*)HelpIntArray, (int)sizeof (int) * xx);
                    csc->objects[o_pos].EquationBehindIdx = HelpInt;
                } else {
                    csc->objects[o_pos].EquationBehindIdx = 0; // none
                }
                // all signale of the objects
                for (s = 0; s < csc->objects[o_pos].signal_count; s++) {
                    if (s >= MAX_SIGNALS_ONE_OBJECT) {
                        ThrowError (1, "max. %i signals for one object allowed", (int)MAX_SIGNALS_ONE_OBJECT);
                        remove_canserver_config (csc);
                        return NULL;
                    }
                    if (s_pos >= MAX_SIGNALS_ALL_CHANNELS) {
                        ThrowError (1, "max. %i signals for all channels allowed", (int)MAX_SIGNALS_ALL_CHANNELS);
                        remove_canserver_config (csc);
                        return NULL;
                    }
                    sprintf (section, "CAN/Variante_%i/Object_%i/Signal_%i", varianten, o, s);
                    if (IniFileDataBaseReadString (section, "name", "", Name, 512, par_Fd) <= 0) {
                        ThrowError (ERROR_NO_STOP, "signal \"%s\" seems to be missing or \"signal_count\" of the parent object are wrong", section);
                        csc->objects[o_pos].signal_count = s;
                        break;
                    }

                    IniFileDataBaseReadString (section, "unit", "", Unit, 64, par_Fd);

                    Size = 0;
                    ExecStack = NULL;

                    IniFileDataBaseReadString (section, "bbtype", "", txt, sizeof (txt), par_Fd);
                    csc->bb_signals[s_pos].bbtype = GetDataTypebyName(txt);
                    if (csc->bb_signals[s_pos].bbtype < 0) csc->bb_signals[s_pos].bbtype = BB_UNKNOWN_DOUBLE;

                    csc->bb_signals[s_pos].vid = add_bbvari (Name, csc->bb_signals[s_pos].bbtype, Unit);
                    
                    if (csc->bb_signals[s_pos].vid < 0) {
                        //ThrowError (1, "cannot add variable \"%s\", ignoring", Name);
                        switch (csc->bb_signals[s_pos].vid) {
                        case -819: //TYPE_CLASH:
                            ThrowError (ERROR_NO_STOP, "cannot attach variable \"%s\" because variable exists with other data type (channel %i, object id 0x%X)", 
                                   Name, c, csc->objects[o_pos].id);
                            break;
                        case -824: //LABEL_NAME_NOT_VALID
                            ThrowError (ERROR_NO_STOP, "cannot attach variable \"%s\" because name is not a valid variable name (channel %i, object id 0x%X)", 
                                   Name, c, csc->objects[o_pos].id);
                            break;
                        default:
                            ThrowError (ERROR_NO_STOP, "cannot attach variable \"%s\" error code %i  (channel %i, object id 0x%X)", 
                                   Name, csc->bb_signals[s_pos].vid, c, csc->objects[o_pos].id);
                            break;
                        }
                    }

                    IniFileDataBaseReadString (section, "convtype", "mx+b", txt, sizeof (txt), par_Fd);
                    if (!strcmp (txt, "mx+b")) {
                        csc->bb_signals[s_pos].ConvType = CAN_CONV_FACTOFF;
                        ;  // doing nothing
                        Size = 0;
				    } else if (!strcmp (txt, "m(x+b)")) {
                        csc->bb_signals[s_pos].ConvType = CAN_CONV_FACTOFF2;
                        ;  // doing nothing
                        Size = 0;
                    } else if (!strcmp (txt, "curve")) {
                        csc->bb_signals[s_pos].ConvType = CAN_CONV_CURVE;
                        IniFileDataBaseReadString (section, "convstring", "", txt, sizeof (txt), par_Fd);
                        if (TranslateString2Curve (txt, sizeof (txt), &Size)) csc->bb_signals[s_pos].ConvType = CAN_CONV_NONE;
                    } else if (!strcmp (txt, "equation")) {
                        IniFileDataBaseReadString (section, "convstring", "", txt, sizeof (txt), par_Fd);
                        if (strlen (txt) == 0) {   // No formula string
                            ThrowError (1, "in CAN configuration for variable \"%s\" is an equation selected for convertion but not defined\n"
                                      "using none convertion instead", Name);
                            csc->bb_signals[s_pos].ConvType = CAN_CONV_NONE;
                        } else {
                            csc->bb_signals[s_pos].ConvType = CAN_CONV_EQU;
                            if ((ExecStack = TranslateString2ByteCode (txt, sizeof (txt), &Size, NULL)) == NULL) {
                                csc->bb_signals[s_pos].ConvType = CAN_CONV_NONE;
                            }
                        }
                    } else csc->bb_signals[s_pos].ConvType = CAN_CONV_NONE;

                    csc = AddSymbol2CanCfg (csc, s_pos, Name, Unit, txt, ExecStack, Size);
                    // Not remmove_exec_stack but only a free (attach counter wil not be decremented)
                    if (ExecStack != NULL) my_free (ExecStack);
                    IniFileDataBaseReadString (section, "type", "", txt, sizeof (txt), par_Fd);
                    if (!strcmpi ("mux", txt)) csc->bb_signals[s_pos].type = MUX_SIGNAL;
                    else if (!strcmpi ("mux_by", txt)) csc->bb_signals[s_pos].type = MUX_BY_SIGNAL;
                    else csc->bb_signals[s_pos].type = NORMAL_SIGNAL;
                    IniFileDataBaseReadString (section, "byteorder", "", txt, sizeof (txt), par_Fd);
                    if (!strcmpi ("msb_first", txt)) csc->bb_signals[s_pos].byteorder = MSB_FIRST_BYTEORDER;
                    else csc->bb_signals[s_pos].byteorder = LSB_FISRT_BYTEORDER;
                    csc->bb_signals[s_pos].startbit = IniFileDataBaseReadInt (section, "startbit", 0, par_Fd);
                    csc->bb_signals[s_pos].bitsize = IniFileDataBaseReadInt (section, "bitsize", 0, par_Fd);
                    csc->bb_signals[s_pos].mux_startbit = IniFileDataBaseReadInt (section, "mux_startbit", 0, par_Fd);
                    csc->bb_signals[s_pos].mux_bitsize = IniFileDataBaseReadInt (section, "mux_bitsize", 0, par_Fd);
                    csc->bb_signals[s_pos].mux_value = IniFileDataBaseReadInt (section, "mux_value", 0, par_Fd);
                    if (csc->bb_signals[s_pos].type == MUX_SIGNAL) {
                        ResetDataBitsValue (csc->bb_signals[s_pos].mux_startbit,
                                            csc->bb_signals[s_pos].mux_bitsize,
                                            csc->bb_signals[s_pos].byteorder,
                                            csc->objects[o_pos].size,
                                            GET_CAN_INIT_DATA_OBJECT(csc, o_pos));
                        csc->bb_signals[s_pos].mux_mask = BuildMask32 (csc->bb_signals[s_pos].mux_bitsize);
                    } else if (csc->bb_signals[s_pos].type == MUX_BY_SIGNAL) {
                        if (IniFileDataBaseReadString (section, "mux_by_signal", "", txt, sizeof (txt), par_Fd) > 0) {
                            csc->bb_signals[s_pos].mux_startbit = add_bbvari (txt, BB_UNKNOWN_WAIT, "");
                            if (csc->bb_signals[s_pos].mux_startbit <= 0) {
                                ThrowError (ERROR_NO_STOP, "cannot attach multiplexed by variable \"%s\" error code %i  (channel %i, object id 0x%X)",
                                       txt, csc->bb_signals[s_pos].mux_startbit, c, csc->objects[o_pos].id);
                            }
                        }
                    }
                    IniFileDataBaseReadString (section, "convert", "", txt, sizeof (txt), par_Fd);
                    csc->bb_signals[s_pos].conv_type = string_to_FloatOrInt64(txt, &csc->bb_signals[s_pos].conv);
                    IniFileDataBaseReadString (section, "offset", "", txt, sizeof (txt), par_Fd);
                    csc->bb_signals[s_pos].offset_type = string_to_FloatOrInt64(txt, &csc->bb_signals[s_pos].offset);

                    // If factor 1.0 and offset 0 switch of conversion
                    if (((csc->bb_signals[s_pos].ConvType == CAN_CONV_FACTOFF) ||
                         (csc->bb_signals[s_pos].ConvType == CAN_CONV_FACTOFF2)) &&
                        is_equal_FloatOrInt64(1.0, csc->bb_signals[s_pos].conv, csc->bb_signals[s_pos].conv_type) &&
                        is_equal_FloatOrInt64(0.0, csc->bb_signals[s_pos].offset, csc->bb_signals[s_pos].offset_type)) {
                        csc->bb_signals[s_pos].ConvType = CAN_CONV_NONE;
                    }

                    IniFileDataBaseReadString (section, "startvalue active", "yes", txt, sizeof (txt), par_Fd);
                    if (strcmpi ("yes", txt) == 0) {
                        csc->bb_signals[s_pos].start_value_flag = 1;
                        IniFileDataBaseReadString (section, "startvalue", "0", txt, sizeof (txt), par_Fd);
                        csc->bb_signals[s_pos].start_value_type = string_to_FloatOrInt64(txt, &csc->bb_signals[s_pos].start_value);
                        if (!(csc->dont_use_init_values_for_existing_variables && 
                            (get_bbvari_attachcount (csc->bb_signals[s_pos].vid) > 1))) {
                            write_bbvari_convert_with_FloatAndInt64 (csc->bb_signals[s_pos].vid, csc->bb_signals[s_pos].start_value, csc->bb_signals[s_pos].start_value_type);
                        }
                    } else {
                        csc->bb_signals[s_pos].start_value_flag = 0;
                        csc->bb_signals[s_pos].start_value.d = 0.0;
                        csc->bb_signals[s_pos].start_value_type = FLOAT_OR_INT_64_TYPE_F64;
                    }
                    IniFileDataBaseReadString (section, "sign", "unsigned", txt, sizeof (txt), par_Fd);
                    if (!strcmpi ("signed", txt)) csc->bb_signals[s_pos].sign = 1;
                    else csc->bb_signals[s_pos].sign = 0;
                    if (!strcmpi ("float", txt)) csc->bb_signals[s_pos].float_type = 1;
                    else csc->bb_signals[s_pos].float_type = 0;

                    ResetDataBitsValue (csc->bb_signals[s_pos].startbit,
                                        csc->bb_signals[s_pos].bitsize,
                                        csc->bb_signals[s_pos].byteorder,
                                        csc->objects[o_pos].size,
                                        GET_CAN_INIT_DATA_OBJECT(csc, o_pos));
                    csc->bb_signals[s_pos].mask = BuildMask64 (csc->bb_signals[s_pos].bitsize);

                    // Init MUX signal states
                    csc->bb_signals[s_pos].mux_next = -1;
                    csc->bb_signals[s_pos].mux_equ_sb_childs = -1;
                    csc->bb_signals[s_pos].mux_equ_v_childs = -1;

                    // this must be recalculate each time it is used! csc can be moved!
                    csc->objects[o_pos].signals_ptr = (short*)(void*)&(csc->Symbols[SignalIndexListOffset]);

                    csc->objects[o_pos].signals_ptr[s] = (int16_t)s_pos;
                    s_pos++;
                }

                csc->channels[c].objects[oo] = (int16_t)o_pos;
                o_pos++;
            }
        }

        // all Objects of the channel
        for (o = 0; o < csc->channels[c].object_count; o++) {
            int o_p, oi, master_o_pos, on;
            o_p = csc->channels[c].objects[o];

            if (csc->objects[o_p].type == J1939_22_MULTI_C_PG) {
                C_PGs_Translate(csc, c, o_p);  // J1939 22 Multi C_PGs translate IDs in positions
            }
            SortMuxSignals (csc, o_p);
            if ((csc->objects[o_p].type == MUX_OBJECT) &&
                (csc->objects[o_p].Protocol.Mux.master == -2)) {
                csc->objects[o_p].Protocol.Mux.master = -1;  // Master Mux-Objekt
                csc->objects[o_p].Protocol.Mux.token = o_p;
                on = master_o_pos = o_p;
                for (oi = o; oi < csc->channels[c].object_count; oi++) {
                    o_p = csc->channels[c].objects[oi];
                    if ((csc->objects[o_p].type == MUX_OBJECT) &&
                        (csc->objects[o_p].Protocol.Mux.master == -2) &&
                        (csc->objects[master_o_pos].id == csc->objects[o_p].id)) {
                        csc->objects[o_p].Protocol.Mux.master = master_o_pos;
                        csc->objects[o_p].Protocol.Mux.next = on;
                        csc->objects[o_p].Protocol.Mux.token = 0;
                        on = o_p;
                    }
                }
                csc->objects[master_o_pos].Protocol.Mux.next = on;
            }
        }
        // For faster search of objects there exists 2 sorted  arrays (one for TX and one for RX objects)
        // alle Objekte des Kanals
        csc->channels[c].rx_object_count = csc->channels[c].tx_object_count = 0;
        for (o = 0; o < csc->channels[c].object_count; o++) {
            o_p = csc->channels[c].objects[o];
            if (csc->objects[o_p].dir == READ_OBJECT) {       // read object
                if ((csc->objects[o_p].type == NORMAL_OBJECT) ||   // normal object
                    (csc->objects[o_p].type == J1939_22_MULTI_C_PG) ||   // J1939 22 multi C_PG
                    // J1939 Objekt nicht einsortieren
                    ((csc->objects[o_p].type == MUX_OBJECT) && (csc->objects[o_p].Protocol.Mux.master == -1))) { // or Master MUX-Objekt
                    csc->channels[c].hash_rx[csc->channels[c].rx_object_count].id = (uint32_t)csc->objects[o_p].id;
                    csc->channels[c].hash_rx[csc->channels[c].rx_object_count].pos = o_p;
                    csc->channels[c].rx_object_count++;
                }
            } else if ((csc->objects[o_p].dir == WRITE_OBJECT) ||
                       (csc->objects[o_p].dir == WRITE_VARIABLE_ID_OBJECT)) {
                if ((csc->objects[o_p].type == NORMAL_OBJECT) ||   // normal object
                    (csc->objects[o_p].type == J1939_OBJECT)  ||   // J1939 object
                    (csc->objects[o_p].type == J1939_22_C_PG) ||   // J1939 22 C_PG
                    (csc->objects[o_p].type == J1939_22_MULTI_C_PG) ||   // J1939 22 multi C_PG
                    ((csc->objects[o_p].type == MUX_OBJECT) && (csc->objects[o_p].Protocol.Mux.master == -1))) { // or Master MUX-Objekt
                    csc->channels[c].hash_tx[csc->channels[c].tx_object_count].id = (uint32_t)csc->objects[o_p].id;
                    csc->channels[c].hash_tx[csc->channels[c].tx_object_count].pos = o_p;
                    csc->channels[c].tx_object_count++;
                }
            }
        }
        for (x = csc->channels[c].rx_object_count; x < MAX_RX_OBJECTS_ONE_CHANNEL; x++) {
            csc->channels[c].hash_rx[x].id = 0xFFFFFFFFUL;
            csc->channels[c].hash_rx[x].pos = 0;
        }
        for (x = csc->channels[c].tx_object_count; x < MAX_TX_OBJECTS_ONE_CHANNEL; x++) {
            csc->channels[c].hash_tx[x].id = 0xFFFFFFFFUL;
            csc->channels[c].hash_tx[x].pos = 0;
        }

        // Sort CAN objects on the basis of there ID's
        qsort((void *)csc->channels[c].hash_rx, MAX_RX_OBJECTS_ONE_CHANNEL, sizeof(NEW_CAN_HASH_ARRAY_ELEM), qsort_function);
        Sort_Multi_C_PG_Tx(csc, c);

    }
    return csc;
}


int SendAddOrRemoveReplaceCanSigConvReq (int par_Channel, int par_Id, char *par_SignalName, struct EXEC_STACK_ELEM *par_ExecStack, int par_UseCANData) 
{
    struct EXEC_STACK_ELEM *ExecStack;
    int LenOfSignalName = 0;
    int LenOfExecStack = 0;
    CAN_SET_SIG_CONV *cssc;
    int SizeOfStruct;

    if (par_SignalName != NULL) {
        LenOfSignalName = (int)strlen (par_SignalName) + 1;
        if (par_ExecStack != NULL) {
            ExecStack = par_ExecStack;
            LenOfExecStack = sizeof_exec_stack (ExecStack);
        } else {
            LenOfExecStack = -1;
        }
    } else {
        LenOfSignalName = -1;
        LenOfExecStack = -1;
    }

    SizeOfStruct = (int)sizeof (CAN_SET_SIG_CONV) + LenOfSignalName + LenOfExecStack + 2;
    cssc = _alloca ((size_t)SizeOfStruct);
    cssc->SizeOfStruct = SizeOfStruct;
    cssc->Channel = par_Channel;
    cssc->Id = par_Id;
    cssc->UseCANData = par_UseCANData;
    if (LenOfSignalName == -1) cssc->IdxOfSignalName = -1;
    else cssc->IdxOfSignalName = 0;
    cssc->LenOfSignalName = LenOfSignalName;
    if (LenOfSignalName > 0) StringCopyMaxCharTruncate (&cssc->Data[cssc->IdxOfSignalName], par_SignalName, LenOfSignalName);
    cssc->IdxOfExecStack = LenOfSignalName;
    if (LenOfExecStack > 0) {
        MEMCPY (&cssc->Data[cssc->IdxOfExecStack], ExecStack, (size_t)LenOfExecStack);
    } else {
        cssc->IdxOfExecStack = -1;
    }
    cssc->LenOfExecStack = LenOfExecStack;
    write_message (get_pid_by_name ("CANServer"),
                                    NEW_CANSERVER_SET_SIG_CONV,
                                    SizeOfStruct,
                                    (char*)cssc);
    return 0;
}


