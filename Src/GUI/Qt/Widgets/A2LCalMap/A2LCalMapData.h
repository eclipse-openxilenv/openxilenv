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


#ifndef A2LCALMAPDATA_H
#define A2LCALMAPDATA_H

#include <QList>
#include <QString>
#include <QVariant>

extern "C" {
    #include "A2LAccessData.h"
    #include "Blackboard.h"
    #include "DebugInfos.h"
    #include "A2LLink.h"
}

class A2LCalMapData
{
public:
    A2LCalMapData();
    ~A2LCalMapData();

    void Reset(bool Free = false);

    void SetProcess(QString &par_ProcessName, void *par_CallbaclParam);
    QString GetProcess();
    void ProcessTerminated();
    void ProcessStarted();
    QString GetCharacteristicName();
    void SetCharacteristicName(QString &par_Characteristic);

    int GetLinkNo();
    int GetIndex();
    int GetType();
    bool IsPhysical();
    void SetPhysical(bool par_Physical);
    int GetXDim();
    int GetYDim();
    enum ENUM_AXIS_TYPE {ENUM_AXIS_TYPE_X_AXIS = 0, ENUM_AXIS_TYPE_Y_AXIS = 1, ENUM_AXIS_TYPE_MAP = 2};
    int GetDataType(enum ENUM_AXIS_TYPE par_Axis, int par_No);
    const char *GetXInput();
    const char *GetXUnit();
    const char *GetXConversion();
    int GetXConversionType();
    const char *GetYInput();
    const char *GetYUnit();
    const char *GetYConversion();
    int GetYConversionType();
    const char *GetMapUnit();
    const char *GetMapConversion();
    int GetMapConversionType();

    int TranslateAxisToArrayNo(enum ENUM_AXIS_TYPE par_Axis);

    void SetNoRecursiveSetPickMapFlag(bool par_Flag);
    bool GetNoRecursiveSetPickMapFlag();

    A2L_SINGLE_VALUE *GetValue(int axis, int x, int y);

    double GetRawValue(int axis, int x, int y);
    double GetDoubleValue(int axis, int x, int y);
    double GetValueNorm(int axis, int x, int y);
    QString GetValueString(int axis, int x, int y);
    int GetXVid() { return m_XVid; }
    int GetYVid() { return m_YVid; }

    QString ConvertValueToString(A2L_SINGLE_VALUE *par_Value, int par_Precision);

    bool XAxisNotCalibratable();
    bool YAxisNotCalibratable();

    int ChangeValueOP (int op,
                       double par_Value,
                       enum ENUM_AXIS_TYPE par_Axis, int x, int y);

    int ChangeValueOP (int op,
                       QVariant par_Value,
                       enum ENUM_AXIS_TYPE par_Axis, int x, int y);

    int WriteDataToTargetLink(bool par_Readback);

    double MapAccess (double *ret_point_x, double *ret_point_y, double *ret_point_z);

    double GetMaxMinusMin (int axis);

    void AdjustMinMax();

    void CalcRawMinMaxValues();

    int UpdateReq();
    int UpdateAck(void *par_Data);

    bool IsValid();

    void GetLastPick(int *ret_XPick, int *ret_YPick);

    void SetPick(int par_X, int par_Y, int par_Op);

    bool IsPicked(int par_X, int par_Y);

    CHARACTERISTIC_AXIS_INFO *GetAxisInfos(int par_AxisNo);

    double GetDecIncOffset();
    void SetDecIncOffset(double par_DecIncOffset);

    int GetAddress(uint64_t *ret_Address);

private:
    void Clear();

    double Conv (double par_Value, CHARACTERISTIC_AXIS_INFO &par_AxisInfo);
    double ConvNorm (double par_Value, CHARACTERISTIC_AXIS_INFO &par_AxisInfo);
    QString ConvertToString(double Value, CHARACTERISTIC_AXIS_INFO &AxisInfo, bool par_PhysFlag);

    bool m_NoRecursiveSetPickMapFlag;  // verhindert dass die schon ausgewaehlen wieder geloescht werden

    QString m_ProcessName;
    QString m_CharacteristicName;
    int m_LinkNo;
    int m_GetDataChannelNo;
    int m_Index;

    char *m_PickMap;
    int m_XDimPickMap;
    int m_YDimPickMap;
    int m_XPickLast;
    int m_YPickLast;

    int m_XVid;
    int m_YVid;
    double m_DecIncOffset;

    bool m_PhysicalFlag;

    void SetValue (A2L_SINGLE_VALUE *par_Value, union BB_VARI par_ToValue, enum BB_DATA_TYPES par_Type);
    //double MutiplyButMinumumIncByOne (double Value, int Type, double Factor);
    //double DivideButMinumumDecByOne (double Value, int Type, double Divider);
    double AddButMinumumIncOrDecByOne (double Value, int Type, double Add);

    A2L_DATA *m_Data;

    CHARACTERISTIC_AXIS_INFO m_XAxisInfo;
    CHARACTERISTIC_AXIS_INFO m_YAxisInfo;
    CHARACTERISTIC_AXIS_INFO m_ZAxisInfo;

    void ResetAxisInfo(CHARACTERISTIC_AXIS_INFO *AxisInfo, bool Free = false);
};

#endif // A2LCALMAPDATA_H
