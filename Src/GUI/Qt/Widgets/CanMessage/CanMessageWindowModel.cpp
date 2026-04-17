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

#include <float.h>
extern "C" {
#include "MyMemory.h"
#include "MemZeroAndCopy.h"
#include "PrintFormatToString.h"
#include "MainValues.h"
#include "RemoteMasterControlProcess.h"
#include "ReadCanCfg.h"
#include "BlackboardAccess.h"
#include "ExecutionStack.h"
#include "Compare2DoubleEqual.h"
#include "ThrowError.h"
}

#include "CanMessageTreeView.h"
#include "CanMessageWindowModel.h"

#ifdef __GNUC__
#define _byteswap_ulong __builtin_bswap32
#define _byteswap_uint64 __builtin_bswap64
#endif

static void GlobalVariableChanged ();
void CurrentCanConfigObserver::ConnectTo()
{
    if (!m_InitFlag) {
        RegisterChangeCanConfigCallback(GlobalVariableChanged);
        m_InitFlag = true;
    }
}

void CurrentCanConfigObserver::CurrentCanConfigChanged()
{
    emit(CurrentCanConfigChangedSignal());
}

static CurrentCanConfigObserver CurrentCanConfigObserverInst;

static void GlobalVariableChanged ()
{
    CurrentCanConfigObserverInst.CurrentCanConfigChanged();
}

CurrentCanConfigObserver::CurrentCanConfigObserver()
{
    m_InitFlag = false;
}

CANMessageWindowModel::CANMessageWindowModel(double par_AbtastPeriodeInMs, bool par_DecodeFlag,
                                             QObject *parent)
   : QAbstractItemModel(parent)
{
    m_CanMessageRowCount = 0;
    m_CanMessageLines = nullptr;
    m_DecodeFlag = par_DecodeFlag;
    if (m_DecodeFlag) {
        m_CanServerConfig = GetACopyOfCurrentCanConfig();
    } else {
        m_CanServerConfig = nullptr;
    }
    CurrentCanConfigObserverInst.ConnectTo();
    connect(&CurrentCanConfigObserverInst, SIGNAL(CurrentCanConfigChangedSignal()), this, SLOT(UpdateCanServerConfigSlot()));

    m_AbtastPeriodeInMs = par_AbtastPeriodeInMs;

    m_RawOrConverted = true;

    m_Roles.append(Qt::DisplayRole);

    m_Root = new CanMessageRoot(this);
}

CANMessageWindowModel::~CANMessageWindowModel()
{
    if (m_CanServerConfig != nullptr) {
        DeleteCopiedCanConfig(m_CanServerConfig);
    }
    DeleteAllRawMessageLines();
    delete m_Root;
}

// return value are milliseconds
double CANMessageWindowModel::TimestampCalc (CAN_FIFO_ELEM_HEADER *pCanMessage)
{
    double Ts;

    if (pCanMessage->node) {  // from oneself
        // transmit messages: the timestamp are the cycle counter
        Ts = static_cast<double>(pCanMessage->timestamp) * m_AbtastPeriodeInMs;
    } else {
        if (s_main_ini_val.ConnectToRemoteMaster) {
            uint32_t Cycle;
            uint16_t T4Master, T4;
            switch (GetCanCardType (pCanMessage->channel)) {
            default:
                // receiving CAN messages are 1ns resolution
                Ts = static_cast<double>(pCanMessage->timestamp) * 0.000001;
                break;
            case 1:   // I_PCI-165
            case 2:   // I_PCI-XC16
                Cycle = static_cast<uint32_t>(pCanMessage->timestamp >> 32);
                T4Master = static_cast<uint16_t>(static_cast<uint32_t>(pCanMessage->timestamp) >> 16);
                T4 = static_cast<uint16_t>(pCanMessage->timestamp);
                Ts = static_cast<double>(Cycle) * m_AbtastPeriodeInMs + static_cast<double>(static_cast<int16_t>(T4 - T4Master)) / 1250.0;
                break;
            case 6: // Flexcard
                 // High DWord ist der UCx, LowDWord ist der FRCx
                Ts = static_cast<double>((pCanMessage->timestamp << 32) | (pCanMessage->timestamp >> 32)) / 80000.0;
                break;
            case 8: // SocketCAN
                Ts = static_cast<double>(pCanMessage->timestamp) / 1000.0;
                break;
            }
        } else {
            // receiving CAN messages are 1ns resolution
            Ts = static_cast<double>(pCanMessage->timestamp) * 0.000001;
        }
    }
    return Ts;
}

static int CmpEntrys (const void *a, const void *b)
{
    const NEW_CAN_HASH_ARRAY_ELEM *ac, *bc;

    ac = static_cast<const NEW_CAN_HASH_ARRAY_ELEM*>(a);
    bc = static_cast<const NEW_CAN_HASH_ARRAY_ELEM*>(b);
    if (ac->id < bc->id) return -1;
    else if (ac->id > bc->id) return 1;
    else return 0;
}

bool CANMessageWindowModel::CheckIfInsideCanConfig(int par_LineNo) const
{
    if (m_CanMessageLines[par_LineNo].ObjectNo >= 0) {
        return true;
    } else if (m_CanMessageLines[par_LineNo].ObjectNo == -1) {
        int Channel = m_CanMessageLines[par_LineNo].CanMessage.channel;
        uint32_t Id = m_CanMessageLines[par_LineNo].CanMessage.id;
        uint32_t Ext = m_CanMessageLines[par_LineNo].CanMessage.ext;
        int Index;
        NEW_CAN_HASH_ARRAY_ELEM *EntryPtr;
        NEW_CAN_HASH_ARRAY_ELEM Entry;

        Entry.id = (Ext << 31) | Id;
        // first check if inside the RX sorted Id list
        EntryPtr = static_cast<NEW_CAN_HASH_ARRAY_ELEM*>(bsearch (&Entry,
                                                         m_CanServerConfig->channels[Channel].hash_rx,
                                                         static_cast<size_t>(m_CanServerConfig->channels[Channel].rx_object_count),
                                                         sizeof (NEW_CAN_HASH_ARRAY_ELEM),
                                                         CmpEntrys));
        if (EntryPtr != nullptr) {
            m_CanMessageLines[par_LineNo].ObjectNo = EntryPtr->pos;
            return true;
        } else {
            // than check if inside the TX sorted Id list
            EntryPtr = static_cast<NEW_CAN_HASH_ARRAY_ELEM*>(bsearch (&Entry,
                                                             m_CanServerConfig->channels[Channel].hash_tx,
                                                             static_cast<size_t>(m_CanServerConfig->channels[Channel].tx_object_count),
                                                             sizeof (NEW_CAN_HASH_ARRAY_ELEM),
                                                             CmpEntrys));
            if (EntryPtr != nullptr) {
                m_CanMessageLines[par_LineNo].ObjectNo = EntryPtr->pos;
                return true;
            }
        }
        // check j1939 multi package objects (j1939 objects are now part of the hash_rx table so this is not neccessary)
        if (m_CanServerConfig->channels[Channel].j1939_flag) {
            for (int o = 0; o < m_CanServerConfig->channels[Channel].object_count; o++) {
                int ObjPos = m_CanServerConfig->channels[Channel].objects[o];
                NEW_CAN_SERVER_OBJECT *Object;
                Object = &(m_CanServerConfig->objects[ObjPos]);
                if (Object->id == Id) {
                    m_CanMessageLines[par_LineNo].ObjectNo = ObjPos;
                    // (Object->ext == )
                    // (Object->id == Id)
                    return true;
                }
            }
        }
        m_CanMessageLines[par_LineNo].ObjectNo = -2; // do not check again
    } else {
        // do not check again
    }
    return false;
}

void CANMessageWindowModel::TimestampCalcLine (CAN_MESSAGE_LINE *pcml)
{
    double Ts;

    Ts = TimestampCalc (&pcml->CanMessage);
    if (pcml->Counter >= 2) {
        pcml->dT = Ts - pcml->LastTime;
        if (pcml->dT < pcml->dTMin) pcml->dTMin = pcml->dT;
        if (pcml->dT > pcml->dTMax) pcml->dTMax = pcml->dT;
    }
    pcml->LastTime = Ts;
}

static inline uint64_t BuildChannelExtId(CAN_FIFO_ELEM_HEADER *par_Message)
{
    return ((uint64_t)par_Message->channel << 32) | (((uint64_t)(par_Message->ext & 0x1)) << 31) | (uint64_t)par_Message->id;
}

bool CANMessageWindowModel::BinarySearchLowestMostChannelId(CAN_FIFO_ELEM_HEADER *par_Message, int *ret_Pos)
{
    uint64_t ChannelAndId = BuildChannelExtId(par_Message);
    int Left = 0;
    int Right = m_CanMessageRowCount;
    while (Left < Right) {
        int Middle = (Left + Right) >> 1;
        uint64_t ChannelAndIdIterate = BuildChannelExtId(&(m_CanMessageLines[Middle].CanMessage));
        if (ChannelAndId < ChannelAndIdIterate) {
            Right = Middle;
        } else if (ChannelAndId == ChannelAndIdIterate) {
            // channel and id match
            *ret_Pos = Middle;
            return true;
        } else {
            Left = Middle + 1;
        }
    }
    *ret_Pos = Right;
    return false;
}

static void CopyData(CAN_MESSAGE_LINE *par_CanMessageLine, uint8_t *par_Data, int par_Size)
{
    if (par_CanMessageLine->DataSize < par_Size) {
        par_CanMessageLine->DataPtr = (uint8_t*)my_realloc(par_CanMessageLine->DataPtr, par_Size);
    }
    if(par_CanMessageLine->DataPtr != nullptr) {
        par_CanMessageLine->DataSize = par_Size;
        MEMCPY(par_CanMessageLine->DataPtr, par_Data, par_Size);
    }
}

void CANMessageWindowModel::AddNewCANMessages(int par_NumberOfMessages, CAN_FIFO_ELEM_HEADER *par_CANMessages, uint8_t **DataPtrs)
{
    for (int x = 0; x < par_NumberOfMessages; x++) {
        CAN_FIFO_ELEM_HEADER *pCANMessage = &par_CANMessages[x];
        if (m_CanMessageLines == nullptr) {
            beginInsertRows(QModelIndex(), 0, 0);
            m_CanMessageRowCount = 1;
            // make it always one row larger as neccessary because GetValue() can read behind the last can data
            m_CanMessageLines = static_cast<CAN_MESSAGE_LINE*>(my_calloc (m_CanMessageRowCount + 1, sizeof (CAN_MESSAGE_LINE)));
            if (m_CanMessageLines == nullptr) return;
            m_CanMessageLines->ObjectNo = -1;
            m_CanMessageLines->UpdateNeeded = true;
            m_CanMessageLines->dTMin = DBL_MAX;
            m_CanMessageLines->dTMax = 0.0;
            m_CanMessageLines->CanMessage = *pCANMessage;
            m_CanMessageLines->DataPtr = nullptr;
            m_CanMessageLines->DataSize = 0;
            m_CanMessageLines->Counter = 1;
            CopyData(m_CanMessageLines, DataPtrs[x], pCANMessage->size);
            TimestampCalcLine (m_CanMessageLines);
            m_Root->AddNewMessageAt(0, m_RawOrConverted);
            endInsertRows();
        } else {
            int Pos;
            if (BinarySearchLowestMostChannelId(pCANMessage, &Pos)) {
                m_CanMessageLines[Pos].CanMessage = *pCANMessage;
                CopyData(&m_CanMessageLines[Pos], DataPtrs[x], pCANMessage->size);
                m_CanMessageLines[Pos].Counter++;
                m_CanMessageLines[Pos].UpdateNeeded = true;
                TimestampCalcLine (&m_CanMessageLines[Pos]);
                m_Root->AnalyzeMessage(m_CanServerConfig, m_CanMessageLines, Pos);
            } else {
                // Pos = (0...m_CanMessageLines) which should be replaced with current message
                // if Pos = m_CanMessageLines the message should be append
                // at the message to the end of the array
                beginInsertRows(QModelIndex(), Pos, Pos);
                m_CanMessageLines = static_cast<CAN_MESSAGE_LINE*>(my_realloc (m_CanMessageLines,
                                                                               static_cast<size_t>(m_CanMessageRowCount + 2) *
                                                                               sizeof (CAN_MESSAGE_LINE)));
                if (Pos < m_CanMessageRowCount) {
                    memmove(m_CanMessageLines + Pos + 1, m_CanMessageLines + Pos,
                            sizeof(CAN_MESSAGE_LINE) * (m_CanMessageRowCount - Pos));
                }
                m_CanMessageLines[Pos].ObjectNo = -1;
                m_CanMessageLines[Pos].UpdateNeeded = true;
                m_CanMessageLines[Pos].dTMin = DBL_MAX;
                m_CanMessageLines[Pos].dTMax = 0.0;
                m_CanMessageLines[Pos].CanMessage = *pCANMessage;
                m_CanMessageLines[Pos].DataPtr = nullptr;
                m_CanMessageLines[Pos].DataSize = 0;
                m_CanMessageLines[Pos].Counter = 1;
                CopyData(&m_CanMessageLines[Pos], DataPtrs[x], pCANMessage->size);
                TimestampCalcLine (&m_CanMessageLines[Pos]);
                m_CanMessageRowCount++;
                m_Root->AddNewMessageAt(Pos, m_RawOrConverted);
                m_Root->AnalyzeMessage(m_CanServerConfig, m_CanMessageLines, Pos);
                endInsertRows();
            }
        }
    }
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

// Read one signals bits from the object data
static __inline uint64_t GetValue (NEW_CAN_SERVER_OBJECT *po,
                                  NEW_CAN_SERVER_SIGNAL *ps,
                                  uint8_t *Data)
{
    uint32_t PosByte;
    uint32_t PosBit;
    uint64_t Ret;

    if (Data != nullptr) {
        if (ps->byteorder == MSB_FIRST_BYTEORDER) {  // 1 -> MSB_FISRT
            int32_t PosByte_m8;
            uint32_t IStartBit = (uint32_t)((po->size << 3) - ps->startbit);
            PosBit = IStartBit & 0x7;
            PosByte = IStartBit >> 3;
            PosByte_m8 = (int32_t)PosByte - 8;
            Ret = _byteswap_uint64 (*(uint64_t *)(void*)(Data + PosByte_m8)) << PosBit;
            Ret |= (uint64_t)Data[PosByte] >> (8 - PosBit);
        } else {  // 0 -> LSB first
            PosBit = ps->startbit & 0x7;
            PosByte = (uint32_t)(ps->startbit >> 3);
            Ret = *(uint64_t *)(void*)(Data + PosByte) >> PosBit;
            Ret |= ((uint64_t)Data[PosByte + 8] << (8 - PosBit)) << 56;
        }
        return Ret & ps->mask;
    } else {
        return 0;
    }
}

int CanMessageSignal::NonInvertedConvertSignal (NEW_CAN_SERVER_CONFIG *csc, int o_pos, int s_pos)
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
        m_ConvertedType = mul_FloatOrInt64(ps->conv, ps->conv_type, m_Value, m_Type, &m_ConvertedValue);
        m_ConvertedType = add_FloatOrInt64(ps->offset, ps->offset_type, m_ConvertedValue, m_ConvertedType, &m_ConvertedValue);
        break;
    case CAN_CONV_FACTOFF2:
        m_ConvertedType = add_FloatOrInt64(ps->offset, ps->offset_type, m_Value, m_Type, &m_ConvertedValue);
        m_ConvertedType = mul_FloatOrInt64(ps->conv, ps->conv_type, m_ConvertedValue, m_ConvertedType, &m_ConvertedValue);
        break;
    case CAN_CONV_EQU:
        m_ConvertedType = execute_stack_whith_can_parameter ((struct EXEC_STACK_ELEM *)GET_CAN_SIG_BYTECODE(csc, s_pos), m_Value, m_Type,
                                                  &csc->objects[o_pos], &m_ConvertedValue);
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
            if (m_Value.d <= x1) break;
        }
        if (CompareDoubleEqual_int(x1, x2)) m_ConvertedValue.d = y1;
        else if (m_Value.d <= x2) m_ConvertedValue.d = y2;
        else if (m_Value.d >= x1) m_ConvertedValue.d = y1;
        else {
            m = (y1-y2)/(x1-x2);
            m_ConvertedValue.d = y2 + (m_Value.d - x2) * m;
        }
        m_ConvertedType = FLOAT_OR_INT_64_TYPE_F64;
        break;
    case CAN_CONV_REPLACED:
        // do not handle this here
        break;
    }
    return 0;
}

int CanMessageSignal::InvertedConvertSignal (NEW_CAN_SERVER_CONFIG *csc, int o_pos, int s_pos)
{
    NEW_CAN_SERVER_SIGNAL *ps;
    int x, dim;
    double *p;
    double x1, x2;
    double y1, y2;
    double m;
    double HelpDiv, HelpSub;

    ps = &(csc->bb_signals[s_pos]);

    switch (ps->ConvType) {
    case CAN_CONV_NONE:
    default:
        // do nothing
        break;
    case CAN_CONV_FACTOFF:
        HelpDiv = to_double_FloatOrInt64(ps->conv, m_Type);
        if (HelpDiv == 0.0) {
            return -1;
        } else {
            HelpSub = to_double_FloatOrInt64(ps->offset, m_Type);
            m_ConvertedValue.d = (to_double_FloatOrInt64(m_Value, m_Type) - HelpSub) / HelpDiv;
            m_ConvertedType = FLOAT_OR_INT_64_TYPE_F64;
        }
        break;
    case CAN_CONV_FACTOFF2:
        HelpDiv = to_double_FloatOrInt64(ps->conv, m_Type);
        if (HelpDiv == 0.0) {
            return -1;
        } else {
            HelpSub = to_double_FloatOrInt64(ps->offset, m_Type);
            m_ConvertedValue.d = to_double_FloatOrInt64(m_Value, m_Type) / HelpDiv - HelpSub;
            m_ConvertedType = FLOAT_OR_INT_64_TYPE_F64;
        }
        break;
    case CAN_CONV_EQU:
        return -1;
        //*type = execute_stack_whith_can_parameter ((struct EXEC_STACK_ELEM *)GET_CAN_SIG_BYTECODE(csc, s_pos), *value, *type,
        //                                          &csc->objects[o_pos], value);
        //break;
    case CAN_CONV_CURVE:
        p = (double*)(void*)GET_CAN_SIG_BYTECODE(csc, s_pos);
        dim = (int)*p;
        p += 2;
        if ((dim < 2) || (dim > 255)) break;  // max. 255 points allowed
        x2 = x1 = *p++;
        y2 = y1 = *p++;
        for (x = 0; x < (dim-1); x++) {
            x2 = x1;
            y2 = y1;
            x1 = *p++;
            y1 = *p++;
            if (m_Value.d <= x1) break;
        }
        if (CompareDoubleEqual_int(x1, x2)) m_ConvertedValue.d = y1;
        else if (m_Value.d <= x2) m_ConvertedValue.d = y2;
        else if (m_Value.d >= x1) m_ConvertedValue.d = y1;
        else {
            m = (y1-y2)/(x1-x2);
            m_ConvertedValue.d = y2 + (m_Value.d - x2) * m;
        }
        m_ConvertedType = FLOAT_OR_INT_64_TYPE_F64;
        break;
    case CAN_CONV_REPLACED:
        // do not handle this here
        break;
    }
    return 0;
}

static __inline int32_t GetMuxValueFromData (NEW_CAN_SERVER_OBJECT *po,
                                             NEW_CAN_SERVER_SIGNAL *ps,
                                             uint8_t *Data)
{
    uint32_t PosByte;
    uint32_t PosBit;
    uint32_t Ret;

    if (Data != nullptr) {
        if (ps->byteorder == MSB_FIRST_BYTEORDER) {  // 1 -> MSB_FISRT
            int32_t PosByte_m4;
            uint32_t IStartBit = (uint32_t)((po->size << 3) - ps->mux_startbit);
            PosBit = IStartBit & 0x7;
            PosByte = IStartBit >> 3;
            PosByte_m4 = (int32_t)PosByte - 4;
            Ret = _byteswap_ulong (*(uint32_t *)(void*)(Data + PosByte_m4)) << PosBit;
            Ret |= (uint32_t)Data[PosByte] >> (8 - PosBit);
        } else {  // 0 -> LSB first
            PosBit = ps->mux_startbit & 0x7;
            PosByte = (uint32_t)(ps->mux_startbit >> 3);
            Ret = *(uint32_t *)(void*)(Data + PosByte) >> PosBit;
            Ret |= ((uint32_t)Data[PosByte + 4] << (8 - PosBit)) << 24;
        }
        return (int32_t)(Ret & ps->mux_mask);
    } else {
        return 0;
    }
}

static __inline uint32_t GetMuxObjValue (NEW_CAN_SERVER_OBJECT *po, unsigned char* Data)
{
    uint32_t PosByte;
    uint32_t PosBit;
    uint32_t Ret;

    if (Data != nullptr) {
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
        return Ret & po->Protocol.Mux.Mask;
    } else {
        return 0;
    }
}

int CanMessageSignal::ConvertSignal(NEW_CAN_SERVER_CONFIG *csc, int o_pos, int s_pos)
{
    NEW_CAN_SERVER_OBJECT *po;
    po = &(csc->objects[o_pos]);
    if (po->dir) {
        return InvertedConvertSignal (csc, o_pos, s_pos);
    } else {
        return NonInvertedConvertSignal (csc, o_pos, s_pos);
    }
}

static int ExtractSignalFromData (NEW_CAN_SERVER_CONFIG *csc, int o_pos, int s_pos, CAN_FIFO_ELEM_HEADER *Message,
                                  uint8_t *DataPtr, union FloatOrInt64 *ret_Value)
{
    int s, s_equ_sb, s_equ_v;
    NEW_CAN_SERVER_OBJECT *po;
    NEW_CAN_SERVER_SIGNAL *ps;
    union FloatOrInt64 value;
    int type;
    int mux_value;
    uint64_t iv;
    int ret = FLOAT_OR_INT_64_TYPE_INVALID;

    po = &(csc->objects[o_pos]);
    ps = &(csc->bb_signals[s_pos]);
    switch (ps->type) {        // 0 normal
    case NORMAL_SIGNAL:
        if (((ps->startbit + ps->bitsize) >> 3) <= Message->size) {
            iv = GetValue (po, ps, DataPtr);
            if (ps->float_type) {
                value.d = ConvWithFloat ((uint32_t)iv);
                type = FLOAT_OR_INT_64_TYPE_F64;
            } else {
                type = ConvWithSign (iv, ps->bitsize, ps->sign, &value);
            }
            ret = type;
            *ret_Value = value;
        }
        break;
    case MUX_SIGNAL:
        if (((ps->mux_startbit + ps->mux_bitsize) >> 3) <= Message->size) {
            mux_value = GetMuxValueFromData (po, ps, DataPtr);
            for (s_equ_sb = s_pos; s_equ_sb >= 0; s_equ_sb = csc->bb_signals[s_equ_sb].mux_equ_sb_childs) {
                if (csc->bb_signals[s_equ_sb].mux_value == mux_value) {
                    for (s_equ_v = s_equ_sb; s_equ_v >= 0; s_equ_v = csc->bb_signals[s_equ_v].mux_equ_v_childs) {
                        ps = &(csc->bb_signals[s_equ_v]);
                        if (((ps->startbit + ps->bitsize) >> 3) <= Message->size) {
                            iv = GetValue (po, ps, DataPtr);
                            if (ps->float_type) {
                                value.d = ConvWithFloat ((uint32_t)iv);
                                type = FLOAT_OR_INT_64_TYPE_F64;
                            } else {
                                type = ConvWithSign (iv, ps->bitsize, ps->sign, &value);
                            }
                            ret = type;
                            *ret_Value = value;
                        }
                    }
                    break;
                }
            }
        }
        break;
    case MUX_BY_SIGNAL:
        // this can not handled -> ignored
        break;
    }
    return ret;
}

static QString MessageDataToString(CAN_FIFO_ELEM_HEADER *par_Message, uint8_t *par_DataPtr)
{
    char *DataString;
    int SizeOfString = par_Message->size * 3 + 1;
    DataString = (char*)alloca(SizeOfString);
    DataString[0] = '\0';
    char *p = DataString;
    int Size = par_Message->size;
    for (int x = 0; x < Size; x++) {
        p += PrintFormatToString (p, SizeOfString - (p - DataString), "%02X ", static_cast<int>(par_DataPtr[x]));
    }
    return QString (DataString);
}

QVariant CANMessageWindowModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        int Row = index.row();
        int Column = index.column();
        CanMessageElement *Element = static_cast<CanMessageElement*>(index.internalPointer());
        switch(Element->GetType()) {
        case CanMessageElement::TREE_ROOT:
            break;
        case CanMessageElement::TREE_RAW_MESSAGE_DATA_LINE:
            // it is a raw message line
            if (Row < m_CanMessageRowCount) {
                switch(Column) {
                case COLUMN_TYPE_COUNT:
                    return QString().number(m_CanMessageLines[Row].Counter);
                case COLUMN_TYPE_TIME:
                    return QString().number(m_CanMessageLines[Row].LastTime, 'f', 3);
                case COLUMN_TYPE_DIFF_TIME:
                    if (m_CanMessageLines[Row].Counter >= 2) {
                        return QString().number(m_CanMessageLines[Row].dT, 'f', 3);
                    } else {
                        return QString("undef");
                    }
                case COLUMN_TYPE_MIN_MAX_DIFF_TIME:
                    if (m_CanMessageLines[Row].Counter >= 2) {
                        QString Ret;
                        Ret.append(QString().number(m_CanMessageLines[Row].dTMin, 'f', 3));
                        Ret.append("/");
                        Ret.append(QString().number(m_CanMessageLines[Row].dTMax, 'f', 3));
                        return Ret;
                    } else {
                         return QString("undef");
                    }
                case COLUMN_TYPE_DIR:
                    if (m_CanMessageLines[Row].CanMessage.node) return QString ("->");
                    else return QString ("<-");
                case COLUMN_TYPE_CHANNEL:
                    return QString().number(m_CanMessageLines[Row].CanMessage.channel);
                case COLUMN_TYPE_IDENTIFIER:
                    return QString().number(m_CanMessageLines[Row].CanMessage.id, 16);
                case COLUMN_TYPE_TYPE:
                    switch (m_CanMessageLines[Row].CanMessage.ext) {
                    case 0:
                        return QString("n");
                    case 1:
                        return QString("e");
                    case 2:
                        return QString("bn");
                    case 3:
                        return QString("be");
                    case 4:
                        return QString("fn");
                    case 5:
                        return QString("fe");
                    case 6:
                        return QString("fbn");
                    case 7:
                        return QString("fbe");
                    case 0x10:
                        return QString("J1939MP");
                    default:
                        return QString("");
                    }
                case COLUMN_TYPE_SIZE:
                    return QString().number(m_CanMessageLines[Row].CanMessage.size);
                case COLUMN_TYPE_NAME:
                    if (m_CanServerConfig != nullptr) {
                        int ObjectNo = m_CanMessageLines[Row].ObjectNo;
                        if (ObjectNo >= 0) {
                            return QString(GET_CAN_OBJECT_NAME (m_CanServerConfig, ObjectNo));
                        }
                    }
                    return QVariant();
                case COLUMN_TYPE_DATA:
                    return MessageDataToString(&(m_CanMessageLines[Row].CanMessage), m_CanMessageLines[Row].DataPtr);
                defualt:
                    return QVariant();
                }
            }
            break;
        case CanMessageElement::TREE_MUX_MESSAGE:
            if (m_CanServerConfig != nullptr) {
                CanMessageMuxObject * MuxObject = static_cast<CanMessageMuxObject*>(index.internalPointer());
                switch (Column) {
                case COLUMN_TYPE_COUNT:
                    return QString().number(MuxObject->GetCounter());
                case COLUMN_TYPE_TIME:
                    return QString().number(MuxObject->GetTimeStamp(), 'f', 3);
                case COLUMN_TYPE_DIFF_TIME:
                    if (MuxObject->GetCounter() >= 2) {
                    return QString().number(MuxObject->GetDiffTime(), 'f', 3);
                } else {
                    return QString("undef");
                }
                case COLUMN_TYPE_MIN_MAX_DIFF_TIME:
                    if (MuxObject->GetCounter() >= 2) {
                        QString Ret;
                        Ret.append(QString().number(MuxObject->GetMinDiffTime(), 'f', 3));
                        Ret.append("/");
                        Ret.append(QString().number(MuxObject->GetMaxDiffTime(), 'f', 3));
                        return Ret;
                    } else {
                        return QString("undef");
                    }
                case COLUMN_TYPE_IDENTIFIER:  // the MUX value will be printed inside the identifier column
                    return QString().number(MuxObject->GetMuxValue());
                case COLUMN_TYPE_SIZE:
                    return QString().number(MuxObject->GetSize());
                case COLUMN_TYPE_DATA:
                    return MuxObject->GetValueString();
                default:
                    return QVariant();
                }
            }
            break;
        case CanMessageElement::TREE_MULTI_C_PG:
            if (m_CanServerConfig != nullptr) {
                switch (Column) {
                case COLUMN_TYPE_IDENTIFIER:  // Id (signal name)
                    return QString("Multi PG");
                default:
                    return QVariant();
                }
            }
            break;
        case CanMessageElement::TREE_C_PG:
            if (m_CanServerConfig != nullptr) {
                CanMessageCPg* CPgObject = static_cast<CanMessageCPg*>(index.internalPointer());
                switch (Column) {
                case COLUMN_TYPE_COUNT:
                    return QString().number(CPgObject->GetCounter());
                case COLUMN_TYPE_TIME:
                    return QString().number(CPgObject->GetTimeStamp(), 'f', 3);
                case COLUMN_TYPE_DIFF_TIME:
                    if (CPgObject->GetCounter() >= 2) {
                        return QString().number(CPgObject->GetDiffTime(), 'f', 3);
                    } else {
                        return QString("undef");
                    }
                case COLUMN_TYPE_MIN_MAX_DIFF_TIME:
                    if (CPgObject->GetCounter() >= 2) {
                        QString Ret;
                        Ret.append(QString().number(CPgObject->GetMinDiffTime(), 'f', 3));
                        Ret.append("/");
                        Ret.append(QString().number(CPgObject->GetMaxDiffTime(), 'f', 3));
                        return Ret;
                    } else {
                        return QString("undef");
                    }
                case COLUMN_TYPE_IDENTIFIER:
                {
                    int ObjectNo = CPgObject->GetObjectNo();
                    QString Ret = QString().number(m_CanServerConfig->objects[ObjectNo].id, 16);
                    return Ret;
                }
                case COLUMN_TYPE_SIZE:
                {
                    int ObjectNo = CPgObject->GetObjectNo();
                    return QString().number(m_CanServerConfig->objects[ObjectNo].size);
                }
                case COLUMN_TYPE_DATA:
                    return CPgObject->GetValueString();
                default:
                    return QVariant();
                }
            }
            break;
        case CanMessageElement::TREE_MESSAGE_MULTIPLEXER:
            if (m_CanServerConfig != nullptr) {
                CanMessageMultiplexer *ObjectMultiplexer = static_cast<CanMessageMultiplexer*>(index.internalPointer());
                switch (Column) {
                case COLUMN_TYPE_IDENTIFIER:  // Id (signal name)
                    return QString("Mux");
                case COLUMN_TYPE_DATA:
                    return QString().number(ObjectMultiplexer->GetMuxValue());
                default:
                    return QVariant();
                }
            }
            break;
        case CanMessageElement::TREE_SIGNAL_MULTIPLEXER:
            if (m_CanServerConfig != nullptr) {
                CanMessageSignalMultiplexer *SignalMultiplexer = static_cast<CanMessageSignalMultiplexer*>(index.internalPointer());
                switch (Column) {
                case COLUMN_TYPE_IDENTIFIER:  // Id (signal name)
                    return QString("Mux");
                case COLUMN_TYPE_DATA:
                    return QString().number(SignalMultiplexer->GetMuxValue());
                default:
                    return QVariant();
                }
            }
            break;
        case CanMessageElement::TREE_SIGNAL:
        case CanMessageElement::TREE_MUX_SIGNAL:
            if (m_CanServerConfig != nullptr) {
                CanMessageSignal *Signal = static_cast<CanMessageSignal*>(index.internalPointer());
                int SignalNo = Signal->GetSignalNo();
                switch (Column) {
                case COLUMN_TYPE_COUNT:
                    return QString().number(Signal->GetCounter());
                case COLUMN_TYPE_TIME:
                    return QString().number(Signal->GetTimeStamp(), 'f', 3);
                case COLUMN_TYPE_DIFF_TIME:
                    if (Signal->GetCounter() >= 2) {
                        return QString().number(Signal->GetDiffTime(), 'f', 3);
                    } else {
                        return QString("undef");
                    }
                case COLUMN_TYPE_MIN_MAX_DIFF_TIME:
                    if (Signal->GetCounter() >= 2) {
                        QString Ret;
                        Ret.append(QString().number(Signal->GetMinDiffTime(), 'f', 3));
                        Ret.append("/");
                        Ret.append(QString().number(Signal->GetMaxDiffTime(), 'f', 3));
                        return Ret;
                    } else {
                        return QString("undef");
                    }
                case COLUMN_TYPE_NAME:
                    return QString(GET_CAN_SIG_NAME (m_CanServerConfig, SignalNo));
                case COLUMN_TYPE_DATA:  // data (signal value)
                    return Signal->GetValueString(m_CanServerConfig);
                default:
                    return QVariant();
                }
            }
            return QVariant();
        }
    }
    return QVariant();
}

QVariant CANMessageWindowModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case COLUMN_TYPE_COUNT:
            return QString ("Count       ");
        case COLUMN_TYPE_TIME:
            return QString ("aTime(ms)    ");
        case COLUMN_TYPE_DIFF_TIME:
            return QString ("dTime(ms)");
        case COLUMN_TYPE_MIN_MAX_DIFF_TIME:
            return QString ("dTime(ms) min/max  ");
        case COLUMN_TYPE_DIR:
            return QString ("Dir");
        case COLUMN_TYPE_CHANNEL:
            return QString ("Ch.");
        case COLUMN_TYPE_IDENTIFIER:
            return QString ("Identifier");
        case COLUMN_TYPE_TYPE:
            return QString ("Type");
        case COLUMN_TYPE_SIZE:
            return QString ("Size");
        case COLUMN_TYPE_NAME:
            return QString ("Name");
        case COLUMN_TYPE_DATA:
            return QString ("Data[0...n]");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

QModelIndex CANMessageWindowModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        CanMessageElement *Element = static_cast<CanMessageElement*>(parent.internalPointer());
        CanMessageElement *Child = Element->child(row);
        return createIndex (row, column, Child);
    } else {
        return createIndex (row, column, m_Root->child(row));
    }
}

QModelIndex CANMessageWindowModel::parent(const QModelIndex &index) const
{
    if (index.isValid() &&
        (m_CanServerConfig != nullptr)) {
        CanMessageElement *Element = static_cast<CanMessageElement*>(index.internalPointer());
        if (Element != nullptr) {
            CanMessageElement *Parent = Element->parent();
            if (Parent != nullptr) {
                CanMessageElement *ParentOfParent = Parent->parent();
                if (ParentOfParent != nullptr) {
                    int Row = ParentOfParent->childNumber(Parent);
                    return createIndex (Row, 0, Parent);
                }
            }
        }
    }
    return QModelIndex();
}

int CANMessageWindowModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        if (m_CanServerConfig != nullptr) {
            CanMessageElement *Element = static_cast<CanMessageElement*>(parent.internalPointer());
            if (Element != nullptr) {
                return Element->childCount();
            }
        }
    } else {
        return m_CanMessageRowCount;
    }
    return 0;
}

void CANMessageWindowModel::UpdateCanServerConfigSlot()
{
    ClearAll(m_DecodeFlag, true);
}

#define IS_COLUMN_VISIBLE(n) ((par_VisibleLeftColumn <= (n)) && (par_VisibleRightColumn >= (n)))

void CANMessageWindowModel::UpdateOneRowCounterTime(int par_Row, QModelIndex &par_ParentIndex,
                                                    int par_VisibleLeftColumn, int par_VisibleRightColumn)
{
    if (m_DisplayFirst4ColumnFlag && (par_VisibleLeftColumn == 0) && (par_VisibleRightColumn >= COLUMN_TYPE_MIN_MAX_DIFF_TIME)) {
        QModelIndex Index = index(par_Row, COLUMN_TYPE_COUNT, par_ParentIndex);
        QModelIndex Index2 = index(par_Row, COLUMN_TYPE_MIN_MAX_DIFF_TIME, par_ParentIndex);
        emit dataChanged(Index, Index2, m_Roles);
    } else {
        if (m_DisplayColumnCounterFlag && IS_COLUMN_VISIBLE(COLUMN_TYPE_COUNT)) {
            QModelIndex Index = index(par_Row, COLUMN_TYPE_COUNT, par_ParentIndex);
            emit dataChanged(Index, Index, m_Roles);
        }
        if (m_DisplayColumnTimeAbsoluteFlag && IS_COLUMN_VISIBLE(COLUMN_TYPE_TIME)) {
            QModelIndex Index = index(par_Row, COLUMN_TYPE_TIME, par_ParentIndex);
            emit dataChanged(Index, Index, m_Roles);
        }
        if (m_DisplayColumnTimeDiffFlag && IS_COLUMN_VISIBLE(COLUMN_TYPE_DIFF_TIME)) {
            QModelIndex Index = index(par_Row, COLUMN_TYPE_DIFF_TIME, par_ParentIndex);
            emit dataChanged(Index, Index, m_Roles);
        }
        if (m_DisplayColumnTimeDiffMinMaxFlag && IS_COLUMN_VISIBLE(COLUMN_TYPE_MIN_MAX_DIFF_TIME)) {
            QModelIndex Index = index(par_Row, COLUMN_TYPE_MIN_MAX_DIFF_TIME, par_ParentIndex);
            emit dataChanged(Index, Index, m_Roles);
        }
    }
}

void CANMessageWindowModel::UpdateOneRowSignal(int par_Row, QModelIndex &par_ParentIndex,
                                               int par_VisibleLeftColumn, int par_VisibleRightColumn)
{
    UpdateOneRowCounterTime(par_Row, par_ParentIndex,
                            par_VisibleLeftColumn, par_VisibleRightColumn);
    if(IS_COLUMN_VISIBLE(COLUMN_TYPE_DATA)) {
        QModelIndex Index = index(par_Row, COLUMN_TYPE_DATA, par_ParentIndex);
        emit dataChanged(Index, Index, m_Roles);
    }
}

void CANMessageWindowModel::UpdateOneRowSignalMultiplexer(CANMessageTreeView *par_View, int par_Row,
                                                          CanMessageSignalMultiplexer* par_SignalMultiplexer,
                                                          QModelIndex &par_ParentIndex,
                                                          int par_VisibleLeftColumn, int par_VisibleRightColumn)
{
    UpdateOneRowCounterTime(par_Row, par_ParentIndex,
                            par_VisibleLeftColumn, par_VisibleRightColumn);
    if(IS_COLUMN_VISIBLE(COLUMN_TYPE_DATA)) {
        QModelIndex Index = index(par_Row, COLUMN_TYPE_DATA, par_ParentIndex);
        emit dataChanged(Index, Index, m_Roles);
    }
    // Iterate throgh all signals inside a signal multiplexer
    QModelIndex SignalMultiplexerIndex = index(par_Row, 0, par_ParentIndex);
    if (par_View->isExpanded(SignalMultiplexerIndex)) {
        for (int s = 0; s < par_SignalMultiplexer->childCount(); s++) {
            CanMessageMuxSignal* Signal = par_SignalMultiplexer->GetMuxSignal(s);
            if (Signal->NeedUpdate()) {
                UpdateOneRowSignal(s, SignalMultiplexerIndex,
                                   par_VisibleLeftColumn, par_VisibleRightColumn);
            }
        }
    }
}

void CANMessageWindowModel::UpdateViews(CANMessageTreeView *par_View,
                                        int par_VisibleLeftColumn, int par_VisibleRightColumn,
                                        int par_VisibleTopRow, int par_VisibleButtomRow)
{
    for (int DataLine = 0; DataLine < m_CanMessageRowCount; DataLine++) {
        if (m_CanMessageLines[DataLine].UpdateNeeded) {
            m_CanMessageLines[DataLine].UpdateNeeded = false;
            QModelIndex Index;
            UpdateOneRowCounterTime(DataLine, Index,
                                    par_VisibleLeftColumn, par_VisibleRightColumn);
            if(IS_COLUMN_VISIBLE(COLUMN_TYPE_DATA)) {
                QModelIndex Index = index(DataLine, COLUMN_TYPE_DIR);
                emit dataChanged(Index, Index, m_Roles);
            }
            if(IS_COLUMN_VISIBLE(COLUMN_TYPE_SIZE)) {
                QModelIndex Index = index(DataLine, COLUMN_TYPE_SIZE);
                emit dataChanged(Index, Index, m_Roles);
            }
            if(IS_COLUMN_VISIBLE(COLUMN_TYPE_DATA)) {
                QModelIndex Index = index(DataLine, COLUMN_TYPE_DATA);
                emit dataChanged(Index, Index, m_Roles);
            }
        }
    }
    if (m_CanServerConfig != nullptr) {
        for (int c = par_VisibleTopRow; (c < m_Root->childCount()) && (c < par_VisibleButtomRow); c++) {
            CanMessageRawDataLine *RawDataLine = static_cast<CanMessageRawDataLine*>(m_Root->child(c));
            QModelIndex RawLineIndex = index(c, 0);
            if ((RawDataLine != nullptr) && par_View->isExpanded(RawLineIndex)) {
                // Iterate throgh all multi PG inside a CAN message line (normaly this has zero or one entry)
                int NumberOfMultiPgMultipexers = RawDataLine->GetNumberOfMultiPgMultipexers();
                for (int om = 0; om < NumberOfMultiPgMultipexers; om++) {
                    QModelIndex MultiPgMultiplexerIndex = index(om, 0, RawLineIndex);
                    CanMessageMultiCPg* MultiPgMultiplexer = RawDataLine->GetMultiPgMultiplexer(om);
                    if (MultiPgMultiplexer->NeedUpdate() && par_View->isExpanded(MultiPgMultiplexerIndex)) {
                        // Iterate throgh all C_PG object inside a multi PG object
                        for (int o = 0; o < MultiPgMultiplexer->childCount(); o++) {
                            QModelIndex CPgObjectIndex = index(o, 0, MultiPgMultiplexerIndex);
                            CanMessageCPg* CPgObject = MultiPgMultiplexer->GetCPgObject(o);
                            if (CPgObject->NeedUpdate()) {
                                UpdateOneRowCounterTime(o, MultiPgMultiplexerIndex,
                                                        par_VisibleLeftColumn, par_VisibleRightColumn);
                                if(IS_COLUMN_VISIBLE(COLUMN_TYPE_DATA)) {
                                    QModelIndex Index = index(o, COLUMN_TYPE_DATA, MultiPgMultiplexerIndex);
                                    emit dataChanged(Index, Index, m_Roles);
                                }
                                if (par_View->isExpanded(CPgObjectIndex)) {
                                    // Iterate throgh all signal multiplexers inside a mux object (normaly this has zero or one entry)
                                    int NumberOfSignalMultiplexers = CPgObject->GetNumberOfSignalMultiplexers();
                                    for (int sm = 0; sm < NumberOfSignalMultiplexers; sm++) {
                                        CanMessageSignalMultiplexer* SignalMultiplexer = CPgObject->GetSignalMultipexer(sm);
                                        if (SignalMultiplexer->NeedUpdate()) {
                                            UpdateOneRowSignalMultiplexer(par_View, sm, SignalMultiplexer,
                                                                          CPgObjectIndex,
                                                                          par_VisibleLeftColumn, par_VisibleRightColumn);
                                        }
                                    }
                                    // Iterate throgh all signals inside a mux object
                                    for (int s = 0; s < CPgObject->GetNumberOfSignals(); s++) {
                                        CanMessageSignal* Signal = CPgObject->GetSignal(s);
                                        if (Signal->NeedUpdate()) {
                                            UpdateOneRowSignal(NumberOfSignalMultiplexers + s, CPgObjectIndex,
                                                               par_VisibleLeftColumn, par_VisibleRightColumn);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                // Iterate throgh all object multiplexers inside a CAN message line (normaly this has zero or one entry)
                int NumberOfObjectMultipexers = RawDataLine->GetNumberOfObjectMultipexers();
                for (int om = 0; om < NumberOfObjectMultipexers; om++) {
                    QModelIndex ObjectMultiplexerIndex = index(om, 0, RawLineIndex);
                    CanMessageMultiplexer* ObjectMultiplexer = RawDataLine->GetObjectMultipexer(om);
                    if (ObjectMultiplexer->NeedUpdate() && par_View->isExpanded(ObjectMultiplexerIndex)) {
                        // Iterate throgh all object multiplexers inside a object multiplexers
                        for (int o = 0; o < RawDataLine->childCount(); o++) {
                            QModelIndex MuxObjectIndex = index(o, 0, ObjectMultiplexerIndex);
                            CanMessageMuxObject* MuxObject = ObjectMultiplexer->GetMuxObject(o);
                            if (MuxObject->NeedUpdate()) {
                                UpdateOneRowCounterTime(om, ObjectMultiplexerIndex,
                                                        par_VisibleLeftColumn, par_VisibleRightColumn);
                                if ((MuxObject != nullptr) && par_View->isExpanded(MuxObjectIndex)) {
                                    // Iterate throgh all signal multiplexers inside a mux object (normaly this has zero or one entry)
                                    int NumberOfSignalMultiplexers = MuxObject->GetNumberOfSignalMultiplexers();
                                    for (int sm = 0; sm < NumberOfSignalMultiplexers; sm++) {
                                        CanMessageSignalMultiplexer* SignalMultiplexer = MuxObject->GetSignalMultipexer(sm);
                                        if (SignalMultiplexer->NeedUpdate()) {
                                            UpdateOneRowSignalMultiplexer(par_View, sm, SignalMultiplexer,
                                                                          MuxObjectIndex,
                                                                          par_VisibleLeftColumn, par_VisibleRightColumn);
                                        }
                                    }
                                    // Iterate throgh all signals inside a mux object
                                    for (int s = 0; s < MuxObject->GetNumberOfSignals(); s++) {
                                        CanMessageSignal* Signal = MuxObject->GetSignal(s);
                                        if (Signal->NeedUpdate()) {
                                            UpdateOneRowSignal(NumberOfSignalMultiplexers + s, MuxObjectIndex,
                                                               par_VisibleLeftColumn, par_VisibleRightColumn);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                // Iterate throgh all signal multiplexers inside a CAN message line (normaly this has zero or one entry)
                int NumberOfSignalMultiplexers = RawDataLine->GetNumberOfSignalMultiplexers();
                for (int sm = 0; sm < NumberOfSignalMultiplexers; sm++) {
                    CanMessageSignalMultiplexer* SignalMultiplexer = RawDataLine->GetSignalMultipexer(sm);
                    if (SignalMultiplexer->NeedUpdate()) {
                        UpdateOneRowSignalMultiplexer(par_View, NumberOfObjectMultipexers + sm, SignalMultiplexer,
                                                      RawLineIndex,
                                                      par_VisibleLeftColumn, par_VisibleRightColumn);
                    }
                }
                // Iterate throgh all signals inside a CAN message line
                for (int s = 0; s < RawDataLine->GetNumberOfSignals(); s++) {
                    CanMessageSignal* Signal = RawDataLine->GetSignal(s);
                    if (Signal->NeedUpdate()) {
                        UpdateOneRowSignal(NumberOfObjectMultipexers + NumberOfSignalMultiplexers + s,
                                           RawLineIndex,
                                           par_VisibleLeftColumn, par_VisibleRightColumn);
                    }
                }
            }
        }
    }
}

void CANMessageWindowModel::DeleteAllRawMessageLines()
{
    for (int x = 0; x < m_CanMessageRowCount; x++) {
        if (m_CanMessageLines[x].DataPtr != nullptr) {
            my_free(m_CanMessageLines[x].DataPtr);
        }
    }
    m_CanMessageRowCount = 0;
    if (m_CanMessageLines != nullptr) my_free (m_CanMessageLines);
    m_CanMessageLines = nullptr;
}

void CANMessageWindowModel::ClearAll(bool par_DecodeFlag, bool par_DelCanCfgFlag)
{
    beginResetModel();
    if (par_DelCanCfgFlag) {
        if (m_CanServerConfig != nullptr) {
            my_free(m_CanServerConfig);
            m_CanServerConfig = nullptr;
        }
    }
    DeleteAllRawMessageLines();
    if (m_Root != nullptr) {
        delete m_Root;
        m_Root = new CanMessageRoot(this);
    }
    if (!par_DecodeFlag) {
        if (m_CanServerConfig != nullptr) {
            my_free(m_CanServerConfig);
            m_CanServerConfig = nullptr;
        }
    } else {
        if (m_CanServerConfig == nullptr) {
            m_CanServerConfig = GetACopyOfCurrentCanConfig();
        }
    }
    m_DecodeFlag = par_DecodeFlag;
    endResetModel();
}

CanMessageRawDataLine::CanMessageRawDataLine(CANMessageWindowModel *par_Model, int par_MessageLineNo, bool par_RawOrConverted, CanMessageElement *par_Parent) :
    CanMessageElement(par_Model, par_Parent)
{
    m_ObjectNo = - 1;
    NEW_CAN_SERVER_CONFIG *CanServerConfig = par_Model->GetCanServerConfig();
    if (CanServerConfig != nullptr) {
        m_ObjectNo = par_Model->GetObjectPos(par_MessageLineNo);
        if (m_ObjectNo == -1) {  // it was not checked before
            if (par_Model->CheckIfInsideCanConfig(par_MessageLineNo)) {
                m_ObjectNo = par_Model->GetObjectPos(par_MessageLineNo);
                if (m_ObjectNo >= 0) {
                    // now we should check if it is a muxed/normal/j1939 object
                    switch(CanServerConfig->objects[m_ObjectNo].type) {
                    case MUX_OBJECT:
                        m_Multiplexers.append(new CanMessageMultiplexer(par_Model, m_ObjectNo, par_RawOrConverted, this));
                        // fall though!
                    case NORMAL_OBJECT:
                    case J1939_OBJECT:
                        for (int s = 0; s < CanServerConfig->objects[m_ObjectNo].signal_count; s++) {
                            short *SignalsPtr = (short*)(void*)&(CanServerConfig->Symbols[CanServerConfig->objects[m_ObjectNo].SignalsArrayIdx]);
                            int SignalNo = SignalsPtr[s];
                            switch (CanServerConfig->bb_signals[SignalNo].type) {
                            case NORMAL_SIGNAL:
                                m_Signals.append(new CanMessageSignal(par_Model, m_ObjectNo, SignalNo, par_RawOrConverted, this));
                                break;
                            case MUX_SIGNAL:
                                m_SignalMultiplexers.append(new CanMessageSignalMultiplexer(par_Model, m_ObjectNo, SignalNo, par_RawOrConverted, this));
                                break;
                            default:
                                break;
                            }
                        }
                        break;
                    case J1939_22_MULTI_C_PG:
                        m_MultiCpg.append(new CanMessageMultiCPg(par_Model, m_ObjectNo, par_RawOrConverted, this));
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }
}

CanMessageRawDataLine::~CanMessageRawDataLine()
{
    foreach(CanMessageMultiplexer *p, m_Multiplexers) delete p;
    foreach(CanMessageSignalMultiplexer *p, m_SignalMultiplexers) delete p;
    foreach(CanMessageSignal *p, m_Signals) delete p;
}

void CanMessageRawDataLine::AnalyzeMessage(NEW_CAN_SERVER_CONFIG *par_CanConfig, CAN_MESSAGE_LINE *par_CanMessageLines)
{
    if (m_MultiCpg.count() >= 1) {
        // Multi C_PG can have only one mutliplexer
        m_MultiCpg.at(0)->AnalyzeMessage(par_CanConfig, par_CanMessageLines);
    } else {
        foreach(CanMessageMultiplexer *p, m_Multiplexers) p->AnalyzeMessage(par_CanConfig, &(par_CanMessageLines->CanMessage), par_CanMessageLines->DataPtr, par_CanMessageLines->LastTime);
        foreach(CanMessageSignalMultiplexer *p, m_SignalMultiplexers) p->AnalyzeMessage(par_CanConfig, &(par_CanMessageLines->CanMessage), par_CanMessageLines->DataPtr, par_CanMessageLines->LastTime);
        foreach(CanMessageSignal *p, m_Signals) p->AnalyzeMessage(par_CanConfig, &(par_CanMessageLines->CanMessage), par_CanMessageLines->DataPtr, par_CanMessageLines->LastTime);
    }
}

CanMessageMultiplexer::CanMessageMultiplexer(CANMessageWindowModel *par_Model, int par_ObjectNo, bool par_RawOrConverted, CanMessageElement *par_Parent) :
    CanMessageElement(par_Model, par_Parent)
{
    m_ObjectNo = par_ObjectNo;
    NEW_CAN_SERVER_CONFIG *CanServerConfig = par_Model->GetCanServerConfig();
    if (CanServerConfig != nullptr) {
        int ObjectNo = par_ObjectNo;
        do {
            if ( CanServerConfig->objects[ObjectNo].type == MUX_OBJECT) {
                m_MuxObjects.append(new CanMessageMuxObject(par_Model, CanServerConfig->objects[ObjectNo].Protocol.Mux.Value, ObjectNo, par_RawOrConverted, this));
            } else {
                ThrowError(1, "invalid element inside MUX object list");
            }
            ObjectNo = CanServerConfig->objects[ObjectNo].Protocol.Mux.next;
        } while (ObjectNo != par_ObjectNo);
    }
}

CanMessageMultiplexer::~CanMessageMultiplexer() {
    foreach(CanMessageMuxObject *p, m_MuxObjects) delete p;
}

void CanMessageMultiplexer::AnalyzeMessage(NEW_CAN_SERVER_CONFIG* par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double par_TimeStamp)
{
    NEW_CAN_SERVER_OBJECT *po;
    po = &(par_CanConfig->objects[m_ObjectNo]);
    m_CurrentMuxValue = GetMuxObjValue (po, par_DataPtr);
    foreach(CanMessageMuxObject *p, m_MuxObjects) {
        p->AnalyzeMessage(par_CanConfig, par_CanMessage, par_DataPtr, par_TimeStamp, m_CurrentMuxValue);
    }
    SetNeedUpdate(par_TimeStamp);
}

CanMessageSignalMultiplexer::CanMessageSignalMultiplexer(CANMessageWindowModel *par_Model, int par_ObjectNo, int par_SignalNo, bool par_RawOrConverted, CanMessageElement *par_Parent) :
    CanMessageElement(par_Model, par_Parent)
{
    m_ObjectNo = par_ObjectNo;
    m_SignalNo = par_SignalNo;
    NEW_CAN_SERVER_CONFIG *CanServerConfig = par_Model->GetCanServerConfig();
    if (CanServerConfig != nullptr) {
        for (int SignalNo = par_SignalNo; SignalNo >= 0; SignalNo = CanServerConfig->bb_signals[SignalNo].mux_equ_sb_childs) {
            if (CanServerConfig->bb_signals[SignalNo].type == MUX_SIGNAL) {
                m_MuxSignals.append(new CanMessageMuxSignal(par_Model, CanServerConfig->bb_signals[SignalNo].mux_value, par_ObjectNo, SignalNo, par_RawOrConverted, this));
            } else {
                ThrowError(1, "invalid element inside MUX signal list");
            }
        }
    }
}

void CanMessageSignalMultiplexer::AnalyzeMessage(NEW_CAN_SERVER_CONFIG *par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double par_TimeStamp)
{
    NEW_CAN_SERVER_OBJECT *po;
    NEW_CAN_SERVER_SIGNAL *ps;

    po = &(par_CanConfig->objects[m_ObjectNo]);
    ps = &(par_CanConfig->bb_signals[m_SignalNo]);
    if (((ps->mux_startbit + ps->mux_bitsize) >> 3) <= par_CanMessage->size) {
        m_CurrentMuxValue = GetMuxValueFromData (po, ps, par_DataPtr);
        foreach(CanMessageMuxSignal *p, m_MuxSignals) {
            p->AnalyzeMessage(par_CanConfig, par_CanMessage, par_DataPtr, par_TimeStamp, m_CurrentMuxValue);
        }
        SetNeedUpdate(par_TimeStamp);
    }
}

void CanMessageSignal::AnalyzeMessage(NEW_CAN_SERVER_CONFIG* par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double pat_TimeStamp)
{
    // now we should decode the signal from the message line
    m_Type = ExtractSignalFromData(par_CanConfig, m_ObjectNo, m_SignalNo, par_CanMessage, par_DataPtr, &m_Value);
    SetNeedUpdate(pat_TimeStamp);
}

QVariant CanMessageSignal::GetValueString(NEW_CAN_SERVER_CONFIG* par_CanConfig)
{
    char ValueText[64];
    QString Ret;
    if (FloatOrInt64_to_string(m_Value, m_Type, 10, ValueText, sizeof(ValueText)) == 0) {
        Ret = QString("[").append(QString(ValueText)).append("]  ");
        if (m_RawOrConverted) {
            if (ConvertSignal(par_CanConfig, m_ObjectNo, m_SignalNo) == 0) {
                if (FloatOrInt64_to_string(m_ConvertedValue, m_ConvertedType, 10, ValueText, sizeof(ValueText)) == 0) {
                    char *Unit = GET_CAN_SIG_UNIT(par_CanConfig, m_SignalNo);
                    Ret.append(QString(ValueText).append(" ").append(Unit));
                }
            } else {
                m_RawOrConverted = false;
            }
        }
    } else {
        Ret = QString("error");
    }
    return Ret;
}

CanMessageMuxObject::CanMessageMuxObject(CANMessageWindowModel *par_Model, int par_MuxValue, int par_ObjectNo, bool par_RawOrConverted, CanMessageElement *par_Parent) :
        CanMessageBaseObject(par_Model, par_ObjectNo, par_RawOrConverted, MUX_OBJECT, par_Parent)
{
    m_MuxValue = par_MuxValue;
}

void CanMessageMuxObject::AnalyzeMessage(NEW_CAN_SERVER_CONFIG *par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double par_TimeStamp, int par_MuxValue)
{
    if (par_MuxValue == m_MuxValue) {
        m_CanMessage = *par_CanMessage;
        CanMessageBaseObject::AnalyzeMessage(par_CanConfig, par_CanMessage, par_DataPtr, par_TimeStamp);
    }
}

void CanMessageMuxSignal::AnalyzeMessage(NEW_CAN_SERVER_CONFIG *par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double par_TimeStamp, int par_MuxValue)
{
    if (par_MuxValue == m_MuxValue) {
        CanMessageSignal::AnalyzeMessage(par_CanConfig, par_CanMessage, par_DataPtr, par_TimeStamp);
    }
}

CanMessageMultiCPg::CanMessageMultiCPg(CANMessageWindowModel *par_Model, int par_ObjectNo, bool par_RawOrConverted, CanMessageElement *par_Parent) :
    CanMessageElement(par_Model, par_Parent)
{
    m_ObjectNo = par_ObjectNo;
    NEW_CAN_SERVER_CONFIG *CanServerConfig = par_Model->GetCanServerConfig();
    if (CanServerConfig != nullptr) {
        uint32_t *Ids = GET_CAN_J1939_MULTI_C_PG_OBJECT(CanServerConfig, par_ObjectNo);
        for (int o = 0; o < Ids[0]; o++) {   // Ids[0] is the size of the array of the C_PG object indexes
            int ObjectNo =Ids[o + 1];
            if (CanServerConfig->objects[ObjectNo].type == J1939_22_C_PG) {
                m_CPgs.append(new CanMessageCPg(par_Model, ObjectNo, par_RawOrConverted, this));
            } else {
                ThrowError(1, "the object 0x%X must be from type C_PG", CanServerConfig->objects[ObjectNo].id);
            }
        }
    }
}

CanMessageMultiCPg::~CanMessageMultiCPg()
{
    foreach(CanMessageCPg *p, m_CPgs) delete p;
}

void CanMessageMultiCPg::AnalyzeMessage(NEW_CAN_SERVER_CONFIG *par_CanConfig, CAN_MESSAGE_LINE *par_CanMessageLines)
{
    int Pos = 0;
    int x;
    uint8_t *p;

    p = par_CanMessageLines->DataPtr;
    if (p != nullptr) {
        while((Pos < par_CanConfig->objects[m_ObjectNo].size) &&
               ((p[Pos] >> 5) > 0)) {   // TOS == 0 padding
            uint32_t TosAndServiceHeaader = (p[Pos + 0] & 0x03) << 24 | // TOS + Service header
                                            p[Pos + 1] << 16 |
                                            p[Pos + 2] << 8;
            int C_PG_Size = p[Pos + 3];

            foreach (CanMessageCPg *p, m_CPgs) p->AnalyzeMessage(par_CanConfig,
                                                                &(par_CanMessageLines->CanMessage), par_CanMessageLines->DataPtr, par_CanMessageLines->LastTime,
                                                                TosAndServiceHeaader, C_PG_Size, Pos + sizeof(TosAndServiceHeaader));
            Pos += C_PG_Size + sizeof(TosAndServiceHeaader);
        }
    }
    SetNeedUpdate(par_CanMessageLines->LastTime);
}

CanMessageCPg::CanMessageCPg(CANMessageWindowModel *par_Model, int par_ObjectNo, bool par_RawOrConverted, CanMessageElement *par_Parent) :
    CanMessageBaseObject(par_Model, par_ObjectNo, par_RawOrConverted, J1939_22_C_PG, par_Parent)
{
}

void CanMessageCPg::AnalyzeMessage(NEW_CAN_SERVER_CONFIG *par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double par_TimeStamp, int TosAndServiceHeaader, int C_PG_Size, int Pos)
{
    NEW_CAN_SERVER_OBJECT *po = &(par_CanConfig->objects[GetObjectNo()]);
    // compare C_PG header
    if ((po->id & 0x03FFFF00UL) == TosAndServiceHeaader) {
        if (C_PG_Size > po->size) C_PG_Size = po->size;
        CopyMessagePart(par_DataPtr + Pos, C_PG_Size);
        m_CanMessage.size = C_PG_Size;
        CanMessageBaseObject::AnalyzeMessage(par_CanConfig, &m_CanMessage, m_DataPtr, par_TimeStamp);
    }
}

CanMessageBaseObject::CanMessageBaseObject(CANMessageWindowModel *par_Model, int par_ObjectNo, bool par_RawOrConverted, int par_Type, CanMessageElement *par_Parent) :
    CanMessageElement(par_Model, par_Parent)
{
    m_DataPtr = nullptr;
    m_DataSize = 0;
    m_ObjectNo = par_ObjectNo;
    NEW_CAN_SERVER_CONFIG *CanServerConfig = par_Model->GetCanServerConfig();
    if (CanServerConfig != nullptr) {
        if (m_ObjectNo >= 0) {
            // now we should check if it is a muxed/normal/j1939 object
            if (CanServerConfig->objects[m_ObjectNo].type == par_Type) {
                for (int s = 0; s < CanServerConfig->objects[m_ObjectNo].signal_count; s++) {
                    short *SignalsPtr = (short*)(void*)&(CanServerConfig->Symbols[CanServerConfig->objects[m_ObjectNo].SignalsArrayIdx]);
                    int SignalNo = SignalsPtr[s];
                    switch (CanServerConfig->bb_signals[SignalNo].type) {
                    case NORMAL_SIGNAL:
                        m_Signals.append(new CanMessageSignal(par_Model, par_ObjectNo, SignalNo, par_RawOrConverted, this));
                        break;
                    case MUX_SIGNAL:
                        m_SignalMultiplexers.append(new CanMessageSignalMultiplexer(par_Model, par_ObjectNo, SignalNo, par_RawOrConverted, this));
                        break;
                    default:
                        break;
                    }
                }
            } else {
                ThrowError(1, "wrong object type %i expected %i", CanServerConfig->objects[m_ObjectNo].type, par_Type);
            }
        }
    }
}

CanMessageBaseObject::~CanMessageBaseObject()
{
    foreach(CanMessageSignalMultiplexer *p, m_SignalMultiplexers) delete p;
    foreach(CanMessageSignal *p, m_Signals) delete p;
    if (m_DataPtr != nullptr) {
        my_free(m_DataPtr);
    }
}

void CanMessageBaseObject::AnalyzeMessage(NEW_CAN_SERVER_CONFIG *par_CanConfig, CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr, double par_TimeStamp)
{
    foreach(CanMessageSignalMultiplexer *p, m_SignalMultiplexers) p->AnalyzeMessage(par_CanConfig, &m_CanMessage, m_DataPtr, par_TimeStamp);
    foreach(CanMessageSignal *p, m_Signals) p->AnalyzeMessage(par_CanConfig, &m_CanMessage, m_DataPtr, par_TimeStamp);
    SetNeedUpdate(par_TimeStamp);
}

QVariant CanMessageBaseObject::GetValueString()
{
    return MessageDataToString(&m_CanMessage, m_DataPtr);
}

void CanMessageBaseObject::CopyMessagePart(uint8_t *par_Src, int par_Size)
{
    if (m_DataSize < par_Size) {
        m_DataSize  = par_Size;
        m_DataPtr = (uint8_t*)my_realloc(m_DataPtr, m_DataSize);
    }
    if (m_DataPtr != nullptr) {
        MEMCPY(m_DataPtr, par_Src, m_DataSize);
    }

}

void CanMessageBaseObject::StoreMessage(CAN_FIFO_ELEM_HEADER *par_CanMessage, uint8_t *par_DataPtr)
{
    CopyMessagePart(par_DataPtr, par_CanMessage->size);
}

CanMessageElement::CanMessageElement(CANMessageWindowModel *par_Model, CanMessageElement *par_Parent)
{
    m_Model = par_Model;
    m_Parent = par_Parent;
    m_NeedUpdate = false;
    m_Counter = 0;
    m_TimeStamp = 0.0;
    m_DiffTime = 0.0;
    m_MinDiffTime = DBL_MAX;
    m_MaxDiffTime = 0.0;
}
