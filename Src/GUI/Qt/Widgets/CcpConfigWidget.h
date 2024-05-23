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


#ifndef CCPCONFIGWIDGET_H
#define CCPCONFIGWIDGET_H

#include <QWidget>
#include <QListWidgetItem>

#include "Platform.h"

typedef struct {
    int LoadedFlag;   // 0 -> Data are not loaded 1 -> Data are loaded
    int CalOrMesListBoxVisible;  // 0 -> Measurment visable 1 -> Calibration visable
    int DebugFlag;
    QString CCPVersionString;
    int CCPVersion;
    QString ByteOrderString;
    int Channel;
    int Timeout;
    int SeedAndKeyFlag;
    QString SeedKeyDll;
    QString SeedKeyDllCal;
    int ExtIds;
    uint32_t CRO_ID;
    uint32_t CRM_ID;
    uint32_t DTO_ID;
    int Prescaler;
    unsigned int StationAddress;
    int EventChannel;
    int OnlyBytePointerInDAQ;   // this is not implemented
    int CalibrationEnable;
    int MoveROM2RAM;
    int SelCalPage;
    uint32_t CalibROMStartAddr;
    uint32_t CalibRAMStartAddr;
    uint32_t CalibROMRAMSize;
    int VarListCount;
    int VarListCountOld;
    int CalListCount;
    int CalListCountOld;
    int ReadParameterAfterCalib;
    int UseUnit;
    int UseConversion;
    int UseMinMax;
} CCPCFGDLG_SHEET_DATA;

namespace Ui {
class CCPConfigWidget;
}

class CCPConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CCPConfigWidget(QWidget *parent = nullptr);
    void saveCCPConfigData();
    ~CCPConfigWidget();

private slots:
    void on_pushButtonCut_clicked();

    void on_pushButtonCopy_clicked();

    void on_pushButtonInsertBefore_clicked();

    void on_pushButtonInsertBehind_clicked();

    void on_pushButtonImport_clicked();

    void on_pushButtonExport_clicked();

    void on_pushButtonAddBefore_clicked();

    void on_pushButtonAddBehind_clicked();

    void on_pushButtonChange_clicked();

    void on_listWidgetOfMeasurements_itemClicked(QListWidgetItem *item);

    void on_listWidgetOfCalibrations_itemClicked(QListWidgetItem *item);

private:
    QString BuildLine();
    void listWidgetListOfLabels_itemClicked(QListWidgetItem *item);
    Ui::CCPConfigWidget *ui;
    static int Connection;
    int curConnection;
    QString TranslateCCPVersionNumber2String (int number);
    int TranslateCCPVersionString2Number (QString &VersionString);
    QStringList tmpCopyBuffer;
    void FillListBoxes();
    int FillListBoxes (int CalibrationOrMeasurement);
    void initccpcfgdata();
    CCPCFGDLG_SHEET_DATA *ccpconfig_data;
};

#endif // CCPCONFIGWIDGET_H
