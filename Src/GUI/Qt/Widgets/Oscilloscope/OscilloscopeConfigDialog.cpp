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


#include "OscilloscopeConfigDialog.h"
#include "ui_OscilloscopeConfigDialog.h"

#include "WindowNameAlreadyInUse.h"
#include "QtIniFile.h"

#include "OscilloscopeCyclic.h"

#include "FileDialog.h"
#include "StringHelpers.h"

#include "MainWindow.h"

extern "C" {
#include "Config.h"
#include "Blackboard.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "Scheduler.h"
#include "my_udiv128.h"

}

OscilloscopeConfigDialog::OscilloscopeConfigDialog(OSCILLOSCOPE_DATA *par_Data, QString par_WindowTitle,
                                                   QWidget *parent) : Dialog(parent),
    ui(new Ui::OscilloscopeConfigDialog)
{
    m_OnOffDescrFrameChanged = false;
    m_WindowTitleChanged = false;
    m_Data = par_Data;
    m_Model = nullptr;
    m_SortModel = nullptr;
    ui->setupUi(this);
    m_WindowTitleSave = par_WindowTitle;
    ui->WindowTitleLineEdit->setText(par_WindowTitle);
    ui->BufferDepthLineEdit->setText(QString("%1").arg(m_Data->buffer_depth));
    ui->DisplayLeftAxisCheckBox->setChecked(par_Data->left_axis_on);
    ui->DisplayRightAxisCheckBox->setChecked(par_Data->right_axis_on);
    ui->Stepping10RadioButton->setChecked(m_Data->window_step_width == 10);
    ui->Stepping20RadioButton->setChecked(m_Data->window_step_width == 20);
    ui->Stepping50RadioButton->setChecked(m_Data->window_step_width == 50);
    ui->Stepping100RadioButton->setChecked(m_Data->window_step_width == 100);

    // Trigger
    ui->SingleShotCheckBox->setChecked(m_Data->trigger_flag >= 2);
    ui->PreTriggerSamplesLineEdit->SetValue(m_Data->pre_trigger);
    ui->TriggerEventComboBox->addItem(QString (">"));
    ui->TriggerEventComboBox->addItem(QString ("<"));

    if (m_Data->trigger_flank == '>')
        ui->TriggerEventComboBox->setCurrentText(QString(">"));
    else if (m_Data->trigger_flank == '<')
        ui->TriggerEventComboBox->setCurrentText(QString("<"));

    EnableDisableTrigger((m_Data->trigger_vid > 0), true);

    ui->TriggerValueLineEdit->SetValue(m_Data->trigger_value);

    m_WindowStartTimeSave = static_cast<double>(m_Data->t_window_start) / TIMERCLKFRQ;
    m_WindowTimeSizeSave = static_cast<double>(m_Data->t_window_size) / TIMERCLKFRQ;
    m_WindowEndTimeSave = static_cast<double>(m_Data->t_window_end) / TIMERCLKFRQ;
    m_WindowSizeTimeConfigeredSave = m_Data->t_window_size_configered;

    ui->TimeAxisBaseLineEdit->SetValue(m_WindowStartTimeSave);
    ui->TimeAxisWidthLineEdit->SetValue(m_WindowSizeTimeConfigeredSave);
    ui->TimeAxisEndLineEdit->SetValue(m_WindowEndTimeSave);

    if (m_Data->state) {
        ui->TimeAxisBaseLineEdit->setEnabled(false);
        ui->TimeAxisEndLineEdit->setEnabled(false);
    }
    ui->BackgroundImageLineEdit->setText(QString(m_Data->BackgroundImage));
}

OscilloscopeConfigDialog::~OscilloscopeConfigDialog()
{
    delete ui;
    if (m_SortModel != nullptr) delete m_SortModel;
}

bool OscilloscopeConfigDialog::OnOffDescrFrameChanged()
{
    return m_OnOffDescrFrameChanged;
}

bool OscilloscopeConfigDialog::WindowTitleChanged()
{
    return m_WindowTitleChanged;
}

QString OscilloscopeConfigDialog::GetWindowTitle()
{
    return ui->WindowTitleLineEdit->text();
}


void OscilloscopeConfigDialog::accept()
{
    bool WindowTitleOk = false;
    QString NewWindowTitle = ui->WindowTitleLineEdit->text().trimmed();
    QString OldWindowTitle = m_WindowTitleSave;

    if (NewWindowTitle.compare (OldWindowTitle)) {
        if (NewWindowTitle.length() >= 1) {
            if(!WindowNameAlreadyInUse(NewWindowTitle)) {
                if (IsAValidWindowSectionName(NewWindowTitle)) {
                    WindowTitleOk = true;
                    m_WindowTitleChanged = true;
                    QDialog::accept();
                } else {
                    WindowTitleOk = false;
                    ThrowError (1, "Inside window name only ascii characters and no []\\ are allowed \"%s\"",
                               QStringToConstChar(NewWindowTitle));
                }
            } else {
                WindowTitleOk = false;
                ThrowError (1, "window name \"%s\" already in use", QStringToConstChar(NewWindowTitle));
            }
        } else {
            WindowTitleOk = false;
            ThrowError (1, "window name must have one or more character \"%s\"", QStringToConstChar(NewWindowTitle));
        }
    } else {
        WindowTitleOk = true;
        QDialog::accept();
    }

    if (WindowTitleOk) {
        if (ui->TriggerCheckBox->checkState() == Qt::Checked) {
            QString TriggerVariable = ui->TriggerVariableComboBox->currentText();
            m_Data->trigger_state = 0;
            if (ui->SingleShotCheckBox->checkState() == Qt::Checked) m_Data->trigger_flag = 2;
            else m_Data->trigger_flag = 1;
            m_Data->pre_trigger = ui->PreTriggerSamplesLineEdit->GetIntValue();
            QString TriggerEvent = ui->TriggerEventComboBox->currentText();
            if (!TriggerEvent.compare(QString("<"))) {
                m_Data->trigger_flank = '<';
            } else {
                m_Data->trigger_flank = '>';
            }
            if (m_Data->pre_trigger < 0) m_Data->pre_trigger = 0;

            if (m_Data->trigger_vid != get_bbvarivid_by_name(QStringToConstChar(TriggerVariable))) {
                if (m_Data->trigger_vid > 0) remove_bbvari_unknown_wait(m_Data->trigger_vid);
                if ((m_Data->trigger_vid = add_bbvari(QStringToConstChar(TriggerVariable), BB_UNKNOWN_WAIT, nullptr)) > 0) {
                    m_Data->vids_left[20] = m_Data->trigger_vid;

                    m_Data->name_left[20] = ReallocCopyString(m_Data->name_left[20], TriggerVariable);
                    m_Data->name_trigger = ReallocCopyString(m_Data->name_trigger, TriggerVariable);
                    if (m_Data->name_trigger != nullptr) strcpy (m_Data->name_trigger, QStringToConstChar(TriggerVariable));

                    if (m_Data->buffer_left[20] == nullptr) {
                        EnterOsziCycleCS ();
                        if ((m_Data->buffer_left[20] = static_cast<double*>(my_calloc(static_cast<size_t>(m_Data->buffer_depth), sizeof (double)))) == nullptr) {
                            remove_bbvari_unknown_wait (m_Data->vids_left[20]);
                            m_Data->vids_left[20] = 0;
                            m_Data->trigger_vid = 0;
                            m_Data->trigger_flag = 0;
                        } else update_vid_owc (m_Data);  // Send the updated vid list to TraceQueue process
                        LeaveOsziCycleCS ();
                    }
                }
            }
        } else {
            if (m_Data->trigger_vid > 0) remove_bbvari_unknown_wait (m_Data->trigger_vid);
            m_Data->vids_left[20] = 0;
            m_Data->trigger_vid = 0;
            m_Data->trigger_flag = 0;
            if (m_Data->state > 1) m_Data->state = 1;
        }
        m_Data->trigger_value = ui->TriggerValueLineEdit->GetDoubleValue();

        // Axis should be paint?
        if (ui->DisplayLeftAxisCheckBox->checkState() == Qt::Checked) {
            if (!m_Data->left_axis_on) m_OnOffDescrFrameChanged = true;
            m_Data->left_axis_on = 1;
        } else {
            if (m_Data->left_axis_on) m_OnOffDescrFrameChanged = true;
            m_Data->left_axis_on = 0;
        }
        if (ui->DisplayRightAxisCheckBox->checkState() == Qt::Checked) {
            if (!m_Data->right_axis_on) m_OnOffDescrFrameChanged = true;
            m_Data->right_axis_on = 1;
        } else {
            if (m_Data->right_axis_on) m_OnOffDescrFrameChanged = true;
            m_Data->right_axis_on = 0;
        }
        // Time
        int s_hs = 0; // Signals if the start time was changed
        int w_hs = 0; // Signals if the time window was changed
        int e_hs = 0; // Signals if the end time was changed
        double t_s, t_w, t_e;

        t_s = ui->TimeAxisBaseLineEdit->GetDoubleValue();
        s_hs =  QString("%1").arg(t_s).compare (QString("%1").arg(m_WindowStartTimeSave)) ? 1 : 0;

        t_w = ui->TimeAxisWidthLineEdit->GetDoubleValue();
        w_hs = QString("%1").arg(t_w).compare (QString("%1").arg(m_WindowSizeTimeConfigeredSave)) ? 1 : 0;

        t_e = ui->TimeAxisEndLineEdit->GetDoubleValue();
        e_hs = QString("%1").arg(t_e).compare (QString("%1").arg(m_WindowEndTimeSave)) ? 1 : 0;

        switch ((s_hs << 2) | (w_hs << 1) | e_hs) {
        case 0:     // Nothing was changed
            break;
        case 3:     // The end time and the time window has changed
            if (m_Data->state == 0) {
                m_Data->t_window_size = static_cast<uint64_t>(t_w * TIMERCLKFRQ);
                m_Data->t_window_size_configered = t_w;
                m_Data->t_window_end = static_cast<uint64_t>(t_e * TIMERCLKFRQ);
                if (t_e > t_s) {
                    m_Data->t_window_size = static_cast<uint64_t>(t_e - t_s * TIMERCLKFRQ);
                } else {
                    m_Data->t_window_start = static_cast<uint64_t>(t_e - t_w * TIMERCLKFRQ);
                }
            }
            break;
        case 1:     // The end time has changed
            if (m_Data->state == 0) {
                m_Data->t_window_end = static_cast<uint64_t>(t_e * TIMERCLKFRQ);
                if (t_e > t_s) {
                    m_Data->t_window_size = static_cast<uint64_t>(t_e - t_s * TIMERCLKFRQ);
                } else {
                    m_Data->t_window_start = static_cast<uint64_t>(t_e - t_w * TIMERCLKFRQ);
                }
           }
            break;
        case 2:     // The time window has changed
            EnterOsziCycleCS ();                                            // 1.  The order is important
            EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber); // 2.
            m_Data->t_window_size = static_cast<uint64_t>(t_w * TIMERCLKFRQ);
            m_Data->t_window_size_configered = t_w;
            m_Data->t_window_start =  m_Data->t_window_end - m_Data->t_window_size;
            m_Data->t_window_end = m_Data->t_window_start + m_Data->t_window_size;
            if (m_Data->state != 0) {
                if (m_Data->t_window_end < m_Data->t_current_buffer_end) {
                    m_Data->t_window_end = m_Data->t_current_buffer_end;
                    m_Data->t_window_start = m_Data->t_window_end - m_Data->t_window_size;
                }
                if (m_Data->t_window_start > m_Data->t_current_buffer_end) {
                   m_Data->t_window_start = m_Data->t_current_buffer_end;
                   m_Data->t_window_end = m_Data->t_window_start + m_Data->t_window_size;
                }
            }
            LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber); // 2.
            LeaveOsziCycleCS ();                                            // 1.
            break;
        case 6:    // The start time the time window has changed
            if (m_Data->state == 0) {
                m_Data->t_window_size = static_cast<uint64_t>(t_w * TIMERCLKFRQ);
                m_Data->t_window_size_configered = t_w;
                m_Data->t_window_start = static_cast<uint64_t>(t_s * TIMERCLKFRQ);
                if (t_s < t_e) {
                    m_Data->t_window_size = static_cast<uint64_t>((t_e - t_s) * TIMERCLKFRQ);
                } else {
                    m_Data->t_window_end = static_cast<uint64_t>((t_s + t_w) * TIMERCLKFRQ);
                }
            }
            break;
        case 4:     // The start time
            if (m_Data->state == 0) {
                m_Data->t_window_start = static_cast<uint64_t>(t_s * TIMERCLKFRQ);
                if (t_s < t_e) {
                    m_Data->t_window_size = static_cast<uint64_t>((t_e - t_s) * TIMERCLKFRQ);
                } else {
                    m_Data->t_window_end = static_cast<uint64_t>((t_s + t_w) * TIMERCLKFRQ);
                }
            }
            break;
        case 7:    // The start time, the time window, the end time  has changed, Then the time window will be ignored
        case 5:    // The start time and the end time  has changed
            if (m_Data->state == 0) {
                m_Data->t_window_end = static_cast<uint64_t>(t_e * TIMERCLKFRQ);
                m_Data->t_window_start = static_cast<uint64_t>(t_s * TIMERCLKFRQ);
                m_Data->t_window_size = static_cast<uint64_t>((t_e - t_s) * TIMERCLKFRQ);
                m_Data->t_window_size_configered = (t_e - t_s);

            }
            break;
        }
        // Scrolling step width
        if (ui->Stepping10RadioButton->isChecked())
            m_Data->window_step_width = 10;
        else if (ui->Stepping20RadioButton->isChecked())
            m_Data->window_step_width = 20;
        else if (ui->Stepping50RadioButton->isChecked())
            m_Data->window_step_width = 50;
        else if (ui->Stepping100RadioButton->isChecked())
            m_Data->window_step_width = 100;
        else m_Data->window_step_width = 20;
        m_Data->t_step_win = my_umuldiv64(m_Data->t_window_end - m_Data->t_window_start, static_cast<uint64_t>(m_Data->window_step_width), 100ULL);

        // Change the buffer depth
        int k = ui->BufferDepthLineEdit->text().toInt();
        if (k > 1000000L) k = 1000000L;  // 1000000  max. samples
        if (k < 1024L) k = 1024L;
        if (k != m_Data->buffer_depth) {
            int err_flag = 0;
            int i;
            m_Data->buffer_depth = k;
            EnterOsziCycleCS ();
            double *tmp;
            for (i = 0; i < 21; i++) {
                if (m_Data->buffer_left[i] != nullptr) {
                    if ((tmp = static_cast<double*>(my_realloc (m_Data->buffer_left[i], static_cast<size_t>(k) * sizeof (double)))) == nullptr) {
                        err_flag = 1;
                        break;
                    } else {
                        m_Data->buffer_left[i] = tmp;
                    }
                }
                if (m_Data->buffer_right[i] != nullptr) {
                    if ((tmp = static_cast<double*>(my_realloc (m_Data->buffer_right[i], static_cast<size_t>(k) * sizeof (double)))) == nullptr) {
                        err_flag = 1;
                        break;
                    } else {
                        m_Data->buffer_right[i] = tmp;
                    }
                }
            }
            LeaveOsziCycleCS ();
            if (err_flag) {  // Not enough memory for all signals
                ThrowError (1, "out of memory");
                EnterOsziCycleCS ();
                for (i--; i >= 0; i--) {
                    if (m_Data->buffer_left[i] != nullptr) {
                        if ((tmp = static_cast<double*>(my_realloc (m_Data->buffer_left[i], static_cast<size_t>(m_Data->buffer_depth) * sizeof (double)))) != nullptr) {
                            m_Data->buffer_left[i] = tmp;
                        }
                    }
                    if (m_Data->buffer_right[i] != nullptr) {
                        if ((tmp = static_cast<double*>(my_realloc (m_Data->buffer_right[i], static_cast<size_t>(m_Data->buffer_depth) * sizeof (double)))) != nullptr) {
                            m_Data->buffer_right[i] = tmp;
                        }
                    }
               }
               LeaveOsziCycleCS ();
            } else {
                m_Data->buffer_depth = k;
            }
            for (i = 0; i < 21; i++) {
                m_Data->wr_poi_left[i] = 0;
                m_Data->depth_left[i] = 0;
                m_Data->wr_poi_right[i] = 0;
                m_Data->depth_right[i] = 0;
            }

            SwitchOscilloscopeOnline (m_Data, GetSimulatedTimeSinceStartedInNanoSecond(), 1);
        }

        if (m_Data->pre_trigger > m_Data->buffer_depth)
            m_Data->pre_trigger = m_Data->buffer_depth;

        QString BackgroundImage = ui->BackgroundImageLineEdit->text().trimmed();
        if ((BackgroundImage.length() > 0) &&
            (strlen(QStringToConstChar(BackgroundImage)) < sizeof(m_Data->BackgroundImage))) {
            strcpy(m_Data->BackgroundImage, QStringToConstChar(BackgroundImage));
            m_Data->BackgroundImageLoaded = SHOULD_LOAD_BACKGROUND_IMAGE;
        } else {
            m_Data->BackgroundImage[0] = 0;
            if (m_Data->BackgroundImageLoaded == BACKGROUND_IMAGE_LOADED) {
                m_Data->BackgroundImageLoaded = SHOULD_DELETE_BACKGROUND_IMAGE;
            }
        }
    }
}

void OscilloscopeConfigDialog::EnableDisableTrigger(bool par_Enable, bool par_Init)
{
    if (par_Enable) {
        if (m_Model == nullptr) {
            // Start Speedup Combobox
            static_cast<QListView*>(ui->TriggerVariableComboBox->view())->setUniformItemSizes(true);
            static_cast<QListView*>(ui->TriggerVariableComboBox->view())->setLayoutMode(QListView::Batched);
            // End Speedup Combobox
            m_Model = MainWindow::GetBlackboardVariableModel();
            m_SortModel = new QSortFilterProxyModel;
            m_SortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
            m_SortModel->sort(0);
            m_SortModel->setSourceModel(m_Model);
            ui->TriggerVariableComboBox->setModel(m_SortModel);
            char temp_variname[BBVARI_NAME_SIZE];
            if (GetBlackboardVariableName (m_Data->trigger_vid, temp_variname, sizeof(temp_variname)) == 0) {
                ui->TriggerVariableComboBox->setCurrentText(QString(temp_variname));
            }
        }
        ui->TriggerVariableComboBox->setEnabled(true);
        ui->TriggerEventComboBox->setEnabled(true);
        ui->TriggerValueLineEdit->setEnabled(true);
        if (par_Init) ui->TriggerCheckBox->setChecked(true);
    } else {
        ui->TriggerVariableComboBox->setDisabled(true);
        ui->TriggerEventComboBox->setDisabled(true);
        ui->TriggerValueLineEdit->setDisabled(true);
        if (par_Init) ui->TriggerCheckBox->setChecked(false);
    }
}


void OscilloscopeConfigDialog::on_TriggerCheckBox_clicked(bool checked)
{
    EnableDisableTrigger(checked);
}

void OscilloscopeConfigDialog::on_BackgroundImagePushButton_clicked()
{
    QString FileName = FileDialog::getOpenFileName(this, "background image file", QString(), IMAGE_EXT);
    if (FileName.size() > 0) {
        ui->BackgroundImageLineEdit->setText(FileName);
    }
}
