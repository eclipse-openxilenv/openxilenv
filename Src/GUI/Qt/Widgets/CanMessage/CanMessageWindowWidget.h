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


#ifndef CANMESSAGEWINDOWWIDGET_H
#define CANMESSAGEWINDOWWIDGET_H
#include <QWidget>
#include <QTreeView>
#include "MdiWindowWidget.h"
#include "CanMessageWindowModel.h"
#include "CanMessageTreeView.h"

extern "C" {
#include "CanFifo.h"
#include "Platform.h"
}


class CANMessageWindowWidget : public MdiWindowWidget
{
    Q_OBJECT

public:
    explicit CANMessageWindowWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent = nullptr);
    ~CANMessageWindowWidget() Q_DECL_OVERRIDE;

    virtual bool writeToIni() Q_DECL_OVERRIDE;
    virtual bool readFromIni() Q_DECL_OVERRIDE;
    virtual void CyclicUpdate() Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    virtual CustomDialogFrame* dialogSettings(QWidget *arg_parent) Q_DECL_OVERRIDE;

public slots:
    void ConfigDialogSlot();
    void ClearSlot();

protected:
    void FlushMessageQueue ();

    void BuildAbsoluteTimeString(double Ts, char *Txt, int Maxc);
    int ReadAndProcessCANMessages();
    double TimestampCalc(CAN_FD_FIFO_ELEM *pCanMessage);
    void TimestampCalcLine(CAN_MESSAGE_LINE *pcml);

    void CalcColumnNummbers();
    void GetAllColumnsWidth();
    void SetAllColumnsWidth();
    void FitNewColumnsToHeader();
    void SwitchColumnsOnOff();
private:
    int m_CanFiFoHandle;
    int m_OnOff;

    CAN_ACCEPT_MASK m_cam[MAX_CAN_ACCEPTANCE_MASK_ELEMENTS+1];   // das letzte ist mit -1
    int m_cam_count;

    int m_DisplayFormat;
#define DISPLAY_HEX 0
#define DISPLAY_DEC 1
#define DISPLAY_BIN 2

    bool m_Record2Disk;
    QString m_RecorderFileName;
    FILE *m_fh;
    bool m_TriggerActiv;
    QString m_TriggerEquation;
    int m_Triggered;
    int m_MaxCanMsgRec;

    int m_RefreshCounter;
    double m_AbtastPeriodeInMs;
    bool m_DisplayColumnCounterFlag;
    bool m_DisplayColumnTimeAbsoluteFlag;
    bool m_DisplayColumnTimeDiffFlag;
    bool m_DisplayColumnTimeDiffMinMaxFlag;

    int m_DisplayColumnCounterWidth;
    int m_DisplayColumnTimeAbsoluteWidth;
    int m_DisplayColumnTimeDiffWidth;
    int m_DisplayColumnTimeDiffMinMaxWidth;
    int m_DisplayColumnDirWidth;
    int m_DisplayColumnChannelWidth;
    int m_DisplayColumnIdWidth;
    int m_DisplayColumnSizeWidth;
    int m_DisplayColumnDataWidth;
#ifdef _WIN32
    SYSTEMTIME m_SystemTimeStartCANLogging;
#else
    struct timespec m_SystemTimeStartCANLogging;
#endif
    int m_WaitForFirstMessageFlag;
    double m_TsOffset;

    CANMessageTreeView *m_TreeView;
    CANMessageWindowModel *m_Model;

//signals:

public slots:
    virtual void changeColor(QColor arg_color) Q_DECL_OVERRIDE;
    virtual void changeFont(QFont arg_font) Q_DECL_OVERRIDE;
    virtual void changeWindowName(QString arg_name) Q_DECL_OVERRIDE;
    virtual void changeVariable(QString arg_variable, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void changeVaraibles(QStringList arg_variables, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void resetDefaultVariables(QStringList arg_variables) Q_DECL_OVERRIDE;

};



#endif // CANMESSAGEWINDOWWIDGET_H
