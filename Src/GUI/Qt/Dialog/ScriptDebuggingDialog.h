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


#ifndef SCRIPTDEBUGGINGDIALOG_H
#define SCRIPTDEBUGGINGDIALOG_H

#include <QDialog>
#include <QAction>
#include "ControlPanel.h"

namespace Ui {
class ScriptDebuggingDialog;
}

class ScriptDebuggingDialog : public QDialog
{
    Q_OBJECT

private:
    void WriteToIni();
    void AddBreakpoint();
    void DeleteBreakpoint(QString &Filename, int LineNr);
    int IsABreakpointOnThisLine (QString &Filename, int LineNr);
    QString ParseBreakpointLineFileName (QString &Line);
    int ParseBreakpointLineLineNr (QString &Line);

    void closeEvent(QCloseEvent *event);

signals:
    void SetScriptDebuggingWindowStateSignal(bool par_IsVisable);

public:
    explicit ScriptDebuggingDialog(QWidget *parent = nullptr);
    ~ScriptDebuggingDialog();

public slots:
    void CyclicUpdate();

private slots:
    void on_StopPushButton_clicked();
    void on_ContinuePushButton_clicked();
    void on_StepOverPushButton_clicked();
    void on_StepIntoPushButton_clicked();
    void on_StepOutpushButton_clicked();
    void on_AddPushButton_clicked();
    void on_DeletePushButton_clicked();
    void on_ScriptFileComboBox_currentIndexChanged(const QString &arg1);
    void on_StayOnTopCheckBox_clicked(bool checked);
    void on_StopAtStartCheckBox_clicked(bool checked);

    void OpenContextMenuSlot (const QPoint &pos);

    void AddBreakpointSlot();
    void DeleteBreakpointSlot();

private:
    Ui::ScriptDebuggingDialog *ui;
    QTimer *m_Timer;

    QAction *m_AddBereakpointAct;
    QAction *m_DeleteBereakpointAct;

    QString m_SelectedScriptFile;

    int m_ScriptFileCount;
    int m_DisplayedScriptNumber;
    int m_ChangedCounter;

    char *m_EnvironmentVariableListBuffer;
    int m_SizeOfEnvironmentVariableListBuffer;
    char *m_EnvironmentVariableListRefBuffer;
    int m_SizeOfEnvironmentVariableListRefBuffer;

    int m_StackChangedCounter;

    char *m_ParameterListBuffer;
    int m_SizeOfParameterListBuffer;
    char *m_ParameterListRefBuffer;
    int m_SizeOfParameterListRefBuffer;

    int m_BreakpointChangedCounter;

    int m_CyclicUpdateLastFile;
    int m_CyclicUpdateLastLine;
    int m_CyclicUpdateLasDelayCounter;

    int m_FileChangedByUserFileNo;  // -1 not changend
    bool m_WillChangedNoFromUser;

    int m_CurrentFileNo;
    int m_CurrentLineNo;
    int m_CurrentFileNoLastCycle;
    int m_CurrentLineNoLastCycle;
};

class ScriptDebuggingToGuiThread : public QObject
{
    Q_OBJECT

private:
    ScriptDebuggingDialog *m_ScriptDebuggingWindow;
    QWidget *m_Parent;    // This must point to the MainWindow so the script message window can be closed
    ControlPanel *m_ControlPanel;  // This have to point to the ControlPanel so the checkbox inside the ControlPanel can be changed

public:
    void OpenWindow(ControlPanel *par_ControlPanel, QWidget *par_Parent);
    void CloseWindow();
};

void InitScriptDebuggingWindow(ControlPanel *par_ControlPanel, QWidget *parent = 0);
void CloseScriptDebuggingWindow();
int IsScriptDebugWindowOpen();
int GetStopScriptAtStartDebuggingFlag();

#endif // SCRIPTDEBUGGINGDIALOG_H
