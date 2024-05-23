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


#ifndef USERCONTROLBUTTON_H
#define USERCONTROLBUTTON_H

#include <QString>
#include <QList>
#include <QPushButton>
#include "UserControlElement.h"

class UserControlButton : public UserControlElement {
    Q_OBJECT
public:
    UserControlButton(int par_Pos, QString &par_ParameterString, UserControlElement *par_Parent = nullptr);
    ~UserControlButton();
    void ResetToDefault() Q_DECL_OVERRIDE;
    void SetDefaultParameterString() Q_DECL_OVERRIDE;
    bool ParseOneParameter(QString& par_Value) Q_DECL_OVERRIDE;
    bool Init() Q_DECL_OVERRIDE;
    QString GetTypeString() Q_DECL_OVERRIDE;
    QWidget *GetWidget() Q_DECL_OVERRIDE;

    void ScriptStateChanged(int par_State) Q_DECL_OVERRIDE;

    static int Is(QString &par_Line);

private slots:
    void ButtonPressed();

private:
    QString m_Text;
    QString m_Script;
    QPushButton *m_PushButton;
};


#endif // USERCONTROLBUTTON_H
