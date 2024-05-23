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


#ifndef OSZILLOSCOPEDIALOGFRAME_H
#define OSZILLOSCOPEDIALOGFRAME_H

#include <QFrame>
#include "DialogFrame.h"
#include "OscilloscopeData.h"

namespace Ui {
class OszilloscopeDialogFrame;
}

class DefaultElementDialog;

class OszilloscopeDialogFrame : public CustomDialogFrame
{
    Q_OBJECT

public:
    explicit OszilloscopeDialogFrame(OSCILLOSCOPE_DATA *arg_data, QWidget *parent = nullptr);
    ~OszilloscopeDialogFrame();
    void userAccept();
    void userReject();
    void SetUsedColors(QList<QColor> &par_UsedColors);

public slots:
    void changeVariable(QString arg_variable, bool arg_visible);
    void changeColor(QColor arg_color);

private:
    bool ChangePhysFlag(char NewPhysFlag, char NewDecHexBinFlag);
    void ResetCurrentBuffer();

    Ui::OszilloscopeDialogFrame *ui;
    OSCILLOSCOPE_DATA *m_data;
    QString m_currentVariableName;
    QColor m_currentColor;
    QList<QColor> m_UsedColors;
    int m_currentVid;
    bool m_currentTextAlignment;
    bool m_currentDecPhysFlag;
    char m_currentDisplayDecHexBin;
    bool m_currentDisable;
    int m_currentRgbColor;
    double m_currentMinValue;
    double m_currentMaxValue;
    int m_currentLineWidth;
    char m_presentation;

    DefaultElementDialog *m_dlg;

};

#endif // OSZILLOSCOPEDIALOGFRAME_H
