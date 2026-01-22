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


#include <QColorDialog>
#include <QFontDialog>

#include "KnobDialogFrame.h"
#include "ui_KnobDialogFrame.h"
#include "DefaultElementDialog.h"
#include "Blackboard.h"
#include <QFileDialog>
#include "FileDialog.h"
#include "MdiWindowType.h"

extern "C" {
#include "Compare2DoubleEqual.h"
#include "FileExtensions.h"
}

void KnobDialogFrame::Init()
{
    // Basics
    m_DefaultElementDialog->getDefaultFrame()->setCurrentColor(m_KnobOrTacho->getColor());
    m_DefaultElementDialog->getDefaultFrame()->setCurrentFont(m_KnobOrTacho->font());
    ui->physicalRadioButton->setChecked(m_KnobOrTacho->isPhys());
    ui->configDirectCheckBox->setChecked(m_KnobOrTacho->getUseBlackboardConfig());
    variableDirectConfig(m_KnobOrTacho->getUseBlackboardConfig());
    ui->lineEditMinValue->SetValue(m_KnobOrTacho->getMin());
    ui->lineEditMaxValue->SetValue(m_KnobOrTacho->getMax());
    // Scale
    ui->ScaleGroupBox->setChecked(m_KnobOrTacho->getScaleFlag());
    ui->ScaleMaxAngleSpinBox->setValue(m_KnobOrTacho->getMaxAngle());
    ui->ScaleMinAngleSpinBox->setValue(m_KnobOrTacho->getMinAngle());
    ui->ScalePosXDoubleSpinBox->setValue(m_KnobOrTacho->getScalePosX());
    ui->ScalePosYDoubleSpinBox->setValue(m_KnobOrTacho->getScalePosY());
    ui->ScaleSizeCheckBox->setChecked(m_KnobOrTacho->getScaleSizeFlag());
    ScaleSizeFlagChanged(m_KnobOrTacho->getScaleSizeFlag());
    ui->ScaleStepCheckBox->setChecked(m_KnobOrTacho->getScaleStepFlag());
    ScaleStepFlagChanged(m_KnobOrTacho->getScaleStepFlag());
    ui->ScaleStepDoubleSpinBox->setValue(m_KnobOrTacho->getScaleStep());
    ui->ScaleSizeCheckBox->setChecked(m_KnobOrTacho->getScaleSizeFlag());
    ui->ScaleSizeDoubleSpinBox->setValue(m_KnobOrTacho->getScaleSize());
    ScaleColorFlagChanged(m_KnobOrTacho->getScaleColorFlag());
    if(m_KnobOrTacho->getScaleColorFlag()) {
        ui->ScaleColorCheckBox->setChecked(true);
        ChangeColorButton(ui->ScaleColorPushButton, m_KnobOrTacho->getScaleColor());
    }
    // Knob or Needle
    ui->knobGroupBox->setChecked(m_KnobOrTacho->getNeedleFlag());
    ui->needlePosXDoubleSpinBox->setValue(m_KnobOrTacho->getNeedlePosX());
    ui->needlePosYDoubleSpinBox->setValue(m_KnobOrTacho->getNeedlePosY());
    needleSizeFlagChanged(m_KnobOrTacho->getNeedleSizeFlag());
    ui->needleSizeCheckBox->setChecked(m_KnobOrTacho->getNeedleSizeFlag());
    ui->needleSizeDoubleSpinBox->setValue(m_KnobOrTacho->getNeedleLength());
    needleLineColorFlagChanged(m_KnobOrTacho->getNeedleLineColorFlag());
    if(m_KnobOrTacho->getNeedleLineColorFlag()) {
        ui->needleLineColorCheckBox->setChecked(true);
        ChangeColorButton(ui->needleLineColorPushButton, m_KnobOrTacho->getNeedleLineColor());
    }
    needleFillColorFlagChanged(m_KnobOrTacho->getNeedleFillColorFlag());
    if(m_KnobOrTacho->getNeedleFillColorFlag()) {
        ui->needleFillColorCheckBox->setChecked(true);
        ChangeColorButton(ui->needleFillColorPushButton, m_KnobOrTacho->getNeedleFillColor());
    }
    // Value
    ui->textGroupBox->setChecked(m_KnobOrTacho->getTextFlag());
    textFontFlagChanged(m_KnobOrTacho->getTextFontFlag());
    if(m_KnobOrTacho->getTextFontFlag()) {
        ui->textFontCheckBox->setChecked(true);
    }
    ui->textAlignComboBox->setCurrentIndex(m_KnobOrTacho->getTextAlign());
    HideTextPos(m_KnobOrTacho->getTextAlign() != 3);
    ui->textPosXDoubleSpinBox->setValue(m_KnobOrTacho->getTextPosX());
    ui->textPosYDoubleSpinBox->setValue(m_KnobOrTacho->getTextPosY());
    // Name
    ui->nameGroupBox->setChecked(m_KnobOrTacho->getNameFlag());
    nameFontFlagChanged(m_KnobOrTacho->getNameFontFlag());
    if(m_KnobOrTacho->getNameFontFlag()) {
        ui->nameFontCheckBox->setChecked(true);
    }
    ui->nameAlignComboBox->setCurrentIndex(m_KnobOrTacho->getNameAlign());
    HideNamePos(m_KnobOrTacho->getNameAlign() != 3);
    ui->namePosXDoubleSpinBox->setValue(m_KnobOrTacho->getNamePosX());
    ui->namePosYDoubleSpinBox->setValue(m_KnobOrTacho->getNamePosY());
    // Bitmap
    ui->bitmapGroupBox->setChecked(m_KnobOrTacho->getBitmapFlag());
    if (m_KnobOrTacho->getBitmapFlag()) {
        ui->bitmapLineEdit->setText(m_KnobOrTacho->getBitmapPath());
        scaleBitmap(m_KnobOrTacho->getBitmapSizeBehaviour());
        switch(m_KnobOrTacho->getBitmapSizeBehaviour()) {
        default:
        case KnobOrTachoWidget::DontScaleBitmapNorFixedWindowSize:
            ui->fixedSizeRadioButton->setChecked(true);
            break;
        case KnobOrTachoWidget::FixedWindowSize:
            ui->resizeToFixedBitmapRadioButton->setChecked(true);
            break;
        case KnobOrTachoWidget::ScaleBitMap:
            ui->scaleToWindowRadioButton->setChecked(true);
            break;
        }
    }
    // If release fallback to
    ui->IfReleaseFallbackToGroupBox->setChecked(m_KnobOrTacho->getIfReleaseFallbackToFlag());
    if (m_KnobOrTacho->getIfReleaseFallbackToFlag()) {
        ui->IfReleaseFallbackToLineEdit->SetValue(m_KnobOrTacho->getIfReleaseFallbackToValue());
    }
    // Drag status
    ui->DragStatusSignalGroupBox->setChecked(m_KnobOrTacho->getDragStatusFlag());
    if (m_KnobOrTacho->getDragStatusFlag()) {
        ui->DragStatusSignalLineEdit->setText(m_KnobOrTacho->getDragStatusSignal());
    }
    // save drag status
    m_DragStatusFlagSave = m_KnobOrTacho->getDragStatusFlag();
    m_DragStatusNameSave = m_KnobOrTacho->getDragStatusSignal();
}

KnobDialogFrame::KnobDialogFrame(KnobOrTachoWidget* arg_tacho, QWidget* arg_parent) :
    CustomDialogFrame(arg_parent),
    ui(new Ui::KnobDialogFrame)
{
    ui->setupUi(this);
    m_DefaultElementDialog = qobject_cast<DefaultElementDialog*>(arg_parent);
    m_KnobOrTacho = arg_tacho;

    m_BitmapButtonGroup = new QButtonGroup(this);
    m_BitmapButtonGroup->addButton(ui->fixedSizeRadioButton, KnobOrTachoWidget::DontScaleBitmapNorFixedWindowSize);
    m_BitmapButtonGroup->addButton(ui->resizeToFixedBitmapRadioButton, KnobOrTachoWidget::FixedWindowSize);
    m_BitmapButtonGroup->addButton(ui->scaleToWindowRadioButton, KnobOrTachoWidget::ScaleBitMap);
    m_RawPhysButtonGroup = new QButtonGroup(this);
    m_RawPhysButtonGroup->addButton(ui->decimalRadioButton, 0);
    m_RawPhysButtonGroup->addButton(ui->physicalRadioButton, 1);

    FillInheritFromComboBox();

    FillAlignComboBox(ui->textAlignComboBox);
    FillAlignComboBox(ui->nameAlignComboBox);

    if (m_KnobOrTacho->GetType() == KnobOrTachoWidget::SliderType) {
        ui->bitmapGroupBox->setVisible(false);
    } else {
        ui->IfReleaseFallbackToGroupBox->setVisible(false);
        ui->DragStatusSignalGroupBox->setVisible(false);
    }
    /*if (m_KnobOrTacho->GetType() == KnobOrTachoWidget::BargraphType) {
        ui->knobGroupBox->setVisible(false);
    }*/
    if ((m_KnobOrTacho->GetType() == KnobOrTachoWidget::BargraphType) ||
        (m_KnobOrTacho->GetType() == KnobOrTachoWidget::SliderType)) {
        ui->ScaleMinAngleLabel->setVisible(false);
        ui->ScaleMinAngleSpinBox->setVisible(false);
        ui->ScaleMaxAngleLabel->setVisible(false);
        ui->ScaleMaxAngleSpinBox->setVisible(false);
        ui->ScaleStepDoubleSpinBox->setSuffix("%");
    }

    Init();

     // Basics
    if(m_DefaultElementDialog) {
        connect(m_DefaultElementDialog->getDefaultFrame(), SIGNAL(variableSelectionChanged(QString,bool)), this, SLOT(changeVaraible(QString,bool)));
        connect(m_DefaultElementDialog->getDefaultFrame(), SIGNAL(colorChanged(QColor)), this, SLOT(ColorChanged(QColor)));
        //connect(m_DefaultElementDialog->getDefaultFrame(), SIGNAL(fontChanged(QFont)), this, SLOT(FontChanged(QFont)));
    }
    connect(ui->inheritComboBox, SIGNAL(currentTextChanged(QString)), this, SLOT(InheritChanged(QString)));
    connect(m_RawPhysButtonGroup, SIGNAL(idClicked(int)), this, SLOT(RawPhysicalChanged(int)));
    connect(ui->configDirectCheckBox, SIGNAL(toggled(bool)), this, SLOT(DirectConfigChanged(bool)));
    connect(ui->lineEditMinValue, SIGNAL(ValueChanged(double,bool)), this, SLOT(minValueCahnged(double,bool)));
    connect(ui->lineEditMaxValue, SIGNAL(ValueChanged(double,bool)), this, SLOT(maxValueChanged(double,bool)));
    // Scale
    connect(ui->ScaleGroupBox, SIGNAL(clicked(bool)), this, SLOT(ScaleVisible(bool)));
    connect(ui->ScaleMinAngleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(minAngleChanged(double)));
    connect(ui->ScaleMaxAngleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(maxAngleChanged(double)));
    connect(ui->ScaleStepCheckBox, SIGNAL(clicked(bool)), this, SLOT(ScaleStepFlagChanged(bool)));
    connect(ui->ScaleStepDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(ScaleStepChanged(double)));
    connect(ui->ScalePosXDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(ScalePositionXChanged(double)));
    connect(ui->ScalePosYDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(ScalePositionYChanged(double)));
    connect(ui->ScaleSizeDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(ScaleSizeChanged(double)));
    connect(ui->ScaleSizeCheckBox, SIGNAL(clicked(bool)), this, SLOT(ScaleSizeFlagChanged(bool)));
    connect(ui->ScaleColorCheckBox, SIGNAL(clicked(bool)), this, SLOT(ScaleColorFlagChanged(bool)));
    connect(ui->ScaleColorPushButton, SIGNAL(clicked()), this, SLOT(ScaleColorChoose()));

    // Knob or Needle
    connect(ui->knobGroupBox, SIGNAL(clicked(bool)), this, SLOT(neeldeVisible(bool)));
    connect(ui->needlePosXDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(needlePositionXChanged(double)));
    connect(ui->needlePosYDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(needlePositionYChanged(double)));
    connect(ui->needleSizeDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(needleSizeChanged(double)));
    connect(ui->needleSizeCheckBox, SIGNAL(clicked(bool)), this, SLOT(needleSizeFlagChanged(bool)));
    connect(ui->needleLineColorCheckBox, SIGNAL(clicked(bool)), this, SLOT(needleLineColorFlagChanged(bool)));
    connect(ui->needleLineColorPushButton, SIGNAL(clicked()), this, SLOT(needleLineColorChoose()));
    connect(ui->needleFillColorCheckBox, SIGNAL(clicked(bool)), this, SLOT(needleFillColorFlagChanged(bool)));
    connect(ui->needleFillColorPushButton, SIGNAL(clicked()), this, SLOT(needleFillColorChoose()));
    // Value
    connect(ui->textGroupBox, SIGNAL(clicked(bool)), this, SLOT(textVisible(bool)));
    connect(ui->textFontCheckBox, SIGNAL(clicked(bool)), this, SLOT(textFontFlagChanged(bool)));
    connect(ui->textFontPushButton, SIGNAL(clicked()), this, SLOT(textFontChoose()));
    connect(ui->textAlignComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(textFontAlignChanged(int)));
    connect(ui->textPosXDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(textPositionXChanged(double)));
    connect(ui->textPosYDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(textPositionYChanged(double)));
    // Name
    connect(ui->nameGroupBox, SIGNAL(clicked(bool)), this, SLOT(nameVisible(bool)));
    connect(ui->nameFontCheckBox, SIGNAL(clicked(bool)), this, SLOT(nameFontFlagChanged(bool)));
    connect(ui->nameFontPushButton, SIGNAL(clicked()), this, SLOT(nameFontChoose()));
    connect(ui->nameAlignComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(nameFontAlignChanged(int)));
    connect(ui->namePosXDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(namePositionXChanged(double)));
    connect(ui->namePosYDoubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(namePositionYChanged(double)));
    // Bitmap
    connect(ui->bitmapGroupBox, SIGNAL(clicked(bool)), this, SLOT(bitmapVisible(bool)));
    connect(ui->bitmapPushButton, SIGNAL(clicked()), this, SLOT(openBitmapPathDialog()));
    connect(m_BitmapButtonGroup, SIGNAL(idClicked(int)), this, SLOT(scaleBitmap(int)));
    // If release fallback to
    connect(ui->IfReleaseFallbackToGroupBox, SIGNAL(clicked(bool)), this, SLOT(IfReleaseFallbackToFlagChanged(bool)));
    connect(ui->IfReleaseFallbackToLineEdit, SIGNAL(ValueChanged(double,bool)), this, SLOT(IfReleaseFallbackToValueChanged(double,bool)));
    // Drag status
    connect(ui->DragStatusSignalGroupBox, SIGNAL(clicked(bool)), this, SLOT(DragStatusFlagChanged(bool)));
    connect(ui->DragStatusSignalLineEdit, SIGNAL(textEdited(QString)), this, SLOT(DragStatusSignalChanged(QString)));
}

KnobDialogFrame::~KnobDialogFrame()
{
    delete m_RawPhysButtonGroup;
    delete m_BitmapButtonGroup;
    delete ui;
}

void KnobDialogFrame::setBBConfigDirect(bool arg_value)
{
    ui->configDirectCheckBox->setChecked(arg_value);
    variableDirectConfig(arg_value);
}

void KnobDialogFrame::setVaraiblePhysical(bool arg_value)
{
    ui->physicalRadioButton->setChecked(arg_value);
}

void KnobDialogFrame::setBitmapPath(QString arg_path)
{
    ui->bitmapLineEdit->setText(arg_path);
}

void KnobDialogFrame::setScaleActive(bool arg_value)
{
    ui->ScaleGroupBox->setChecked(arg_value);
}

void KnobDialogFrame::setNeedleActive(bool arg_value)
{
    ui->knobGroupBox->setChecked(arg_value);
}

void KnobDialogFrame::setTextActive(bool arg_value)
{
    ui->textGroupBox->setChecked(arg_value);
}

void KnobDialogFrame::setBitmapActive(bool arg_value)
{
    ui->bitmapGroupBox->setChecked(arg_value);
}

void KnobDialogFrame::setTextforknoborslider(QString arg_text)
{
    ui->knobGroupBox->setTitle(arg_text);
}

void KnobDialogFrame::userAccept()
{
    m_KnobOrTacho->CheckChangeOfDragStatusSignal(m_DragStatusFlagSave, m_DragStatusNameSave);
}

void KnobDialogFrame::userReject()
{
}

// Basic

void KnobDialogFrame::changeVaraible(QString arg_variable, bool arg_checked)
{
    double loc_min, loc_max;
    if(arg_checked) {
        VID loc_vid = get_bbvarivid_by_name(arg_variable.toLocal8Bit().data());
        loc_min = get_bbvari_min(loc_vid);
        loc_max = get_bbvari_max(loc_vid);
    } else {
        loc_min = 0.0;
        loc_max = 1.0;
    }

    QString loc_tooltip = QString("Lower limit: %1\nUpper limit: %2").arg(QString::number(loc_min), QString::number(loc_max));

    ui->lineEditMinValue->setToolTip(loc_tooltip);
    ui->lineEditMaxValue->setToolTip(loc_tooltip);
    ui->minValueLabel->setToolTip(loc_tooltip);
    ui->maxValueLabel->setToolTip(loc_tooltip);

    if(ui->configDirectCheckBox->isChecked() || !arg_checked) {
        ui->lineEditMinValue->SetValue(loc_min);
        ui->lineEditMaxValue->SetValue(loc_max);
    }
}

void KnobDialogFrame::ColorChanged(QColor par_Color)
{
    m_KnobOrTacho->setColor(par_Color);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::minValueCahnged(double arg_value, bool par_OutOfRange)
{
    if (!par_OutOfRange) {
        double loc_maxValue = ui->lineEditMaxValue->GetDoubleValue();
        if(loc_maxValue <= arg_value) {
            ui->lineEditMaxValue->SetValue(arg_value + 1.0);
            m_KnobOrTacho->setMax(arg_value + 1.0);
        }
        m_KnobOrTacho->setMin(arg_value);
        m_KnobOrTacho->update();
    }
}

void KnobDialogFrame::maxValueChanged(double arg_value, bool par_OutOfRange)
{
    if (!par_OutOfRange) {
        double loc_minValue = ui->lineEditMinValue->GetDoubleValue();
        if(loc_minValue >= arg_value) {
            ui->lineEditMinValue->SetValue(arg_value - 1.0);
            m_KnobOrTacho->setMin(arg_value - 1.0);
        }
        m_KnobOrTacho->setMax(arg_value);
        m_KnobOrTacho->update();
    }
}

void KnobDialogFrame::RawPhysicalChanged(int par_RawPhysical)
{
    m_KnobOrTacho->setPhys(par_RawPhysical);
}

void KnobDialogFrame::DirectConfigChanged(bool arg_value)
{
    m_KnobOrTacho->setUseBlackboardConfig(arg_value);
    variableDirectConfig(arg_value);
}

void KnobDialogFrame::InheritChanged(QString par_Value)
{
    MdiWindowType *Type = m_KnobOrTacho->GetMdiWindowType();
    QList<MdiWindowWidget*> ListOfWidget =Type->GetAllOpenWidgetsOfThisType();
    foreach(MdiWindowWidget* Widget, ListOfWidget) {
        if (Widget != m_KnobOrTacho) {
            if (par_Value.compare(Widget->GetWindowTitle()) == 0) {
                KnobOrTachoWidget *CopyFrom = static_cast<KnobOrTachoWidget*>(Widget);
                m_KnobOrTacho->CopyAttributesFrom(CopyFrom);
                Init();
            }
        }
    }

}

// Scale

void KnobDialogFrame::ScaleVisible(bool arg_value)
{
    if(arg_value) {
        m_KnobOrTacho->setScaleFlag(1);
    } else {
        m_KnobOrTacho->setScaleFlag(0);
    }
    m_KnobOrTacho->update();
}

void KnobDialogFrame::minAngleChanged(double arg_value)
{
    if(ui->ScaleMaxAngleSpinBox->value() <= arg_value) {
        ui->ScaleMaxAngleSpinBox->setValue(arg_value + ui->ScaleStepDoubleSpinBox->value());
    }
    m_KnobOrTacho->setMinAngle(arg_value);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::maxAngleChanged(double arg_value)
{
    if(ui->ScaleMinAngleSpinBox->value() >= arg_value) {
        ui->ScaleMinAngleSpinBox->setValue(arg_value - ui->ScaleStepDoubleSpinBox->value());
    }
    m_KnobOrTacho->setMaxAngle(arg_value);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::ScaleStepFlagChanged(bool arg_StepFlag)
{
    ui->ScaleStepDoubleSpinBox->setEnabled(arg_StepFlag);
    m_KnobOrTacho->setScaleStepFlag(arg_StepFlag);
    m_KnobOrTacho->update();

}

void KnobDialogFrame::ScaleStepChanged(double arg_value)
{
    bool loc_isDouble = false;
    double loc_maxValue = ui->lineEditMaxValue->text().toDouble(&loc_isDouble);
    Q_UNUSED(loc_maxValue);
    if(loc_isDouble) {
        loc_isDouble = false;
        double loc_minValue = ui->lineEditMinValue->text().toDouble(&loc_isDouble);
        Q_UNUSED(loc_minValue);
        if(loc_isDouble) {
            if(!CompareDoubleEqual_int(arg_value, 0.0)) {
                m_KnobOrTacho->setScaleStep(arg_value);
                m_KnobOrTacho->update();
            }
        }
    }
}

void KnobDialogFrame::ScalePositionXChanged(double arg_value)
{
    m_KnobOrTacho->setScalePosX(arg_value);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::ScalePositionYChanged(double arg_value)
{
    m_KnobOrTacho->setScalePosY(arg_value);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::ScaleSizeFlagChanged(bool par_SizeFlag)
{
    ui->ScaleSizeDoubleSpinBox->setEnabled(par_SizeFlag);
    m_KnobOrTacho->setScaleSizeFlag(par_SizeFlag);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::ScaleSizeChanged(double arg_changed)
{
    m_KnobOrTacho->setScaleSize(arg_changed);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::ScaleColorFlagChanged(bool par_ColorFlag)
{
    ui->ScaleColorPushButton->setEnabled(par_ColorFlag);
    m_KnobOrTacho->setScaleColorFlag(par_ColorFlag);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::ScaleColorChoose()
{
    QColor Color = m_KnobOrTacho->getScaleColor();
    if (OpenColorDialog(&Color)) {
        ChangeColorButton(ui->ScaleColorPushButton, Color);
        m_KnobOrTacho->setScaleColor(Color);
        m_KnobOrTacho->update();
    }
}

// Knob

void KnobDialogFrame::neeldeVisible(bool arg_value)
{
    if(arg_value) {
        m_KnobOrTacho->setNeedleFlag(1);
    } else {
        m_KnobOrTacho->setNeedleFlag(0);
    }
    m_KnobOrTacho->update();
}

void KnobDialogFrame::needlePositionXChanged(double arg_value)
{
    m_KnobOrTacho->setNeedlePosX(arg_value);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::needlePositionYChanged(double arg_value)
{
    m_KnobOrTacho->setNeedlePosY(arg_value);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::needleSizeFlagChanged(bool par_SizeFlag)
{
    ui->needleSizeDoubleSpinBox->setEnabled(par_SizeFlag);
    m_KnobOrTacho->setNeedleSizeFlag(par_SizeFlag);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::needleSizeChanged(double arg_changed)
{
    m_KnobOrTacho->setNeedleLength(arg_changed);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::needleLineColorFlagChanged(bool par_ColorFlag)
{
    ui->needleLineColorPushButton->setEnabled(par_ColorFlag);
    m_KnobOrTacho->setNeedleLineColorFlag(par_ColorFlag);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::needleLineColorChoose()
{
    QColor Color = m_KnobOrTacho->getNeedleLineColor();
    if (OpenColorDialog(&Color)) {
        ChangeColorButton(ui->needleLineColorPushButton, Color);
        m_KnobOrTacho->setNeedleLineColor(Color);
        m_KnobOrTacho->update();
    }
}

void KnobDialogFrame::needleFillColorFlagChanged(bool par_ColorFlag)
{
    ui->needleFillColorPushButton->setEnabled(par_ColorFlag);
    m_KnobOrTacho->setNeedleFillColorFlag(par_ColorFlag);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::needleFillColorChoose()
{
    QColor Color = m_KnobOrTacho->getNeedleFillColor();
    if (OpenColorDialog(&Color)) {
        ChangeColorButton(ui->needleFillColorPushButton, Color);
        m_KnobOrTacho->setNeedleFillColor(Color);
        m_KnobOrTacho->update();
    }
}

// Value

void KnobDialogFrame::textVisible(bool arg_value)
{
    if(arg_value) {
        m_KnobOrTacho->setTextFlag(1);
    } else {
        m_KnobOrTacho->setTextFlag(0);
    }
    m_KnobOrTacho->update();
}

void KnobDialogFrame::textPositionXChanged(double arg_value)
{
    m_KnobOrTacho->setTextPosX(arg_value);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::textPositionYChanged(double arg_value)
{
    m_KnobOrTacho->setTextPosY(arg_value);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::textFontFlagChanged(bool par_FontFlag)
{
    ui->textFontPushButton->setEnabled(par_FontFlag);
    m_KnobOrTacho->setTextFontFlag(par_FontFlag);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::textFontChoose()
{
    QFont Font = m_KnobOrTacho->getTextFont();
    if (OpenFontDialog(&Font)) {
        m_KnobOrTacho->setTextFont(Font);
        m_KnobOrTacho->update();
    }
}

void KnobDialogFrame::textFontAlignChanged(int par_Align)
{
    bool DisablePos = (par_Align != 3);
    ui->textPosXDoubleSpinBox->setHidden(DisablePos);
    ui->textPosXLabel->setHidden(DisablePos);
    ui->textPosYDoubleSpinBox->setHidden(DisablePos);
    ui->textPosYLabel->setHidden(DisablePos);
    m_KnobOrTacho->SetTextAlign(par_Align);
    m_KnobOrTacho->update();
}

// Name

void KnobDialogFrame::nameVisible(bool arg_value)
{
    if(arg_value) {
        m_KnobOrTacho->setNameFlag(1);
    } else {
        m_KnobOrTacho->setNameFlag(0);
    }
    m_KnobOrTacho->update();
}

void KnobDialogFrame::namePositionXChanged(double arg_value)
{
    m_KnobOrTacho->setNamePosX(arg_value);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::namePositionYChanged(double arg_value)
{
    m_KnobOrTacho->setNamePosY(arg_value);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::nameFontFlagChanged(bool par_FontFlag)
{
    ui->nameFontPushButton->setEnabled(par_FontFlag);
    m_KnobOrTacho->setNameFontFlag(par_FontFlag);
    m_KnobOrTacho->update();
}

void KnobDialogFrame::nameFontChoose()
{
    QFont Font = m_KnobOrTacho->getNameFont();
    if (OpenFontDialog(&Font)) {
        m_KnobOrTacho->setNameFont(Font);
        m_KnobOrTacho->update();
    }
}

void KnobDialogFrame::nameFontAlignChanged(int par_Align)
{
    HideNamePos(par_Align != 3);
    m_KnobOrTacho->SetNameAlign(par_Align);
    m_KnobOrTacho->update();
}

// Bitmap

void KnobDialogFrame::bitmapVisible(bool arg_value)
{
    if(arg_value) {
        m_KnobOrTacho->setBitmapFlag(1);
    } else {
        m_KnobOrTacho->setBitmapFlag(0);
    }
    m_KnobOrTacho->update();
}

void KnobDialogFrame::variableDirectConfig(bool arg_value)
{
    ui->lineEditMinValue->setEnabled(!arg_value);
    ui->lineEditMaxValue->setEnabled(!arg_value);
}

void KnobDialogFrame::HideNamePos(bool par_Hide)
{
    ui->namePosXDoubleSpinBox->setHidden(par_Hide);
    ui->namePosXLabel->setHidden(par_Hide);
    ui->namePosYDoubleSpinBox->setHidden(par_Hide);
    ui->namePosYLabel->setHidden(par_Hide);
}

void KnobDialogFrame::HideTextPos(bool par_Hide)
{
    ui->textPosXDoubleSpinBox->setHidden(par_Hide);
    ui->textPosXLabel->setHidden(par_Hide);
    ui->textPosYDoubleSpinBox->setHidden(par_Hide);
    ui->textPosYLabel->setHidden(par_Hide);
}

void KnobDialogFrame::openBitmapPathDialog()
{
    QString FileName = FileDialog::getOpenFileName(this, "open bitmap", ui->bitmapLineEdit->text(), IMAGE_EXT);
    if (!FileName.isEmpty()) {
        ui->bitmapLineEdit->setText(FileName);
        m_KnobOrTacho->setBitmapPath(FileName);
        m_KnobOrTacho->update();
    }
}

void KnobDialogFrame::scaleBitmap(int par_Option)
{
    m_KnobOrTacho->setBitmapSizeBehaviour(static_cast<enum KnobOrTachoWidget::BitmapWindowSizeBehaviour>(par_Option));
}

void KnobDialogFrame::IfReleaseFallbackToFlagChanged(bool par_Flag)
{
    m_KnobOrTacho->setIfReleaseFallbackToFlag(par_Flag);
}

void KnobDialogFrame::IfReleaseFallbackToValueChanged(double par_Value, bool par_OutOfRange)
{
    if (!par_OutOfRange) {
        m_KnobOrTacho->setIfReleaseFallbackToValue(par_Value);
    }
}

void KnobDialogFrame::DragStatusFlagChanged(bool arg_Flag)
{
    m_KnobOrTacho->setDragStatusFlag(arg_Flag);
}

void KnobDialogFrame::DragStatusSignalChanged(QString arg_Name)
{
    m_KnobOrTacho->setDragStatusSignal(arg_Name);
}

void KnobDialogFrame::ChangeColorButton(QPushButton *par_Button, QColor par_Color)
{
    QSize Size = par_Button->iconSize();
    QPixmap Pixmap = QPixmap(Size);
    Pixmap.fill(par_Color);
    QIcon Icon(Pixmap);
    par_Button->setIcon(Icon);
}

bool KnobDialogFrame::OpenColorDialog(QColor *ptr_Color)
{
    QColorDialog ColorDialog(*ptr_Color, this);
    if(ColorDialog.exec() == QDialog::Accepted) {
        *ptr_Color = ColorDialog.selectedColor();
        return true;
    } else {
        return false;
    }
}

bool KnobDialogFrame::OpenFontDialog(QFont *ptr_Font)
{
    QFontDialog FontDialog(*ptr_Font, this);
    if(FontDialog.exec() == QDialog::Accepted) {
        *ptr_Font = FontDialog.selectedFont();
        return true;
    } else {
        return false;
    }
}

void KnobDialogFrame::FillInheritFromComboBox()
{
    MdiWindowType *Type = m_KnobOrTacho->GetMdiWindowType();
    QList<MdiWindowWidget*> ListOfWidget =Type->GetAllOpenWidgetsOfThisType();
    ui->inheritComboBox->addItem(QString(""));  // empty item for no inherit
    foreach(MdiWindowWidget* Widget, ListOfWidget) {
        if (Widget != m_KnobOrTacho) {
            ui->inheritComboBox->addItem(Widget->GetWindowTitle());
        }
    }
}

void KnobDialogFrame::FillAlignComboBox(QComboBox *par_ComboBox)
{
    par_ComboBox->addItem("center");   // 0
    par_ComboBox->addItem("left");     // 1
    par_ComboBox->addItem("right");    // 2
    par_ComboBox->addItem("inside");   // 3
}
