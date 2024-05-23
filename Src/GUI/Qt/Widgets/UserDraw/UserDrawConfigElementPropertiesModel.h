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


#ifndef USERDRAWCONFIGELEMENTPROPERTIESMODEL_H
#define USERDRAWCONFIGELEMENTPROPERTIESMODEL_H

#include <QList>
#include <QWidget>
#include <QTableView>

#include <QAbstractListModel>
#include <QString>
#include "UserDrawElement.h"

/*class UserDrawConfigElementPropertiesModelItem {
public:
    QString m_Name;
    QString m_Value;
};*/

class UserDrawConfigElementPropertiesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    UserDrawConfigElementPropertiesModel(QObject *parent = nullptr);
    ~UserDrawConfigElementPropertiesModel();

    void SetPorperties(UserDrawPropertiesList *par_Properties);

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    void AppendProperty(QString par_Name, QString par_Value);
    //void AppendBaseProperties(UserDrawElement &par_Element);

    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) Q_DECL_OVERRIDE;

    QString GetPropertiesString();
    void Reset();
private:
    //UserDrawConfigElementPropertiesModelItem *GetItem(const QModelIndex &index) const;
    //QList<UserDrawConfigElementPropertiesModelItem*> m_Items;
    UserDrawPropertiesListElem *GetItem(const QModelIndex &index) const;
    UserDrawPropertiesList *m_Properties;
    UserDrawPropertiesList m_EmptyProperties;
};


/*
class UserDrawConfigElement : public QWidget
{
    Q_OBJECT
public:
    explicit UserDrawConfigElement(QWidget *parent = nullptr);
    ~UserDrawConfigElement();
    void AddPoperty(QString &par_Name, QString &par_Value);
    void AddBasePoperties(UserDrawElement *par_Values);
    void ResetAllProperies();
signals:
    void ProperiesHasChanged(QString &par_PropertiesString);
private:
    QTableView m_TableView;
    UserDrawConfigElementPropertiesModel m_Model;
};*/

#endif // USERDRAWCONFIGELEMENTPROPERTIESMODEL_H
