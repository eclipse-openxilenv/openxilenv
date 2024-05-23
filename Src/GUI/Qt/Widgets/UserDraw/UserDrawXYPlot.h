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


#ifndef USERDRAWXYPLOT_H
#define USERDRAWXYPLOT_H

#include <QString>
#include <QList>
#include "UserDrawElement.h"
#include "OscilloscopeData.h"
extern "C" {

}

class UserDrawXYPlot : public UserDrawElement {
public:
    UserDrawXYPlot(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent = nullptr);
    ~UserDrawXYPlot();
    void ResetToDefault() Q_DECL_OVERRIDE;
    void SetDefaultParameterString() Q_DECL_OVERRIDE;
    bool ParseOneParameter(QString& par_Value) Q_DECL_OVERRIDE;
    void Paint (QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw) Q_DECL_OVERRIDE;
    bool Init() Q_DECL_OVERRIDE;
    QString GetTypeString() Q_DECL_OVERRIDE;
    static int Is(QString &par_Line);
private:
    void ParseOneSignal(QString &par_Signal);

    bool InitDataStruct();

    bool m_ClearFlag;

    int m_BufferDepth;

    int m_SignalCount;
    QStringList m_XYIns;
    QList<bool> m_XYPhys;
    QList<double> m_XYMin;
    QList<double> m_XYMax;
    QList<int> m_Color;
    QList<int> m_LineOrPoints;
    QList<int> m_Width;

    OSCILLOSCOPE_DATA m_Data;
};


#endif // USERDRAWXYPLOT_H
