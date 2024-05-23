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


#include "CanConfigSearchVariableDialog.h"
#include "ui_CanConfigSearchVariableDialog.h"

#include "CanConfigDialog.h"

CanConfigSearchVariableDialog::CanConfigSearchVariableDialog(CanConfigTreeModel *par_Model, QModelIndex par_BaseIndex, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CanConfigSearchVariableDialog)
{
    ui->setupUi(this);
    m_Model = par_Model;
    m_BaseIndex = par_BaseIndex;
    m_SerchPos = m_BaseIndex;
}

CanConfigSearchVariableDialog::~CanConfigSearchVariableDialog()
{
    delete ui;
}

void CanConfigSearchVariableDialog::on_SeachPushButton_clicked()
{
    QString Filter = ui->FilterLineEdit->text();
    if (!Filter.isEmpty()) {
        QRegularExpression RegExp(Filter);
        //RegExp.setPatternSyntax(QRegExp::Wildcard);
        QModelIndex FoundSignalIndex = m_Model->SeachNextSignal(m_BaseIndex, m_SerchPos, &RegExp);
        if (FoundSignalIndex.isValid()) {
            emit SelectSignal(FoundSignalIndex);
            ui->SeachPushButton->setText(QString("Next"));
            m_SerchPos = FoundSignalIndex;
        } else {
            ui->SeachPushButton->setText(QString("Search"));
            m_SerchPos = m_BaseIndex;
        }
    }
}

void CanConfigSearchVariableDialog::on_ClosePushButton_clicked()
{
    this->close();
}
