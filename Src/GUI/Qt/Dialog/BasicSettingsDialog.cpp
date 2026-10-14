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


#include <QStyleFactory>
#include <QFontDialog>
#include <QApplication>

#include "Platform.h"

#include "BasicSettingsDialog.h"
#include "ui_BasicSettingsDialog.h"

#include "DarkMode.h"
#include "FileDialog.h"
#include "MainWinowSyncWithOtherThreads.h"
#include "StringHelpers.h"

extern "C"
{
    #include "MainValues.h"
    #include "ThrowError.h"
    #include "StringMaxChar.h"
    #include "PrintFormatToString.h"
    #include "ConfigurablePrefix.h"
    #include "EnvironmentVariables.h"
    #include "IniDataBase.h"
    #include "InitProcess.h"
    #include "Scheduler.h"
    #include "InterfaceToScript.h"
    #include "FileExtensions.h"
    #include "RpcSocketServer.h"
}


BasicSettingsDialog::BasicSettingsDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::BasicSettingsDialog)
{
    ui->setupUi(this);

    ui->ProjectNameLineEdit->setText(QString (s_main_ini_val.ProjectName));
    ui->WorkDirectoryLineEdit->setText(QString (s_main_ini_val.WorkDir));
    ui->SchedulingLineEdit->SetValue(s_main_ini_val.SchedulerPeriode);
    ui->BlackboardElemLineEdit->SetOnlyInteger(true);
    ui->BlackboardElemLineEdit->SetValue(s_main_ini_val.BlackboardSize);
#ifdef _WIN32
    ui->EditorLineEdit->setText(QString (s_main_ini_val.Editor));
#else
    ui->EditorLineEdit->setText(QString (s_main_ini_val.EditorX));
#endif
    ui->WriteProcessListCheckBox->setChecked(s_main_ini_val.WriteProtectIniProcessList != 0);
    ui->AutosaveCheckBox->setChecked(s_main_ini_val.SwitchAutomaticSaveIniOff != 0);
    ui->SaveFileCheckBox->setEnabled(s_main_ini_val.SwitchAutomaticSaveIniOff != 0);
    if (ui->SaveFileCheckBox->isEnabled()) {
        ui->SaveFileCheckBox->setChecked(s_main_ini_val.AskSaveIniAtExit != 0);
    }
    ui->RemManRefLabelCheckBox->setChecked(s_main_ini_val.RememberReferencedLabels != 0);
    ui->RealtimeCheckBox->setChecked(s_main_ini_val.NotFasterThanRealTime != 0);
    ui->UseRelativePathsCheckBox->setChecked(s_main_ini_val.UseRelativePaths != 0);
    ui->SuprNoneExistValCheckBox->setChecked(s_main_ini_val.SuppressDisplayNonExistValues != 0);
    ui->PriorityComboBox->setCurrentText(QString (s_main_ini_val.Priority));
    ui->ShowUnitsCheckBox->setChecked(DisplayUnitForNonePhysicalValues != 0);
    ui->ShowStatusCarCheckBox->setChecked(s_main_ini_val.StatusbarCar != 0);
    ui->SpeedLineEdit->setText(QString("%1").arg((s_main_ini_val.SpeedStatusbarCar)));
    ui->NoCaseSensFilCheckBox->setChecked(s_main_ini_val.NoCaseSensitiveFilters != 0);
    ui->ReplaceLabelnameCheckBox->setChecked(s_main_ini_val.AsapCombatibleLabelnames != 0);

    ui->ReplaceAllOtherCharsCheckBox->setChecked(s_main_ini_val.ReplaceAllNoneAsapCombatibleChars != 0);
    ui->Replace1RadioButton->setEnabled(s_main_ini_val.ReplaceAllNoneAsapCombatibleChars != 0);
    ui->Replace2RadioButton->setEnabled(s_main_ini_val.ReplaceAllNoneAsapCombatibleChars != 0);
    ui->Replace1RadioButton->setChecked(s_main_ini_val.ReplaceAllNoneAsapCombatibleChars == 1);
    ui->Replace2RadioButton->setChecked(s_main_ini_val.ReplaceAllNoneAsapCombatibleChars == 2);

    ui->DisplStatObjCheckBox->setChecked(s_main_ini_val.ViewStaticSymbols != 0);
    ui->AddSourceFilenameCheckBox->setEnabled(s_main_ini_val.ViewStaticSymbols != 0);
    if (ui->AddSourceFilenameCheckBox->isEnabled()) {
        ui->AddSourceFilenameCheckBox->setChecked(s_main_ini_val.ExtendStaticLabelsWithFilename != 0);
    }
    ui->WriteINIFileCheckBox->setChecked(s_main_ini_val.SaveSortedINIFiles != 0);
    ui->MakeBackupCheckBox->setChecked(s_main_ini_val.MakeBackupBeforeSaveINIFiles != 0);
    ui->NoScriptDebugFileCheckBox->setChecked(s_main_ini_val.DontMakeScriptDebugFile != 0);
    ui->TerminateScriptLineEdit->setText(QString (s_main_ini_val.TerminateScript));
    ui->StrictAutomaticCheckBox->setChecked(s_main_ini_val.ScrictAtomicEndAtomicCheck != 0);
    ui->StoreReferenceCheckBox->setChecked(s_main_ini_val.UseTempIniForFunc.RefLists != 0);

    // If remote master is active the scheduler should not be stopped
    if (s_main_ini_val.ConnectToRemoteMaster) {
        ui->StopSchedulerCheckBox->setDisabled(true);
    }
    ui->StopSchedulerCheckBox->setChecked(s_main_ini_val.ShouldStopSchedulerWhileDialogOpen != 0);
    ui->SeparateCyclesForRefAndInitFunctionCheckBox->setChecked(s_main_ini_val.SeparateCyclesForRefAndInitFunction != 0);

    QStyleFactory Styles;
    QStringList StyleNames = Styles.keys();
    ui->SelectStartupAppStyleComboBox->addItems(StyleNames);

    if (strlen (s_main_ini_val.SelectStartupAppStyle)) {
        ui->SelectStartupAppStyleCheckBox->setChecked(true);
        ui->SelectStartupAppStyleComboBox->setEnabled(true);
        ui->SelectStartupAppStyleComboBox->setCurrentText(QString(s_main_ini_val.SelectStartupAppStyle));
    } else {
        ui->SelectStartupAppStyleCheckBox->setChecked(false);
        ui->SelectStartupAppStyleComboBox->setEnabled(false);
    }

    // Text window:
    ui->DefaultsTextShowDisplayTypeColumnCheckBox->setChecked(s_main_ini_val.TextDefaultShowDispayTypeColumn);
    ui->DefaultsTextShowUnitColumnCheckBox->setChecked(s_main_ini_val.TextDefaultShowUnitColumn);
    if (strlen(s_main_ini_val.TextDefaultFont)) {
        ui->DefaultsTextDisplayFontCheckBox->setChecked(true);
        ui->DefaultsTextFontLineEdit->setText(QString(s_main_ini_val.TextDefaultFont));
        ui->DefaultsTextFontSizeLineEdit->setText(QString().number(s_main_ini_val.TextDefaultFontSize));
        ui->DefaultsTextFontPushButton->setEnabled(true);
    } else {
        ui->DefaultsTextDisplayFontCheckBox->setChecked(false);
        ui->DefaultsTextFontPushButton->setEnabled(false);
    }
    if (ui->DefaultsTextDisplayFontCheckBox->isChecked()) {
        StringCopyMaxCharTruncate (s_main_ini_val.TextDefaultFont, QStringToConstChar(ui->DefaultsTextFontLineEdit->text()),
                                   sizeof (s_main_ini_val.TextDefaultFont));
        s_main_ini_val.TextDefaultFontSize = ui->DefaultsTextFontSizeLineEdit->text().toInt();
    }

    // Oscilloscope window:
    ui->OscilloscopeDefaultBufferDepthLineEdit->setText(QString("%1").arg((s_main_ini_val.OscilloscopeDefaultBufferDepth)));

    if (strlen(s_main_ini_val.OscilloscopeDefaultFont)) {
        ui->DefaultsOscilloscopeFontCheckBox->setChecked(true);
        ui->DefaultsOscilloscopeFontLineEdit->setText(QString(s_main_ini_val.OscilloscopeDefaultFont));
        ui->DefaultsOscilloscopeFontSizeLineEdit->setText(QString().number(s_main_ini_val.OscilloscopeDefaultFontSize));
        ui->DefaultsOscilloscopeFontPushButton->setEnabled(true);
    } else {
        ui->DefaultsOscilloscopeFontCheckBox->setChecked(false);
        ui->DefaultsOscilloscopeFontPushButton->setEnabled(false);
    }

    if (ui->DefaultsOscilloscopeFontCheckBox->isChecked()) {
        StringCopyMaxCharTruncate (s_main_ini_val.OscilloscopeDefaultFont,
                                  QStringToConstChar(ui->DefaultsOscilloscopeFontLineEdit->text()),
                                  sizeof (s_main_ini_val.OscilloscopeDefaultFont));
        s_main_ini_val.OscilloscopeDefaultFontSize = ui->DefaultsOscilloscopeFontSizeLineEdit->text().toInt();
    }
    // Remote Master
    ui->EnableRemoteMasterCheckBox->setChecked(s_main_ini_val.ShouldConnectToRemoteMaster != 0);
    ui->RemoteMasterNameLineEdit->setText(s_main_ini_val.RemoteMasterName);
    ui->RemoteMasterPortLineEdit->setText(QString().number(s_main_ini_val.RemoteMasterPort));
    ui->CopyAndStartRemoteMasterCheckBox->setChecked(s_main_ini_val.CopyAndStartRemoteMaster != 0);
    ui->RemoteMasterExecutableLineEdit->setText(s_main_ini_val.RemoteMasterExecutable);
    if (GetRemoteMasterExecutableCmdLine() != nullptr) {
        ui->RemoteMasterLoadedLineEdit->setText(GetRemoteMasterExecutableCmdLine());
    }
    ui->RemoteMasterCopyToLineEdit->setText(s_main_ini_val.RemoteMasterCopyTo);

    ui->DontWriteBlackbardVariableInfosToIniCheckBox->setChecked(s_main_ini_val.DontWriteBlackbardVariableInfosToIni);
    ui->DontSaveBlackbardVariableInfosIniSectionCheckBox->setChecked(s_main_ini_val.DontSaveBlackbardVariableInfosIniSection);

    // Remote Procedure Calls
    ui->NamedPipeRadioButton->setChecked(s_main_ini_val.RpcOverSocketOrNamedPipe == 0);   // 0 -> Named pipe 1 -> Socket
    ui->SocketsRadioButton->setChecked(s_main_ini_val.RpcOverSocketOrNamedPipe != 0);   // 0 -> Named pipe 1 -> Socket
    ui->CommandNameCheckBox->setChecked((s_main_ini_val.RpcDebugLoggingFlags & DEBUG_PRINT_COMMAND_NAME) == DEBUG_PRINT_COMMAND_NAME);
    ui->ParameterHexCheckBox->setChecked((s_main_ini_val.RpcDebugLoggingFlags & DEBUG_PRINT_COMMAND_PARAMETER_AS_HEX) == DEBUG_PRINT_COMMAND_PARAMETER_AS_HEX);
    ui->ReturnValueHexCheckBox->setChecked((s_main_ini_val.RpcDebugLoggingFlags & DEBUG_PRINT_COMMAND_RETURN_AS_HEX) == DEBUG_PRINT_COMMAND_RETURN_AS_HEX);
    ui->ParameterDecodedCheckBox->setChecked((s_main_ini_val.RpcDebugLoggingFlags & DEBUG_PRINT_COMMAND_PARAMETER_DECODED) == DEBUG_PRINT_COMMAND_PARAMETER_DECODED);
    ui->ReturnValueDecodedCheckBox->setChecked((s_main_ini_val.RpcDebugLoggingFlags & DEBUG_PRINT_COMMAND_RETURN_DECODED) == DEBUG_PRINT_COMMAND_RETURN_DECODED);
    ui->FlushLogFileCheckBox->setChecked((s_main_ini_val.RpcDebugLoggingFlags & DEBUG_PRINT_COMMAND_FFLUSH) == DEBUG_PRINT_COMMAND_FFLUSH);
    ui->AttachToLogFileCheckBox->setChecked((s_main_ini_val.RpcDebugLoggingFlags & DEBUG_PRINT_COMMAND_ATTACH) == DEBUG_PRINT_COMMAND_ATTACH);
    ui->WriteToOneLogFileCheckBox->setChecked((s_main_ini_val.RpcDebugLoggingFlags & DEBUG_PRINT_COMMAND_ONE_FILE) == DEBUG_PRINT_COMMAND_ONE_FILE);
    on_CommandNameCheckBox_stateChanged((s_main_ini_val.RpcDebugLoggingFlags & 0x1) == 0x1);
    ui->PortLineEdit->setText(QString().number(s_main_ini_val.RpcSocketPort));
    ui->WarningLabel->setStyleSheet("QLabel { color : red; }");

    ui->StopScriptIfLabelDosNotExistInsideSVLCheckBox->setChecked(s_main_ini_val.StopScriptIfLabelDosNotExistInsideSVLIni);

    ui->ExternProcessLoginSocketPortCheckBox->setChecked(s_main_ini_val.ExternProcessLoginSocketPortIni > 0);
    ui->ExternProcessLoginSocketPortLineEdit->setText(QString().number(s_main_ini_val.ExternProcessLoginSocketPortIni));

    ui->SyncWithFlexrayCheckBox->setChecked(s_main_ini_val.SyncWithFlexray);

    ui->DarkModeCheckBox->setChecked(s_main_ini_val.ShouldUseDarkModeIni);
}

BasicSettingsDialog::~BasicSettingsDialog()
{
    delete ui;
}

void BasicSettingsDialog::on_DisplStatObjCheckBox_clicked(bool checked)
{
    ui->AddSourceFilenameCheckBox->setDisabled(!checked);
}


void BasicSettingsDialog::on_AutosaveCheckBox_clicked(bool checked)
{
    ui->SaveFileCheckBox->setDisabled(!checked);
}

void BasicSettingsDialog::accept()
{
    char  szTmpString[2*MAX_PATH+50];
    int help;
    double sched_periode = 0.01;
    int blackboard_size = 1024;
    char txt[2*MAX_PATH+50];
    bool Ok;

    sched_periode = ui->SchedulingLineEdit->GetDoubleValue(&Ok);
    if (!Ok || (sched_periode > 1.0) || (sched_periode < 0.000000001)) {
        ThrowError(MESSAGE_ONLY,"scheduling period have to be betwen 1.0s and 1.0ns.");
        ui->SchedulingLineEdit->setFocus();
        return;
    }
    blackboard_size = ui->BlackboardElemLineEdit->GetUIntValue(&Ok);
    if (!Ok || (blackboard_size > (1024*1024)) || (blackboard_size < 1024)) {
        ThrowError(MESSAGE_ONLY, "Blackboard elements count must be in the range 1024...1048576.");
        ui->BlackboardElemLineEdit->setFocus();
        return;
    }

    write_process_list2ini ();  // in global\init.c


    // If a script is running this should be stopped
    // because there can be changed some importend settings for the script
    if(script_status_flag == RUNNING) {
        script_status_flag = STOP;
        ThrowError(MESSAGE_ONLY, "Script stopped, because of possible changes in the BASIC settings");
    }

    STRING_COPY_TO_ARRAY(s_main_ini_val.ProjectName, QStringToConstChar(ui->ProjectNameLineEdit->text()));

    STRING_COPY_TO_ARRAY(szTmpString, QStringToConstChar(ui->WorkDirectoryLineEdit->text()));
    if (szTmpString[0] != '\0') {
        char TempString[MAX_PATH];
        STRING_COPY_TO_ARRAY (s_main_ini_val.WorkDir, szTmpString);
        SearchAndReplaceEnvironmentStrings (s_main_ini_val.WorkDir,
                                            TempString,
                                            sizeof(s_main_ini_val.WorkDir));
        // set the path
        if (!SetCurrentDirectory (TempString)) {
            STRING_COPY_TO_ARRAY (txt, "work directory \"%s\" not exist, use ");
            GetCurrentDirectory (static_cast<DWORD>(sizeof (txt) - strlen (txt)), txt + strlen (txt));
            ThrowError(ERROR_NO_CRITICAL_STOP, txt, TempString);
        }
        GetCurrentDirectory (sizeof (s_main_ini_val.StartDirectory), s_main_ini_val.StartDirectory);
    } else {
        STRING_COPY_TO_ARRAY (s_main_ini_val.WorkDir, ".");
    }

    // resolution are 1ns
    PrintFormatToString (txt, sizeof(txt), "%.9f", sched_periode);
    sched_periode = atof(txt);

    s_main_ini_val.SchedulerPeriode = sched_periode;

    if (s_main_ini_val.BlackboardSize != blackboard_size) {
        ThrowError (1, "You have changed the blackboard size. The modification will be accepted after restarting.");
    }
    s_main_ini_val.BlackboardSize = blackboard_size;

#ifdef _WIN32
    STRING_COPY_TO_ARRAY (s_main_ini_val.Editor, QStringToConstChar(ui->EditorLineEdit->text()));
#else
    STRING_COPY_TO_ARRAY (s_main_ini_val.EditorX, QStringToConstChar(ui->EditorLineEdit->text()));
#endif
    STRING_COPY_TO_ARRAY (s_main_ini_val.Priority, QStringToConstChar(ui->PriorityComboBox->currentText()));

    s_main_ini_val.DontCallSleep = SetPriority (s_main_ini_val.Priority);

    ui->RemManRefLabelCheckBox->isChecked() ? s_main_ini_val.RememberReferencedLabels = 1 : s_main_ini_val.RememberReferencedLabels = 0;

    ui->RealtimeCheckBox->isChecked() ? s_main_ini_val.NotFasterThanRealTime = 1 : s_main_ini_val.NotFasterThanRealTime = 0;
    SetNotFasterThanRealtimeState (s_main_ini_val.NotFasterThanRealTime);
    ui->UseRelativePathsCheckBox->isChecked() ? s_main_ini_val.UseRelativePaths = 1 : s_main_ini_val.UseRelativePaths = 0;
    ui->SuprNoneExistValCheckBox->isChecked() ? s_main_ini_val.SuppressDisplayNonExistValues = 1 : s_main_ini_val.SuppressDisplayNonExistValues = 0;
    SetSuppressDisplayNonExistValuesState (s_main_ini_val.SuppressDisplayNonExistValues);
    ui->ShowUnitsCheckBox->isChecked() ? DisplayUnitForNonePhysicalValues = 1 : DisplayUnitForNonePhysicalValues = 0;
    ui->ShowStatusCarCheckBox->isChecked() ? s_main_ini_val.StatusbarCar = 1 : s_main_ini_val.StatusbarCar = 0;
    STRING_COPY_TO_ARRAY (s_main_ini_val.SpeedStatusbarCar, QStringToConstChar(ui->SpeedLineEdit->text()));
    ui->NoCaseSensFilCheckBox->isChecked() ? s_main_ini_val.NoCaseSensitiveFilters = 1 : s_main_ini_val.NoCaseSensitiveFilters = 0;
    ui->ReplaceLabelnameCheckBox->isChecked() ? s_main_ini_val.AsapCombatibleLabelnames = 1 : s_main_ini_val.AsapCombatibleLabelnames = 0;

    if (ui->ReplaceAllOtherCharsCheckBox->isChecked()) {
        if (ui->Replace1RadioButton->isChecked()) {
            s_main_ini_val.ReplaceAllNoneAsapCombatibleChars = 1;
        } else if (ui->Replace2RadioButton->isChecked()) {
            s_main_ini_val.ReplaceAllNoneAsapCombatibleChars = 2;
        } else {
            s_main_ini_val.ReplaceAllNoneAsapCombatibleChars = 0;
        }
    } else {
        s_main_ini_val.ReplaceAllNoneAsapCombatibleChars = 0;
    }

    ui->DisplStatObjCheckBox->isChecked() ? s_main_ini_val.ViewStaticSymbols = 1 : s_main_ini_val.ViewStaticSymbols = 0;
    ui->AddSourceFilenameCheckBox->isChecked() ? s_main_ini_val.ExtendStaticLabelsWithFilename = 1 : s_main_ini_val.ExtendStaticLabelsWithFilename = 0;
    ui->WriteINIFileCheckBox->isChecked() ? s_main_ini_val.SaveSortedINIFiles = 1 : s_main_ini_val.SaveSortedINIFiles = 0;
    ui->MakeBackupCheckBox->isChecked() ? s_main_ini_val.MakeBackupBeforeSaveINIFiles = 1 : s_main_ini_val.MakeBackupBeforeSaveINIFiles = 0;
    ui->NoScriptDebugFileCheckBox->isChecked() ? s_main_ini_val.DontMakeScriptDebugFile = 1 : s_main_ini_val.DontMakeScriptDebugFile = 0;

    STRING_COPY_TO_ARRAY (s_main_ini_val.TerminateScript, QStringToConstChar(ui->TerminateScriptLineEdit->text()));
    RemoveWhitespacesOnlyAtBeginingAndEnd (s_main_ini_val.TerminateScript);
    s_main_ini_val.TerminateScriptFlag = strlen (s_main_ini_val.TerminateScript) > 0;

    ui->StrictAutomaticCheckBox->isChecked() ? s_main_ini_val.ScrictAtomicEndAtomicCheck = 1 : s_main_ini_val.ScrictAtomicEndAtomicCheck = 0;
    ui->StoreReferenceCheckBox->isChecked() ? s_main_ini_val.UseTempIniForFunc.RefLists  = 1 : s_main_ini_val.UseTempIniForFunc.RefLists  = 0;

    if (!s_main_ini_val.ConnectToRemoteMaster) {
        if (ui->StopSchedulerCheckBox->isChecked() && s_main_ini_val.StopSchedulerWhileDialogOpen == 0) {
            disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_SYSTEM, nullptr, nullptr);
        }
        if (!ui->StopSchedulerCheckBox->isChecked() && s_main_ini_val.StopSchedulerWhileDialogOpen == 1) {
            enable_scheduler(SCHEDULER_CONTROLED_BY_SYSTEM);
        }
    }
    ui->StopSchedulerCheckBox->isChecked() ? s_main_ini_val.ShouldStopSchedulerWhileDialogOpen = 1 : s_main_ini_val.ShouldStopSchedulerWhileDialogOpen = 0;
    if(ui->SeparateCyclesForRefAndInitFunctionCheckBox->isChecked()) {
        s_main_ini_val.SeparateCyclesForRefAndInitFunction = 1;
    } else {
        s_main_ini_val.SeparateCyclesForRefAndInitFunction = 0;
    }
    if (!s_main_ini_val.ConnectToRemoteMaster) {
        s_main_ini_val.StopSchedulerWhileDialogOpen = s_main_ini_val.ShouldStopSchedulerWhileDialogOpen;
    }

    if (ui->SelectStartupAppStyleCheckBox->isChecked()) {
        QString StyleName = ui->SelectStartupAppStyleComboBox->currentText();
        QString OldStyleName = QString(s_main_ini_val.SelectStartupAppStyle);
        if (StyleName.compare(OldStyleName)) {
            MEMSET (s_main_ini_val.SelectStartupAppStyle, 0, sizeof(s_main_ini_val.SelectStartupAppStyle));
            strncpy (s_main_ini_val.SelectStartupAppStyle, QStringToConstChar(StyleName), sizeof(s_main_ini_val.SelectStartupAppStyle)-1);
            qApp->setStyle(StyleName);
        }
    }

    // Text-Fenster:
    s_main_ini_val.TextDefaultShowUnitColumn = ui->DefaultsTextShowUnitColumnCheckBox->isChecked();
    s_main_ini_val.TextDefaultShowDispayTypeColumn = ui->DefaultsTextShowDisplayTypeColumnCheckBox->isChecked();
    MEMSET (s_main_ini_val.TextDefaultFont, 0, sizeof (s_main_ini_val.TextDefaultFont));
    s_main_ini_val.TextDefaultFontSize = 0;
    if (ui->DefaultsTextDisplayFontCheckBox->isChecked()) {
        StringCopyMaxCharTruncate (s_main_ini_val.TextDefaultFont, QStringToConstChar(ui->DefaultsTextFontLineEdit->text()), sizeof (s_main_ini_val.TextDefaultFont));
        s_main_ini_val.TextDefaultFontSize = ui->DefaultsTextFontSizeLineEdit->text().toInt();
    }
    // Ozi-Fenster:
    s_main_ini_val.OscilloscopeDefaultBufferDepth = ui->OscilloscopeDefaultBufferDepthLineEdit->text().toUInt();
    MEMSET (s_main_ini_val.OscilloscopeDefaultFont, 0, sizeof (s_main_ini_val.OscilloscopeDefaultFont));
    s_main_ini_val.OscilloscopeDefaultFontSize = 0;
    if (ui->DefaultsOscilloscopeFontCheckBox->isChecked()) {
        StringCopyMaxCharTruncate (s_main_ini_val.OscilloscopeDefaultFont,
                                   QStringToConstChar(ui->DefaultsOscilloscopeFontLineEdit->text()),
                                   sizeof (s_main_ini_val.OscilloscopeDefaultFont));
        s_main_ini_val.OscilloscopeDefaultFontSize = ui->DefaultsOscilloscopeFontSizeLineEdit->text().toInt();
    }

    // This have to be always the last!
    ui->WriteProcessListCheckBox->isChecked() ? s_main_ini_val.WriteProtectIniProcessList = 1 : s_main_ini_val.WriteProtectIniProcessList = 0;
    ui->AutosaveCheckBox->isChecked() ? help = 1 : help = 0;
    if ((help == 1) && (s_main_ini_val.SwitchAutomaticSaveIniOff == 0)) {
        if (ThrowError (ERROR_OKCANCEL, "You cannot set write-protection without saving the INI-file!\nSave it now ?") == IDCANCEL) {
            ThrowError (IDOKALL, "cannot set write protection for INI-File");
            return;
        } else {
            s_main_ini_val.SwitchAutomaticSaveIniOff = help;
            SaveAllInfosToIniDataBase();
            char IniFileName[MAX_PATH];
            IniFileDataBaseGetFileNameByDescriptor(GetMainFileDescriptor(), IniFileName, sizeof(IniFileName));
            if (IniFileDataBaseSave(GetMainFileDescriptor(), nullptr, 0)) {
                ThrowError (1, "cannot save %s", IniFileName);
            }
        }
    } else {
        s_main_ini_val.SwitchAutomaticSaveIniOff = help;
    }
    if (help == 1) { // Only set if automatic save is activated
        ui->SaveFileCheckBox->isChecked() ? s_main_ini_val.AskSaveIniAtExit = 1 : s_main_ini_val.AskSaveIniAtExit = 0;
    }

    // Remote master
    s_main_ini_val.ShouldConnectToRemoteMaster = ui->EnableRemoteMasterCheckBox->isChecked();

    QString RemoteMasterName = ui->RemoteMasterNameLineEdit->text().trimmed();
    RemoteMasterName.truncate(sizeof(s_main_ini_val.RemoteMasterName)-1);
    STRING_COPY_TO_ARRAY (s_main_ini_val.RemoteMasterName,
            QStringToConstChar(RemoteMasterName));

    QString RemoteMasterPort = ui->RemoteMasterPortLineEdit->text();
    s_main_ini_val.RemoteMasterPort = RemoteMasterPort.toInt();

    s_main_ini_val.CopyAndStartRemoteMaster = ui->CopyAndStartRemoteMasterCheckBox->isChecked();

    QString RemoteMasterExecutable = ui->RemoteMasterExecutableLineEdit->text().trimmed();
    RemoteMasterExecutable.truncate(sizeof(s_main_ini_val.RemoteMasterExecutable)-1);
    STRING_COPY_TO_ARRAY (s_main_ini_val.RemoteMasterExecutable,
            QStringToConstChar(RemoteMasterExecutable));

    QString RemoteMasterCopyTo = ui->RemoteMasterCopyToLineEdit->text().trimmed();
    RemoteMasterCopyTo.truncate(sizeof(s_main_ini_val.RemoteMasterCopyTo)-1);
    STRING_COPY_TO_ARRAY (s_main_ini_val.RemoteMasterCopyTo,
            QStringToConstChar(RemoteMasterCopyTo));

    s_main_ini_val.DontWriteBlackbardVariableInfosToIni = ui->DontWriteBlackbardVariableInfosToIniCheckBox->isChecked();
    s_main_ini_val.DontSaveBlackbardVariableInfosIniSection = ui->DontSaveBlackbardVariableInfosIniSectionCheckBox->isChecked();

    // Remote procedure calls
    s_main_ini_val.RpcOverSocketOrNamedPipe = ui->SocketsRadioButton->isChecked();   // 0 -> named pipe 1 -> socket
    s_main_ini_val.RpcDebugLoggingFlags = ui->CommandNameCheckBox->isChecked() ? DEBUG_PRINT_COMMAND_NAME : 0;
    s_main_ini_val.RpcDebugLoggingFlags |= ui->ParameterHexCheckBox->isChecked() ? DEBUG_PRINT_COMMAND_PARAMETER_AS_HEX : 0;
    s_main_ini_val.RpcDebugLoggingFlags |= ui->ReturnValueHexCheckBox->isChecked() ? DEBUG_PRINT_COMMAND_RETURN_AS_HEX : 0;
    s_main_ini_val.RpcDebugLoggingFlags |= ui->ParameterDecodedCheckBox->isChecked() ? DEBUG_PRINT_COMMAND_PARAMETER_DECODED : 0;
    s_main_ini_val.RpcDebugLoggingFlags |= ui->ReturnValueDecodedCheckBox->isChecked() ? DEBUG_PRINT_COMMAND_RETURN_DECODED : 0;

    s_main_ini_val.RpcDebugLoggingFlags |= ui->FlushLogFileCheckBox->isChecked() ? DEBUG_PRINT_COMMAND_FFLUSH : 0;
    s_main_ini_val.RpcDebugLoggingFlags |= ui->AttachToLogFileCheckBox->isChecked() ? DEBUG_PRINT_COMMAND_ATTACH : 0;
    s_main_ini_val.RpcDebugLoggingFlags |= ui->WriteToOneLogFileCheckBox->isChecked() ? DEBUG_PRINT_COMMAND_ONE_FILE : 0;

    s_main_ini_val.RpcSocketPort = ui->PortLineEdit->text().toInt();
    s_main_ini_val.StopScriptIfLabelDosNotExistInsideSVL =
    s_main_ini_val.StopScriptIfLabelDosNotExistInsideSVLIni = ui->StopScriptIfLabelDosNotExistInsideSVLCheckBox->isChecked();

    if (ui->ExternProcessLoginSocketPortCheckBox->isChecked()) {
        s_main_ini_val.ExternProcessLoginSocketPortIni = ui->ExternProcessLoginSocketPortLineEdit->text().trimmed().toUInt();
    }
    s_main_ini_val.SyncWithFlexray = ui->SyncWithFlexrayCheckBox->isChecked();

    s_main_ini_val.ShouldUseDarkModeIni = ui->DarkModeCheckBox->isChecked();

    QDialog::accept();
}

void BasicSettingsDialog::RemoveWhitespacesOnlyAtBeginingAndEnd(char *p)
{
    char *d = p;
    char *s = p;
    while (isascii (*s) && isspace (*s)) s++;
    while (*s != 0) *d++ = *s++;
    d--;
    while (isascii (*d) && isspace (*d) && (d > p)) d--;
    d++;
    *d = 0;
}


void BasicSettingsDialog::on_SelectStartupAppStyleCheckBox_toggled(bool checked)
{
    ui->SelectStartupAppStyleComboBox->setEnabled(checked);
}

void BasicSettingsDialog::on_DefaultsTextDisplayFontCheckBox_toggled(bool checked)
{
    ui->DefaultsTextFontPushButton->setEnabled(checked);
}

void BasicSettingsDialog::on_DefaultsOscilloscopeFontCheckBox_toggled(bool checked)
{
    ui->DefaultsOscilloscopeFontPushButton->setEnabled(checked);
}

void BasicSettingsDialog::on_DefaultsOscilloscopeFontPushButton_clicked()
{
    QFont  loc_Font(ui->DefaultsOscilloscopeFontLineEdit->text(), ui->DefaultsOscilloscopeFontSizeLineEdit->text().toInt());
    QFontDialog loc_fontDialog(loc_Font, this);
    if(loc_fontDialog.exec() == QDialog::Accepted) {
        QFont loc_Font = loc_fontDialog.currentFont();
        ui->DefaultsOscilloscopeFontLineEdit->setText(loc_Font.family());
        ui->DefaultsOscilloscopeFontSizeLineEdit->setText(QString().number(loc_Font.pointSize()));
    }
}

void BasicSettingsDialog::on_DefaultsTextFontPushButton_clicked()
{
    QFont  loc_Font(ui->DefaultsTextFontLineEdit->text(), ui->DefaultsTextFontSizeLineEdit->text().toInt());
    QFontDialog loc_fontDialog(loc_Font, this);
    if(loc_fontDialog.exec() == QDialog::Accepted) {
        QFont loc_Font = loc_fontDialog.currentFont();
        ui->DefaultsTextFontLineEdit->setText(loc_Font.family());
        ui->DefaultsTextFontSizeLineEdit->setText(QString().number(loc_Font.pointSize()));
    }
}

void BasicSettingsDialog::on_ReplaceAllOtherCharsCheckBox_toggled(bool checked)
{
    ui->Replace1RadioButton->setEnabled(checked);
    ui->Replace2RadioButton->setEnabled(checked);
}

void BasicSettingsDialog::on_RemoteMasterExecutablePushButton_clicked()
{
    QString FileName = FileDialog::getOpenFileName(this, "background image file", QString(), ALL_EXT);
    if (FileName.size() > 0) {
        ui->RemoteMasterExecutableLineEdit->setText(FileName);
    }
}

void BasicSettingsDialog::on_EnableRemoteMasterCheckBox_stateChanged(int arg1)
{
    ui->RemoteMasterNameLineEdit->setEnabled(arg1);
    ui->RemoteMasterPortLineEdit->setEnabled(arg1);
    ui->CopyAndStartRemoteMasterCheckBox->setEnabled(arg1);
    ui->RemoteMasterExecutableLineEdit->setEnabled(arg1);
    ui->RemoteMasterExecutablePushButton->setEnabled(arg1);
    ui->RemoteMasterCopyToLineEdit->setEnabled(arg1);
}

void BasicSettingsDialog::on_CopyAndStartRemoteMasterCheckBox_stateChanged(int arg1)
{
    ui->RemoteMasterExecutableLineEdit->setEnabled(arg1);
    ui->RemoteMasterExecutablePushButton->setEnabled(arg1);
    ui->RemoteMasterCopyToLineEdit->setEnabled(arg1);
}

void BasicSettingsDialog::on_CommandNameCheckBox_stateChanged(int arg1)
{
    ui->ParameterHexCheckBox->setEnabled(arg1);
    ui->ReturnValueHexCheckBox->setEnabled(arg1);
    ui->ParameterDecodedCheckBox->setEnabled(arg1);
    ui->ReturnValueDecodedCheckBox->setEnabled(arg1);
    ui->FlushLogFileCheckBox->setEnabled(arg1);
    ui->AttachToLogFileCheckBox->setEnabled(arg1);
    ui->WriteToOneLogFileCheckBox->setEnabled(arg1);
}

void BasicSettingsDialog::on_DarkModeCheckBox_clicked(bool checked)
{
    // this do not work!
    // you have to restart OpenXilEnv to change the dark mode setting
#if 0
    QApplication *App = static_cast<QApplication*>(s_main_ini_val.QtApplicationPointer);
    if (App != nullptr) {
        SetDarkMode(App, checked);
    }
#else
    Q_UNUSED(checked)
#endif
}

