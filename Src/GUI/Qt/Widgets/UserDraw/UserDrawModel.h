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


#ifndef USERDRAWMODEL_H
#define USERDRAWMODEL_H

#include <QAbstractItemModel>
#include <QString>

#include "UserDrawElement.h"

class UserDrawModel : public QAbstractItemModel {
    Q_OBJECT
public:
    UserDrawModel(UserDrawElement *par_Root, QObject *par_Parent = nullptr);
    ~UserDrawModel();
    QVariant data(const QModelIndex &par_Index, int par_Role) const Q_DECL_OVERRIDE;
    QVariant headerData(int par_Section, Qt::Orientation par_Orientation,
                        int par_Role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QModelIndex index(int par_Row, int par_Column,
                      const QModelIndex &par_Parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &par_Index) const Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &par_Parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &par_Parent = QModelIndex()) const Q_DECL_OVERRIDE;

    int InsertItem(int par_Row, UserDrawElement *par_NewItem, const QModelIndex &par_Parent);
    void DeleteItem(int par_Row, const QModelIndex &par_Parent);
    Qt::ItemFlags flags(const QModelIndex &par_Index) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &par_Iindex, const QVariant &par_Value,
                 int par_Role = Qt::EditRole) Q_DECL_OVERRIDE;
private:   

    UserDrawElement *GetItem(const QModelIndex &par_Index) const;

    UserDrawElement *m_Root;
};


#endif // USERDRAWMODEL_H
