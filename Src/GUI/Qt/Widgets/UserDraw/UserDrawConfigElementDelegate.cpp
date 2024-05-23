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


#include "UserDrawConfigElementDelegate.h"
#include "UserDrawPropertiesList.h"
#include "UserDrawConfigElementPropertiesModel.h"

#include "UserDrawConfigXYPlotElementDialog.h"
#include "UserDrawConfigPointsElementDialog.h"
#include "StringHelpers.h"

#include <QComboBox>
#include <QMenu>
#include <QContextMenuEvent>


extern "C" {
#include "ThrowError.h"
}

UserDrawConfigElementDelegate::UserDrawConfigElementDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* UserDrawConfigElementDelegate::createEditor(QWidget *parent,
                                                     const QStyleOptionViewItem &option,
                                                     const QModelIndex &index) const
{
    if (index.column() == 1) {
        UserDrawPropertiesListElem *Property = (UserDrawPropertiesListElem*)index.internalPointer();
        switch (Property->m_Type) {
        case UserDrawPropertiesListElem::UserDrawPropertyXYPlotSignalType:
        case UserDrawPropertiesListElem::UserDrawPropertyPointsSignalType:
            {
                UserDrawConfigElementSignalPropertyEditor *SignalEditor = new UserDrawConfigElementSignalPropertyEditor(Property->m_Type, parent);
                return SignalEditor;
            }
            break;
        case UserDrawPropertiesListElem::UserDrawPropertyEnumType:
            {
                QComboBox *ComboBox = new QComboBox(parent);
                ComboBox->addItems(Property->m_Emums);
                return ComboBox;
            }
            break;
        case UserDrawPropertiesListElem::UserDrawPropertyValueType:
        case UserDrawPropertiesListElem::UserDrawPropertyStringType:
        case UserDrawPropertiesListElem::UserDrawPropertyColorType:
            break;
        }
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void UserDrawConfigElementDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString Value = index.model()->data(index, Qt::EditRole).toString();

    if (index.column() == 1) {
        UserDrawPropertiesListElem *Property = (UserDrawPropertiesListElem*)index.internalPointer();
        switch (Property->m_Type) {
        case UserDrawPropertiesListElem::UserDrawPropertyXYPlotSignalType:
        case UserDrawPropertiesListElem::UserDrawPropertyPointsSignalType:
            {
                UserDrawConfigElementSignalPropertyEditor *SignalEditor = static_cast<UserDrawConfigElementSignalPropertyEditor*>(editor);
                SignalEditor->Set(Value);
                break;
            }
        case UserDrawPropertiesListElem::UserDrawPropertyEnumType:
            {
                QComboBox *ComboBox = static_cast<QComboBox*>(editor);
                ComboBox->setCurrentText(Value);
                return;
            }
        case UserDrawPropertiesListElem::UserDrawPropertyValueType:
        case UserDrawPropertiesListElem::UserDrawPropertyStringType:
        case UserDrawPropertiesListElem::UserDrawPropertyColorType:
            break;
        }
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void UserDrawConfigElementDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QString Value = index.model()->data(index, Qt::EditRole).toString();

    if (index.column() == 1) {
        UserDrawPropertiesListElem *Property = (UserDrawPropertiesListElem*)index.internalPointer();
        switch (Property->m_Type) {
        case UserDrawPropertiesListElem::UserDrawPropertyXYPlotSignalType:
        case UserDrawPropertiesListElem::UserDrawPropertyPointsSignalType:
            {
                UserDrawConfigElementSignalPropertyEditor *SignalEditor = static_cast<UserDrawConfigElementSignalPropertyEditor*>(editor);
                model->setData(index, SignalEditor->Get(), Qt::EditRole);
                break;
            }
        case UserDrawPropertiesListElem::UserDrawPropertyEnumType:
            {
                QComboBox *ComboBox = static_cast<QComboBox*>(editor);
                model->setData(index, ComboBox->currentText(), Qt::EditRole);
                break;
            }
        case UserDrawPropertiesListElem::UserDrawPropertyValueType:
        case UserDrawPropertiesListElem::UserDrawPropertyStringType:
        case UserDrawPropertiesListElem::UserDrawPropertyColorType:
            QStyledItemDelegate::setModelData(editor, model, index);
            break;
        }
        QString PropertiesString = ((UserDrawConfigElementPropertiesModel*)model)->GetPropertiesString();
        emit PropertiesChanged(PropertiesString);
    }
}

QSize UserDrawConfigElementDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString Value = index.model()->data(index, Qt::EditRole).toString();
    if (index.column() == 1) {
        UserDrawPropertiesListElem *Property = (UserDrawPropertiesListElem*)index.internalPointer();
        switch (Property->m_Type) {
        case UserDrawPropertiesListElem::UserDrawPropertyXYPlotSignalType:
            {
                QSize Ret;
                Ret.setWidth(100);
                Ret.setHeight(100);
                return Ret;
            }
        case UserDrawPropertiesListElem::UserDrawPropertyPointsSignalType:
            {
                QSize Ret;
                Ret.setWidth(100);
                Ret.setHeight(100);
                return Ret;
            }
        case UserDrawPropertiesListElem::UserDrawPropertyEnumType:
        case UserDrawPropertiesListElem::UserDrawPropertyValueType:
        case UserDrawPropertiesListElem::UserDrawPropertyStringType:
        case UserDrawPropertiesListElem::UserDrawPropertyColorType:
            break;
        }
    }
    return QStyledItemDelegate::sizeHint(option, index);

}

void UserDrawConfigElementDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}

UserDrawConfigElementSignalPropertyEditor::UserDrawConfigElementSignalPropertyEditor(enum UserDrawPropertiesListElem::UserDrawPropertyType par_Type, QWidget *parent) :
    QListWidget(parent)
{
    m_AddBeforeAction = new QAction("Add before", this);
    connect(m_AddBeforeAction, SIGNAL(triggered()), this, SLOT(AddBeforeAct()));
    m_AddBehindAction = new QAction("Add behind", this);
    connect(m_AddBehindAction, SIGNAL(triggered()), this, SLOT(AddBehindAct()));
    m_EditAction = new QAction("Edit", this);
    connect(m_EditAction, SIGNAL(triggered()), this, SLOT(EditAct()));
    m_DeleteAction = new QAction("Delete", this);
    connect(m_DeleteAction, SIGNAL(triggered()), this, SLOT(DeleteAct()));
    setContextMenuPolicy(Qt::DefaultContextMenu); //CustomContextMenu);
    //connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ContextMenuRequestedSlot(const QPoint &)));

    m_Type = par_Type;
}

/*UserDrawConfigElementSignalPropertyEditor::~UserDrawConfigElementSignalPropertyEditor()
{
}*/

void UserDrawConfigElementSignalPropertyEditor::Set(QString &par_Line)
{
    int Len = par_Line.length();
    if ((Len >= 2) && (par_Line.at(0) == '(') && (par_Line.at(Len-1) == ')')) {
        int x;
        QChar c;
        for (x = 0; x < Len; x++) {
            c = par_Line.at(x);
            if (!c.isSpace()) break;
        }
        int BraceCounter = 0;
        int StartParameter = x;
        for (; x < Len; x++) {
            c = par_Line.at(x);
            if (c == '(') {
                BraceCounter++;
                if (BraceCounter == 1) {
                    StartParameter = x;
                }
            } else if (c == ')') {
                if ((BraceCounter == 0) && (x < (Len - 1))) {
                    ThrowError (1, "missing '(' in \"%s\"", QStringToConstChar(par_Line));
                    return;
                }
                if (BraceCounter == 1) {
                    QString Parameter = par_Line.mid(StartParameter+1, x - StartParameter-1);
                    addItem(Parameter.trimmed());
                    StartParameter = x;
                }
                BraceCounter--;
            } else if (c == ',') {
                if (BraceCounter == 1) {
                    QString Parameter = par_Line.mid(StartParameter+1, x - StartParameter-1);
                    addItem(Parameter.trimmed());
                    StartParameter = x;
                }
            }
        }
        if (count() > 0) setCurrentRow(count()-1);  // last element should be selected
    }
}

QString UserDrawConfigElementSignalPropertyEditor::Get()
{
    QString Ret;
    Ret.append("(");
    for (int x = 0; x < count(); x++) {
        QString Signal = item(x)->data(Qt::DisplayRole).toString();
        if (x > 0) Ret.append(",");
        Ret.append(Signal);
    }
    Ret.append(")");
    return Ret;
}

/*void UserDrawConfigElementSignalPropertyEditor::ContextMenuRequestedSlot(const QPoint &par_Point)
{
    QMenu menu(this);

    QModelIndex Index = indexAt (par_Point);

    if (Index.isValid()) {
        menu.addAction (m_AddBeforeAction);
        menu.addAction (m_AddBehindAction);
        menu.addAction (m_EditAction);
        menu.addAction (m_DeleteAction);
    }
    menu.exec(mapToGlobal(par_Point));
}*/

/*QSize UserDrawConfigElementSignalPropertyEditor::sizeHint() const
{
    QSize Ret;
    Ret.setWidth(100);
    Ret.setHeight(100);
    return Ret;
}*/

void UserDrawConfigElementSignalPropertyEditor::AddBeforeAct()
{
    OpenDialog(AddBefore);
}

void UserDrawConfigElementSignalPropertyEditor::AddBehindAct()
{
    OpenDialog(AddBehind);
}

void UserDrawConfigElementSignalPropertyEditor::EditAct()
{
    OpenDialog(Edit);
}

void UserDrawConfigElementSignalPropertyEditor::DeleteAct()
{
    QListWidgetItem *Item = this->currentItem();
    delete Item;
}

void UserDrawConfigElementSignalPropertyEditor::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);


    QModelIndex Index = currentIndex();  //indexAt (event->pos());
    /*if (!Index.isValid()) {
        if (count() > 0) {
            setCurrentRow(count()-1);
            Index = currentIndex();
        }
    }*/
    if (Index.isValid()) {
        menu.addAction (m_AddBeforeAction);
        menu.addAction (m_AddBehindAction);
        menu.addAction (m_EditAction);
        menu.addAction (m_DeleteAction);
    } else {
        menu.addAction (m_AddBehindAction);
    }
    menu.exec(mapToGlobal(event->pos()));
    //QListWidget::contextMenuEvent(event);
}

void UserDrawConfigElementSignalPropertyEditor::OpenDialog(UserDrawConfigElementSignalPropertyEditor::Action par_Action)
{
    QString Parameter;
    QListWidgetItem *Item;
    if (par_Action == Edit) {
        Item = this->currentItem();
        if (Item != nullptr) {
            Parameter = Item->data(Qt::DisplayRole).toString();
        }
    }
    switch (m_Type) {
    case UserDrawPropertiesListElem::UserDrawPropertyXYPlotSignalType:
        {
            UserDrawConfigXYPlotElementDialog *Dlg = new UserDrawConfigXYPlotElementDialog(Parameter, this);
            if(Dlg->exec() == QDialog::Accepted) {
                QString Line = Dlg->Get();
                switch(par_Action) {
                case AddBefore:
                    insertItem(currentRow(), Line);
                    setCurrentRow(currentRow());
                    break;
                case AddBehind:
                    insertItem(currentRow()+1, Line);
                    setCurrentRow(currentRow()+1);
                    break;
                case Edit:
                    Item->setData(Qt::EditRole, Line);
                    break;
                }
            }
            delete Dlg;
        }
        break;
    case UserDrawPropertiesListElem::UserDrawPropertyPointsSignalType:
        {
            UserDrawConfigPointsElementDialog *Dlg = new UserDrawConfigPointsElementDialog(Parameter, this);
            if(Dlg->exec() == QDialog::Accepted) {
                QString Line = Dlg->Get();
                switch(par_Action) {
                case AddBefore:
                    insertItem(currentRow(), Line);
                    setCurrentRow(currentRow());
                    break;
                case AddBehind:
                    insertItem(currentRow()+1, Line);
                    setCurrentRow(currentRow()+1);
                    break;
                case Edit:
                    Item->setData(Qt::EditRole, Line);
                    break;
                }
            }
            delete Dlg;
        }
        break;
    default:
        break;
    }
}

