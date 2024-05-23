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


#include <QPainter>
#include "UserControlModel.h"
#include "UserControlParser.h"


UserControlModel::UserControlModel(UserControlElement *par_Root, QObject *par_Parent)
    : QAbstractItemModel(par_Parent)
{
    m_Root = par_Root;
}

UserControlModel::~UserControlModel()
{
}

QVariant UserControlModel::data(const QModelIndex &par_Index, int par_Role) const
{
    UserControlElement *Item = GetItem (par_Index);

    switch (par_Role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        if (par_Index.column() == 1) {
            return Item->m_ParameterLine;
        } else {
            return Item->GetTypeString();
        }
        break;
    case Qt::UserRole:
        return Item->m_ParameterLine;
        break;
    }
    return QVariant();
}

QVariant UserControlModel::headerData(int par_Section, Qt::Orientation par_Orientation, int par_Role) const
{
    if (par_Orientation == Qt::Horizontal && par_Role == Qt::DisplayRole) {
        switch (par_Section) {
        case 0:
            return QString ("Type");
        case 1:
            return QString ("Parameters");
        }
    }
    return QVariant();
}

QModelIndex UserControlModel::index(int par_Row, int par_Column, const QModelIndex &par_Parent) const
{
    UserControlElement *ParentItem;
    if (par_Parent.isValid() && par_Parent.column() != 0) {
        ParentItem = m_Root;
    } else {
        ParentItem = GetItem(par_Parent);
    }
    if  (par_Row >= ParentItem->GetChildCount()) {
        return QModelIndex();
    }
    UserControlElement *ChildItem = ParentItem->GetChild(par_Row);
    if (ChildItem) {
        return createIndex (par_Row, par_Column, ChildItem);
    } else {
        return QModelIndex();
    }
}

QModelIndex UserControlModel::parent(const QModelIndex &par_Index) const
{
    if (!par_Index.isValid()) {
        return QModelIndex();
    }
    UserControlElement *ChildItem = GetItem(par_Index);
    UserControlElement *ParentItem = ChildItem->GetParent();

    if (ParentItem == m_Root) {
        return QModelIndex();
    }
    return createIndex (ParentItem->ChildNumber(ChildItem), 0, ParentItem);
}

int UserControlModel::rowCount(const QModelIndex &par_Parent) const
{
    UserControlElement *Item = GetItem (par_Parent);
    return Item->GetChildCount();
}

int UserControlModel::columnCount(const QModelIndex &par_Parent) const
{
    Q_UNUSED(par_Parent);
    return 2;
}

int UserControlModel::InsertItem(int par_Row, UserControlElement *par_NewItem, const QModelIndex &par_Parent)
{
    UserControlElement *ParentPtr = GetItem(par_Parent);
    int ToInsertRow;
    int ChildCount = ParentPtr->GetChildCount();
    if ((par_Row < 0) || (par_Row > ChildCount)) ToInsertRow = ChildCount;
    else ToInsertRow = par_Row;
    beginInsertRows(par_Parent, ToInsertRow, ToInsertRow);
    ParentPtr->AddChild(par_NewItem, ToInsertRow);
    endInsertRows();
    emit layoutChanged();
    return ToInsertRow;
}

void UserControlModel::DeleteItem(int par_Row, const QModelIndex &par_Parent)
{
    QModelIndex Child;
    UserControlElement *Item;
    Child = index(par_Row, 0, par_Parent);
    Item = GetItem(Child);
    if (Item != nullptr) {
        if (Item->IsGroup()) {
            int CildCount = Item->GetChildCount();
            for (int x = CildCount - 1; x >= 0 ; x--) {  // delete from the end
                DeleteItem(x, Child);
            }
        }
    }
    Item = GetItem(par_Parent);
    if (Item != nullptr) {
        beginRemoveRows(par_Parent, par_Row, par_Row);
        Item->DeleteChild(par_Row);
        endRemoveRows();
    }
    emit layoutChanged();
}

Qt::ItemFlags UserControlModel::flags(const QModelIndex &par_Index) const
{
    if (par_Index.column() == 1) {  // Value column is editable
        return Qt::ItemIsEditable | QAbstractItemModel::flags(par_Index);
    } else {
        return QAbstractItemModel::flags(par_Index);
    }
}

bool UserControlModel::setData(const QModelIndex &par_Index, const QVariant &par_Value, int par_Role)
{
    if (par_Role != Qt::EditRole) {
        return false;
    }
    UserControlElement *Item = GetItem (par_Index);
    Item->m_ParameterLine = par_Value.toString();
    Item->ParseParameterString();
    Item->Init();
    UserControlElement *Parent = Item->GetParent();
    if (Parent != nullptr) Parent->ChildHasChanged(Item);

    emit dataChanged(par_Index, par_Index);
    return true;
}

UserControlElement *UserControlModel::GetItem(const QModelIndex &par_Index) const
{
    if (par_Index.isValid()) {
        UserControlElement *item = static_cast<UserControlElement*>(par_Index.internalPointer());
        if (item)
            return item;
    }
    return m_Root;
}

