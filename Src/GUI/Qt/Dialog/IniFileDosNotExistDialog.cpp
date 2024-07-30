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


#include "IniFileDosNotExistDialog.h"
#include "ui_IniFileDosNotExistDialog.h"

#include <QFileDialog>

#include "DarkMode.h"
#include "StringHelpers.h"
#include "QtIniFile.h"

#include "Platform.h"
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <dirent.h>
#endif
extern "C" {
#include "Config.h"
#include "StringMaxChar.h"
#include "ConfigurablePrefix.h"
#include "ThrowError.h"
#include "MainValues.h"
}


int IniFileDosNotExist::BuidANewIniFile()
{
    FILE *fh;

    m_NewSelectedIniFile = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                        "",
                                                        tr("INI files (*.INI)"));
    if (m_NewSelectedIniFile.isEmpty()) {
        return 0;
    }
    // create INI file
    fh = fopen(QStringToConstChar(m_NewSelectedIniFile), "w");
    if (fh != nullptr) {
        fprintf (fh, "[FileInfos]\n"
                    "  Version=%i.%i.%i\n\n", (int)XILENV_VERSION, (int)XILENV_MINOR_VERSION, (int)XILENV_PATCH_VERSION);
        fprintf (fh, "[InitStartProcesses]\n"
                     "  WriteProtect=0\n"
                     "  P0=StimuliPlayer\n"
                     "  P1=SignalGeneratorCompiler\n"
                     "  P2=EquationCompiler\n"
                     "  P3=TraceRecorder\n"
                     "  P4=Oscilloscope\n"
                     "  P5=Script\n"
                     "  P6=StimuliQueue\n"
                     "  P7=SignalGeneratorCalculator\n"
                     "  P8=EquationCalculator\n"
                     "  P9=TraceQueue\n");
        fclose (fh);
    } else {
        ThrowError (1, "cannot build file %s", QStringToConstChar(m_NewSelectedIniFile));
        return 0;
    }
    int Fd = ScQt_IniFileDataBaseOpenNoFilterPossible(m_NewSelectedIniFile);
    if (Fd > 0) {
        MAIN_INI_VAL BasicSettings;
        STRUCT_ZERO_INIT (BasicSettings, MAIN_INI_VAL);

        // take over basic settings
        ScQt_SetMainFileDescriptor(Fd);
        ReadBasicConfigurationFromIni(&BasicSettings);
        ScQt_SetMainFileDescriptor(-1);
        // change something
        strcpy (BasicSettings.Editor, "");
        strcpy (BasicSettings.EditorX, "");
        strcpy (BasicSettings.WorkDir, ".");
        WriteBasicConfigurationToIni (&BasicSettings);
        //   op == 2 -> remove infos from INI-DB */
        ScQt_IniFileDataBaseSave(Fd, m_NewSelectedIniFile, 2);
        return 1;
    }
    return 0;
}

void IniFileDosNotExist::ChangeDarkMode(bool par_DarkMode)
{
    SetDarkMode(m_Application, par_DarkMode);
}


IniFileDosNotExist::IniFileDosNotExist(char *par_SelectedIniFile, void *par_Application, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IniFileDosNotExist)
{
    ui->setupUi(this);

    m_DarkModeHasChanged = false;
    m_DarkMode = DEFAULT_MODE;

    m_Application = (QApplication*)par_Application;

    ui->label->setText(QString ("INI file \"%1\" doesn't exist").arg (QString(par_SelectedIniFile)));
    TimerUpdateSoftIceIcon();

    QString Name = QString (":/Icons/DarkModeSmall.png");
    QPixmap Pixmap1 (Name);
    ui->DarkModeLabel->setPixmap(Pixmap1);
    Name = QString(":/Icons/NormalModeSmall.png");
    QPixmap Pixmap2 (Name);
    ui->NormalModeLabel->setPixmap(Pixmap2);
    Name = QString(":/Icons/DefaultModeSmall.png");
    QPixmap Pixmap3 (Name);
    ui->DefaultModeLabel->setPixmap(Pixmap3);

    const char *ProgramName = GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME);

    char HistoryFileChar[MAX_PATH];
#ifdef _WIN32
    HRESULT Result = SHGetFolderPath (nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, HistoryFileChar);
    if (Result == S_OK) {
        StringAppendMaxCharTruncate(HistoryFileChar, "\\", sizeof(HistoryFileChar));
        StringAppendMaxCharTruncate(HistoryFileChar, ProgramName, sizeof(HistoryFileChar));
        StringAppendMaxCharTruncate(HistoryFileChar, "\\History.ini", sizeof(HistoryFileChar));
#else
        {
            DIR *Dir;
            GetXilEnvHomeDirectory(HistoryFileChar);
            StringAppendMaxCharTruncate(HistoryFileChar, "/.", sizeof(HistoryFileChar));
            StringAppendMaxCharTruncate(HistoryFileChar, ProgramName, sizeof(HistoryFileChar));
            if ((Dir = opendir(HistoryFileChar)) == nullptr) {
                mkdir(HistoryFileChar, 0777);
            } else {
                closedir(Dir);
            }
            StringAppendMaxCharTruncate(HistoryFileChar, "/LastLoadedFiles.ini", sizeof(HistoryFileChar));
        }
#endif
        m_HistoryFile = CharToQString(HistoryFileChar);
        int Fd = ScQt_IniFileDataBaseOpenNoFilterPossible(m_HistoryFile);
        if (Fd <= 0) {
            Fd = ScQt_IniFileDataBaseCreateAndOpenNewIniFile(m_HistoryFile);
        }
        if (Fd <= 0) {
            m_Fd = -1;
        } else {
            m_Fd = Fd;
            QString Line = ScQt_IniFileDataBaseReadString (ProgramName, "DarkMode", "default", m_Fd);
            if (!Line.compare("yes")) {
                m_DarkMode = DARK_MODE;
            } else if (!Line.compare("no")) {
                m_DarkMode = NORMAL_MODE;
            } else {
                m_DarkMode = DEFAULT_MODE;
            }
            if (m_DarkMode == DARK_MODE) {
                // preselect dark mode
                ui->DarkModeRadioButton->setChecked(true);
                SetDarkMode(m_Application, true);
                SetDarkModeIntoMainSettings(1);
            } else if (m_DarkMode == NORMAL_MODE) {
                ui->NormalModeRadioButton->setChecked(true);
                SetDarkModeIntoMainSettings(0);
            } else {
                ui->DefaultModeRadioButton->setChecked(true);
                SetDarkModeIntoMainSettings(-1);
            }
            int x;
            Line = ScQt_IniFileDataBaseReadString(ProgramName, "last_used_ini_file", "",  m_Fd);
            if (!Line.isEmpty()) {
                ui->LastIniFilesComboBox->addItem(Line);
                ui->LastIniFilesComboBox->setCurrentText(Line);
                for (x = 0; x < 20; x++) {
                    QString Entry = QString("X_%1").arg(x);
                    Line = ScQt_IniFileDataBaseReadString(ProgramName, Entry, "", m_Fd);
                    if (Line.isEmpty()) {
                        break;
                    }
                    ui->LastIniFilesComboBox->addItem(Line);
                }
                ui->LastIniFilesComboBox->addItem(Line);
            }
        }
#ifdef _WIN32
    }
#endif
    m_Timer = new QTimer (this);
    bool Ret = connect (m_Timer, SIGNAL(timeout()), this, SLOT(TimerUpdateSoftIceIcon()));
    if (Ret == false) {
        ThrowError (1, "cannot connect timer");
    }
    m_Counter = 0;
    m_Timer->start (1000);
}

IniFileDosNotExist::~IniFileDosNotExist()
{
    if (m_Fd > 0) {
        if (m_DarkModeHasChanged) {
            const char *ProgramName = GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME);
            switch(m_DarkMode) {
            case DARK_MODE:
                ScQt_IniFileDataBaseWriteString (ProgramName, "DarkMode", "yes", m_Fd);
                break;
            case NORMAL_MODE:
                ScQt_IniFileDataBaseWriteString (ProgramName, "DarkMode", "no", m_Fd);
                break;
            case DEFAULT_MODE:
                ScQt_IniFileDataBaseWriteString (ProgramName, "DarkMode", "default", m_Fd);
                break;
            }
            ScQt_IniFileDataBaseSave(m_Fd, m_HistoryFile, 2);
            m_Fd = -1;
        }
        if (m_Fd > 0) {
            ScQt_IniFileDataBaseClose(m_Fd);
        }
    }
    delete ui;
    delete m_Timer;
}

void IniFileDosNotExist::on_ChooseExistingPushButton_clicked()
{
    // Do not use FileDialog! because at that time no INI file is loaded
    QString NewName = QFileDialog::getOpenFileName (this, tr("Open File"),
                                                    "",
                                                    tr("INI files (*.INI)"));
    if (!NewName.isEmpty()) {
        if (access(QStringToConstChar(NewName), 0) != 0) {  // check if file exist
            ThrowError(1, "File \"%s\" doesn't exist. Choose an other one",
                       QStringToConstChar(m_NewSelectedIniFile));
        } else {
            m_NewSelectedIniFile = NewName;
            this->accept();
        }
    }
}

void IniFileDosNotExist::TimerUpdateSoftIceIcon()
{
    if (m_Counter <= 17) {
        QString Name = QString (":/Icons/SoftIceCream_%1.ico").arg (m_Counter);
        QPixmap Pixmap (Name);
        ui->SoftIceLabel->setPixmap(Pixmap);
        m_Counter++;
    }
}

void IniFileDosNotExist::on_ExitImmediatelyPushButton_clicked()
{
    this->reject();
}

void IniFileDosNotExist::on_GenerateNewOnePushButton_clicked()
{
    if (BuidANewIniFile()) {
        this->accept();
    }
}

void IniFileDosNotExist::on_OpenLastOnePushButton_clicked()
{
    QString NewName = ui->LastIniFilesComboBox->currentText();
    if (NewName.isEmpty() ||
        (access (QStringToConstChar(NewName), 0) != 0)) {  // check if file exist
        ThrowError (1, "File \"%s\" doesn't exist. Choose an other one", QStringToConstChar(NewName));
    } else {
        m_NewSelectedIniFile = NewName;
        this->accept();
    }
}

void IniFileDosNotExist::on_DefaultModeRadioButton_clicked()
{
    m_DarkModeHasChanged = true;
    m_DarkMode = DEFAULT_MODE;
    SetDarkModeIntoMainSettings(-1);
}

void IniFileDosNotExist::on_NormalModeRadioButton_clicked()
{
    m_DarkModeHasChanged = true;
    m_DarkMode = NORMAL_MODE;
    SetDarkMode(m_Application, 0);
    SetDarkModeIntoMainSettings(0);
}

void IniFileDosNotExist::on_DarkModeRadioButton_clicked()
{
    m_DarkModeHasChanged = true;
    m_DarkMode = DARK_MODE;
    SetDarkMode(m_Application, 1);
    SetDarkModeIntoMainSettings(1);
}

int IniFileDosNotExist::GetNewSelectedIni (char *ret_NewSelectedIniFile, int par_MaxChars)
{
    if (strlen(QStringToConstChar(m_NewSelectedIniFile)) < static_cast<size_t>(par_MaxChars)) {
        StringCopyMaxCharTruncate(ret_NewSelectedIniFile, QStringToConstChar(m_NewSelectedIniFile), par_MaxChars);
        return 0;
    } else {
        ThrowError (1, "file name \"%s\" to long", QStringToConstChar(m_NewSelectedIniFile));
        return -1;
    }
}

bool IniFileDosNotExist::GetDarkMode()
{
    return m_DarkMode;
}

extern "C" {
int IniFileDosNotExistDialog (char *SelectedIniFile, char *ret_NewSelectedIniFile, int par_MaxChars,
                              void *par_Application)
{
    int Ret = -1;
    IniFileDosNotExist *Dlg;
    Dlg = new IniFileDosNotExist (SelectedIniFile, par_Application);
    if (Dlg->exec() == QDialog::Accepted) {
        Ret = Dlg->GetNewSelectedIni(ret_NewSelectedIniFile, par_MaxChars);
    }
    delete Dlg;
    return Ret;
}
}
