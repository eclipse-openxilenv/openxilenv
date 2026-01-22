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


#include <stdint.h>
#include <inttypes.h>

#include "LoadValuesDialog.h"
#include "ui_LoadValuesDialog.h"

#include "FileDialog.h"
#include "QtIniFile.h"
#include "StringHelpers.h"

#include <QListWidget>

#define GHFFI_NOT_SUPPORTED    0
#define GHFFI_SUPPORTED        1
#define GHFFI_INTEL_SUPPORTED  (2+GHFFI_SUPPORTED)
#define GHFFI_SREC_SUPPORTED   (4+GHFFI_SUPPORTED)

extern "C"
{
    #include "DebugInfos.h"
    #include "LoadSaveToFile.h"
    #include "ThrowError.h"
    #include "Scheduler.h"
    #include "FileExtensions.h"
    #include "UniqueNumber.h"
    #include "DebugInfoDB.h"
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


LoadValuesDialog::LoadValuesDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::LoadValuesDialog)
{
    ui->setupUi(this);

    static int  TEN_DAT_File_Flag;
    char SymFileName[MAX_PATH], BinFileName[MAX_PATH], *ptr,
         SvlFileName[MAX_PATH], CurProcess[MAX_PATH], strg[MAX_PATH];
    int ApplTarget;

    // Raed process name from INI file
    DBGetPrivateProfileString("BasicCalibrationSettings", "ApplProcessSymbol",
                  "", CurProcess, sizeof(CurProcess), ini_path);

    // Read memory type  from INI file
    DBGetPrivateProfileString("BasicCalibrationSettings", "MemoryType",
                  "LSB_FISRT", strg, sizeof(strg), ini_path);
    if (strcmp(strg, "LSB_FISRT") == 0) {
        LittleBigEndian = LSB_FIRST_FORMAT;
    }
    else if (strcmp(strg, "MSB_FIRST") == 0) {
        LittleBigEndian = MSB_FIRST_FORMAT;
    }
    else {
        ThrowError(MESSAGE_STOP, "Wrong Memory-Type in INI-File \"%s\" !\n"
                            "Using \"LSB_FIRST\" as default !", ini_path);
        LittleBigEndian = LSB_FIRST_FORMAT;
    }

    DBGetPrivateProfileString("BasicCalibrationSettings",
                "StartAddress", "0x0", strg, sizeof (strg), ini_path);
    ui->lineEditStartAdress->setText(CharToQString(strg));
    DBGetPrivateProfileString("BasicCalibrationSettings",
                "BlockSize", "0xFFFFFFFF", strg, sizeof (strg), ini_path);
    ui->lineEditAdressWinSize->setText(CharToQString(strg));

    // Read target of the calibration data from INI file
    DBGetPrivateProfileString("BasicCalibrationSettings", "ApplicationSource",
                  "PROCESS", strg, sizeof(strg), ini_path);
    if (strcmp(strg, "PROCESS") == 0) {
        ApplTarget = TO_PROCESS;
    }
    else if (strcmp(strg, "FILE") == 0) {
        ApplTarget = TO_FILE;
    }
    else {
        ThrowError(MESSAGE_STOP, "Wrong Application-Source in INI-File "
                            "\"%s\" !\nUsing \"PROCESS\" as default !",
                            ini_path);
        ApplTarget = TO_PROCESS;
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

    // If there is only one process selct this
    if (ui->listWidgetProcesses->count() == 1)
    {
        ui->listWidgetProcesses->row(nullptr);
    }

    DBGetPrivateProfileString("BasicCalibrationSettings", "SymbolFileName",
                  "*.dat", SymFileName, sizeof(SymFileName), ini_path);
    ptr = strrchr(SymFileName, '.');
    if (ptr != nullptr) {              /* file extension exists?  */
        if (strcmp(".DAT", strupr(ptr)) == 0) {
            TEN_DAT_File_Flag=TRUE;
        }
        else {
            TEN_DAT_File_Flag=FALSE;
        }
    }
    else {
        TEN_DAT_File_Flag=TRUE;
    }

    DBGetPrivateProfileString("BasicCalibrationSettings", "BinaryFileName",
                  "*.bin", BinFileName, sizeof(BinFileName), ini_path);
    DBGetPrivateProfileString("BasicCalibrationSettings", "SvlWriteFileName",
                  "*.svl", SvlFileName, sizeof(SvlFileName), ini_path);

    if (TEN_DAT_File_Flag == TRUE)
    {
        DBGetPrivateProfileString("BasicCalibrationSettings", "GpLabelAddress",
                      "B000", GpLabelAddressStr, sizeof(GpLabelAddressStr),
                      ini_path);
    }
    else
    {
        DBGetPrivateProfileString("BasicCalibrationSettings", "GpLabelAddress",
                      "", GpLabelAddressStr, sizeof(GpLabelAddressStr),
                      ini_path);
    }

    ui->lineEditSymbolFile->setText(SymFileName);
    ui->lineEditBinaryFile->setText(BinFileName);
    ui->lineEditChooseSVL->setText(SvlFileName);
    ui->lineEditGPLabelAdr->setText(GpLabelAddressStr);

    if (ApplTarget == TO_PROCESS) {
        ui->tabWidgetTarget->setCurrentIndex(0);
    }
    else {
        ui->tabWidgetTarget->setCurrentIndex(1);
    }

    if (LittleBigEndian == LSB_FIRST_FORMAT) {
        ui->radioButtonLsbFirst->setChecked(true);
    }
    else {
        ui->radioButtonMsbFirst->setChecked(true);
    }

    INCLUDE_EXCLUDE_FILTER *Filter = BuildIncludeExcludeFilterFromIni("BasicCalibrationSettings", "", ini_path);
    ui->groupBoxFilter->SetFilter(Filter);
    FreeIncludeExcludeFilter (Filter);

    QString File(INIFILE);
    QString Section("GUI/AllCalibrationTreeWindows");
    QString EmptyString("");
    for (int x = 0;;x++) {
        QString Entry = QString("ow%1").arg(x);
        QString WindowName = ScQt_IniFileDataBaseReadString(Section, Entry, EmptyString, File);
        if (WindowName.isEmpty()) break;
        Entry = QString("process");
        QString ProcessName = ScQt_IniFileDataBaseReadString(WindowName, Entry, EmptyString, File);
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

LoadValuesDialog::~LoadValuesDialog()
{
    delete ui;
}

void LoadValuesDialog::on_pushButtonFileSVL_clicked()
{
    QString fileName = FileDialog::getOpenFileName(this, QString ("Open SVL file"), QString(), QString (SVL_EXT));
    if(!fileName.isEmpty())
    {
        ui->lineEditChooseSVL->setText(fileName);
    }
}

void LoadValuesDialog::on_pushButtonFileSymbol_clicked()
{
    const char *ptr = "Symbol file (*.sym);;"
                      "DAT Symbol file (*.dat);;"
                      "SRF Symbol file (*.srf);;"
                      "All files (*.*)";

    QString fileName = FileDialog::getOpenFileName(this, QString ("Open symbol file"), QString(), QString (ptr));
    if(!fileName.isEmpty()) {
        ui->lineEditSymbolFile->setText(fileName);
    }
}

void LoadValuesDialog::on_pushButtonFileBinary_clicked()
{
    QString fileName = FileDialog::getOpenFileName(this, QString ("Open binary file"), QString(), QString ("Binary file (*.bin);;"
                                                                                                           "Hex files (*.hex);;"
                                                                                                           "S19 files (*.s19);;"
                                                                                                           "All files (*.*);;"));
    if(!fileName.isEmpty())
    {
        ui->lineEditBinaryFile->setText(fileName);
    }
}

void LoadValuesDialog::accept()
{
    static int  FileFlag;
    char SymFileName[MAX_PATH], BinFileName[MAX_PATH], HexFileName[MAX_PATH], *ptr,
         SvlFileName[MAX_PATH], *BinArray, CurProcess[MAX_PATH], strg[MAX_PATH];
    char *endptr;    
    FILE *fp_sym;
    int idx;
    INCLUDE_EXCLUDE_FILTER *Filter;
    uint64_t StartAddr;
    uint64_t BlockSize;
    int IsBinFile=TRUE;
    uint64_t BinOffset;
    PROCESS_APPL_DATA *pappldata;
    int UniqueNumber;
    int Pid;


    STRING_COPY_TO_ARRAY(SymFileName, QStringToConstChar(ui->lineEditSymbolFile->text()));
    STRING_COPY_TO_ARRAY(BinFileName, QStringToConstChar(ui->lineEditBinaryFile->text()));

    ptr = strrchr(SymFileName, '.');
    if (ptr != nullptr) {              /* file extension exists?  */
        if (strcmp(".DAT", strupr(ptr)) == 0) {
            FileFlag=TRUE;
        } else {
            FileFlag=FALSE;
        }
    } else {
        FileFlag=TRUE;
    }

    STRING_COPY_TO_ARRAY(GpLabelAddressStr, ui->lineEditGPLabelAdr->text().toLatin1());

    if (FileFlag == TRUE) {
        strtoull(GpLabelAddressStr, &endptr, 16);
        if ((*endptr != 0) && (strlen(GpLabelAddressStr) != 0)) {
            ThrowError(MESSAGE_STOP, "Enter GP-Label-Address in hex !");
            return;
        }
    }
    DBWritePrivateProfileString("BasicCalibrationSettings",
                "GpLabelAddress", GpLabelAddressStr, ini_path);

    Filter = ui->groupBoxFilter->GetFilter();
    SaveIncludeExcludeListsToIni (Filter, "BasicCalibrationSettings", "", ini_path);

    char *End;
    StartAddr = strtoull (QStringToConstChar(ui->lineEditStartAdress->text()), &End, 0);
    BlockSize = strtoull (QStringToConstChar(ui->lineEditAdressWinSize->text()), &End, 0);
    if ((StartAddr + BlockSize) > 0xFFFFFFFFULL)
        BlockSize = 0xFFFFFFFFUL - StartAddr;
    PrintFormatToString (strg, sizeof(strg), "0x%" PRIX64 "", StartAddr);
    DBWritePrivateProfileString("BasicCalibrationSettings",
                "StartAddress", strg, ini_path);
    PrintFormatToString (strg, sizeof(strg), "0x%" PRIX64 "", BlockSize);
    DBWritePrivateProfileString("BasicCalibrationSettings",
                "BlockSize", strg, ini_path);

    if (ui->listWidgetProcesses->selectedItems().isEmpty()) {
        ThrowError(MESSAGE_STOP, "No Process selected !");
        FreeIncludeExcludeFilter (Filter);
        return;
    } else {
        idx = ui->listWidgetProcesses->currentIndex().row();
        if (idx >= 0) {
            STRING_COPY_TO_ARRAY(CurProcess, QStringToConstChar(ui->listWidgetProcesses->item(idx)->text()));
        }
    }

    DBWritePrivateProfileString("BasicCalibrationSettings",
                   "ApplProcessSymbol", CurProcess, ini_path);


    if (!ConvertBinToHexIfNecessary(BinFileName, HexFileName, SymFileName, CurProcess, &IsBinFile, &BinOffset, GpLabelAddressStr)) {
        FreeIncludeExcludeFilter (Filter);
        return;
    }

    fp_sym = OpenSymBin(SymFileName, BinFileName, &BinArray, BinOffset,
                        &FileFlag, GpLabelAddressStr);
    if (fp_sym == nullptr) {
        FreeIncludeExcludeFilter (Filter);
        return;
    }

    DBWritePrivateProfileString("BasicCalibrationSettings",
                "SymbolFileName", SymFileName, ini_path);
    if (IsBinFile) {
        DBWritePrivateProfileString("BasicCalibrationSettings",
                    "BinaryFileName", BinFileName, ini_path);
    } else {
        DBWritePrivateProfileString("BasicCalibrationSettings",
                    "BinaryFileName", HexFileName, ini_path);

    }

    if (ui->radioButtonLsbFirst->isChecked()) {    /* select lsb first format */
        LittleBigEndian = LSB_FIRST_FORMAT;
        DBWritePrivateProfileString("BasicCalibrationSettings",
                      "MemoryType", "LSB_FIRST", ini_path);
    } else {
        LittleBigEndian = MSB_FIRST_FORMAT;
        DBWritePrivateProfileString("BasicCalibrationSettings",
                      "MemoryType", "MSB_FIRST", ini_path);
    }

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

        switch (ui->tabWidgetFilter->currentIndex()) {
        case 0:
        default:    // Schreib es in einen Prozess
            DBWritePrivateProfileString("BasicCalibrationSettings",
                        "ApplicationSource", "PROCESS", ini_path);
            if (!WriteValuesToProcess(fp_sym, BinArray, BinOffset, CurProcess, Pid, pappldata,
                                      FileFlag, Filter, GlobalSectionFilter,
                                      StartAddr, BlockSize, LittleBigEndian))
            {
                FreeIncludeExcludeFilter (Filter);
                DeleteAddrRangeFilter (GlobalSectionFilter);
                RemoveConnectFromProcessDebugInfos (UniqueNumber);
                FreeUniqueNumber (UniqueNumber);
                return;
            }
            break;
        case 1:      // Schreib es in ein SVL-File
            STRING_COPY_TO_ARRAY(SvlFileName, QStringToConstChar(ui->lineEditChooseSVL->text()));
            if (file_exists(SvlFileName)) {
                if (ThrowError(ERROR_OKCANCEL, "File \"%s\" exists already. "
                               "Overwrite it ?", SvlFileName) == IDCANCEL)
                {
                    FreeIncludeExcludeFilter (Filter);
                    DeleteAddrRangeFilter (GlobalSectionFilter);
                    RemoveConnectFromProcessDebugInfos (UniqueNumber);
                    FreeUniqueNumber (UniqueNumber);
                    return;
                }
            }
            DBWritePrivateProfileString("BasicCalibrationSettings",
                        "ApplicationSource", "FILE", ini_path);
            DBWritePrivateProfileString("BasicCalibrationSettings",
                        "SvlWriteFileName", SvlFileName, ini_path);
            if (!WriteValuesToSVL(fp_sym, BinArray, BinOffset, SvlFileName,
                                  CurProcess, pappldata, FileFlag,
                                  Filter, GlobalSectionFilter, StartAddr, BlockSize, LittleBigEndian))
            {
                FreeIncludeExcludeFilter (Filter);
                RemoveConnectFromProcessDebugInfos (UniqueNumber);
                FreeUniqueNumber (UniqueNumber);
                return;
            }
            break;
        }
        DeleteAddrRangeFilter (GlobalSectionFilter);
        RemoveConnectFromProcessDebugInfos (UniqueNumber);
    }
    FreeUniqueNumber (UniqueNumber);
    QDialog::accept();
}

