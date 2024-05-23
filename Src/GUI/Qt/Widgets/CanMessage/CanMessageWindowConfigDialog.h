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


#ifndef CANMESSAGEWINDOWCONFIGDIALOG_H
#define CANMESSAGEWINDOWCONFIGDIALOG_H

#include <QDialog>
#include "CanMessageWindowWidget.h"
#include "Dialog.h"
extern "C" {
#include "CanFifo.h"
}

namespace Ui {
class CANMessageWindowConfigDialog;
}

class CANMessageWindowConfigDialog : public Dialog
{
    Q_OBJECT

public:
    explicit CANMessageWindowConfigDialog(QString par_WindowTitle,
                                          int par_AcceptanceMaskRows,
                                          CAN_ACCEPT_MASK *par_AcceptanceMask,
                                          bool par_DisplayColumnCounterFlag,
                                          bool par_DisplayColumnTimeAbsoluteFlag,
                                          bool par_DisplayColumnTimeDiffFlag,
                                          bool par_DisplayColumnTimeDiffMinMaxFlag,
                                          bool par_Record2Disk,
                                          QString &par_RecorderFileName,
                                          bool par_TriggerActiv,
                                          QString &par_TriggerEquation,
                                          QWidget *parent = nullptr);
    ~CANMessageWindowConfigDialog();

    QString GetWindowTitle ();
    bool WindowTitleChanged ();
    int GetAcceptanceMask (CAN_ACCEPT_MASK *AcceptanceMask);
    void GetDisplayFlags (bool *ret_DisplayColumnCounterFlag,
                          bool *ret_DisplayColumnTimeAbsoluteFlag,
                          bool *ret_DisplayColumnTimeDiffFlag,
                          bool *ret_DisplayColumnTimeDiffMinMaxFlag);
    QString GetRecorderFilename (bool *ret_Record2Disk);
    QString GetTrigger (bool *ret_TriggerActiv);

private slots:
    void on_NewPushButton_clicked();

    void on_DeletePushButton_clicked();

//public slots:
    void accept();

    void on_RecorderFilePushButton_clicked();

private:
    Ui::CANMessageWindowConfigDialog *ui;

    QString m_WindowTitle;
};

#endif // CANMESSAGEWINDOWCONFIGDIALOG_H
