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


#ifndef A2LCALSINGLEDATA_H
#define A2LCALSINGLEDATA_H

#include <QList>
#include <QString>
#include <QVariant>
#include <QFontMetrics>

extern "C" {
    #include "A2LAccessData.h"
    #include "Blackboard.h"
    #include "DebugInfos.h"
    #include "A2LLink.h"
//#include "A2LLinkThread.h"
}

class A2LCalSingleData
{
public:
    A2LCalSingleData();
    ~A2LCalSingleData();

    void Reset(bool Free = false);

    void SetProcess(QString &par_ProcessName);
    QString GetProcess();
    void ProcessTerminated();
    void ProcessStarted();
    QString GetCharacteristicName();
    void SetCharacteristicName(QString &par_Characteristic);
    int GetIndex();
    A2L_DATA *GetData();
    void SetData(A2L_DATA *par_Data);
    int GetLinkNo();
    int GetNamePixelWidth();
    void CalcNamePixelWidth(QFontMetrics *par_FontMetrics);
    int GetValuePixelWidth();
    void CalcValuePixelWidth(QFontMetrics *par_FontMetrics);
    int GetUnitPixelWidth();
    void CalcUnitPixelWidth(QFontMetrics *par_FontMetrics);
    bool CheckExistance(int par_LinkNo);

    int GetType();
    bool IsPhysical();
    void SetPhysical(bool par_Physical);
    int GetDataType();
    const char *GetMapUnit();
    const char *GetMapConversion();

    A2L_SINGLE_VALUE *GetValue();

    double GetRawValue();
    double GetDoubleValue();
    int ToString(bool par_UpdateAlways);
    QString GetValueString();

    QString ConvertValueToString(A2L_SINGLE_VALUE *par_Value);

    int ChangeValueInsideExternProcess (int op,
                                        const QVariant par_Value, int par_GetDataChannelNo, int par_Row,
                                        bool par_Readback = false);

    double GetMaxMinusMin ();

    void CalcRawMinMaxValues();

    //int Update();

    void SetToNotExiting();
    bool Exists();
    QString GetUnit();
    QString GetValueStr();
    int GetDisplayType();
    bool SetValue(const QVariant &par_Value, int par_GetDataChannelNo, int par_Row);
    void SetValueStr(QString par_String);
    void SetDisplayType(int par_DisplayType);
    bool GetMinMax(double *ret_Min, double *ret_Max);
    int GetAddress(uint64_t *ret_Address);

    CHARACTERISTIC_AXIS_INFO *GetInfos();

    double GetDecIncOffset();
    void SetDecIncOffset(double par_DecIncOffset);

    bool NotifyCheckDataChanged(int par_LinkNo, int par_Index, A2L_DATA *par_Data);
private:
    void Clear();

    double Conv (double par_Value, CHARACTERISTIC_AXIS_INFO &par_AxisInfo);
    QString ConvertToString(double Value, CHARACTERISTIC_AXIS_INFO &AxisInfo);

    bool m_Exists;
    bool m_HasChanged;
    int m_DisplayType;

    QString m_ProcessName;
    QString m_CharacteristicName;
    int m_LinkNo;
    int m_Index;
    QString m_ValueStr;

    int m_NamePixelWidth;
    int m_ValuePixelWidth;
    int m_UnitPixelWidth;

    double m_DecIncOffset;

    bool m_PhysicalFlag;

    void SetValue (A2L_SINGLE_VALUE *par_Value, union BB_VARI par_ToValue, enum BB_DATA_TYPES par_Type);
    //double MutiplyButMinumumIncByOne (double Value, int Type, double Factor);
    //double DivideButMinumumDecByOne (double Value, int Type, double Divider);
    double AddButMinumumIncOrDecByOne (double Value, int Type, double Add);

    A2L_DATA *m_Data;

    CHARACTERISTIC_AXIS_INFO m_Info;

    void ResetAxisInfo(CHARACTERISTIC_AXIS_INFO *AxisInfo, bool Free = false);
};

#endif // A2LCALSINGLEDATA_H
