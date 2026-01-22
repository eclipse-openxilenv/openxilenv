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


#ifndef OSCILLOSCOPEDESCFRAME_H
#define OSCILLOSCOPEDESCFRAME_H

#include <QWidget>
#include <QPainter>
#include <QGridLayout>

#include "OscilloscopeDesc.h"
#include "OscilloscopeData.h"
#include "DragAndDrop.h"

#define MAX_SIGNALS_INSIDE_FRAME  20

class OscilloscopeWidget;

class OscilloscopeDescFrame : public QWidget
{
    Q_OBJECT
public:
    explicit OscilloscopeDescFrame(OscilloscopeWidget *par_OscilloscopeWidget, OSCILLOSCOPE_DATA *par_Data, enum Side par_Side, QWidget *parent = nullptr);
    ~OscilloscopeDescFrame() Q_DECL_OVERRIDE;

    void UpdateAllUsedDescs (bool OnlyUsed);

    void UpdateYAxis ();

    int DropSignal (enum Side par_Side, int par_Pos, DragAndDropInfos *par_DropInfos);

    void SetSelectedSignal(enum Side par_Side, int par_Number);

    void DeleteSignal(int par_Numer);

    OscilloscopeDesc *GetDesciption(int par_no);

signals:
    void variableForStandardDialog(QStringList arg_variableName, bool arg_showAllVariable, bool arg_multiSelect, QColor arg_color, QFont arg_font);
public slots:

protected:
    void resizeEvent(QResizeEvent * /* event */) Q_DECL_OVERRIDE;

private:
    enum Side m_Side;

    OscilloscopeDesc *m_Desc[MAX_SIGNALS_INSIDE_FRAME];

    OSCILLOSCOPE_DATA *m_Data;

    OscilloscopeWidget *m_OscilloscopeWidget;
};

#endif // OSCILLOSCOPEDESCFRAME_H
