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


#ifndef UserDrawImageModel_H
#define UserDrawImageModel_H

#include <QAbstractListModel>
#include <QString>
#include <QImage>

#include "UserDrawBinaryImage.h"
#include "UserDrawImageItem.h"

class UserDrawImageModel : public QAbstractListModel {
    Q_OBJECT
public:
    UserDrawImageModel(QList<UserDrawImageItem*> *par_Images, QObject *parent = nullptr);
    ~UserDrawImageModel();
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    int InsertItem(int par_Row, UserDrawImageItem *par_NewItem);
    void DeleteItem(int par_Row);

    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) Q_DECL_OVERRIDE;

    QString GetUniqueName(QString par_Name);
    int GetUniqueNumber(int par_Number);

private:

    UserDrawImageItem *GetItem(const QModelIndex &index) const;

    QList<UserDrawImageItem*> *m_Images;
};


#endif // UserDrawImageModel_H
