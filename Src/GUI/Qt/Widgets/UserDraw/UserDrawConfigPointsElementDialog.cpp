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


#include "UserDrawConfigPointsElementDialog.h"
#include "ui_UserDrawConfigPointsElementDialog.h"
#include "StringHelpers.h"

extern "C" {
#include "ThrowError.h"
}

UserDrawConfigPointsElementDialog::UserDrawConfigPointsElementDialog(QString &par_Parameter, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UserDrawConfigPointsElementDialog)
{
    ui->setupUi(this);
    m_Parameter = par_Parameter.trimmed();

    Parse();

}

UserDrawConfigPointsElementDialog::~UserDrawConfigPointsElementDialog()
{
    delete ui;
}

QString UserDrawConfigPointsElementDialog::Get()
{
    QString Ret;
    Ret.append("(");
    Ret.append(ui->XPositionLineEdit->text().trimmed());
    Ret.append(",");
    Ret.append(ui->YPositionLineEdit->text().trimmed());
    Ret.append(")");
    return Ret;
}

void UserDrawConfigPointsElementDialog::accept()
{
    if ((ui->XPositionLineEdit->text().trimmed().size() == 0) ||
        (ui->XPositionLineEdit->text().trimmed().size() == 0)) {
        ThrowError (1, "you must enter a X/Y pair of values");
    } else {
        QDialog::accept();
    }
}

bool UserDrawConfigPointsElementDialog::Parse()
{
    int Len = m_Parameter.length();
    if ((Len > 0) && (m_Parameter.at(0) != '(')) {
        ThrowError (1, "expecting a '(' in \"%s\"", QStringToConstChar(m_Parameter));
        return false;
    }
    int ParameterCounter = 0;
    int BraceCounter = 0;
    int StartParameter = 0;
    for (int x = 0; x < Len; x++) {
        QChar c = m_Parameter.at(x);
        if (c == '(') {
            BraceCounter++;
            if (BraceCounter == 1) {
                StartParameter = x;
            }
        } else if (c == ')') {
            if ((BraceCounter == 0) && (x < (Len - 1))) {
                ThrowError (1, "missing '(' in \"%s\"", QStringToConstChar(m_Parameter));
                return false;
            }
            if (BraceCounter == 1) {
                QString Parameter = m_Parameter.mid(StartParameter+1, x - StartParameter-1);
                if (ParameterCounter == 0) ui->XPositionLineEdit->setText(Parameter.trimmed());
                else if (ParameterCounter == 1) ui->YPositionLineEdit->setText(Parameter.trimmed());
                ParameterCounter++;
                StartParameter = x;
            }
            BraceCounter--;
        } else if (c == ',') {
            if (BraceCounter == 1) {
                QString Parameter = m_Parameter.mid(StartParameter+1, x - StartParameter-1);
                if (ParameterCounter == 0) ui->XPositionLineEdit->setText(Parameter.trimmed());
                else if (ParameterCounter == 1) ui->YPositionLineEdit->setText(Parameter.trimmed());
                ParameterCounter++;
                StartParameter = x;
            }
        }
    }
    return true;
}
