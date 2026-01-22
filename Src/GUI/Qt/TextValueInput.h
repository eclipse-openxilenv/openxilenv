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


#ifndef TEXTVALUEINPUT_H
#define TEXTVALUEINPUT_H

#include <QLineEdit>
#include <QRegularExpressionValidator>

extern "C" {
#include "Blackboard.h"
}


class TextValueInput : public QLineEdit
{
    Q_OBJECT
public:
    TextValueInput(QWidget* parent=nullptr);
    ~TextValueInput()  Q_DECL_OVERRIDE;

    void SetValue (int par_Value, int par_Base = 10);
    void SetValue (unsigned int par_Value, int par_Base = 10);
    void SetValue (uint64_t par_Value, int par_Base = 10);
    void SetValue (int64_t par_Value, int par_Base = 10);
    void SetValue (double par_Value);
    void SetValue (union FloatOrInt64 par_Value, int par_Type);
    void SetMinMaxValue (union BB_VARI par_MinValue, union BB_VARI par_MaxValue, enum BB_DATA_TYPES par_Type);
    void SetMinMaxValue (double par_MinValue, double par_MaxValue);
    double GetDoubleValue(bool *ret_Ok = nullptr);
    int GetIntValue(bool *ret_Ok = nullptr);
    unsigned int GetUIntValue(bool *ret_Ok = nullptr);
    uint64_t GetUInt64Value(bool *ret_Ok = nullptr);
    enum BB_DATA_TYPES GetUnionValue(union BB_VARI *ret_Value, bool *ret_Ok = nullptr);
    int GetFloatOrInt64Value(union FloatOrInt64 *ret_Value, bool *ret_Ok = nullptr);
    void EnableAutoScaling();
    void DisableAutoScaling();

    QSize sizeHint() const Q_DECL_OVERRIDE;

protected:
    void resizeEvent(QResizeEvent * /* event */) Q_DECL_OVERRIDE;

private:
    QRegularExpressionValidator m_Validator;
    bool m_MinMaxCheck;
    union BB_VARI m_MinValue;
    union BB_VARI m_MaxValue;
    enum BB_DATA_TYPES m_Type;
    bool m_OutOfRange;
    bool m_AutoScalingActive;

    int m_WinHeight;
    int m_FontSize;

    QPalette m_PaletteInvalidValueRange;
    QPalette m_PaletteSave;

private slots:
    void CheckValueRangesSlot (const QString &par_Text);

};

#endif // TEXTVALUEINPUT_H
