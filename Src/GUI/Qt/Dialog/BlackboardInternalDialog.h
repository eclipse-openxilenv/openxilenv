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


#ifndef BLACKBOARDINTERNALDIALOG_H
#define BLACKBOARDINTERNALDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <Dialog.h>

extern "C" {
#include "Wildcards.h"
}

namespace Ui {
class BlackboardInternalDialog;
}

class BlackboardInternalDialog : public Dialog
{
    Q_OBJECT

public:
    explicit BlackboardInternalDialog(QWidget *parent = nullptr);
    ~BlackboardInternalDialog();

private:
    void FillVarianleList();

    Ui::BlackboardInternalDialog *ui;

    INCLUDE_EXCLUDE_FILTER *m_IncExcFilter;

private slots:
    void FilterSlot();

    void on_VariableListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_OnlyVisableCheckBox_toggled(bool checked);
};

#endif // BLACKBOARDINTERNALDIALOG_H
