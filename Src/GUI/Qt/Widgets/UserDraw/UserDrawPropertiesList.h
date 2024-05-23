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


#ifndef USERDRAWPROPERTIESLIST_H
#define USERDRAWPROPERTIESLIST_H

#include <QString>
#include <QList>

class UserDrawPropertiesListElem
{
public:
    enum UserDrawPropertyType { UserDrawPropertyValueType, UserDrawPropertyStringType, UserDrawPropertyEnumType,
                                UserDrawPropertyColorType, UserDrawPropertyPointsSignalType, UserDrawPropertyXYPlotSignalType} m_Type;
    QString m_Name;
    QString m_Value;
    QStringList m_Emums;
};

class UserDrawPropertiesList
{
public:
    UserDrawPropertiesList();
    ~UserDrawPropertiesList();
    void clear();
    void Add(QString par_Name, QString par_Value, enum UserDrawPropertiesListElem::UserDrawPropertyType par_Type = UserDrawPropertiesListElem::UserDrawPropertyValueType, QStringList par_Enums = QStringList());
    int count();
    UserDrawPropertiesListElem *at(int par_Pos);
    QString GetPropertiesString();
private:
    QList<UserDrawPropertiesListElem*>m_Properties;
};

#endif // USERDRAWPROPERTIESLIST_H
