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


#include "A2LCalSingleModel.h"
#include "StringHelpers.h"

#include <QBrush>

extern "C" {
#include "MyMemory.h"
#include "EquationParser.h"
#include "TextReplace.h"
#include "BlackboardConvertFromTo.h"
#include "ThrowError.h"
#define FKTCALL_PROTOCOL
#include "RunTimeMeasurement.h"
#include "A2LLinkThread.h"
}
#include "A2LCalWidgetSync.h"

extern FILE *Debug_fh;
FILE *Debug_fh;

A2LCalSingleModel::A2LCalSingleModel(QObject* arg_parent) : QAbstractTableModel(arg_parent)
{
    //if (Debug_fh == nullptr) Debug_fh = fopen("debug.txt", "wt");
    m_LinkNo = -1;
    m_DataFromLinkChannelNo = -1;
    m_FontMetrics = nullptr;
    m_nameMaxPixelWidth = 0;
    m_valueMaxPixelWidth = 0;
    m_unitMaxPixelWidth = 0;
    m_BackgroundColor = QColor(Qt::white);
    m_ColumnAlignments[0] = Qt::AlignVCenter | Qt::AlignLeft;
    m_ColumnAlignments[1] = Qt::AlignVCenter | Qt::AlignRight;
    m_ColumnAlignments[2] = Qt::AlignVCenter | Qt::AlignHCenter;
    m_ColumnAlignments[3] = Qt::AlignVCenter | Qt::AlignLeft;
}

A2LCalSingleModel::~A2LCalSingleModel()
{
    if (m_FontMetrics == nullptr) {
        delete (m_FontMetrics);
    }
    foreach(A2LCalSingleData *Element, m_listOfElements) {
        delete(Element);
    }
    m_listOfElements.clear();
    if (m_DataFromLinkChannelNo >= 1){
        DeleteDataFromLinkChannel(m_DataFromLinkChannelNo);
        m_DataFromLinkChannelNo = -1;
    }
}

void A2LCalSingleModel::SetProcessName(QString &par_ProcessName, void *par_Parent)
{
    m_ProcessName = par_ProcessName;
    SetupLinkTo(par_Parent);
}

QString A2LCalSingleModel::GetProcessName()
{
    return m_ProcessName;
}

int A2LCalSingleModel::GetLinkNo()
{
    return m_LinkNo;
}

void A2LCalSingleModel::ProcessTerminated()
{
    beginResetModel();
    foreach(A2LCalSingleData *loc_variable, m_listOfElements) {
        loc_variable->SetToNotExiting();
    }
    endResetModel();
}

void A2LCalSingleModel::ProcessStarted(void *par_Parent)
{
    beginResetModel();
    if (Debug_fh != nullptr) {fprintf(Debug_fh, "ProcessStarted()\n"); fflush(Debug_fh);}
    SetupLinkTo(par_Parent);
    foreach(A2LCalSingleData *loc_variable, m_listOfElements) {
        loc_variable->SetToExiting();
    }
    endResetModel();
}

void A2LCalSingleModel::NotifyDataChanged(int par_LinkNo, int par_Index, A2L_DATA *par_Data)
{
    int Size = m_listOfElements.size();
    for(int Row = 0; Row < Size; Row++) {
        A2LCalSingleData *Element = m_listOfElements.at(Row);
        if (Element->NotifyCheckDataChanged(par_LinkNo, par_Index, par_Data)) {
            UpdateOneValueReq(Row, true);
            QModelIndex Index = index(Row, 1);
            emit dataChanged(Index, Index);
        }
    }
}

int A2LCalSingleModel::rowCount(const QModelIndex& arg_parent) const
{
    Q_UNUSED(arg_parent);
    return m_listOfElements.size();
}

int A2LCalSingleModel::columnCount(const QModelIndex& arg_parent) const
{
    Q_UNUSED(arg_parent);
    return 4;
}

QVariant A2LCalSingleModel::data(const QModelIndex& arg_index, int arg_role) const
{
    QVariant loc_ret;
    BEGIN_RUNTIME_MEASSUREMENT ("A2LCalSingleModel::data")
    if(!arg_index.isValid()) {
        loc_ret = QVariant();
    } else if(arg_index.row() >= m_listOfElements.size() || arg_index.row() < 0) {
        loc_ret = QVariant();
    } else {
        if(arg_role == Qt::DisplayRole) {
            A2LCalSingleData *loc_variable = m_listOfElements.value(arg_index.row());
            switch (arg_index.column()) {
                case 0:
                    loc_ret = loc_variable->GetCharacteristicName();
                    break;
                case 1:
                    loc_ret = loc_variable->GetValueStr();
                    break;
                case 2:
                    loc_ret = loc_variable->GetUnit();
                    break;
                case 3:
                    switch (loc_variable->GetDisplayType()) {
                        case 0:
                            loc_ret = "[dec]";
                            break;
                        case 1:
                            loc_ret = "[hex]";
                            break;
                        case 2:
                            loc_ret = "[bin]";
                            break;
                        case 3:
                            loc_ret = "[phys]";
                            break;
                        default:
                            loc_ret = "[dec]";
                            break;
                    }
                    break;
                default:
                    loc_ret = QVariant();
                    break;
            }
        } else if(arg_role == Qt::BackgroundRole) {
        } else if(arg_role == Qt::TextAlignmentRole) {
            if (arg_index.column() < 4) {
                loc_ret = m_ColumnAlignments[arg_index.column()];
            } else {
                loc_ret = QVariant();
            }
        } else {
            loc_ret = QVariant();
        }
    }
    END_RUNTIME_MEASSUREMENT
    return loc_ret;
}

bool A2LCalSingleModel::setData(const QModelIndex& arg_index, const QVariant& arg_value, int arg_role)
{
    if(!arg_index.isValid()) {
        return false;
    }

    if(arg_role == Qt::EditRole) {
        A2LCalSingleData *loc_variable = m_listOfElements.value(arg_index.row());
        switch (arg_index.column()) {
            case 0:
                break;
            case 1:
                loc_variable->SetValue(arg_value, m_DataFromLinkChannelNo, arg_index.row());
                break;
            case 3:
                loc_variable->SetDisplayType(arg_value.toInt());
                {   // additionally update display value
                    QModelIndex ValueIndex = index(arg_index.row(), 1);
                    UpdateOneValueReq(arg_index.row(), true, false);
                    emit dataChanged(ValueIndex, ValueIndex);
                }
                break;
            default:
                return false;
        }
        emit dataChanged(arg_index, arg_index);
        return true;
    } else {
        if(arg_role == Qt::BackgroundRole) {
            emit dataChanged(arg_index, arg_index);
            return true;
        } else {
            return false;
        }
    }
    // return false;
}

QVariant A2LCalSingleModel::headerData(int arg_section, Qt::Orientation arg_orientation, int arg_role) const
{
    if (arg_role == Qt::DisplayRole) {
        if (arg_orientation == Qt::Horizontal) {
            switch (arg_section) {
                case 0:
                    return QString("Name");
                case 1:
                    return QString("Value");
                case 2:
                    return QString("Unit");
                case 3:
                    return QString("DisplayType");
                default:
                    return QString();
            }
        } else {
            return QString();
        }
    } else if (arg_role == Qt::BackgroundRole) {
        QColor Color = Qt::lightGray;  // this doesn't work
        return Color;
    }
    return QVariant();
}

Qt::ItemFlags A2LCalSingleModel::flags(const QModelIndex& arg_index) const
{
    int Column = arg_index.column();
    if(!arg_index.isValid()) {
        return Qt::ItemIsEnabled;
    }
    if ((Column == 1)   ||  // Value and
        (Column == 3)) {    // display type are editable
        return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
    }
    return QAbstractTableModel::flags(arg_index);
}

bool A2LCalSingleModel::insertRows(int arg_position, int arg_rows, const QModelIndex& arg_index)
{
    Q_UNUSED(arg_index);
    beginInsertRows(QModelIndex(), arg_position, arg_position + arg_rows - 1);
    for (int loc_row = 0; loc_row < arg_rows; ++loc_row) {
        A2LCalSingleData *loc_variable = new A2LCalSingleData;
        m_listOfElements.insert(arg_position, loc_variable);
    }
    endInsertRows();
    return true;
}

bool A2LCalSingleModel::removeRows(int arg_position, int arg_rows, const QModelIndex& arg_index)
{
    Q_UNUSED(arg_index);
    if (arg_rows) {
        beginRemoveRows(QModelIndex(), arg_position, arg_position + arg_rows - 1);
        for (int loc_row = 0; loc_row < arg_rows; ++loc_row) {
            A2LCalSingleData *loc_variable = m_listOfElements.at(arg_position);
            m_listOfElements.removeAt(arg_position);
            delete loc_variable;
        }
        endRemoveRows();
        return true;
    }
    return false;
}

bool A2LCalSingleModel::moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild)
{
    bool loc_insert = false;
    if ((sourceRow == destinationChild) ||
        ((destinationChild - sourceRow) == 1)) return true;        // don't copy to themself
    A2LCalSingleData *loc_variable = m_listOfElements.at(sourceRow);
    if (destinationChild < 0) {
        destinationChild = m_listOfElements.count();  // last element
        loc_insert = true;
    }
    beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, destinationChild);
    if (loc_insert) {
        m_listOfElements.removeAt(sourceRow);
        m_listOfElements.append(loc_variable);
    } else if (sourceRow < destinationChild) {
        m_listOfElements.insert(destinationChild, loc_variable);
        m_listOfElements.removeAt(sourceRow);
    } else if (sourceRow > destinationChild) {
        m_listOfElements.removeAt(sourceRow);
        m_listOfElements.insert(destinationChild, loc_variable);
    }
    endMoveRows();
    return true;
}

QList<A2LCalSingleData *> A2LCalSingleModel::getList()
{
    return m_listOfElements;
}

A2LCalSingleData *A2LCalSingleModel::getVariable(int arg_row) const
{
    return m_listOfElements.at(arg_row);
}

void A2LCalSingleModel::setVariable(int arg_row, A2LCalSingleData *arg_variable)
{
    m_listOfElements.replace(arg_row, arg_variable);
    emit dataChanged(index(arg_row, 1), index(arg_row, 1));
}


int A2LCalSingleModel::getRowIndex(int par_Index)
{
    for(int i = 0; i < m_listOfElements.size(); i++) {
        if(m_listOfElements.at(i)->GetIndex() == par_Index) {
            return i;
        }
    }
    return -1;
}

int A2LCalSingleModel::getRowIndex(QString par_Name)
{
    for(int i = 0; i < m_listOfElements.size(); i++) {
        if(m_listOfElements.at(i)->GetCharacteristicName().compare(par_Name) == 0) {
            return i;
        }
    }

    return -1;
}


void A2LCalSingleModel::SetCharacteristicName(int arg_row, QString &par_Characteristic)
{
    A2LCalSingleData *loc_variable = m_listOfElements.value(arg_row);
    loc_variable->SetProcess(m_ProcessName);
    loc_variable->SetCharacteristicName(par_Characteristic);
    CheckExistanceOfOne(arg_row, m_LinkNo);
    emit dataChanged(index(arg_row, 0), index(arg_row, 0));    // 0 -> NameColumn
}

void A2LCalSingleModel::SetDisplayType(int par_Row, int par_DisplayType)
{
    A2LCalSingleData *loc_variable = m_listOfElements.value(par_Row);
    loc_variable->SetDisplayType(par_DisplayType);
}

void A2LCalSingleModel::setBackgroundColor(QColor &arg_Color)
{
    beginResetModel();
    m_BackgroundColor = arg_Color;
    endResetModel();
}

int A2LCalSingleModel::columnAlignment(int par_Column)
{
    if ((par_Column >= 0) && (par_Column < 4)) {
        return m_ColumnAlignments[par_Column];
    } else {
        return Qt::AlignRight | Qt::AlignVCenter;
    }
}

void A2LCalSingleModel::setColumnAlignment(int par_Column, int par_ColumnAlignments)
{
    if ((par_Column >= 0) && (par_Column < 4)) {
        m_ColumnAlignments[par_Column] = par_ColumnAlignments;
    }
}

void A2LCalSingleModel::UpdateAllValuesReq(bool arg_updateAlways, bool arg_checkUnitColumnWidth)
{
    int ElementCount =  m_listOfElements.size();
    INDEX_DATA_BLOCK *idb = GetIndexDataBlock(ElementCount);
    for(int x = 0; x < ElementCount; x++) {
        A2LCalSingleData *Variable = m_listOfElements.at(x);
        idb->Data[x].LinkNo =  Variable->GetLinkNo();
        idb->Data[x].Index = Variable->GetIndex();
        idb->Data[x].Data = Variable->GetData();
        idb->Data[x].User = (uint64_t)x | ((uint64_t)arg_updateAlways << 32)  | ((uint64_t)arg_checkUnitColumnWidth << 33);
        idb->Data[x].Flags = 0;
    }
    A2LGetDataFromLinkReq(m_DataFromLinkChannelNo, 0, idb);
}

void A2LCalSingleModel::UpdateOneValueReq(int par_Row, bool arg_updateAlways, bool arg_checkUnitColumnWidth)
{
    int size = m_listOfElements.size();

    if ((m_LinkNo > 0) && (m_DataFromLinkChannelNo >= 0)) {
        if ((par_Row >= 0) && (par_Row < size)) {
            INDEX_DATA_BLOCK *idb = GetIndexDataBlock(1);
            A2LCalSingleData *Variable = m_listOfElements.at(par_Row);
            idb->Data->LinkNo = Variable->GetLinkNo();
            idb->Data->Index = Variable->GetIndex();
            idb->Data->Data = Variable->GetData();
            idb->Data->User = (uint64_t)par_Row | ((uint64_t)arg_updateAlways << 32)  | ((uint64_t)arg_checkUnitColumnWidth << 33);
            idb->Data->Flags = 0;
            A2LGetDataFromLinkReq(m_DataFromLinkChannelNo, 0, idb);
        }
    }
}

void A2LCalSingleModel::UpdateValuesAck(void *par_IndexData)
{
    int ColumnUpdateFlags = 0;
    bool UpdateAlways;
    bool CheckUnitColumnWidth;
    int Row;
    INDEX_DATA_BLOCK *IndexData = (INDEX_DATA_BLOCK*)par_IndexData;
    int ElementCount = IndexData->Size;
    for(int x = 0; x < ElementCount; x++) {
        if(IndexData->Data[x].Data != NULL) {
            // it must be a single characteristics
            if(((A2L_DATA*)IndexData->Data[x].Data)->Type == A2L_DATA_TYPE_VALUE) {
                // the row information will not be used
                //Row = (int)IndexData->Data[x].User;
                UpdateAlways = (IndexData->Data[x].User >> 32) & 0x1;
                CheckUnitColumnWidth = (IndexData->Data[x].User >> 33) & 0x1;
                for (Row = 0; Row < m_listOfElements.count(); Row++) {
                    A2LCalSingleData *Variable = m_listOfElements.at(Row);
                    if ((IndexData->Data[x].LinkNo == Variable->GetLinkNo()) &&
                        (IndexData->Data[x].Index == Variable->GetIndex())) {
                        Variable->SetData((A2L_DATA*)IndexData->Data[x].Data);
                        if ((Variable != nullptr) && (Variable->GetIndex() >= 0)) {
                            ColumnUpdateFlags |= UpdateOneVariable(Variable, Row, UpdateAlways);
                        }
                        if (CheckUnitColumnWidth) {
                            if (calculateMinMaxColumnSize(Row, Row, CALC_PIXELWIDTH_OF_UNIT) == CALC_PIXELWIDTH_OF_UNIT) {
                                // The width of the unit column has changed
                                ColumnUpdateFlags |= CALC_PIXELWIDTH_OF_UNIT;
                            }
                        }
                    }
                }
            }
        }
    }
    if (ColumnUpdateFlags != 0) {
        emit columnWidthChanged(ColumnUpdateFlags, m_nameMaxPixelWidth, m_valueMaxPixelWidth, m_unitMaxPixelWidth);
    }
}

void A2LCalSingleModel::setFont(QFont &arg_font)
{
    m_FontMetrics = new QFontMetrics(arg_font);
    int loc_FontSize = m_FontMetrics->height();
    if ((loc_FontSize / 8) < 2) {
        m_RowHeight = loc_FontSize + loc_FontSize / 8;
    } else {
        m_RowHeight = loc_FontSize + 2;
    }
}


int A2LCalSingleModel::calculateMinMaxColumnSize(int arg_rowFirst, int arg_rowLast, int arg_columnFlags)
{
    int Ret = 0;
    if (m_FontMetrics != nullptr) {
        for (int row = arg_rowFirst; (row <= arg_rowLast) && (row < m_listOfElements.size()); row++) {
            A2LCalSingleData *loc_variable = m_listOfElements.at(row);
            if ((arg_columnFlags & CALC_PIXELWIDTH_OF_NAME) == CALC_PIXELWIDTH_OF_NAME) {
                loc_variable->CalcNamePixelWidth(m_FontMetrics);
                if (loc_variable->GetNamePixelWidth() > m_nameMaxPixelWidth) {
                    m_nameMaxPixelWidth = loc_variable->GetNamePixelWidth();
                    m_nameMaxWidthIdx = row;
                    Ret |= CALC_PIXELWIDTH_OF_NAME;
                } else {
                    if (m_nameMaxWidthIdx == row) {  // It was the longest name
                        if (loc_variable->GetNamePixelWidth() < (m_nameMaxPixelWidth - (m_nameMaxPixelWidth >> 2))) { // It the new would be became a 1/4 smaller
                            m_nameMaxPixelWidth = 0;
                            for (int x = 0; x < m_listOfElements.size(); x++) {  // Than search new longest name
                                A2LCalSingleData *loc_variable2 = m_listOfElements.at(x);
                                if (loc_variable2->GetNamePixelWidth() > m_nameMaxPixelWidth) {
                                    m_nameMaxPixelWidth = loc_variable2->GetNamePixelWidth();
                                    m_nameMaxWidthIdx = x;
                                    Ret |= CALC_PIXELWIDTH_OF_NAME;
                                }
                            }
                        }
                    }
                }
            }
            if ((arg_columnFlags & CALC_PIXELWIDTH_OF_VALUES) == CALC_PIXELWIDTH_OF_VALUES) {
                loc_variable->CalcValuePixelWidth(m_FontMetrics);
                if (loc_variable->GetValuePixelWidth() > m_valueMaxPixelWidth) {
                    m_valueMaxPixelWidth = loc_variable->GetValuePixelWidth();
                    m_valueMaxWidthIdx = row;
                    Ret |= CALC_PIXELWIDTH_OF_VALUES;
                } else {
                    if (m_valueMaxWidthIdx == row) {  // It was the longest name
                        if (loc_variable->GetValuePixelWidth() < (m_valueMaxPixelWidth - (m_valueMaxPixelWidth >> 2))) { // It the new would be became a 1/4 smaller
                            m_valueMaxPixelWidth = 0;
                            for (int x = 0; x < m_listOfElements.size(); x++) {  // Than search new longest name
                                A2LCalSingleData *loc_variable2 = m_listOfElements.at(x);
                                if (loc_variable2->GetValuePixelWidth() > m_valueMaxPixelWidth) {
                                    m_valueMaxPixelWidth = loc_variable2->GetValuePixelWidth();
                                    m_valueMaxWidthIdx = x;
                                    Ret |= CALC_PIXELWIDTH_OF_VALUES;
                                }
                            }
                        }
                    }
                }
            }
            if ((arg_columnFlags & CALC_PIXELWIDTH_OF_UNIT) == CALC_PIXELWIDTH_OF_UNIT) {
                loc_variable->CalcUnitPixelWidth(m_FontMetrics);
                if (loc_variable->GetUnitPixelWidth() > m_unitMaxPixelWidth) {
                    m_unitMaxPixelWidth = loc_variable->GetUnitPixelWidth();
                    m_unitMaxWidthIdx = row;
                    Ret |= CALC_PIXELWIDTH_OF_UNIT;
                } else {
                    if (m_unitMaxWidthIdx == row) {  // It was the longest name
                        if (loc_variable->GetUnitPixelWidth() < (m_unitMaxPixelWidth - (m_unitMaxPixelWidth >> 2))) { // It the new would be became a 1/4 smaller
                            m_unitMaxPixelWidth = 0;
                            for (int x = 0; x < m_listOfElements.size(); x++) {  // Than search new longest name
                                A2LCalSingleData *loc_variable2 = m_listOfElements.at(x);
                                if (loc_variable2->GetUnitPixelWidth() > m_unitMaxPixelWidth) {
                                    m_unitMaxPixelWidth = loc_variable2->GetUnitPixelWidth();
                                    m_unitMaxWidthIdx = x;
                                    Ret |= CALC_PIXELWIDTH_OF_UNIT;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return Ret;
}

int A2LCalSingleModel::UpdateOneVariable(A2LCalSingleData *par_Variable, int par_Row, bool par_UpdateAlways)
{
    int ColumnUpdateFlags = 0;

    if (par_Variable->ToString(par_UpdateAlways)) {
        if (calculateMinMaxColumnSize(par_Row, par_Row, CALC_PIXELWIDTH_OF_VALUES) == CALC_PIXELWIDTH_OF_VALUES) {
            // The width of the value column has changed
            ColumnUpdateFlags |= CALC_PIXELWIDTH_OF_VALUES;
        }
        QVector<int> Roles;
        Roles.append(Qt::DisplayRole);
        emit dataChanged(index(par_Row, 1), index(par_Row, 1), Roles);
    } else {
        // ???
        if (calculateMinMaxColumnSize(par_Row, par_Row, CALC_PIXELWIDTH_OF_VALUES) == CALC_PIXELWIDTH_OF_VALUES) {
            // The width of the value column has changed
            ColumnUpdateFlags |= CALC_PIXELWIDTH_OF_VALUES;
        }
        QVector<int> Roles;
        Roles.append(Qt::DisplayRole);
        emit dataChanged(index(par_Row, 1), index(par_Row, 1), Roles);
    }
    return ColumnUpdateFlags;
}

QStringList A2LCalSingleModel::getAllCharacteristicsNames()
{
    QStringList loc_variableNames;
    for(int i = 0; i < m_listOfElements.count(); ++i) {
        loc_variableNames <<  m_listOfElements.at(i)->GetCharacteristicName();
    }
    return loc_variableNames;
}

void A2LCalSingleModel::makeBackup()
{
    // first delete
    foreach(A2LCalSingleData *loc_variable, m_backupListOfElements) {
        delete loc_variable;
    }
    m_backupListOfElements.clear();
    foreach(A2LCalSingleData *loc_variable, m_listOfElements) {
        A2LCalSingleData *loc_Copy = new A2LCalSingleData();
        *loc_Copy = *loc_variable;
        m_backupListOfElements.append(loc_Copy);
    }
}

void A2LCalSingleModel::restoreBackup()
{
    beginResetModel();
    foreach(A2LCalSingleData *loc_variable, m_listOfElements) {
        delete loc_variable;
    }
    m_listOfElements.clear();
    foreach(A2LCalSingleData *loc_variable, m_backupListOfElements) {
        A2LCalSingleData *loc_Copy = new A2LCalSingleData();
        *loc_Copy = *loc_variable;
        m_listOfElements.append(loc_Copy);
    }
    endResetModel();
}

bool A2LCalSingleModel::CheckExistanceOfAll(int par_LinkNo)
{
    bool Ret = true;
    for(int x = 0; x < m_listOfElements.size(); x++) {
        Ret = Ret && CheckExistanceOfOne(x, par_LinkNo);
    }
    return Ret;
}

bool A2LCalSingleModel::CheckExistanceOfOne(int par_Index, int par_LinkNo)
{
    A2LCalSingleData *loc_variable = m_listOfElements.at(par_Index);
    return loc_variable->CheckExistance(par_LinkNo);
}

void A2LCalSingleModel::SetupLinkTo(void *par_Parent)
{
    m_LinkNo = A2LGetLinkToExternProcess(QStringToConstChar(m_ProcessName));
    if (m_DataFromLinkChannelNo >= 0) {
        DeleteDataFromLinkChannel(m_DataFromLinkChannelNo);
        m_DataFromLinkChannelNo = -1;
    }
    if (m_LinkNo > 0) {
        m_DataFromLinkChannelNo = NewDataFromLinkChannel(m_LinkNo, 100000000, GlobalNotifiyGetDataFromLinkAckCallback, par_Parent);
        CheckExistanceOfAll(m_LinkNo);
    }

}
