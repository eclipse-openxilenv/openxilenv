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


#include "OscilloscopeZoomHistoryDialog.h"
#include "ui_OscilloscopeZoomHistoryDialog.h"

OscilloscopeZoomHistoryDialog::OscilloscopeZoomHistoryDialog(OSCILLOSCOPE_DATA *par_Data, QWidget *parent) : Dialog(parent),
    ui(new Ui::OscilloscopeZoomHistoryDialog)
{
    m_Data = par_Data;
    ui->setupUi(this);

    int UsedRows = 0;
    for (int Row = 0; Row < 10; Row++) {
        if (m_Data->y_zoom[Row] > 0.0) {
            UsedRows++;
        } else {
            break;
        }
    }

    ui->tableWidget->setColumnCount(5);   // Nummer | Factor | Offset | base time | time window
    ui->tableWidget->setRowCount(UsedRows);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    QStringList m_TableHeader;
    m_TableHeader << "#" << "Factor" << "Offset" << "Base time" << "Time window";
    ui->tableWidget->setHorizontalHeaderLabels(m_TableHeader);

    for (int Row = 0; Row < UsedRows; Row++) {
        if (m_Data->y_zoom[Row] > 0.0) {
            ui->tableWidget->setItem(Row, 0, new QTableWidgetItem(QString("%1").arg(Row)));
            ui->tableWidget->setItem(Row, 1, new QTableWidgetItem(QString("%1").arg(m_Data->y_zoom[Row])));
            ui->tableWidget->setItem(Row, 2, new QTableWidgetItem(QString("%1").arg(m_Data->y_off[Row])));
            ui->tableWidget->setItem(Row, 3, new QTableWidgetItem(QString("%1").arg(m_Data->t_window_base[Row])));
            ui->tableWidget->setItem(Row, 4, new QTableWidgetItem(QString("%1").arg(m_Data->t_window_width[Row])));
        }
    }
    ui->tableWidget->selectRow(m_Data->zoom_pos);
}

OscilloscopeZoomHistoryDialog::~OscilloscopeZoomHistoryDialog()
{
    delete ui;
}

int OscilloscopeZoomHistoryDialog::GetSelectedZoomRow()
{
    QList<QTableWidgetItem*> Selected = ui->tableWidget->selectedItems();
    if (Selected.size() > 0) {
        int SelectedRow = Selected.at(0)->row();
        return SelectedRow;
    }
    return 0;
}

