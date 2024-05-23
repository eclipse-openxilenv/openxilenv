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


#ifndef XCPCONFIGWIDGET_H
#define XCPCONFIGWIDGET_H

#include <QWidget>
#include <QListWidgetItem>

#include "Platform.h"

typedef struct {
    int LoadedFlag;   // 0 -> Data are not loaded 1 -> Data are loaded
    int CalOrMesListBoxVisible;  // 0 -> Measurment visable 1 -> Calibration visable
    int DebugFlag;
    QString ByteOrderString;
    int Channel;
    int Timeout;
    int SeedAndKeyFlag;
    QString SeedKeyDll;
    int ExtIds;
    uint32_t CRO_ID;
    uint32_t CRM_ID;
    uint32_t DTO_ID;
    int Prescaler;
    int EventChannel;
    int OnlyBytePointerInDAQ;  // this is not implemented
    int CalibrationEnable;
    int MoveROM2RAM;
    int SelCalPage;
    uint32_t CalibROMSegment;
    uint32_t CalibROMPageNo;
    uint32_t CalibRAMSegment;
    uint32_t CalibRAMPageNo;
    int VarListCount;
    int VarListCountOld;
    int CalListCount;
    int CalListCountOld;
    int ReadParameterAfterCalib;
    int UseUnit;
    int UseConversion;
    int UseMinMax;
} XCPCFGDLG_SHEET_DATA;

namespace Ui {
class XCPConfigWidget;
}

class XCPConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit XCPConfigWidget(QWidget *parent = nullptr);
    ~XCPConfigWidget();
    void saveXCPConfigData();

private slots:
    void on_pushButtonChange_clicked();

    void on_pushButtonAddBehind_clicked();

    void on_pushButtonAddBefore_clicked();

    void on_pushButtonExport_clicked();

    void on_pushButtonImport_clicked();

    void on_pushButtonInsertBehind_clicked();

    void on_pushButtonInsertBefore_clicked();

    void on_pushButtonCopy_clicked();

    void on_pushButtonCut_clicked();

    void on_listWidgetOfMeasurements_itemClicked(QListWidgetItem *item);

    void on_listWidgetOfCalibrations_itemClicked(QListWidgetItem *item);

private:
    QString BuildLine();
    void listWidgetListOfLabels_itemClicked(QListWidgetItem *item);

    Ui::XCPConfigWidget *ui;
    XCPCFGDLG_SHEET_DATA *xcpconfig_data;
    static int Connection;
    int curConnection;
    QStringList tmpCopyBuffer;
    void initData();
    int FillListBoxes(int CalibrationOrMeasurement);
};

#endif // XCPCONFIGWIDGET_H
