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


#ifndef SCRIPTUSERDIALOG_H
#define SCRIPTUSERDIALOG_H

#include <QLabel>
#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>

#include "PhysValueInput.h"

#define SCRIPT_USER_DIALOG_HEADER_HIGHT      30
#define SCRIPT_USER_DIALOG_LINE_HIGHT        30
#define SCRIPT_USER_DIALOG_EDIT_WIDTH        200
#define SCRIPT_USER_DIALOG_EDIT_BORDER       2
#define SCRIPT_USER_DIALOG_ADDITIONAL_GAP    60
#define SCRIPT_USER_DIALOG_ADDITIONAL_HIGHT  84


class ScriptUserDialogItem : public QWidget {
public:
    ScriptUserDialogItem(QWidget *Parent);
    enum Type {Header, Value};

    virtual enum Type GetType() = 0;
};

class ScriptUserDialogHeaderItem : public ScriptUserDialogItem {
public:
    ScriptUserDialogHeaderItem(QString &Text, QWidget *Parent);
    enum Type GetType() { return Header; }
    int GetNeededWidth();
private:
    QLabel *m_Header;

};

class ScriptUserDialogValueItem : public ScriptUserDialogItem {
public:
    ScriptUserDialogValueItem(QString &Text, QString &Label, QWidget *Parent);
    enum Type GetType()  Q_DECL_OVERRIDE { return Value; }
    void RemoveBlackboardVariable();
    void WriteBlackboardVariableBack(bool Remove);
    int GetNeededWidth();
protected:
    void resizeEvent(QResizeEvent * /* event */) Q_DECL_OVERRIDE;
private:
    QLabel *m_Text;
    PhysValueInput *m_Value;
    QLabel *m_ErrorText;
    long m_Vid;
};


class ScriptUserDialog : public QDialog
{
public:
    ScriptUserDialog(QString &HeadLine);
    ~ScriptUserDialog();

    void ReInit(QString &HeadLine);
    int AddItem(QString &Element, QString &Label);
    int Show();
    bool IsClosed();

public slots:
    void accept();
    void reject();

private:

    void AddPage(const QString &PageName);
    int AddHeadLineToCurrentPage(QString &Text);
    int AddItemToCurrentPage(QString &Text, QString &Label);

    void WriteAllBlackboardVariableBack(bool Remove);
    void RemoveAllBlackboardVariable();

    QTabWidget *m_Tab;
    QDialogButtonBox *m_ButtonBox;
    QListWidget *m_CurrentPage;

    int m_NeededMaxWidth;
    int m_NeededMaxHight;
    int m_NeededHightCurrentPage;
    bool m_IsClosed;
};


#endif // SCRIPTUSERDIALOG_H
