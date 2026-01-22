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


#ifndef CONFIGCALLIBRATIONTREEVIEWDIALOG_H
#define CONFIGCALLIBRATIONTREEVIEWDIALOG_H

#include <QDialog>
#include <Dialog.h>

extern "C" {
#include "Wildcards.h"  // wegen INCLUDE_EXCLUDE_FILTER
}

namespace Ui {
class ConfigCalibrationTreeViewDialog;
}

class ConfigCalibrationTreeViewDialog : public Dialog
{
    Q_OBJECT

public:
    explicit ConfigCalibrationTreeViewDialog(QString &par_WindowName,
                                              char *pProcessName,
                                              INCLUDE_EXCLUDE_FILTER *par_IncExcFilter,
                                              int par_ShowValue, int par_ShowAddresse, int par_ShowDataType,
                                              int par_HexOrDec,
                                              QWidget *parent = nullptr);
    ~ConfigCalibrationTreeViewDialog();

    QString GetWindowName ();
    void GetProcessName (char *ret_Processname, int par_Maxc);
    INCLUDE_EXCLUDE_FILTER *GetFilter ();
    int GetShowValue();
    int GetShowAddress();
    int GetShowDataType();
    int GetHexOrDec();

private:
    Ui::ConfigCalibrationTreeViewDialog *ui;
};

#endif // CONFIGCALLIBRATIONTREEVIEWDIALOG_H
