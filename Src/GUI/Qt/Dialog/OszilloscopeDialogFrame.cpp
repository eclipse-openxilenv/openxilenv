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


#include "OszilloscopeDialogFrame.h"
#include "ui_OszilloscopeDialogFrame.h"
#include "OscilloscopeLineWidthWidget.h"
#include "DefaultElementDialog.h"
#include "OscilloscopeCyclic.h"
#include "ColorHelpers.h"
#include "StringHelpers.h"

#include <math.h>

extern "C" {
#include "MyMemory.h"
#include "ThrowError.h"
}

OszilloscopeDialogFrame::OszilloscopeDialogFrame(OSCILLOSCOPE_DATA* arg_data, QWidget *parent) :
    CustomDialogFrame(parent),
    ui(new Ui::OszilloscopeDialogFrame)
{
    ui->setupUi(this);
    m_data = arg_data;
    int loc_rgbColor;
    long loc_vid;
    char *loc_varName;
    QColor loc_color;
    int loc_textAlignment;
    bool loc_decPhysFlag;
    char loc_displayDecHexBin;
    bool loc_disable;
    double loc_minValue;
    double loc_maxValue;
    int loc_lineWidth;
    char loc_presentation;

    if(m_data->sel_left_right) {
        loc_vid = m_data->vids_right[m_data->sel_pos_right];
        loc_varName = m_data->name_right[m_data->sel_pos_right];
        loc_lineWidth = m_data->LineSize_right[m_data->sel_pos_right];
        loc_minValue = m_data->min_right[m_data->sel_pos_right];
        loc_maxValue = m_data->max_right[m_data->sel_pos_right];
        loc_disable = m_data->vars_disable_right[m_data->sel_pos_right];
        loc_rgbColor = m_data->color_right[m_data->sel_pos_right];
        loc_textAlignment = m_data->txt_align_right[m_data->sel_pos_right];
        loc_decPhysFlag = m_data->dec_phys_right[m_data->sel_pos_right];
        loc_displayDecHexBin = m_data->dec_hex_bin_right[m_data->sel_pos_right];
        loc_presentation = m_data->presentation_right[m_data->sel_pos_right];
    } else {
        loc_vid = m_data->vids_left[m_data->sel_pos_left];
        loc_varName = m_data->name_left[m_data->sel_pos_left];
        loc_lineWidth = m_data->LineSize_left[m_data->sel_pos_left];
        loc_minValue = m_data->min_left[m_data->sel_pos_left];
        loc_maxValue = m_data->max_left[m_data->sel_pos_left];
        loc_disable = m_data->vars_disable_left[m_data->sel_pos_left];
        loc_rgbColor = m_data->color_left[m_data->sel_pos_left];
        loc_textAlignment = m_data->txt_align_left[m_data->sel_pos_left];
        loc_decPhysFlag = m_data->dec_phys_left[m_data->sel_pos_left];
        loc_displayDecHexBin = m_data->dec_hex_bin_left[m_data->sel_pos_left];
        loc_presentation = m_data->presentation_left[m_data->sel_pos_left];
    }
    loc_color = QColor((loc_rgbColor >> 0) & 0xFF, (loc_rgbColor >> 8) & 0xFF, (loc_rgbColor >> 16) & 0xFF);

    m_dlg = qobject_cast<DefaultElementDialog*>(parent);
    if(m_dlg) {
        connect(m_dlg->getDefaultFrame(), SIGNAL(variableSelectionChanged(QString,bool)), this, SLOT(changeVariable(QString,bool)));
        connect(m_dlg->getDefaultFrame(), SIGNAL(colorChanged(QColor)), this, SLOT(changeColor(QColor)));
        connect(m_dlg->getDefaultFrame(), SIGNAL(colorChanged(QColor)), ui->widgetLineWidth, SLOT(setLineColor(QColor)));
    }
    connect(ui->spinBoxLineWidth, SIGNAL(valueChanged(int)), ui->widgetLineWidth, SLOT(setLineWidth(int)));

    if(loc_vid > 0) {
        if(m_dlg) {
            m_dlg->getDefaultFrame()->setCurrentVisibleVariable(QString(loc_varName));
            m_dlg->getDefaultFrame()->setCurrentColor(loc_color);
        }
    } else {
        m_dlg->getDefaultFrame()->setCurrentVisibleVariables(QStringList());
    }

    if(loc_decPhysFlag) {
        ui->radioButtonPhysically->setChecked(true);
    } else {
        switch(loc_displayDecHexBin) {
            case 0:
            default:
                ui->radioButtonDecimal->setChecked(true);
                break;
            case 1:
                ui->radioButtonHexadecimal->setChecked(true);
                break;
            case 2:
                ui->radioButtonBinary->setChecked(true);
                break;
        }
    }
    ui->checkBoxSignalNotVisible->setChecked(loc_disable != 0);
    ui->checkBoxLabelRightAligned->setChecked(loc_textAlignment != 0);
    ui->spinBoxLineWidth->setValue(loc_lineWidth);
    ui->widgetLineWidth->SetLineWidthAndColor(loc_lineWidth, loc_color);
    ui->lineEditMinValue->SetValue(loc_minValue);
    ui->lineEditMaxValue->SetValue(loc_maxValue);
    switch (loc_presentation) {
    default:
    case 0:
        ui->LinesRadioButton->setChecked(true);
        break;
    case 1:
        ui->PointsRadioButton->setChecked(true);
        break;
    }

    m_currentVid = loc_vid;
    m_currentTextAlignment = loc_textAlignment;
    m_currentDecPhysFlag = loc_decPhysFlag;
    m_currentDisplayDecHexBin = loc_displayDecHexBin;
    m_currentDisable = loc_disable;
    m_currentRgbColor = loc_rgbColor;
    m_currentLineWidth = loc_lineWidth;
    m_currentMinValue = loc_minValue;
    m_currentMaxValue = loc_maxValue;
    m_currentColor = loc_color;
    m_presentation = loc_presentation;

}

OszilloscopeDialogFrame::~OszilloscopeDialogFrame()
{
    m_data = nullptr;
    delete m_data;
    delete ui;
}



void OszilloscopeDialogFrame::userAccept()
{
    int vid;
    char *variname;
    double loc_minValue;
    double loc_maxValue;
    bool loc_VariableChanged = false;
    bool loc_PhysChanged = false;

    if (m_data->sel_left_right) {
        vid = m_data->vids_right[m_data->sel_pos_right];
        variname = m_data->name_right[m_data->sel_pos_right];
    } else {
        vid = m_data->vids_left[m_data->sel_pos_left];
        variname = m_data->name_left[m_data->sel_pos_left];
    }

    if (!m_currentVariableName.isEmpty()) {
        if (vid > 0) {
            if (variname != nullptr) {
                if(m_currentVariableName.compare(variname)) {
                    remove_bbvari_unknown_wait (vid);   // Delete the old variable
                    vid = 0;
                }
            }
        }
        // Register new variable
        if (vid <= 0) {
            vid = add_bbvari (QStringToConstChar(m_currentVariableName), BB_UNKNOWN_WAIT, nullptr);
            loc_VariableChanged = true;
        }
        if (vid <= 0) {
            ThrowError (1, "cannot attach variable \"%s\"", QStringToConstChar(m_currentVariableName));
            return;
        }

        if (loc_VariableChanged) {
            EnterOsziCycleCS ();                                            // 1.  Order is importante
            EnterOsciWidgetCriticalSection (m_data->CriticalSectionNumber); // 2.
            if (m_data->sel_left_right) {
                m_data->vids_right[m_data->sel_pos_right] = vid;
                m_data->name_right[m_data->sel_pos_right] = ReallocCopyString(m_data->name_right[m_data->sel_pos_right],
                                                                              m_currentVariableName);
                if (m_data->buffer_right[m_data->sel_pos_right] != nullptr) my_free (m_data->buffer_right[m_data->sel_pos_right]);
                if ((m_data->buffer_right[m_data->sel_pos_right] = static_cast<double*>(my_calloc (static_cast<size_t>(m_data->buffer_depth), sizeof(double)))) == nullptr) {
                    ThrowError (1, "out of memory");
                }
            } else {
                m_data->vids_left[m_data->sel_pos_left] = vid;
                m_data->name_left[m_data->sel_pos_left] = ReallocCopyString(m_data->name_left[m_data->sel_pos_left],
                                                                            m_currentVariableName);
                if (m_data->buffer_left[m_data->sel_pos_left] != nullptr) my_free (m_data->buffer_left[m_data->sel_pos_left]);
                if ((m_data->buffer_left[m_data->sel_pos_left] = static_cast<double*>(my_calloc (static_cast<size_t>(m_data->buffer_depth), sizeof(double)))) == nullptr) {
                    ThrowError (1, "out of memory");
                }
            }
            ResetCurrentBuffer();
            LeaveOsciWidgetCriticalSection (m_data->CriticalSectionNumber); // 2.
            LeaveOsziCycleCS ();                                            // 1.
        }
    }

    loc_minValue = ui->lineEditMinValue->GetDoubleValue();
    loc_maxValue = ui->lineEditMaxValue->GetDoubleValue();

    if(loc_minValue >= loc_maxValue) {
        loc_maxValue = loc_minValue + 1.0;
    }

    if(m_data->sel_left_right) {
        m_data->min_right[m_data->sel_pos_right] = loc_minValue;
        m_data->max_right[m_data->sel_pos_right] = loc_maxValue;
        m_data->color_right[m_data->sel_pos_right] = (m_currentColor.red() << 0) + (m_currentColor.green() << 8) + (m_currentColor.blue() << 16);
        m_data->LineSize_right[m_data->sel_pos_right] = ui->spinBoxLineWidth->value();
    } else {
        m_data->min_left[m_data->sel_pos_left] = loc_minValue;
        m_data->max_left[m_data->sel_pos_left] = loc_maxValue;
        m_data->color_left[m_data->sel_pos_left] = (m_currentColor.red() << 0) + (m_currentColor.green() << 8) + (m_currentColor.blue() << 16);
        m_data->LineSize_left[m_data->sel_pos_left] = ui->spinBoxLineWidth->value();
    }

    if(ui->radioButtonPhysically->isChecked()) {
        loc_PhysChanged = ChangePhysFlag(1, 0);
    } else if(ui->radioButtonHexadecimal->isChecked()) {
        loc_PhysChanged = ChangePhysFlag(0, 1);
    } else if(ui->radioButtonBinary->isChecked()) {
        loc_PhysChanged = ChangePhysFlag(0, 2);
    } else {
        loc_PhysChanged = ChangePhysFlag(0, 0);   // Decimal raw value
    }

    if(ui->checkBoxLabelRightAligned->isChecked()) {
        if(m_data->sel_left_right) {
            m_data->txt_align_right[m_data->sel_pos_right] = 1;
        } else {
            m_data->txt_align_left[m_data->sel_pos_left] = 1;
        }
    } else {
        if(m_data->sel_left_right) {
            m_data->txt_align_right[m_data->sel_pos_right] = 0;
        } else {
            m_data->txt_align_left[m_data->sel_pos_left] = 0;
        }
    }
    if(ui->checkBoxSignalNotVisible->isChecked()) {
        if(m_data->sel_left_right) {
            m_data->vars_disable_right[m_data->sel_pos_right] = 1;
        } else{
            m_data->vars_disable_left[m_data->sel_pos_left] = 1;
        }
    } else {
        if(m_data->sel_left_right) {
            m_data->vars_disable_right[m_data->sel_pos_right] = 0;
        } else {
            m_data->vars_disable_left[m_data->sel_pos_left] = 0;
        }
    }
    if (ui->PointsRadioButton->isChecked()) {
        if(m_data->sel_left_right) {
            m_data->presentation_right[m_data->sel_pos_right] = 1;
        } else {
            m_data->presentation_left[m_data->sel_pos_left] = 1;
        }
    } else {
        if(m_data->sel_left_right) {
            m_data->presentation_right[m_data->sel_pos_right] = 0;
        } else {
            m_data->presentation_left[m_data->sel_pos_left] = 0;
        }
    }
    if (loc_VariableChanged || loc_PhysChanged) {
        if (update_vid_owc (m_data)) {
            ThrowError (1, "cannot update variable list");
        }
    }
}

void OszilloscopeDialogFrame::userReject()
{
    if(m_data->sel_left_right) {
        m_data->vids_right[m_data->sel_pos_right] = m_currentVid;
        m_data->LineSize_right[m_data->sel_pos_right] = m_currentLineWidth;
        m_data->min_right[m_data->sel_pos_right] = m_currentMinValue;
        m_data->max_right[m_data->sel_pos_right] = m_currentMaxValue;
        m_data->vars_disable_right[m_data->sel_pos_right] = m_currentDisable;
        m_data->color_right[m_data->sel_pos_right] = m_currentRgbColor;
        m_data->txt_align_right[m_data->sel_pos_right] = m_currentTextAlignment;
        m_data->dec_phys_right[m_data->sel_pos_right] = m_currentDecPhysFlag;
        m_data->dec_hex_bin_right[m_data->sel_pos_right] = m_currentDisplayDecHexBin;
        m_data->presentation_right[m_data->sel_pos_right] = m_presentation;
    } else {
        m_data->vids_left[m_data->sel_pos_left] = m_currentVid;
        m_data->LineSize_left[m_data->sel_pos_left] = m_currentLineWidth;
        m_data->min_left[m_data->sel_pos_left] = m_currentMinValue;
        m_data->max_left[m_data->sel_pos_left] = m_currentMaxValue;
        m_data->vars_disable_left[m_data->sel_pos_left] = m_currentDisable;
        m_data->color_left[m_data->sel_pos_left] = m_currentRgbColor;
        m_data->txt_align_left[m_data->sel_pos_left] = m_currentTextAlignment;
        m_data->dec_phys_left[m_data->sel_pos_left] = m_currentDecPhysFlag;
        m_data->dec_hex_bin_left[m_data->sel_pos_left] = m_currentDisplayDecHexBin;
        m_data->presentation_left[m_data->sel_pos_left] = m_presentation;
    }
}

void OszilloscopeDialogFrame::SetUsedColors(QList<QColor> &par_UsedColors)
{
    m_UsedColors = par_UsedColors;
}

void OszilloscopeDialogFrame::changeVariable(QString arg_variable, bool arg_visible)
{
    if(arg_visible) {
        m_currentVariableName = arg_variable;
        int Vid = get_bbvarivid_by_name(QStringToConstChar(m_currentVariableName));
        if (Vid > 0) {
            QColor loc_color;
            int loc_rgbColor = get_bbvari_color(Vid);
            if (loc_rgbColor < 0) {
                loc_color = GetColorProposal(m_UsedColors);
            } else {
                loc_color = QColor((loc_rgbColor >> 0) & 0xFF, (loc_rgbColor >> 8) & 0xFF, (loc_rgbColor >> 16) & 0xFF);
            }
            m_dlg->getDefaultFrame()->setCurrentColor(loc_color);
            double loc_minValue = get_bbvari_min(Vid);
            double loc_maxValue = get_bbvari_max(Vid);
            int loc_convType = get_bbvari_conversiontype(Vid);
            if (loc_convType == BB_CONV_NONE) {
                ui->radioButtonPhysically->setEnabled(false);
                ui->radioButtonDecimal->setChecked(true);
            } else {
                ui->radioButtonPhysically->setEnabled(true);
                ui->radioButtonPhysically->setChecked(true);
            }
            ui->lineEditMinValue->SetValue(loc_minValue);
            ui->lineEditMaxValue->SetValue(loc_maxValue);
            changeColor(loc_color);
        }
    } else {
        m_currentVariableName = QString();
    }
}

void OszilloscopeDialogFrame::changeColor(QColor arg_color)
{
    m_currentColor = arg_color;
/*    if(m_data->sel_left_right) {
        m_data->color_right[m_data->sel_pos_right] = (m_currentColor.red() << 0) + (m_currentColor.green() << 8) + (m_currentColor.blue() << 16);
    } else {
        m_data->color_left[m_data->sel_pos_left] = (m_currentColor.red() << 0) + (m_currentColor.green() << 8) + (m_currentColor.blue() << 16);
    }
    */
}

bool OszilloscopeDialogFrame::ChangePhysFlag(char NewPhysFlag, char NewDecHexBinFlag)
{
    bool Ret = false;
    EnterOsziCycleCS ();                                            // 1.  Order is important
    EnterOsciWidgetCriticalSection (m_data->CriticalSectionNumber); // 2.
    if(m_data->sel_left_right) {
        if (m_data->dec_phys_right[m_data->sel_pos_right] != NewPhysFlag) {
            m_data->dec_phys_right[m_data->sel_pos_right] = NewPhysFlag;
            if (get_bbvari_conversiontype(m_data->vids_right[m_data->sel_pos_right]) == BB_CONV_FORMULA) {
                ResetCurrentBuffer();
                Ret = true;
            }
        }
        m_data->dec_hex_bin_right[m_data->sel_pos_right] = NewDecHexBinFlag;
    } else {
        if (m_data->dec_phys_left[m_data->sel_pos_left] != NewPhysFlag) {
            m_data->dec_phys_left[m_data->sel_pos_left] = NewPhysFlag;
            if (get_bbvari_conversiontype(m_data->vids_left[m_data->sel_pos_left]) == BB_CONV_FORMULA) {
                ResetCurrentBuffer();
                Ret = true;
            }
        }
        m_data->dec_hex_bin_left[m_data->sel_pos_left] = NewDecHexBinFlag;
    }
    LeaveOsciWidgetCriticalSection (m_data->CriticalSectionNumber); // 2.
    LeaveOsziCycleCS ();                                            // 1.

    return Ret;
}

void OszilloscopeDialogFrame::ResetCurrentBuffer()
{
    if(m_data->sel_left_right) {
        m_data->wr_poi_right[m_data->sel_pos_right] = 0;
        m_data->depth_right[m_data->sel_pos_right] = 0;
        double *Buffer = m_data->buffer_right[m_data->sel_pos_right];
        for (int x = 0; x < m_data->buffer_depth; x++) {
           Buffer[x] = static_cast<double>(NAN);
        }
    } else {
        m_data->wr_poi_left[m_data->sel_pos_left] = 0;
        m_data->depth_left[m_data->sel_pos_left] = 0;
        double *Buffer = m_data->buffer_left[m_data->sel_pos_left];
        for (int x = 0; x < m_data->buffer_depth; x++) {
           Buffer[x] = static_cast<double>(NAN);
        }
    }

}
