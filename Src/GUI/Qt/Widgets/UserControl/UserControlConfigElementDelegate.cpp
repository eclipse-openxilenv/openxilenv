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


#include "UserControlConfigElementDelegate.h"
#include "UserControlPropertiesList.h"
#include "UserControlConfigElementPropertiesModel.h"

#include <QComboBox>
#include <QMenu>
#include <QContextMenuEvent>


extern "C" {
#include "ThrowError.h"
}

UserControlConfigElementDelegate::UserControlConfigElementDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* UserControlConfigElementDelegate::createEditor(QWidget *parent,
                                                     const QStyleOptionViewItem &option,
                                                     const QModelIndex &index) const
{
    /*if (index.column() == 1) {
        UserControlPropertiesListElem *Property = (UserControlPropertiesListElem*)index.internalPointer();
        switch (Property->m_Type) {
            default:
            break;
        }
    }*/
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void UserControlConfigElementDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QString Value = index.model()->data(index, Qt::EditRole).toString();

    if (index.column() == 1) {
        /*UserControlPropertiesListElem *Property = (UserControlPropertiesListElem*)index.internalPointer();
        switch (Property->m_Type) {
        default:
            break;
        }*/
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void UserControlConfigElementDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QString Value = index.model()->data(index, Qt::EditRole).toString();

    if (index.column() == 1) {
        //UserControlPropertiesListElem *Property = (UserControlPropertiesListElem*)index.internalPointer();
        //switch (Property->m_Type) {
        //default:
            QStyledItemDelegate::setModelData(editor, model, index);
         //   break;
        //}
        QString PropertiesString = ((UserControlConfigElementPropertiesModel*)model)->GetPropertiesString();
        emit PropertiesChanged(PropertiesString);
    }
}

QSize UserControlConfigElementDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString Value = index.model()->data(index, Qt::EditRole).toString();
    /*if (index.column() == 1) {
        UserControlPropertiesListElem *Property = (UserControlPropertiesListElem*)index.internalPointer();
        switch (Property->m_Type) {
        default:
            break;
        }
    }*/
    return QStyledItemDelegate::sizeHint(option, index);

}

void UserControlConfigElementDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}

