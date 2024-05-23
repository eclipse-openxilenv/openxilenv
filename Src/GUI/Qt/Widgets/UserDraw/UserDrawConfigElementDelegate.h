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


#ifndef USERDRAWCONFIGELEMENTDELEGATE_H
#define USERDRAWCONFIGELEMENTDELEGATE_H

#include <QStyledItemDelegate>
#include <QListWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAction>

#include "UserDrawPropertiesList.h"


class UserDrawConfigElementSignalPropertyEditor : public QListWidget
{
    Q_OBJECT
public:
    UserDrawConfigElementSignalPropertyEditor(enum UserDrawPropertiesListElem::UserDrawPropertyType par_Type, QWidget *parent = nullptr);
    //~UserDrawConfigElementSignalPropertyEditor();
    void Set(QString &par_Line);
    QString Get();
    //QSize sizeHint() const Q_DECL_OVERRIDE;
Q_SIGNALS:
    void PropertiesChanged(const QString &par_Properties) const;
private slots:
    //void ContextMenuRequestedSlot(const QPoint &par_Point);
    void AddBeforeAct();
    void AddBehindAct();
    void EditAct();
    void DeleteAct();
protected:
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;

private:
    enum Action {AddBefore,AddBehind,Edit};
    void OpenDialog(enum Action par_Action);
    QAction *m_AddBeforeAction;
    QAction *m_AddBehindAction;
    QAction *m_EditAction;
    QAction *m_DeleteAction;
    enum UserDrawPropertiesListElem::UserDrawPropertyType m_Type;
};

class UserDrawConfigElementDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    UserDrawConfigElementDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const Q_DECL_OVERRIDE;

    void updateEditorGeometry(QWidget *editor,
        const QStyleOptionViewItem &option, const QModelIndex &index) const override;
Q_SIGNALS:
    void PropertiesChanged(const QString &par_Properties) const;

private:
    UserDrawConfigElementSignalPropertyEditor *m_SignalEditor;
};

#endif // USERDRAWCONFIGELEMENTDELEGATE_H
