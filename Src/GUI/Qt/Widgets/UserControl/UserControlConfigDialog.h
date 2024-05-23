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


#ifndef USERCONTROLCONFIGDIALOG_H
#define USERCONTROLCONFIGDIALOG_H

#include <QDialog>
#include "Dialog.h"
#include "UserControlRoot.h"
#include "UserControlModel.h"
#include "UserControlConfigElementPropertiesModel.h"

#define DEFINE_ACTION(Name) \
    QAction *m_AddNew##Name##Act;\
    QAction *m_AddNew##Name##BeforeAct;\
    QAction *m_AddNew##Name##BehindAct;

#define BUIILD_ACTION(Name, name) {\
    m_AddNew##Name##Act = new QAction(tr("Add new " name), this);\
    connect(m_AddNew##Name##Act, SIGNAL(triggered()), this, SLOT(AddNew##Name##Act()));\
    m_AddNew##Name##BeforeAct = new QAction(tr("Add new " name " before"), this);\
    connect(m_AddNew##Name##BeforeAct, SIGNAL(triggered()), this, SLOT(AddNew##Name##BeforeAct()));\
    m_AddNew##Name##BehindAct = new QAction(tr("Add new " name " behind"), this);\
    connect(m_AddNew##Name##BehindAct, SIGNAL(triggered()), this, SLOT(AddNew##Name##BehindAct()));}

#define DEFINE_SLOT_METHODS(Name)\
    void AddNew##Name##Act(); \
    void AddNew##Name##BeforeAct(); \
    void AddNew##Name##BehindAct();

#define BUIILD_USER_CONTROL_SLOT_METHODS(Name)\
void UserControlConfigDialog::AddNew##Name##Act() \
{ QString Empty; AddNewItemToSelected(new UserControl##Name(-1, Empty)); } \
void UserControlConfigDialog::AddNew##Name##BeforeAct() \
{ QString Empty; AddNewItemBeforeSelected(new UserControl##Name(-1, Empty)); } \
void UserControlConfigDialog::AddNew##Name##BehindAct() \
{ QString Empty; AddNewItemBehindSelected(new UserControl##Name(-1, Empty)); }

namespace Ui {
class UserControlConfigDialog;
}

class UserControlConfigDialog : public Dialog
{
    Q_OBJECT

public:
    explicit UserControlConfigDialog(QString &par_WindowName, UserControlRoot *par_Root, QWidget *parent = nullptr);
    ~UserControlConfigDialog();
    bool WindowTitleChanged();
    QString GetWindowTitle();

public slots:
    void accept();
    void reject();

    void DrawItemsTreeContextMenuRequestedSlot(const QPoint &par_Pos);
    void ImageListContextMenuRequestedSlot(const QPoint &par_Pos);

    void currentChanged(const QModelIndex &current, const QModelIndex &previos);

    DEFINE_SLOT_METHODS(Group)
    DEFINE_SLOT_METHODS(Button)

    void DeleteAct();
    void CopyAct();
    void PasteAct();
    void PasteBeforeAct();
    void PasteBehindAct();

    void DeleteImageAct();
    void AddImageAct();

private slots:
    void on_UpdatePushButton_clicked();

    void on_FixedRatioCheckBox_stateChanged(int arg1);

    void PropertiesChanged(const QString &par_Properties);

    void PropertiesChangedInsideTreeview(const QModelIndex &topLeft, const QModelIndex &bottomRight);

private:
    Ui::UserControlConfigDialog *ui;

    bool WriteImagesToIni(QString &par_WindowName);

    UserControlRoot *m_Root;

    class CopyBufferElem {
    public:
        ~CopyBufferElem() { foreach (CopyBufferElem *i, m_Childs) delete i; }
        QString m_Line;
        QList<CopyBufferElem *> m_Childs;
    };
    static CopyBufferElem *m_CopyBuffer;
    CopyBufferElem m_BackupBuffer;

    void CopyRecursiveToBuffer(CopyBufferElem *par_Buffer, QModelIndex &par_Index);
    UserControlElement *PasteBufferRecursive(CopyBufferElem *par_Buffer, UserControlElement *par_Parent, int par_Row);

    void AddNewItemToSelected(UserControlElement *par_NewItem);
    void AddNewItemBeforeSelected(UserControlElement *par_NewItem);
    void AddNewItemBehindSelected(UserControlElement *par_NewItem);

    UserControlModel *m_Model;

    bool m_AddToRootFlag;
    DEFINE_ACTION(Group)
    DEFINE_ACTION(Button)

    QAction *m_DeleteAct;
    QAction *m_CopyAct;

    QAction *m_PasteAct;
    QAction *m_PasteBeforeAct;
    QAction *m_PasteBehindAct;

    UserControlConfigElementPropertiesModel *m_ProprtiesModel;

    QAction *m_DeleteImageAct;
    QAction *m_AddImageAct;

    QString m_WindowTitle;
    bool m_WindowTitleChanged;
};

#endif // USERDRAWCONFIGDIALOG_H
