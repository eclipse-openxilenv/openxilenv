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


#ifndef UserDrawCircle_H
#define UserDrawCircle_H

#include <QString>
#include <QList>
#include "UserDrawElement.h"

class UserDrawCircle : public UserDrawElement {
public:
    UserDrawCircle(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent = nullptr);
    ~UserDrawCircle();
    void ResetToDefault() Q_DECL_OVERRIDE;
    void SetDefaultParameterString() Q_DECL_OVERRIDE;
    bool ParseOneParameter(QString& par_Value) Q_DECL_OVERRIDE;
    void Paint(QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw) Q_DECL_OVERRIDE;
    bool Init() Q_DECL_OVERRIDE;
    QString GetTypeString() Q_DECL_OVERRIDE;
    static int Is(QString &par_Line);
private:
    BaseValue m_Radius;
    enum RotationType {RotationTypeNone, RotationTypeCenter, RotationTypePos} m_RotationType;
    BaseValue m_RotXPos;
    BaseValue m_RotYPos;
    enum CenterType {CenterTypeCenter,
                     CenterTypeLeftTop, CenterTypeLeftBottom,
                     CenterTypeRightTop, CenterTypeRightBottom,
                     CenterTypeCenterLeft, CenterTypeCenterRight, CenterTypeCenterBottom, CenterTypeCenterTop,
                     CenterTypePos} m_CenterType;
    BaseValue m_CenterXPos;
    BaseValue m_CenterYPos;
};


#endif // UserDrawCircle_H
