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


#include "BlackboardInfoDialog.h"
#include "ui_BlackboardInfoDialog.h"

#include <QStandardItemModel>
#include <QColorDialog>

#include "EditTextReplaceDialog.h"
#include "MainWindow.h"
#include "StringHelpers.h"

extern "C" {
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "MyMemory.h"
}


BlackboardInfoDialog::BlackboardInfoDialog(QWidget *parent) : Dialog(parent),
    ui(new Ui::BlackboardInfoDialog)
{
    m_BufferSize = 1024;
    m_Buffer = static_cast<char*>(my_malloc (static_cast<size_t>(m_BufferSize)));
    ui->setupUi(this);
    ui->ConversionLineEdit->setMaxLength(BBVARI_CONVERSION_SIZE);
    ui->ConversionTypeComboBox->addItem ("none");  // BB_CONV_NONE
    ui->ConversionTypeComboBox->addItem ("formula");  // BB_CONV_FORMULA
    ui->ConversionTypeComboBox->addItem ("text replace");  // BB_CONV_TEXTREP
    ui->ConversionTypeComboBox->addItem ("factor offset");  // BB_CONV_FACTOFF
    ui->ConversionTypeComboBox->addItem ("reference");  // BB_CONV_REF
    // factor offset und reference are not implemented
    qobject_cast<QStandardItemModel*>(ui->ConversionTypeComboBox->model())->item(3)->setEnabled (false);
    qobject_cast<QStandardItemModel*>(ui->ConversionTypeComboBox->model())->item(4)->setEnabled (false);

    connect(ui->VariableListView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(handleSelectionChanged(QItemSelection,QItemSelection)));


    ui->ColorLineEdit->setValidator(new ColorTextValidator (ui->ColorLineEdit));
    // select first element
    ui->VariableListView->selectionModel()->select(ui->VariableListView->model()->index(0, 0, QModelIndex()), QItemSelectionModel::Select);
}

BlackboardInfoDialog::~BlackboardInfoDialog()
{
    delete ui;
    my_free (m_Buffer);
}

void BlackboardInfoDialog::editCurrentVariable(QString arg_variableName)
{
    BlackboardVariableModel *Model = MainWindow::GetBlackboardVariableModel();
    ui->VariableListView->SetFilter(arg_variableName);
    QModelIndex loc_index = Model->indexOfVariable(arg_variableName);
    loc_index = ((QSortFilterProxyModel*)ui->VariableListView->model())->mapFromSource(loc_index);
    ui->VariableListView->selectionModel()->setCurrentIndex(loc_index, QItemSelectionModel::ClearAndSelect);
    ui->VariableListView->scrollTo(loc_index, QAbstractItemView::PositionAtTop);
}

void BlackboardInfoDialog::NewVariableSelected (QString VariableName)
{
    // Load if not loaded
    if (!m_Data.contains (VariableName)) {
        DialogData New;
        // Process ID
        if ((New.Vid = get_bbvarivid_by_name (QStringToConstChar(VariableName))) > 0) {
            int Ret;
            BB_VARIABLE BaseInfos;
            BB_VARIABLE_ADDITIONAL_INFOS AdditionalInfos;
            do {
                Ret = get_bbvari_infos (New.Vid, &BaseInfos, &AdditionalInfos, m_Buffer, m_BufferSize);
                // Buffer too small
                if (Ret > m_BufferSize) {
                    m_BufferSize = Ret;
                    m_Buffer = static_cast<char*>(my_realloc (m_Buffer, static_cast<size_t>(m_BufferSize)));
                }
            } while (Ret > 0);
            New.DataType = BaseInfos.Type;
            New.AttachCounter = AdditionalInfos.AttachCount;
            New.Value = read_bbvari_convert_double (New.Vid);
            New.Unit = CharToQString(reinterpret_cast<char*>(AdditionalInfos.Unit));
            New.MinValue = AdditionalInfos.Min;
            New. MaxValue = AdditionalInfos.Max;
            New.ConversionType = AdditionalInfos.Conversion.Type;
            switch (AdditionalInfos.Conversion.Type) {
            case BB_CONV_FORMULA:
                New.ConversionString = QString (AdditionalInfos.Conversion.Conv.Formula.FormulaString);
                break;
            case BB_CONV_TEXTREP:
                New.ConversionString = QString (AdditionalInfos.Conversion.Conv.TextReplace.EnumString);
                break;
            case BB_CONV_REF:
                New.ConversionString = QString (AdditionalInfos.Conversion.Conv.Reference.Name);
                break;
            default:
                New.ConversionString = QString ("");
                break;
            }
            if (AdditionalInfos.RgbColor >= 0) {
                New.Color = QColor ((AdditionalInfos.RgbColor >> 0) & 0xFF,    // R
                                    (AdditionalInfos.RgbColor >> 8) & 0xFF,    // G
                                    (AdditionalInfos.RgbColor >> 16) & 0xFF);  // B
                New.ColorValid = true;
            } else {
                New.ColorValid = false;
            }
            New.Width = AdditionalInfos.Width;
            New.Precision = AdditionalInfos.Prec;

            m_Data.insert (VariableName, New);
            m_DataSave.insert (VariableName, New);
        }
    }

    // Insert selected variables
    if (m_Data.contains (VariableName)) {
        DialogData &P = m_Data[VariableName];
        ui->ValueLineEdit->SetValue(P.Value);
        ui->UnitLineEdit->setText(P.Unit);
        ui->MinValueLineEdit->SetValue(P.MinValue);
        ui->MaxValueLineEdit->SetValue(P.MaxValue);
        ui->ConversionTypeComboBox->setCurrentIndex(P.ConversionType);
        ui->ConversionLineEdit->setText(P.ConversionString);
        ui->ColorValidCheckBox->setChecked(P.ColorValid);
        ui->ColorViewLabel->setVisible(P.ColorValid);
        ui->ColorLineEdit->setVisible(P.ColorValid);
        char Help[64];
        if (P.ColorValid) {
            sprintf (Help , "0x%02X:0x%02X:0x%02X", P.Color.red(), P.Color.green(), P.Color.blue());
        } else {
            sprintf (Help, "not set");
        }
        ui->ColorLineEdit->setText(Help);
        SetColorView (P.Color);
        ui->WidthLineEdit->setText (QString ("%1").arg (P.Width));
        ui->PrecisionLineEdit->setText (QString ("%1").arg (P.Precision));

        sprintf (Help , "0x%X", P.Vid);
        ui->VariableIdLineEdit->setText (Help);

        ui->DataTypeLineEdit->setText(GetDataTypeName (P.DataType));
        ui->AttachCounterLineEdit->setText (QString ("%1").arg (P.AttachCounter));
    }
}


void BlackboardInfoDialog::StoreVariableSelected (QString VariableName)
{
    if (m_Data.contains (VariableName)) {
        DialogData &P = m_Data[VariableName];
        P.Value = ui->ValueLineEdit->GetDoubleValue();
        P.Unit = ui->UnitLineEdit->text();
        P.MinValue = ui->MinValueLineEdit->GetDoubleValue();
        P.MaxValue = ui->MaxValueLineEdit->GetDoubleValue();
        P.ConversionType = ui->ConversionTypeComboBox->currentIndex();
        P.ConversionString = ui->ConversionLineEdit->text();
        P.ColorValid = ui->ColorValidCheckBox->isChecked();
        if (P.ColorValid) {
            QString Help = ui->ColorLineEdit->text();
            QValidator::State ValidatorState;
            int Color = ColorStringToInt (ui->ColorLineEdit->text(), &ValidatorState);
            P.Color = QColor ((Color >> 0) & 0xFF,
                             (Color >> 8) & 0xFF,
                             (Color >> 16) & 0xFF);
            if (ValidatorState != QValidator::Acceptable) {
                P.ColorValid = false;
            }
        }
        P.Width = ui->WidthLineEdit->text().toInt();
        P.Precision = ui->PrecisionLineEdit->text().toInt();
    }
}

static uint64_t CompareDoubleEqual(double a, double b)
{
    double diff = a - b;
    if ((diff <= 0.0) && (diff >= 0.0)) return 1;
    else return 0;
}

void BlackboardInfoDialog::accept()
{
    QModelIndexList SelVari = ui->VariableListView->selectionModel()->selectedIndexes();
    if (SelVari.size()) {
        StoreVariableSelected (SelVari.at(0).data(Qt::DisplayRole).toString());
    }
    QList<QString> Keys = m_Data.keys();
    foreach (QString Item, Keys) {
        DialogData &Data = m_Data[Item];
        DialogData &DataSave = m_DataSave[Item];
        if (!CompareDoubleEqual(Data.Value,DataSave.Value)) {
            attach_bbvari (Data.Vid);
            write_bbvari_minmax_check (Data.Vid, Data.Value);
            remove_bbvari (Data.Vid);
        }
        if (Data.Unit.compare(DataSave.Unit)) {
            if (!Data.Unit.isNull()) {
                set_bbvari_unit (Data.Vid, QStringToConstChar(Data.Unit));
            }
        }
        if (!CompareDoubleEqual(Data.MinValue, DataSave.MinValue)) {
            set_bbvari_min (Data.Vid, Data.MinValue);
        }
        if (!CompareDoubleEqual(Data.MaxValue, DataSave.MaxValue)) {
            set_bbvari_max (Data.Vid, Data.MaxValue);
        }
        if ((Data.ConversionType != DataSave.ConversionType) ||
            Data.ConversionString.compare (DataSave.ConversionString)) {
            if (!Data.ConversionString.isNull()) {
                set_bbvari_conversion (Data.Vid, Data.ConversionType, QStringToConstChar(Data.ConversionString));
            }
        }
        if ((Data.Color != DataSave.Color) ||
            (Data.ColorValid != DataSave.ColorValid)) {
            int Color;
            if (Data.ColorValid) {
                Color = Data.Color.red() | (Data.Color.green() << 8) | (Data.Color.blue() << 16);
            } else {
                Color = -1;
            }
            set_bbvari_color(Data.Vid, Color);
        }
        if ((Data.Precision != DataSave.Precision) ||
            (Data.Width != DataSave.Precision)) {
            set_bbvari_format(Data.Vid, Data.Width, Data.Precision);
        }
    }
    QDialog::accept();
}

void BlackboardInfoDialog::on_ConversionEditPushButton_clicked()
{
    QString TextReplace = ui->ConversionLineEdit->text();
    EditTextReplaceDialog Dlg (TextReplace);
    if (Dlg.exec() == QDialog::Accepted) {
        TextReplace = Dlg.GetModifiedTextReplaceString();
        ui->ConversionLineEdit->setText(TextReplace);
    }
}

void BlackboardInfoDialog::on_ChoiceColorPushButton_clicked()
{
    int IntColor = ColorStringToInt (ui->ColorLineEdit->text(), nullptr);
    QColor Color = QColor ((IntColor >> 0) & 0xFF,
                           (IntColor >> 8) & 0xFF,
                           (IntColor >> 16) & 0xFF);
    QColorDialog  Dlg;
    Dlg.setCurrentColor(Color);
    if (Dlg.exec() == QDialog::Accepted) {
        Color = Dlg.currentColor();
        ui->ColorValidCheckBox->setChecked(true);
        char Help[64];
        sprintf (Help , "0x%02X:0x%02X:0x%02X", Color.red(), Color.green(), Color.blue());
        ui->ColorLineEdit->setText(Help);
        SetColorView (Color);
    }
}

void BlackboardInfoDialog::SetColorView (QColor color)
{
    ui->ColorViewLabel->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));
}

void BlackboardInfoDialog::on_ColorLineEdit_editingFinished()
{
    int IntColor = ColorStringToInt (ui->ColorLineEdit->text(), nullptr);
    QColor Color = QColor ((IntColor >> 0) & 0xFF,
                           (IntColor >> 8) & 0xFF,
                           (IntColor >> 16) & 0xFF);
    SetColorView (Color);
}

void BlackboardInfoDialog::on_ConversionTypeComboBox_currentTextChanged(const QString &ConversionType)
{
    ui->ConversionEditPushButton->setDisabled (ConversionType.compare (QString ("text replace")));
}

void BlackboardInfoDialog::handleSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    if (!deselected.isEmpty()) {
        QString VariableName = deselected.indexes().at(0).data(Qt::DisplayRole).toString();
        StoreVariableSelected (VariableName);
    }
    if (!selected.isEmpty()) {
        QString VariableName = selected.indexes().at(0).data(Qt::DisplayRole).toString();
        NewVariableSelected (VariableName);
    }
}

void BlackboardInfoDialog::on_ColorValidCheckBox_stateChanged(int arg1)
{
    ui->ColorViewLabel->setVisible(arg1);
    ui->ColorLineEdit->setVisible(arg1);
}

