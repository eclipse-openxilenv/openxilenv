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


#ifndef KNOBDIALOGFRAME_H
#define KNOBDIALOGFRAME_H

#include <QComboBox>
#include <QButtonGroup>

#include "DialogFrame.h"
#include "KnobOrTachoWidget.h"

namespace Ui {
class KnobDialogFrame;
}

class DefaultElementDialog;

class KnobDialogFrame : public CustomDialogFrame
{
    Q_OBJECT

public:
    explicit KnobDialogFrame(KnobOrTachoWidget* arg_tacho, QWidget *arg_parent = nullptr);
    ~KnobDialogFrame();
    void setBBConfigDirect(bool arg_value);
    void setVaraiblePhysical(bool arg_value);
    void setBitmapPath(QString arg_path);
    void setScaleActive(bool arg_value);
    void setNeedleActive(bool arg_value);
    void setTextActive(bool arg_value);
    void setBitmapActive(bool arg_value);
    void setTextforknoborslider(QString arg_text);
    void userAccept();
    void userReject();

public slots:
    void changeVaraible(QString arg_variable, bool arg_checked);
    void ColorChanged(QColor par_Color);
    void minValueCahnged(double arg_value, bool par_OutOfRange);
    void maxValueChanged(double arg_value, bool par_OutOfRange);
    void RawPhysicalChanged(int par_RawPhysical);
    void DirectConfigChanged(bool arg_value);
    void InheritChanged(QString par_Value);
    // Scale
    void ScaleVisible(bool arg_value);
    void minAngleChanged(double arg_value);
    void maxAngleChanged(double arg_value);
    void ScaleStepFlagChanged(bool arg_StepFlag);
    void ScaleStepChanged(double arg_value);
    void ScalePositionXChanged(double arg_value);
    void ScalePositionYChanged(double arg_value);
    void ScaleSizeFlagChanged(bool par_SizeFlag);
    void ScaleSizeChanged(double arg_changed);
    void ScaleColorFlagChanged(bool par_ColorFlag);
    void ScaleColorChoose();
    // Knob
    void neeldeVisible(bool arg_value);
    void needlePositionXChanged(double arg_value);
    void needlePositionYChanged(double arg_value);
    void needleSizeFlagChanged(bool par_SizeFlag);
    void needleSizeChanged(double arg_changed);
    void needleLineColorFlagChanged(bool par_ColorFlag);
    void needleLineColorChoose();
    void needleFillColorFlagChanged(bool par_ColorFlag);
    void needleFillColorChoose();
    // Value
    void textVisible(bool arg_value);
    void textPositionXChanged(double arg_value);
    void textPositionYChanged(double arg_value);
    void textFontFlagChanged(bool par_FontFlag);
    void textFontChoose();
    void textFontAlignChanged(int par_Align);
    // Name
    void nameVisible(bool arg_value);
    void namePositionXChanged(double arg_value);
    void namePositionYChanged(double arg_value);
    void nameFontFlagChanged(bool par_FontFlag);
    void nameFontChoose();
    void nameFontAlignChanged(int par_Align);
    // Bitmap
    void bitmapVisible(bool arg_value);
    void openBitmapPathDialog();
    void scaleBitmap(int par_Option);
    // If release fallback to
    void IfReleaseFallbackToFlagChanged(bool par_Flag);
    void IfReleaseFallbackToValueChanged(double pat_Value, bool par_OutOfRange);
    // Drag status
    void DragStatusFlagChanged(bool arg_Flag);
    void DragStatusSignalChanged(QString arg_Name);

signals:
    //void setVariableDirectConfig(bool arg_value);
    //void setVariablePhysical(bool arg_value);
    //void setBitmapVisible(bool arg_value);
    //void bitmapPathChanged(QString arg_path);

private:
    void Init();
    void ChangeColorButton(QPushButton *par_Button, QColor par_Color);
    bool OpenColorDialog(QColor *ptr_Color);
    bool OpenFontDialog(QFont *ptr_Font);
    void FillInheritFromComboBox();
    void FillAlignComboBox(QComboBox *par_ComboBox);
    void variableDirectConfig(bool arg_value);

    Ui::KnobDialogFrame *ui;

    QButtonGroup *m_RawPhysButtonGroup;
    QButtonGroup *m_BitmapButtonGroup;
    DefaultElementDialog *m_DefaultElementDialog;
    KnobOrTachoWidget *m_KnobOrTacho;

    bool m_DragStatusFlagSave;
    QString m_DragStatusNameSave;
};

#endif // KNOBDIALOGFRAME_H
