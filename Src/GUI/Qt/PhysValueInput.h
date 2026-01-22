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


#ifndef PHYSVALUEINPUT_H
#define PHYSVALUEINPUT_H

#include <QWidget>
#include <QComboBox>

#include "ValueInput.h"

extern "C" {
#include "Blackboard.h"
}

class EnumComboBoxModelItem
{
public:
    QString m_Enum;
    int64_t From;
    int64_t To;
    int32_t Color;
    bool Valid;
};

class EnumComboBoxModel : public QAbstractListModel
{
   Q_OBJECT
public:
   EnumComboBoxModel(char *par_EnumString, QObject *parent = nullptr);

   int rowCount(const QModelIndex &parent = QModelIndex()) const;
   QVariant data(const QModelIndex &index, int role) const;

   int GetFromValue(int Index);
   int GetToValue(int Index);
   QString GetEnumString(int Index);
   bool IsValid(int Index);
   int GetMaxEnumWidth(QFontMetrics par_FontMetrics);
private:
   QList<EnumComboBoxModelItem> m_EnumList;

};


class EComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit EComboBox(QWidget *parent = nullptr);
signals:
    void editingFinished();
protected:
    bool eventFilter(QObject *obj, QEvent *event);
};

class EnumComboBox : public QWidget
{
    Q_OBJECT
public:
    explicit EnumComboBox(QWidget *parent = nullptr);
    void EnableAutoScaling();
    void DisableAutoScaling();
    void SetValue(int par_Value);
    int GetFromValue(bool *ret_Ok);

    void SetHeader(QString par_Header);

    void setCurrentText(const QString &par_EnumValue);
    QString currentText();
    void setModel(EnumComboBoxModel *par_Model);
    EnumComboBoxModel *model();

    bool hasFocus() const;
    void SetWideningSignalEnable(bool par_Enable);
    void CheckResizeSignalShouldSent();

signals:
    void currentIndexChanged(int Index);
    void editingFinished();
    void ShouldWideningSignal(int Widening, int Heightening);

private slots:
    void currentIndexChangedSlot(int Index);
    void editingFinishedSlot();

protected:
    void resizeEvent(QResizeEvent * event) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;

private:
    void ChangeStructurePos(QSize Size);
    bool m_AutoScalingActive;
    EComboBox *m_ComboBox;
    QLabel *m_Header;
    bool m_WideningSignal;
};

class PhysValueInput : public QWidget
{
    Q_OBJECT
public:
    enum DisplayTypeRawValue {DISPLAY_RAW_VALUE_DECIMAL, DISPLAY_RAW_VALUE_HEXAMAL, DISPLAY_RAW_VALUE_BINARY};
    explicit PhysValueInput(QWidget *parent = nullptr, int par_Vid = -1, bool par_Raw = true, bool par_Phys = false, enum DisplayTypeRawValue par_DisplayRawValue = DISPLAY_RAW_VALUE_DECIMAL);
    ~PhysValueInput() Q_DECL_OVERRIDE;

    int DispayTypeToBase (enum DisplayTypeRawValue par_DisplayType);

    void SetDisplayPhysValue(bool par_State);
    void SetDisplayRawValue(bool par_State);

    void SetBlackboardVariableId(int par_Vid, bool par_ChangeRawPhys = false);
    void SetBlackboardVariableName(char *par_VariableName);

    void SetConersionTypeAndString(enum BB_CONV_TYPES par_ConvType, const char *par_ConvString);
    void SetFormulaString(const char *par_Formula);
    void SetEnumString(const char *par_EnumString);

    void SetRawValue(int par_Value, int par_Base = 10);
    void SetRawValue(unsigned int par_Value, int par_Base = 10);
    void SetRawValue(uint64_t par_Value, int par_Base = 10);
    void SetRawValue(int64_t par_Value, int par_Base = 10);
    void SetRawValue(double par_Value);
    void SetRawValue(union BB_VARI par_Value, enum BB_DATA_TYPES par_Type, int par_Base = 10, bool par_ChangeDataType = true);

    void SetPhysValue(double par_PhysValue);
    void SetEnumValue(char *par_EnumValue);

    void SetRawMinMaxValue (double par_MinValue, double par_MaxValue);
    void SetRawOnlyInt (bool par_OnlyInt);

    double GetDoubleRawValue(bool *ret_Ok = nullptr);
    int GetIntRawValue(bool *ret_Ok = nullptr);
    unsigned int GetUIntRawValue(bool *ret_Ok = nullptr);
    unsigned long long GetUInt64RawValue(bool *ret_Ok = nullptr);
    long long GetInt64RawValue(bool *ret_Ok = nullptr);
    enum BB_DATA_TYPES GetUnionRawValue(union BB_VARI *ret_Value, bool *ret_Ok = nullptr);

    double GetDoublePhysValue(bool *ret_Ok);
    QString GetStringEnumValue(void);

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

    void SetDispayFormat(int par_Base);   // Base == 2, 10, 16

    void SetHeaderState(bool pat_State);

    QSize sizeHint() const Q_DECL_OVERRIDE;
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    void setFocus(Qt::FocusReason reason);

    bool hasFocus() const;

    void SetWideningSignalEnable(bool par_Enable);

public slots:
    inline void setFocus() { setFocus(Qt::OtherFocusReason); }

signals:
    void ValueChanged(); //(double par_Value, bool par_OutOfRange);
    void editingFinished();
    void LeaveEditor();
    void ShouldWideningSignal(int Widening, int Heightening);
    
private slots:
    void PhysValueChanged(double par_Value, bool par_OutOfRange);
    void RawValueChanged(double par_Value, bool par_OutOfRange);
    void EmunValueChanged(int Index);
    void editingFinishedSlot();
    void ShouldWideningSlot(int Widening, int Highing);

protected:
    void resizeEvent(QResizeEvent * event) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;

private:
    void ChangeStructureElem();
    void ChangeStructurePos(QSize Size);
    double ConvertRawToPhys(double par_RawValue, bool *ret_Ok = nullptr);
    double ConvertPhysToRaw(double par_PhysValue, double *ret_PhysValue, bool *ret_Ok = nullptr);

    void FillComboBox();

    QString DoubleToString(double par_Value);

    bool m_AutoScalingActive;

    bool m_ShowRaw;
    bool m_ShowPhys;
    bool m_SwapValueBetweeRaWPhys;
    int m_AssignedVid;

    ValueInput *m_RowValue;
    ValueInput *m_PhysValue;
    EnumComboBox *m_EnumValue;
    EnumComboBoxModel *m_Model;

    int m_WinHeight;
    int m_FontSize;

    bool m_WithPlusMinus;
    bool m_WithConfig;

    double m_PlusMinusIncrement;
    enum ValueInput::StepType m_StepType;

    bool m_RawPhysValueChangedPingPong;

    int m_Vid;
    char *m_BlackboardVariableName;

    enum BB_CONV_TYPES m_ConvType;
    char *m_ConvString;

    bool m_RawOnlyInt;

    bool m_Header;

    enum DataType {DATA_TYPE_INT, DATA_TYPE_UINT, DATA_TYPE_DOUBLE} m_DataType;
    int64_t m_RawIntValue;
    uint64_t m_RawUIntValue;
    double m_RawDoubleValue;

    enum DisplayTypeRawValue m_DisplayRawValue;

    bool m_WideningSignal;
};

#endif // PHYSVALUEINPUT_H
