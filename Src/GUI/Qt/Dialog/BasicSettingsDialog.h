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


#ifndef BASICSETTINGSDIALOG_H
#define BASICSETTINGSDIALOG_H

#include <QDialog>
#include <Dialog.h>


namespace Ui {
class BasicSettingsDialog;
}

class BasicSettingsDialog : public Dialog
{
    Q_OBJECT

public:
    explicit BasicSettingsDialog(QWidget *parent = nullptr);
    ~BasicSettingsDialog();

public slots:
    void accept();

private slots:
    void on_DisplStatObjCheckBox_clicked(bool checked);

    void on_AutosaveCheckBox_clicked(bool checked);

    void on_SelectStartupAppStyleCheckBox_toggled(bool checked);

    void on_DefaultsTextDisplayFontCheckBox_toggled(bool checked);

    void on_DefaultsOscilloscopeFontCheckBox_toggled(bool checked);

    void on_DefaultsOscilloscopeFontPushButton_clicked();

    void on_DefaultsTextFontPushButton_clicked();

    void on_ReplaceAllOtherCharsCheckBox_toggled(bool checked);

    void on_RemoteMasterExecutablePushButton_clicked();

    void on_EnableRemoteMasterCheckBox_stateChanged(int arg1);

    void on_CopyAndStartRemoteMasterCheckBox_stateChanged(int arg1);

    void on_CommandNameCheckBox_stateChanged(int arg1);

    void on_DarkModeCheckBox_clicked(bool checked);

private:
    void RemoveWhitespacesOnlyAtBeginingAndEnd (char *p);

    Ui::BasicSettingsDialog *ui;
};

#endif // BASICSETTINGSDIALOG_H
