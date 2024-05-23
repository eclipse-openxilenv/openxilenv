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


#include "ScriptUserDialog.h"
#include "PhysValueInput.h"
#include "StringHelpers.h"

#include <QVBoxLayout>

extern "C" {
#include "ThrowError.h"
#include "MyMemory.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
}

ScriptUserDialog::ScriptUserDialog(QString &HeadLine)
{
    setWindowTitle(HeadLine);
    m_Tab = new QTabWidget(this);
    m_ButtonBox = new QDialogButtonBox(this);
    m_ButtonBox->addButton(QDialogButtonBox::Ok);
    m_ButtonBox->addButton(QDialogButtonBox::Cancel);
    connect (m_ButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect (m_ButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QVBoxLayout* Layout = new QVBoxLayout(this);
    Layout->addWidget(m_Tab);
    Layout->addWidget(m_ButtonBox);
    AddPage(QString("Main"));

    m_IsClosed = false;
    m_NeededMaxWidth = 300;
    m_NeededHightCurrentPage = SCRIPT_USER_DIALOG_ADDITIONAL_HIGHT;
    m_NeededMaxHight = 300;
}

ScriptUserDialog::~ScriptUserDialog()
{

}

void ScriptUserDialog::ReInit(QString &HeadLine)
{
    m_Tab->clear();
    AddPage(QString("Main"));
    setWindowTitle(HeadLine);
}

int ScriptUserDialog::AddItem(QString &Element, QString &Label)
{
    QString loc_Element = Element.trimmed();
    QString loc_Label = Label.trimmed();
    if (loc_Label.size() == 0) {
        ThrowError (1, "empty label in command ADD_DIALOG_ITEM are not allowed");
        return -1;
    }
    bool ErrMsgFlag = false;
    for (int x = 0; x < loc_Label.size(); x++) {
        if (loc_Label.at(x).isSpace()) {
            if (!ErrMsgFlag) {
                ErrMsgFlag = true;
                ThrowError (1, "label \"%s\" in command ADD_DIALOG_ITEM should not have whitespaces",
                           QStringToConstChar(loc_Label));
            }
        }
    }
    // add new page
    if (!loc_Label.compare(QString("*NEWPAGE*"))) {
        if (!loc_Element.compare(QString("*NEWPAGE*"))) {
           loc_Element = QString("Page(%1)").arg(m_Tab->count());
           m_NeededHightCurrentPage = SCRIPT_USER_DIALOG_ADDITIONAL_HIGHT;
        }
        AddPage(loc_Element);
        return 0;
    }

    // If headline
    if (!loc_Label.compare(QString("*HEADLINE*"))) {
        int NeededWidth = AddHeadLineToCurrentPage(loc_Element);
        if (NeededWidth > m_NeededMaxWidth) m_NeededMaxWidth = NeededWidth;
        m_NeededHightCurrentPage += SCRIPT_USER_DIALOG_HEADER_HIGHT;
        if (m_NeededHightCurrentPage > m_NeededMaxHight) m_NeededMaxHight = m_NeededHightCurrentPage;
    } else {
        // otherwise it is a label (add to blackboard)
        int NeededWidth = AddItemToCurrentPage(loc_Element, loc_Label);
        if (NeededWidth > m_NeededMaxWidth) m_NeededMaxWidth = NeededWidth;
        m_NeededHightCurrentPage += SCRIPT_USER_DIALOG_LINE_HIGHT;
        if (m_NeededHightCurrentPage > m_NeededMaxHight) m_NeededMaxHight = m_NeededHightCurrentPage;
    }
    return 0;
}

int ScriptUserDialog::Show()
{
    if (m_NeededMaxWidth > 800) m_NeededMaxWidth = 800;
    if (m_NeededMaxHight > 800) m_NeededMaxHight = 800;
    this->resize(m_NeededMaxWidth, m_NeededMaxHight);
    this->show();
    return 0;
}

bool ScriptUserDialog::IsClosed()
{
    return m_IsClosed;
}

void ScriptUserDialog::accept()
{
    WriteAllBlackboardVariableBack(true);
    m_IsClosed = true;
    QDialog::accept();
}

void ScriptUserDialog::reject()
{
    RemoveAllBlackboardVariable();
    m_IsClosed = true;
    QDialog::reject();
}

void ScriptUserDialog::AddPage(const QString &PageName)
{
    m_CurrentPage = new QListWidget(m_Tab);
    m_Tab->addTab(m_CurrentPage, PageName);
}

int ScriptUserDialog::AddHeadLineToCurrentPage(QString &Text)
{
    QListWidgetItem* NewItem = new QListWidgetItem(m_CurrentPage);
    NewItem->setBackground(QColor().fromRgb(200,200,200));
    NewItem->setFlags(Qt::NoItemFlags);
     m_CurrentPage->addItem(NewItem);
    ScriptUserDialogHeaderItem *New = new ScriptUserDialogHeaderItem(Text, m_CurrentPage);
    NewItem->setSizeHint(QSize(200, SCRIPT_USER_DIALOG_HEADER_HIGHT));
    m_CurrentPage->setItemWidget(NewItem, New);
    return New->GetNeededWidth();
}

int ScriptUserDialog::AddItemToCurrentPage(QString &Text, QString &Label)
{
    QListWidgetItem* NewItem = new QListWidgetItem(m_CurrentPage);
    m_CurrentPage->addItem(NewItem);
    ScriptUserDialogValueItem *New = new ScriptUserDialogValueItem(Text, Label, m_CurrentPage);
    NewItem->setSizeHint(QSize(200, SCRIPT_USER_DIALOG_LINE_HIGHT));
    m_CurrentPage->setItemWidget(NewItem, New);
    return New->GetNeededWidth();
}

void ScriptUserDialog::WriteAllBlackboardVariableBack(bool Remove)
{
    for (int x = 0; x < m_Tab->count(); x++) {
        QListWidget *List = static_cast<QListWidget*>(m_Tab->widget(x));
        for (int y = 0; y < List->count(); y++) {
            QListWidgetItem* Item = List->item(y);
            ScriptUserDialogItem *i = static_cast<ScriptUserDialogItem*>(List->itemWidget(Item));
            if (i->GetType() == ScriptUserDialogItem::Value) {
                (static_cast<ScriptUserDialogValueItem*>(i))->WriteBlackboardVariableBack(Remove);
            }
        }
    }
}

void ScriptUserDialog::RemoveAllBlackboardVariable()
{
    for (int x = 0; x < m_Tab->count(); x++) {
        QListWidget *List = static_cast<QListWidget*>(m_Tab->widget(x));
        for (int y = 0; y < List->count(); y++) {
            QListWidgetItem* Item = List->item(y);
            ScriptUserDialogItem *i = static_cast<ScriptUserDialogItem*>(List->itemWidget(Item));
            if (i->GetType() == ScriptUserDialogItem::Value) {
                (static_cast<ScriptUserDialogValueItem*>(i))->RemoveBlackboardVariable();
            }
        }
    }
}

ScriptUserDialogValueItem::ScriptUserDialogValueItem(QString &Text, QString &Label, QWidget *Parent) :
    ScriptUserDialogItem(Parent)
{
    m_Vid = add_bbvari (QStringToConstChar(Label), BB_UNKNOWN, "");
    m_Text = new QLabel(Text, this);
    if (m_Vid > 0) {
        m_Value = new PhysValueInput(this);
        m_Value->SetPlusMinusButtonEnable(true);
        unsigned char StepType;
        double Step;
        if (get_bbvari_step(m_Vid, &StepType, &Step) == 0) {
            m_Value->SetPlusMinusIncrement(Step);
            m_Value->SetStepLinear(StepType == 0);
        }
        m_Value->SetBlackboardVariableId(m_Vid);
        switch (get_bbvari_conversiontype(m_Vid)) {
        case 1:  // Formula
        case 2:  // ENUM
            m_Value->SetDisplayPhysValue(true);
            break;
        }
        m_ErrorText = nullptr;
    } else {
        m_ErrorText = new QLabel(QString("Variable \"%1\" doesn't exist").arg(Label), this);
        m_Value = nullptr;
    }
}

void ScriptUserDialogValueItem::RemoveBlackboardVariable()
{
    if (m_Vid > 0) {
        remove_bbvari(m_Vid);
    }
}

void ScriptUserDialogValueItem::WriteBlackboardVariableBack(bool Remove)
{
    if (m_Vid > 0) {
        if (m_Value != nullptr) {
            double Value = m_Value->GetDoubleRawValue();
            write_bbvari_minmax_check(m_Vid, Value);
        }
        if (Remove) remove_bbvari(m_Vid);
    }

}

int ScriptUserDialogValueItem::GetNeededWidth()
{
    QRect Rect = this->m_Text->fontMetrics().boundingRect(m_Text->text());
    return Rect.width() + SCRIPT_USER_DIALOG_EDIT_WIDTH + SCRIPT_USER_DIALOG_ADDITIONAL_GAP;
}

void ScriptUserDialogValueItem::resizeEvent(QResizeEvent *)
{

    m_Text->resize(width()-SCRIPT_USER_DIALOG_EDIT_WIDTH, height());
    m_Text->move(0,0);
    if(m_Value != nullptr) {
        m_Value->resize(SCRIPT_USER_DIALOG_EDIT_WIDTH, height()-(SCRIPT_USER_DIALOG_EDIT_BORDER*2));
        m_Value->move(width()-SCRIPT_USER_DIALOG_EDIT_WIDTH, SCRIPT_USER_DIALOG_EDIT_BORDER);
    }
    if(m_ErrorText != nullptr) {
        m_ErrorText->resize(SCRIPT_USER_DIALOG_EDIT_WIDTH, height());
        m_ErrorText->move(width()-SCRIPT_USER_DIALOG_EDIT_WIDTH, 0);
    }
}

ScriptUserDialogHeaderItem::ScriptUserDialogHeaderItem(QString &Text, QWidget *Parent) :
       ScriptUserDialogItem(Parent)
{
    QVBoxLayout* Layout = new QVBoxLayout(this);
    m_Header = new QLabel(Text, this);
    Layout->addWidget(m_Header);
}

int ScriptUserDialogHeaderItem::GetNeededWidth()
{
    QRect Rect = this->m_Header->fontMetrics().boundingRect(m_Header->text());
    return Rect.width() + SCRIPT_USER_DIALOG_ADDITIONAL_GAP;
}


ScriptUserDialogItem::ScriptUserDialogItem(QWidget *Parent) :
    QWidget(Parent)
{

}

