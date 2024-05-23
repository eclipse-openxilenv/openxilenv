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


#ifndef VALUEINPUT_H
#define VALUEINPUT_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QRegularExpressionValidator>

extern "C" {
#include "Blackboard.h"
}

class MyLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit MyLineEdit(QWidget* parent=nullptr);
    explicit MyLineEdit(const QString &name, QWidget* parent=nullptr);
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

class ValueInput : public QWidget
{
    Q_OBJECT
public:
    enum StepType {LINEAR_STEP=0, PERCENTAGE_STEP=1};

    ValueInput(QWidget* parent=nullptr);
    ~ValueInput()  Q_DECL_OVERRIDE;

    void SetValue (int par_Value, int par_Base = 10);
    void SetValue (unsigned int par_Value, int par_Base = 10);
    void SetValue (int64_t par_Value, int par_Base = 10);
    void SetValue (uint64_t par_Value, int par_Base = 10);
    void SetValue (double par_Value);
    void SetValue (union FloatOrInt64 par_Value, int par_Type);
    void SetMinMaxValue (double par_MinValue, double par_MaxValue);
    void SetMinMaxValue (union BB_VARI par_MinValue, union BB_VARI par_MaxValue, enum BB_DATA_TYPES par_Type);
    double GetDoubleValue(bool *ret_Ok = nullptr);
    int GetIntValue(bool *ret_Ok = nullptr);
    unsigned int GetUIntValue(bool *ret_Ok = nullptr);
    long long GetInt64Value(bool *ret_Ok = nullptr);
    unsigned long long GetUInt64Value(bool *ret_Ok = nullptr);
    enum BB_DATA_TYPES GetUnionValue(union BB_VARI *ret_Value, bool *ret_Ok);
    int GetFloatOrInt64Value(union FloatOrInt64 *ret_Value, bool *ret_Ok);
    void EnableAutoScaling();
    void DisableAutoScaling();

    bool GetConfigButtonEnable();
    void SetConfigButtonEnable(bool par_State);
    bool GetPlusMinusButtonEnable();
    void SetPlusMinusButtonEnable(bool par_State);

    double GetPlusMinusIncrement();
    void SetPlusMinusIncrement(double par_Increment);
    void SetStepLinear(bool par_Linear);
    void SetStepPercentage(bool par_Percentage);
    void SetStepType(enum StepType par_StepType);

    void SetOnlyInteger(bool par_OnlyInteger);
    void SetHeader(QString par_Header);

    QSize sizeHint() const  Q_DECL_OVERRIDE;

    void setFocus(Qt::FocusReason reason);

    bool hasFocus() const;

    QString text();
    void setText(QString &par_String);

    void SetWideningSignalEnable(bool par_Enable);

    void SetValueInvalid(bool par_Invalid);
    void CheckResizeSignalShouldSent();

public slots:
    inline void setFocus() { setFocus(Qt::OtherFocusReason); }

signals:
   void ValueChanged(double par_Value, bool par_OutOfRange);
   void editingFinished();
   void ShouldWideningSignal(int Widening, int Heightening);

protected:
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;

private:
    void ChangeStructureElem();
    void ChangeStructurePos(QSize Size);
    void SetIntValue (double par_Value);
    bool MinMaxRangeCheck(union BB_VARI par_In, enum BB_DATA_TYPES par_Type, union BB_VARI *ret_Out);
    int BaseOfText();
    void AddOffset(double par_Offset);

    QLabel *m_Header;
    MyLineEdit *m_LineEdit;
    QPushButton *m_ButtonPlus;
    QPushButton *m_ButtonMinus;
    QPushButton *m_ButtonConfig;
    QRegularExpressionValidator m_Validator;
    bool m_MinMaxCheck;
    union BB_VARI m_MinValue;
    union BB_VARI m_MaxValue;
    enum BB_DATA_TYPES m_Type;
    bool m_OutOfRange;
    bool m_AutoScalingActive;
    bool m_OnlyInteger;
    bool m_WideningSignal;

    int m_WinHeight;
    int m_FontSize;

    QPalette m_PaletteInvalidValueRange;
    QPalette m_PaletteSave;

    bool m_WithPlusMinus;
    bool m_WithConfig;

    double m_PlusMinusIncrement;
    enum StepType m_StepType;

private slots:
    void CheckValueRangesSlot (const QString &par_Text);
    void IncrementValue();
    void DecrementValue();
    void editingFinishedSlot();
};

#endif // TEXTVALUEINPUT_H
