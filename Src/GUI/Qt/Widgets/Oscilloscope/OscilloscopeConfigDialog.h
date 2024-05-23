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


#ifndef OSCILLOSCOPECONFIGDIALOG_H
#define OSCILLOSCOPECONFIGDIALOG_H

#include <QDialog>
#include <QSortFilterProxyModel>

#include "BlackboardVariableModel.h"
#include "OscilloscopeData.h"
#include <Dialog.h>


namespace Ui {
class OscilloscopeConfigDialog;
}

class OscilloscopeConfigDialog : public Dialog
{
    Q_OBJECT

public:
    explicit OscilloscopeConfigDialog(OSCILLOSCOPE_DATA *par_Data, QString par_WindowTitle,
                                      QWidget *parent = nullptr);
    ~OscilloscopeConfigDialog();

    bool OnOffDescrFrameChanged();

    bool WindowTitleChanged();
    QString GetWindowTitle();

public slots:
    void accept();

private slots:
    void on_TriggerCheckBox_clicked(bool checked);

    void on_BackgroundImagePushButton_clicked();

private:
    void EnableDisableTrigger(bool par_Enable, bool par_Init = false);
    Ui::OscilloscopeConfigDialog *ui;

    OSCILLOSCOPE_DATA *m_Data;

    double m_WindowStartTimeSave;
    double m_WindowTimeSizeSave;
    double m_WindowEndTimeSave;
    double m_WindowSizeTimeConfigeredSave;

    bool m_OnOffDescrFrameChanged;
    bool m_WindowTitleChanged;

    QString m_WindowTitleSave;
    BlackboardVariableModel *m_Model;
    QSortFilterProxyModel *m_SortModel;
};

#endif // OSCILLOSCOPECONFIGDIALOG_H
