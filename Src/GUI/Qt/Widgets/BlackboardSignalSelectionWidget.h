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


#ifndef BLACKBOARDSIGNALSELECTIONWIDGET_H
#define BLACKBOARDSIGNALSELECTIONWIDGET_H

#include <QWidget>
#include <QListView>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpacerItem>

#include "BlackboardVariableModel.h"
#include "CheckableSortFilterProxyModel.h"

class BlackboardSignalSelectionWidget : public QGroupBox
{
    Q_OBJECT
public:
    BlackboardSignalSelectionWidget(QWidget *parent = nullptr);
    ~BlackboardSignalSelectionWidget();
    QItemSelectionModel *selectionModel() const;
    void scrollTo(const QModelIndex &index, QAbstractItemView::ScrollHint hint = QAbstractItemView::EnsureVisible);
    QAbstractItemModel *model() const;
    void SetFilter(QString &par_Filter);
    void SetCheckable(bool &par_Checkable);
public slots:
    void FilterAutomaticallyExtendRegExpr(bool par_Switch);
    void InternSetFilterSlot(QString par_Filter);
private:
    QHBoxLayout *m_HorizontalLayout_3;
    QHBoxLayout *m_HorizontalLayout_4;
    QVBoxLayout *m_VerticalLayout;
    QLabel *m_FilterLabel;
    QListView *m_ListView;
    QLineEdit *m_FilterLineEdit;
    QCheckBox *m_FilterVisibleCheckBox;
    QCheckBox *m_FilterAutomaticallyExtendCheckBox;
    QSpacerItem *m_HorizontalSpacer_2;

    BlackboardVariableModel *m_BlackboardModel;
    CheckableSortFilterProxyModel2 *m_CheckableSortFilterProxyModel;

    bool m_RegExpCheckedByUser;
    bool m_AutomaticallyExtendRegExpr;

};

#endif // BLACKBOARDSIGNALSELECTIONWIDGET_H
