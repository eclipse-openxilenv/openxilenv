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


#ifndef USERCONTROLELEMENT_H
#define USERCONTROLELEMENT_H

#include <math.h>
#include <QString>
#include <QColor>
#include <QPainter>
#include <QObject>

#include "UserControlPropertiesList.h"


class UserControlElement : public QObject {
    Q_OBJECT
public:
    UserControlElement(UserControlElement *par_Parent = nullptr);
    virtual ~UserControlElement();
    virtual bool Init();
    virtual void ResetToDefault();
    virtual void SetDefaultParameterString();

    virtual bool AddChild(UserControlElement* par_Child, int par_Pos = -1);
    virtual void DeleteChild(int par_Pos);
    virtual void ChildHasChanged(UserControlElement* par_Child);

    int ChildNumber();
    virtual int ChildNumber(UserControlElement *par_Child);

    virtual bool IsGroup();

    virtual bool ParseOneParameter(QString &par_Value) = 0;
    bool ParseOneBaseParameter(QString& Key, QString &ValueString);
    void SetColor(QPainter *par_Painter);
    void SetFillColor(QPainter *par_Painter);

    virtual QString GetTypeString() = 0;
    virtual int GetChildCount();
    UserControlElement *GetParent() {return m_Parent;}
    virtual UserControlElement *GetChild(int par_Index);

    void WriteToINI(QString &par_WindowName, QString& par_Entry, int par_Fd);

    virtual QWidget *GetWidget() = 0;

    int GetX() { return m_X; }
    int GetY() { return m_Y; }
    int GetWidth() { return m_Width; }
    int GetHeight() { return m_Height; }

    virtual void ScriptStateChanged(int par_State);

//private:
    bool ParseParameterString();
    bool InitParameter(int par_Pos, QString &par_ParameterString);
    UserControlElement *m_Parent;

    int m_Color;

    int m_X;
    int m_Y;
    int m_Width;
    int m_Height;

    QString m_ParameterLine;

    UserControlPropertiesList m_Properties;
};


#endif // USERCONTROLELEMENT_H
