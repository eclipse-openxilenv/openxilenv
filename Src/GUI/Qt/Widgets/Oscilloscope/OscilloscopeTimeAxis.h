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


#ifndef OSCILLOSCOPETIMEAXIS_H
#define OSCILLOSCOPETIMEAXIS_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

#include "OscilloscopeData.h"

#define TIME_AXIS_PIXEL_STEP 120

class OscilloscopeWidget;

class OscilloscopeTimeAxis : public QWidget
{
    Q_OBJECT
public:
    explicit OscilloscopeTimeAxis(OscilloscopeWidget *par_OscilloscopeWidget, OSCILLOSCOPE_DATA *par_Data, QWidget *parent = nullptr);
    ~OscilloscopeTimeAxis() Q_DECL_OVERRIDE;
    void paint(QPainter &painter, bool border_flag);

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent * event) Q_DECL_OVERRIDE;

private:
    OSCILLOSCOPE_DATA *m_Data;

    OscilloscopeWidget *m_OscilloscopeWidget;

    int m_MouseGrabbed;
    int m_x1;
    int m_x2;
    uint64_t m_t_window_start_save;

};

#endif // OSCILLOSCOPETIMEAXIS_H
