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


#include "CalibrationTreeAddReferenceDialog.h"
#include "ui_CalibrationTreeAddReferenceDialog.h"

extern "C" {
    #include "Scheduler.h"
    #include "PipeMessages.h"
    #include "ExtProcessReferences.h"
    #include "ThrowError.h"
}

CalibrationTreeAddReferenceDialog::CalibrationTreeAddReferenceDialog(QStringList &par_Labels, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibrationTreeAddReferenceDialog)
{
    ui->setupUi(this);
    m_Model = new CalibrationTreeAddReferenceDialogModel;

    m_Model->setColumnCount(2);
    m_Model->setHeaderData (0, Qt::Horizontal, QObject::tr("label"));
    m_Model->setHeaderData (1, Qt::Horizontal, QObject::tr("rename to"));

    int Row = 0;
    foreach (QString Label, par_Labels) {
        m_Model->insertRow (Row);
        m_Model->setData (m_Model->index (Row, 0), Label);
        m_Model->setData (m_Model->index (Row, 1), Label);
        Row++;
    }

    ui->treeView->setModel(m_Model);

    ui->treeView->resizeColumnToContents(0);
    ui->treeView->resizeColumnToContents(1);

    ui->treeView->edit(m_Model->index(0,1));

}

CalibrationTreeAddReferenceDialog::~CalibrationTreeAddReferenceDialog()
{
    delete ui;
    delete m_Model;
}

QStringList CalibrationTreeAddReferenceDialog::GetRenamings()
{
    QStringList Ret;
    for (int Row = 0; Row < m_Model->rowCount(); Row++) {
        Ret.append(m_Model->data(m_Model->index(Row,1)).toString());
    }
    return Ret;
}
