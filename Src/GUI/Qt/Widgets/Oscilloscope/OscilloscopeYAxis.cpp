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

#include "PrintFormatToString.h"
#include "OscilloscopeYAxis.h"
#include "OscilloscopeDesc.h"
#include "OscilloscopeWidget.h"
#include "GetEventPos.h"

extern "C" {
#include "MainValues.h"
}

OscilloscopeYAxis::OscilloscopeYAxis(OscilloscopeWidget *par_OscilloscopeWidget, OSCILLOSCOPE_DATA *par_Data, enum Side par_Side, QWidget *parent) : QWidget(parent)
{
    m_OscilloscopeWidget = par_OscilloscopeWidget;
    m_Data = par_Data;
    m_Side = par_Side;
    setMaximumWidth(OSCILLOSCOPE_YAXIS_WIDTH);
    setBackgroundRole(QPalette::Window);
    setAutoFillBackground(true);
}

OscilloscopeYAxis::~OscilloscopeYAxis()
{

}


void OscilloscopeYAxis::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    paint(painter, true);
}

void OscilloscopeYAxis::paint(QPainter &painter, bool border_flag)
{
    int win_width, win_height;
    double value_window, value_min, value_max;
    double pixel_per_unit, pixel_per_line;
    double pixel_first_line, value_first_text;
    int x, y;
    char txt[128];

    /* min und max-Werte lesen */
    if (m_Side == Right) { /* rechts */
        if (!m_Data->right_axis_on) {
            return;
        }
        if (m_Data->xy_view_flag) {
            // im XY-View immer die geradezahligen
            value_max =  m_Data->max_right[m_Data->sel_pos_right & ~0x1];
            value_min =  m_Data->min_right[m_Data->sel_pos_right & ~0x1];
        } else {
            value_max =  m_Data->max_right[m_Data->sel_pos_right];
            value_min =  m_Data->min_right[m_Data->sel_pos_right];
        }
        win_width = width ();
        win_height = height ();
    } else {
        if (!m_Data->left_axis_on) {
            return;
        }
        if (m_Data->xy_view_flag) {
            // im XY-View immer die geradezahligen
            value_max =  m_Data->max_left[m_Data->sel_pos_left & ~0x1];
            value_min =  m_Data->min_left[m_Data->sel_pos_left & ~0x1];
        } else {
            value_max =  m_Data->max_left[m_Data->sel_pos_left];
            value_min =  m_Data->min_left[m_Data->sel_pos_left];
        }
        win_width = width ();
        win_height = height ();
    }
    // wenn Fenster nicht sichtbar auch nichts malen
    if ((win_width <= 0) || (win_height <= 0)) return;

    // max-Wert immer groesser als min-Wert
    if (value_max <= value_min) value_max = value_min + 1.0;

    // aktuelle Zoomfaktor in min-/max-Werte einrechnen
    value_window = value_max - value_min;
    value_min += value_window * m_Data->y_off[m_Data->zoom_pos];
    value_max = value_window / m_Data->y_zoom[m_Data->zoom_pos] + value_min;

    QPen PenSave = painter.pen();
    QPen Pen(Qt::darkGray);
    Pen.setWidth(2);
    painter.setPen(Pen);

    // Rahmen um Achse Zeichenen
    if (m_Side == Right) {  /* rechts */
        painter.drawLine (0, 0, 0, win_height-1);
        if (border_flag) {
            painter.drawLine (0, 0, 0, win_height-1);
            painter.drawLine (0, win_height-1, win_width-1, win_height-1);
            painter.drawLine (win_width-1, win_height-1, win_width-1, win_height - win_height % OSCILLOSCOPE_DESC_HIGHT);
        }
    } else {      /* links */
        painter.drawLine (win_width-1, 0, win_width-1, win_height-1);
        if (border_flag) {
            painter.drawLine (0, 0, win_width-1, 0);
            painter.drawLine (win_width-1, win_height-1, 0, win_height-1);
            painter.drawLine (0, win_height-1, 0, win_height - win_height % OSCILLOSCOPE_DESC_HIGHT);
        }
    }
    if (!border_flag || !s_main_ini_val.DarkMode) {
        painter.setPen (Qt::black);
    } else {
        painter.setPen (Qt::white);
    }
    if (border_flag) {
        // Flaeche zum veraendern der Beschriftungsbreite
        int Width = width ();
        int Width3 = Width / 3;
        int Width6 = Width3 >> 1;
        int Width12 = Width / 12;
        if (m_Side == Right) {
            painter.drawRect (QRect (Width - Width3 - 1, 0, Width3, Width3));
            x = Width - Width3 + 1;
            y = Width6;
            /*  /  */
            painter.drawLine (x, y, x + Width12, y - Width12);
            /*  \  */
            painter.drawLine (x, y, x + Width12, y + Width12);
            /* --  */
            painter.drawLine (x, y, Width - 2, Width6);
            x = Width - 2;
            y = Width6;
            /* \   */
            painter.drawLine (x, y, x - (Width12), y - (Width12));
            /* /   */
            painter.drawLine (x, y, x - (Width12), y + (Width12));
        } else if (m_Side == Left) {
            painter.drawRect (QRect (0, 0, Width3, Width3));
            x = 1;
            y = Width6;
            /*  /  */
            painter.drawLine (x, y, x + (Width12), y - (Width12));
            /*  \  */
            painter.drawLine (x, y, x + (Width / 12), y + (Width12));
            /* --  */
            painter.drawLine (x, y, Width3 - 2, Width6);
            x = Width3 - 2;
            y = Width6;
            /* \   */
            painter.drawLine (x, y, x - (Width12), y - (Width12));
            /* /   */
            painter.drawLine (x, y, x - (Width12), y + (Width12));
        }
    }

    // Skala automatisch anpassen
    value_window = value_max - value_min;
    pixel_per_line = pixel_per_unit = static_cast<double>(win_height) / value_window;
    if (pixel_per_line <= 0.0) pixel_per_line = 1.0;
    x = 0;
    if (pixel_per_line > AMPL_AXIS_PIXEL_STEP) {
        while (pixel_per_line > AMPL_AXIS_PIXEL_STEP) {
            if ((x % 3 == 0) || (x % 3 == 2)) pixel_per_line /= 2.0;
            else if (x % 3 == 1) pixel_per_line /= 2.5;
            x++;
        }
    } else if (pixel_per_line < AMPL_AXIS_PIXEL_STEP) {
        while (pixel_per_line < AMPL_AXIS_PIXEL_STEP) {
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
    pixel_per_line /= 10;  /* 10 Teilung */

    // Achse Zeichnen
    x = 0;
    while (pixel_first_line < static_cast<double>(win_height-1)) {
        if (pixel_first_line >= 0.0) {
            if (m_Side == Right) {  /* rechts */
                painter.drawLine (0, win_height - static_cast<int>(pixel_first_line),
                                  ((x%5)?6:(x%10)?10:14), win_height - static_cast<int>(pixel_first_line));
            } else {      /* links */
                painter.drawLine (win_width-1, win_height - static_cast<int>(pixel_first_line),
                                  win_width-((x%5)?6:(x%10)?10:14), win_height - static_cast<int>(pixel_first_line));
            }
            if (!(x%10)) {
                if ((value_first_text > 1000000.0) || (value_first_text < -1000000.0))
                    PrintFormatToString (txt, sizeof(txt), "%g", value_first_text);
                else PrintFormatToString (txt, sizeof(txt), "%4.1lf", value_first_text);
                if (m_Side == Right) {
                    painter.drawText (15, win_height - static_cast<int>(pixel_first_line), tr (txt));
                } else {
                    painter.drawText (2, win_height - static_cast<int>(pixel_first_line), tr (txt));
                }
            }
        }
        value_first_text += pixel_per_line / pixel_per_unit;
        pixel_first_line += pixel_per_line;
        x++;
    }
    painter.setPen(PenSave);
}


void OscilloscopeYAxis::mouseReleaseEvent(QMouseEvent * event)
{
    if (event->button () == Qt::LeftButton) {
        if (m_MouseGrabbed) {
            releaseMouse();
            m_MouseGrabbed = 0;
            if (m_DescSizeChangeFlag) {
                m_x2 = GetEventXPos(event);
                if (m_Side == Left) {
                    m_Data->width_all_left_desc = m_SaveWidthAllDescs + (m_x2 - m_x1) / m_Data->FontAverageCharWidth ;
                    if (m_Data->width_all_left_desc < 10) m_Data->width_all_left_desc = 10;
                    if (m_Data->width_all_left_desc > 200) m_Data->width_all_left_desc = 200;
                } else if (m_Side == Right) {
                    m_Data->width_all_right_desc = m_SaveWidthAllDescs + (m_x1 - m_x2) / m_Data->FontAverageCharWidth;
                    if (m_Data->width_all_right_desc < 10) m_Data->width_all_right_desc = 10;
                    if (m_Data->width_all_right_desc > 200) m_Data->width_all_right_desc = 200;
                }
                m_OscilloscopeWidget->ResizeDescFrames();
            } else {
                m_y2 = GetEventYPos(event);
                // recalculate the offset
                m_Data->y_off[m_Data->zoom_pos] = m_y_off_save + static_cast<double>(m_y2 - m_y1) / (static_cast<double>(height()) * m_Data->y_zoom[m_Data->zoom_pos]);
                // repaint left and right axis
                repaint();
                m_OscilloscopeWidget->UpdateDrawArea();
            }

            event->accept();
        } 
    }
}

void OscilloscopeYAxis::mousePressEvent(QMouseEvent * event)
{
    if (event->button () == Qt::LeftButton) {
        //if (!m_Data->state) {
            int x = GetEventXPos(event);
            int y = GetEventYPos(event);
            int Width = width();
            int Width3 = Width / 3;

            if ((y <= (Width3)) && (
                 ((m_Side == Left) && (x < Width3)) ||
                 ((m_Side == Right) && (x > (Width - Width3))))) {
                m_DescSizeChangeFlag = 1;
                if (m_Side == Left) m_SaveWidthAllDescs = m_Data->width_all_left_desc;
                else if (m_Side == Right) m_SaveWidthAllDescs = m_Data->width_all_right_desc;
            } else {
                m_DescSizeChangeFlag = 0;
                // merke aktuell Offseteinstellung
                m_y_off_save = m_Data->y_off[m_Data->zoom_pos];
            }
            grabMouse();
            m_MouseGrabbed = 1;
            m_x2 = m_x1 = x;
            m_y2 = m_y1 = y;
            event->accept();
        //}
    }
}


void OscilloscopeYAxis::mouseMoveEvent(QMouseEvent * event)
{
    if (m_MouseGrabbed) {
        if (m_DescSizeChangeFlag) {
                m_x2 = GetEventXPos(event);
            if (m_Side == Left) {
                m_Data->width_all_left_desc = m_SaveWidthAllDescs + (m_x2 - m_x1) / m_Data->FontAverageCharWidth;
            } else if (m_Side == Right) {
                m_Data->width_all_right_desc = m_SaveWidthAllDescs + (m_x1 - m_x2) / m_Data->FontAverageCharWidth;
            }
            if (m_Data->width_all_left_desc < 5) m_Data->width_all_left_desc = 5;
            if (m_Data->width_all_left_desc > 40) m_Data->width_all_left_desc = 40;
        } else {
            m_y2 = GetEventYPos(event);
            // neue Offseteinstellung berechnen
            m_Data->y_off[m_Data->zoom_pos] = m_y_off_save + static_cast<double>(m_y2 - m_y1) / (static_cast<double>(height()) * m_Data->y_zoom[m_Data->zoom_pos]);
            // linke und rechte Achse neu zeichenen
            repaint();
        }
    }
}

