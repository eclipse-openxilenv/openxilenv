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


#include "A2LMeasurementCalibrationModel.h"
#include <QString>
#include <QMimeData>
#include <QColor>
#include "DragAndDrop.h"
#include "StringHelpers.h"

extern "C" {
#include "Blackboard.h"
#include "ThrowError.h"
#include "MainValues.h"
#include "Scheduler.h"
#include "A2LLink.h"
#define FKTCALL_PROTOCOL
#include "RunTimeMeasurement.h"
}

A2LMeasurementCalibrationModel::A2LMeasurementCalibrationModel(QObject* arg_parent) :
    QAbstractListModel(arg_parent)
{
}

A2LMeasurementCalibrationModel::~A2LMeasurementCalibrationModel()
{
    FreeList();
}

void A2LMeasurementCalibrationModel::SetProcessesAndTypes(QStringList &par_Processes, int par_Types, QStringList &par_Functions, int par_ReferencedFlags)
{
    beginResetModel();
    FreeList();
    foreach (QString Process, par_Processes) {
        int Pid = get_pid_by_name(QStringToConstChar(Process));
        if (Pid > 0) {
            TASK_CONTROL_BLOCK *Tcb = GetPointerToTaskControlBlock(Pid);
            if (Tcb != NULL) {
                int LinkNr = Tcb->A2LLinkNr;
                if (LinkNr > 0) {
                    char Name[1024];
                    if (par_Functions.empty()) {
                        int Idx = -1;
                        // A2L_LABEL_TYPE_MEASUREMENT
                        while ((Idx = A2LGetNextSymbolFromLink(LinkNr, Idx, par_Types, "*", Name, sizeof(Name))) >= 0) {
                            bool IsReferenced = (A2LIncDecGetReferenceToDataFromLink(LinkNr, Idx, 0, 0, 0) != 0);
                            if ((par_ReferencedFlags == 0) ||                                // list all
                                (((par_ReferencedFlags & 0x1) == 0x1) && IsReferenced) ||    // list all referenced
                                (((par_ReferencedFlags & 0x2) == 0x2) && !IsReferenced)) {    // list all referenced
                                A2LVariableElemente *Elem = new A2LVariableElemente;
                                Elem->m_Name = QString(Name);
                                Elem->m_IsReferenced = IsReferenced;
                                Elem->m_Index = Idx;
                                Elem->m_LinkNr = LinkNr;
                                Elem->m_Pid = Pid;
                                m_Elements.append(Elem);
                            }
                        }
                    } else {
                        foreach (QString Function, par_Functions) {
                            int FuncIdx = A2LGetFunctionIndexFromLink(LinkNr, QStringToConstChar(Function));
                            if (FuncIdx >= 0) {
                                int Idx = -1;
                                int Type;
                                 while ((Idx = A2LGetNextFunctionMemberFromLink(LinkNr, FuncIdx, Idx, "*", 0xFF, Name, sizeof(Name), &Type)) >= 0) {
                                     int SigIdx = A2LGetIndexFromLink(LinkNr, Name, par_Types);
                                     if (SigIdx >= 0) {
                                         bool IsReferenced = (A2LIncDecGetReferenceToDataFromLink(LinkNr, Idx, 0, 0, 0) != 0);
                                         if ((par_ReferencedFlags == 0) ||                                // list all
                                             (((par_ReferencedFlags & 0x1) == 0x1) && IsReferenced) ||    // list all referenced
                                             (((par_ReferencedFlags & 0x2) == 0x2) && !IsReferenced)) {    // list all referenced
                                             A2LVariableElemente *Elem = new A2LVariableElemente;
                                             Elem->m_Name = QString(Name);
                                             Elem->m_Index = SigIdx;
                                             Elem->m_LinkNr = LinkNr;
                                             Elem->m_Pid = Pid;
                                             m_Elements.append(Elem);
                                         }
                                     }
                                 }
                            }
                        }
                    }
                }
            }
        }
    }
    endResetModel();
}

int A2LMeasurementCalibrationModel::GetIndexPidLink(QModelIndex &par_Index, int *ret_Pid, int *ret_Link) const
{
    int x = par_Index.row();
     A2LVariableElemente *Elem = m_Elements.at(x);
     *ret_Pid = Elem->m_Pid;
     *ret_Link = Elem->m_LinkNr;
     return Elem->m_Index;
}

void A2LMeasurementCalibrationModel::ReferenceChanged(int par_Row)
{
    QVector<int> Roles;
    Roles.append(Qt::BackgroundRole);
    QModelIndex Index = createIndex(par_Row, 0);
    emit dataChanged(Index, Index, Roles);
}


int A2LMeasurementCalibrationModel::rowCount(const QModelIndex& arg_parent) const
{
    Q_UNUSED(arg_parent)
    return m_Elements.size();
}

QVariant A2LMeasurementCalibrationModel::data(const QModelIndex& arg_index, int arg_role) const
{
    QVariant Ret = QVariant();
    BEGIN_RUNTIME_MEASSUREMENT ("A2LMeasurementCalibrationModel::data")
    if(arg_index.isValid()) {
        switch(arg_role) {
            case Qt::DisplayRole:
            {
                int Row =  arg_index.row();
                if ((Row >= 0) && (Row < m_Elements.size())) {
                    Ret = m_Elements.at(Row)->m_Name;
                } else {
                    Ret = QString ("@Error Row out of bounds (%1)").arg(Row);
                }
                break;
            }
            case Qt::BackgroundRole:
            {
                int Row =  arg_index.row();
                if ((Row >= 0) && (Row < m_Elements.size())) {
                    A2LVariableElemente *Elem = m_Elements.at(Row);
                    if (A2LIncDecGetReferenceToDataFromLink(Elem->m_LinkNr, Elem->m_Index, 0, 0, 0) > 0) {
                        QColor Color = Qt::green; //lightGray;
                        Ret = Color;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    END_RUNTIME_MEASSUREMENT
    return Ret;
}

Qt::ItemFlags A2LMeasurementCalibrationModel::flags(const QModelIndex& arg_index) const
{
    if(arg_index.isValid()) {
        return QAbstractListModel::flags(arg_index) | Qt::ItemIsDragEnabled;
    } else {
        return QAbstractListModel::flags(arg_index);
    }
}

QModelIndex A2LMeasurementCalibrationModel::indexOfVariable(const QString &arg_Name)
{
    int Row;
    for (Row = 0; Row < m_Elements.size(); Row++) {
        if (arg_Name.compare(m_Elements.at(Row)->m_Name) == 0) {
            return index(Row, 0);
        }
    }
    return QModelIndex();
}

bool A2LMeasurementCalibrationModel::DeleteVariableByName(const QString &arg_Name)
{
    A2LVariableElemente Element;
    Element.m_Name = arg_Name;
    int Pos = m_Elements.indexOf(&Element);
    if (Pos >= 0) {
        this->beginRemoveRows(QModelIndex(), Pos, Pos);
        m_Elements.removeAt(Pos);
        this->endRemoveRows();
        return true;
    }
    return false;
}

bool A2LMeasurementCalibrationModel::ExistVariableWithName(const QString &arg_Name)
{
    A2LVariableElemente Element;
    Element.m_Name = arg_Name;
    int Pos = m_Elements.indexOf(&Element);
    if (Pos >= 0) {
        return true;
    }
    return false;
}

void A2LMeasurementCalibrationModel::FreeList()
{
    foreach(A2LVariableElemente *Elem, m_Elements) {
        delete Elem;
    }
    m_Elements.clear();
}

void A2LMeasurementCalibrationModel::AddVariableByName(const QString &arg_Name)
{
    A2LVariableElemente *NewElement = new A2LVariableElemente;
    NewElement->m_Name = arg_Name;
    int Size = m_Elements.size();
    this->beginInsertRows(QModelIndex(), Size, Size);
    m_Elements.append(NewElement);
    this->endInsertRows();
}

/*
void A2LMeasurementCalibrationModel::CopyElements(QList<A2LVariableElemente> *par_ElementsToCopy)
{
    beginResetModel();
    for (int x = 0; x < par_ElementsToCopy->size(); x++) {
        m_Elements.append(par_ElementsToCopy->at(x));
    }
    endResetModel();
}*/


bool A2LVariableElemente::operator==(const A2LVariableElemente &x)
{
    return (m_Name.compare(x.m_Name) == 0);
}

bool A2LVariableElemente::operator<(const A2LVariableElemente &x)
{
    return (m_Name.compare(x.m_Name) < 0);
}

bool A2LVariableElemente::operator>(const A2LVariableElemente &x)
{
    return (m_Name.compare(x.m_Name) > 0);
}

