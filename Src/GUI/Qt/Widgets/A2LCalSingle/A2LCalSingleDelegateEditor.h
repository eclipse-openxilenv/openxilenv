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


#ifndef A2LCALSINGLEDELEGATEEDITOR_H
#define A2LCALSINGLEDELEGATEEDITOR_H

#include <QItemDelegate>

class A2LCalSingleDelegateEditor : public QItemDelegate
{
    Q_OBJECT
    public:
        explicit A2LCalSingleDelegateEditor(QObject *parent = nullptr);
        ~A2LCalSingleDelegateEditor();
    protected:
        QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        void setEditorData(QWidget * editor, const QModelIndex & index) const;
        void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const;
        void updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const;

    signals:

    public slots:
        void commitAndCloseEditor();

};


#endif // A2LCALSINGLEDELEGATEEDITOR_H
