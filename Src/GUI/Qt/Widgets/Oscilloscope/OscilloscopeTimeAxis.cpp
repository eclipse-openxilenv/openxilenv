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


#include "OscilloscopeTimeAxis.h"
#include "OscilloscopeWidget.h"
#include "GetEventPos.h"

extern "C" {
#include "Config.h"
}

OscilloscopeTimeAxis::OscilloscopeTimeAxis(OscilloscopeWidget *par_OscilloscopeWidget, OSCILLOSCOPE_DATA *par_Data, QWidget *parent) : QWidget(parent)
{
    m_Data = par_Data;
    m_OscilloscopeWidget = par_OscilloscopeWidget;
    setMaximumHeight(OSCILLOSCOPE_TIME_AXIS_HIGHT);
    setBackgroundRole(QPalette::Window);
    setAutoFillBackground(true);
}

OscilloscopeTimeAxis::~OscilloscopeTimeAxis()
{

}

void OscilloscopeTimeAxis::paintEvent(QPaintEvent * /* event */)
{
    int win_width, win_height;

    double value_window, value_min, value_max;
    double pixel_per_unit, pixel_per_line;
    double pixel_first_line, value_first_text;
    int x;

    QPainter painter(this);

    if (m_Data->xy_view_flag) {
        if (m_Data->sel_left_right == Left) {
            value_max = m_Data->max_left[m_Data->sel_pos_left | 0x1];
            value_min = m_Data->min_left[m_Data->sel_pos_left | 0x1];
        } else {
            value_max = m_Data->max_right[m_Data->sel_pos_right | 0x1];
            value_min = m_Data->min_right[m_Data->sel_pos_right | 0x1];
        }
    } else {
        /* Calculate min. and max. values of the time window */
        value_max = static_cast<double>(m_Data->t_window_end) / TIMERCLKFRQ;
        value_min = static_cast<double>(m_Data->t_window_start) / TIMERCLKFRQ;
    }
    if (value_max <= value_min) value_max = value_min + 1.0;

    /* Get the window size */
    win_width = width ();
    win_height = height ();
    // If window is nit visable do not paint
    if ((win_width <= 0) || (win_height <= 0)) return;

    QPen PenSave = painter.pen();
    painter.setPen(Qt::darkGray);
    painter.drawRect(QRect(0, 0, width() - 1, height() - 1));
    painter.setPen(PenSave);

    // Automatic adjust scala
    value_window = value_max - value_min;
    pixel_per_line = pixel_per_unit = static_cast<double>(win_width) / value_window;
    x = 0;
    if (pixel_per_line > TIME_AXIS_PIXEL_STEP) {
        while (pixel_per_line > TIME_AXIS_PIXEL_STEP) {
            if ((x % 3 == 0) || (x % 3 == 2)) pixel_per_line /= 2.0;
            else if (x % 3 == 1) pixel_per_line /= 2.5;
            x++;
        }
    } else if (pixel_per_line < TIME_AXIS_PIXEL_STEP) {
        while (pixel_per_line < TIME_AXIS_PIXEL_STEP) {
            if ((x % 3 == 0) || (x % 3 == 2)) pixel_per_line *= 2.0;
            else if (x % 3 == 1) pixel_per_line *= 2.5;
            x++;
        }
    }

    pixel_first_line = ceil (value_min * pixel_per_unit / pixel_per_line) * pixel_per_line
                           - value_min * pixel_per_unit;
    value_first_text = ceil (value_min * pixel_per_unit / pixel_per_line) * pixel_per_line / pixel_per_unit;

    pixel_first_line -= pixel_per_line;
    value_first_text -= pixel_per_line / pixel_per_unit;
    pixel_per_line /= 10;  /* 10 Divided */

    // Paint the axis
    x = 0;
    while (pixel_first_line < static_cast<double>(win_width-1)) {
        if (pixel_first_line >= 0.0) {
            painter.drawLine (static_cast<int>(pixel_first_line), 0, static_cast<int>(pixel_first_line), ((x%5)?6:(x%10)?10:14));
            if (!(x%10)) {
                char Help[32];
                sprintf (Help, "%4.2f", value_first_text);
                QString txt(Help);
                painter.drawText (static_cast<int>(pixel_first_line), 20, txt);
            }
        }
        value_first_text += pixel_per_line / pixel_per_unit;
        pixel_first_line += pixel_per_line;
        x++;
    }
}


void OscilloscopeTimeAxis::mouseReleaseEvent(QMouseEvent * event)
{
    if(event->button () == Qt::LeftButton) {
        if (m_MouseGrabbed) {
            releaseMouse();
            int x = GetEventXPos(event);
            if (m_x2 != x) {
                m_x2 = x;
                m_Data->pixelpersec = static_cast<double>(width()) / static_cast<double>(m_Data->t_window_end - m_Data->t_window_start) / TIMERCLKFRQ;
                m_OscilloscopeWidget->set_t_window_start_end (m_t_window_start_save - static_cast<uint64_t>(static_cast<double>(m_x2 - m_x1) / m_Data->pixelpersec),
                                                              m_t_window_start_save - static_cast<uint64_t>(static_cast<double>(m_x2 - m_x1) / m_Data->pixelpersec + static_cast<double>(width()) / m_Data->pixelpersec));
                this->repaint();
            }
            m_MouseGrabbed = 0;

            m_OscilloscopeWidget->UpdateDrawArea();

            event->accept();
        }
    }
}

void OscilloscopeTimeAxis::mousePressEvent(QMouseEvent * event)
{
    if (event->button () == Qt::LeftButton) {
        if (!m_Data->state) {
            m_t_window_start_save = m_Data->t_window_start;
            grabMouse();
            m_MouseGrabbed = 1;
            m_x2 = m_x1 = GetEventXPos(event);
            event->accept();
        }
    }
}


void OscilloscopeTimeAxis::mouseMoveEvent(QMouseEvent * event)
{
    if (m_MouseGrabbed) {
        int x = GetEventXPos(event);
        if (m_x2 != x) {
            m_x2 = x;
            m_Data->pixelpersec = static_cast<double>(width()) / (m_Data->t_window_end - m_Data->t_window_start);
            double MoveDiff = static_cast<double>(m_x2 - m_x1) / m_Data->pixelpersec;
            uint64_t NewStartTime;
            if (MoveDiff < 0.0) {
                uint64_t MoveDiffI = static_cast<uint64_t>(MoveDiff * -1.0);
                NewStartTime = m_t_window_start_save + MoveDiffI;
            } else {
                uint64_t MoveDiffI = static_cast<uint64_t>(MoveDiff);
                if (MoveDiffI < m_t_window_start_save) {
                    NewStartTime = m_t_window_start_save - MoveDiffI;
                } else {
                    NewStartTime = 0;
                }
            }
            if ((NewStartTime + m_Data->t_window_size) <= m_Data->t_current_buffer_end) {
                m_OscilloscopeWidget->set_t_window_start_end (NewStartTime,
                                                              NewStartTime + m_Data->t_window_size);
            }
            this->repaint();
        }
        event->accept();
    }
}
