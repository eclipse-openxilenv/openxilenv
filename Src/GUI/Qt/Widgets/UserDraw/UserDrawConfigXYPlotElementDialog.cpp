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


#include "UserDrawConfigXYPlotElementDialog.h"
#include "ui_UserDrawConfigXYPlotElementDialog.h"

#include <QColorDialog>
#include "StringHelpers.h"
#include "UserDrawElement.h"
#include "EditTextReplaceDialog.h"
extern "C" {
#include "Blackboard.h"
}

UserDrawConfigXYPlotElementDialog::UserDrawConfigXYPlotElementDialog(QString &par_Parameter, QWidget *parent) : Dialog(parent),
    ui(new Ui::UserDrawConfigXYPlotElementDialog)
{
    ui->setupUi(this);
    ui->LineWidthSpinBox->setValue(1);
    m_Parameter = par_Parameter.trimmed();
    int Len = m_Parameter.length();
    if ((Len >= 2) && (m_Parameter.at(0) == '(') && (m_Parameter.at(Len-1) == ')')) {
       m_Parameter.remove(Len-1, 1);
       m_Parameter.remove(0, 1);
    }

    Parse();

    ui->ColorLineEdit->setValidator(new ColorTextValidator (ui->ColorLineEdit));
}

UserDrawConfigXYPlotElementDialog::~UserDrawConfigXYPlotElementDialog()
{
    delete ui;
}

QString UserDrawConfigXYPlotElementDialog::Get()
{
    QString Ret;
    Ret.append("(");
    // X
    Ret.append(ui->XSignalLineEdit->text().trimmed());
    Ret.append(",");
    if (ui->XPhysicalRadioButton->isChecked()) {
        Ret.append("phys");
    } else {
        Ret.append("raw");
    }
    Ret.append(",");
    Ret.append(ui->XMinLineEdit->text().trimmed());
    Ret.append(",");
    Ret.append(ui->XMaxLineEdit->text().trimmed());
    Ret.append(",");
    // Y
    Ret.append(ui->YSignalLineEdit->text().trimmed());
    Ret.append(",");
    if (ui->YPhysicalRadioButton->isChecked()) {
        Ret.append("phys");
    } else {
        Ret.append("raw");
    }
    Ret.append(",");
    Ret.append(ui->YMinLineEdit->text().trimmed());
    Ret.append(",");
    Ret.append(ui->YMaxLineEdit->text().trimmed());
    Ret.append(",");

    // Color,line/point,wdith
    Ret.append(ui->ColorLineEdit->text().trimmed());
    Ret.append(",");
    if (ui->PointRadioButton->isChecked()) {
        Ret.append("point");
    } else {
        Ret.append("line");
    }
    Ret.append(",");
    Ret.append(ui->LineWidthSpinBox->text().trimmed());
    Ret.append(")");
    return Ret;
}

bool UserDrawConfigXYPlotElementDialog::Parse()
{
    QStringList InList = m_Parameter.split(',');
    if (InList.size() >= 11) {
        QString ValueString;
        BaseValue Value;
        // X
        ui->XSignalLineEdit->setText(InList.at(0).trimmed());
        if (InList.at(1).trimmed().compare("phys", Qt::CaseInsensitive) == 0) {
            ui->XPhysicalRadioButton->setChecked(true);
        } else {
            ui->XRawRadioButton->setChecked(true);
        }
        ui->XMinLineEdit->setText(InList.at(2).trimmed());
        ui->XMaxLineEdit->setText(InList.at(3).trimmed());

        // Y
        ui->YSignalLineEdit->setText(InList.at(4).trimmed());
        if (InList.at(5).trimmed().compare("phys", Qt::CaseInsensitive) == 0) {
            ui->YPhysicalRadioButton->setChecked(true);
        } else {
            ui->YRawRadioButton->setChecked(true);
        }
        ui->YMinLineEdit->setText(InList.at(6).trimmed());
        ui->YMaxLineEdit->setText(InList.at(7).trimmed());

        // Color
        ui->ColorLineEdit->setText(InList.at(8).trimmed());
        int IntColor = ColorStringToInt (ui->ColorLineEdit->text(), nullptr);
        QColor Color = QColor ((IntColor >> 0) & 0xFF,
                               (IntColor >> 8) & 0xFF,
                               (IntColor >> 16) & 0xFF);
        SetColorView (Color);
        // Line/Point
        if (InList.at(9).trimmed().compare("point", Qt::CaseInsensitive) == 0) {
            ui->PointRadioButton->setChecked(true);
        } else {
            ui->LineRadioButton->setChecked(true);
        }
        // Width
        ui->LineWidthSpinBox->setValue(InList.at(10).toUInt());

    }
    return false;
}

void UserDrawConfigXYPlotElementDialog::SetColorView(QColor color)
{
    ui->ColorLabel->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));
}

void UserDrawConfigXYPlotElementDialog::on_SetXPushButton_clicked()
{
    QModelIndex Index = ui->SignalsListView->selectionModel()->currentIndex();
    if (Index.isValid()) {
        QString Name = Index.data(Qt::DisplayRole).toString();
        ui->XSignalLineEdit->setText(Name);
        int Vid = get_bbvarivid_by_name(QStringToConstChar(Name));
        if (Vid > 0) {
            double Min = get_bbvari_min(Vid);
            ui->XMinLineEdit->setText(QString("%1").arg(Min));
            double Max = get_bbvari_max(Vid);
            ui->XMaxLineEdit->setText(QString("%1").arg(Max));
            int IntColor = get_bbvari_color(Vid);
            QColor Color = QColor ((IntColor >> 0) & 0xFF,
                                   (IntColor >> 8) & 0xFF,
                                   (IntColor >> 16) & 0xFF);
            char Help[64];
            sprintf (Help , "0x%02X:0x%02X:0x%02X", Color.red(), Color.green(), Color.blue());
            ui->ColorLineEdit->setText(Help);
            SetColorView (Color);
            if (get_bbvari_conversiontype(Vid) == 1) {
                ui->XPhysicalRadioButton->setChecked(true);
            } else {
                ui->XRawRadioButton->setChecked(true);
            }
        }
    }
}

void UserDrawConfigXYPlotElementDialog::on_SetYPushButton_clicked()
{
    QModelIndex Index = ui->SignalsListView->selectionModel()->currentIndex();
    if (Index.isValid()) {
        QString Name = Index.data(Qt::DisplayRole).toString();
        ui->YSignalLineEdit->setText(Name);
        int Vid = get_bbvarivid_by_name(QStringToConstChar(Name));
        if (Vid > 0) {
            double Min = get_bbvari_min(Vid);
            ui->YMinLineEdit->setText(QString("%1").arg(Min));
            double Max = get_bbvari_max(Vid);
            ui->YMaxLineEdit->setText(QString("%1").arg(Max));
            int IntColor = get_bbvari_color(Vid);
            QColor Color = QColor ((IntColor >> 0) & 0xFF,
                                   (IntColor >> 8) & 0xFF,
                                   (IntColor >> 16) & 0xFF);
            char Help[64];
            sprintf (Help , "0x%02X:0x%02X:0x%02X", Color.red(), Color.green(), Color.blue());
            ui->ColorLineEdit->setText(Help);
            SetColorView (Color);
            if (get_bbvari_conversiontype(Vid) == 1) {
                ui->YPhysicalRadioButton->setChecked(true);
            } else {
                ui->YRawRadioButton->setChecked(true);
            }
        }
    }

}

void UserDrawConfigXYPlotElementDialog::on_ColorPushButton_clicked()
{
    int IntColor = ColorStringToInt (ui->ColorLineEdit->text(), nullptr);
    QColor Color = QColor ((IntColor >> 0) & 0xFF,
                           (IntColor >> 8) & 0xFF,
                           (IntColor >> 16) & 0xFF);
    QColorDialog  Dlg;
    Dlg.setCurrentColor(Color);
    if (Dlg.exec() == QDialog::Accepted) {
        Color = Dlg.currentColor();
        char Help[64];
        sprintf (Help , "0x%02X:0x%02X:0x%02X", Color.red(), Color.green(), Color.blue());
        ui->ColorLineEdit->setText(Help);
        SetColorView (Color);
    }
}

void UserDrawConfigXYPlotElementDialog::on_ColorLineEdit_editingFinished()
{
    int IntColor = ColorStringToInt (ui->ColorLineEdit->text(), nullptr);
    QColor Color = QColor ((IntColor >> 0) & 0xFF,
                           (IntColor >> 8) & 0xFF,
                           (IntColor >> 16) & 0xFF);
    SetColorView (Color);
}
