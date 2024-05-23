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


#ifndef OSCILLOSCOPEYAXIS_H
#define OSCILLOSCOPEYAXIS_H

#include <QWidget>
#include <QPainter>

#include "OscilloscopeData.h"

#define OSCILLOSCOPE_YAXIS_WIDTH  40

#define AMPL_AXIS_PIXEL_STEP  60

class OscilloscopeWidget;

class OscilloscopeYAxis : public QWidget
{
    Q_OBJECT
public:
    enum Side { Left, Right};
    explicit OscilloscopeYAxis(OscilloscopeWidget *par_OscilloscopeWidget, OSCILLOSCOPE_DATA *par_Data, enum Side par_Side, QWidget *parent = nullptr);
    ~OscilloscopeYAxis() Q_DECL_OVERRIDE;

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent * event) Q_DECL_OVERRIDE;

private:
    enum Side m_Side;
    OSCILLOSCOPE_DATA *m_Data;
    OscilloscopeWidget *m_OscilloscopeWidget;

    int m_MouseGrabbed;
    int m_x1;
    int m_y1;
    int m_x2;
    int m_y2;
    int m_DescSizeChangeFlag;
    int m_SaveWidthAllDescs;
    double m_y_off_save;
};

#endif // OSCILLOSCOPEYAXIS_H
