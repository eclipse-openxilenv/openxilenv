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
#include <limits.h>
#include <float.h>
#include <string.h>
#include <math.h>
#include "ThrowError.h"
#include "MyMemory.h"
#include "StringMaxChar.h"
#include "Message.h"
#include "tcb.h"
#include "Scheduler.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "ExecutionStack.h"
#include "ReadCanCfg.h"
#include "CanReplaceConversion.h"
#include "CanFifo.h"
#include "CcpAndXcpFilterHook.h"
#include "Compare2DoubleEqual.h"
#ifdef REMOTE_MASTER
#include "MapSocketCAN.h"
#include "ReadWriteSocketCAN.h"
#else
#include "IniDataBase.h"
#include "VirtualCanDriver.h"
#if BUILD_WITH_GATEWAY_VIRTUAL_CAN
#include "GatewayCanDriver.h"
#endif
#endif
#include "ThrowError.h"
#include "ConfigurablePrefix.h"

#ifdef BUILD_WITH_GATEWAY_VIRTUAL_CAN
#include "GatewayCanDriver.h"
#endif
#include "CanServer.h"

#define UNUSED(x) (void)(x)

#define INC_CCP        1

#ifdef BUILD_WITH_J1939_SUPPORT
#include "J1939_21_MultiPackage.h"
#endif

static int can_init_flag = 0;
#define CANSERVER_NOT_INIT        0
#define CANSERVER_READ_INIT       1
#define CANSERVER_SELECT_CAN_CARD 2
#define CANSERVER_OPEN_CAN        3
#define CANSERVER_CYCLIC          4
#define CANSERVER_STOP            5

/* This is a pointer to the complete CAN server configuration */
static NEW_CAN_SERVER_CONFIG *CanServerConfig;

/* CAN fault simulation */
static CAN_BIT_ERROR CanBitError;
static VID CanBitErrorVid;
static uint64_t CycleAtStartPoint;

static int32_t CanServerPid;

#ifdef REMOTE_MASTER

static int SocketCAN_CardInfos(int Pos, CAN_CARD_INFOS *CanCardInfos)
{
    int x, y;

    // Dann die Flexcards
    for (y = 0, x = Pos; (x < MAX_CAN_CHANNELS) && (y < GetNumberOfFoundCanChannelsSocketCAN()); y++, x++) {
        CanCardInfos->Cards[x].CardType = CAN_CARD_TYPE_SOCKETCAN;
        strcpy(CanCardInfos->Cards[x].CANCardString, "SocketCAN");
        CanCardInfos->Cards[x].State = 1;
        CanCardInfos->Cards[x].NumOfChannels = 1;
    }
    return x;
}


static int SendCANCardInfosToProcess (int SendToPid)
{
    CAN_CARD_INFOS CanCardInfos;
    int x, y, Ret;

    memset (&CanCardInfos, 0, sizeof (CanCardInfos));
    remove_message ();
    CanCardInfos.NumOfChannels = GetNumberOfFoundCanChannelsSocketCAN();
    x = 0;
    x = SocketCAN_CardInfos(x, &CanCardInfos);
    CanCardInfos.NumOfCards = x;
    Ret = write_message (SendToPid, GET_CAN_CARD_INFOS_ACK, sizeof (CanCardInfos), (char*)&CanCardInfos);
    // ThrowError(RT_INFO_MESSAGE, "CAN-Cards transmit status %i", Ret);
    return Ret;
}
#else
#define get_rt_cycle_counter() (blackboard_infos.ActualCycleNumber)
#endif


int new_canserv_init (void)
{
    char Name[BBVARI_NAME_SIZE];

    can_init_flag = CANSERVER_NOT_INIT;

    CanServerPid = GET_PID();

    if (sizeof (CAN_FIFO_ELEM) != 32) {
        ThrowError (1, "sizeof (CAN_FIFO_ELEM) = %i != 32", sizeof (CAN_FIFO_ELEM));
    }
    if (sizeof (CAN_FD_FIFO_ELEM) != 88) {
        ThrowError (1, "sizeof (CAN_FD_FIFO_ELEM) = %i != 88", sizeof (CAN_FD_FIFO_ELEM));
    }
    if (sizeof (CAN_ACCEPT_MASK) != 16) {
        ThrowError (1, "sizeof (CAN_ACCEPT_MASK) = %i != 16", sizeof (CAN_ACCEPT_MASK));
    }
    if (sizeof (NEW_CAN_SERVER_OBJECT) != 128) {
        ThrowError (1, "sizeof (NEW_CAN_SERVER_OBJECT) = %i != 128", sizeof (NEW_CAN_SERVER_OBJECT));
    }
    if (sizeof (NEW_CAN_SERVER_SIGNAL) != 128) {
        ThrowError (1, "sizeof (NEW_CAN_SERVER_SIGNAL) = %i != 128", sizeof (NEW_CAN_SERVER_SIGNAL));
    }
    if (sizeof (sJ1939Tp_MP_RxChannel) != 64) {
        ThrowError (1, "sizeof (sJ1939Tp_MP_RxChannel) = %i != 64", sizeof (sJ1939Tp_MP_RxChannel));
    }
    if (sizeof (sJ1939Tp_MP_TxChannel) != 64) {
        ThrowError (1, "sizeof (sJ1939Tp_MP_TxChannel) = %i != 64", sizeof (sJ1939Tp_MP_TxChannel));
    }
    if (sizeof (CanServerConfig->channels[0]) != 8192) {
        ThrowError (1, "sizeof (CanServerConfig->channels[0]) = %i != 8192", sizeof (CanServerConfig->channels[0]));
    }

    CanBitError.Id = -1;   // Do not simulate bit errors

#ifdef REMOTE_MASTER
    SendCANCardInfosToProcess (get_pid_by_name ("RemoteMasterControl"));
#endif
    CanBitErrorVid = add_bbvari (ExtendsWithConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES, ".CANBitError.Counter", Name, sizeof(Name)),
                                 BB_UDWORD, "");
    return 0;
}

__inline static uint64_t ConvMinMaxCheckSign (union FloatOrInt64 value, int type, int size, int sign)
{
    uint64_t max;
    int64_t min;

    int64_t sl;
    uint64_t ul = 0;

    switch(type) {
    case FLOAT_OR_INT_64_TYPE_INT64:
        if (sign) {
            min = -(0x1L << (size-1));
            max = (0x1UL << (size-1)) - 1;
            if (value.qw <= min) value.qw = min;
            if (value.qw >= (int64_t)max) value.qw = (int64_t)max;
            sl = value.qw;
            sl = sl << (64 - size);
            ul = (uint64_t)sl;
            ul = ul >> (64 - size);
        } else {
            if (size >= 64) max = 0xFFFFFFFFFFFFFFFFULL;
            else max = (0x1ULL << size) - 1;
            min = 0;
            if (value.qw <= min) value.qw = min;
            if (value.qw >= (int64_t)max) value.qw = (int64_t)max;
            ul = (uint64_t)value.qw;
        }
        break;
    case FLOAT_OR_INT_64_TYPE_UINT64:
        if (sign) {
            min = 0; //-(0x1L << (size-1));
            max = (0x1UL << (size-1)) - 1;
            if (value.uqw <= (uint64_t)min) value.uqw = (uint64_t)min;
            if (value.uqw >= max) value.uqw = max;
            sl = (int64_t)value.uqw;
            sl = sl << (64 - size);
            ul = (uint64_t)sl;
            ul = ul >> (64 - size);
        } else {
            if (size >= 64) max = 0xFFFFFFFFFFFFFFFFULL;
            else max = (0x1ULL << size) - 1;
            min = 0;
            if (value.uqw <= (uint64_t)min) value.uqw = (uint64_t)min;
            if (value.uqw >= max) value.uqw = max;
            ul = value.uqw;
        }
        break;
    case FLOAT_OR_INT_64_TYPE_F64:
        if (sign) {
            double d;
            min = -(0x1L << (size-1));
            max = (0x1UL << (size-1)) - 1;
            if (value.d <= (double)min) value.d = (double)min;
            if (value.d >= (double)max) value.d = (double)max;
            d = round(value.d);
            sl = (int64_t)d;
            sl = sl << (64 - size);
            ul = (uint64_t)sl;
            ul = ul >> (64 - size);
        } else {
            double d;
            if (size >= 64) max = 0xFFFFFFFFFFFFFFFFULL;
            else max = (0x1ULL << size) - 1;
            min = 0;
            if (value.d <= (double)min) value.d = (double)min;
            if (value.d >= (double)max) value.d = (double)max;
            d = round(value.d);
            ul = (uint64_t)d;
        }
        break;
    }
    return ul;
}

__inline static uint32_t ConvMinMaxCheckFloat (union FloatOrInt64 value, int type)
{
    float f;
    uint32_t ul;

    double v = to_double_FloatOrInt64(value, type);
    if (v > (double)FLT_MAX) f = FLT_MAX;
    else if (v < (double)-FLT_MAX) f = -FLT_MAX;
    else f = (float)v;

    ul = *(uint32_t*)&f;
    return ul;
}



__inline static int ConvWithSign (uint64_t value, int size, int sign, union FloatOrInt64 *ret_value)
{
    if (sign) {
        if ((0x1ULL << (size - 1)) & value) { // Is a sign set?
            if (size < 64) {
                ret_value->qw = (int64_t)(value - (0x1ULL << size));
            } else {
                ret_value->qw = (int64_t)value;
            }
        } else ret_value->qw = (int64_t)value;
        return FLOAT_OR_INT_64_TYPE_INT64;
    } else {
        ret_value->uqw = value;
        return FLOAT_OR_INT_64_TYPE_UINT64;
    }
}

__inline static double ConvWithFloat (uint32_t v)
{
    float ret;

    ret = *(float*)&v;
    return (double)ret;
}


static int ConvertSignal (NEW_CAN_SERVER_CONFIG *csc, int o_pos, int s_pos, union FloatOrInt64 *value, int *type)
{
    NEW_CAN_SERVER_SIGNAL *ps;
    int x, dim;
    double *p;
    double x1, x2;
    double y1, y2;
    double m;

    ps = &(csc->bb_signals[s_pos]);

    switch (ps->ConvType) {
    case CAN_CONV_NONE:
    default:
        // do nothing
        break;
    case CAN_CONV_FACTOFF:
        *type = mul_FloatOrInt64(ps->conv, ps->conv_type, *value, *type, value);
        *type = add_FloatOrInt64(ps->offset, ps->offset_type, *value, *type, value);
        break;
    case CAN_CONV_FACTOFF2:
        *type = add_FloatOrInt64(ps->offset, ps->offset_type, *value, *type, value);
        *type = mul_FloatOrInt64(ps->conv, ps->conv_type, *value, *type, value);
        break;
    case CAN_CONV_EQU:
        *type = execute_stack_whith_can_parameter ((struct EXEC_STACK_ELEM *)GET_CAN_SIG_BYTECODE(csc, s_pos), *value, *type,
                                                   &csc->objects[o_pos], value);
        break;
    case CAN_CONV_CURVE:
        p = (double*)(void*)GET_CAN_SIG_BYTECODE(csc, s_pos);
        dim = (int)*p;
        p += 2;
        if ((dim < 2) || (dim > 255)) break;  // max. 255 points allowed
        y2 = y1 = *p++;
        x2 = x1 = *p++;
        for (x = 0; x < (dim-1); x++) {
            x2 = x1;
            y2 = y1;
            y1 = *p++;
            x1 = *p++;
            if (value->d <= x1) break;
        }
        if (CompareDoubleEqual_int(x1, x2)) value->d = y1;
        else if (value->d <= x2) value->d = y2;
        else if (value->d >= x1) value->d = y1;
        else {
            m = (y1-y2)/(x1-x2);
            value->d = y2 + (value->d - x2) * m;
        }
        break;
    case CAN_CONV_REPLACED:
        *type = CalcReplaceCanSigConv (ps->ConvIdx, *value, *type, &(csc->objects[o_pos]), value);
        break;
    }
    return 0;
}

static __inline void CalcEquationArray (int Idx, NEW_CAN_SERVER_OBJECT *po)
{
    union FloatOrInt64 value;
    int type = FLOAT_OR_INT_64_TYPE_F64;
    value.d = 0.0;
    int *pIdx = (int*)(void*)&CanServerConfig->Symbols[Idx];
    do {
        execute_stack_whith_can_parameter ((struct EXEC_STACK_ELEM *)&CanServerConfig->Symbols[*pIdx], value, type, po, &value);
        pIdx++;
    } while (*pIdx != 0);
}

#ifdef __GNUC__
#define _byteswap_ulong __builtin_bswap32
#define _byteswap_uint64 __builtin_bswap64
#endif

// Read one signals bits from the object data
static __inline uint64_t GetValue (NEW_CAN_SERVER_OBJECT *po,
                                   NEW_CAN_SERVER_SIGNAL *ps)
{
    uint32_t PosByte;
    uint32_t PosBit;
    uint64_t Ret;

    if (ps->byteorder == MSB_FIRST_BYTEORDER) {  // 1 -> MSB_FISRT
        int32_t PosByte_m8;
        uint32_t IStartBit = (uint32_t)((po->size << 3) - ps->startbit);
        PosBit = IStartBit & 0x7;
        PosByte = IStartBit >> 3;
        PosByte_m8 = (int32_t)PosByte - 8;
        Ret = _byteswap_uint64 (*(uint64_t *)(void*)(po->Data + PosByte_m8)) << PosBit;
        Ret |= (uint64_t)po->Data[PosByte] >> (8 - PosBit);
    } else {  // 0 -> LSB first
        PosBit = ps->startbit & 0x7;
        PosByte = (uint32_t)(ps->startbit >> 3);
        Ret = *(uint64_t *)(void*)(po->Data + PosByte) >> PosBit;
        Ret |= ((uint64_t)po->Data[PosByte + 8] << (8 - PosBit)) << 56;
   }
    return Ret &= ps->mask;
}


// Remark Data and OldData arrays must be +-7 Bytes larger as needed!
static __inline void SetValue (uint64_t Value,
                               NEW_CAN_SERVER_OBJECT *po,
                               NEW_CAN_SERVER_SIGNAL *ps)
{
    uint64_t PosBit;
    uint64_t Help64;
    unsigned char Help8;

    Value &= ps->mask;
    if (ps->byteorder == MSB_FIRST_BYTEORDER) {  // 1 -> MSB_FISRT
        int32_t PosByte;   // This must be signed! because the value can be negative
        PosBit = ps->startbit & 0x7;
        PosByte = po->size - 8 - (ps->startbit >> 3);
        Help64 = Value << PosBit;
        Help64 = _byteswap_uint64 (Help64);
        *(uint64_t *)(void*)(po->Data + PosByte) |= Help64;
        Help8 = (Value >> 56) >> (8 - PosBit);
        *(unsigned char *)(po->Data + PosByte - 1) |= Help8;
    } else {  // 0 -> LSB first
        uint32_t PosByte;
        PosBit = ps->startbit & 0x7;
        PosByte = (uint32_t)(ps->startbit >> 3);
        Help64 = (uint64_t)Value << PosBit;
        *(uint64_t *)(void*)(po->Data + PosByte) |= Help64;
        Help8 = (Value >> 56) >> (8 - PosBit);
        *(unsigned char *)(po->Data + PosByte + 8) |= Help8;
    }
}


static __inline int32_t GetMuxValue (NEW_CAN_SERVER_OBJECT *po,
                                      NEW_CAN_SERVER_SIGNAL *ps)
{
    uint32_t PosByte;
    uint32_t PosBit;
    uint32_t Ret;

    if (ps->byteorder == MSB_FIRST_BYTEORDER) {  // 1 -> MSB_FISRT
        int32_t PosByte_m4;
        uint32_t IStartBit = (uint32_t)((po->size << 3) - ps->mux_startbit);
        PosBit = IStartBit & 0x7;
        PosByte = IStartBit >> 3;
        PosByte_m4 = (int32_t)PosByte - 4;
        Ret = _byteswap_ulong (*(uint32_t *)(void*)(po->Data + PosByte_m4)) << PosBit;
        Ret |= (uint32_t)po->Data[PosByte] >> (8 - PosBit);
    } else {  // 0 -> LSB first
        PosBit = ps->mux_startbit & 0x7;
        PosByte = (uint32_t)(ps->mux_startbit >> 3);
        Ret = *(uint32_t *)(void*)(po->Data + PosByte) >> PosBit;
        Ret |= ((uint32_t)po->Data[PosByte + 4] << (8 - PosBit)) << 24;
   }
    return (int32_t)(Ret &= ps->mux_mask);
}

// Remark Data and OldData arrays must be +-3 Bytes larger as needed!
static __inline void SetMuxValue (NEW_CAN_SERVER_OBJECT *po,
                                  NEW_CAN_SERVER_SIGNAL *ps)
{
    uint32_t PosBit;
    uint32_t Value;
    uint32_t Help32;
    unsigned char Help8;

    Value = (uint32_t)ps->mux_value & ps->mux_mask;
    if (ps->byteorder == MSB_FIRST_BYTEORDER) {  // 1 -> MSB_FISRT
        int32_t PosByte;
        PosBit = ps->mux_startbit & 0x7;
        PosByte = po->size - 4 - (ps->mux_startbit >> 3);
        Help32 = Value << PosBit;
        Help32 = _byteswap_ulong (Help32);
        *(uint32_t *)(void*)(po->Data + PosByte) |= Help32;
        Help8 = (Value >> 24) >> (8 - PosBit);
        PosByte--;
        *(unsigned char *)(po->Data + PosByte) |= Help8;
    } else {  // 0 -> LSB first
        uint32_t PosByte;
        PosBit = ps->mux_startbit & 0x7;
        PosByte = (uint32_t)(ps->mux_startbit >> 3);
        Help32 = (uint32_t)Value << PosBit;
        *(uint32_t *)(void*)(po->Data + PosByte) |= Help32;
        Help8 = (Value >> 24) >> (8 - PosBit);
        *(unsigned char *)(po->Data + PosByte + 4) |= Help8;
    }
}


static __inline uint32_t GetMuxObjValue (NEW_CAN_SERVER_OBJECT *po, unsigned char* Data)
{
    uint32_t PosByte;
    uint32_t PosBit;
    uint32_t Ret;

    if (po->byteorder == MSB_FIRST_BYTEORDER) {  // 1 -> MSB_FISRT
        int32_t PosByte_m4;
        uint32_t IStartBit = (uint32_t)((po->size << 3) - po->Protocol.Mux.BitPos);
        PosBit = IStartBit & 0x7;
        PosByte = IStartBit >> 3;
        PosByte_m4 = (int32_t)(PosByte - 4);
        Ret = _byteswap_ulong (*(uint32_t *)(void*)(Data + PosByte_m4)) << PosBit;
        Ret |= (uint32_t)Data[PosByte] >> (8 - PosBit);
    } else {  // 0 -> LSB first
        PosBit = po->Protocol.Mux.BitPos & 0x7;
        PosByte = (uint32_t)(po->Protocol.Mux.BitPos >> 3);
        Ret = *(uint32_t*)(void*)(Data + PosByte) >> PosBit;
        Ret |= ((uint32_t)Data[PosByte + 4] << (8 - PosBit)) << 24;
   }
    return Ret &= po->Protocol.Mux.Mask;
}


static __inline void SetMuxObjValue (NEW_CAN_SERVER_OBJECT *po)
{
    uint32_t PosBit;
    uint32_t Value;
    uint32_t Help32;
    unsigned char Help8;

    Value = (uint32_t)po->Protocol.Mux.Value & po->Protocol.Mux.Mask;
    if (po->byteorder == MSB_FIRST_BYTEORDER) {  // 1 -> MSB_FISRT
        int32_t PosByte;
        PosBit = po->Protocol.Mux.BitPos & 0x7;
        PosByte = po->size - 4 - (po->Protocol.Mux.BitPos >> 3);
        Help32 = Value << PosBit;
        Help32 = _byteswap_ulong (Help32);
        *(uint32_t *)(void*)(po->Data + PosByte) |= Help32;
        Help8 = (Value >> 24) >> (8 - PosBit);
        PosByte--;
        *(unsigned char *)(po->Data + PosByte) |= Help8;
    } else {  // 0 -> LSB first
        uint32_t PosByte;
        PosBit = po->Protocol.Mux.BitPos & 0x7;
        PosByte = (uint32_t)(po->Protocol.Mux.BitPos >> 3);
        Help32 = (uint32_t)Value << PosBit;
        *(uint32_t *)(void*)(po->Data + PosByte) |= Help32;
        Help8 = (Value >> 24) >> (8 - PosBit);
        *(unsigned char *)(po->Data + PosByte + 4) |= Help8;
    }
}

static int CheckCanFdPossibleMessageSize(int par_Size)
{
    if (par_Size <= 8) {
        return 8; //par_Size;  always transmit 8 bytes
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

static int PackMulti_C_PCs(NEW_CAN_SERVER_CONFIG *csc, int o_pos)
{
    int Ret = 0;
    int Pos = 0;
    union {
        uint32_t udw;
        uint8_t ub[4];
    } C_PG_Header;
    int x, Count;
    uint32_t *Ids = GET_CAN_J1939_MULTI_C_PG_OBJECT(csc, o_pos);
    Count = Ids[0] + 1;

    for (x = 1; x < Count; x++) { // starts with 1 because first entry is the size
        uint32_t o_p = Ids[x];
        if (o_p != (uint32_t)-1) {
            if (csc->objects[o_p].flag == 1) { // there existing a new C_PG that should be transmitted
                // check if fits into the multi C_PG
                int NeededSpace = csc->objects[o_p].size + sizeof(C_PG_Header);
                if (csc->objects[o_pos].max_size >= (Pos + NeededSpace)) {
                    // build C_PG header
                    C_PG_Header.ub[0] = csc->objects[o_p].id >> 24; // TOS + Service header
                    C_PG_Header.ub[0] &= 0x3;
                    C_PG_Header.ub[0] += 0x40; //TOS=010b TF Value=000b
                    C_PG_Header.ub[1] = csc->objects[o_p].id >> 16; // TOS + Service header
                    C_PG_Header.ub[2] = csc->objects[o_p].id >> 8;  // TOS + Service header
                    C_PG_Header.ub[3] = (uint8_t)csc->objects[o_p].size;     // Payload length
                    *(uint32_t*)(csc->objects[o_pos].Data + Pos) = C_PG_Header.udw;
                    MEMCPY(csc->objects[o_pos].Data + Pos + sizeof(C_PG_Header), csc->objects[o_p].Data, csc->objects[o_p].size);
                    csc->objects[o_p].flag = 0;
                    Pos += NeededSpace;
                    Ret++;
                } else {
                    // no break maybe a smaller one will be following
                }
            }
        }
    }
    csc->objects[o_pos].size = CheckCanFdPossibleMessageSize(Pos);
    // Add a padding C_PG
    for (x = Pos; x < csc->objects[o_pos].size; x++) {
        if (x < (Pos + 3)) csc->objects[o_pos].Data[x] = 0;
        else csc->objects[o_pos].Data[x] = 0xAA;
    }

    return Ret;
}

static int Blackboard2Object (NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos)
{
    int s, m_t_sav, s_eqv;
    NEW_CAN_SERVER_OBJECT *po;
    NEW_CAN_SERVER_SIGNAL *ps, *ps_eqv;
    union FloatOrInt64 value;
    int type;
    uint64_t iv;
    int Ret = 0;
    
    po = &(csc->objects[o_pos]);
    if (po->EquationBeforeIdx) CalcEquationArray (po->EquationBeforeIdx, po);
    MEMCPY (po->Data, GET_CAN_INIT_DATA_OBJECT (csc, o_pos), (size_t)po->size);
    if (po->type == J1939_22_MULTI_C_PG) {
        if (PackMulti_C_PCs(csc, o_pos) == 0) {
            return -1;  // do not sent multi_PG message because no PG included!
        }
    }
    if (po->type == MUX_OBJECT) {
        SetMuxObjValue (po);
    }
    for (s = 0; s < po->signal_count; s++) {
        ps = &(csc->bb_signals[po->signals_ptr[s]]);
        switch (ps->type) {        // 0 normal
        case NORMAL_SIGNAL:
            if (((ps->startbit + ps->bitsize) >> 3) <= po->size) {
                type = read_bbvari_convert_to_FloatAndInt64(ps->vid, &value);
                ConvertSignal (csc, o_pos, po->signals_ptr[s], &value, &type);
                if (ps->float_type) {
                    iv = ConvMinMaxCheckFloat (value, type);
                } else {
                    iv = ConvMinMaxCheckSign (value, type, ps->bitsize, ps->sign);
                }
                SetValue (iv, po, ps);
            }
            break;
        case MUX_SIGNAL:
            m_t_sav = ps->mux_token;
            /*ThrowError (1, "MUX-Signal %s %i %i", ps->name, po->signals[s], m_t_sav);*/
            ps->mux_token = csc->bb_signals[m_t_sav].mux_equ_sb_childs;
            if (ps->mux_token == -1) {
                ps->mux_token = po->signals_ptr[s];
            }
            ps = &(csc->bb_signals[m_t_sav]);
            if (((ps->startbit + ps->bitsize) >> 3) <= po->size) {
                type = read_bbvari_convert_to_FloatAndInt64(ps->vid, &value);
                ConvertSignal (csc, o_pos, m_t_sav, &value, &type);
                if (ps->float_type) {
                    iv = ConvMinMaxCheckFloat (value, type);
                } else {
                    iv = ConvMinMaxCheckSign (value, type, ps->bitsize, ps->sign);
                }
                SetValue (iv, po, ps);
            }
            // Exists any additional signals with same MUX value?
            for (s_eqv = ps->mux_equ_v_childs; s_eqv >= 0; s_eqv = csc->bb_signals[s_eqv].mux_equ_v_childs) {
                ps_eqv = &(csc->bb_signals[s_eqv]);
                if (((ps_eqv->startbit + ps_eqv->bitsize) >> 3) <= po->size) {
                    type = read_bbvari_convert_to_FloatAndInt64(ps->vid, &value);
                    ConvertSignal (csc, o_pos, s_eqv, &value, &type);
                    if (ps->float_type) {
                        iv = ConvMinMaxCheckFloat (value, type);
                    } else {
                        iv = ConvMinMaxCheckSign (value, type, ps_eqv->bitsize, ps_eqv->sign);
                    }
                    SetValue (iv, po, ps_eqv);
                }
            }
            // Insert MUX value inside the CAN object
            if (((ps->mux_startbit + ps->mux_bitsize) >> 3) <= po->size) {
                SetMuxValue (po, ps);
            }
            break;
        case MUX_BY_SIGNAL:
            if (ps->mux_startbit > 0) {
                double d = read_bbvari_convert_double(ps->mux_startbit);
                if ((int)d == ps->mux_value) {
                    if (((ps->startbit + ps->bitsize) >> 3) <= po->size) {
                        type = read_bbvari_convert_to_FloatAndInt64(ps->vid, &value);
                        ConvertSignal (csc, o_pos, po->signals_ptr[s], &value, &type);
                        if (ps->float_type) {
                            iv = ConvMinMaxCheckFloat (value, type);
                        } else {
                            iv = ConvMinMaxCheckSign (value, type, ps->bitsize, ps->sign);
                        }
                        SetValue (iv, po, ps);
                    }
                }
            }
            break;
        }
    }
    if (po->EquationBehindIdx) CalcEquationArray (po->EquationBehindIdx, po);

    if (CanBitError.Command) {   // Simulate an bit error inside a CAN object.
        if ((CanBitError.Channel == channel) &&
            ((uint32_t)CanBitError.Id == po->id)) {
            int Size;
            switch (CanBitError.Command) {
            case OVERWRITE_DATA_BYTES:
                Size = po->size;
                if (Size > CAN_BIT_ERROR_MAX_SIZE) Size = CAN_BIT_ERROR_MAX_SIZE;
                for (int x = 0; x < Size; x++) {
                    po->Data[x] &= CanBitError.AndMask[x];
                    po->Data[x] |= CanBitError.OrMask[x];
                }
                if (CanBitError.ByteOrder) {
                    for (int x = 0; x < Size; x++) {
                        int xx = (Size - 1) - x;
                        uint8_t Data =  po->Data[x];
                        Data &= CanBitError.AndMask[xx];
                        Data |= CanBitError.OrMask[xx];
                        po->Data[x] = Data;
                    }
                } else {
                    for (int x = 0; x < Size; x++) {
                        uint8_t Data =  po->Data[x];
                        Data &= CanBitError.AndMask[x];
                        Data |= CanBitError.OrMask[x];
                        po->Data[x] = Data;
                    }
                }
                break;
            case CHANGE_DATA_LENGTH:
                if (CanBitError.SizeSave == -1) {
                    CanBitError.SizeSave = po->size;
                    CanBitError.LastCANObjectPos = o_pos;
                }
                po->size = (int16_t)CanBitError.Size;
                CanBitError.LastCANObjectPos = o_pos;
                break;
            case SUSPEND_TRANSMITION:
                Ret = 1;    // This Object should not be transmitted
                break;
            default:
                break;
            }
        }
    }
    return Ret;
}


static void Object2Blackboard (NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos, int Size)
{
    UNUSED(channel);
    int s, s_pos, s_equ_sb, s_equ_v;
    NEW_CAN_SERVER_OBJECT *po;
    NEW_CAN_SERVER_SIGNAL *ps;
    union FloatOrInt64 value;
    int type;
    int mux_value;
    uint64_t iv;
    
    po = &(csc->objects[o_pos]);
    if (po->EquationBeforeIdx) CalcEquationArray (po->EquationBeforeIdx, po);
    for (s = 0; s < po->signal_count; s++) {
        s_pos = po->signals_ptr[s];
        ps = &(csc->bb_signals[s_pos]);
        switch (ps->type) {        // 0 normal
        case NORMAL_SIGNAL:
            if (((ps->startbit + ps->bitsize) >> 3) <= Size) {
                iv = GetValue (po, ps);
                if (ps->float_type) {
                    value.d = ConvWithFloat ((uint32_t)iv);
                    type = FLOAT_OR_INT_64_TYPE_F64;
                } else {
                    type = ConvWithSign (iv, ps->bitsize, ps->sign, &value);
                }
                ConvertSignal (csc, o_pos, po->signals_ptr[s], &value, &type);
                write_bbvari_convert_with_FloatAndInt64 (ps->vid, value, type);
            }
            break;
        case MUX_SIGNAL:
            if (((ps->mux_startbit + ps->mux_bitsize) >> 3) <= Size) {
                mux_value = GetMuxValue (po, ps);
                for (s_equ_sb = s_pos; s_equ_sb >= 0; s_equ_sb = csc->bb_signals[s_equ_sb].mux_equ_sb_childs) {
                    if (csc->bb_signals[s_equ_sb].mux_value == mux_value) {
                        for (s_equ_v = s_equ_sb; s_equ_v >= 0; s_equ_v = csc->bb_signals[s_equ_v].mux_equ_v_childs) {
                            ps = &(csc->bb_signals[s_equ_v]);
                            if (((ps->startbit + ps->bitsize) >> 3) <= Size) {
                                iv = GetValue (po, ps);
                                if (ps->float_type) {
                                    value.d = ConvWithFloat ((uint32_t)iv);
                                    type = FLOAT_OR_INT_64_TYPE_F64;
                                } else {
                                    type = ConvWithSign (iv, ps->bitsize, ps->sign, &value);
                                }
                                ConvertSignal (csc, o_pos, s_equ_v, &value, &type);
                                write_bbvari_convert_with_FloatAndInt64 (ps->vid, value, type);
                            }
                        }
                        break;
                    }
                }
            }
            break;
        case MUX_BY_SIGNAL:
            if (ps->mux_startbit > 0) {
                double d = read_bbvari_convert_double(ps->mux_startbit);
                if ((int)d == ps->mux_value) {
                    if (((ps->startbit + ps->bitsize) >> 3) <= Size) {
                        iv = GetValue (po, ps);
                        if (ps->float_type) {
                            value.d = ConvWithFloat ((uint32_t)iv);
                            type = FLOAT_OR_INT_64_TYPE_F64;
                        } else {
                            type = ConvWithSign (iv, ps->bitsize, ps->sign, &value);
                        }
                        ConvertSignal (csc, o_pos, po->signals_ptr[s], &value, &type);
                        write_bbvari_convert_with_FloatAndInt64 (ps->vid, value, type);
                    }
                }
            }
            break;
        }
    }
    if (po->EquationBehindIdx) CalcEquationArray (po->EquationBehindIdx, po);
}

static int UnPackMulti_C_PCs(NEW_CAN_SERVER_CONFIG *csc, int channel, int o_pos)
{
    int Pos = 0;
    int x, Count;
    uint8_t *p;
    uint32_t *Ids = GET_CAN_J1939_MULTI_C_PG_OBJECT(csc, o_pos);
    Count = Ids[0] + 1;

    p = csc->objects[o_pos].Data;
    while((Pos < csc->objects[o_pos].size) &&
          ((p[Pos] >> 5) > 0)) {   // TOS == 0 padding
        uint32_t TosAndServiceHeaader = (p[Pos + 0] & 0x03) << 24 | // TOS + Service header
                                        p[Pos + 1] << 16 |
                                        p[Pos + 2] << 8;
        int C_PG_Size = p[Pos + 3];
        for (x = 1; x < Count; x++) { // starts with 1 because first entry is the size
            uint32_t o_p = Ids[x];
            if (o_p != (uint32_t)-1) {
                NEW_CAN_SERVER_OBJECT *po = &(csc->objects[o_p]);
                // compare C_PG header
                if ((po->id & 0x03FFFF00UL) == TosAndServiceHeaader) {
                    uint32_t cc;
                    if (C_PG_Size > po->size) C_PG_Size = po->size;
                    MEMCPY(po->Data, csc->objects[o_pos].Data + Pos + sizeof(TosAndServiceHeaader), C_PG_Size);
                    po->flag = 1;
                    cc = read_bbvari_udword (po->vid); // increment the receive counter
                    cc++;
                    write_bbvari_udword (po->vid, cc);
                    Object2Blackboard (csc, channel, o_p, C_PG_Size);
                    break;  // for(;;)
                }
            }
        }
        Pos += C_PG_Size + sizeof(TosAndServiceHeaader);
    }

    return Pos;
}

#ifdef REMOTE_MASTER
static void AttachAdditionalVariabales (int Idx)
{
    int *pIdx = (int*)(void*)&CanServerConfig->Symbols[Idx];
    do {
        attach_bbvari (*pIdx);
        pIdx++;
    } while (*pIdx != 0);
}

static void AttachEquationArray (int Idx)
{
    int *pIdx = (int*)(void*)&CanServerConfig->Symbols[Idx];
    do {
        attach_exec_stack ((struct EXEC_STACK_ELEM*)&CanServerConfig->Symbols[*pIdx]);
        pIdx++;
    } while (*pIdx != 0);
}

static void DetachAdditionalVariabales (int Idx)
{
    int *pIdx = (int*)(void*)&CanServerConfig->Symbols[Idx];
    do {
        remove_bbvari (*pIdx);
        pIdx++;
    } while (*pIdx != 0);
}

static void DetachEquationArray (int Idx)
{
    int *pIdx = (int*)(void*)&CanServerConfig->Symbols[Idx];
    do {
        detach_exec_stack ((struct EXEC_STACK_ELEM*)&CanServerConfig->Symbols[*pIdx]);
        pIdx++;
    } while (*pIdx != 0);
}
#endif

static int add_additional_bbvaris (NEW_CAN_SERVER_CONFIG *csc)
{
    int c, o, o_pos;
    char CanObjectName[BBVARI_NAME_SIZE];
    char Name[BBVARI_NAME_SIZE];

    for (c = 0; c < csc->channel_count; c++) {
        sprintf (CanObjectName, "%s.CAN%i", GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_CAN_NAMES), c);
        switch(csc->channels[c].StartupState) {
        case 0:
            csc->channels[c].global_tx_enable_vid = add_bbvari (CanObjectName, BB_UDWORD, "[]");
            write_bbvari_udword (csc->channels[c].global_tx_enable_vid, 0);
            break;
        default:
        case 1:
            csc->channels[c].global_tx_enable_vid = add_bbvari (CanObjectName, BB_UDWORD, "[]");
            write_bbvari_udword (csc->channels[c].global_tx_enable_vid, 1);
            break;
        case 2:
        case 3:
            csc->channels[c].global_tx_enable_vid = add_bbvari (CanObjectName, BB_UDWORD, "[]");
            if (get_bbvari_attachcount(csc->channels[c].global_tx_enable_vid) == 1) {
                write_bbvari_udword (csc->channels[c].global_tx_enable_vid, csc->channels[c].StartupState - 2);
            }
            break;
        }
        for (o = 0; o < csc->channels[c].object_count; o++) {
            o_pos = csc->channels[c].objects[o];
            if (csc->objects[o_pos].dir == WRITE_VARIABLE_ID_OBJECT) write_bbvari_udword (csc->objects[o_pos].vid, csc->objects[o_pos].id);
            else if (csc->objects[o_pos].dir == READ_OBJECT) write_bbvari_udword (csc->objects[o_pos].vid, 0);
            else write_bbvari_udword (csc->objects[o_pos].vid, 1);
        }
    }
    return 0;
}

#ifdef REMOTE_MASTER
static void attach_one_bbvari (NEW_CAN_SERVER_CONFIG *csc, int c, int o_pos, int s_pos)
{
    int ErrCode;
    if ((ErrCode = attach_bbvari (csc->bb_signals[s_pos].vid)) < 0) {
        ThrowError(1, "cannot attach variable \"%s\" error code %i  (channel %i, object id 0x%X)",
                GET_CAN_SIG_NAME (csc, s_pos), ErrCode, c, csc->objects[o_pos].id);
    } else {
        if (!(csc->dont_use_init_values_for_existing_variables && (ErrCode > 2)) &&
            csc->bb_signals[s_pos].start_value_flag) {
            write_bbvari_convert_with_FloatAndInt64 (csc->bb_signals[s_pos].vid, csc->bb_signals[s_pos].start_value, csc->bb_signals[s_pos].start_value_type);
        }
    }
}

static void attach_one_bbvari_equation (NEW_CAN_SERVER_CONFIG *csc, int c, int o_pos, int s_pos)
{
    UNUSED(csc);
    UNUSED(c);
    UNUSED(o_pos);
    if (csc->bb_signals[s_pos].ConvType == CAN_CONV_EQU) {
        attach_exec_stack ((struct EXEC_STACK_ELEM*)GET_CAN_SIG_BYTECODE(csc, s_pos));
    }
}
#endif

#if defined BUILD_WITH_J1939_SUPPORT
static int config_all_j1939_messages (NEW_CAN_SERVER_CONFIG *csc)
{
    int c, o, o_pos;

    for (c = 0; c < csc->channel_count; c++) {
        for (o = 0; o < csc->channels[c].object_count; o++) {
            o_pos = csc->channels[c].objects[o];
            if (csc->objects[o_pos].type == J1939_OBJECT) {
                if (csc->channels[c].j1939_flag) {
                    //ThrowError (1, "J1939ConfigMessage ()");
#ifdef BUILD_WITH_J1939_SUPPORT
                    if (csc->channels[c].j1939_rx_object_count < J1939TP_MP_MAX_OBJECTS) {
                        csc->channels[c].j1939_rx_objects[csc->channels[c].j1939_rx_object_count] = o_pos;
                        csc->channels[c].j1939_rx_object_count++;
                    } else {
                        ThrowError (1, "not more than %i CAN j1939 objects allowed on channel %i\n"
                               "Id = 0x%X will be ignored", J1939TP_MP_MAX_OBJECTS, c, csc->objects[o_pos].id);
                    }
#endif
                }
            }
        }
    }
    return 0;
}
#endif

#ifdef REMOTE_MASTER
static int attach_all_bbvaris_and_equations (NEW_CAN_SERVER_CONFIG *csc)
{
    int c, o, s, o_pos, s_pos, s_equ_sb, s_equ_v;

    for (c = 0; c < csc->channel_count; c++) {

        for (o = 0; o < csc->channels[c].object_count; o++) {
            o_pos = csc->channels[c].objects[o];
            attach_bbvari (csc->objects[o_pos].vid);
#if defined BUILD_WITH_J1939_SUPPORT
            if (csc->objects[o_pos].type == J1939_OBJECT) {
                if (!csc->channels[c].j1939_flag) {
                    ThrowError(1, "the global J1939 flag is not set for that reason there are no J1939 objects allowed (will be ignored)");
                } else {
                    if (csc->objects[o_pos].Protocol.J1939.vid_dlc > 0) {
                        attach_bbvari (csc->objects[o_pos].Protocol.J1939.vid_dlc);
                        write_bbvari_uword (csc->objects[o_pos].Protocol.J1939.vid_dlc, (uint16_t)csc->objects[o_pos].size);
                    }
                    if (csc->objects[o_pos].Protocol.J1939.dst_addr_vid > 0) {
                        attach_bbvari (csc->objects[o_pos].Protocol.J1939.dst_addr_vid);
                        write_bbvari_ubyte (csc->objects[o_pos].Protocol.J1939.dst_addr_vid, csc->objects[o_pos].Protocol.J1939.dst_addr);
                    }
                }
            }
#endif
            for (s = 0; s < csc->objects[o_pos].signal_count; s++) {
                s_pos = csc->objects[o_pos].signals_ptr[s];
                if (csc->bb_signals[s_pos].type == MUX_SIGNAL) {
                    // weitere Signale mit gleichem MUX-Startbit vorhanden?
                    for (s_equ_sb = s_pos; s_equ_sb >= 0; s_equ_sb = csc->bb_signals[s_equ_sb].mux_equ_sb_childs) {
                        // weitere Signale mit gleichem MUX-Value vorhanden?
                        for (s_equ_v = s_equ_sb; s_equ_v >= 0; s_equ_v = csc->bb_signals[s_equ_v].mux_equ_v_childs) {
                            attach_one_bbvari (csc, c, o_pos, s_equ_v);
                            attach_one_bbvari_equation (csc, c, o_pos, s_equ_v);
                        }
                    }
                } else {
                    // normales Signal
                    attach_one_bbvari (csc, c, o_pos, s_pos);
                    attach_one_bbvari_equation (csc, c, o_pos, s_pos);
                    if (csc->bb_signals[s_pos].type == MUX_BY_SIGNAL) {
                        if (csc->bb_signals[s_pos].mux_startbit > 0) {
                            int ErrCode;
                            if ((ErrCode = attach_bbvari_unknown_wait (csc->bb_signals[s_pos].mux_startbit)) < 0) {
                                ThrowError(1, "cannot attach multiplexer signal variable error code %i  (channel %i, object id 0x%X)",
                                       ErrCode, c, csc->objects[o_pos].id);
                            }
                        }
                    }
                }
            }

            if (csc->objects[o_pos].EventOrCyclic >= 1) {
                attach_exec_stack ((struct EXEC_STACK_ELEM*)GET_CAN_EVENT_BYTECODE(csc, o_pos));
            }
            if (csc->objects[o_pos].cycle_is_execst) {
                attach_exec_stack ((struct EXEC_STACK_ELEM*)GET_CAN_CYCLES_BYTECODE(csc, o_pos));
            }
            if (csc->objects[o_pos].AdditionalVariablesIdx) {
                AttachAdditionalVariabales (csc->objects[o_pos].AdditionalVariablesIdx);
            }
            if (csc->objects[o_pos].EquationBeforeIdx) {
                AttachEquationArray (csc->objects[o_pos].EquationBeforeIdx);
            }
            if (csc->objects[o_pos].EquationBehindIdx) {
                AttachEquationArray (csc->objects[o_pos].EquationBehindIdx);
            }
        }
    }
    return 0;
}
#endif

static int alloc_all_data_blocks (NEW_CAN_SERVER_CONFIG *csc)
{
    int c, o, o_pos;
    int block_size;
    int pos;

    // This blocks must be 16 bytes larger as needed
    // because the access function can be oversep (max +-7 bytes)
    block_size = 16;
    // First count the needed bytes
    for (c = 0; c < csc->channel_count; c++) {
        for (o = 0; o < csc->channels[c].object_count; o++) {
            o_pos = csc->channels[c].objects[o];
            // Each object have 2 daten blocks. T
            block_size += csc->objects[o_pos].size * 2; 
        }
    }
    // Than alloc one block for all datas
    csc->PtrDataBuffer = my_malloc (block_size);
    if (csc->PtrDataBuffer == NULL) {
        ThrowError (1, "cannot alloc memory for the CAN objects (%i bytes)", block_size);
        return -1;
    }
    pos = 8;
    for (c = 0; c < csc->channel_count; c++) {
        for (o = 0; o < csc->channels[c].object_count; o++) {
            o_pos = csc->channels[c].objects[o];
            csc->objects[o_pos].DataPtr =            
            csc->objects[o_pos].Data = (uint8_t*)(csc->PtrDataBuffer + pos);
            pos += csc->objects[o_pos].size; 
            csc->objects[o_pos].OldData = (uint8_t*)(csc->PtrDataBuffer + pos);
            pos += csc->objects[o_pos].size; 
            // Recalculate signal array addresses
            csc->objects[o_pos].signals_ptr = (short*)(void*)&(csc->Symbols[csc->objects[o_pos].SignalsArrayIdx]);
        }
    }
    return 0;
}

static int free_all_data_blocks (NEW_CAN_SERVER_CONFIG *csc)
{
    if (csc->PtrDataBuffer != NULL) {
        my_free (csc->PtrDataBuffer);
        csc->PtrDataBuffer = NULL;
    }
    return 0;
}


static int remove_additional_bbvaris (NEW_CAN_SERVER_CONFIG *csc)
{
    int c, o, o_pos;

    //ThrowError (MESSAGE_ONLY, "remove_all_bbvaris");
    for (c = 0; c < csc->channel_count; c++) {
        remove_bbvari (csc->channels[c].global_tx_enable_vid);
        for (o = 0; o < csc->channels[c].object_count; o++) {
            o_pos = csc->channels[c].objects[o];
            remove_bbvari (csc->objects[o_pos].vid);
        }
    }
    return 0;
}

#ifdef REMOTE_MASTER
static int detach_all_bbvaris_and_equations (NEW_CAN_SERVER_CONFIG *csc)
{
    int c, o, s, o_pos, s_pos, s_equ_sb, s_equ_v;

    //ThrowError (MESSAGE_ONLY, "detach_all_equations");
    for (c = 0; c < csc->channel_count; c++) {
        for (o = 0; o < csc->channels[c].object_count; o++) {
            o_pos = csc->channels[c].objects[o];
            remove_bbvari (csc->objects[o_pos].vid);
#if defined BUILD_WITH_J1939_SUPPORT
            if (csc->objects[o_pos].type == J1939_OBJECT) {
                if (csc->objects[o_pos].Protocol.J1939.vid_dlc > 0) {
                    remove_bbvari (csc->objects[o_pos].Protocol.J1939.vid_dlc);
                }
                if (csc->objects[o_pos].Protocol.J1939.dst_addr_vid > 0) {
                    remove_bbvari (csc->objects[o_pos].Protocol.J1939.dst_addr_vid);
                }
            }
#endif
            if (csc->objects[o_pos].EventOrCyclic >= 1) {   // 1,2 -> Event in Byte-Code uebersetzt
                detach_exec_stack ((struct EXEC_STACK_ELEM*)GET_CAN_EVENT_BYTECODE(csc, o_pos));
            }
            if (csc->objects[o_pos].cycle_is_execst) {   // Cycles Byte-Code uebersetzt
                detach_exec_stack ((struct EXEC_STACK_ELEM*)GET_CAN_CYCLES_BYTECODE(csc, o_pos));
            }
            for (s = 0; s < csc->objects[o_pos].signal_count; s++) {
                s_pos = csc->objects[o_pos].signals_ptr[s];
                if (csc->bb_signals[s_pos].type == MUX_SIGNAL) {
                    // weitere Signale mit gleichem MUX-Startbit vorhanden?
                    for (s_equ_sb = s_pos; s_equ_sb >= 0; s_equ_sb = csc->bb_signals[s_equ_sb].mux_equ_sb_childs) {
                        remove_bbvari (csc->bb_signals[s_equ_sb].vid);
                        if (csc->bb_signals[s_equ_sb].ConvType == CAN_CONV_EQU) {
                            detach_exec_stack ((struct EXEC_STACK_ELEM*)GET_CAN_SIG_BYTECODE(csc, s_equ_sb));
                        }
                        // weitere Signale mit gleichem MUX-Value vorhanden?
                        for (s_equ_v = s_equ_sb /*csc->signals[s_equ_sb].mux_equ_v_childs*/; s_equ_v >= 0; s_equ_v = csc->bb_signals[s_equ_v].mux_equ_v_childs) {
                            remove_bbvari (csc->bb_signals[s_equ_v].vid);
                            if (csc->bb_signals[s_equ_v].ConvType == CAN_CONV_EQU) {
                                detach_exec_stack ((struct EXEC_STACK_ELEM*)GET_CAN_SIG_BYTECODE(csc, s_equ_v));
                            }
                        }
                    }
                } else {
                    remove_bbvari (csc->bb_signals[s_pos].vid);
                    // normales Signal
                    if (csc->bb_signals[s_pos].ConvType == CAN_CONV_EQU) {
                        detach_exec_stack ((struct EXEC_STACK_ELEM*)GET_CAN_SIG_BYTECODE(csc, s_pos));
                    }
                    if (csc->bb_signals[s_pos].type == MUX_BY_SIGNAL) {
                        if (csc->bb_signals[s_pos].mux_startbit > 0) {
                            remove_bbvari_unknown_wait (csc->bb_signals[s_pos].mux_startbit);
                        }
                    }
                }
            }
            if (csc->objects[o_pos].AdditionalVariablesIdx) {
                DetachAdditionalVariabales (csc->objects[o_pos].AdditionalVariablesIdx);
            }
            if (csc->objects[o_pos].EquationBeforeIdx) {
                DetachEquationArray (csc->objects[o_pos].EquationBeforeIdx);
            }
            if (csc->objects[o_pos].EquationBehindIdx) {
                DetachEquationArray (csc->objects[o_pos].EquationBehindIdx);
            }
        }
    }
    return 0;
}
#endif


static int ObjectShouldBeSend ( NEW_CAN_SERVER_OBJECT *po)
{
    return  (po->should_be_send > 0);
}

static void ObjectAreSended ( NEW_CAN_SERVER_OBJECT *po)
{
    /*if (po->should_be_send)*/ po->should_be_send--;
}

static void ObjectIncOnlyCounter (NEW_CAN_SERVER_CONFIG *csc, int o_pos)
{
    int cycles;
    NEW_CAN_SERVER_OBJECT *po = &csc->objects[o_pos];

    if (po->should_be_send < 2) {
        po->should_be_send += (po->delay == po->cycles_counter);
        po->cycles_counter++;
    }
    if (po->cycle_is_execst) {
        union FloatOrInt64 value;
        value.d = 0.0;
        int type = FLOAT_OR_INT_64_TYPE_F64;
        type = execute_stack_whith_can_parameter ((struct EXEC_STACK_ELEM *)GET_CAN_CYCLES_BYTECODE(csc, o_pos), value, type, po, &value);
        cycles = to_int_FloatOrInt64(value, type);
    } else {
        cycles = po->cycles;
    }
    if (po->cycles_counter >= cycles) {
        po->cycles_counter = 0;
    }
}


static void CanServerRequestInit (void)
{
#ifdef REMOTE_MASTER
    write_message(get_pid_by_name ("RemoteMasterControl"), NEW_CAN_INI, sizeof (can_init_flag), (char *)&can_init_flag);
    can_init_flag = CANSERVER_READ_INIT;
#else
    // only skip
    can_init_flag = CANSERVER_READ_INIT;
#endif
}


static void CanServerReadInit (void)
{
#ifdef REMOTE_MASTER
    MESSAGE_HEAD mhead;
	int FoundCANChannels;

    if (test_message (&mhead)) {
        //ThrowError (1, "CanServerReadInit() Message %i", mhead.mid);
        if (mhead.mid == NEW_CANSERVER_INIT) {
            if (CanServerConfig != NULL) my_free (CanServerConfig);
            CanServerConfig = (NEW_CAN_SERVER_CONFIG*)my_malloc (mhead.size);
            if (CanServerConfig != NULL) {
                read_message (&mhead, (char*)CanServerConfig, mhead.size);
                CanBitError.Command = 0;

                FoundCANChannels = GetNumberOfFoundCanChannelsSocketCAN();
                if (CanServerConfig->channel_count > FoundCANChannels) {
                    ThrowError(1, "more CAN channels configured (%i) than found (%i)", CanServerConfig->channel_count, FoundCANChannels);
                    CanServerConfig->channel_count = FoundCANChannels;
                }
                can_init_flag = CANSERVER_SELECT_CAN_CARD;
            } else {
                ThrowError(1, "cannot config CAN (out of memory)");
                remove_message ();
            }
        } else if (mhead.mid == NEW_CANSERVER_STOP) {
            remove_message ();
        } else if (mhead.mid == IS_NEW_CANSERVER_RUNNING) {
            remove_message ();
        } else {
            // alle anderen Messages werden ignoriert!
            // ohne Fehlermeldung!!!
            //ThrowError (1, "CanServerReadInit() CAN-Server got a unknown message %i", mhead.mid);
            remove_message ();
        }
    }
#else
    if (CanServerConfig != NULL) remove_canserver_config (CanServerConfig);
    CanServerConfig = ReadCanConfig (GetMainFileDescriptor());
    can_init_flag = CANSERVER_SELECT_CAN_CARD;
#endif
}

static void CheckCanFdAllowedOnChannel(int Channel)
{
    int o, o_pos;

    if (Channel < CanServerConfig->channel_count) {
        if (CanServerConfig->channels[Channel].can_fd_enabled) {
            if (!CanServerConfig->channels[Channel].fd_support) {
                ThrowError (1, "CAN channel %i have no CAN FD support disable it", Channel);
                CanServerConfig->channels[Channel].can_fd_enabled = 0;
            }
        }
        for (o = 0; o < CanServerConfig->channels[Channel].object_count; o++) {
            o_pos = CanServerConfig->channels[Channel].objects[o];
            if (!CanServerConfig->channels[Channel].can_fd_enabled) {
                if ((CanServerConfig->objects[o_pos].ext & CANFD_FDF_MASK) == CANFD_FDF_MASK) {
                    ThrowError (1, "CAN channel %i have no CAN FD support remove bit rate switch from object 0x%X", Channel, CanServerConfig->objects[o_pos].id);
                    CanServerConfig->objects[o_pos].ext &= ~(CANFD_FDF_MASK | CANFD_BTS_MASK);
                }
            }
            if (CanServerConfig->objects[o_pos].type == J1939_OBJECT) {
                // do nothing
            } else if ((CanServerConfig->objects[o_pos].type == J1939_22_C_PG) ||
                       (CanServerConfig->objects[o_pos].type == J1939_22_MULTI_C_PG)) {
                if (!CanServerConfig->channels[Channel].can_fd_enabled) {
                    ThrowError (1, "CAN channel %i is no CAN FD but object 0x%X is a (multi) C_PG", Channel, CanServerConfig->objects[o_pos].id);
                    if (CanServerConfig->objects[o_pos].size > 8) {
                        ThrowError (1, "On CAN channel %i object 0x%X truncate size from %i to 8 byte", Channel, CanServerConfig->objects[o_pos].id, (int)CanServerConfig->objects[o_pos].size);
                        CanServerConfig->objects[o_pos].size = 8;
                    }
                } else {
                    if (CanServerConfig->objects[o_pos].size > 64) {
                        ThrowError (1, "On CAN channel %i object 0x%X truncate size from %i to 64 byte", Channel, CanServerConfig->objects[o_pos].id, (int)CanServerConfig->objects[o_pos].size);
                        CanServerConfig->objects[o_pos].size = 64;
                    }
                }
            } else {
               if ((CanServerConfig->objects[o_pos].ext & CANFD_FDF_MASK) == CANFD_FDF_MASK) {
                    if (CanServerConfig->objects[o_pos].size > 64) {
                        ThrowError (1, "On CAN channel %i object 0x%X truncate size from %i to 64 byte", Channel, CanServerConfig->objects[o_pos].id, (int)CanServerConfig->objects[o_pos].size);
                        CanServerConfig->objects[o_pos].size = 64;
                    }
                } else {
                    if (CanServerConfig->objects[o_pos].size > 8) {
                        ThrowError (1, "On CAN channel %i object 0x%X truncate size from %i to 8 byte", Channel, CanServerConfig->objects[o_pos].id, (int)CanServerConfig->objects[o_pos].size);
                        CanServerConfig->objects[o_pos].size = 8;
                    }
                }
            }
        }
    }
}

static int CanServerSelectCanCard (void)
{
    int c;
    if (CanServerConfig == NULL) return 0;

#ifdef REMOTE_MASTER
    int NumOfChannelsOnSocketCan = GetNumberOfFoundCanChannelsSocketCAN();

    int OffsetOfChannelsOnSocketCan = 0;
    int StartChannelsOnSocketCan = OffsetOfChannelsOnSocketCan;
    int StopChannelsOnSocketCan = StartChannelsOnSocketCan + NumOfChannelsOnSocketCan;

#else
#if defined(_WIN32) && defined(BUILD_WITH_GATEWAY_VIRTUAL_CAN)
    if (CanServerConfig->EnableGatewayDeviceDriver) {
        InitCanGatewayDevice(CanServerConfig->EnableGatewayDeviceDriver);
    }
#endif
#endif
    for (c = 0; c < CanServerConfig->channel_count; c++) {
#ifdef REMOTE_MASTER
        if ((c >= StartChannelsOnSocketCan) && (c < StopChannelsOnSocketCan)) {
            CanServerConfig->channels[c].CardTypeNrChannel = GetCanSocketByChannelNumber(c - OffsetOfChannelsOnSocketCan);
            CanServerConfig->channels[c].read_can = SocketCAN_read_can;
            CanServerConfig->channels[c].write_can = SocketCAN_write_can;
            CanServerConfig->channels[c].open_can = SocketCAN_open_can;
            CanServerConfig->channels[c].close_can = SocketCAN_close_can;
            CanServerConfig->channels[c].status_can = SocketCAN_status_can;
            CanServerConfig->channels[c].queue_read_can = SocketCAN_ReadNextObjectFromQueue;
            CanServerConfig->channels[c].queue_write_can = SocketCAN_WriteObjectToQueue;
            CanServerConfig->channels[c].fd_support = 1;
        } else {
            ThrowError(1, "more CAN channels configured than found");
            return -1;
        }
#else
        CanServerConfig->channels[c].VirtualNetworkId = -1;
#if defined(_WIN32) && defined(BUILD_WITH_GATEWAY_VIRTUAL_CAN)
        if (c < GetCanGatewayDeviceCount()) {
            CanServerConfig->channels[c].read_can = CanGatewayDevice_read_can;
            CanServerConfig->channels[c].write_can = CanGatewayDevice_write_can;
            CanServerConfig->channels[c].open_can = CanGatewayDevice_open_can;
            CanServerConfig->channels[c].close_can = CanGatewayDevice_close_can;
            CanServerConfig->channels[c].status_can = CanGatewayDevice_status_can;
            CanServerConfig->channels[c].queue_read_can =CanGatewayDevice_ReadNextObjectFromQueue;
            CanServerConfig->channels[c].queue_write_can = CanGatewayDevice_WriteObjectToQueue;
            CanServerConfig->channels[c].fd_support = GetCanGatewayDeviceFdSupport(c);
        } else
#endif
        {
            CanServerConfig->channels[c].read_can = virtdev_read_can;
            CanServerConfig->channels[c].write_can = virtdev_write_can;
            CanServerConfig->channels[c].open_can = virtdev_open_can;
            CanServerConfig->channels[c].close_can = virtdev_close_can;
            CanServerConfig->channels[c].status_can = virtdev_status_can;
            CanServerConfig->channels[c].queue_read_can = virtdev_ReadNextObjectFromQueue;
            CanServerConfig->channels[c].queue_write_can = virtdev_WriteObjectToQueue;
            CanServerConfig->channels[c].fd_support = 1;
        }
#endif
        CheckCanFdAllowedOnChannel(c);
    }
    return 0;
}

static void CanServerOpenCan (void)
{
    int c;

    if (CanServerConfig == NULL) return;

    if (alloc_all_data_blocks (CanServerConfig)) {
        can_init_flag = CANSERVER_NOT_INIT;
    } else {
#ifdef BUILD_WITH_J1939_SUPPORT
        //ThrowError (1, "GetCanServerCycleTime_ms() = %i, get_sched_periode_timer_clocks() = %i", GetCanServerCycleTime_ms(), get_sched_periode_timer_clocks());
        NewJ1939Config (CanServerConfig, GetCanServerCycleTime_ms () * (get_sched_periode_timer_clocks() / 1000));
        config_all_j1939_messages (CanServerConfig);
#endif

#ifdef REMOTE_MASTER
        attach_all_bbvaris_and_equations (CanServerConfig);
#endif
        add_additional_bbvaris (CanServerConfig);
#ifdef REMOTE_MASTER
        write_message (get_pid_by_name ("RemoteMasterControl"), NEW_CAN_ATTACH_CFG_ACK, 0, NULL);
#endif

        for (c = 0; c < CanServerConfig->channel_count; c++) {
            if (CanServerConfig->channels[c].open_can (CanServerConfig, c)) {
                CanServerConfig->channels[c].enable = 0;
                ThrowError (1, "cannot open CAN channel %i", c);
                // can_init_flag = CANSERVER_READ_INIT;
                // break;
            } else {
                CanServerConfig->channels[c].enable = 1;
            }
        }
        can_init_flag = CANSERVER_CYCLIC;
    }
}


static __inline int BasicCANMessageFilter (uint32_t IdIn,
                                           NEW_CAN_HASH_ARRAY_ELEM *HashArray)
{
    int pos;
    int dx;
    uint32_t Id;

    pos = MAX_RX_OBJECTS_ONE_CHANNEL / 2;        // max 256 CAN objects. 128 are the centerof the object-buffer-array
    dx = pos >> 1;
    pos--;
#if (MAX_RX_OBJECTS_ONE_CHANNEL > 1024)
#error "MAX_RX_OBJECTS_ONE_CHANNEL must be smaller or equal 1024"
#endif
#if (MAX_RX_OBJECTS_ONE_CHANNEL > 512)
    Id = HashArray[pos].id;
    pos += ((IdIn <= Id) - 1) & dx;  // If IdIn is larger than Id in [pos] than do not increment
    pos -= ((IdIn >= Id) - 1) & dx;  // If IdIn is smaller than Id in [pos] than do not decrement
    dx = dx >> 1;   // 64
#endif
#if (MAX_RX_OBJECTS_ONE_CHANNEL > 256)
    Id = HashArray[pos].id;
    pos += ((IdIn <= Id) - 1) & dx;  // If IdIn is larger than Id in [pos] than do not increment
    pos -= ((IdIn >= Id) - 1) & dx;  // If IdIn is smaller than Id in [pos] than do not decrement
    dx = dx >> 1;   // 64
#endif
#if (MAX_RX_OBJECTS_ONE_CHANNEL > 128)
    Id = HashArray[pos].id;
    pos += ((IdIn <= Id) - 1) & dx;  // If IdIn is larger than Id in [pos] than do not increment
    pos -= ((IdIn >= Id) - 1) & dx;  // If IdIn is smaller than Id in [pos] than do not decrement
    dx = dx >> 1;   // 32
#endif
    Id = HashArray[pos].id;
    pos += ((IdIn <= Id) - 1) & dx;  // If IdIn is larger than Id in [pos] than do not increment
    pos -= ((IdIn >= Id) - 1) & dx;  // If IdIn is smaller than Id in [pos] than do not decrement
    dx = dx >> 1;   // 16
    Id = HashArray[pos].id;
    pos += ((IdIn <= Id) - 1) & dx;  // If IdIn is larger than Id in [pos] than do not increment
    pos -= ((IdIn >= Id) - 1) & dx;  // If IdIn is smaller than Id in [pos] than do not decrement
    dx = dx >> 1;   // 8
    Id = HashArray[pos].id;
    pos += ((IdIn <= Id) - 1) & dx;  // If IdIn is larger than Id in [pos] than do not increment
    pos -= ((IdIn >= Id) - 1) & dx;  // If IdIn is smaller than Id in [pos] than do not decrement
    dx = dx >> 1;   // 4
    Id = HashArray[pos].id;
    pos += ((IdIn <= Id) - 1) & dx;  // If IdIn is larger than Id in [pos] than do not increment
    pos -= ((IdIn >= Id) - 1) & dx;  // If IdIn is smaller than Id in [pos] than do not decrement
    dx = dx >> 1;   // 2
    Id = HashArray[pos].id;
    pos += ((IdIn <= Id) - 1) & dx;  // If IdIn is larger than Id in [pos] than do not increment
    pos -= ((IdIn >= Id) - 1) & dx;  // If IdIn is smaller than Id in [pos] than do not decrement
    dx = dx >> 1;   // 1
    Id = HashArray[pos].id;
    pos += ((IdIn <= Id) - 1) & dx;  // If IdIn is larger than Id in [pos] than do not increment
    pos -= ((IdIn >= Id) - 1) & dx;  // If IdIn is smaller than Id in [pos] than do not decrement
    Id = HashArray[pos].id;
    if (IdIn == Id) {
        return HashArray[pos].pos;
    } else {
        return -1;
    }
}


static void CanServerCyclic (void)
{
    int c, o, o_pos, s_o_pos;
    MESSAGE_HEAD mhead;
    uint32_t cc;
    uint64_t TimeStamp;
    uint32_t id;
    unsigned char data[64];
    unsigned char ext, size;
    NEW_CAN_SERVER_OBJECT *po, *pmo;
    int32_t mux_value;

    if (CanBitError.Command) {   // Simulate an bit error inside a CAN object.
        uint64_t Cycle = GetCycleCounter64();
        if (CycleAtStartPoint + CanBitError.Counter > Cycle) {
            write_bbvari_udword(CanBitErrorVid, (CycleAtStartPoint + CanBitError.Counter) - Cycle);
        } else {
            // Timeout reset the error condition
            if (CanBitError.Command == CHANGE_DATA_LENGTH) {
                if (CanServerConfig != NULL) {
                    if ((CanBitError.LastCANObjectPos >= 0) &&
                        (CanBitError.LastCANObjectPos < MAX_OBJECTS_ALL_CHANNELS)) { // Reset the size to the original value
                        CanServerConfig->objects[CanBitError.LastCANObjectPos].size = (int16_t)CanBitError.SizeSave;
                    }
                }
            }
            CanBitError.Command = 0;
            write_bbvari_udword(CanBitErrorVid, 0);
        }
    }

#ifdef REMOTE_MASTER
    // muss vor CcpMessageFilter Aufruf kommen!
    //InitObjectsOneCycleBuffer();
#endif

    if (test_message (&mhead)) {
        //ThrowError (1, "Message %i", mhead.mid);
        switch (mhead.mid) {
        case NEW_CANSERVER_STOP:
            remove_message ();
            can_init_flag = CANSERVER_STOP;
            //ThrowError (INFO_NO_STOP, "stop CAN server");
            return;
        case NEW_CANSERVER_INIT:
            can_init_flag = CANSERVER_STOP;
            // ThrowError (1, "new CAN Config");
            return;
#ifdef REMOTE_MASTER
        case GET_CAN_CARD_INFOS:
            remove_message ();
            SendCANCardInfosToProcess (mhead.pid);
            return;
#endif

        case IS_NEW_CANSERVER_RUNNING:        // Ask if CAN server is running request
            if (can_init_flag == CANSERVER_CYCLIC) write_message (mhead.pid, mhead.mid, 0, NULL);
            remove_message ();
            break;
        case NEW_CANSERVER_SET_BIT_ERR:
            // There should be insert CAN bit errors
            if (sizeof(CanBitError) != mhead.size) {
                ThrowError(1, "sizeof(CanBitError) != mhead.size (%i != %i)", (int)sizeof(CanBitError), mhead.size);
            } else {
                if (CanBitError.Command == CHANGE_DATA_LENGTH) {
                    if ((CanBitError.LastCANObjectPos >= 0) &&
                        (CanBitError.LastCANObjectPos < MAX_OBJECTS_ALL_CHANNELS)) { // Reset the size to the original value
                        if (CanServerConfig != NULL) {
                            CanServerConfig->objects[CanBitError.LastCANObjectPos].size = (int16_t)CanBitError.SizeSave;
                        }
                    }
                }
                read_message (&mhead, (char*)&CanBitError, mhead.size);
                CanBitError.LastCANObjectPos = -1;
                CycleAtStartPoint = GetCycleCounter64();
                write_bbvari_udword (CanBitErrorVid, CanBitError.Counter);
            }
            break;
        case NEW_CANSERVER_RESET_BIT_ERR:
            if (CanBitError.Command == CHANGE_DATA_LENGTH) {
                if ((CanBitError.LastCANObjectPos >= 0) &&
                    (CanBitError.LastCANObjectPos < MAX_OBJECTS_ALL_CHANNELS)) { // Reset the size to the original value
                    if (CanServerConfig != NULL) {
                        CanServerConfig->objects[CanBitError.LastCANObjectPos].size = (int16_t)CanBitError.SizeSave;
                    }
                }
            }
            CanBitError.Command = 0;
            CanBitError.Id = -1;
            CanBitError.Counter = 0;
            CanBitError.Channel = 0;
            for (int x = 0; x < CAN_BIT_ERROR_MAX_SIZE; x++) {
                CanBitError.AndMask[x] = 0;
                CanBitError.OrMask[x] = 0;
            }
            CanBitError.Size = -1;
            CanBitError.SizeSave = -1;
            write_bbvari_udword (CanBitErrorVid, 0);
            remove_message();
            break;
        case NEW_CANSERVER_SET_SIG_CONV:
            if (CanServerConfig != NULL) {
                AddOrRemoveReplaceCanSigConvMsg (CanServerConfig, &mhead);
            }
            break;

        default:
#ifdef INC_CCP
            if (CcpMessageFilter (&mhead, CanServerConfig)) break;
#endif
            remove_message ();
            ThrowError (1, "CAN-Server got a unknown message %i", mhead.mid);
        }
    }

    if (CanServerConfig != NULL)
    for (c = 0; c < CanServerConfig->channel_count; c++) {
        static int BusOffCounter[MAX_CAN_CHANNELS];
        
        if (!CanServerConfig->channels[c].enable) {
            continue;
        }
        
        // If bus off wait 100 cycles than restart the CAN channel
        if (BusOffCounter[c]) {
            BusOffCounter[c]--;
            if (!BusOffCounter[c]) CanServerConfig->channels[c].open_can (CanServerConfig, c);
            continue;
        }

        // First all receive objects
        while (CanServerConfig->channels[c].queue_read_can (CanServerConfig, c, &id, data, &ext, &size, &TimeStamp)) {
            WriteCanMessageFromBus2Fifos (c, id, data, ext, size, 0, TimeStamp);
            if  (size <= 8) {
#ifdef INC_CCP
                if (GlobalXcpCcpActiveFlag) CppCanMessageFilter (CanServerConfig, c, id, data, size);
#endif
#if defined BUILD_WITH_J1939_SUPPORT
                if (CanServerConfig->channels[c].j1939_flag) {
                    if (NewJ1939CanMessageFilter (CanServerConfig, c, id, data, size) == IS_J1939_SIGNLE_FRAME_MESSAGE_RET) {
                        // nothing todo all happens inside NewJ1939CanMessageFilter
                    }
                }
#endif
            }
            if ((o_pos = BasicCANMessageFilter (id, CanServerConfig->channels[c].hash_rx)) >= 0) {
                size_t size_checked;
                po = &CanServerConfig->objects[o_pos];
                cc = read_bbvari_udword (po->vid);
                cc++;
                write_bbvari_udword (po->vid, cc);
                if (size > po->size) size = (unsigned char)po->size;
                switch (po->type) {
                case J1939_22_MULTI_C_PG:
                    if (size > po->size) size_checked = (size_t)po->size;
                    else size_checked = size;
                    MEMCPY (po->Data, (void*)data, size_checked);
                    po->flag = 1;
                    UnPackMulti_C_PCs (CanServerConfig, c, o_pos);
                    break;

                case J1939_22_C_PG:
                    break;

                case NORMAL_OBJECT:    // Normal object
                    if (size > po->size) size_checked = (size_t)po->size;
                    else size_checked = size;
                    MEMCPY (po->Data, (void*)data, size_checked);
                    po->flag = 1;
                    /*ThrowError (1, "  Data %i 0x%X (0x%08X%08X)", status, CanServerConfig->channels[c].address,
                                     (unsigned int)(CanServerConfig->objects[o_pos].data >> 32),
                                     (unsigned int)(CanServerConfig->objects[o_pos].data));*/
                    Object2Blackboard (CanServerConfig, c, o_pos, po->size);
                    break;

                case MUX_OBJECT:     // Master MUX object
                    mux_value = (int32_t)GetMuxObjValue (po, data);
                    pmo = po;
                    do {
                        //ThrowError (1, "mux_value = %i, po->Protocol.Mux.value = %i", mux_value, po->Protocol.Mux.value);
                        if (mux_value == po->Protocol.Mux.Value) {
                            if (size > po->size) size_checked = (size_t)po->size;
                            else size_checked = size;
                            MEMCPY (po->Data, (void*)data, size_checked);
                            po->flag = 1;
                            Object2Blackboard (CanServerConfig, c, o_pos, po->size);
                            break;
                        }
                        o_pos = po->Protocol.Mux.next;
                        po = &CanServerConfig->objects[o_pos];
                    } while (pmo != po);
                    break;
                case J1939_OBJECT:
                    break;
                }
            }
        }

        // Than all transmit object
        if (read_bbvari_udword (CanServerConfig->channels[c].global_tx_enable_vid)) {
            //ThrowError (1, " objects %i", CanServerConfig->channels[c].object_count);
            for (o = 0; o < CanServerConfig->channels[c].tx_object_count; o++) {
                int id;
                o_pos = CanServerConfig->channels[c].hash_tx[o].pos;
                po = &CanServerConfig->objects[o_pos];
                id = read_bbvari_udword (po->vid);
                if (id > 0) {
                    if (po->dir == WRITE_VARIABLE_ID_OBJECT) {
                        po->id = id;
                    }
                    ObjectIncOnlyCounter(CanServerConfig, o_pos);
                    switch (po->type) {
                    case J1939_22_C_PG:
                    case J1939_22_MULTI_C_PG:
                    case NORMAL_OBJECT:   // No MUX object
                        if (!po->EventOrCyclic) {   // Cyclic
                            if (ObjectShouldBeSend (po)) {
                                if (Blackboard2Object (CanServerConfig, c, o_pos) == 0) {
                                    if (po->type != J1939_22_C_PG) {
                                        CanServerConfig->channels[c].write_can (CanServerConfig, c, o_pos);
                                        WriteCanMessageFromBus2Fifos (c, po->id, po->DataPtr, (unsigned char)po->ext, (unsigned char)po->size, 1, get_rt_cycle_counter());
                                    } else {
                                        po->flag = 1;
                                    }
                                }
                                ObjectAreSended(po);
                            }
                        } else if (po->EventOrCyclic == 1) {
                            union FloatOrInt64 value;
                            value.d = 0.0;
                            int type = FLOAT_OR_INT_64_TYPE_F64;
                            type = execute_stack_whith_can_parameter ((struct EXEC_STACK_ELEM *)GET_CAN_EVENT_BYTECODE(CanServerConfig, o_pos), value, type, po, &value);
                            if (to_bool_FloatOrInt64(value, type)) {
                                if (Blackboard2Object (CanServerConfig, c, o_pos) == 0) {
                                    if (po->type != J1939_22_C_PG) {
                                        CanServerConfig->channels[c].write_can (CanServerConfig, c, o_pos);
                                        WriteCanMessageFromBus2Fifos (c, po->id, po->DataPtr, (unsigned char)po->ext, (unsigned char)po->size, 1, get_rt_cycle_counter());
                                    } else {
                                        po->flag = 1;
                                    }
                                    ObjectAreSended(po);
                                }
                            }
                        } else {   // == 2:  Event uses CAN data
                            union FloatOrInt64 value;
                            value.d = 0.0;
                            int type = FLOAT_OR_INT_64_TYPE_F64;
                            if (Blackboard2Object (CanServerConfig, c, o_pos) == 0) {
                                type = execute_stack_whith_can_parameter ((struct EXEC_STACK_ELEM *)GET_CAN_EVENT_BYTECODE(CanServerConfig, o_pos), value, type, po, &value);
                                if (to_bool_FloatOrInt64(value, type)) {
                                    if (po->type != J1939_22_C_PG) {
                                        CanServerConfig->channels[c].write_can (CanServerConfig, c, o_pos);
                                        WriteCanMessageFromBus2Fifos (c, po->id, po->DataPtr, (unsigned char)po->ext, (unsigned char)po->size, 1, get_rt_cycle_counter());
                                    } else {
                                        po->flag = 1;
                                    }
                                    ObjectAreSended(po);
                                }
                                MEMCPY (po->OldData, po->Data, (size_t)po->size);
                            }
                        }
                        break;
                    case MUX_OBJECT:   // MUX object
                        if (po->Protocol.Mux.master == -1) {      // Master MUX object
                            s_o_pos = po->Protocol.Mux.token;
                            pmo = &CanServerConfig->objects[s_o_pos];
                            if (ObjectShouldBeSend (pmo)) {
                                if (Blackboard2Object (CanServerConfig, c, s_o_pos) == 0) {
                                    MEMCPY (po->Data, pmo->Data, (size_t)po->size);
                                    MEMCPY (po->OldData, po->Data, (size_t)po->size);
                                    CanServerConfig->channels[c].write_can (CanServerConfig, c, o_pos);
                                    WriteCanMessageFromBus2Fifos (c, po->id, po->DataPtr, (unsigned char)po->ext, (unsigned char)po->size, 1, get_rt_cycle_counter());
                                    po->Protocol.Mux.token = pmo->Protocol.Mux.next;  // switch to next MUX object
                                }
                                ObjectAreSended(pmo);
                            }
                        }
                        break;
                    case J1939_OBJECT:
#if defined BUILD_WITH_J1939_SUPPORT
                        if (po->Protocol.J1939.status != SC_J1939_STATUS_PENDING) {
                            int do_call_J1939Transmit = 0;
                            if (!po->EventOrCyclic) {   // Cyclic
                                if (ObjectShouldBeSend (po)) {
                                    if (Blackboard2Object (CanServerConfig, c, o_pos) == 0) { do_call_J1939Transmit = 1; }
                                }
                            } else if (po->EventOrCyclic == 1) {
                                union FloatOrInt64 value;
                                value.d = 0.0;
                                int type = FLOAT_OR_INT_64_TYPE_F64;
                                type = execute_stack_whith_can_parameter ((struct EXEC_STACK_ELEM *)GET_CAN_EVENT_BYTECODE(CanServerConfig, o_pos), value, type, po, &value);
                                if (to_bool_FloatOrInt64(value, type)) {
                                    if (Blackboard2Object (CanServerConfig, c, o_pos) == 0) { do_call_J1939Transmit = 1; }
                                }
                            } else {   // == 2
                                union FloatOrInt64 value;
                                value.d = 0.0;
                                int type = FLOAT_OR_INT_64_TYPE_F64;
                                if (Blackboard2Object (CanServerConfig, c, o_pos) == 0) {
                                    type = execute_stack_whith_can_parameter ((struct EXEC_STACK_ELEM *)GET_CAN_EVENT_BYTECODE(CanServerConfig, o_pos), value, type, po, &value);
                                    if (to_bool_FloatOrInt64(value, type)) { do_call_J1939Transmit = 1; }
                                    MEMCPY (po->OldData, po->Data, (size_t)po->size);
                                }
                            }
                            if (do_call_J1939Transmit != 0) { // It should be transmit
                                unsigned short dlc;
                                unsigned short da;
                                if (po->Protocol.J1939.vid_dlc > 0) {
                                    dlc = read_bbvari_uword(po->Protocol.J1939.vid_dlc);
                                    if (dlc > po->size) { dlc = (unsigned short)po->size; } // check max.
                                } else {
                                    dlc = (unsigned short)po->size;
                                }
                                if (po->Protocol.J1939.dst_addr_vid > 0) {
                                    da = read_bbvari_ubyte(po->Protocol.J1939.dst_addr_vid);
                                } else {
                                    da = po->Protocol.J1939.dst_addr;
                                }
                                //ThrowError (1, "J1939Transmit");
#ifdef BUILD_WITH_J1939_SUPPORT
                                NewJ1939Transmit (CanServerConfig, c, (short)o_pos, dlc, (unsigned char)da);
#endif
                                po->should_be_send = 0;  // do not call ObjectAreSended(po);
                            }
                        }
#endif
                        break;
                    }
                }
            }
        }

        // polling the FIFOs
        WriteCanMessageFromFifo2Can (CanServerConfig, c);

        if (CanServerConfig->channels[c].status_can (CanServerConfig, c)) {
            ThrowError (1, "CAN Bus Off channel %i (0...7)", c);
            // If bus off wait 100 cycles than restart the CAN channel
            // TODO: configurable?
            BusOffCounter[c] = 100;
        }
    }
#ifdef REMOTE_MASTER
    //TransmitAllObjectsToFlexcardsOneCycle();
#endif
}

static void CanServerStop (void)
{
    int c;

    CleanAllReplaceConvs ();

    if (CanServerConfig == NULL) return;

    for (c = 0; c < CanServerConfig->channel_count; c++) {
        CanServerConfig->channels[c].close_can (CanServerConfig, c);
    }
#ifdef BUILD_WITH_J1939_SUPPORT
    NewJ1939Stop ();
#endif
    remove_additional_bbvaris (CanServerConfig);
    free_all_data_blocks (CanServerConfig);
#ifdef REMOTE_MASTER
    detach_all_bbvaris_and_equations (CanServerConfig);
    if (CanServerConfig != NULL) my_free (CanServerConfig);
#else
    if (CanServerConfig != NULL) remove_canserver_config (CanServerConfig);
#endif

    CanServerConfig = NULL;
    can_init_flag = CANSERVER_NOT_INIT; 
}


void new_canserv_cyclic (void)
{
#ifdef INC_CCP
    if (GlobalXcpCcpActiveFlag) CcpCyclic ();
#endif
    switch (can_init_flag) {
    case CANSERVER_NOT_INIT:
        CanServerRequestInit ();
        break;

    case CANSERVER_READ_INIT:
        CanServerReadInit ();
        break;

    case CANSERVER_SELECT_CAN_CARD:
        CanServerConfig->CanServerPid = GET_PID();

#ifdef REMOTE_MASTER
        if (CanServerSelectCanCard ()) {
#else
        if (CanServerSelectCanCard ()) {
#endif
            can_init_flag = CANSERVER_NOT_INIT;
        } else {
            can_init_flag = CANSERVER_OPEN_CAN;
        }
        break;

    case CANSERVER_OPEN_CAN:
        CanServerOpenCan ();
        break;

    case CANSERVER_CYCLIC:
        CanServerCyclic ();
#ifdef BUILD_WITH_J1939_SUPPORT
        if (CanServerConfig->j1939_flag) {
            int c;
            for (c = 0; c < CanServerConfig->channel_count; c++) {
                if (CanServerConfig->channels[c].j1939_flag) {
                    NewJ1939Cyclic (CanServerConfig, c);
                }
            }
        }
#endif
        break;

    case CANSERVER_STOP:
        CanServerStop ();
        break;
    }
}


void new_canserv_terminate (void)
{
#ifdef INC_CCP
    CcpTerminate ();
#endif
    if (can_init_flag >= CANSERVER_OPEN_CAN) {
        CanServerStop ();
    }
    remove_bbvari (CanBitErrorVid);

#ifndef REMOTE_MASTER
    if (CanServerConfig != NULL) remove_canserver_config (CanServerConfig);
    CanServerConfig = NULL;
#endif
}


int SendCanObjectForOtherProcesses (int Channel, unsigned int Id, int Ext, int Size, unsigned char *Data)
{
    //ThrowError (1, "SendCanObjectForOtherProcesses(%i, 0x%X, %i, %i, ...)", Channel, Id, Ext, Size);
    if (can_init_flag == CANSERVER_CYCLIC) {
        if (CanServerConfig->channel_count > Channel) {
            if ((CanBitError.Command != 0) &&   // Simulate an bit error inside a CAN object.
                 (CanBitError.Channel == Channel) &&
                 ((uint32_t)CanBitError.Id == Id)) {
                int SizeChecked;
                uint8_t DataCopy[CAN_BIT_ERROR_MAX_SIZE];
                switch (CanBitError.Command) {
                case OVERWRITE_DATA_BYTES:
                SizeChecked = Size;
                if (SizeChecked > CAN_BIT_ERROR_MAX_SIZE) SizeChecked = CAN_BIT_ERROR_MAX_SIZE;
                for (int x = 0; x < SizeChecked; x++) {
                    DataCopy[x] = Data[x] & CanBitError.AndMask[x];
                    DataCopy[x] |= CanBitError.OrMask[x];
                }
                if (CanBitError.ByteOrder) {
                    for (int x = 0; x < SizeChecked; x++) {
                        int xx = (SizeChecked - 1) - x;
                        uint8_t LocalData = Data[x];
                        LocalData &= CanBitError.AndMask[xx];
                        LocalData |= CanBitError.OrMask[xx];
                        DataCopy[x] = LocalData;
                    }
                } else {
                    for (int x = 0; x < SizeChecked; x++) {
                        uint8_t LocalData =  Data[x];
                        LocalData &= CanBitError.AndMask[x];
                        LocalData |= CanBitError.OrMask[x];
                        DataCopy[x] = LocalData;
                    }
                }
                break;
                case CHANGE_DATA_LENGTH:
                SizeChecked = CanBitError.Size;
                for (int x = 0; x < SizeChecked; x++) {
                    DataCopy[x] = Data[x];
                }
                break;
                case SUSPEND_TRANSMITION:
                Channel = -1;    // This Object should not be transmitted
                break;
                default:
                break;
                }
                if (Channel >= 0) {
                int Status = CanServerConfig->channels[Channel].queue_write_can (CanServerConfig, Channel, Id, DataCopy, (unsigned char)Ext, (unsigned char)SizeChecked);
                WriteCanMessageFromBus2Fifos (Channel, Id, DataCopy, (unsigned char)Ext, (unsigned char)SizeChecked, 1, get_rt_cycle_counter());
                return Status;
                } else {
                return 0; // This Object should not be transmitted
                }
            } else {
                int Status = CanServerConfig->channels[Channel].queue_write_can (CanServerConfig, Channel, Id, Data, (unsigned char)Ext, (unsigned char)Size);
                WriteCanMessageFromBus2Fifos (Channel, Id, Data, (unsigned char)Ext, (unsigned char)Size, 1, get_rt_cycle_counter());
                return Status;
            }
        }
    }
    return -1;
}

/* ------------------------------------------------------------ *\
 *    Task Control Block                                     *
\* ------------------------------------------------------------ */

TASK_CONTROL_BLOCK new_canserv_tcb
#ifdef REMOTE_MASTER
    = INIT_TASK_COTROL_BLOCK("CANServer", INTERN_SYNC, 333, new_canserv_cyclic, new_canserv_init, new_canserv_terminate, 1024*1024);
#else
= INIT_TASK_COTROL_BLOCK("CANServer", INTERN_ASYNC, 333, new_canserv_cyclic, new_canserv_init, new_canserv_terminate, 1024*1024);
#endif

uint32_t GetCanServerCycleTime_ms (void)
{
    return (uint32_t)new_canserv_tcb.time_steps;
}

int Mixed11And29BitIdsAllowed (int Channel)
{
    int Ret = 0;
    
    if ((Channel >= 0) && (Channel < MAX_CAN_CHANNELS)) {
        if (CanServerConfig != NULL) {
            if (CanServerConfig->channels[Channel].info_can != NULL) {
                CanServerConfig->channels[Channel].info_can ((struct NEW_CAN_SERVER_CONFIG_STRUCT*)CanServerConfig, 
                                                             Channel, CAN_CHANNEL_INFO_MIXED_IDS_ALLOWED, sizeof (int), &Ret);
            }
        }
    }
    return Ret;
}

void DecodeJ1939RxMultiPackageFrame(NEW_CAN_SERVER_CONFIG *csc, int Channel, int ObjectPos)
{
    NEW_CAN_SERVER_OBJECT *po;
    uint32_t cc;

    po = &csc->objects[ObjectPos];
    cc = read_bbvari_udword (po->vid); // Increment receive counter
    cc++;
    write_bbvari_udword (po->vid, cc);
    if (po->Protocol.J1939.vid_dlc > 0) { // Write receive DLC to blackboard
        write_bbvari_uword (po->Protocol.J1939.vid_dlc, (uint16_t)po->Protocol.J1939.ret_dlc);
    }
    Object2Blackboard (csc, Channel, ObjectPos, po->Protocol.J1939.ret_dlc);
}
