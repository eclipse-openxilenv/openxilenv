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


#ifndef OSCILLOSCOPESELECTVARIABLEDIALOG_H
#define OSCILLOSCOPESELECTVARIABLEDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <Dialog.h>

#include "OscilloscopeData.h"

namespace Ui {
class OscilloscopeSelectVariableDialog;
}

class OscilloscopeSelectVariableDialog : public Dialog
{
    Q_OBJECT

public:
    explicit OscilloscopeSelectVariableDialog(OSCILLOSCOPE_DATA *par_Data, QWidget *parent = 0);
    ~OscilloscopeSelectVariableDialog();

public slots:
    void accept();

private slots:

    void on_VariableListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

    void on_tabWidget_currentChanged(int index);

    void on_LineWidthSpinBox_valueChanged(int arg1);

    void on_ColorPushButton_clicked();

    void on_DeletePushButton_clicked();

private:
    void FillVarianleList();
    void SetColorView (QColor color);

    Ui::OscilloscopeSelectVariableDialog *ui;

    OSCILLOSCOPE_DATA *m_Data;
    INCLUDE_EXCLUDE_FILTER *m_IncExcFilter;

    double m_Min;
    double m_Max;
    bool m_DecPhysFlag;
    int m_DispayDecHexBin;
    int m_TextAlignment;
    bool m_Disable;

    int m_LineWidth;
    QColor m_Color;

};

#endif // OSCILLOSCOPESELECTVARIABLEDIALOG_H
