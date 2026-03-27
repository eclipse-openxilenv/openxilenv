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


#include "TextTableModel.h"
#include <QBrush>
#include <QPen>

extern "C" {
#include "MyMemory.h"
#include "Files.h"
#include "MainValues.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "ThrowError.h"
#define FKTCALL_PROTOCOL
#include "RunTimeMeasurement.h"
}

TextTableModel::Variable::Variable()
{
    m_type = 0;
    m_vid = -1;
    m_step = 1.0;
    m_stepType = 0;
    m_exists = false;
    m_hasConversion = false;
    m_hasOwnBackgroundColor = false;
    m_precision = 3;
    m_conversionType = 0;
    m_codedDataType = SIGNEDINT_DATATYTE;
    m_rawDoubleValue = 0.0;
    m_rawUnsigendValue = 0;
    m_rawSigendValue = 0;
    m_dataType = BB_UBYTE;
    m_rawValue.d = 0.0;
    m_hasChanged = false;
}


TextTableModel::TextTableModel(QObject* arg_parent) : QAbstractTableModel(arg_parent)
{
    m_FontMetrics = nullptr;
    m_nameMaxPixelWidth = 0;
    m_valueMaxPixelWidth = 0;
    m_unitMaxPixelWidth = 0;
    if (s_main_ini_val.DarkMode) {
        m_BackgroundColor = QColor(Qt::black);
    } else {
        m_BackgroundColor = QColor(Qt::white);
    }
    m_ColumnAlignments[0] = Qt::AlignVCenter | Qt::AlignLeft;
    m_ColumnAlignments[1] = Qt::AlignVCenter | Qt::AlignRight;
    m_ColumnAlignments[2] = Qt::AlignVCenter | Qt::AlignHCenter;
    m_ColumnAlignments[3] = Qt::AlignVCenter | Qt::AlignLeft;

    m_SizeExisting = 0;
    m_BufferSize = 0;
    m_Vids = nullptr;
    m_Types = nullptr;
    m_Values = nullptr;
    m_Pos = nullptr;
    m_Size = 0;
}

TextTableModel::~TextTableModel()
{
    if (m_FontMetrics == nullptr) {
        delete (m_FontMetrics);
    }
    if (m_Vids != nullptr) my_free(m_Vids);
    if (m_Types != nullptr) my_free(m_Types);
    if (m_Values != nullptr) my_free(m_Values);
    if (m_Pos != nullptr) my_free(m_Pos);
}

int TextTableModel::rowCount(const QModelIndex& arg_parent) const
{
    Q_UNUSED(arg_parent);
    return m_listOfElements.size();
}

int TextTableModel::columnCount(const QModelIndex& arg_parent) const
{
    Q_UNUSED(arg_parent);
    return 4;
}

QVariant TextTableModel::data(const QModelIndex& arg_index, int arg_role) const
{
    QVariant loc_ret;
    BEGIN_RUNTIME_MEASSUREMENT ("TextTableModel::data")
    if(!arg_index.isValid()) {
        loc_ret = QVariant();
    } else  if(arg_index.row() >= m_listOfElements.size() || arg_index.row() < 0) {
        loc_ret = QVariant();
    } else if(arg_role == Qt::DisplayRole) {
        Variable *loc_variable = m_listOfElements.value(arg_index.row());
        switch (arg_index.column()) {
            case 0:
                loc_ret = loc_variable->m_name;
                break;
            case 1:
                if(loc_variable->m_exists) {
                    loc_ret = loc_variable->m_value;
                } else {
                    loc_ret = QString("doesn't exist");
                }
                break;
            case 2:
                if(loc_variable->m_unit.isEmpty()) {
                    loc_ret = QString("-");
                } else {
                    loc_ret = loc_variable->m_unit;
                }
                break;
            case 3:
                switch (loc_variable->m_type) {
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
        Variable *loc_variable = m_listOfElements.value(arg_index.row());
        switch (arg_index.column()) {
            case 1:
                if (loc_variable->m_hasOwnBackgroundColor) {
                    loc_ret = QBrush(loc_variable->m_backgroundColor);
                } else {
                    loc_ret = QBrush(m_BackgroundColor);
                }
                break;
            default:
                loc_ret = QBrush(m_BackgroundColor);
                break;
        }
    } else if(arg_role == Qt::ForegroundRole) {
        Variable *loc_variable = m_listOfElements.value(arg_index.row());
        switch (arg_index.column()) {
            case 1:
                if (loc_variable->m_hasOwnBackgroundColor) {
                    int r = loc_variable->m_backgroundColor.red();
                    int g = loc_variable->m_backgroundColor.green();
                    int b = loc_variable->m_backgroundColor.blue();
                    if ((r*300 + g*588 + b*115 > (185 << 8))) {
                        loc_ret = QColor(Qt::black);
                    } else {
                        loc_ret = QColor(Qt::white);
                    }
                } else {
                    loc_ret = QVariant();
                }
                break;
            default:
                loc_ret = QVariant();
                break;
        }
    } else if(arg_role == Qt::TextAlignmentRole) {
        if (arg_index.column() < 4) {
            loc_ret = m_ColumnAlignments[arg_index.column()];
        } else {
            loc_ret = QVariant();
        }
    } else {
        loc_ret = QVariant();
    }
    END_RUNTIME_MEASSUREMENT
    return loc_ret;
}

bool TextTableModel::setData(const QModelIndex& arg_index, const QVariant& arg_value, int arg_role)
{
    if(!arg_index.isValid()) {
        return false;
    }

    if(arg_role == Qt::EditRole) {
        Variable *loc_variable = m_listOfElements.value(arg_index.row());
        switch (arg_index.column()) {
            case 0:
                loc_variable->m_name = arg_value.toString();
                break;
            case 1:
                loc_variable->m_value = arg_value.toString();
                break;
            case 2:
                loc_variable->m_type = arg_value.toInt();
                {   // Additional show the value
                    QModelIndex ValueIndex = index(arg_index.row(), 1);
                    CyclicUpdateValues(arg_index.row(), arg_index.row(), true, false);
                    emit dataChanged(ValueIndex, ValueIndex);
                }
                break;
            case 3:
                loc_variable->m_unit = arg_value.toString();
                break;
            case 4:
                loc_variable->m_vid = arg_value.toInt();
                break;
            case 5:
                loc_variable->m_step = arg_value.toDouble();
                break;
            case 6:
                loc_variable->m_stepType = arg_value.toInt();
                break;
            default:
                return false;
        }
        emit dataChanged(arg_index, arg_index);
        return true;
    } else {
        if(arg_role == Qt::BackgroundRole) {
            Variable *loc_variable = m_listOfElements.value(arg_index.row());
            loc_variable->m_backgroundColor = arg_value.value<QColor>();
            emit dataChanged(arg_index, arg_index);
            return true;
        } else {
            return false;
        }
    }
}

QVariant TextTableModel::headerData(int arg_section, Qt::Orientation arg_orientation, int arg_role) const
{
    if(arg_role == Qt::DisplayRole) {
        if(arg_orientation == Qt::Horizontal) {
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
                    break;
            }
        }
    }
    return QVariant();
}

Qt::ItemFlags TextTableModel::flags(const QModelIndex& arg_index) const
{
    int Column = arg_index.column();
    if(!arg_index.isValid()) {
        return Qt::ItemIsEnabled;
    }
    if ((Column == 1) ||    // Value and
        (Column == 3)) {    // type are editable
        Qt::ItemFlags  Ret = Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
        return Ret;
    }
    return QAbstractTableModel::flags(arg_index);
}

bool TextTableModel::insertRows(int arg_position, int arg_rows, const QModelIndex& arg_index)
{
    Q_UNUSED(arg_index);
    beginInsertRows(QModelIndex(), arg_position, arg_position + arg_rows - 1);
    for (int loc_row = 0; loc_row < arg_rows; ++loc_row) {
        Variable *loc_variable = new Variable;
        loc_variable->m_name = QString("");
        loc_variable->m_type = 0;
        loc_variable->m_unit = QString("");
        loc_variable->m_vid = 0;
        loc_variable->m_value = QString("0");
        loc_variable->m_exists = false;
        loc_variable->m_hasConversion = false;
        loc_variable->m_backgroundColor = QColor(255, 255, 255);
        loc_variable->m_step = 1.0;
        loc_variable->m_stepType = 0;
        loc_variable->m_precision = 0;
        loc_variable->m_rawValue.d = qQNaN();
        loc_variable->m_dataType = BB_UNKNOWN;
        loc_variable->m_hasChanged = true;
        m_listOfElements.insert(arg_position, loc_variable);
    }
    endInsertRows();
    SyncCopyBuffer();
    return true;
}

bool TextTableModel::removeRows(int arg_position, int arg_rows, const QModelIndex& arg_index)
{
    Q_UNUSED(arg_index);
    if (arg_rows) {
        beginRemoveRows(QModelIndex(), arg_position, arg_position + arg_rows - 1);
        for (int loc_row = 0; loc_row < arg_rows; ++loc_row) {
            Variable *loc_variable = m_listOfElements.at(arg_position);
            m_listOfElements.removeAt(arg_position);
            if (loc_variable->m_vid > 0) {
                remove_bbvari_unknown_wait(loc_variable->m_vid);
            }
            delete loc_variable;
        }
        endRemoveRows();
        SyncCopyBuffer();
        return true;
    }
    return false;
}

bool TextTableModel::moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild)
{
    bool loc_insert = false;
    if ((sourceRow == destinationChild) ||
        ((destinationChild - sourceRow) == 1)) {
        return true;        // not copy to oneself this crashes
    }
    Variable *loc_variable = m_listOfElements.at(sourceRow);
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
    SyncCopyBuffer();
    return true;
}

QList<TextTableModel::Variable*> TextTableModel::getList()
{
    return m_listOfElements;
}

TextTableModel::Variable *TextTableModel::getVariable(int arg_row) const
{
    return m_listOfElements.at(arg_row);
}

void TextTableModel::setVariable(int arg_row, TextTableModel::Variable *arg_variable)
{
    m_listOfElements.replace(arg_row, arg_variable);
    SyncCopyBuffer();
    emit dataChanged(index(arg_row, 1), index(arg_row, 1));
}

void TextTableModel::setVidValue(int arg_row, int arg_vid)
{
    Variable *loc_variable = m_listOfElements.value(arg_row);
    loc_variable->m_vid = arg_vid;
    SyncCopyBuffer();
}

void TextTableModel::setVariableExists(int arg_row, bool arg_show)
{
    Variable *loc_variable = m_listOfElements.value(arg_row);
    loc_variable->m_exists = arg_show;
    SyncCopyBuffer();
    emit dataChanged(index(arg_row, 1), index(arg_row, 1));
}

void TextTableModel::setVariableHasConversion(int arg_row, bool arg_hasConversion)
{
    Variable *loc_variable = m_listOfElements.value(arg_row);
    loc_variable->m_hasConversion = arg_hasConversion;
}

void TextTableModel::setVariablePrecision(int arg_row, int arg_precision)
{
    Variable *loc_variable = m_listOfElements.value(arg_row);
    loc_variable->m_precision = arg_precision;
}

void TextTableModel::setVariableDataType(int arg_row, enum BB_DATA_TYPES arg_dataType)
{
    Variable *loc_variable = m_listOfElements.value(arg_row);
    loc_variable->m_dataType = arg_dataType;
}

int TextTableModel::getRowIndex(int arg_vid)
{
    for(int i = 0; i < m_listOfElements.size(); i++) {
        if(m_listOfElements.at(i)->m_vid == arg_vid) {
            return i;
        }
    }
    return -1;
}

int TextTableModel::getRowIndex(QString arg_name)
{
    for(int i = 0; i < m_listOfElements.size(); i++) {
        if(m_listOfElements.at(i)->m_name.compare(arg_name) == 0) {
            return i;
        }
    }

    return -1;
}

void TextTableModel::setStep(int arg_row, double arg_step)
{
    Variable *loc_variable = m_listOfElements.value(arg_row);
    loc_variable->m_step = arg_step;
}

void TextTableModel::setStepType(int arg_row, int arg_stepType)
{
    Variable *loc_variable = m_listOfElements.value(arg_row);
    loc_variable->m_stepType = arg_stepType;
}

void TextTableModel::setDisplayType(int arg_row, int arg_displayType)
{
    Variable *loc_variable = m_listOfElements.value(arg_row);
    loc_variable->m_type = arg_displayType;
    loc_variable->m_hasChanged = true;
    emit dataChanged(index(arg_row, 1), index(arg_row, 1));
}

void TextTableModel::setConversionType(int arg_row, int arg_conversionType)
{
    Variable *loc_variable = m_listOfElements.value(arg_row);
    loc_variable->m_conversionType = arg_conversionType;
    emit dataChanged(index(arg_row, 1), index(arg_row, 1));
}

void TextTableModel::setUnit(int arg_row, QString arg_unit)
{
    Variable *loc_variable = m_listOfElements.value(arg_row);
    loc_variable->m_unit = arg_unit;
    emit dataChanged(index(arg_row, 2), index(arg_row, 2));    // 2 -> Unit column
}

void TextTableModel::setName(int arg_row, QString arg_name)
{
    Variable *loc_variable = m_listOfElements.value(arg_row);
    loc_variable->m_name = arg_name;
    emit dataChanged(index(arg_row, 0), index(arg_row, 0));    // 0 -> Name column
}

void TextTableModel::setBackgroundColor(QColor &arg_Color)
{
    beginResetModel();
    m_BackgroundColor = arg_Color;
    endResetModel();
}

int TextTableModel::columnAlignment(int par_Column)
{
    if ((par_Column >= 0) && (par_Column < 4)) {
        return m_ColumnAlignments[par_Column];
    } else {
        return Qt::AlignRight | Qt::AlignVCenter;
    }
}

void TextTableModel::setColumnAlignment(int par_Column, int par_ColumnAlignments)
{
    if ((par_Column >= 0) && (par_Column < 4)) {
        m_ColumnAlignments[par_Column] = par_ColumnAlignments;
    }
}

void TextTableModel::CyclicUpdateValues(int arg_fromRow, int arg_toRow, bool arg_updateAlways, bool arg_checkUnitColumnWidth)
{
    Q_UNUSED(arg_fromRow);
    Q_UNUSED(arg_toRow);
    Q_UNUSED(arg_checkUnitColumnWidth);
    int ColumnUpdateFlags = 0;
    int row = 0;

    BEGIN_RUNTIME_MEASSUREMENT ("TextTableModel::CyclicUpdateValues")
    if (read_bbvari_union_type_frame(m_SizeExisting, m_Vids, m_Types, m_Values) == m_SizeExisting) {
        int x;
        for (x = 0; x < m_Size; x++) {
            row = m_Pos[x];
            Variable *loc_Variable = m_listOfElements.at(row);
            ColumnUpdateFlags |= UpdateOneVariable(loc_Variable, row, arg_updateAlways, m_Values[x], m_Types[x]);
        }
    }
    if (ColumnUpdateFlags != 0) {
        emit columnWidthChanged(ColumnUpdateFlags, m_nameMaxPixelWidth, m_valueMaxPixelWidth, m_unitMaxPixelWidth);
    }
    END_RUNTIME_MEASSUREMENT
}

void TextTableModel::setFont(QFont &arg_font)
{
    m_FontMetrics = new QFontMetrics(arg_font);
    int loc_FontSize = m_FontMetrics->height();
    if ((loc_FontSize / 8) < 2) {
        m_RowHeight = loc_FontSize + loc_FontSize / 8;
    } else {
        m_RowHeight = loc_FontSize + 2;
    }
}


int TextTableModel::calculateMinMaxColumnSize(int arg_rowFirst, int arg_rowLast, int arg_columnFlags)
{
    int Ret = 0;
    if (m_FontMetrics != nullptr) {
        for (int row = arg_rowFirst; (row <= arg_rowLast) && (row < m_listOfElements.size()); row++) {
            Variable *loc_variable = m_listOfElements.at(row);
            if ((arg_columnFlags & CALC_PIXELWIDTH_OF_NAME) == CALC_PIXELWIDTH_OF_NAME) {
                loc_variable->m_namePixelWidth = m_FontMetrics->boundingRect(loc_variable->m_name).width();
                if (loc_variable->m_namePixelWidth > m_nameMaxPixelWidth) {
                    m_nameMaxPixelWidth = loc_variable->m_namePixelWidth;
                    m_nameMaxWidthIdx = row;
                    Ret |= CALC_PIXELWIDTH_OF_NAME;
                } else {
                    if (m_nameMaxWidthIdx == row) {  // It was the longest name
                        if (loc_variable->m_namePixelWidth < (m_nameMaxPixelWidth - (m_nameMaxPixelWidth >> 2))) { // It the new would be became a 1/4 smaller
                            m_nameMaxPixelWidth = 0;
                            for (int x = 0; x < m_listOfElements.size(); x++) {  // Than search new longest name
                                Variable *loc_variable2 = m_listOfElements.at(x);
                                if (loc_variable2->m_namePixelWidth > m_nameMaxPixelWidth) {
                                    m_nameMaxPixelWidth = loc_variable2->m_namePixelWidth;
                                    m_nameMaxWidthIdx = x;
                                    Ret |= CALC_PIXELWIDTH_OF_NAME;
                                }
                            }
                        }
                    }
                }
            }
            if ((arg_columnFlags & CALC_PIXELWIDTH_OF_VALUES) == CALC_PIXELWIDTH_OF_VALUES) {
                loc_variable->m_valuePixelWidth = m_FontMetrics->boundingRect(loc_variable->m_value).width();
                if (loc_variable->m_valuePixelWidth > m_valueMaxPixelWidth) {
                    m_valueMaxPixelWidth = loc_variable->m_valuePixelWidth;
                    m_valueMaxWidthIdx = row;
                    Ret |= CALC_PIXELWIDTH_OF_VALUES;
                } else {
                    if (m_valueMaxWidthIdx == row) {  // It was the longest name
                        if (loc_variable->m_valuePixelWidth < (m_valueMaxPixelWidth - (m_valueMaxPixelWidth >> 2))) { // It the new would be became a 1/4 smaller
                            m_valueMaxPixelWidth = 0;
                            for (int x = 0; x < m_listOfElements.size(); x++) {  // Than search new longest name
                                Variable *loc_variable2 = m_listOfElements.at(x);
                                if (loc_variable2->m_valuePixelWidth > m_valueMaxPixelWidth) {
                                    m_valueMaxPixelWidth = loc_variable2->m_valuePixelWidth;
                                    m_valueMaxWidthIdx = x;
                                    Ret |= CALC_PIXELWIDTH_OF_VALUES;
                                }
                            }
                        }
                    }
                }
            }
            if ((arg_columnFlags & CALC_PIXELWIDTH_OF_UNIT) == CALC_PIXELWIDTH_OF_UNIT) {
                loc_variable->m_unitPixelWidth = m_FontMetrics->boundingRect(loc_variable->m_unit).width();
                if (loc_variable->m_unitPixelWidth > m_unitMaxPixelWidth) {
                    m_unitMaxPixelWidth = loc_variable->m_unitPixelWidth;
                    m_unitMaxWidthIdx = row;
                    Ret |= CALC_PIXELWIDTH_OF_UNIT;
                } else {
                    if (m_unitMaxWidthIdx == row) {  //  It was the longest name
                        if (loc_variable->m_unitPixelWidth < (m_unitMaxPixelWidth - (m_unitMaxPixelWidth >> 2))) { // It the new would be became a 1/4 smaller
                            m_unitMaxPixelWidth = 0;
                            for (int x = 0; x < m_listOfElements.size(); x++) {  // Than search new longest name
                                Variable *loc_variable2 = m_listOfElements.at(x);
                                if (loc_variable2->m_unitPixelWidth > m_unitMaxPixelWidth) {
                                    m_unitMaxPixelWidth = loc_variable2->m_unitPixelWidth;
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

void TextTableModel::SyncCopyBuffer()
{
    m_Size = m_listOfElements.size();
    if (m_Size > (int)m_BufferSize) {
        m_BufferSize = m_Size + 16;
        m_Vids = static_cast<int*>(my_realloc(m_Vids, m_BufferSize * sizeof(int32_t)));
        m_Types = static_cast<enum BB_DATA_TYPES*>(my_realloc(m_Types, m_BufferSize * sizeof(enum BB_DATA_TYPES)));
        m_Values = static_cast<union BB_VARI*>(my_realloc(m_Values, m_BufferSize * sizeof(union BB_VARI)));
        m_Pos = static_cast<int*>(my_realloc(m_Pos, m_BufferSize * sizeof(int32_t)));
    }
    // sorting first all existing than all none existing variables
    int x, y = 0;
    int z = 1;
    for (x = 0; x < m_Size; x++) {
        Variable *loc_Variable = m_listOfElements.at(x);
        if (loc_Variable->m_exists) {
            m_Vids[y] = loc_Variable->m_vid;
            m_Pos[y] = x;
            y++;
        } else {
            m_Vids[m_Size - z] = loc_Variable->m_vid;
            m_Pos[m_Size - z] = x;
            z++;
        }
    }
    m_SizeExisting = y;
}

static int CompareDoubleEqual(double a, double b)
{
    double diff = a - b;
    if ((diff <= 0.0) && (diff >= 0.0)) return 1;
    else return 0;
}

static int CompareFloatEqual(float a, float b)
{
    float diff = a - b;
    if ((diff <= 0.0f) && (diff >= 0.0f)) return 1;
    else return 0;
}

int TextTableModel::UpdateOneVariable(TextTableModel::Variable *loc_Variable, int row, bool arg_updateAlways,
                                      union BB_VARI loc_rawValue, enum BB_DATA_TYPES loc_dataType)
{
    int ColumnUpdateFlags = 0;

    if (loc_Variable->m_exists) {
        bool loc_hasChanged = false;
        switch (loc_dataType) {
        case BB_BYTE:
            loc_hasChanged = loc_Variable->m_rawValue.b != loc_rawValue.b;
            loc_Variable->m_rawDoubleValue = loc_Variable->m_rawSigendValue = loc_Variable->m_rawValue.b = loc_rawValue.b;
            loc_Variable->m_codedDataType = Variable::SIGNEDINT_DATATYTE;
            break;
        case BB_UBYTE:
            loc_hasChanged = loc_Variable->m_rawValue.ub != loc_rawValue.ub;
            loc_Variable->m_rawDoubleValue = loc_Variable->m_rawUnsigendValue = loc_Variable->m_rawValue.ub =loc_rawValue.ub;
            loc_Variable->m_codedDataType = Variable::UNSIGNEDINT_DATATYTE;
            break;
        case BB_WORD:
            loc_hasChanged = loc_Variable->m_rawValue.w != loc_rawValue.w;
            loc_Variable->m_rawDoubleValue = loc_Variable->m_rawSigendValue = loc_Variable->m_rawValue.w = loc_rawValue.w;
            loc_Variable->m_codedDataType = Variable::SIGNEDINT_DATATYTE;
            break;
        case BB_UWORD:
            loc_hasChanged = loc_Variable->m_rawValue.uw != loc_rawValue.uw;
            loc_Variable->m_rawDoubleValue = loc_Variable->m_rawUnsigendValue = loc_Variable->m_rawValue.uw = loc_rawValue.uw;
            loc_Variable->m_codedDataType = Variable::UNSIGNEDINT_DATATYTE;
            break;
        case BB_DWORD:
            loc_hasChanged = loc_Variable->m_rawValue.dw != loc_rawValue.dw;
            loc_Variable->m_rawDoubleValue = loc_Variable->m_rawSigendValue = loc_Variable->m_rawValue.dw = loc_rawValue.dw;
            loc_Variable->m_codedDataType = Variable::SIGNEDINT_DATATYTE;
            break;
        case BB_UDWORD:
            loc_hasChanged = loc_Variable->m_rawValue.udw != loc_rawValue.udw;
            loc_Variable->m_rawDoubleValue = loc_Variable->m_rawUnsigendValue = loc_Variable->m_rawValue.udw = loc_rawValue.udw;
            loc_Variable->m_codedDataType = Variable::UNSIGNEDINT_DATATYTE;
            break;
        case BB_QWORD:
            loc_hasChanged = loc_Variable->m_rawValue.qw != loc_rawValue.qw;
            loc_Variable->m_rawDoubleValue = loc_Variable->m_rawSigendValue = loc_Variable->m_rawValue.qw = loc_rawValue.qw;
            loc_Variable->m_codedDataType = Variable::SIGNEDINT_DATATYTE;
            break;
        case BB_UQWORD:
            loc_hasChanged = loc_Variable->m_rawValue.uqw != loc_rawValue.uqw;
            loc_Variable->m_rawDoubleValue = loc_Variable->m_rawUnsigendValue = loc_Variable->m_rawValue.uqw = loc_rawValue.uqw;
            loc_Variable->m_codedDataType = Variable::UNSIGNEDINT_DATATYTE;
            break;
        case BB_FLOAT:
            loc_hasChanged = !CompareFloatEqual(loc_Variable->m_rawValue.f, loc_rawValue.f);
            loc_Variable->m_rawDoubleValue = static_cast<double>(loc_Variable->m_rawValue.f = loc_rawValue.f);
            loc_Variable->m_codedDataType = Variable::FLOAT_DATATYTE;
            break;
        case BB_DOUBLE:
            loc_hasChanged = !CompareDoubleEqual(loc_Variable->m_rawValue.d, loc_rawValue.d);
            loc_Variable->m_rawDoubleValue = loc_Variable->m_rawValue.d = loc_rawValue.d;
            loc_Variable->m_codedDataType = Variable::FLOAT_DATATYTE;
            break;
        default:
            loc_Variable->m_codedDataType = Variable::ERROR_DATATYPE;
            break;
        }
        loc_Variable->m_hasChanged = loc_Variable->m_hasChanged || loc_hasChanged;
        if (arg_updateAlways ||
            loc_Variable->m_hasChanged  ||
            (loc_dataType != loc_Variable->m_dataType)) {
            loc_Variable->m_dataType = loc_dataType;
            loc_Variable->m_hasChanged = false;
            switch(loc_Variable->m_type) {
            case 0:  // Raw
                switch (loc_Variable->m_codedDataType) {
                case Variable::FLOAT_DATATYTE:
                    loc_Variable->m_value = QString().number(loc_Variable->m_rawDoubleValue, 'f', loc_Variable->m_precision);
                    break;
                case Variable::UNSIGNEDINT_DATATYTE:
                    loc_Variable->m_value = QString().number(loc_Variable->m_rawUnsigendValue, 10);
                    break;
                case Variable::SIGNEDINT_DATATYTE:
                    loc_Variable->m_value = QString().number(loc_Variable->m_rawSigendValue, 10);
                    break;
                case Variable::ERROR_DATATYPE:
                //default:
                    loc_Variable->m_value = QString("data type error");
                    break;
                }
                break;
            case 1:  // Hex
                switch (loc_Variable->m_codedDataType) {
                case Variable::FLOAT_DATATYTE:
                    // todo Float as HEX?!
                    loc_Variable->m_value = QString().number(loc_Variable->m_rawDoubleValue, 'f', loc_Variable->m_precision);
                    break;
                case Variable::UNSIGNEDINT_DATATYTE:
                    loc_Variable->m_value = QString("0x");
                    loc_Variable->m_value.append(QString().number(loc_Variable->m_rawUnsigendValue, 16));
                    break;
                case Variable::SIGNEDINT_DATATYTE:
                    if (loc_Variable->m_rawSigendValue >= 0) {
                        loc_Variable->m_value = QString("0x");
                        loc_Variable->m_value.append(QString().number(loc_Variable->m_rawSigendValue, 16));
                    } else {
                        loc_Variable->m_value = QString("-0x");
                        loc_Variable->m_value.append(QString().number(-loc_Variable->m_rawSigendValue, 16));
                    }
                    break;
                case Variable::ERROR_DATATYPE:
                //default:
                    loc_Variable->m_value = QString("data type error");
                    break;
                }
                break;
            case 2: // Binary
                switch (loc_Variable->m_codedDataType) {
                case Variable::FLOAT_DATATYTE:
                    // todo Float als Bin?!
                    loc_Variable->m_value = QString().number(loc_Variable->m_rawDoubleValue, 'f', loc_Variable->m_precision);
                    break;
                case Variable::UNSIGNEDINT_DATATYTE:
                    loc_Variable->m_value = QString("0b");
                    loc_Variable->m_value.append(QString().number(loc_Variable->m_rawUnsigendValue, 2));
                    break;
                case Variable::SIGNEDINT_DATATYTE:
                    if (loc_Variable->m_rawSigendValue >= 0) {
                        loc_Variable->m_value = QString("0b");
                        loc_Variable->m_value.append(QString().number(loc_Variable->m_rawSigendValue, 2));
                    } else {
                        loc_Variable->m_value = QString("-0b");
                        loc_Variable->m_value.append(QString().number(-loc_Variable->m_rawSigendValue, 2));
                    }
                    break;
                case Variable::ERROR_DATATYPE:
                //default:
                    loc_Variable->m_value = QString("data type error");
                    break;
                }
                break;
            case 3: // Phys
                switch(loc_Variable->m_conversionType = get_bbvari_conversiontype(loc_Variable->m_vid)) {  // todo: das sollte auch nur vom Observer uerbnommen werden
                    case BB_CONV_FORMULA:
                    case BB_CONV_FACTOFF:
                    case BB_CONV_OFFFACT:
                    case BB_CONV_TAB_INTP:
                    case BB_CONV_TAB_NOINTP:
                    case BB_CONV_RAT_FUNC:
                        {
                            double loc_physValue;
                            if (get_phys_value_for_raw_value (loc_Variable->m_vid, loc_Variable->m_rawDoubleValue, &loc_physValue)) {
                                loc_Variable->m_value = QString ("Conversion error");
                            } else {
                                loc_Variable->m_value = QString().number(loc_physValue, 'f', loc_Variable->m_precision);
                            }
                            loc_Variable->m_hasOwnBackgroundColor = false;
                        }
                        break;
                    case BB_CONV_TEXTREP:
                        {
                            char loc_textReplace[BBVARI_ENUM_NAME_SIZE];
                            int loc_color;
                            convert_value_textreplace (loc_Variable->m_vid, static_cast<int32_t>(loc_Variable->m_rawDoubleValue), loc_textReplace, sizeof(loc_textReplace), &loc_color);
                            loc_Variable->m_value = QString(loc_textReplace);
                            loc_Variable->m_backgroundColor = QColor(loc_color&0x000000FF, (loc_color&0x0000FF00)>>8, (loc_color&0x00FF0000)>>16);
                            loc_Variable->m_hasOwnBackgroundColor = true;
                        }
                        break;
                    default:
                        loc_Variable->m_value = QString("unknown conversion type (%1)").arg(loc_Variable->m_conversionType);
                        loc_Variable->m_hasOwnBackgroundColor = false;
                        break;
                }
                break;
            default:
                loc_Variable->m_value = QString("unknown display type (%1)").arg(loc_Variable->m_type);
                break;
            }
            if (calculateMinMaxColumnSize(row, row, CALC_PIXELWIDTH_OF_VALUES) == CALC_PIXELWIDTH_OF_VALUES) {
                // Width of the value column has changed
                ColumnUpdateFlags |= CALC_PIXELWIDTH_OF_VALUES;
            }
            QVector<int> Roles;
            Roles.append(Qt::DisplayRole);
            emit dataChanged(index(row, 1), index(row, 1), Roles);
        }
    } else {
         loc_Variable->m_value = QString("dosen't exist");
         if (calculateMinMaxColumnSize(row, row, CALC_PIXELWIDTH_OF_VALUES) == CALC_PIXELWIDTH_OF_VALUES) {
            // Width of the value column has changed
             ColumnUpdateFlags |= CALC_PIXELWIDTH_OF_VALUES;
         }
         QVector<int> Roles;
         Roles.append(Qt::DisplayRole);
         emit dataChanged(index(row, 1), index(row, 1), Roles);
    }
    return ColumnUpdateFlags;
}

QStringList TextTableModel::getAllVariableNames()
{
    QStringList loc_variableNames;
    for(int i = 0; i < m_listOfElements.count(); ++i) {
        loc_variableNames <<  m_listOfElements.at(i)->m_name;
    }
    return loc_variableNames;
}

void TextTableModel::makeBackup()
{
    // First delete
    foreach(Variable *loc_variable, m_backupListOfElements) {
        delete loc_variable;
    }
    m_backupListOfElements.clear();
    foreach(Variable *loc_variable, m_listOfElements) {
        Variable *loc_Copy = new Variable();
        *loc_Copy = *loc_variable;
        m_backupListOfElements.append(loc_Copy);
    }
}

void TextTableModel::restoreBackup()
{
    beginResetModel();
    foreach(Variable *loc_variable, m_listOfElements) {
        delete loc_variable;
    }
    m_listOfElements.clear();
    foreach(Variable *loc_variable, m_backupListOfElements) {
        Variable *loc_Copy = new Variable();
        *loc_Copy = *loc_variable;
        m_listOfElements.append(loc_Copy);
    }
    endResetModel();
    SyncCopyBuffer();
}


void TextTableModel::blackboardVariableConfigChanged(int arg_vid, unsigned int arg_observationFlag)
{
    Q_UNUSED(arg_observationFlag)

    Variable *Var = nullptr;
    Variable *BackupVar = nullptr;
    int Index;

    // first check if a backup exists (this can happen if the config dialog of the text window is open)
    for (Index = 0; Index < m_backupListOfElements.size(); Index++) {
        Variable *v = m_backupListOfElements.at(Index);
        if(v->m_vid == arg_vid) {
            BackupVar = v;
            break;
        }
    }
    for (Index = 0; Index < m_listOfElements.size(); Index++) {
        Variable *v = m_listOfElements.at(Index);
        if(v->m_vid == arg_vid) {
            Var = v;
            break;
        }
    }
    if ((Var != nullptr) || (BackupVar != nullptr)) {
        bool CallSyncCopyBufferFlag = false;
        char loc_unit[BBVARI_UNIT_SIZE];
        char loc_name[BBVARI_NAME_SIZE];
        GetBlackboardVariableName(arg_vid, loc_name, sizeof(loc_name));
        if (Var != nullptr) Var->m_name = QString(loc_name);
        if (BackupVar != nullptr) BackupVar->m_name = QString(loc_name);
        get_bbvari_unit(arg_vid, loc_unit, BBVARI_UNIT_SIZE);
        if (Var != nullptr) Var->m_unit = QString::fromLocal8Bit(loc_unit);
        if (BackupVar != nullptr) BackupVar->m_unit = QString::fromLocal8Bit(loc_unit);
        bool Exist;
        if (arg_vid > 0) {
            enum BB_DATA_TYPES loc_dataType = static_cast<enum BB_DATA_TYPES>(get_bbvaritype(arg_vid));
            if (Var != nullptr) Var->m_dataType = loc_dataType;
            if (BackupVar != nullptr) BackupVar->m_dataType = loc_dataType;
            Exist = (loc_dataType != BB_UNKNOWN_WAIT);
        } else {
            Exist = false;
        }
        if (Var != nullptr) {
            if (Var->m_exists != Exist) {
                CallSyncCopyBufferFlag = true;
            }
            Var->m_exists = Exist;
        }
        if (BackupVar != nullptr) BackupVar->m_exists = Exist;
        int loc_conversionType = get_bbvari_conversiontype(arg_vid);
        if (Var != nullptr) Var->m_conversionType = loc_conversionType;
        if (BackupVar != nullptr) BackupVar->m_conversionType = loc_conversionType;
        if((loc_conversionType == BB_CONV_FORMULA) ||
            (loc_conversionType == BB_CONV_TEXTREP) ||
            (loc_conversionType == BB_CONV_FACTOFF) ||
            (loc_conversionType == BB_CONV_OFFFACT) ||
            (loc_conversionType == BB_CONV_TAB_INTP) ||
            (loc_conversionType == BB_CONV_TAB_NOINTP) ||
            (loc_conversionType == BB_CONV_RAT_FUNC)) {
            if (Var != nullptr) Var->m_hasConversion = true;
            if (BackupVar != nullptr) BackupVar->m_hasConversion = true;
        } else {
            if (Var != nullptr) Var->m_hasConversion = false;
            if (BackupVar != nullptr) BackupVar->m_hasConversion = false;
        }
        int Precision = get_bbvari_format_prec(arg_vid);
        if (Var != nullptr) Var->m_precision = Precision;
        if (BackupVar != nullptr) BackupVar->m_precision = Precision;

        if (Var != nullptr) CyclicUpdateValues(Index, Index, true, true);

        double Step;
        unsigned char StepType;
        if (get_bbvari_step(arg_vid, &StepType, &Step) == 0) {
            if (Var != nullptr) Var->m_step = Step;
            if (BackupVar != nullptr) BackupVar->m_step = Step;
            if (Var != nullptr) Var->m_stepType = StepType;
            if (BackupVar != nullptr) BackupVar->m_stepType = StepType;
        }
        if (CallSyncCopyBufferFlag) {
            SyncCopyBuffer();
        }
    }
}

void TextTableModel::WriteContentToFile(bool par_IndentLine, enum FileTypeEnum par_FileType, QTextStream &Stream)
{
    int x = 0;

    if (1) {
        QString Prefix;
        if (par_IndentLine) {
            Prefix = "    ";
        } else {
            Prefix = "";
        }
        foreach(Variable* Element, m_listOfElements) {
            if (Element->m_exists) {
                QModelIndex Index = createIndex(x, 1);
                QString Value = data(Index, Qt::DisplayRole).toString();
                if (Element->m_type == 3) {  // phys
                    if (get_bbvari_conversiontype(Element->m_vid) == 2) {   // Textreplace
                        if(!Value.compare("out of range")) {
                            char ValueString[256];
                            bbvari_to_string (m_Types[x],
                                             m_Values[x],
                                             10,
                                             ValueString,
                                             sizeof(ValueString));
                            Value = QString(ValueString);
                            goto __CANNOT_USE_PHYS;
                        }
                        switch(par_FileType) {
                        case SCRIPT_FILE_TYPE:
                            Stream << Prefix << "SET(" << Element->m_name << " = enum(" << Element->m_name << "@" << Value << "))\n";
                            break;
                        case EQU_FILE_TYPE:
                            Stream << Prefix << Element->m_name << " = enum(" << Element->m_name << "@" << Value << ");\n";
                            break;
                        }
                    } else {
                        switch(par_FileType) {
                        case SCRIPT_FILE_TYPE:
                            Stream << Prefix << "SET(phys(" << Element->m_name << ") = " << Value << ")\n";
                            break;
                        case EQU_FILE_TYPE:
                            Stream << Prefix << "phys(" << Element->m_name << ") = " << Value << ";\n";
                            break;
                        }
                    }
                } else {
__CANNOT_USE_PHYS:
                    switch(par_FileType) {
                    case SCRIPT_FILE_TYPE:
                        Stream << Prefix << "SET(" << Element->m_name << " = " << Value << ")\n";
                        break;
                    case EQU_FILE_TYPE:
                         Stream << Prefix <<  Element->m_name << " = " << Value << ";\n";
                        break;
                    }
                }
            }
            x++;
        }
    }
}
