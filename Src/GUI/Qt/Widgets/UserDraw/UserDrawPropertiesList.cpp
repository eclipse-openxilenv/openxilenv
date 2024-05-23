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


#include "UserDrawPropertiesList.h"

UserDrawPropertiesList::UserDrawPropertiesList()
{

}

UserDrawPropertiesList::~UserDrawPropertiesList()
{
    clear();
}

void UserDrawPropertiesList::clear()
{
    foreach (UserDrawPropertiesListElem* Property, m_Properties)  {
        delete Property;
    }
    m_Properties.clear();
}

void UserDrawPropertiesList::Add(QString par_Name, QString par_Value, enum UserDrawPropertiesListElem::UserDrawPropertyType par_Type, QStringList par_Enums)
{
    foreach (UserDrawPropertiesListElem* Property, m_Properties)  {
        if (par_Name.compare(Property->m_Name) == 0) {
            Property->m_Value = par_Value;
            return;
        }
    }
    // not found -> new one
    UserDrawPropertiesListElem *NewProperty = new UserDrawPropertiesListElem;
    NewProperty->m_Name = par_Name;
    NewProperty->m_Value = par_Value;
    NewProperty->m_Type = par_Type;
    NewProperty->m_Emums = par_Enums;
    m_Properties.append(NewProperty);
}

int UserDrawPropertiesList::count()
{
    return m_Properties.count();
}

UserDrawPropertiesListElem *UserDrawPropertiesList::at(int par_Pos)
{
    return m_Properties.at(par_Pos);
}

QString UserDrawPropertiesList::GetPropertiesString()
{
    QString Ret;
    Ret.append("(");
    bool First = true;
    foreach (UserDrawPropertiesListElem* Property, m_Properties)  {
        if (!Property->m_Value.isEmpty()) {
            if (First) {
                First = false;
            } else {
                Ret.append(",");
            }
            Ret.append(Property->m_Name);
            Ret.append("=");
            Ret.append(Property->m_Value);
        }
    }
    Ret.append(")");
    return Ret;
}
