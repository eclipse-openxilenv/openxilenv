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


#ifndef USERDRAWBUTTON_H
#define USERDRAWBUTTON_H

#include <QString>
#include <QList>
#include "UserDrawElement.h"

class UserDrawButton : public UserDrawElement {
public:
    UserDrawButton(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent = nullptr);
    ~UserDrawButton();
    void ResetToDefault() Q_DECL_OVERRIDE;
    void SetDefaultParameterString() Q_DECL_OVERRIDE;
    bool ParseOneParameter(QString& par_Value) Q_DECL_OVERRIDE;
    void Paint (QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw) Q_DECL_OVERRIDE;
    bool MousePressEvent(QMouseEvent *event, DrawParameter &par_Draw) Q_DECL_OVERRIDE;
    bool Init() Q_DECL_OVERRIDE;
    QString GetTypeString() Q_DECL_OVERRIDE;
    static int Is(QString &par_Line);
private:
    //void PaintText(QPainter *par_Painter, double par_Hight, double par_Width, DrawParameter &par_Draw);
    BaseValue m_Width;
    BaseValue m_Height;
    QString m_Text;
    QString m_Script;

    enum { IMAGE_TYPE_NUMBER, IMAGE_TYPE_NAME, IMAGE_TYPE_FILE } m_ImageType;
    BaseValue m_ImageNr;
    int LastImageNr;
    QString m_ImageFileOrName;
    QImage *m_Image;
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


#endif // USERDRAWBUTTON_H
