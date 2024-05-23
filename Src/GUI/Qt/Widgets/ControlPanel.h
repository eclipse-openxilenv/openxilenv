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


#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <QWidget>
#include <QCloseEvent>
#include <QMessageBox>
#include <QComboBox>
#include <QTimer>
extern "C"{
#include "Scheduler.h"
#include <InterfaceToScript.h>
#include "MainValues.h"
#include "BlackboardAccess.h"
}

namespace Ui {
class ControlPanel;
}

class ControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ControlPanel(QWidget *parent = nullptr);
    ~ControlPanel();
    void updateCycleCount(ulong);
    void runControlContinueClicked();
    void runControlStopClicked();
    void SaveHistoryToIni();
    QString GetActualScriptFilename();
    void DeactivateBreakpoint();

public slots:
    void CyclicUpdate();
    void SetMessageWindowStateSlot(bool par_IsVisable);
    void SetScriptDebugWindowStateSlot(bool par_IsVisable);


private:
    void closeEvent(QCloseEvent *event);
    void ReadHistoryFromIni();
    void UpdateStatusDisplay();
private slots:
    void btnRunControlContinue();
    void btnRunControlStop();
    void btnRunControlNextOne();
    void btnRunControlNext();
    void btnRunControlBreakpoint(int value);
    void btnProcessAdd();
    void btnProcessRemove();
    void btnProcessInfo();
    void btnCycleCountReset();
    void btnBlackboardAccess();
    void btnBlackboardDisplay();
    void btnBlackboardInfo();
    void btnInternalProcessRun();
    void btnInternalProcessStop();
    void btnInternalProcessConfig();
    void btnInternalProcessFile();
    void btnInternalProcessScript(bool value);
    void btnInternalProcessGenerator(bool value);
    void btnInternalProcessStimuli(bool value);
    void btnInternalProcessEquation(bool value);
    void btnInternalProcessTrace(bool value);
    void on_d_pushBtnScriptRun_clicked();

    void on_d_pushBtnTraceFile_clicked();

    void on_d_pushBtnScriptFile_clicked();

    void on_d_pushBtnGeneratorFile_clicked();

    void on_d_pushBtnStimuliFile_clicked();

    void on_d_pushBtnEquationFile_clicked();

    void on_d_pushBtnGeneratorRun_clicked();

    void on_d_pushBtnStimuliRun_clicked();

    void on_d_pushBtnEquationRun_clicked();

    void on_d_pushBtnTraceRun_clicked();

    void on_d_pushBtnScriptStop_clicked();

    void on_d_pushBtnGeneratorStop_clicked();

    void on_d_pushBtnStimuliStop_clicked();

    void on_d_pushBtnEquationStop_clicked();


    void on_d_pushBtnTraceStop_clicked();

    void on_d_pushBtnScriptConfig_clicked();

    void on_d_pushBtnGeneratorConfig_clicked();

    void on_d_pushBtnStimuliConfig_clicked();

    void on_d_pushBtnEquationConfig_clicked();

    void on_d_pushBtnTraceConfig_clicked();

    void on_d_checkBoxScriptMessage_stateChanged(int arg1);

    void on_d_checkBoxScriptDebugging_stateChanged(int arg1);

    void on_d_checkBoxBreakpoint_toggled(bool checked);

    void on_d_comboBoxBreakpoints_currentTextChanged(const QString &arg1);

    void ComboBoxBreakpointsEditingFinished();
    void ComboBoxScriptFileEditingFinished();
    void ComboBoxGeneratorFileEditingFinished();
    void ComboBoxTraceFileEditingFinished();
    void ComboBoxStimuliFileEditingFinished();
    void ComboBoxEquationFileEditingFinished();

    void on_d_spinBoxCycles_valueChanged(int arg1);
    void on_d_pushBtnAddBBVariable_clicked();

signals:
    void hideControlPanel(void);
    void runControlContinue(void);
    void runControlStop(void);
    void openAddBBDialog(void);

private:
    void SetSchedulerUserState(bool par_Stop);
    void WriteHistoryOf(QString par_Section, QString par_EntryPrefix, QComboBox *par_Compbox);
    void ReadHistoryOf(QString par_Section, QString par_EntryPrefix, QComboBox *par_Compbox);
    Ui::ControlPanel *ui;

    int m_ScriptStatusVid;
    int m_GeneratorStatusVid;
    int m_StimuliStatusVid;
    int m_EqauationStatusVid;
    int m_RecorderStatusVid;
    unsigned int m_variableCount;

    int m_SystemDisableCounter;
    int m_UserDisableCounter;
    int m_RpcDisableCounter;

    bool m_SchedulerUserStop;

    bool m_FirstCall;
};

#endif // CONTROLPANEL_H
