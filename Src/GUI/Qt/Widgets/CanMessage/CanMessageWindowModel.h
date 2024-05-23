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


#ifndef CANMESSAGEWINDOWMODEL_H
#define CANMESSAGEWINDOWMODEL_H

#include <QAbstractItemModel>
#include <QVariant>

extern "C" {
#include "CanFifo.h"
}

typedef struct CAN_MESSAGE_LINE_STRUCT {
    CAN_FD_FIFO_ELEM CanMessage;
    double LastTime;
    double dT;
    double dTMin;
    double dTMax;
    int Counter;
} CAN_MESSAGE_LINE;


class CANMessageWindowModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    CANMessageWindowModel(double par_AbtastPeriodeInMs,
                          QObject *parent = nullptr);
    ~CANMessageWindowModel() Q_DECL_OVERRIDE;

    void AddNewCANMessages (int par_NumberOfMessages, CAN_FD_FIFO_ELEM *par_CANMessages);

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    void Clear();
private:
    void TimestampCalcLine (CAN_MESSAGE_LINE *pcml);
    double TimestampCalc (CAN_FD_FIFO_ELEM *pCanMessage);

    CAN_MESSAGE_LINE *m_CanMessageLines;
    int m_CanMessageRowCount;

    double m_AbtastPeriodeInMs;
};

#endif // CANMESSAGEWINDOWMODEL_H
