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


#include "UserControlConfigElementPropertiesModel.h"
extern "C" {
#include "ThrowError.h"
}

UserControlConfigElementPropertiesModel::UserControlConfigElementPropertiesModel(QObject *parent) :
    QAbstractListModel(parent)
{
    m_Properties = &m_EmptyProperties;
}

UserControlConfigElementPropertiesModel::~UserControlConfigElementPropertiesModel()
{
}

void UserControlConfigElementPropertiesModel::SetPorperties(UserControlPropertiesList *par_Properties)
{
    beginResetModel();
    m_Properties = par_Properties;
    endResetModel();
}

#if 0
static QString RepaceCommaWithNewLine(QString &par_String)
{
    QString Ret;
    int Len = par_String.length();
    if ((Len > 0) && (par_String.at(0) != '(')) {
        ThrowError (1, "expecting a '(' in \"%s\"", QStringToConstChar(par_String));
        return Ret;;
    } else {
        int BraceCounter = 0;
        int StartParameter = 0;
        int ElemCount = 0;
        for (int x = 0; x < Len; x++) {
            QChar c = par_String.at(x);
            if (c == '(') {
                BraceCounter++;
                if (BraceCounter == 1) {
                    StartParameter = x;
                }
            } else if (c == ')') {
                if ((BraceCounter == 0) && (x < (Len - 1))) {
                    ThrowError (1, "missing '(' in \"%s\"", QStringToConstChar(par_String));
                    goto __OUT;
                }
                if (BraceCounter == 1) {
                    QString Parameter = par_String.mid(StartParameter+1, x - StartParameter-1);
                    if (ElemCount) Ret.append("\n");
                    ElemCount++;
                    Ret.append(Parameter);
                    StartParameter = x;
                }
                BraceCounter--;
            } else if (c == ',') {
                if (BraceCounter == 1) {
                    QString Parameter = par_String.mid(StartParameter+1, x - StartParameter-1);
                    if (ElemCount) Ret.append("\n");
                    ElemCount++;
                    Ret.append(Parameter);
                    StartParameter = x;
                }
            }
        }
    }
__OUT:
    return Ret;
}
#endif

QVariant UserControlConfigElementPropertiesModel::data(const QModelIndex &index, int role) const
{
    UserControlPropertiesListElem *Property = GetItem (index);

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch(index.column()) {
        case 0:
            return Property->m_Name;
        case 1:
            return Property->m_Value;
        }
        break;
    }
    return QVariant();
}

QVariant UserControlConfigElementPropertiesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return QString ("Name");
        case 1:
            return QString ("Value");
        }
    }
    return QVariant();
}

QModelIndex UserControlConfigElementPropertiesModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (row < m_Properties->count()) {
        return createIndex (row, column, m_Properties->at(row));
    } else {
        return QModelIndex();
    }

}

int UserControlConfigElementPropertiesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_Properties->count();
}

int UserControlConfigElementPropertiesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

void UserControlConfigElementPropertiesModel::AppendProperty(QString par_Name, QString par_Value)
{
    m_Properties->Add(par_Name, par_Value);

}

Qt::ItemFlags UserControlConfigElementPropertiesModel::flags(const QModelIndex &index) const
{
    if (index.column() <= 1) {  // name and value columns are editable
        return Qt::ItemIsEditable | QAbstractListModel::flags(index);
    } else {
        return QAbstractItemModel::flags(index);
    }
}

bool UserControlConfigElementPropertiesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    UserControlPropertiesListElem *Property = GetItem (index);
    switch(index.column()) {
    case 1:
        Property->m_Value = value.toString();
        emit dataChanged(index, index);
        break;
    }
    return true;
}

QString UserControlConfigElementPropertiesModel::GetPropertiesString()
{
    return m_Properties->GetPropertiesString();
}

void UserControlConfigElementPropertiesModel::Reset()
{
    SetPorperties(&m_EmptyProperties);
}

UserControlPropertiesListElem *UserControlConfigElementPropertiesModel::GetItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        UserControlPropertiesListElem *Property = static_cast<UserControlPropertiesListElem*>(index.internalPointer());
        if (Property)
            return Property;
    }
    return nullptr;
}
