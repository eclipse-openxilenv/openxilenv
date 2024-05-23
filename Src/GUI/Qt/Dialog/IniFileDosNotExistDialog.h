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


#ifndef INIFILEDOSNOTEXIST_H
#define INIFILEDOSNOTEXIST_H

#ifdef __cplusplus

#include <QDialog>
#include <QTimer>

namespace Ui {
class IniFileDosNotExist;
}

class IniFileDosNotExist : public QDialog
{
    Q_OBJECT

public:
    explicit IniFileDosNotExist (char *par_SelectedIniFile, void *par_Application, QWidget *parent = nullptr);
    ~IniFileDosNotExist();
    int GetNewSelectedIni (char *ret_NewSelectedIniFile, int par_MaxChars);
    bool GetDarkMode();

private slots:
    void on_ChooseExistingPushButton_clicked();

    void on_ExitImmediatelyPushButton_clicked();

    void on_GenerateNewOnePushButton_clicked();

    void on_OpenLastOnePushButton_clicked();

    void on_DefaultModeRadioButton_clicked();
    void on_NormalModeRadioButton_clicked();
    void on_DarkModeRadioButton_clicked();

    void TimerUpdateSoftIceIcon();

private:
    int BuidANewIniFile ();

    void ChangeDarkMode(bool par_DarkMode);

    Ui::IniFileDosNotExist *ui;

    QString m_HistoryFile;
    QString m_NewSelectedIniFile;
    QTimer *m_Timer;
    int m_Counter;
    QApplication *m_Application;
    int m_Fd;
    bool m_DarkModeHasChanged;
    enum {DEFAULT_MODE, NORMAL_MODE, DARK_MODE} m_DarkMode;
};

extern "C" {
#endif
    int IniFileDosNotExistDialog (char *SelectedIniFile, char *ret_NewSelectedIniFile, int par_MaxChars,
                                  void *par_Application);
#ifdef __cplusplus
}
#endif

#endif // INIFILEDOSNOTEXIST_H
