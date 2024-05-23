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


#include "BlackboardSignalSelectionWidget.h"

#include "MainWindow.h"

BlackboardSignalSelectionWidget::BlackboardSignalSelectionWidget(QWidget *parent) :
    QGroupBox(parent)
{
    this->setTitle("Variable");
    m_VerticalLayout = new QVBoxLayout(this);
    m_VerticalLayout->setObjectName(QString::fromUtf8("m_VerticalLayout"));

    m_HorizontalLayout_3 = new QHBoxLayout();
    m_HorizontalLayout_3->setObjectName(QString::fromUtf8("m_HorizontalLayout_3"));
    m_FilterLabel = new QLabel(this);
    m_FilterLabel->setObjectName(QString::fromUtf8("filterLabel"));
    m_FilterLabel->setMinimumSize(QSize(30, 0));
    m_FilterLabel->setText("Filter:");

    m_HorizontalLayout_3->addWidget(m_FilterLabel);

    m_FilterLineEdit = new QLineEdit(this);
    m_FilterLineEdit->setObjectName(QString::fromUtf8("m_FilterLineEdit"));

    m_HorizontalLayout_3->addWidget(m_FilterLineEdit);

    m_VerticalLayout->addLayout(m_HorizontalLayout_3);

    m_HorizontalLayout_4 = new QHBoxLayout();
    m_HorizontalLayout_4->setObjectName(QString::fromUtf8("m_HorizontalLayout_4"));
    m_FilterVisibleCheckBox = new QCheckBox(this);
    m_FilterVisibleCheckBox->setObjectName(QString::fromUtf8("m_FilterVisibleCheckBox"));
    m_FilterVisibleCheckBox->setText("Only visable");
    m_FilterVisibleCheckBox->setVisible(false);
    m_HorizontalLayout_4->addWidget(m_FilterVisibleCheckBox);

    m_FilterAutomaticallyExtendCheckBox = new QCheckBox(this);
    m_FilterAutomaticallyExtendCheckBox->setObjectName(QString::fromUtf8("m_FilterAutomaticallyExtendCheckBox"));
    m_FilterAutomaticallyExtendCheckBox->setText("Automatically extend search patter \"*xxxxx*\"");
    m_HorizontalLayout_4->addWidget(m_FilterAutomaticallyExtendCheckBox);

    m_HorizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_HorizontalLayout_4->addItem(m_HorizontalSpacer_2);

    m_VerticalLayout->addLayout(m_HorizontalLayout_4);

    m_ListView = new QListView(this);
    m_ListView->setObjectName(QString::fromUtf8("m_ListView"));
    m_ListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ListView->setUniformItemSizes(true);

    m_VerticalLayout->addWidget(m_ListView);

    m_BlackboardModel = MainWindow::GetBlackboardVariableModel();
    m_CheckableSortFilterProxyModel = new CheckableSortFilterProxyModel2(this);
    m_CheckableSortFilterProxyModel->SetCheckable(false);
    m_CheckableSortFilterProxyModel->setSourceModel(m_BlackboardModel);
    m_CheckableSortFilterProxyModel->sort(0);
    m_ListView->setModel(m_CheckableSortFilterProxyModel);

    connect(m_FilterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(InternSetFilterSlot(QString)));
    connect(m_FilterAutomaticallyExtendCheckBox, SIGNAL(toggled(bool)), this, SLOT(FilterAutomaticallyExtendRegExpr(bool)));
    connect(m_FilterVisibleCheckBox, SIGNAL(toggled(bool)), m_CheckableSortFilterProxyModel, SLOT(SetOnlyVisable(bool)));

    m_RegExpCheckedByUser = false;
    m_FilterLineEdit->setText("*");
    // At the beginning expand always "xxxx" to "*xxxx*"
    m_AutomaticallyExtendRegExpr = true;
    m_FilterAutomaticallyExtendCheckBox->setChecked(m_AutomaticallyExtendRegExpr);
    m_FilterLineEdit->setFocus();
}

BlackboardSignalSelectionWidget::~BlackboardSignalSelectionWidget()
{
    delete m_CheckableSortFilterProxyModel;
}

QItemSelectionModel *BlackboardSignalSelectionWidget::selectionModel() const
{
    return m_ListView->selectionModel();
}

void BlackboardSignalSelectionWidget::scrollTo(const QModelIndex &index, QAbstractItemView::ScrollHint hint)
{
    m_ListView->scrollTo(index, hint);
}

QAbstractItemModel *BlackboardSignalSelectionWidget::model() const
{
    return m_ListView->model();
}

void BlackboardSignalSelectionWidget::SetFilter(QString &par_Filter)
{
    m_FilterLineEdit->setText(par_Filter);
}

void BlackboardSignalSelectionWidget::SetCheckable(bool &par_Checkable)
{
    m_FilterVisibleCheckBox->setVisible(par_Checkable);
    m_CheckableSortFilterProxyModel->SetCheckable(par_Checkable);
}

void BlackboardSignalSelectionWidget::InternSetFilterSlot(QString par_Filter)
{
    if (m_AutomaticallyExtendRegExpr) {
        QString NewPattern;
        int Len = par_Filter.size();
        if (Len == 0) {
            NewPattern = QString("*");
        } else {
            if (par_Filter.at(0) != QChar('*')) {
                if (par_Filter.at(Len-1) != QChar('*')) {
                    NewPattern = QString("*").append(par_Filter).append("*");
                } else {
                    NewPattern = QString("*").append(par_Filter);
                }
            } else {
                if (par_Filter.at(Len-1) != QChar('*')) {
                    NewPattern = par_Filter;
                    NewPattern.append("*");
                } else {
                    // concentrate ** to one *
                    if (Len == 2) {
                        NewPattern = QString("*");
                    } else {
                        NewPattern = par_Filter;
                    }
                }
            }
        }
        m_CheckableSortFilterProxyModel->SetFilter(NewPattern);
    } else {
        m_CheckableSortFilterProxyModel->SetFilter(par_Filter);
    }
}

void BlackboardSignalSelectionWidget::FilterAutomaticallyExtendRegExpr(bool par_Switch)
{
    m_AutomaticallyExtendRegExpr = par_Switch;
    QString Filter = m_FilterLineEdit->text();
    InternSetFilterSlot(Filter);
}

