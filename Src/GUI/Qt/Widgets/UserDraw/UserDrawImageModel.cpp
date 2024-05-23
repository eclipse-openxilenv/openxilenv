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
#include "UserDrawImageModel.h"
#include "UserDrawParser.h"
#include "QtIniFile.h"

UserDrawImageModel::UserDrawImageModel(QList<UserDrawImageItem *> *par_Images, QObject *parent)
    : QAbstractListModel(parent)
{
    m_Images = par_Images;
}

UserDrawImageModel::~UserDrawImageModel()
{
}

QVariant UserDrawImageModel::data(const QModelIndex &index, int role) const
{
    UserDrawImageItem *Item = GetItem (index);

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch(index.column()) {
        case 0:
            return Item->m_Number;
        case 1:
            return Item->m_Name;
        case 3:
            {
                char Help[64];
                sprintf (Help, "%i x %i", Item->m_Image.width(), Item->m_Image.height());
                return QString(Help);
            }
        }
        break;
    case Qt::DecorationRole:
        if (index.column() == 2) {
            QSize Size = Item->m_Image.size();
            if (Size.height() <= 100) {
                return Item->m_Image;
            } else {
                return Item->m_Image.scaledToHeight(100, Qt::FastTransformation);
            }
        }
        break;
    case Qt::SizeHintRole:
        if (index.column() == 2) {
            QSize Size = Item->m_Image.size();
            if (Size.height() > 100) Size.scale(100, 100, Qt::KeepAspectRatioByExpanding);
            return Size;
        }
        break;
    }
    return QVariant();
}

QVariant UserDrawImageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return QString ("Number");
        case 1:
            return QString ("Name");
        case 2:
            return QString ("Image");
        case 3:
            return QString ("Size");
        }
    }
    return QVariant();
}

QModelIndex UserDrawImageModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (row < m_Images->count()) {
        return createIndex (row, column, m_Images->at(row));
    } else {
        return QModelIndex();
    }
}


int UserDrawImageModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_Images->count();
}

int UserDrawImageModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 4;
}

int UserDrawImageModel::InsertItem(int par_Row, UserDrawImageItem *par_NewItem)
{
    beginInsertRows(QModelIndex(), par_Row, par_Row);
    m_Images->insert(par_Row, par_NewItem);
    endInsertRows();
    emit layoutChanged();
    return par_Row;
}

void UserDrawImageModel::DeleteItem(int par_Row)
{
    UserDrawImageItem *Item;
    Item = GetItem(index(par_Row, 0));
    if (Item != nullptr) {
        beginRemoveRows(QModelIndex(), par_Row, par_Row);
        m_Images->removeAt(par_Row);
        endRemoveRows();
    }
    emit layoutChanged();
}


Qt::ItemFlags UserDrawImageModel::flags(const QModelIndex &index) const
{
    if (index.column() <= 1) {  // number and name columns are editable
        return Qt::ItemIsEditable | QAbstractListModel::flags(index);
    } else {
        return QAbstractItemModel::flags(index);
    }
}

bool UserDrawImageModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    UserDrawImageItem *Item = GetItem (index);
    switch(index.column()) {
    case 0:
        Item->m_Number = value.toUInt();
        break;
    case 1:
        Item->m_Name = value.toString();
        break;
    case 2:
        Item->m_Image = value.value<QImage>();
        break;
    }
    return true;
}

QString UserDrawImageModel::GetUniqueName(QString par_Name)
{
    QString Ret = par_Name;
    bool Exist;
    int x = 0;
    do {
        Exist = false;
        for(int x = 0; x < m_Images->count(); x++) {
            UserDrawImageItem *Item = m_Images->at(x);
            if (Ret.compare(Item->m_Name, Qt::CaseInsensitive) == 0) {
                Exist = true;
            }
        }
        if (Exist) {
            Ret = par_Name;
            x++;
            Ret.append(QString("_%1").arg(x));
        }
    } while (Exist);
    return Ret;
}

int UserDrawImageModel::GetUniqueNumber(int par_Number)
{
    int Ret = par_Number;
    bool Exist;
    do {
        Exist = false;
        for(int x = 0; x < m_Images->count(); x++) {
            UserDrawImageItem *Item = m_Images->at(x);
            if (Ret == Item->m_Number) {
                Exist = true;
            }
        }
        if (Exist) {
            Ret++;
        }
    } while (Exist);
    return Ret;
}

UserDrawImageItem *UserDrawImageModel::GetItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        UserDrawImageItem *item = static_cast<UserDrawImageItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return nullptr;
}

