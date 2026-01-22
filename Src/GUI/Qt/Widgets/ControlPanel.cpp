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


#include "ControlPanel.h"
#include "ui_ControlPanel.h"

#include "ConfigureProcessDialog.h"

#include "FileDialog.h"
#include "StringHelpers.h"

#include "TraceConfiguration.h"
#include "StimuliConfiguration.h"
#include "StartProcessDialog.h"
#include "StopProcessDialog.h"
#include "BlackboardAccessDialog.h"
#include "BlackboardInfoDialog.h"
#include "MessageWindow.h"
#include "ScriptDebuggingDialog.h"
#include "QtIniFile.h"

extern "C" {
    #include "Config.h"
    #include "StringMaxChar.h"
    #include "Message.h"
    #include "ThrowError.h"
    #include "ConfigurablePrefix.h"
    #include "FileExtensions.h"
    #include "IniSectionEntryDefines.h"
    #include "Blackboard.h"
    #include "MainValues.h"
    #include "Scheduler.h"
}


extern int LaunchEditor(const char *szFilename);   // this is from MainWindow.cpp

ControlPanel::ControlPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlPanel)
{
    m_FirstCall = true;
    m_SchedulerUserStop = false;
    unsigned int loc_cycle_no = s_main_ini_val.NumberOfNextCycles;
    ui->setupUi(this);

    // The control panel should be always in foreground
    int WindowFlags = this->windowFlags();
    WindowFlags |= Qt::Tool;

    this->setWindowFlags (static_cast<Qt::WindowFlags>(WindowFlags));

    QString Title = QString(GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME));
    Title.append(" Control Panel");
    this->setWindowTitle(Title);

    runControlContinueClicked();
    ReadHistoryFromIni();
    ui->d_labelVariableCount->setText(QString().setNum(get_num_of_bbvaris()));

    m_ScriptStatusVid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_SCRIPT), BB_UNKNOWN_WAIT, "");
    m_GeneratorStatusVid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_GENERATOR), BB_UNKNOWN_WAIT, "");
    m_StimuliStatusVid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_STIMULUS_PLAYER), BB_UNKNOWN_WAIT, "");
    m_EqauationStatusVid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_EQUATION_CALCULATOR), BB_UNKNOWN_WAIT, "");
    m_RecorderStatusVid = add_bbvari (GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_TRACE_RECORDER), BB_UNKNOWN_WAIT, "");

    ui->d_checkBoxScriptMessage->setChecked(s_main_ini_val.ScriptMessageDialogVisable);

    m_variableCount = 0;
    ui->d_labelVariableCount->setText(QString("0"));

    ui->d_spinBoxCycles->setMinimum(1);
    ui->d_spinBoxCycles->setValue(static_cast<int>(loc_cycle_no));

    if (s_main_ini_val.ConnectToRemoteMaster) {
        ui->d_groupBoxRunControl->setDisabled(true);
        ui->d_pushBtnReset->setDisabled(true);
    }

    m_SystemDisableCounter = 0;
    m_UserDisableCounter = 0;
    m_RpcDisableCounter = 0;
}

void ControlPanel::runControlContinueClicked()
{
    ui->d_pushBtnContinue->setEnabled(false);
    ui->d_pushBtnNext->setEnabled(false);
    ui->d_pushBtnNextOne->setEnabled(false);
    ui->d_pushBtnStop->setEnabled(true);
}

void ControlPanel::runControlStopClicked()
{
    ui->d_pushBtnContinue->setEnabled(true);
    ui->d_pushBtnNext->setEnabled(true);
    ui->d_pushBtnNextOne->setEnabled(true);
    ui->d_pushBtnStop->setEnabled(false);
}

void ControlPanel::closeEvent(QCloseEvent *event)
{
    this->setVisible(false);
    emit hideControlPanel();
    event->ignore();
}

void ControlPanel::btnRunControlContinue()
{
    SetSchedulerUserState(false);
    enable_scheduler(SCHEDULER_CONTROLED_BY_USER);
}

void ControlPanel::btnRunControlStop()
{
    SetSchedulerUserState(true);
    disable_scheduler_at_end_of_cycle(SCHEDULER_CONTROLED_BY_USER, nullptr, nullptr);
}

void ControlPanel::btnRunControlNextOne()
{
    SetSchedulerUserState(false);
    make_n_next_cycles(SCHEDULER_CONTROLED_BY_USER, 1, nullptr, nullptr, nullptr);
}

void ControlPanel::btnRunControlNext()
{
    int Cycles = ui->d_spinBoxCycles->value();
    SetSchedulerUserState(false);
    make_n_next_cycles(SCHEDULER_CONTROLED_BY_USER, Cycles, nullptr, nullptr, nullptr);
}

void ControlPanel::btnRunControlBreakpoint(int value)
{
}

void ControlPanel::btnProcessAdd()
{
    StartProcessDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void ControlPanel::btnProcessRemove()
{
    StopProcessDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void ControlPanel::btnProcessInfo()
{
    ConfigureProcessDialog *Dlg;

    Dlg = new ConfigureProcessDialog (QString());
    if (Dlg->exec() == QDialog::Accepted) {
        ;
    }
    delete Dlg;
}

void ControlPanel::btnCycleCountReset()
{
    write_bbvari_udword (CycleCounterVid, 0);
}

void ControlPanel::btnBlackboardAccess()
{
    BlackboardAccessDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void ControlPanel::btnBlackboardDisplay()
{
}

void ControlPanel::btnBlackboardInfo()
{
    BlackboardInfoDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void ControlPanel::btnInternalProcessRun()
{
}

void ControlPanel::btnInternalProcessStop()
{
}

void ControlPanel::btnInternalProcessConfig()
{
}

void ControlPanel::btnInternalProcessFile()
{
}


void ControlPanel::btnInternalProcessScript(bool value)
{
    if(value) {
        // nothing
    }
}

void ControlPanel::btnInternalProcessGenerator(bool value)
{
    if(value) {
        // nothing
    }
}

void ControlPanel::btnInternalProcessStimuli(bool value)
{
    if(value) {
        // nothing
    }
}

void ControlPanel::btnInternalProcessEquation(bool value)
{
    if(value) {
        // nothing
    }
}

void ControlPanel::btnInternalProcessTrace(bool value)
{
    if(value) {
        // nothing
    }
}

void ControlPanel::ReadHistoryFromIni()
{
    ReadHistoryOf(LAST_10_BP_SECT, "B", ui->d_comboBoxBreakpoints);
    ReadHistoryOf(LAST_10_SCR_SECT, "F", ui->d_comboBoxScriptFile);
    ReadHistoryOf(LAST_10_GEN_SECT, "F", ui->d_comboBoxGeneratorFile);
    ReadHistoryOf(LAST_10_REC_SECT, "F", ui->d_comboBoxTraceFile);
    ReadHistoryOf(LAST_10_PLY_SECT, "F", ui->d_comboBoxStimuliFile);
    ReadHistoryOf(LAST_10_TRG_SECT, "F", ui->d_comboBoxEquationFile);
}


void ControlPanel::SaveHistoryToIni()
{
    s_main_ini_val.NumberOfNextCycles = static_cast<unsigned int>(ui->d_spinBoxCycles->value());
    WriteHistoryOf(LAST_10_BP_SECT, "B", ui->d_comboBoxBreakpoints);
    WriteHistoryOf(LAST_10_SCR_SECT, "F", ui->d_comboBoxScriptFile);
    WriteHistoryOf(LAST_10_GEN_SECT, "F", ui->d_comboBoxGeneratorFile);
    WriteHistoryOf(LAST_10_REC_SECT, "F", ui->d_comboBoxTraceFile);
    WriteHistoryOf(LAST_10_PLY_SECT, "F", ui->d_comboBoxStimuliFile);
    WriteHistoryOf(LAST_10_TRG_SECT, "F", ui->d_comboBoxEquationFile);
}

QString ControlPanel::GetActualScriptFilename()
{
    return ui->d_comboBoxScriptFile->currentText();
}

void ControlPanel::DeactivateBreakpoint()
{
    ui->d_checkBoxBreakpoint->setChecked(false);
}

void ControlPanel::CyclicUpdate()
{
    if (m_FirstCall) { // workaround to be small as possible
        resize(10,10);
        m_FirstCall = false;
    }
    unsigned int loc_variableCount = get_num_of_bbvaris();
    if(m_variableCount != loc_variableCount) {
        ui->d_labelVariableCount->setText(QString().setNum(loc_variableCount));
        m_variableCount = loc_variableCount;
    }
    if (!s_main_ini_val.ConnectToRemoteMaster) {
        int SystemDisableCounter;
        int UserDisableCounter;
        int RpcDisableCounter;
        GetStopRequestCounters (&SystemDisableCounter, &UserDisableCounter, &RpcDisableCounter);

        // change the run control buttons
        SetSchedulerUserState(UserDisableCounter > 0);

        if ((m_SystemDisableCounter != SystemDisableCounter) ||
            (m_UserDisableCounter != UserDisableCounter) ||
            (m_RpcDisableCounter != RpcDisableCounter)) {
            QString Title = "Run-Control";
            if ((SystemDisableCounter) ||
                (UserDisableCounter) ||
                (RpcDisableCounter)) {
                Title.append(" (stopped by:");
                bool Comma = false;
                if (SystemDisableCounter) {
                    Title.append(" System=");
                    Title.append(QString().number(SystemDisableCounter));
                    Comma = true;
                }
                if (UserDisableCounter) {
                    if (Comma) Title.append(", User=");
                    else Title.append(" User=");
                    Title.append(QString().number(UserDisableCounter));
                    Comma = true;
                }
                if (RpcDisableCounter) {
                    if (Comma) Title.append(", RPC=");
                    else Title.append(" RPC=");
                    Title.append(QString().number(RpcDisableCounter));
                }
                Title.append(")");
            }
            ui->d_groupBoxRunControl->setTitle(Title);
            m_SystemDisableCounter = SystemDisableCounter;
            m_UserDisableCounter = UserDisableCounter;
            m_RpcDisableCounter = RpcDisableCounter;
        }
    }
    // Cycle counter
    uint32_t Cycles = read_bbvari_udword (CycleCounterVid);
    ui->d_labelCycleCount->setText(QString().setNum(Cycles));

    // update the avtive internal process state display
    UpdateStatusDisplay();
}

void ControlPanel::SetMessageWindowStateSlot(bool par_IsVisable)
{
    ui->d_checkBoxScriptMessage->setChecked(par_IsVisable);
}

void ControlPanel::SetScriptDebugWindowStateSlot(bool par_IsVisable)
{
    ui->d_checkBoxScriptDebugging->setChecked(par_IsVisable);
}

void ControlPanel::UpdateStatusDisplay()
{
    char Text[256];
    int Color;

    STRING_COPY_TO_ARRAY (Text, "unknown");
    switch (ui->d_tabWidget->currentIndex()) {
    case 0:  // Script
        if (read_bbvari_textreplace (m_ScriptStatusVid, Text, sizeof (Text), &Color) == 0) {
            ui->d_labelScriptStatusPrint->setText (QString(Text));
        } else {
            ui->d_labelScriptStatusPrint->setText ("unknown");
        }
        break;
    case 1:  // Generator
        if (read_bbvari_textreplace (m_GeneratorStatusVid, Text, sizeof (Text), &Color) == 0) {
            ui->d_labelGeneratorStatusPrint->setText (QString(Text));
        } else {
            ui->d_labelGeneratorStatusPrint->setText ("unknown");
        }
        break;
    case 2:  // Stimulus
        if (read_bbvari_textreplace (m_StimuliStatusVid, Text, sizeof (Text), &Color) == 0) {
            ui->d_labelStimuliStatusPrint->setText (QString(Text));
        } else {
            ui->d_labelStimuliStatusPrint->setText ("unknown");
        }
        break;
    case 3:  // Equation
        if (read_bbvari_textreplace (m_EqauationStatusVid, Text, sizeof (Text), &Color) == 0) {
            ui->d_labelEquationStatusPrint->setText (QString(Text));
        } else {
            ui->d_labelEquationStatusPrint->setText ("unknown");
        }
        break;
    case 4:  // Trace
        if (read_bbvari_textreplace (m_RecorderStatusVid, Text, sizeof (Text), &Color) == 0) {
            ui->d_labelTraceStatusPrint->setText (QString(Text));
        } else {
            ui->d_labelTraceStatusPrint->setText ("unknown");
        }
        break;
    }
}

ControlPanel::~ControlPanel()
{
    remove_bbvari (m_ScriptStatusVid);
    remove_bbvari (m_GeneratorStatusVid);
    remove_bbvari (m_StimuliStatusVid);
    remove_bbvari (m_EqauationStatusVid);
    remove_bbvari (m_RecorderStatusVid);

    SaveHistoryToIni();
    delete ui;
}

void ControlPanel::on_d_pushBtnScriptRun_clicked()
{
    // Copy file name to global variable and set flag to start script
    if (script_status_flag != RUNNING) {
         QString ScriptFile = ui->d_comboBoxScriptFile->currentText();
         if (!ScriptFile.isEmpty()) {
             STRING_COPY_TO_ARRAY (script_filename, QStringToConstChar(ScriptFile));
             script_status_flag = START;
         }
    }
}

void ControlPanel::on_d_pushBtnTraceFile_clicked()
{
    QString TraceFile = ui->d_comboBoxTraceFile->currentText();
    TraceFile = FileDialog::getOpenFileName (this, QString("Select file"),
                                               TraceFile,
                                               QString(RECORDER_EXT));
    if (!TraceFile.isEmpty()) {
        ui->d_comboBoxTraceFile->insertItem(0, TraceFile);
        ui->d_comboBoxTraceFile->setCurrentText(TraceFile);
    }
}

void ControlPanel::on_d_pushBtnScriptFile_clicked()
{
    QString ScriptFile = ui->d_comboBoxScriptFile->currentText();
    ScriptFile = FileDialog::getOpenFileName (this, QString("Select file"),
                                               ScriptFile,
                                               QString(SCRIPT_EXT));
    if (!ScriptFile.isEmpty()) {
        ui->d_comboBoxScriptFile->insertItem(0, ScriptFile);
        ui->d_comboBoxScriptFile->setCurrentText(ScriptFile);
    }
}

void ControlPanel::on_d_pushBtnGeneratorFile_clicked()
{
    QString GeneratorFile = ui->d_comboBoxGeneratorFile->currentText();
    GeneratorFile = FileDialog::getOpenFileName (this, QString("Select file"),
                                               GeneratorFile,
                                               QString(GENERATOR_EXT));
    if (!GeneratorFile.isEmpty()) {
        ui->d_comboBoxGeneratorFile->insertItem(0, GeneratorFile);
        ui->d_comboBoxGeneratorFile->setCurrentText(GeneratorFile);
    }
}

void ControlPanel::on_d_pushBtnStimuliFile_clicked()
{
    QString StimuliFile = ui->d_comboBoxStimuliFile->currentText();
    StimuliFile = FileDialog::getOpenFileName (this, QString("Select file"),
                                               StimuliFile,
                                               QString(PLAYER_EXT));
    if (!StimuliFile.isEmpty()) {
        ui->d_comboBoxStimuliFile->insertItem(0, StimuliFile);
        ui->d_comboBoxStimuliFile->setCurrentText(StimuliFile);
    }
}

void ControlPanel::on_d_pushBtnEquationFile_clicked()
{
    QString EquationFile = ui->d_comboBoxEquationFile->currentText();
    EquationFile = FileDialog::getOpenFileName (this, QString("Select file"),
                                               EquationFile,
                                               QString(TRIGGER_EXT));
    if (!EquationFile.isEmpty()) {
        ui->d_comboBoxEquationFile->insertItem(0, EquationFile);
        ui->d_comboBoxEquationFile->setCurrentText(EquationFile);
    }
}

void ControlPanel::on_d_pushBtnGeneratorRun_clicked()
{
    QString GeneratorFile = ui->d_comboBoxGeneratorFile->currentText();
    if (!GeneratorFile.isEmpty()) {
        // Transmit message to internal process if it is running
        int pid = get_pid_by_name (PN_SIGNAL_GENERATOR_COMPILER);
        if (pid > 0) {
             write_message (pid, RAMPEN_FILENAME, static_cast<int>(strlen(QStringToConstChar(GeneratorFile)) + 1),
                           QStringToConstChar(GeneratorFile));
            write_message (pid, RAMPEN_START, 0, nullptr);
        } else {
            ThrowError (1, "Generator process \"%s\" are not running", PN_SIGNAL_GENERATOR_COMPILER);
        }
    }
}

void ControlPanel::on_d_pushBtnStimuliRun_clicked()
{
    QString StimuliFile = ui->d_comboBoxStimuliFile->currentText();
    if (!StimuliFile.isEmpty()) {
        // Transmit message to internal process if it is running
        int pid = get_pid_by_name (PN_STIMULI_PLAYER);
        if (pid > 0) {
            write_message (pid, HDPLAY_CONFIG_FILENAME_MESSAGE,
                          static_cast<int>(strlen(QStringToConstChar(StimuliFile)) + 1), QStringToConstChar(StimuliFile));
        } else {
            ThrowError (1, "Stimuli process \"%s\" are not running", PN_STIMULI_PLAYER);
        }
    }
}

void ControlPanel::on_d_pushBtnEquationRun_clicked()
{
    QString EquationFile = ui->d_comboBoxEquationFile->currentText();
    if (!EquationFile.isEmpty()) {
        // Transmit message to internal process if it is running
        int pid = get_pid_by_name (PN_EQUATION_COMPILER);
        if (pid > 0) {
            write_message (pid, TRIGGER_FILENAME_MESSAGE,
                           static_cast<int>(strlen(QStringToConstChar(EquationFile)) + 1), QStringToConstChar(EquationFile));
        } else {
            ThrowError (1, "Equation process \"%s\" are not running", PN_EQUATION_COMPILER);
        }
    }
}

void ControlPanel::on_d_pushBtnTraceRun_clicked()
{
    QString TraceFile = ui->d_comboBoxTraceFile->currentText();
    if (!TraceFile.isEmpty()) {
        // Transmit message to internal process if it is running
        int pid = get_pid_by_name(PN_TRACE_RECORDER);
        if (pid > 0) {
            write_message (pid, REC_CONFIG_FILENAME_MESSAGE,
                           static_cast<int>(strlen(QStringToConstChar(TraceFile)) + 1), QStringToConstChar(TraceFile));
        } else {
            ThrowError (1, "Trace process \"%s\" are not running", PN_TRACE_RECORDER);
        }
    }
}

void ControlPanel::on_d_pushBtnScriptStop_clicked()
{
    // reset the flag
    script_status_flag = 0;
}

void ControlPanel::on_d_pushBtnGeneratorStop_clicked()
{
    // Transmit message to internal process if it is running
    int pid = get_pid_by_name (PN_SIGNAL_GENERATOR_COMPILER);
    if (pid > 0) {
        write_message (pid, RAMPEN_STOP, 0, nullptr);
    }
}

void ControlPanel::on_d_pushBtnStimuliStop_clicked()
{
    // Transmit message to internal process if it is running
    int pid = get_pid_by_name(PN_STIMULI_PLAYER);
    if (pid > 0) {
        write_message(pid, HDPLAY_STOP_MESSAGE, 0, nullptr);
    }

}

void ControlPanel::on_d_pushBtnEquationStop_clicked()
{
    // Transmit message to internal process if it is running
    int pid = get_pid_by_name (PN_EQUATION_COMPILER);
    if (pid > 0) {
        write_message (pid, TRIGGER_STOP_MESSAGE, 0, nullptr);
    }
}

void ControlPanel::on_d_pushBtnTraceStop_clicked()
{
    // Transmit message to internal process if it is running
    int pid = get_pid_by_name (PN_TRACE_RECORDER);
    if (pid > 0) {
      write_message (pid, STOP_REC_MESSAGE, 0, nullptr);
    }
}

void ControlPanel::on_d_pushBtnScriptConfig_clicked()
{
    QString ScriptFile = ui->d_comboBoxScriptFile->currentText();
    if (!ScriptFile.isEmpty()) {
        LaunchEditor (QStringToConstChar(ScriptFile));
    }
}

void ControlPanel::on_d_pushBtnGeneratorConfig_clicked()
{
    QString GeneratorFile = ui->d_comboBoxGeneratorFile->currentText();
    if (!GeneratorFile.isEmpty()) {
        LaunchEditor (QStringToConstChar(GeneratorFile));
    }
}

void ControlPanel::on_d_pushBtnStimuliConfig_clicked()
{
    QString ConfigFileName = ui->d_comboBoxStimuliFile->currentText();
    StimuliConfiguration Dlg (ConfigFileName);
    if (Dlg.exec() == QDialog::Accepted) {
        ConfigFileName = Dlg.GetConfigurationFileName();
        if (ui->d_comboBoxStimuliFile->findText (ConfigFileName, Qt::MatchFixedString) < 0) {
            ui->d_comboBoxStimuliFile->addItem (ConfigFileName);
        }
        ui->d_comboBoxStimuliFile->setCurrentText (ConfigFileName);
    }
}

void ControlPanel::on_d_pushBtnEquationConfig_clicked()
{
    QString EquationFile = ui->d_comboBoxEquationFile->currentText();
    if (!EquationFile.isEmpty()) {
        LaunchEditor (QStringToConstChar(EquationFile));
    }
}

void ControlPanel::on_d_pushBtnTraceConfig_clicked()
{
    QString ConfigFileName = ui->d_comboBoxTraceFile->currentText();
    TraceConfiguration Dlg (ConfigFileName);
    if (Dlg.exec() == QDialog::Accepted) {
        ConfigFileName = Dlg.GetConfigurationFileName();
        if (ui->d_comboBoxTraceFile->findText (ConfigFileName, Qt::MatchFixedString) < 0) {
            ui->d_comboBoxTraceFile->addItem (ConfigFileName);
        }
        ui->d_comboBoxTraceFile->setCurrentText (ConfigFileName);
    }
}

void ControlPanel::on_d_checkBoxScriptMessage_stateChanged(int arg1)
{
    s_main_ini_val.ScriptMessageDialogVisable = (arg1 != 0);
    if(!s_main_ini_val.ScriptMessageDialogVisable) {
        CloseMessageWindow();
    }
}

void ControlPanel::on_d_checkBoxScriptDebugging_stateChanged(int arg1)
{
    s_main_ini_val.ScriptDebugDialogVisable = static_cast<char>(arg1);
    if(arg1) {
        InitScriptDebuggingWindow (this, this);
    } else{
        CloseScriptDebuggingWindow();
    }
}

void ControlPanel::on_d_checkBoxBreakpoint_toggled(bool checked)
{
    if (checked) {
        QString Breakpoint = ui->d_comboBoxBreakpoints->currentText();
        if (Breakpoint.size() > 0) {
            SetSchedulerBreakPoint (QStringToConstChar(Breakpoint));
        }
    } else {
        SetSchedulerBreakPoint (nullptr);  // deactivate the breakpoint
    }
}

void ControlPanel::on_d_comboBoxBreakpoints_currentTextChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    ui->d_checkBoxBreakpoint->setChecked(false);
}

void ControlPanel::ComboBoxBreakpointsEditingFinished()
{
    QString Current = ui->d_comboBoxBreakpoints->currentText();
    if (Current.size() == 0) {
        ui->d_comboBoxBreakpoints->removeItem(ui->d_comboBoxBreakpoints->currentIndex());
    }
}

void ControlPanel::ComboBoxScriptFileEditingFinished()
{
    QString Current = ui->d_comboBoxScriptFile->currentText();
    if (Current.size() == 0) {
        ui->d_comboBoxScriptFile->removeItem(ui->d_comboBoxScriptFile->currentIndex());
    }
}

void ControlPanel::ComboBoxGeneratorFileEditingFinished()
{
    QString Current = ui->d_comboBoxGeneratorFile->currentText();
    if (Current.size() == 0) {
        ui->d_comboBoxGeneratorFile->removeItem(ui->d_comboBoxGeneratorFile->currentIndex());
    }
}

void ControlPanel::ComboBoxTraceFileEditingFinished()
{
    QString Current = ui->d_comboBoxTraceFile->currentText();
    if (Current.size() == 0) {
        ui->d_comboBoxTraceFile->removeItem(ui->d_comboBoxTraceFile->currentIndex());
    }
}

void ControlPanel::ComboBoxStimuliFileEditingFinished()
{
    QString Current = ui->d_comboBoxStimuliFile->currentText();
    if (Current.size() == 0) {
        ui->d_comboBoxStimuliFile->removeItem(ui->d_comboBoxStimuliFile->currentIndex());
    }
}

void ControlPanel::ComboBoxEquationFileEditingFinished()
{
    QString Current = ui->d_comboBoxEquationFile->currentText();
    if (Current.size() == 0) {
        ui->d_comboBoxEquationFile->removeItem(ui->d_comboBoxEquationFile->currentIndex());
    }
}


void ControlPanel::WriteHistoryOf(QString par_Section, QString par_EntryPrefix, QComboBox *par_Compbox)
{
    int Fd = ScQt_GetMainFileDescriptor();

    ScQt_IniFileDataBaseWriteString (par_Section, nullptr, nullptr, Fd);  // delete the section
    int pos = par_Compbox->currentIndex();
    int size  = par_Compbox->count();
    QString History = par_Compbox->currentText();
    if (pos >= 0) { // It was a selected item
        History = par_Compbox->currentText();
    }
    if (History.size()) {
        QString Entry = par_EntryPrefix;
        Entry.append(QString().number(1));
        ScQt_IniFileDataBaseWriteString (par_Section, Entry, History, Fd);
        if (History.compare(par_Compbox->itemText(pos))) {
            for (int i = 0; (i < size) && (i < 10); i++) {
                History = par_Compbox->itemText(i);
                Entry = par_EntryPrefix;
                Entry.append(QString().number(i + 2));
                ScQt_IniFileDataBaseWriteString (par_Section, Entry, History, Fd);
            }
        } else {
            for (int i = 0; (i < pos) && (i < 10); i++) {
                History = par_Compbox->itemText(i);
                Entry = par_EntryPrefix;
                Entry.append(QString().number(i + 2));
                ScQt_IniFileDataBaseWriteString (par_Section, Entry, QStringToConstChar(History), Fd);
            }
            for (int i = pos + 1; (i < size) && (i < 10); i++) {
                History = par_Compbox->itemText(i);
                Entry = par_EntryPrefix;
                Entry.append(QString().number(i + 1));
                ScQt_IniFileDataBaseWriteString (par_Section, Entry, QStringToConstChar(History), Fd);
            }
        }
    } else {
        for (int i = 0; (i < size) && (i < 10); i++) {
            History = par_Compbox->itemText(i);
            QString Entry = par_EntryPrefix;
            Entry.append(QString().number(i + 1));
            ScQt_IniFileDataBaseWriteString (par_Section, Entry, QStringToConstChar(History), Fd);
        }
    }
}

void ControlPanel::ReadHistoryOf(QString par_Section, QString par_EntryPrefix, QComboBox *par_Compbox)
{
    int i;

    par_Compbox->clear();
    for (i = 0; i < 10; i++) {
        QString Entry = par_EntryPrefix;
        Entry.append(QString().number(i + 1));
        QString Line = ScQt_IniFileDataBaseReadString(par_Section, Entry, "", ScQt_GetMainFileDescriptor());
        if (Line.isEmpty()) {
            break;
        }
        par_Compbox->addItem(Line);
    }
    par_Compbox->setCurrentIndex(0);
}


void ControlPanel::on_d_spinBoxCycles_valueChanged(int arg1)
{
    s_main_ini_val.NumberOfNextCycles = static_cast<unsigned int>(arg1);
}

void ControlPanel::SetSchedulerUserState(bool par_Stop)
{
    if (par_Stop != m_SchedulerUserStop) {
        ui->d_pushBtnContinue->setEnabled(par_Stop);
        ui->d_pushBtnNext->setEnabled(par_Stop);
        ui->d_pushBtnNextOne->setEnabled(par_Stop);
        ui->d_pushBtnStop->setEnabled(!par_Stop);
        // inform the main window toolbar
        if (par_Stop) {
            runControlStop();
        } else {
            runControlContinue();
        }
        m_SchedulerUserStop = par_Stop;
    }
}



void ControlPanel::on_d_pushBtnAddBBVariable_clicked()
{
    emit openAddBBDialog();
}

