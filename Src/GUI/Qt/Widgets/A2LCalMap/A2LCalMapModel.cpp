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


#include "A2LCalMapModel.h"
#include "ChangeValueTextDialog.h"
#include "QVariantConvert.h"
#include <QBrush>

A2LCalMapModel::A2LCalMapModel()
{
    m_Data = new A2LCalMapData;
}

A2LCalMapModel::~A2LCalMapModel()
{
    delete m_Data;
}

int A2LCalMapModel::GetType()
{
    return m_Data->GetType();
}

int A2LCalMapModel::GetAddress(uint64_t *ret_Address)
{
    return m_Data->GetAddress(ret_Address);
}

QString A2LCalMapModel::GetCharacteristicName()
{
    return m_Data->GetCharacteristicName();
}

QString A2LCalMapModel::GetProcess()
{
    return m_Data->GetProcess();
}

void A2LCalMapModel::SetCharacteristicName(QString &par_Characteristic)
{
    beginResetModel();
    m_Data->SetCharacteristicName(par_Characteristic);
    endResetModel();
}

void A2LCalMapModel::SetProcess(QString par_Process, void *par_CallbaclParam)
{
    m_Data->SetProcess(par_Process, par_CallbaclParam);
}

bool A2LCalMapModel::IsPhysical()
{
    return m_Data->IsPhysical();
}

void A2LCalMapModel::SetPhysical(bool par_PhysFlag)
{
    beginResetModel();
    m_Data->SetPhysical(par_PhysFlag);
    endResetModel();
}

void A2LCalMapModel::ProcessTerminated()
{
    beginResetModel();
    m_Data->ProcessTerminated();
    endResetModel();
}

void A2LCalMapModel::ProcessStarted()
{
    m_Data->ProcessStarted();
}

void A2LCalMapModel::Update()
{
    //beginResetModel();
    m_Data->UpdateReq();
    //endResetModel();
}

void A2LCalMapModel::UpdateAck(int par_LinkNo, int par_Index, void *par_Data)
{
    if ((par_LinkNo == m_Data->GetLinkNo()) && (par_Index == m_Data->GetIndex())) {
        beginResetModel();
        m_Data->UpdateAck(par_Data);
        endResetModel();
    }
}

void A2LCalMapModel::Reset()
{
    m_Data->Reset();
}

double A2LCalMapModel::GetMaxMinusMin(int axis)
{
    return  m_Data->GetMaxMinusMin(axis);
}

void A2LCalMapModel::CalcRawMinMaxValues()
{
    m_Data->CalcRawMinMaxValues();
}

void A2LCalMapModel::SetNoRecursiveSetPickMapFlag(bool par_Flag)
{
    m_Data->SetNoRecursiveSetPickMapFlag(par_Flag);
}

bool A2LCalMapModel::GetNoRecursiveSetPickMapFlag()
{
    return m_Data->GetNoRecursiveSetPickMapFlag();
}

int A2LCalMapModel::GetMapDataType()
{
    return m_Data->GetDataType(A2LCalMapData::ENUM_AXIS_TYPE_MAP, 0);  // 2 -> map, 0 -> all have the same data type
}

int A2LCalMapModel::GetMapConversionType()
{
    return m_Data->GetMapConversionType();
}

const char *A2LCalMapModel::GetMapConversion()
{
    return m_Data->GetMapConversion();
    ;
}

int A2LCalMapModel::ChangeSelectedMapValues(double NewValue, int op)
{
    int Ret = 0;
    int XDim = m_Data->GetXDim();
    int YDim = m_Data->GetYDim();
    for (int x = 0; x < XDim; x++) {
        for (int y = 0; y < YDim; y++) {
            if (m_Data->IsPicked(x, y)) {
                Ret = Ret || (m_Data->ChangeValueOP (op, NewValue, A2LCalMapData::ENUM_AXIS_TYPE_MAP, x, y) != 0);  // 2 -> Map
                ChangeData(2, x, y);   // update view of value inside table
            }
        }
    }
    m_Data->WriteDataToTargetLink(0);
    return Ret;
}

// not used !?
int A2LCalMapModel::ChangeSelectedMapValues(union FloatOrInt64 par_Value, int par_Type, int op)
{
    int Ret = 0;
    int XDim = m_Data->GetXDim();
    int YDim = m_Data->GetYDim();
    for (int x = 0; x < XDim; x++) {
        for (int y = 0; y < YDim; y++) {
            if (m_Data->IsPicked(x, y)) {
                QVariant Variant = ConvertFloatOrInt64ToQVariant(par_Value, par_Type);
                Ret = Ret || (m_Data->ChangeValueOP (op, Variant, A2LCalMapData::ENUM_AXIS_TYPE_MAP, x, y) != 0);  // 2 -> Map
                ChangeData(2, x, y);   // update view of value inside table
            }
        }
    }
    m_Data->WriteDataToTargetLink(0);
    return Ret;
}

void A2LCalMapModel::ChangeSelectionOfAll(bool par_Select)
{
    int XDim = m_Data->GetXDim();
    int YDim = m_Data->GetYDim();
    for (int y = 0; y < YDim; y++) {
        for (int x = 0; x < XDim; x++) {
            m_Data->SetPick(x, y, (par_Select) ? 1 : 2);
        }
    }
}

void A2LCalMapModel::AdjustMinMax()
{
    m_Data->AdjustMinMax();
}

int A2LCalMapModel::ChangeSelectedValuesWithDialog(QWidget *parent, int op)
{
    double RetValue = 0.0;;
    int ret;
    ChangeValueTextDialog *Dlg = nullptr;

    switch (op) {
    case '+':
        Dlg = new ChangeValueTextDialog(parent, "add to value", GetMapDataType(), IsPhysical(), GetMapConversionType(), GetMapConversion());
        break;
    case '-':
        Dlg = new ChangeValueTextDialog(parent, "subtract from value", GetMapDataType(), IsPhysical(), GetMapConversionType(), GetMapConversion());
        break;
    case '*':
        Dlg = new ChangeValueTextDialog(parent, "multiplay with value");
        break;
    case '/':
        Dlg = new ChangeValueTextDialog(parent, "divide value by");
        break;
    case 'i':
    case 'I':
        Dlg = new ChangeValueTextDialog(parent, "set value", GetMapDataType(), IsPhysical(),  GetMapConversionType(), GetMapConversion());
        {
            int XLastPick, YLastPick;
            m_Data->GetLastPick(&XLastPick, &YLastPick);
            if ((XLastPick >= 0) && (YLastPick >= 0)) {
                double Value = m_Data->GetDoubleValue(A2LCalMapData::ENUM_AXIS_TYPE_MAP, XLastPick, YLastPick);
                Dlg->SetValue(Value);
            }
        }
        break;
    case 'o':
    case 'O':
        Dlg = new ChangeValueTextDialog(parent, "set step size");
        break;
    }
    if (Dlg != nullptr) {
        if (Dlg->exec() == QDialog::Accepted) {
            RetValue = Dlg->GetDoubleValue();
            ret = 0;
        } else {
            ret = -1;
        }
        delete Dlg;
    } else ret = -1;

    if (ret == 0) {
        ret = ChangeSelectedMapValues (RetValue, op);
    }
    return ret;

}

QVariant A2LCalMapModel::data(const QModelIndex &index, int role) const
{
    int Row = index.row();
    int Column = index.column();

    switch (role) {
    case Qt::DisplayRole:
        if (Row == 0) {
            if (Column == 0) {
                if (m_Data->IsPhysical()) {
                    QString Phys("phys");
                    return Phys;
                } else {
                    QString Raw("raw");
                    return Raw;
                }
            } else {
                // X axis
                if (Column <= m_Data->GetXDim()) {
                    return m_Data->GetValueString(0, Column-1, 0);
                }
            }
        } else {
            if (Column == 0) {
                // Y axis
                if (Row <= m_Data->GetYDim()) {
                    return m_Data->GetValueString(1, Row-1, 0);
                }
            } else {
                // Map
                if ((Row <= m_Data->GetYDim()) && (Column <= m_Data->GetXDim())) {
                    return m_Data->GetValueString(2, Column-1, Row-1);
                }
            }
        }
        break;
    case Qt::BackgroundRole:
        if ((Row == 0) || (Column == 0)) {
            QBrush Background(Qt::lightGray);
            return Background;
        }
        break;
    case Qt::TextAlignmentRole:
        if ((Row != 0) || (Column != 0)) {
            return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
        }
        break;
    case Qt::UserRole + 1:  // Double value
        if (Row == 0) {
            if (Column == 0) {
                QVariant Value(0.0);
                return Value;
            } else {
                // X axis
                if (Column <= m_Data->GetXDim()) {
                    A2L_SINGLE_VALUE *Value = m_Data->GetValue(0, Column-1, 0);
                    if (Value == nullptr) return QVariant();
                    QVariant RetValue (ConvertRawValueToDouble(Value));
                    return RetValue;
                }
            }
        } else {
            if (Column == 0) {
                // Y axis
                if (Row <= m_Data->GetYDim()) {
                    A2L_SINGLE_VALUE *Value = m_Data->GetValue(1, Row-1, 0);
                    if (Value == nullptr) return QVariant();
                    QVariant RetValue (ConvertRawValueToDouble(Value));
                    return RetValue;
                }
            } else {
                // Map
                int XDim = m_Data->GetXDim();
                int YDim = m_Data->GetYDim();
                if ((Row <= YDim) && (Column <= XDim)) {
                    A2L_SINGLE_VALUE *Value = m_Data->GetValue(2, Column-1, Row-1);
                    if (Value == nullptr) return QVariant();
                    QVariant RetValue (ConvertRawValueToDouble(Value));
                    return RetValue;
                }
            }
        }
        break;

    default:
        break;
    }

    return QVariant();
}

bool A2LCalMapModel::setData(const QModelIndex &par_index, const QVariant &par_value, int par_role)
{
    int Row = par_index.row();
    int Column = par_index.column();

    switch (par_role) {
    case Qt::UserRole + 1:  // Double Value
        if (Row == 0) {
            if (Column == 0) {
                bool Ok;
                int help = par_value.toInt(&Ok);
                if (Ok) {
                    bool Changed = m_Data->IsPhysical() != (help != 0);
                    if (Changed) {
                        //beginResetModel();
                        m_Data->SetPhysical(help != 0);
                        //endResetModel();
                        QModelIndex TopLeft = index(0, 0);
                        QModelIndex BottomRight = index(this->rowCount()-1, this->columnCount()-1);
                        emit dataChanged(TopLeft, BottomRight);
                        emit PhysRawChangedFromTable(m_Data->IsPhysical());
                    }
                }
            } else {
                // X axis
                if (Column <= m_Data->GetXDim()) {
                    bool Ok;
                    double help = par_value.toDouble(&Ok);
                    if (Ok) {
                        if (m_Data->ChangeValueOP ('I', help, A2LCalMapData::ENUM_AXIS_TYPE_X_AXIS, Column-1, 0) == 0) {  // 0 -> X-Achse
                            m_Data->WriteDataToTargetLink(0);
                            //emit dataChangedFromTable(0, Column-1, 0, help);
                        }
                    }
                }
            }
        } else {
            if (Column == 0) {
                // Y axis
                if (Row <= m_Data->GetYDim()) {
                    bool Ok;
                    double help = par_value.toDouble(&Ok);
                    if (Ok) {
                        if (m_Data->ChangeValueOP ('I', help, A2LCalMapData::ENUM_AXIS_TYPE_Y_AXIS, Row-1, 0) == 0) {  // 1 -> Y-Achse
                            m_Data->WriteDataToTargetLink(0);
                            //emit dataChangedFromTable(1, Row-1, 0, help);
                        }
                    }
                }
            } else {
                // Map
                if ((Row <= m_Data->GetYDim()) && (Column <= m_Data->GetXDim())) {
                    bool Ok;
                    double help = par_value.toDouble(&Ok);
                    if (Ok) {
                        if (m_Data->ChangeValueOP ('I', help, A2LCalMapData::ENUM_AXIS_TYPE_MAP, Column-1, Row-1) == 0) {  // 2 -> Map
                            m_Data->WriteDataToTargetLink(0);
                            //emit dataChangedFromTable(2, Column-1, Row-1, help);
                        }
                    }
                }
            }
        }
        break;

    default:
        break;
    }

    return TRUE;
}


Qt::ItemFlags A2LCalMapModel::flags(const QModelIndex &index) const
{
    int Row = index.row();
    int Column = index.column();

    if (Row == 0) {
        if (Column != 0) {
            if (m_Data->XAxisNotCalibratable()) {
                return Qt::ItemIsEnabled;
            }
        } else {
            return  Qt::ItemIsEditable | Qt::ItemIsEnabled;  // (0,0) Element is the Raw/Phys switch
        }
    } else {
        if (Column == 0) {
            if (m_Data->XAxisNotCalibratable()) {
                return Qt::ItemIsEnabled;
            }
        }
    }

    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

void A2LCalMapModel::BeginUpdateData()
{
    this->beginResetModel();
}

void A2LCalMapModel::EndUpdateData()
{
    this->endResetModel();
}

void A2LCalMapModel::ChangeData(int axis, int x, int y)
{
    int Row, Column;
    switch (axis) {
    case 0: // X axis
        Row = 0;
        Column = x;
        break;
    case 1: // Y axis
        Row = y;
        Column = 0;
        break;
    case 2: // Map
        Row = y + 1;
        Column = x + 1;
        break;
    default:
        return;
    }
    QModelIndex Index = index(Row, Column);
    emit dataChanged(Index, Index);
}

A2LCalMapData *A2LCalMapModel::GetInternalDataStruct() const
{
    return m_Data;
}

void A2LCalMapModel::NotifyDataChanged(int par_LinkNo, int par_Index, A2L_DATA *par_Data)
{
    Q_UNUSED(par_LinkNo);
    Q_UNUSED(par_Index);
    Q_UNUSED(par_Data);
    // TODO:
}

QVariant A2LCalMapModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);
    return QVariant();
}

int A2LCalMapModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    int Rows =  m_Data->GetYDim() + 1; // +1 the first row are the header and is not ediable
    return Rows;
}

int A2LCalMapModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    int Columns = m_Data->GetXDim() + 1; // +1 the first column are the header and is not ediable
    return Columns;
}


QModelIndex A2LCalMapModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return createIndex (row, column);
}

QModelIndex A2LCalMapModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return QModelIndex();
}
