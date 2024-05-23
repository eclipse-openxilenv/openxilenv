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


#ifndef USERCONTROLGROUP_H
#define USERCONTROLGROUP_H

#include <QString>
#include <QList>
#include <QGroupBox>
#include <QGridLayout>
#include "UserControlElement.h"

class UserControlGroup : public UserControlElement {
    Q_OBJECT
public:
    UserControlGroup(int par_Pos, QString &par_ParameterString, UserControlElement *par_Parent = nullptr);
    ~UserControlGroup();
    void ResetToDefault() Q_DECL_OVERRIDE;
    void SetDefaultParameterString() Q_DECL_OVERRIDE;
    bool AddChild(UserControlElement* par_Child, int par_Pos = -1) Q_DECL_OVERRIDE;
    void DeleteChild(int par_Pos) Q_DECL_OVERRIDE;
    void ChildHasChanged(UserControlElement* par_Child) Q_DECL_OVERRIDE;
    bool IsGroup() Q_DECL_OVERRIDE;
    int ChildNumber(UserControlElement *par_Child) Q_DECL_OVERRIDE;
    bool ParseOneParameter(QString& par_Value) Q_DECL_OVERRIDE;
    int GetChildCount() Q_DECL_OVERRIDE;
    UserControlElement *GetChild(int par_Index) Q_DECL_OVERRIDE;
    QString GetTypeString() Q_DECL_OVERRIDE;
    QWidget *GetWidget() Q_DECL_OVERRIDE;

    static int Is(QString &par_Line);

private:
    QList<UserControlElement*> m_Childs;

    QString m_Text;
    QGroupBox *m_GroupBox;
    QGridLayout *m_Layout;
};


#endif // USERCONTROLGROUP_H
