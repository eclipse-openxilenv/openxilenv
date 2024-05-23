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


#include "CanConfigJ1939MultiPGDialog.h"
#include "ui_CanConfigJ1939MultiPGDialog.h"

#include "CanConfigDialog.h"

extern "C" {
#include "MainValues.h"
#include "Wildcards.h"
}

CheckableSortFilterProxyModel::CheckableSortFilterProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
    m_Only_C_PG = false;
    QSortFilterProxyModel::setFilterCaseSensitivity((s_main_ini_val.NoCaseSensitiveFilters) ? Qt::CaseInsensitive : Qt::CaseSensitive);
}

CheckableSortFilterProxyModel::~CheckableSortFilterProxyModel()
{
}

void CheckableSortFilterProxyModel::SetCheckedObjectIds(QList<int> &par_CheckedObjectIds)
{
    m_CheckedCanObjects = par_CheckedObjectIds;
}

QVariant CheckableSortFilterProxyModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::CheckStateRole) {
        QModelIndex SrcIndex = mapToSource(index);
        CanConfigElement *Element = ((CanConfigTreeModel*)sourceModel())->GetItem(SrcIndex);
        if (Element->GetType() != CanConfigElement::Object) return QVariant();
        CanConfigObject *ObjectElemnt = (CanConfigObject*)Element;
        if (m_CheckedCanObjects.contains(ObjectElemnt->GetId())) {
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

bool CheckableSortFilterProxyModel::setData(const QModelIndex &arg_index, const QVariant &arg_value, int arg_role)
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
            QModelIndex SrcIndex = mapToSource(arg_index);
            return sourceModel()->setData(SrcIndex, arg_value, arg_role);
        }
    } else {
        return sourceModel()->setData(arg_index, arg_value, arg_role);
    }
}

Qt::ItemFlags CheckableSortFilterProxyModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    //return sourceModel()->flags(index);
}

void CheckableSortFilterProxyModel::setCheckedState(int par_ObjectId, bool arg_checked)
{
    if (arg_checked) {
        if (!m_CheckedCanObjects.contains(par_ObjectId)) {
            m_CheckedCanObjects.append(par_ObjectId);
        }
    } else {
        m_CheckedCanObjects.removeAll(par_ObjectId);
    }
    emit checkedNodesChanged();
    emit checkedStateChanged(par_ObjectId, arg_checked);
}

void CheckableSortFilterProxyModel::setCheckedState(QModelIndex par_Index, bool arg_checked)
{
    QModelIndex SrcIndex = mapToSource(par_Index);
    CanConfigElement *Element = ((CanConfigTreeModel*)sourceModel())->GetItem(SrcIndex);
    if (Element->GetType() != CanConfigElement::Object) return;
    CanConfigObject *ObjectElemnt = (CanConfigObject*)Element;
    setCheckedState(ObjectElemnt->GetId(), arg_checked);
}


Qt::CheckState CheckableSortFilterProxyModel::currentCheckState(QModelIndex par_Index) const
{
    QModelIndex SrcIndex = mapToSource(par_Index);
    CanConfigElement *Element = ((CanConfigTreeModel*)sourceModel())->GetItem(SrcIndex);
    if (Element->GetType() != CanConfigElement::Object) return Qt::Unchecked;
    CanConfigObject *ObjectElemnt = (CanConfigObject*)Element;
    if (m_CheckedCanObjects.contains(ObjectElemnt->GetId())) {
        enum Qt::CheckState Ret = Qt::Checked;
        return Ret;
    } else {
        enum Qt::CheckState Ret = Qt::Unchecked;
        return Ret;
    }
}

QList<int> CheckableSortFilterProxyModel::Get_C_PGs()
{
    return m_CheckedCanObjects;
}

void CheckableSortFilterProxyModel::Set_C_PGs(QList<int> &par_PGs)
{
    m_CheckedCanObjects = par_PGs;
}

bool CheckableSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!m_Only_C_PG) return true;
    QModelIndex Index = sourceModel()->index(source_row, 0, source_parent);
    CanConfigElement *Element = ((CanConfigTreeModel*)sourceModel())->GetItem(Index);
    if (Element->GetType() != CanConfigElement::Object) return false;
    CanConfigObject *ObjectElemnt = (CanConfigObject*)Element;
    if (m_Only_C_PG) {
        if (ObjectElemnt->GetObjectType() == CanConfigObject::J1939_C_PG) {
            if (m_WriteOrRead == ObjectElemnt->GetDir()) {
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        return true;
    }
}

/*bool CheckableSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    return (sourceModel()->data(left).toString().compare(sourceModel()->data(right).toString(), Qt::CaseInsensitive) < 0);
}*/

CanConfigJ1939MultiPGDialog::CanConfigJ1939MultiPGDialog(CanConfigTreeModel *par_Model, QModelIndex &par_Index, int par_WriteOrRead,
                                                         QList<int> &par_C_PGs, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CanConfigJ1939MultiPGDialog)
{
    ui->setupUi(this);
    m_ProxyModel = new CheckableSortFilterProxyModel();
    m_ProxyModel->Set_C_PGs(par_C_PGs);
    m_ProxyModel->setSourceModel(par_Model);
    ui->listView->setModel(m_ProxyModel);
    //QModelIndex ObjectIndex = m_ProxyModel->mapFromSource(par_Index);
    QModelIndex VariantIndex = par_Index.parent();
    QModelIndex VariantIndex2 = m_ProxyModel->mapFromSource(VariantIndex);
    m_ProxyModel->SetOnly_C_PG(true, par_WriteOrRead);
    ui->listView->setRootIndex(VariantIndex2);
}

CanConfigJ1939MultiPGDialog::~CanConfigJ1939MultiPGDialog()
{
    delete ui;
}

QList<int> CanConfigJ1939MultiPGDialog::Get_C_PGs()
{
    return m_ProxyModel->Get_C_PGs();
}
