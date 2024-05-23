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


#include "CheckableSortFilterProxyModel.h"
#include "StringHelpers.h"

extern "C" {
#include "MainValues.h"
#include "Wildcards.h"
#include "StringMaxChar.h"
}

CheckableSortFilterProxyModel2::CheckableSortFilterProxyModel2(QObject *parent) :
    QSortFilterProxyModel(parent)
{
    m_Checkable = false;
    m_OnlyVisable = false;
    m_ConversionType = static_cast<enum BB_CONV_TYPES>(-1);

    QSortFilterProxyModel::setFilterCaseSensitivity((s_main_ini_val.NoCaseSensitiveFilters) ? Qt::CaseInsensitive : Qt::CaseSensitive);
}

CheckableSortFilterProxyModel2::~CheckableSortFilterProxyModel2()
{
}

void CheckableSortFilterProxyModel2::SetCheckedVariableNames(QStringList &par_CheckedVariableNames)
{
    m_CheckedVariableNames = par_CheckedVariableNames;
}

QVariant CheckableSortFilterProxyModel2::data(const QModelIndex &index, int role) const
{
    if (role == Qt::CheckStateRole) {
        if (!m_Checkable ||
            !index.isValid()) return QVariant();
        QString Name = data(index, Qt::DisplayRole).toString();
        if (m_CheckedVariableNames.contains(Name)) {
            enum Qt::CheckState Ret = Qt::Checked;
            return Ret;
        } else {
            enum Qt::CheckState Ret = Qt::Unchecked;
            return Ret;
        }
    } else {
        return QSortFilterProxyModel::data(index, role);
    }
}

bool CheckableSortFilterProxyModel2::setData(const QModelIndex &arg_index, const QVariant &arg_value, int arg_role)
{
    if (arg_role == Qt::CheckStateRole) {
        if (!arg_index.isValid()) return false;
        if (arg_value.toInt() == Qt::Checked) {
            setCheckedState(arg_index, true);
            QVector<int> Roles;
            Roles.append(static_cast<int>(Qt::CheckStateRole));
            emit dataChanged(arg_index, arg_index, Roles);
            return true;
        } else if (arg_value.toInt() == Qt::Unchecked) {
            setCheckedState(arg_index, false);
            QVector<int> Roles;
            Roles.append(static_cast<int>(Qt::CheckStateRole));
            emit dataChanged(arg_index, arg_index, Roles);
            return true;
        } else {
            return sourceModel()->setData(arg_index, arg_value, arg_role);
        }
    } else {
        return sourceModel()->setData(arg_index, arg_value, arg_role);
    }
}

Qt::ItemFlags CheckableSortFilterProxyModel2::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    if (m_Checkable) {
        return Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    } else {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
    //return sourceModel()->flags(index);
}

void CheckableSortFilterProxyModel2::setCheckedState(QString arg_VariableName, bool arg_checked)
{
    if (!m_multiSelect) {
        foreach (QString Name, m_CheckedVariableNames) {
            emit checkedNodesChanged();
            emit checkedStateChanged(Name, false);
        }
        m_CheckedVariableNames.clear();
    }
    if (arg_checked) {
        if (!m_CheckedVariableNames.contains(arg_VariableName)) {
            m_CheckedVariableNames.append(arg_VariableName);
        }
    } else {
        m_CheckedVariableNames.removeAll(arg_VariableName);
    }
    emit checkedNodesChanged();
    emit checkedStateChanged(arg_VariableName, arg_checked);
}

void CheckableSortFilterProxyModel2::setCheckedState(QModelIndex arg_proxyModelIndex, bool arg_checked)
{
    if(arg_proxyModelIndex.isValid())
    {
        QString Name = data(arg_proxyModelIndex, Qt::DisplayRole).toString();
        setCheckedState(Name, arg_checked);
    }
}

Qt::CheckState CheckableSortFilterProxyModel2::currentCheckState(QModelIndex arg_modelIndex) const
{
    QString Name = data(arg_modelIndex, Qt::DisplayRole).toString();
    if (m_CheckedVariableNames.contains(Name)) {
        enum Qt::CheckState Ret = Qt::Checked;
        return Ret;
    } else {
        enum Qt::CheckState Ret = Qt::Unchecked;
        return Ret;
    }
}

void CheckableSortFilterProxyModel2::resetToDefault(void)
{
    m_CheckedVariableNames.clear();
}

void CheckableSortFilterProxyModel2::ResetProxyModel()
{
    beginResetModel();
    invalidate();
    resetInternalData();
    invalidateFilter();
    endResetModel();
}

void CheckableSortFilterProxyModel2::SetCheckable(bool par_Checkable)
{
    m_Checkable = par_Checkable;
}

// Only visable
void CheckableSortFilterProxyModel2::SetOnlyVisable(bool par_OnlyVisable)
{
    m_OnlyVisable = par_OnlyVisable;
    ResetProxyModel();
}

void CheckableSortFilterProxyModel2::SetFilter(const QString &par_Filter)
{
    StringCopyMaxCharTruncate (m_OldFilter, QStringToConstChar(par_Filter), sizeof(m_OldFilter));
    m_OldFilter[sizeof(m_OldFilter)-1] = 0;
    m_Filter = par_Filter;
    ResetProxyModel();
}

void CheckableSortFilterProxyModel2::SetFilterConversionType(BB_CONV_TYPES par_ConversionType)
{
     m_ConversionType = par_ConversionType;
     ResetProxyModel();
}

bool CheckableSortFilterProxyModel2::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if(m_OnlyVisable) {
        QModelIndex Index = sourceModel()->index(source_row, 0, source_parent);
        QString Name =  sourceModel()->data(Index, Qt::DisplayRole).toString();
        if (m_CheckedVariableNames.contains(Name, (s_main_ini_val.NoCaseSensitiveFilters) ? Qt::CaseInsensitive : Qt::CaseSensitive)) {
            return true;
        }
    } else {        
        QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
        if ((m_ConversionType == static_cast<enum BB_CONV_TYPES>(-1)) ||
            (sourceModel()->data(index, Qt::UserRole + 2).toInt() == m_ConversionType)) {
            QString Text = sourceModel()->data(index).toString();
            if (Compare2StringsWithWildcardsCaseSensitive(QStringToConstChar(Text), m_OldFilter, !s_main_ini_val.NoCaseSensitiveFilters) == 0) {
                return true;
            }
        }
    }
    return false;
}

bool CheckableSortFilterProxyModel2::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    return (sourceModel()->data(left).toString().compare(sourceModel()->data(right).toString(), Qt::CaseInsensitive) < 0);
}


