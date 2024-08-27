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


#include "BlackboardVariableModel.h"
#include <QString>
#include <QMimeData>
#include <QColor>
#include <QIcon>
#include "DragAndDrop.h"
extern "C" {
#include "Blackboard.h"
#include "ThrowError.h"
#include "MainValues.h"
#define FKTCALL_PROTOCOL
#include "RunTimeMeasurement.h"
}

BlackboardVariableModel::BlackboardVariableModel(QObject* arg_parent) : QAbstractListModel(arg_parent), m_ObserverConnection(this)
{
    m_ConnectedToObserver = false;
    QString Name = QString (":/Icons/Ghost.png");
    m_GhostIcon = QIcon(Name);
}

BlackboardVariableModel::~BlackboardVariableModel()
{
}

int BlackboardVariableModel::rowCount(const QModelIndex& arg_parent) const
{
    Q_UNUSED(arg_parent)
    return m_Elements.size();
}

QVariant BlackboardVariableModel::data(const QModelIndex& arg_index, int arg_role) const
{
    QVariant Ret = QVariant();
    BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::data")
    if(arg_index.isValid()) {
        switch(arg_role) {
            case Qt::DisplayRole:
            {
                int Row =  arg_index.row();
                if ((Row >= 0) && (Row < m_Elements.size())) {
                    if (m_Elements.at(Row).m_DataType == BB_UNKNOWN_WAIT) {
                        QString Help = m_Elements.at(Row).m_Name;
                        //Help.append("  (not existing)");
                        Ret = Help;
                    } else {
                        Ret = m_Elements.at(Row).m_Name;
                    }
                } else {
                    Ret = QString ("@Error Row out of bounds (%1)").arg(Row);
                }
                break;
            }
            case Qt::BackgroundRole:
            {
                int Row =  arg_index.row();
                if ((Row >= 0) && (Row < m_Elements.size())) {
                    if (m_Elements.at(Row).m_DataType == BB_UNKNOWN_WAIT) {
                        QColor Color = Qt::lightGray;
                        Ret = Color;
                    }
                }
                break;
            }
            case Qt::DecorationRole:
            {
                int Row =  arg_index.row();
                if ((Row >= 0) && (Row < m_Elements.size())) {
                    if (m_Elements.at(Row).m_DataType == BB_UNKNOWN_WAIT) {
                        Ret = m_GhostIcon;
                    }
                }
                break;
            }
            case Qt::UserRole + 1:
            {
                int Row =  arg_index.row();
                if ((Row >= 0) && (Row < m_Elements.size())) {
                    Ret = m_Elements.at(Row).m_DataType;
                } else {
                    Ret = -1;
                }
                break;
            }
            case Qt::UserRole + 2:
            {
                int Row =  arg_index.row();
                if ((Row >= 0) && (Row < m_Elements.size())) {
                    Ret = m_Elements.at(Row).m_ConversionType;
                } else {
                    Ret = -1;
                }
                break;
            }
            case Qt::UserRole + 3:  // Name without "  (not existing)" postfix
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

QStringList BlackboardVariableModel::mimeTypes() const
{
    QStringList loc_types;
    loc_types << "application/text";
    return loc_types;
}

QMimeData*BlackboardVariableModel::mimeData(const QModelIndexList& arg_indexes) const
{
    QMimeData *loc_mime = new QMimeData();
    DragAndDropInfos loc_infos;
    QModelIndex Index = arg_indexes.first();
    QString Name = m_Elements.at(Index.row()).m_Name;
    loc_infos.SetName(Name);
    loc_mime->setText(loc_infos.GetInfosString());
    return loc_mime;
}

Qt::DropActions BlackboardVariableModel::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags BlackboardVariableModel::flags(const QModelIndex& arg_index) const
{
    if(arg_index.isValid()) {
        return QAbstractListModel::flags(arg_index) | Qt::ItemIsDragEnabled;
    } else {
        return QAbstractListModel::flags(arg_index);
    }
}

QModelIndex BlackboardVariableModel::indexOfVariable(const QString &arg_Name)
{
    VariableElemente NewElement;
    NewElement.m_Vid = -1;
    NewElement.m_Name = arg_Name;
    int Index = m_Elements.indexOf(NewElement);
    return index(Index, 0);
}

bool BlackboardVariableModel::DeleteVariableByName(const QString &arg_Name)
{
    VariableElemente Element;
    Element.m_Vid = -1;
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

bool BlackboardVariableModel::ExistVariableWithName(const QString &arg_Name)
{
    VariableElemente Element;
    Element.m_Vid = -1;
    Element.m_Name = arg_Name;
    int Pos = m_Elements.indexOf(Element);
    if (Pos >= 0) {
        return true;
    }
    return false;
}

void BlackboardVariableModel::AddVariableByName(const QString &arg_Name)
{
    VariableElemente NewElement;
    NewElement.m_Vid = -1;
    NewElement.m_Name = arg_Name;
    int Size = m_Elements.size();
    this->beginInsertRows(QModelIndex(), Size, Size);
    m_Elements.append(NewElement);
    this->endInsertRows();
}

void BlackboardVariableModel::CopyElements(QList<VariableElemente> *par_ElementsToCopy)
{
    beginResetModel();
    for (int x = 0; x < par_ElementsToCopy->size(); x++) {
        m_Elements.append(par_ElementsToCopy->at(x));
    }
    endResetModel();
}

BlackboardVariableModel *BlackboardVariableModel::MakeASnapshotCopy()
{
    BlackboardVariableModel *Ret;

    Ret = new BlackboardVariableModel;
    Ret->CopyElements(&m_Elements);
    return Ret;
}

void BlackboardVariableModel::InsertVid(int arg_vid)
{
    BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::InsertVid")
    int Size = m_Elements.size();
    VariableElemente NewElement;
    NewElement.m_Vid = arg_vid;
    char Name[BBVARI_NAME_SIZE];
    if (GetBlackboardVariableNameAndTypes(arg_vid, Name, sizeof(Name), &NewElement.m_DataType, &NewElement.m_ConversionType) == 0) {
        NewElement.m_Name = QString(Name);
    } else {
        NewElement.m_Name = QString ("@Error _get_bbvari_name (%1)").arg(arg_vid);
    }
    this->beginInsertRows(QModelIndex(), Size, Size);
    m_Elements.append(NewElement);
    this->endInsertRows();
    END_RUNTIME_MEASSUREMENT
}

void BlackboardVariableModel::RemoveVid(int arg_vid)
{
    int Pos;
    BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::RemoveVid")
    VariableElemente Element;
    Element.m_Vid = arg_vid;
    BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::RemoveVid.indexOf")
    Pos = m_Elements.indexOf(Element);
    END_RUNTIME_MEASSUREMENT
    if (Pos >= 0) {
        BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::RemoveVid.beginRemoveRows")
        this->beginRemoveRows(QModelIndex(), Pos, Pos);
        END_RUNTIME_MEASSUREMENT
        BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::RemoveVid.removeAt")
        m_Elements.removeAt(Pos);
        END_RUNTIME_MEASSUREMENT
        BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::RemoveVid.endRemoveRows")
        this->endRemoveRows();
        END_RUNTIME_MEASSUREMENT
    }
    END_RUNTIME_MEASSUREMENT
}

void BlackboardVariableModel::ChangeDataType(int arg_vid)
{
    int Pos;
    BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::ChangeDataType")
    VariableElemente Element;
    Element.m_Vid = arg_vid;
    BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::ChangeDataType.indexOf")
    Pos = m_Elements.indexOf(Element);
    END_RUNTIME_MEASSUREMENT
    if (Pos >= 0) {
        VariableElemente ChangedElemnet = m_Elements.at(Pos);
        ChangedElemnet.m_DataType = static_cast<enum BB_DATA_TYPES>(get_bbvaritype (arg_vid));
        BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::ChangeDataType.replace")
        m_Elements.replace(Pos, ChangedElemnet);
        END_RUNTIME_MEASSUREMENT
        BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::ChangeDataType.endRemoveRows")
        QVector<int> Role;
        Role.append(Qt::BackgroundRole);
        QModelIndex Index = index(Pos);
        this->dataChanged(Index, Index, Role);
        END_RUNTIME_MEASSUREMENT
    }
    END_RUNTIME_MEASSUREMENT
}

void BlackboardVariableModel::ChangeConvertionType(int arg_vid)
{
    int Pos;
    BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::ChangeDataType")
    VariableElemente Element;
    Element.m_Vid = arg_vid;
    BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::ChangeDataType.indexOf")
    Pos = m_Elements.indexOf(Element);
    END_RUNTIME_MEASSUREMENT
    if (Pos >= 0) {
        VariableElemente ChangedElemnet = m_Elements.at(Pos);
        ChangedElemnet.m_ConversionType = static_cast<enum BB_CONV_TYPES>(get_bbvari_conversiontype(arg_vid));
        BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::ChangeDataType.replace")
        m_Elements.replace(Pos, ChangedElemnet);
        END_RUNTIME_MEASSUREMENT
        BEGIN_RUNTIME_MEASSUREMENT ("BlackboardVariableModel::ChangeDataType.endRemoveRows")
        QVector<int> Role;
        Role.append(Qt::BackgroundRole);
        QModelIndex Index = index(Pos);
        this->dataChanged(Index, Index, Role);
        END_RUNTIME_MEASSUREMENT
    }
    END_RUNTIME_MEASSUREMENT
}

void BlackboardVariableModel::blackboardVariableConfigChanged(int vid, unsigned int flags)
{
    if (vid > 0) {
        if ((flags & OBSERVE_ADD_VARIABLE) == OBSERVE_ADD_VARIABLE) {
            InsertVid(vid);
        } else if ((flags & OBSERVE_REMOVE_VARIABLE) == OBSERVE_REMOVE_VARIABLE) {
            RemoveVid(vid);
        }
        if ((flags & OBSERVE_TYPE_CHANGED) == OBSERVE_TYPE_CHANGED) {
            ChangeDataType(vid);
        }
        if ((flags & OBSERVE_CONVERSION_TYPE_CHANGED) == OBSERVE_CONVERSION_TYPE_CHANGED) {
            ChangeConvertionType(vid);
        }
    }
}

void BlackboardVariableModel::change_connection_to_observer(bool Connect)
{
    if (Connect) {
        if (!m_ConnectedToObserver) {
            int VidCount;
            int *Vids;
            beginResetModel();
            m_ObserverConnection.AddObserveBlackboard(OBSERVE_ADD_VARIABLE | OBSERVE_REMOVE_VARIABLE | OBSERVE_TYPE_CHANGED | OBSERVE_CONVERSION_TYPE_CHANGED,
                                                      &Vids, &VidCount);
            char Name[BBVARI_NAME_SIZE];

            if (Vids != nullptr) {
                for (int x = 0; x < VidCount; x++) {
                    VariableElemente NewElement;
                    NewElement.m_Vid = Vids[x];
                    if (GetBlackboardVariableNameAndTypes(Vids[x], Name, sizeof(Name), &NewElement.m_DataType, &NewElement.m_ConversionType) == 0) {
                        NewElement.m_Name = QString(Name);
                    } else {
                         NewElement.m_Name = QString ("@Error _get_bbvari_name (%1)").arg(Vids[x]);
                    }
                    m_Elements.append(NewElement);
                }
                free_all_bbvari_ids(Vids);
            }
            endResetModel();
            m_ConnectedToObserver = true;
        }
    } else {
        if (m_ConnectedToObserver) {
            m_ObserverConnection.AddObserveBlackboard(0);
            m_ConnectedToObserver = false;
        }
        beginResetModel();
        m_Elements.clear();
        endResetModel();
    }
}

bool VariableElemente::operator==(const VariableElemente &x) const
{
    if (x.m_Vid < 0) {  // If Vid valid compare the names
        return (m_Name.compare(x.m_Name) == 0);
    }
    return (m_Vid == x.m_Vid);
}

bool VariableElemente::operator<(const VariableElemente &x) const
{
    return (m_Name.compare(x.m_Name) < 0);
}

bool VariableElemente::operator>(const VariableElemente &x) const
{
    return (m_Name.compare(x.m_Name) > 0);
}
