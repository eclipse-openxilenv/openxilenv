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


#include "RemoteMasterCallStatitsticsDialog.h"
#include "ui_RemoteMasterCallStatitsticsDialog.h"

extern "C" {
#include "RemoteMasterNet.h"
}

#include "Platform.h"

RemoteMasterCallStatitsticsDialog::RemoteMasterCallStatitsticsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RemoteMasterCallStatitsticsDialog)
{
    ui->setupUi(this);
    m_Model = new RemoteMasterCallStatitsticsModel();
    ui->tableView->setModel(m_Model);
}

RemoteMasterCallStatitsticsDialog::~RemoteMasterCallStatitsticsDialog()
{
    delete ui;
}

RemoteMasterCallStatitsticsModel::RemoteMasterCallStatitsticsModel()
{
#ifdef _WIN32
    QueryPerformanceFrequency(static_cast<LARGE_INTEGER*>(static_cast<void*>(&m_Freq)));
    m_Convesion = 1000000.0 / static_cast<double>(m_Freq);   // micro sec.

    /*LARGE_INTEGER Start, End;
    QueryPerformanceCounter(&Start);
    Sleep(1000);
    QueryPerformanceCounter(&End);
    int64_t Diff = End.QuadPart - Start.QuadPart;
    double Time = Diff * m_Convesion;*/
#else
    m_Convesion = 1000.0;
#endif
}

int RemoteMasterCallStatitsticsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return RemoteMasterGetRowCount();
}

int RemoteMasterCallStatitsticsModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 6;  // CallCount, ActualTime, AccumulatedTime, AverageTime, MinTime, MaxTime
}

QVariant RemoteMasterCallStatitsticsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Vertical) {
            return QString().number(section);
        } else {
            switch (section) {
            case 0:
                return "CallCount";
            case 1:
                return "ActualTime (us)";
            case 2:
                return "AccumulatedTime (us)";
            case 3:
                return "AverageTime (us)";
            case 4:
                return "MinTime (us)";
            case 5:
                return "MaxTime (us)";
            default:
                return "unkown";
            }
        }
    }
    return QVariant();
}

QVariant RemoteMasterCallStatitsticsModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        int Row = index.row();
        int Column = index.column();
        uint64_t CallCounter;
        uint64_t LastTime;
        uint64_t AccumulatedTime;
        uint64_t MaxTime;
        uint64_t MinTime;
        if (RemoteMasterGetNextCallToStatistic(Row, &CallCounter,
                                               &LastTime,
                                               &AccumulatedTime,
                                               &MaxTime,
                                               &MinTime) >= 0) {
            if (CallCounter > 0) {
                switch (Column) {
                case 0:
                    return QString().number(CallCounter);
                case 1:
                    return QString().number(static_cast<double>(LastTime) * m_Convesion, 'f', 3);
                case 2:
                    return QString().number(static_cast<double>(AccumulatedTime) * m_Convesion, 'f', 3);
                case 3:
                    return QString().number(static_cast<double>(AccumulatedTime) * m_Convesion / CallCounter, 'f', 3);
                case 4:
                    return QString().number(static_cast<double>(MinTime) * m_Convesion, 'f', 3);
                case 5:
                    return QString().number(static_cast<double>(MaxTime) * m_Convesion, 'f', 3);
                default:
                    return "unkown";
                }
            }
        }
    }
    return QVariant();
}

void RemoteMasterCallStatitsticsDialog::on_ResetPushButton_clicked()
{
    ResetRemoteMasterCallStatistics();
}
