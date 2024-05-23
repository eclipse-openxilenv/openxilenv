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


#ifndef CONFIGSIGNALCONVERSIONDIALOG_H
#define CONFIGSIGNALCONVERSIONDIALOG_H

#include <QDialog>
#include "Dialog.h"

namespace Ui {
class ConfigSignalConversionDialog;
}

class ConfigSignalConversionDialog : public Dialog
{
    Q_OBJECT

public:
    explicit ConfigSignalConversionDialog(QString par_SignalType, QString par_Unit, QWidget *parent = nullptr);
    ~ConfigSignalConversionDialog();
    void SetFacOff(double par_Factor, double par_Offset);
    void SetCurce(QString &par_Curve);
    void SetEquation(QString &par_Equation);

    void SetMinMax(double par_Min, double par_Max);

    int GetConversionType();
    double GetMin();
    double GetMax();
    double GetFactor();
    double GetOffset();
    QString GetCurve();
    QString GetEquation();

private slots:
    void on_FacOffRadioButton_toggled(bool checked);

    void on_CurveRadioButton_toggled(bool checked);

    void on_EquationRadioButton_toggled(bool checked);

private:
    Ui::ConfigSignalConversionDialog *ui;
};

#endif // CONFIGSIGNALCONVERSIONDIALOG_H
