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


#ifndef CONFIGUREXCPOVERETHERNETWIDGET_H
#define CONFIGUREXCPOVERETHERNETWIDGET_H

#include <QWidget>
#include <QListWidgetItem>

extern "C"
{
    #include "MainValues.h"
}

namespace Ui {
class ConfigureXCPOverEthernetWidget;
}

class ConfigureXCPOverEthernetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigureXCPOverEthernetWidget(QWidget *parent = nullptr);
    ~ConfigureXCPOverEthernetWidget();
    void saveXCPConfigData();

private slots:
    void on_pushButtonCut_clicked();

    void on_pushButtonCopy_clicked();

    void on_pushButtonInsertBefore_clicked();

    void on_pushButtonInsertBehind_clicked();

    void on_pushButtonImport_clicked();

    void on_pushButtonExport_clicked();

    void on_pushButtonAddBefore_clicked();

    void on_pushButtonAddBehind_clicked();

    void on_pushButtonChange_clicked();

    void on_listWidgetMeasurementTasks_itemClicked(QListWidgetItem *item);

    void on_checkBoxEnableConnection_clicked(bool checked);

private:
    Ui::ConfigureXCPOverEthernetWidget *ui;
    int VarListHasChanged;
    static int Connection;
    int CurrentConnection;
    QString tmpLabelName;
    void initData();
    void FillListBoxes();
};

#endif // CONFIGUREXCPOVERETHERNETWIDGET_H
