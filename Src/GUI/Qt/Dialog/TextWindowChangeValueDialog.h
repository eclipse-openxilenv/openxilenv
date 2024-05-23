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


#ifndef TEXTWINDOWCHANGEVALUEDIALOG_H
#define TEXTWINDOWCHANGEVALUEDIALOG_H

#include <QDialog>
#include "Dialog.h"

extern "C" {
#include "Blackboard.h"
}

namespace Ui {
class TextWindowChangeValueDialog;
}

class TextWindowChangeValueDialog : public Dialog
{
    Q_OBJECT

public:
    enum Displaytype {
        decimal = 0,
        binary,
        hexadecimal,
        physically
    };

    explicit TextWindowChangeValueDialog(QWidget *parent = nullptr);
    ~TextWindowChangeValueDialog();

    void setVariableName(QString arg_name);
    QString variableName();
    void setDisplaytype(Displaytype arg_displaytype);
    Displaytype displaytype();
    void setLinear(bool arg_linear);
    bool isLinear();
    void setStep(double arg_step);
    double step();
    void setValue(double arg_value);
    void setValue(union BB_VARI arg_value, enum BB_DATA_TYPES par_Type, int par_Base = 10);
    double value();
    enum BB_DATA_TYPES value(union BB_VARI *ret_Value);
    void SetVid(int par_Vid);


private slots:
    void on_d_radioButtonDecimal_toggled(bool checked);

    void on_d_radioButtonHexadecimal_toggled(bool checked);

    void on_d_radioButtonBinary_toggled(bool checked);

    void on_d_radioButtonPhysically_toggled(bool checked);

    void on_d_lineEditStep_ValueChanged();

    void on_d_radioButtonLinear_toggled(bool checked);

    void on_d_radioButtonPerCent_toggled(bool checked);

    void ShouldWideningSlot(int par_Widening, int par_Heightening);

private:
    Ui::TextWindowChangeValueDialog *ui;
};

#endif // TEXTWINDOWCHANGEVALUEDIALOG_H
