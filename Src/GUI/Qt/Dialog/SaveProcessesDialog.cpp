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


#include "SaveProcessesDialog.h"
#include "ui_SaveProcessesDialog.h"

#include "FileDialog.h"
#include "QtIniFile.h"
#include "StringHelpers.h"

extern "C"
{
    #include "DebugInfos.h"
    #include "LoadSaveToFile.h"
    #include "ThrowError.h"
    #include "Scheduler.h"
    #include "FileExtensions.h"
    #include "UniqueNumber.h"
    #include "DebugInfoDB.h"
    #include "IniDataBase.h"
}

#ifndef _WIN32
static char *strupr(char *Src)
{
    char *Dst;
    for (Dst = Src; *Dst!=0; Dst++) {
        *Dst = toupper(*Dst);
    }
    return Src;
}
#endif

saveprocessesdialog::saveprocessesdialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::saveprocessesdialog)
{
    ui->setupUi(this);
    int Fd = GetMainFileDescriptor();
    char SymFileName[MAX_PATH], BinFileName[MAX_PATH], *ptr, SvlFileName[MAX_PATH],
         SalFileName[MAX_PATH], SatvlFileName[MAX_PATH], CurProcess[MAX_PATH], strg[MAX_PATH];
    int ApplTarget;

    // Prozessname aus INI-Datei einlesen
    IniFileDataBaseReadString("BasicCalibrationSettings", "ApplProcessSymbol",
                  "", CurProcess, sizeof(CurProcess), Fd);

    // Speichertyp aus INI-Datei einlesen
    IniFileDataBaseReadString("BasicCalibrationSettings", "MemoryType",
                  "LSB_FIRST", strg, sizeof(strg), Fd);
    if (strcmp(strg, "LSB_FIRST") == 0) {
        LittleBigEndian = LSB_FIRST_FORMAT;
    } else if (strcmp(strg, "MSB_FIRST") == 0) {
        LittleBigEndian = MSB_FIRST_FORMAT;
    } else {
        ThrowError(MESSAGE_STOP, "Wrong Memory-Type in INI-File!\n"
                   "Using \"LSB_FIRST\" as default !");
        LittleBigEndian = LSB_FIRST_FORMAT;
    }

    // Ziel der Applikationsdaten aus INI-Datei einlesen
    IniFileDataBaseReadString("BasicCalibrationSettings", "ApplicationTarget",
                              "BINFILE", strg, sizeof(strg), Fd);
    if (strcmp(strg, "FILE") == 0) {
        ApplTarget = TO_FILE;
    } else if (strcmp(strg, "SALFILE") == 0) {
        ApplTarget = TO_SALFILE;
    } else if (strcmp(strg, "SATVLFILE") == 0) {
        ApplTarget = TO_SATVLFILE;
    } else {
        ThrowError(MESSAGE_STOP, "Wrong Application-Target in INI-File"
                   "\nUsing \"to svl\" as default !");
        ApplTarget = TO_FILE;
    }

    // Fill the process list
    char *pname;
    READ_NEXT_PROCESS_NAME *Buffer = init_read_next_process_name (2 | 0x100);
    while ((pname = read_next_process_name(Buffer)) != nullptr) {
        if (!IsInternalProcess (pname)) {
            QListWidgetItem *ProcessItem = new QListWidgetItem(pname);
            ui->listWidgetProcesses->addItem(ProcessItem);
            if (!Compare2ProcessNames(pname, CurProcess)) {
                ui->listWidgetProcesses->setCurrentItem(ProcessItem);
            }
        }
    }
    close_read_next_process_name(Buffer);

    // If only one external process than select this
    if (ui->listWidgetProcesses->count() == 1) {
        ui->listWidgetProcesses->row(nullptr);
    }

    IniFileDataBaseReadString("BasicCalibrationSettings", "SvlWriteFileName",
                  "*.svl", SvlFileName, sizeof(SvlFileName), Fd);
    IniFileDataBaseReadString("BasicCalibrationSettings", "SalWriteFileName",
                  "*.sal", SalFileName, sizeof(SalFileName), Fd);
    IniFileDataBaseReadString("BasicCalibrationSettings", "SatvlWriteFileName",
                  "*.satvl", SatvlFileName, sizeof(SatvlFileName), Fd);

    ui->lineEditSVL->setText(SvlFileName);
    ui->lineEditSAL->setText(SalFileName);
    ui->lineEditSATVL->setText(SatvlFileName);

    if (ApplTarget == TO_SATVLFILE) {
        ui->tabWidgetTarget->setCurrentIndex(2);
    } else if (ApplTarget == TO_SALFILE) {
        ui->tabWidgetTarget->setCurrentIndex(1);
    } else {
        ui->tabWidgetTarget->setCurrentIndex(0);
    }

    INCLUDE_EXCLUDE_FILTER *Filter = BuildIncludeExcludeFilterFromIni("BasicCalibrationSettings", "", Fd);
    ui->groupBoxFilter->SetFilter(Filter);
    FreeIncludeExcludeFilter (Filter);

    QString Section("GUI/AllCalibrationTreeWindows");
    for (int x = 0;;x++) {
        QString Entry = QString("ow%1").arg(x);
        QString WindowName = ScQt_IniFileDataBaseReadString(Section, Entry, "", ScQt_GetMainFileDescriptor());
        if (WindowName.isEmpty()) break;
        Entry = QString("process");
        QString ProcessName = ScQt_IniFileDataBaseReadString(WindowName, Entry, "", ScQt_GetMainFileDescriptor());
        QString Line = WindowName;
        Line.append(QString("   "));
        Line.append(QString("("));
        Line.append(ProcessName);
        Line.append(QString(")"));
        ui->tableWidgetCalibrationTreeWindows->insertRow(x);
        QTableWidgetItem *Item;
        Item = new QTableWidgetItem(WindowName);
        ui->tableWidgetCalibrationTreeWindows->setItem (x, 0, Item);
        Item = new QTableWidgetItem(ProcessName);
        ui->tableWidgetCalibrationTreeWindows->setItem (x, 1, Item);
    }
    ui->tableWidgetCalibrationTreeWindows->resizeColumnsToContents();
}

saveprocessesdialog::~saveprocessesdialog()
{
    delete ui;
}

void saveprocessesdialog::accept()
{
    char SvlFileName[MAX_PATH],
         SalFileName[MAX_PATH], SatvlFileName[MAX_PATH], CurProcess[MAX_PATH];
    INCLUDE_EXCLUDE_FILTER *Filter = nullptr;
    FILE *fp_sym;
    int idx;
    PROCESS_APPL_DATA *pappldata;
    int UniqueNumber;
    int Pid;

    switch (ui->tabWidgetFilter->currentIndex()) {
    case 0:
    default:
        Filter = ui->groupBoxFilter->GetFilter();
        SaveIncludeExcludeListsToIni (Filter, "BasicCalibrationSettings", "", GetMainFileDescriptor());
        break;
    case 1:
        {
            int Row = ui->tableWidgetCalibrationTreeWindows->currentRow();
            if (Row >= 0) {
                QString WindowName = ui->tableWidgetCalibrationTreeWindows->item(Row, 0)->data(Qt::DisplayRole).toString();
                Filter = BuildIncludeExcludeFilterFromIni (QStringToConstChar(WindowName), "", GetMainFileDescriptor());
            }
        }
        break;
    }

    if (ui->listWidgetProcesses->count() == 0)
    {
        ThrowError(MESSAGE_STOP, "No Process selected !");
        FreeIncludeExcludeFilter (Filter);
        return;
    }
    else {
        idx = ui->listWidgetProcesses->currentIndex().row();
        strcpy(CurProcess, QStringToConstChar(ui->listWidgetProcesses->item(idx)->text()));
    }

    IniFileDataBaseWriteString("BasicCalibrationSettings",
                               "ApplProcessSymbol", CurProcess, GetMainFileDescriptor());

    UniqueNumber = AquireUniqueNumber();
    pappldata = ConnectToProcessDebugInfos (UniqueNumber,
                                            CurProcess,
                                            &Pid,
                                            nullptr,
                                            nullptr,
                                            nullptr,
                                            0);
    if (pappldata != nullptr) {
        SECTION_ADDR_RANGES_FILTER *GlobalSectionFilter = BuildGlobalAddrRangeFilter (pappldata);


        switch (ui->tabWidgetTarget->currentIndex()) {
        case 0:                  /* Save into a SVL file        */
        default:
            strcpy(SvlFileName, QStringToConstChar(ui->lineEditSVL->text()));
            if (file_exists(SvlFileName)) {
                if (ThrowError(ERROR_OKCANCEL, "File \"%s\" exists already. "
                               "Overwrite it ?", SvlFileName) == IDCANCEL) {
                    FreeIncludeExcludeFilter (Filter);
                    DeleteAddrRangeFilter (GlobalSectionFilter);
                    RemoveConnectFromProcessDebugInfos (UniqueNumber);
                    FreeUniqueNumber (UniqueNumber);
                    return;
                }
            }

            IniFileDataBaseWriteString("BasicCalibrationSettings",
                                       "ApplicationTarget", "FILE", GetMainFileDescriptor());
            IniFileDataBaseWriteString("BasicCalibrationSettings",
                        "SvlWriteFileName", SvlFileName, GetMainFileDescriptor());
            IniFileDataBaseWriteString("BasicCalibrationSettings",
                        "ApplProcessSymbol", CurProcess, GetMainFileDescriptor());

            if (!WriteProcessToSVLOrSAL(SvlFileName, CurProcess, pappldata,
                                        nullptr, Filter, GlobalSectionFilter, WPTSOS_SVL, 0)) {
                FreeIncludeExcludeFilter (Filter);
                DeleteAddrRangeFilter (GlobalSectionFilter);
                RemoveConnectFromProcessDebugInfos (UniqueNumber);
                FreeUniqueNumber (UniqueNumber);
                return;
            }
            break;
        case 1:                   /* Save into SAL file        */
            strcpy(SalFileName, QStringToConstChar(ui->lineEditSAL->text()));
            if (file_exists(SalFileName)) {
                if (ThrowError(ERROR_OKCANCEL, "File \"%s\" exists already. "
                               "Overwrite it ?", SalFileName) == IDCANCEL) {
                    FreeIncludeExcludeFilter (Filter);
                    DeleteAddrRangeFilter (GlobalSectionFilter);
                    RemoveConnectFromProcessDebugInfos (UniqueNumber);
                    FreeUniqueNumber (UniqueNumber);
                    return;
                }
            }

            IniFileDataBaseWriteString("BasicCalibrationSettings",
                        "ApplicationTarget", "SALFILE", GetMainFileDescriptor());
            IniFileDataBaseWriteString("BasicCalibrationSettings",
                        "SalWriteFileName", SalFileName, GetMainFileDescriptor());
            if (!WriteProcessToSVLOrSAL(SalFileName, CurProcess, pappldata,
                                        nullptr, Filter, GlobalSectionFilter, WPTSOS_SAL, 0)) {
                FreeIncludeExcludeFilter (Filter);
                DeleteAddrRangeFilter (GlobalSectionFilter);
                RemoveConnectFromProcessDebugInfos (UniqueNumber);
                FreeUniqueNumber (UniqueNumber);
                return;
            }
            break;
        case 2:                   /* Save into SATVL file        */
            strcpy(SatvlFileName, QStringToConstChar(ui->lineEditSATVL->text()));
            if (file_exists(SatvlFileName)) {
                if (ThrowError(ERROR_OKCANCEL, "File \"%s\" exists already. "
                               "Overwrite it ?", SatvlFileName) == IDCANCEL) {
                    FreeIncludeExcludeFilter (Filter);
                    DeleteAddrRangeFilter (GlobalSectionFilter);
                    RemoveConnectFromProcessDebugInfos (UniqueNumber);
                    FreeUniqueNumber (UniqueNumber);
                    return;
                }
            }

            IniFileDataBaseWriteString("BasicCalibrationSettings",
                        "ApplicationTarget", "SATVLFILE", GetMainFileDescriptor());
            IniFileDataBaseWriteString("BasicCalibrationSettings",
                        "SatvlWriteFileName", SatvlFileName, GetMainFileDescriptor());
            if (!WriteProcessToSVLOrSAL(SatvlFileName, CurProcess, pappldata,
                                        nullptr, Filter, GlobalSectionFilter, WPTSOS_SATVL, 0))
            {
                FreeIncludeExcludeFilter (Filter);
                DeleteAddrRangeFilter (GlobalSectionFilter);
                RemoveConnectFromProcessDebugInfos (UniqueNumber);
                FreeUniqueNumber (UniqueNumber);
                return;
            }
            break;
        }
        DeleteAddrRangeFilter (GlobalSectionFilter);
        RemoveConnectFromProcessDebugInfos (UniqueNumber);
    }
    FreeIncludeExcludeFilter (Filter);
    FreeUniqueNumber (UniqueNumber);
    QDialog::accept();
}

void saveprocessesdialog::on_pushButtonSVL_clicked()
{
    QString fileName = FileDialog::getSaveFileName(this, QString ("Save SVL file"), QString(), QString (SVL_EXT));
    if(!fileName.isEmpty())
    {
        ui->lineEditSVL->setText(fileName);
    }
}

void saveprocessesdialog::on_pushButtonSAL_clicked()
{
    QString fileName = FileDialog::getSaveFileName(this, QString ("Save SAL file"), QString(), QString (SAL_EXT));
    if(!fileName.isEmpty())
    {
        ui->lineEditSAL->setText(fileName);
    }
}

void saveprocessesdialog::on_pushButtonSATVL_clicked()
{
    QString fileName = FileDialog::getSaveFileName(this, QString ("Save SATVL file"), QString(), QString (SATVL_EXT));
    if(!fileName.isEmpty())
    {
        ui->lineEditSATVL->setText(fileName);
    }
}
