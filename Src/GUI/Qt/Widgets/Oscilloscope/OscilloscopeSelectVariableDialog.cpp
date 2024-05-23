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


#include "OscilloscopeSelectVariableDialog.h"
#include "ui_OscilloscopeSelectVariableDialog.h"
#include "StringHelpers.h"

#include <QColorDialog>

extern "C" {
#include "blackboard.h"
#include "error2.h"
#include "memory2.h"
#include "OscilloscopeCyclic.h"
}


OscilloscopeSelectVariableDialog::OscilloscopeSelectVariableDialog(OSCILLOSCOPE_DATA *par_Data, QWidget *parent) :
    Dialog(parent),
    ui(new Ui::OscilloscopeSelectVariableDialog)
{
    long vid;
    char *variname;
    unsigned long RgbColor;

    m_Data = par_Data;
    m_IncExcFilter = m_Data->IncExcFilter;
    ui->setupUi(this);
    ui->FilterGroupBox->SetFilter(m_IncExcFilter);
    ui->LineWidthSpinBox->setMaximum(10);
    ui->LineWidthSpinBox->setMinimum(1);

    FillVarianleList();

    if (m_Data->sel_left_right) {
        vid = m_Data->vids_right[m_Data->sel_pos_right];
        variname = m_Data->name_right[m_Data->sel_pos_right];
        m_LineWidth = m_Data->LineSize_right[m_Data->sel_pos_right];
        m_Min = m_Data->min_right[m_Data->sel_pos_right];
        m_Max = m_Data->max_right[m_Data->sel_pos_right];
        m_DecPhysFlag = m_Data->dec_phys_right[m_Data->sel_pos_right];
        m_DispayDecHexBin = m_Data->dec_hex_bin_right[m_Data->sel_pos_right];
        RgbColor = m_Data->color_right[m_Data->sel_pos_right];
        m_TextAlignment = m_Data->txt_align_right[m_Data->sel_pos_right];
        m_Disable = m_Data->vars_disable_right[m_Data->sel_pos_right];
    } else {
        vid = m_Data->vids_left[m_Data->sel_pos_left];
        variname = m_Data->name_left[m_Data->sel_pos_left];
        m_LineWidth = m_Data->LineSize_left[m_Data->sel_pos_left];
        m_Min = m_Data->min_left[m_Data->sel_pos_left];
        m_Max = m_Data->max_left[m_Data->sel_pos_left];
        m_DecPhysFlag = m_Data->dec_phys_left[m_Data->sel_pos_left];
        m_DispayDecHexBin = m_Data->dec_hex_bin_left[m_Data->sel_pos_left];
        RgbColor = m_Data->color_left[m_Data->sel_pos_left];
        m_TextAlignment = m_Data->txt_align_left[m_Data->sel_pos_left];
        m_Disable = m_Data->vars_disable_left[m_Data->sel_pos_left];
    }
    if (m_LineWidth < 1) m_LineWidth = 1;
    if (m_LineWidth > 10) m_LineWidth = 10;
    ui->LineWidthSpinBox->setValue(m_LineWidth);
    m_Color = QColor ((RgbColor >> 0) & 0xFF, (RgbColor >> 8) & 0xFF, (RgbColor >> 16) & 0xFF);
    ui->LineWidthWidget->SetLineWidthAndColor (m_LineWidth, m_Color);
    SetColorView (m_Color);

    if (vid > 0) {
        QList<QListWidgetItem*>Items = ui->VariableListWidget->findItems(QString (variname), Qt::MatchFixedString);
        if (Items.count() >= 1) {
            Items.at(0)->setSelected(true);
        } else {
            // Wenn ausgewaehlte Variable nicht in der Liste ist dann eintragen
            QListWidgetItem *NewItem = new  QListWidgetItem (QString (variname));
            ui->VariableListWidget->addItem (NewItem);
            NewItem->setSelected(true);
        }
        ui->MaxLineEdit->SetValue (m_Max);
        ui->MinLineEdit->SetValue (m_Min);
        if (m_DecPhysFlag)
            ui->PhysicallyRadioButton->setChecked(true);
        else switch (m_DispayDecHexBin) {
        case 0:
        default:  // dec
            ui->DecimalRadioButton->setChecked(true);
            break;
        case 1:  // hex
            ui->HexadecimalRadioButton->setChecked(true);
            break;
        case 2:  // bin
            ui->BinaryRadioButton->setChecked(true);
            break;
        }
        ui->NotVisableCheckBox->setChecked(m_Disable != 0);
        ui->LabelRightAlignedCheckBox->setChecked(m_TextAlignment != 0);
    }

}

OscilloscopeSelectVariableDialog::~OscilloscopeSelectVariableDialog()
{
    delete ui;
}

void OscilloscopeSelectVariableDialog::FillVarianleList()
{
    ui->VariableListWidget->clear();
    char *SelVarName;
    if (m_Data->sel_left_right) {
        SelVarName = m_Data->name_right[m_Data->sel_pos_right];
    } else {
        SelVarName = m_Data->name_left[m_Data->sel_pos_left];
    }
    int index = 0;
    char Name[BBVARI_NAME_SIZE];
    bool SelVarNaleInList = false;
    while((index = read_next_blackboard_vari (index, Name, sizeof(Name))) >= 0)  {
        if ((m_IncExcFilter == nullptr) || CheckIncludeExcludeFilter (Name, m_IncExcFilter)) {
            if (SelVarName != nullptr) {
                if (!strcmp (SelVarName, Name)) {
                    SelVarNaleInList = true;   // ausgewaehlte Variable ist in Liste vorhanden
                }
            }
            ui->VariableListWidget->addItem (QString (Name));
        }
    }
    if (!SelVarNaleInList && (SelVarName != nullptr)) {
        QListWidgetItem *NewItem = new  QListWidgetItem (QString (SelVarName));
        ui->VariableListWidget->addItem (NewItem);
        NewItem->setSelected(true);
    }
}

void OscilloscopeSelectVariableDialog::SetColorView (QColor color)
{
    ui->ColorLabel->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));
}


void OscilloscopeSelectVariableDialog::accept()
{
    unsigned long vid;
    char *variname;

    if (m_Data->sel_left_right) {
        vid = m_Data->vids_right[m_Data->sel_pos_right];
        variname = m_Data->name_right[m_Data->sel_pos_right];
    } else {
        vid = m_Data->vids_left[m_Data->sel_pos_left];
        variname = m_Data->name_left[m_Data->sel_pos_left];
    }

    QModelIndex Index = ui->VariableListWidget->currentIndex();
    if (Index.isValid()) {
        QString Name = Index.data().toString();
        if (vid > 0) {
            if (variname != nullptr) {
                if (Name.compare(QString(variname))) {
                    remove_bbvari_unknown_wait (vid);   // alte Variable erst mal loeschen
                }
            }
        }
        // neu Variable anmelden
        vid = add_bbvari(QStringToConstChar(Name), BB_UNKNOWN_WAIT, nullptr);  // und Neue anmelden
        if (vid <= 0) {
            error (1, "cannot attach variable \"%s\"", QStringToConstChar(Name));
            return;
        }
        if (m_Data->sel_left_right) {
            EnterOsziCycleCS ();                    // 1.  Reihenfoge ist wichtig
            EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber); // 2.
            m_Data->vids_right[m_Data->sel_pos_right] = vid;
            m_Data->name_right[m_Data->sel_pos_right] = ReallocCopyString(m_Data->name_right[m_Data->sel_pos_right], Name);
            if (m_Data->buffer_right[m_Data->sel_pos_right] != nullptr) my_free (m_Data->buffer_right[m_Data->sel_pos_right]);
            if ((m_Data->buffer_right[m_Data->sel_pos_right] = (double*)my_calloc (m_Data->buffer_depth, sizeof(double))) != nullptr) {
                ;//attach_bbvari (vid);
            } else error (1, "out of memory");
            LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber); // 2.
            LeaveOsziCycleCS ();
        } else {
            EnterOsziCycleCS ();                     // 1.  Reihenfoge ist wichtig
            EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber); // 2.
            m_Data->vids_left[m_Data->sel_pos_left] = vid;
            m_Data->name_left[m_Data->sel_pos_left] = v(m_Data->name_left[m_Data->sel_pos_left], Name);
            if (m_Data->buffer_left[m_Data->sel_pos_left] != nullptr) my_free (m_Data->buffer_left[m_Data->sel_pos_left]);
            if ((m_Data->buffer_left[m_Data->sel_pos_left] = (double*)my_calloc (m_Data->buffer_depth, sizeof(double))) != nullptr) {
                ;//attach_bbvari (vid);
            } else error (1, "out of memory");
            LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber); // 2.
            LeaveOsziCycleCS ();
        }
    }

    m_Max = ui->MaxLineEdit->GetDoubleValue();
    m_Min = ui->MinLineEdit->GetDoubleValue();
    if (m_Min >= m_Max) m_Max = m_Min + 1.0;
    if (m_Data->sel_left_right) {
        m_Data->min_right[m_Data->sel_pos_right] = m_Min;
        m_Data->max_right[m_Data->sel_pos_right] = m_Max;
        m_Data->color_right[m_Data->sel_pos_right] = m_Color.red() + (m_Color.green() << 8) + (m_Color.blue() << 16);
        m_Data->LineSize_right[m_Data->sel_pos_right] = m_LineWidth;
    } else {
        m_Data->min_left[m_Data->sel_pos_left] = m_Min;
        m_Data->max_left[m_Data->sel_pos_left] = m_Max;
        m_Data->color_left[m_Data->sel_pos_left] = m_Color.red() + (m_Color.green() << 8) + (m_Color.blue() << 16);
        m_Data->LineSize_left[m_Data->sel_pos_left] = m_LineWidth;
    }

    if (ui->PhysicallyRadioButton->isChecked()) {  // phys ?
        if (m_Data->sel_left_right) {
            m_Data->dec_phys_right[m_Data->sel_pos_right] = 1;
            m_Data->dec_hex_bin_right[m_Data->sel_pos_right] = 0;
        } else {
            m_Data->dec_phys_left[m_Data->sel_pos_left] = 1;
            m_Data->dec_hex_bin_left[m_Data->sel_pos_left] = 0;
        }
    } else if (ui->HexadecimalRadioButton->isChecked()) { // hex ?
        if (m_Data->sel_left_right) {
            m_Data->dec_phys_right[m_Data->sel_pos_right] = 0;
            m_Data->dec_hex_bin_right[m_Data->sel_pos_right] = 1;
        } else {
            m_Data->dec_phys_left[m_Data->sel_pos_left] = 0;
            m_Data->dec_hex_bin_left[m_Data->sel_pos_left] = 1;
        }
    } else if (ui->BinaryRadioButton->isChecked()) { // bin ?
        if (m_Data->sel_left_right) {
            m_Data->dec_phys_right[m_Data->sel_pos_right] = 0;
            m_Data->dec_hex_bin_right[m_Data->sel_pos_right] = 2;
        } else {
            m_Data->dec_phys_left[m_Data->sel_pos_left] = 0;
            m_Data->dec_hex_bin_left[m_Data->sel_pos_left] = 2;
        }
    } else { // dec ?
        if (m_Data->sel_left_right) {
            m_Data->dec_phys_right[m_Data->sel_pos_right] = 0;
            m_Data->dec_hex_bin_right[m_Data->sel_pos_right] = 0;
        } else {
            m_Data->dec_phys_left[m_Data->sel_pos_left] = 0;
            m_Data->dec_hex_bin_left[m_Data->sel_pos_left] = 0;
        }
    }

    if (ui->LabelRightAlignedCheckBox->isChecked()) {
        if (m_Data->sel_left_right)
            m_Data->txt_align_right[m_Data->sel_pos_right] = 1;
        else
            m_Data->txt_align_left[m_Data->sel_pos_left] = 1;
    } else {
        if (m_Data->sel_left_right)
            m_Data->txt_align_right[m_Data->sel_pos_right] = 0;
        else
            m_Data->txt_align_left[m_Data->sel_pos_left] = 0;
    }
    if (ui->NotVisableCheckBox->isChecked()) {
        if (m_Data->sel_left_right)
            m_Data->vars_disable_right[m_Data->sel_pos_right] = 1;
        else
            m_Data->vars_disable_left[m_Data->sel_pos_left] = 1;
    } else {
        if (m_Data->sel_left_right)
            m_Data->vars_disable_right[m_Data->sel_pos_right] = 0;
        else
            m_Data->vars_disable_left[m_Data->sel_pos_left] = 0;
    }

    m_Data->IncExcFilter = m_IncExcFilter;
    if (update_vid_owc (m_Data)) {
        error (1, "cannot update variable list");
    }
    QDialog::accept();
}

void OscilloscopeSelectVariableDialog::on_VariableListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (current != nullptr) {
        QString SelectedVariable = current->data(Qt::DisplayRole).toString();
        int vid = get_bbvarivid_by_name(QStringToConstChar(SelectedVariable));
        if (vid > 0) {
            m_Max = get_bbvari_max (vid);
            m_Min = get_bbvari_min (vid);
            ui->MaxLineEdit->SetValue (m_Max);
            ui->MinLineEdit->SetValue (m_Min);
            m_DecPhysFlag = get_bbvari_conversiontype (vid) != 0; //BB_NOTHING;
            if (m_DecPhysFlag) ui->PhysicallyRadioButton->setChecked(true);
            else ui->DecimalRadioButton->setChecked(true);
            unsigned long RgbColor = get_bbvari_color (vid);
            m_Color = QColor ((RgbColor >> 0) & 0xFF, (RgbColor >> 8) & 0xFF, (RgbColor >> 16) & 0xFF);
            SetColorView (m_Color);
        }
    }
}

void OscilloscopeSelectVariableDialog::on_tabWidget_currentChanged(int index)
{
    if (index == 0) {
        m_IncExcFilter = ui->FilterGroupBox->GetFilter();
        FillVarianleList();
    }
}

void OscilloscopeSelectVariableDialog::on_LineWidthSpinBox_valueChanged(int arg1)
{
    m_LineWidth = arg1;
    ui->LineWidthWidget->SetLineWidthAndColor (m_LineWidth, m_Color);
}

void OscilloscopeSelectVariableDialog::on_ColorPushButton_clicked()
{
    QColorDialog ColorDialog;
    ColorDialog.setCurrentColor(m_Color);
    if(ColorDialog.exec() == QDialog::Accepted) {
        m_Color = ColorDialog.currentColor();
        SetColorView (m_Color);
        ui->LineWidthWidget->SetLineWidthAndColor (m_LineWidth, m_Color);
    }
}

void OscilloscopeSelectVariableDialog::on_DeletePushButton_clicked()
{
    if (m_Data->sel_left_right) {
        if (m_Data->vids_right[m_Data->sel_pos_right] > 0) {
            remove_bbvari_unknown_wait (m_Data->vids_right[m_Data->sel_pos_right]);
        }
        m_Data->vids_right[m_Data->sel_pos_right] = 0;
        if (m_Data->buffer_right[m_Data->sel_pos_right] != nullptr) my_free (m_Data->buffer_right[m_Data->sel_pos_right]);
        m_Data->buffer_right[m_Data->sel_pos_right] = nullptr;
        m_Data->LineSize_right[m_Data->sel_pos_right] = 1;
        m_Data->color_right[m_Data->sel_pos_right] = 0;
    } else {
        if (m_Data->vids_left[m_Data->sel_pos_left] > 0) {
            remove_bbvari_unknown_wait (m_Data->vids_left[m_Data->sel_pos_left]);
        }
        m_Data->vids_left[m_Data->sel_pos_left] = 0;
        if (m_Data->buffer_left[m_Data->sel_pos_left] != nullptr) my_free (m_Data->buffer_left[m_Data->sel_pos_left]);
        m_Data->buffer_left[m_Data->sel_pos_left] = nullptr;
        m_Data->LineSize_left[m_Data->sel_pos_left] = 1;
        m_Data->color_left[m_Data->sel_pos_left] = 0;
    }
    if (update_vid_owc (m_Data)) {
        error (1, "cannot update variable list");
    }
    QDialog::accept();
}
