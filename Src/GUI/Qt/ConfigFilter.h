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


#ifndef ConfigFilter_H
#define ConfigFilter_H

#include <QWidget>

#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

extern "C" {
#include "Wildcards.h"
}

class ConfigFilter : public QGroupBox
{
    Q_OBJECT
public:
    explicit ConfigFilter(QWidget *parent = nullptr);
    ~ConfigFilter();

    void SetFilter (INCLUDE_EXCLUDE_FILTER *Filter);
    INCLUDE_EXCLUDE_FILTER *GetFilter (void);

signals:
    void FilterClicked();

private slots:
    void ContextMenuIncludeSlot (const QPoint &pos);
    void ContextMenuExcludeSlot (const QPoint &pos);

    void EditIncludeFilterItemSlot (QListWidgetItem *Item);
    void EditExcludeFilterItemSlot (QListWidgetItem *Item);

    void AddBeforeIncludeSlot();
    void AddBehindIncludeSlot();
    void InsertBeforeIncludeSlot();
    void InsertBehindIncludeSlot();
    void DeleteIncludeSlot();
    void CopyIncludeSlot();

    void AddBeforeExcludeSlot();
    void AddBehindExcludeSlot();
    void InsertBeforeExcludeSlot();
    void InsertBehindExcludeSlot();
    void DeleteExcludeSlot();
    void CopyExcludeSlot();

    void FilterSlot();

public slots:

private:
    void createActions(void);

    void SetCopyBufferState (bool par_State);

    bool eventFilter(QObject *target, QEvent *event);

    QLabel *m_MainFilterLabel;
    QHBoxLayout *m_MainFilterLayout;

    QLineEdit *m_MainFilterLineEdit;
    QPushButton *m_FilterPushButton;
    QTabWidget *m_IncludeExcludeFilterTab;

    QWidget *m_IncludeFilterTab;
    QPushButton *m_AddBeforeIncludePushButton;
    QPushButton *m_AddBehindIncludePushButton;
    QPushButton *m_InsertBeforeIncludePushButton;
    QPushButton *m_InsertBehindIncludePushButton;
    QPushButton *m_DeleteIncludePushButton;
    QPushButton *m_CopyIncludePushButton;
    QVBoxLayout *m_IncludeButtonsLayout;
    QListWidget *m_IncludeFilterListWidget;
    QHBoxLayout *m_IncludeLayout;

    QWidget *m_ExcludeFilterTab;
    QPushButton *m_AddBeforeExcludePushButton;
    QPushButton *m_AddBehindExcludePushButton;
    QPushButton *m_InsertBeforeExcludePushButton;
    QPushButton *m_InsertBehindExcludePushButton;
    QPushButton *m_DeleteExcludePushButton;
    QPushButton *m_CopyExcludePushButton;
    QVBoxLayout *m_ExcludeButtonsLayout;
    QListWidget *m_ExcludeFilterListWidget;
    QHBoxLayout *m_ExcludeLayout;

    QVBoxLayout *m_FilterGroupLayout;

    QAction *m_AddBeforeIncludeAct;
    QAction *m_AddBehindIncludeAct;
    QAction *m_InsertBeforeIncludeAct;
    QAction *m_InsertBehindIncludeAct;
    QAction *m_DeleteIncludeAct;
    QAction *m_CopyIncludeAct;

    QAction *m_AddBeforeExcludeAct;
    QAction *m_AddBehindExcludeAct;
    QAction *m_InsertBeforeExcludeAct;
    QAction *m_InsertBehindExcludeAct;
    QAction *m_DeleteExcludeAct;
    QAction *m_CopyExcludeAct;

    QStringList m_CopyBuffer;
};

#endif // ConfigFilter_H
