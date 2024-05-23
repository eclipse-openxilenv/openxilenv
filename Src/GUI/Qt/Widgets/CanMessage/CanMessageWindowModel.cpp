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


extern "C" {
#include "MyMemory.h"
#include "MainValues.h"
#include "RemoteMasterControlProcess.h"
}

#include "CanMessageWindowModel.h"


CANMessageWindowModel::CANMessageWindowModel(double par_AbtastPeriodeInMs,
                                             QObject *parent)
   : QAbstractItemModel(parent)
{
    m_CanMessageRowCount = 0;
    m_CanMessageLines = nullptr;

    m_AbtastPeriodeInMs = par_AbtastPeriodeInMs;
}

CANMessageWindowModel::~CANMessageWindowModel()
{
    // Speicher frei geben
    Clear();
}

// return value are milliseconds
double CANMessageWindowModel::TimestampCalc (CAN_FD_FIFO_ELEM *pCanMessage)
{
    double Ts;

    if (pCanMessage->node) {  // from oneself
        // transmit messages: the timestampare are the cycle counter
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


void CANMessageWindowModel::TimestampCalcLine (CAN_MESSAGE_LINE *pcml)
{
    double Ts;

    Ts = TimestampCalc (&pcml->CanMessage);
    if (pcml->Counter) {
        pcml->dT = Ts - pcml->LastTime;
        if (pcml->dT < pcml->dTMin) pcml->dTMin = pcml->dT;
        else if (pcml->dT > pcml->dTMax) pcml->dTMax = pcml->dT;
    }
    pcml->LastTime = Ts;
}

int CmpCanMessageIds (const void *a, const void *b)
{
    const CAN_MESSAGE_LINE *ac, *bc;

    ac = static_cast<const CAN_MESSAGE_LINE*>(a);
    bc = static_cast<const CAN_MESSAGE_LINE*>(b);
    if (ac->CanMessage.channel < bc->CanMessage.channel) return -1;
    else if (ac->CanMessage.channel > bc->CanMessage.channel) return 1;
    else {   // gleicher Kannal
        if (ac->CanMessage.id < bc->CanMessage.id) return -1;
        else if (ac->CanMessage.id == bc->CanMessage.id) return 0;
        else return 1;
    }
}


void CANMessageWindowModel::AddNewCANMessages(int par_NumberOfMessages, CAN_FD_FIFO_ELEM *par_CANMessages)
{
    for (int x = 0; x < par_NumberOfMessages; x++) {
        CAN_FD_FIFO_ELEM *pCANMessage = &par_CANMessages[x];
        if (m_CanMessageLines == nullptr) {
            m_CanMessageLines = static_cast<CAN_MESSAGE_LINE*>(my_calloc (1, sizeof (CAN_MESSAGE_LINE)));
            if (m_CanMessageLines == nullptr) return;
            m_CanMessageLines->dTMin = 1000000.0;
            m_CanMessageLines->CanMessage = *pCANMessage;
            TimestampCalcLine (m_CanMessageLines);
            m_CanMessageLines->Counter++;
            m_CanMessageRowCount = 1;
        } else {
            CAN_MESSAGE_LINE *pCANMessageLine = static_cast<CAN_MESSAGE_LINE*>(bsearch (pCANMessage,
                                                                            m_CanMessageLines,
                                                                            static_cast<size_t>(m_CanMessageRowCount),
                                                                            sizeof (CAN_MESSAGE_LINE),
                                                                            CmpCanMessageIds));
            if (pCANMessageLine != nullptr) {
                pCANMessageLine->CanMessage = *pCANMessage;
                TimestampCalcLine (pCANMessageLine);
                pCANMessageLine->Counter++;
            } else {
                m_CanMessageRowCount++;
                m_CanMessageLines = static_cast<CAN_MESSAGE_LINE*>(my_realloc (m_CanMessageLines,
                                                                   static_cast<size_t>(m_CanMessageRowCount) *
                                                                   sizeof (CAN_MESSAGE_LINE)));
                memset (&(m_CanMessageLines[m_CanMessageRowCount - 1]), 0, sizeof (CAN_MESSAGE_LINE));
                m_CanMessageLines[m_CanMessageRowCount - 1].dTMin = 1000000.0;
                m_CanMessageLines[m_CanMessageRowCount - 1].CanMessage = *pCANMessage;
                TimestampCalcLine (&m_CanMessageLines[m_CanMessageRowCount - 1]);
                m_CanMessageLines[m_CanMessageRowCount - 1].Counter = 1;
                qsort(m_CanMessageLines,
                      static_cast<size_t>(m_CanMessageRowCount),
                      sizeof (CAN_MESSAGE_LINE),
                      CmpCanMessageIds);
            }
        }
    }
}


QVariant CANMessageWindowModel::data(const QModelIndex &index, int role) const
{
    int Row = index.row();
    int Column = index.column();
    if (role == Qt::DisplayRole) {
        if (Row < m_CanMessageRowCount) {
            if (Column == 0) {
                return QString().number(m_CanMessageLines[Row].Counter);
            } else if (Column == 1) {
                return QString().number(m_CanMessageLines[Row].LastTime, 'f', 3);
            } else if (Column == 2) {
                return QString().number(m_CanMessageLines[Row].dT, 'f', 3);
            } else if (Column == 3) {
                QString Ret;
                Ret.append(QString().number(m_CanMessageLines[Row].dTMin, 'f', 3));
                Ret.append("/");
                Ret.append(QString().number(m_CanMessageLines[Row].dTMax, 'f', 3));
                return Ret;
            } else if (Column == 4) {
                if (m_CanMessageLines[Row].CanMessage.node) return QString ("->");
                else return QString ("<-");
            } else if (Column == 5) {
                return QString().number(m_CanMessageLines[Row].CanMessage.channel);
            } else if (Column == 6) {
                QString Ret = QString().number(m_CanMessageLines[Row].CanMessage.id, 16);
                switch (m_CanMessageLines[Row].CanMessage.ext) {
                case 0:
                    Ret.append("n");
                    break;
                case 1:
                    Ret.append("e");
                    break;
                case 2:
                    Ret.append("bn");
                    break;
                case 3:
                    Ret.append("be");
                    break;
                case 4:
                    Ret.append("fn");
                    break;
                case 5:
                    Ret.append("fe");
                    break;
                case 6:
                    Ret.append("fbn");
                    break;
                case 7:
                    Ret.append("fbe");
                    break;
                }
                return Ret;
            } else if (Column == 7) {
                return QString().number(m_CanMessageLines[Row].CanMessage.size);
            } else if (Column == 8) {
                char DataString[64*5+1];
                DataString[0] = '\0';
                char *p = DataString;
                //CAN_FD_FIFO_ELEM *CANMessage = &m_CanMessageLines[Row].CanMessage;
                int Size = m_CanMessageLines[Row].CanMessage.size;
                if (Size > 64) Size = 64;
                unsigned char *Data = m_CanMessageLines[Row].CanMessage.data;
                for (int x = 0; x < Size; x++) {
                    p += sprintf (p, "0x%02X ", static_cast<int>(Data[x]));
                }
                return QString (DataString);
            } else {
                return QVariant();
            }
        }
    }
    return QVariant();
}

QVariant CANMessageWindowModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0) {
            return QString ("Count       ");
        } else if (section == 1) {
            return QString ("aTime(ms)    ");
        } else if (section == 2) {
            return QString ("dTime(ms)");
        } else if (section == 3) {
            return QString ("dTime(ms) min/max  ");
        } else if (section == 4) {
            return QString ("Dir");
        } else if (section == 5) {
            return QString ("Channel");
        } else if (section == 6) {
            return QString ("Identifier");
        } else if (section == 7) {
            return QString ("Size");
        } else if (section == 8) {
            return QString ("Data[0...7/63]");
        } else {
            return QVariant();
        }
    }
    return QVariant();
}

QModelIndex CANMessageWindowModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return createIndex (row, column);
}

QModelIndex CANMessageWindowModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}

int CANMessageWindowModel::rowCount(const QModelIndex &parent) const
{
//    int Columns = parent.column();
//    int Row = parent.row();
    if (!parent.isValid()) {
       return m_CanMessageRowCount;
    } else {
        return 0;
    }
}

int CANMessageWindowModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 9; //m_Columns;
}

void CANMessageWindowModel::Clear()
{
    m_CanMessageRowCount = 0;
    if (m_CanMessageLines != nullptr) my_free (m_CanMessageLines);
    m_CanMessageLines = nullptr;
}

