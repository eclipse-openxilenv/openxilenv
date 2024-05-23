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


#ifndef CHANGEVALUETEXTDIALOG_H
#define CHANGEVALUETEXTDIALOG_H

#include <stdint.h>
#include <QDialog>
#include <Dialog.h>

extern "C" {
#include "Blackboard.h"
}

namespace Ui {
class ChangeValueTextDialog;
}

class ChangeValueTextDialog : public Dialog
{
    Q_OBJECT

public:
    explicit ChangeValueTextDialog(QWidget *parent = nullptr, QString par_Header = QString(), int par_DataType = BB_DOUBLE, bool par_PhysicalFlag = false, const char *par_Converion = nullptr);
    ~ChangeValueTextDialog();
    void SetValue (int32_t par_Value, int par_Base);
    void SetValue (uint32_t par_Value, int par_Base);
    void SetValue (int64_t  par_Value, int par_Base);
    void SetValue (uint64_t par_Value, int par_Base);
    void SetValue (double par_Value);
    void SetMinMaxValues (double par_MinValue, double par_MaxValue);

    double GetDoubleValue();
    long long GetInt64Value();
    unsigned long long GetUInt64Value();
    int GetIntValue();

private:
    Ui::ChangeValueTextDialog *ui;
};

#endif // CHANGEVALUETEXTDIALOG_H
