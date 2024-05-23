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


#ifndef USERCONTROLCONFIGELEMENTPROPERTIESMODEL_H
#define USERCONTROLCONFIGELEMENTPROPERTIESMODEL_H

#include <QList>
#include <QWidget>
#include <QTableView>

#include <QAbstractListModel>
#include <QString>
#include "UserControlElement.h"

class UserControlConfigElementPropertiesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    UserControlConfigElementPropertiesModel(QObject *parent = nullptr);
    ~UserControlConfigElementPropertiesModel();

    void SetPorperties(UserControlPropertiesList *par_Properties);

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    void AppendProperty(QString par_Name, QString par_Value);
    //void AppendBaseProperties(UserControlElement &par_Element);

    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) Q_DECL_OVERRIDE;

    QString GetPropertiesString();
    void Reset();
private:
    //UserControlConfigElementPropertiesModelItem *GetItem(const QModelIndex &index) const;
    //QList<UserControlConfigElementPropertiesModelItem*> m_Items;
    UserControlPropertiesListElem *GetItem(const QModelIndex &index) const;
    UserControlPropertiesList *m_Properties;
    UserControlPropertiesList m_EmptyProperties;
};

#endif // USERCONTROLCONFIGELEMENTPROPERTIESMODEL_H
