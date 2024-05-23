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


#include "A2LFunctionModel.h"
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

A2LFunctionModel::A2LFunctionModel(QObject* arg_parent) :
    QAbstractListModel(arg_parent)
{
}

void A2LFunctionModel::InitForProcess(QStringList &par_Processes)
{
    beginResetModel();
    m_Elements.clear();
    foreach (QString Process, par_Processes) {
        int Pid = get_pid_by_name(QStringToConstChar(Process));
        if (Pid > 0) {
            TASK_CONTROL_BLOCK *Tcb = GetPointerToTaskControlBlock(Pid);
            if (Tcb != NULL) {
                int LinkNr = Tcb->A2LLinkNr;
                if (LinkNr > 0) {
                    char Name[1024];
                    int Idx = -1;
                    // A2L_LABEL_TYPE_MEASUREMENT
                    while ((Idx = A2LGetNextFunktionFromLink(LinkNr, Idx, "*", Name, sizeof(Name))) >= 0)  {
                        A2LFunctionElement Elem;
                        Elem.m_Name = QString(Name);
                        Elem.m_Index = Idx;
                        Elem.m_LinkNr = LinkNr;
                        m_Elements.append(Elem);
                    }
                }
            }
        }
    }
    endResetModel();
}

A2LFunctionModel::~A2LFunctionModel()
{
}

int A2LFunctionModel::rowCount(const QModelIndex& arg_parent) const
{
    Q_UNUSED(arg_parent)
    return m_Elements.size();
}

QVariant A2LFunctionModel::data(const QModelIndex& arg_index, int arg_role) const
{
    QVariant Ret = QVariant();
    BEGIN_RUNTIME_MEASSUREMENT ("A2LGroupModel::data")
    if(arg_index.isValid()) {
        switch(arg_role) {
            case Qt::DisplayRole:
            {
                int Row =  arg_index.row();
                if ((Row >= 0) && (Row < m_Elements.size())) {
                    Ret = m_Elements.at(Row).m_Name;
                } else {
                    Ret = QString ("@Error Row out of bounds (%1)").arg(Row);
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

Qt::ItemFlags A2LFunctionModel::flags(const QModelIndex& arg_index) const
{
    if(arg_index.isValid()) {
        return QAbstractListModel::flags(arg_index) | Qt::ItemIsDragEnabled;
    } else {
        return QAbstractListModel::flags(arg_index);
    }
}

QModelIndex A2LFunctionModel::indexOfVariable(const QString &arg_Name)
{
    A2LFunctionElement NewElement;
    NewElement.m_Name = arg_Name;
    int Index = m_Elements.indexOf(NewElement);
    return index(Index, 0);
}

bool A2LFunctionModel::DeleteVariableByName(const QString &arg_Name)
{
    A2LFunctionElement Element;
    Element.m_Name = arg_Name;
    int Pos = m_Elements.indexOf(Element);
    if (Pos >= 0) {
        this->beginRemoveRows(QModelIndex(), Pos, Pos);
        m_Elements.removeAt(Pos);
        this->endRemoveRows();
        return true;
    }
    return false;
}

bool A2LFunctionModel::ExistVariableWithName(const QString &arg_Name)
{
    A2LFunctionElement Element;
    Element.m_Name = arg_Name;
    int Pos = m_Elements.indexOf(Element);
    if (Pos >= 0) {
        return true;
    }
    return false;
}

void A2LFunctionModel::AddVariableByName(const QString &arg_Name)
{
    A2LFunctionElement NewElement;
    NewElement.m_Name = arg_Name;
    int Size = m_Elements.size();
    this->beginInsertRows(QModelIndex(), Size, Size);
    m_Elements.append(NewElement);
    this->endInsertRows();
}

void A2LFunctionModel::CopyElements(QList<A2LFunctionElement> *par_ElementsToCopy)
{
    beginResetModel();
    for (int x = 0; x < par_ElementsToCopy->size(); x++) {
        m_Elements.append(par_ElementsToCopy->at(x));
    }
    endResetModel();
}


int A2LFunctionElement::operator==(const A2LFunctionElement &x) const
{
    return (m_Name.compare(x.m_Name) == 0);
}

int A2LFunctionElement::operator<(const A2LFunctionElement &x) const
{
    return (m_Name.compare(x.m_Name) < 0);
}

int A2LFunctionElement::operator>(const A2LFunctionElement &x) const
{
    return (m_Name.compare(x.m_Name) > 0);
}

