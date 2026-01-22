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


#include "OscilloscopeDrawArea.h"

#include "FileDialog.h"
#include "OscilloscopeWidget.h"
#include "OscilloscopeZoomHistoryDialog.h"
#include "GetEventPos.h"
#include "StringHelpers.h"

#include <inttypes.h>
#include <math.h>

extern "C" {
    #include "my_udiv128.h"
    #include "ThrowError.h"
    #include "PrintFormatToString.h"
    #include "MainValues.h"
    #include "Scheduler.h"
    #include "FileExtensions.h"
    #include "OscilloscopeFile.h"
}

//#define DEBUG_PRINT_TO_FILE

#ifdef DEBUG_PRINT_TO_FILE
void OscilloscopeDrawAreaDebugPrint (OscilloscopeDrawArea *DrawArea, const char *txt, ...);
#define DEBUG_PRINT(DrawArea, txt, ...) OscilloscopeDrawAreaDebugPrint(DrawArea, txt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(DrawArea, txt, ...)
#endif

OscilloscopeDrawArea::OscilloscopeDrawArea(OscilloscopeWidget *par_OscilloscopeWidget, OSCILLOSCOPE_DATA *par_Data, QWidget *parent) : QWidget(parent)
{
    m_OscilloscopeWidget = par_OscilloscopeWidget;
    m_Data = par_Data;
    m_FileHandle = nullptr;
    //m_OnlyUpdateCursorFlag = NO_CURSOR;
    m_OnlyUpdateZoomRectFlag = NO_ZOOM_RECT;
    m_MouseGrabbed = 0;

    DEBUG_PRINT (this, "QGuiApplication::platformName() = %s\n", QGuiApplication::platformName().toLatin1().data());
    if(!QGuiApplication::platformName().compare(QString("xcb"))) {
        m_IsX11 = true;
    } else {
        m_IsX11 = false;
    }

    CursorPen = QPen (Qt::blue, 1, Qt::DashLine);
    RefCursorPen = QPen (Qt::red, 1, Qt::DashLine);

    createActions ();

    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    if (s_main_ini_val.DarkMode) {
        m_BackgroundColor = QColor (Qt::black);
    } else {
        m_BackgroundColor = QColor (Qt::white);
    }
    setAcceptDrops(true);

    m_BufferPixMap = nullptr;
    m_ClearFlag = true;
    m_BackgroundImage = nullptr;
}

OscilloscopeDrawArea::~OscilloscopeDrawArea()
{
    if (m_BufferPixMap != nullptr) {
        delete m_BufferPixMap;
    }
}

static QPen BuildPen(int par_LineWidth, int par_Color)
{
    QPen Pen;
    Pen.setWidth (par_LineWidth);
    int r = par_Color & 0xFF;
    int g = (par_Color >> 8) & 0xFF;
    int b = (par_Color >> 16) & 0xFF;
    QColor Color (r, g, b);
    Pen.setColor (Color);
    return Pen;
}

void OscilloscopeDrawArea::PaintTimeLineToPixmap(QPainter &painter, QRect par_Rec)
{
    int i;
    int StartXPos = par_Rec.x() - 2;
    int EndXPos = par_Rec.x() + par_Rec.width();
    if (StartXPos < 0) {
        StartXPos = 0; // Otherwise there can be go to negative time
    }

    // Fill background
    DEBUG_PRINT (this, "fill background: rect.x()) = %" PRIi32 ", "
                 "rect.y()) = %" PRIi32 ", "
                 "rect.width()) = %" PRIi32 ", "
                 "rect.height()) = %" PRIi32 "\n", par_Rec.x(), par_Rec.y(), par_Rec.width(), par_Rec.height());
    painter.fillRect (par_Rec, m_BackgroundColor);

    uint64_t w = static_cast<uint64_t>(width());
    uint64_t d = m_Data->t_window_end - m_Data->t_window_start;
    // Y axis == time
    uint64_t StartXTime = my_umuldiv64(static_cast<uint64_t>(StartXPos), d, w) + m_Data->t_window_start;
    uint64_t EndXTime =  my_umuldiv64(static_cast<uint64_t>(EndXPos), d, w) + m_Data->t_window_start;
    StartXTime /= m_Data->t_step;
    StartXTime *= m_Data->t_step;
    EndXTime /= m_Data->t_step;
    EndXTime *= m_Data->t_step;
    DEBUG_PRINT (this, "paintEvent: StartXTime = %" PRIu64 ", EndXTime = %" PRIu64 ", t_current_to_update_end = %" PRIu64 "\n", StartXTime, EndXTime, m_Data->t_current_to_update_end);

    if (m_Data->t_current_to_update_end > StartXTime) {   // There are no valid values in this time range
        if (EndXTime > m_Data->t_current_to_update_end) {
            EndXTime = m_Data->t_current_to_update_end;
        }
        int win_height = height();
        double win_height_d = static_cast<double>(win_height);
        double win_height_p20_d = win_height_d + 20;  // Easyer for out of window hight check, the max. line with is limited to 40 pixek

        // First paint all lines on the right side
        for (i = 0; i < 20; i++) {
            if ((m_Data->vids_right[i] > 0) && (!m_Data->vars_disable_right[i])) {
                // set the color
                painter.setPen (BuildPen(m_Data->LineSize_right[i], m_Data->color_right[i]));
                uint64_t t = StartXTime;
                int x_m1, y_m1, valid_m1 = 0;
                double ymax = -DBL_MAX;
                double ymin = DBL_MAX;
                bool ymaxflag = false;
                bool yminflag = false;
                for (;;) {
                    int x;
                    if (t >= m_Data->t_window_start) {
                        x = static_cast<int>(my_umuldiv64(t - m_Data->t_window_start, w, d));
                        double Value = FiFoPosRight (m_Data, i, t);
                        if (!_isnan (Value)) {
                            double y_d = (((Value - m_Data->min_right[i]) / (m_Data->max_right[i] - m_Data->min_right[i]) - m_Data->y_off[m_Data->zoom_pos]) * m_Data->y_zoom[m_Data->zoom_pos] * win_height_d);
                            if (y_d > win_height_p20_d) y_d = win_height_p20_d;
                            if (y_d < -20.0) y_d = -20.0;  // Line width  max. 40 pixel
                            int y = win_height - static_cast<int>(y_d + 0.5);

                            if (m_Data->presentation_right[i] == 1) {
                                QPoint Point(x, y);
                                painter.drawPoint(Point);
                            } else {
                                if (valid_m1) { // Value before was valid too
                                    if (y != y_m1) {  // Are the y changed?
                                        if (x != x_m1) {  // Are the x changed?
                                            if ((x - x_m1) > 1) { // Are the x value change more as one pixel? Than we have to paint a _| otherwise only a |
                                                painter.drawLine (x_m1, y_m1, x, y_m1);
                                                painter.drawLine (x, y_m1, x, y);
                                            } else {
                                                double y1, y2;
                                                // If some sample point are not drawn because the x axis is not changed
                                                // we muss check against the min./max. value of the ignored samples.
                                                y1 = (y_m1 > y) ? y_m1 : y;
                                                y2 = (y_m1 < y) ? y_m1 : y;
                                                if (ymaxflag) y1 = (y1 > ymax) ? y1 : ymax;
                                                if (yminflag) y2 = (y2 < ymin) ? y2 : ymin;
                                                painter.drawLine (x, y1, x, y2);
                                                ymaxflag = false;
                                                yminflag = false;
                                                ymax = -DBL_MAX;
                                                ymin = DBL_MAX;
                                            }
                                            x_m1 = x;
                                            y_m1 = y;
                                        } else {
                                            if (y > ymax) {
                                                ymaxflag = true;
                                                ymax = y;
                                            }
                                            if (y < ymin) {
                                                yminflag = true;
                                                ymin = y;
                                            }
                                        }
                                    }
                                } else {
                                    // Value before was not valid than only store it
                                    x_m1 = x;
                                    y_m1 = y;
                                }
                            }
                            valid_m1 = 1;
                        } else if (m_Data->presentation_right[i] == 0) {
                            // This is only neccassary for lines not for points
                            if (valid_m1) { // The value before was valid and are now invalid
                                if (x != x_m1) {  // Are the x changed?
                                    if ((x - x_m1) > 1) { // Are the x value change more as one pixel? Than we have to paint a _| otherwise only a |
                                        painter.drawLine (x_m1, y_m1, x, y_m1);
                                    } else {
                                        painter.drawPoint(x, y_m1);
                                    }
                                }
                            }
                            valid_m1 = 0;
                        }
                    }
                    t += m_Data->t_step;
                    if (t > EndXTime) {
                        if (m_Data->presentation_right[i] == 0) {
                            // This is only neccassary for lines not for points
                            if (valid_m1) { // The value before was valid
                                if (x != x_m1) {  // Are the x changed?
                                    if ((x - x_m1) > 1) { // Are the x value change more as one pixel? Than we have to paint a _| otherwise only a |
                                        painter.drawLine (x_m1, y_m1, x, y_m1);
                                    } else {
                                        painter.drawPoint(x, y_m1);
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
        // Second paint all lines on the left side
        for (i = 0; i < 20; i++) {
            if ((m_Data->vids_left[i] > 0) && (!m_Data->vars_disable_left[i])) {
                // set the color
                painter.setPen (BuildPen(m_Data->LineSize_left[i], m_Data->color_left[i]));
                uint64_t t = StartXTime;
                int x_m1, y_m1, valid_m1 = 0;
                double ymax = -DBL_MAX;
                double ymin = DBL_MAX;
                bool ymaxflag = false;
                bool yminflag = false;
                for (;;) {
                    int x;
                    if (t >= m_Data->t_window_start) {
                        x = static_cast<int>(my_umuldiv64(t - m_Data->t_window_start, w, d));
                        double Value = FiFoPosLeft (m_Data, i, t);
                        if (!_isnan (Value)){
                            double y_d = (((Value - m_Data->min_left[i]) / (m_Data->max_left[i] - m_Data->min_left[i]) - m_Data->y_off[m_Data->zoom_pos]) * m_Data->y_zoom[m_Data->zoom_pos] * win_height_d);
                            if (y_d > win_height_p20_d) y_d = win_height_p20_d;
                            if (y_d < -20.0) y_d = -20.0;  // Line width  max. 40 pixel
                            int y = win_height - static_cast<int>(y_d + 0.5);

                            if (m_Data->presentation_left[i] == 1) {
                                QPoint Point(x, y);
                                painter.drawPoint(Point);
                            } else {
                                if (valid_m1) { // Value before was valid too
                                    if (y != y_m1) {  // Are the y changed?
                                        if (x != x_m1) {  //  Are the x changed?
                                            if ((x - x_m1) > 1) { // Are the x value change more as one pixel? Than we have to paint a _| otherwise only a |
                                                painter.drawLine (x_m1, y_m1, x, y_m1);
                                                painter.drawLine (x, y_m1, x, y);
                                            } else {
                                                double y1, y2;
                                                // If some sample point are not drawn because the x axis is not changed
                                                // we muss check against the min./max. value of the ignored samples.
                                                y1 = (y_m1 > y) ? y_m1 : y;
                                                y2 = (y_m1 < y) ? y_m1 : y;
                                                if (ymaxflag) y1 = (y1 > ymax) ? y1 : ymax;
                                                if (yminflag) y2 = (y2 < ymin) ? y2 : ymin;
                                                painter.drawLine (x, y1, x, y2);
                                                ymaxflag = false;
                                                yminflag = false;
                                                ymax = -DBL_MAX;
                                                ymin = DBL_MAX;
                                            }
                                            x_m1 = x;
                                            y_m1 = y;
                                        } else {
                                            if (y > ymax) {
                                                ymaxflag = true;
                                                ymax = y;
                                            }
                                            if (y < ymin) {
                                                yminflag = true;
                                                ymin = y;
                                            }
                                        }
                                    }
                                } else {
                                    // Value before was not valid than only store it
                                    x_m1 = x;
                                    y_m1 = y;
                                }
                            }
                            valid_m1 = 1;
                        } else if (m_Data->presentation_left[i] == 0) {
                            // This is only neccassary for lines not for points
                            if (valid_m1) { // The value before was valid
                                if (x != x_m1) {  // Are the x changed?
                                    if ((x - x_m1) > 1) { // Are the x value change more as one pixel? Than we have to paint a _| otherwise only a |
                                        painter.drawLine (x_m1, y_m1, x, y_m1);
                                        painter.drawLine (x_m1, y_m1, x, y_m1);
                                    } else {
                                        painter.drawPoint(x, y_m1);
                                    }
                                }
                            }
                            valid_m1 = 0;
                        }
                    }
                    t += m_Data->t_step;
                    if (t > EndXTime) {
                        if (m_Data->presentation_left[i] == 0) {
                            // This is only neccassary for lines not for points
                            if (valid_m1) { // The value before was valid
                                if (x != x_m1) {  // Are the x changed?
                                    if ((x - x_m1) > 1) { // Are the x value change more as one pixel? Than we have to paint a _| otherwise only a |
                                        painter.drawLine (x_m1, y_m1, x, y_m1);
                                    } else {
                                        painter.drawPoint(x, y_m1);
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}

void OscilloscopeDrawArea::paint(QPainter &painter)
{
    QColor SaveColor = m_BackgroundColor;
    m_BackgroundColor = Qt::white;
    if (m_Data->xy_view_flag) {
        PaintXYToPixmap(painter);
    } else {
        QRect Rect;
        Rect.setX(0);
        Rect.setY(0);
        Rect.setWidth(width());
        Rect.setHeight(height());
        PaintTimeLineToPixmap(painter, Rect);
    }
    m_BackgroundColor = SaveColor;
}

void OscilloscopeDrawArea::paint_cursor(QPainter *painter)
{
    if (m_Data->state == 0) {
        PaintCursor(painter, false);
    }
}

void OscilloscopeDrawArea::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    if (m_OnlyUpdateZoomRectFlag) {
        painter.drawPixmap(this->rect(), *m_BufferPixMap);
        // Paint only the zoom frame
        PaintZoomRectangle (&painter);
        m_OnlyUpdateZoomRectFlag = NO_ZOOM_RECT;
    } else {
        if (m_BufferPixMap == nullptr) {
            m_BufferPixMap = new QPixmap(size());
            m_ClearFlag = true;
        } else if (m_BufferPixMap->size() != size()) {
            delete m_BufferPixMap;
            m_BufferPixMap = new QPixmap(size());
            m_ClearFlag = true;
        }
        EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
        // Store how many time elapsed till the paint method is called.
        // this have to paint inside the next call
        m_Data->t_current_buffer_end_diff = m_Data->t_current_buffer_end - m_Data->t_current_buffer_end_save;
        m_Data->t_current_to_update_end = m_Data->t_current_buffer_end;
        if (m_Data->xy_view_flag) {
            QPainter painter (m_BufferPixMap);
            PaintXYToPixmap(painter);
        } else {
            QPainter painter (m_BufferPixMap);
            PaintTimeLineToPixmap(painter, event->rect());
        }
        painter.drawPixmap(this->rect(), *m_BufferPixMap);
        if (!m_Data->state) PaintCursor (&painter, true);
        m_Data->t_current_updated_end = m_Data->t_current_to_update_end;
        LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
    }
}

void OscilloscopeDrawArea::PaintZoomRectangle (QPainter *painter)
{
    DEBUG_PRINT(this, "PaintZoomRectangle: paint zoom box\n");
    QColor Color;
    if (s_main_ini_val.DarkMode) {
        Color = QColor(Qt::white);
    } else {
        Color = QColor(Qt::black);
    }
    QPen Pen(Color);
    Pen.setStyle(Qt::DashLine);
    painter->setPen (Pen);
    painter->drawLine (m_x1, m_y1, m_x1, m_y2);
    painter->drawLine (m_x1, m_y2, m_x2, m_y2);
    painter->drawLine (m_x2, m_y2, m_x2, m_y1);
    painter->drawLine (m_x2, m_y1, m_x1, m_y1);
}

void OscilloscopeDrawArea::PaintCursor (QPainter *painter, bool xor_flag)
{
    double x_d, y_d;
    double Min, Max, Value, ValueX, MinX, MaxX;
    bool Visable = true;

    // Cursor is only visable if oscilloscope is offline
    if (!m_Data->state) {
        if (m_Data->sel_left_right) {  /* Right */
            if ((m_Data->xy_view_flag && ((m_Data->buffer_right[m_Data->sel_pos_right & ~0x1] == nullptr) ||
                                          (m_Data->buffer_right[(m_Data->sel_pos_right & ~0x1) + 1] == nullptr))) ||
                (!m_Data->xy_view_flag && (m_Data->buffer_right[m_Data->sel_pos_right] == nullptr))) {
                Visable = false;
            } else {
                if (m_Data->xy_view_flag) {
                    // If in XY mode the following channel is the x axis position
                    Value = FiFoPosRight (m_Data, m_Data->sel_pos_right & ~0x1, m_Data->t_cursor);
                    ValueX = FiFoPosRight (m_Data, (m_Data->sel_pos_right & ~0x1) + 1, m_Data->t_cursor);
                } else {
                    Value = FiFoPosRight (m_Data, m_Data->sel_pos_right, m_Data->t_cursor);
                }
            }
            if (Visable) {
                if (_isnan (Value)) Visable = false;
                if (m_Data->xy_view_flag) if (_isnan (ValueX)) Visable = false;
                if (m_Data->xy_view_flag) {
                    Min = m_Data->min_right[m_Data->sel_pos_right & ~0x1];
                    Max = m_Data->max_right[m_Data->sel_pos_right & ~0x1];
                    MinX = m_Data->min_right[(m_Data->sel_pos_right & ~0x1) + 1];
                    MaxX = m_Data->max_right[(m_Data->sel_pos_right & ~0x1) + 1];

                } else {
                    Min = m_Data->min_right[m_Data->sel_pos_right];
                    Max = m_Data->max_right[m_Data->sel_pos_right];
                }
            }
        } else {                          /* Or left */
            if ((m_Data->xy_view_flag && ((m_Data->buffer_left[m_Data->sel_pos_left & ~0x1] == nullptr) ||
                                          (m_Data->buffer_left[(m_Data->sel_pos_left & ~0x1) + 1] == nullptr))) ||
                (!m_Data->xy_view_flag && (m_Data->buffer_left[m_Data->sel_pos_left] == nullptr))) {
                Visable = false;
            } else {
                if (m_Data->xy_view_flag) {
                    // If in XY mode the following channel is the x axis position
                    Value = FiFoPosLeft (m_Data, m_Data->sel_pos_left & ~0x1, m_Data->t_cursor);
                    ValueX = FiFoPosLeft (m_Data, (m_Data->sel_pos_left & ~0x1) + 1, m_Data->t_cursor);
                } else {
                    Value = FiFoPosLeft (m_Data, m_Data->sel_pos_left, m_Data->t_cursor);
                }
            }
            if (Visable) {
                if (_isnan (Value)) Visable = false;
                if (m_Data->xy_view_flag) if (_isnan (ValueX)) Visable = false;
                if (m_Data->xy_view_flag) {
                    Min = m_Data->min_left[m_Data->sel_pos_left & ~0x1];
                    Max = m_Data->max_left[m_Data->sel_pos_left & ~0x1];
                    MinX = m_Data->min_left[(m_Data->sel_pos_left & ~0x1) + 1];
                    MaxX = m_Data->max_left[(m_Data->sel_pos_left & ~0x1) + 1];

                } else {
                    Min = m_Data->min_left[m_Data->sel_pos_left];
                    Max = m_Data->max_left[m_Data->sel_pos_left];
                }
            }
        }
        if (Visable) {
            if (m_Data->xy_view_flag) {
                double YFactor = -static_cast<double>(height()) / (Max - Min);
                double YOffset = static_cast<double>(height()) - Min * YFactor;
                double XFactor = static_cast<double>(width()) / (MaxX - MinX);
                double XOffset = - MinX * XFactor;
                y_d = Value * YFactor + YOffset;
                x_d = ValueX * XFactor + XOffset;
            } else {
                y_d = static_cast<double>(height()) - (((Value - Min) / (Max - Min) - m_Data->y_off[m_Data->zoom_pos]) *
                                                       m_Data->y_zoom[m_Data->zoom_pos] * static_cast<double>(height()));

                double Fac1 = static_cast<double>(width()) / (m_Data->t_window_end - m_Data->t_window_start);
                x_d = (m_Data->t_cursor - m_Data->t_window_start) * Fac1;
            }
        }
        QPen SavePen = painter->pen();
        QColor Color(Qt::lightGray);
        QPen Pen(Color);
        Pen.setStyle(Qt::DashLine);
        painter->setPen (Pen);

        if ((qIsInf(x_d)) | (qIsInf(y_d)) | (qIsNaN(x_d)) | (qIsNaN(y_d))) {
            // no zoom possibility
            // actual no drawing of the line
            // better solution can be developed
        } else {
            if (Visable) {
                QLineF Line1(x_d, y_d, x_d, static_cast<double>(height()));
                painter->drawLine (Line1);
                QLineF Line2(x_d, y_d, x_d, 0.0);
                painter->drawLine (Line2);
                QLineF Line3(x_d, y_d, static_cast<double>(width()), y_d);
                painter->drawLine (Line3);
                QLineF Line4(x_d, y_d, 0.0, y_d);
                painter->drawLine (Line4);
            } else {
                QLineF Line(x_d, 0.0, x_d, static_cast<double>(height()));
                painter->drawLine (Line);
            }
            // If refence point is active but not XY view
            if (m_Data->ref_point_flag && !m_Data->xy_view_flag) {
                painter->setPen (RefCursorPen);
                double Fac1 = static_cast<double>(width()) / (m_Data->t_window_end - m_Data->t_window_start);
                x_d = ((m_Data->t_ref_point - m_Data->t_window_start) * Fac1);
                if ((x_d > 0.0) && (x_d < static_cast<double>(width()))) {
                    QLineF Line1(x_d, 0, x_d, static_cast<double>(height()));
                    painter->drawLine (Line1);
                    painter->drawText (static_cast<int>(x_d)+3, 12, tr ("ref"));
                }
            }
        }
        painter->setPen (SavePen);
    }
}

void OscilloscopeDrawArea::PaintXYToPixmap(QPainter &painter)
{
    QPen SavePen = painter.pen();
    if (m_ClearFlag) {
        // Fill background
        if (m_BackgroundImage != nullptr) {
            painter.drawImage(this->rect(), *m_BackgroundImage);
        } else {
            painter.fillRect (m_BufferPixMap->rect(), m_BackgroundColor);
        }
    }
    int win_height = height();
    double win_height_d = static_cast<double>(win_height);
    int win_width = width();
    double win_width_d = static_cast<double>(win_width);

    // Left side
    for (int i = 0; i < 20; i+=2) {
        if (((m_Data->vids_left[i] > 0) && (!m_Data->vars_disable_left[i])) &&     // XY view are only posible if there where 2 signals one behind.
            ((m_Data->vids_left[i+1] > 0) && (!m_Data->vars_disable_left[i+1]))) {
            // set the color
            painter.setPen (BuildPen(m_Data->LineSize_left[i], m_Data->color_left[i]));

            bool valid_m1 = false;
            double x_m1_d, y_m1_d;

            double YFactor = -win_height_d / (m_Data->max_left[i] - m_Data->min_left[i]);
            double YOffset = win_height_d - m_Data->min_left[i] * YFactor;
            double XFactor = win_width_d / (m_Data->max_left[i+1] - m_Data->min_left[i+1]);
            double XOffset = - m_Data->min_left[i+1] * XFactor;

            int new_since_last_draw = m_Data->new_since_last_draw_left[i] + 2;
            int ToPaintPoints =  (m_Data->depth_left[i] < m_Data->depth_left[i+1]) ? m_Data->depth_left[i] : m_Data->depth_left[i+1];
            if (!m_ClearFlag) {
                ToPaintPoints = (new_since_last_draw < ToPaintPoints) ? new_since_last_draw : ToPaintPoints;
            }
            m_Data->new_since_last_draw_left[i] = 0;
            for (int Idx = 1; Idx < ToPaintPoints; Idx++) {
                int IdxY = m_Data->wr_poi_left[i] - Idx;
                if (IdxY < 0) IdxY = m_Data->buffer_depth + IdxY;
                int IdxX = m_Data->wr_poi_left[i+1] - Idx;
                if (IdxX < 0) IdxX = m_Data->buffer_depth + IdxX;

                double ValueY = m_Data->buffer_left[i][IdxY];
                double ValueX = m_Data->buffer_left[i+1][IdxX];
                if (!_isnan (ValueX) && !_isnan (ValueY)) {

                    double y_d = ValueY * YFactor + YOffset;
                    double x_d = ValueX * XFactor + XOffset;

                    if (m_Data->presentation_left[i] == 1) {  // Only check this for the y axis
                        QPointF Point(x_d, y_d);
                        painter.drawPoint(Point);
                    } else {
                        if (valid_m1) { // Value before was also valid
                            QLineF Line (x_m1_d, y_m1_d, x_d, y_d);
                            painter.drawLine (Line);
                            x_m1_d = x_d;
                            y_m1_d = y_d;
                        } else {
                            // Store only the value if the value before was invalid
                            x_m1_d = x_d;
                            y_m1_d = y_d;
                        }
                    }
                    valid_m1 = true;
                } else {
                    valid_m1 = false;
                }

            }
        }
    }
    // Right side
    for (int i = 0; i < 20; i+=2) {
        if (((m_Data->vids_right[i] > 0) && (!m_Data->vars_disable_right[i])) &&     // XY view are only posible if there where 2 signals one behind.
            ((m_Data->vids_right[i+1] > 0) && (!m_Data->vars_disable_right[i+1]))) {
            // set the color
            painter.setPen (BuildPen(m_Data->LineSize_right[i], m_Data->color_right[i]));

            bool valid_m1 = false;
            double x_m1_d, y_m1_d;

            double YFactor = -win_height_d / (m_Data->max_right[i] - m_Data->min_right[i]);
            double YOffset = win_height_d - m_Data->min_right[i] * YFactor;
            double XFactor = win_width_d / (m_Data->max_right[i+1] - m_Data->min_right[i+1]);
            double XOffset = - m_Data->min_right[i+1] * XFactor;

            int new_since_last_draw = m_Data->new_since_last_draw_right[i] + 2;
            int ToPaintPoints =  (m_Data->depth_right[i] < m_Data->depth_right[i+1]) ? m_Data->depth_right[i] : m_Data->depth_right[i+1];
            if (!m_ClearFlag) {
                ToPaintPoints = (new_since_last_draw < ToPaintPoints) ? new_since_last_draw : ToPaintPoints;
            }
            m_Data->new_since_last_draw_right[i] = 0;
            for (int Idx = 1; Idx < ToPaintPoints; Idx++) {
                int IdxY = m_Data->wr_poi_right[i] - Idx;
                if (IdxY < 0) IdxY = m_Data->buffer_depth + IdxY;
                int IdxX = m_Data->wr_poi_right[i+1] - Idx;
                if (IdxX < 0) IdxX = m_Data->buffer_depth + IdxX;

                double ValueY = m_Data->buffer_right[i][IdxY];
                double ValueX = m_Data->buffer_right[i+1][IdxX];
                if (!_isnan (ValueX) && !_isnan (ValueY)) {

                    double y_d = ValueY * YFactor + YOffset;
                    double x_d = ValueX * XFactor + XOffset;

                    if (m_Data->presentation_right[i] == 1) {  // Only check this for the y axis
                        QPointF Point(x_d, y_d);
                        painter.drawPoint(Point);
                    } else {
                        if (valid_m1) { // Value before was also valid
                            QLineF Line (x_m1_d, y_m1_d, x_d, y_d);
                            painter.drawLine (Line);
                            x_m1_d = x_d;
                            y_m1_d = y_d;
                        } else {
                            // Store only the value if the value before was invalid
                            x_m1_d = x_d;
                            y_m1_d = y_d;
                        }
                    }
                    valid_m1 = true;
                } else {
                    valid_m1 = false;
                }

            }
        }
    }
    m_ClearFlag = false;
    painter.setPen (SavePen);
}

bool OscilloscopeDrawArea::PickingXYPoint(int x, int y, uint64_t *ret_Time)
{
    int win_height = height();
    double win_height_d = static_cast<double>(win_height);
    int win_width = width();
    double win_width_d = static_cast<double>(win_width);

    double DistanceMin = DBL_MAX;
    int IMin = 0;
    int IdxMin = 0;
    enum Side LeftOrRight = Left;
    double XMin = 0.0;
    double YMin = 0.0;

    // Left side
    if (m_Data->sel_left_right == 0) {
        int i = m_Data->sel_pos_left & ~0x1;
        if (((m_Data->vids_left[i] > 0) && (!m_Data->vars_disable_left[i])) &&     // XY view are only posible if there where 2 signals one behind.
            ((m_Data->vids_left[i+1] > 0) && (!m_Data->vars_disable_left[i+1]))) {

            double YFactor = -win_height_d / (m_Data->max_left[i] - m_Data->min_left[i]);
            double YOffset = win_height_d - m_Data->min_left[i] * YFactor;
            double XFactor = win_width_d / (m_Data->max_left[i+1] - m_Data->min_left[i+1]);
            double XOffset = - m_Data->min_left[i+1] * XFactor;

            int ToSearchPoints = (m_Data->depth_left[i] < m_Data->depth_left[i+1]) ? m_Data->depth_left[i] : m_Data->depth_left[i+1];
            for (int Idx = 1; Idx < ToSearchPoints; Idx++) {
                int IdxY = m_Data->wr_poi_left[i] - Idx;
                if (IdxY < 0) IdxY = m_Data->buffer_depth + IdxY;
                int IdxX = m_Data->wr_poi_left[i+1] - Idx;
                if (IdxX < 0) IdxX = m_Data->buffer_depth + IdxX;

                double ValueY = m_Data->buffer_left[i][IdxY];
                double ValueX = m_Data->buffer_left[i+1][IdxX];
                if (!_isnan (ValueY) && !_isnan (ValueY)) {

                    double y_d = ValueY * YFactor + YOffset;
                    double x_d = ValueX * XFactor + XOffset;

                    y_d -= static_cast<double>(y);
                    y_d = y_d * y_d;
                    x_d -= static_cast<double>(x);
                    x_d = x_d * x_d;

                    double Distance = sqrt(y_d + x_d);
                    if (Distance < DistanceMin) {
                        DistanceMin = Distance;
                        IdxMin = Idx;
                        LeftOrRight = Left;
                        XMin = ValueY;
                        YMin = ValueX;
                        IMin = i;
                    }
                }
            }
        }
    } else {
        // Right side
        int i = m_Data->sel_pos_right & ~0x1;
        if (((m_Data->vids_right[i] > 0) && (!m_Data->vars_disable_right[i])) &&     // XY view are only posible if there where 2 signals one behind.
            ((m_Data->vids_right[i+1] > 0) && (!m_Data->vars_disable_right[i+1]))) {

            double YFactor = -win_height_d / (m_Data->max_right[i] - m_Data->min_right[i]);
            double YOffset = win_height_d - m_Data->min_right[i] * YFactor;
            double XFactor = win_width_d / (m_Data->max_right[i+1] - m_Data->min_right[i+1]);
            double XOffset = - m_Data->min_right[i+1] * XFactor;

            int ToSearchPoints = (m_Data->depth_right[i] < m_Data->depth_right[i+1]) ? m_Data->depth_right[i] : m_Data->depth_right[i+1];
            for (int Idx = 1; Idx < ToSearchPoints; Idx++) {
                int IdxY = m_Data->wr_poi_right[i] - Idx;
                if (IdxY < 0) IdxY = m_Data->buffer_depth + IdxY;
                int IdxX = m_Data->wr_poi_right[i+1] - Idx;
                if (IdxX < 0) IdxX = m_Data->buffer_depth + IdxX;

                double ValueY = m_Data->buffer_right[i][IdxY];
                double ValueX = m_Data->buffer_right[i+1][IdxX];
                if (!_isnan (ValueY) && !_isnan (ValueY)) {

                    double y_d = ValueY * YFactor + YOffset;
                    double x_d = ValueX * XFactor + XOffset;

                    y_d -= static_cast<double>(y);
                    y_d = y_d * y_d;
                    x_d -= static_cast<double>(x);
                    x_d = x_d * x_d;

                    double Distance = sqrt(y_d + x_d);
                    if (Distance < DistanceMin) {
                        DistanceMin = Distance;
                        IdxMin = Idx;
                        LeftOrRight = Right;
                        XMin = ValueY;
                        YMin = ValueX;
                        IMin = i;
                    }
                }
            }
        }
    }
    if (DistanceMin < DBL_MAX) {
        // We have found something
        *ret_Time = m_Data->t_current_buffer_end - IdxMin * m_Data->t_step;
        return true;
    }
    return false;
}

void OscilloscopeDrawArea::createActions(void)
{
    OnlineAct = new QAction(tr("&online"), this);
    OnlineAct->setStatusTip(tr("switch oscilloscope to online mode"));
    connect(OnlineAct, SIGNAL(triggered()), this, SLOT(OnlineSlot()));

    OfflineAct = new QAction(tr("o&ffline"), this);
    OfflineAct->setStatusTip(tr("switch the oscilloscope to offline mode"));
    connect(OfflineAct, SIGNAL(triggered()), this, SLOT(OfflineSlot()));

    AllOnlineAct = new QAction(tr("all oscilloscopes online"), this);
    AllOnlineAct->setStatusTip(tr("switch all oscilloscope to online mode"));
    connect(AllOnlineAct, SIGNAL(triggered()), this, SLOT(AllOnlineSlot()));

    AllOfflineAct = new QAction(tr("all oscilloscopes offline"), this);
    AllOfflineAct->setStatusTip(tr("switch all oscilloscope to offline mode"));
    connect(AllOfflineAct, SIGNAL(triggered()), this, SLOT(AllOfflineSlot()));

    ConfigAct = new QAction(tr("&config"), this);
    ConfigAct->setStatusTip(tr("configure the oscilloscope"));
    connect(ConfigAct, SIGNAL(triggered()), this, SLOT(ConfigSlot()));

    ZoomInAct = new QAction(tr("zoom &in"), this);
    ZoomInAct->setStatusTip(tr("zoom in"));
    connect(ZoomInAct, SIGNAL(triggered()), this, SLOT(ZoomInSlot()));

    ZoomOutAct = new QAction(tr("zoom &out"), this);
    ZoomOutAct->setStatusTip(tr("zoom out"));
    connect(ZoomOutAct, SIGNAL(triggered()), this, SLOT(ZoomOutSlot()));

    ZoomHistoryAct = new QAction(tr("zoom &history"), this);
    ZoomHistoryAct->setStatusTip(tr("zoom history"));
    connect(ZoomHistoryAct, SIGNAL(triggered()), this, SLOT(ZoomHistorySlot()));

    ZoomResetAct = new QAction(tr("Zoom &reset"), this);
    ZoomResetAct->setStatusTip(tr("zoom reset"));
    connect(ZoomResetAct, SIGNAL(triggered()), this, SLOT(ZoomResetSlot()));

    MetafileToClipboardAct = new QAction(tr("&metafile to clipboard"), this);
    MetafileToClipboardAct->setStatusTip(tr("copy the oscilloscope to the clipboard you can paste this to an other application"));
    connect(MetafileToClipboardAct, SIGNAL(triggered()), this, SLOT(MetafileToClipboardSlot()));

    SaveToFileAct = new QAction(tr("Save to file"), this);
    SaveToFileAct->setStatusTip(tr("save the oscilloscope buffer to a data file"));
    connect(SaveToFileAct, SIGNAL(triggered()), this, SLOT(SaveToFileSlot()));

    SetReferencePointAct = new QAction(tr("set reference &point"), this);
    SetReferencePointAct->setStatusTip(tr("set reference point"));
    connect(SetReferencePointAct, SIGNAL(triggered()), this, SLOT(SetReferencePointSlot()));

    ResetReferencePointAct = new QAction(tr("reset reference point"), this);
    ResetReferencePointAct->setStatusTip(tr("reset reference point"));
    connect(ResetReferencePointAct, SIGNAL(triggered()), this, SLOT(ResetReferencePointSlot()));

    DifferenceToReferencePointAct = new QAction(tr("&difference to reference point"), this);
    DifferenceToReferencePointAct->setStatusTip(tr("difference to reference poin"));
    connect(DifferenceToReferencePointAct, SIGNAL(triggered()), this, SLOT(DifferenceToReferencePointSlot()));

    SwitchTimeXYViewAct = new QAction(tr("&switch between time and xy view"), this);
    SwitchTimeXYViewAct->setStatusTip(tr("switch between time and xy view"));
    connect(SwitchTimeXYViewAct, SIGNAL(triggered()), this, SLOT(SwitchTimeXYViewSlot()));

    ClearAct = new QAction(tr("&clear data"), this);
    ClearAct->setStatusTip(tr("clear data"));
    connect(ClearAct, SIGNAL(triggered()), this, SLOT(ClearSlot()));

    YZoomAct = new QAction(tr("only &y axis zoom"), this);
    connect(YZoomAct, SIGNAL(triggered()), this, SLOT(YZoomSlot()));
    TimeZoomAct = new QAction(tr("only &time zoom"), this);
    connect(TimeZoomAct, SIGNAL(triggered()), this, SLOT(TimeZoomSlot()));
    YandTimeZoomAct = new QAction(tr("y axis &and time zoom"), this);
    connect(YandTimeZoomAct, SIGNAL(triggered()), this, SLOT(YandTimeZoomSlot()));
    NoZoomAct = new QAction(tr("no zoom"), this);
    connect(NoZoomAct, SIGNAL(triggered()), this, SLOT(NoZoomSlot()));
}


void OscilloscopeDrawArea::OnlineSlot (void)
{
    m_OscilloscopeWidget->GoOnline(GetSimulatedTimeSinceStartedInNanoSecond());
}

void OscilloscopeDrawArea::OfflineSlot (void)
{
    m_OscilloscopeWidget->GoOffline();
}

void OscilloscopeDrawArea::AllOnlineSlot()
{
    m_OscilloscopeWidget->GoAllOnline();
}

void OscilloscopeDrawArea::AllOfflineSlot()
{
    m_OscilloscopeWidget->GoAllOffline();
}

void OscilloscopeDrawArea::ConfigSlot (void)
{
    m_OscilloscopeWidget->ConfigDialog();
}

void OscilloscopeDrawArea::ZoomInSlot (void)
{
    m_OscilloscopeWidget->ZoomIn();
}

void OscilloscopeDrawArea::ZoomOutSlot (void)
{
    m_OscilloscopeWidget->ZoomOut();
}

void OscilloscopeDrawArea::ZoomHistorySlot (void)
{
    OscilloscopeZoomHistoryDialog Dlg (m_Data);

    if (Dlg.exec() == QDialog::Accepted) {
        m_OscilloscopeWidget->ZoomIndex (Dlg.GetSelectedZoomRow());
    }
}

void OscilloscopeDrawArea::ZoomResetSlot (void)
{
    m_OscilloscopeWidget->ZoomReset();
}

#ifdef QT_SVG_LIB
void OscilloscopeDrawArea::MetafileToClipboardSlot (void)
{
    m_OscilloscopeWidget->PrintSvgToClipboard();
}
#endif

void OscilloscopeDrawArea::SaveToFileSlot (void)
{
    QString FileName = FileDialog::getSaveFileName(this, "save as", QString(), QString (DATA_EXT));
    if (!FileName.isEmpty()) {
        WriteOsziBuffer2StimuliFile(m_Data, QStringToConstChar(FileName), FILE_FORMAT_DAT_TE,
                                    QStringToConstChar(m_OscilloscopeWidget->GetWindowTitle()));
    }
}

void OscilloscopeDrawArea::ResetReferencePointSlot (void)
{
    if (!m_Data->ref_point_flag) {
        m_Data->time_step_ref_point = m_Data->time_step_cursor;
        m_Data->t_ref_point = m_Data->t_cursor;
        m_Data->ref_point_flag = 1;
    } else {
        m_Data->ref_point_flag = 0;
        m_Data->diff_to_ref_point_flag = 0;
    }
    m_OscilloscopeWidget->UpdateDrawArea();
    m_OscilloscopeWidget->UpdateAllUsedDescs(true);
}

void OscilloscopeDrawArea::DifferenceToReferencePointSlot (void)
{
    if (!m_Data->diff_to_ref_point_flag) {
        m_Data->diff_to_ref_point_flag = 1;
    } else {
        m_Data->diff_to_ref_point_flag = 0;
    }
    m_OscilloscopeWidget->UpdateDrawArea();
    m_OscilloscopeWidget->UpdateAllUsedDescs(true);
}

void OscilloscopeDrawArea::SetReferencePointSlot (void)
{
    m_OscilloscopeWidget->SetReferencePoint();
}
void OscilloscopeDrawArea::SwitchTimeXYViewSlot()
{
    if (m_Data->xy_view_flag) m_Data->xy_view_flag = 0;
    else m_Data->xy_view_flag = 1;
    m_OscilloscopeWidget->UpdateYAxises();
    m_OscilloscopeWidget->UpdateAllUsedDescs(false);
    m_OscilloscopeWidget->UpdateTimeAxis();
    update();
}

void OscilloscopeDrawArea::ClearSlot()
{
    m_ClearFlag = true;
    m_OscilloscopeWidget->ClearAllData();
    m_OscilloscopeWidget->UpdateYAxises();
    m_OscilloscopeWidget->UpdateAllUsedDescs(true);
    m_OscilloscopeWidget->UpdateTimeAxis();
    update ();
}


void OscilloscopeDrawArea::YZoomSlot (void)
{
    if (abs(m_y1 - m_y2) > 5)
    {
        m_OscilloscopeWidget->NewZoom (1, 0, width(), height(), m_x1, m_y1, m_x2, m_y2);
        update ();
        m_OscilloscopeWidget->UpdateYAxises();
    }
}

void OscilloscopeDrawArea::TimeZoomSlot (void)
{
    if (abs(m_x1 - m_x2) > 5)
    {
        m_OscilloscopeWidget->NewZoom (0, 1, width(), height(), m_x1, m_y1, m_x2, m_y2);
        update ();
        m_OscilloscopeWidget->UpdateTimeAxis();
    }
}

void OscilloscopeDrawArea::YandTimeZoomSlot (void)
{
    if ((abs(m_x1 - m_x2) > 5) && (abs(m_y1 - m_y2) > 5))
    {
        m_OscilloscopeWidget->NewZoom (1, 1, width(), height(), m_x1, m_y1, m_x2, m_y2);
        update ();
        m_OscilloscopeWidget->UpdateYAxises();
        m_OscilloscopeWidget->UpdateTimeAxis();
    }
    else if (abs(m_x1 - m_x2) > 5)
    {
        TimeZoomSlot();
    }
    else if (abs(m_y1 - m_y2) > 5)
    {
        YZoomSlot();
    }
}

void OscilloscopeDrawArea::NoZoomSlot (void)
{
}

void OscilloscopeDrawArea::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText()) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void OscilloscopeDrawArea::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasText()) {
        const QMimeData *mime = event->mimeData();
        DragAndDropInfos Infos (mime->text());

        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
            m_OscilloscopeWidget->DropSignal(Left, -1, &Infos);
        }
    } else {
        event->ignore();
    }
}


void OscilloscopeDrawArea::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    if (m_Data->state) {
        menu.addAction (OfflineAct);
        menu.addAction (AllOfflineAct);
    } else {
        menu.addAction (OnlineAct);
        menu.addAction (AllOnlineAct);
    }
    menu.addAction (ConfigAct);

    if (!m_Data->xy_view_flag) {  // Zoom is not able inside xy view
        ZoomInAct->setEnabled (m_Data->zoom_pos > 0);
        menu.addAction (ZoomInAct);

        ZoomOutAct->setEnabled ((m_Data->zoom_pos < 9) && (m_Data->y_zoom[m_Data->zoom_pos+1] > 0.0));
        menu.addAction (ZoomOutAct);
        menu.addAction (ZoomHistoryAct);
        menu.addAction (ZoomResetAct);
    }
#ifdef QT_SVG_LIB
    menu.addAction (MetafileToClipboardAct);
#endif
    menu.addAction (SaveToFileAct);

    if ((m_Data->state == 0) && !m_Data->xy_view_flag) {
        if (m_Data->ref_point_flag) {
            menu.addAction (ResetReferencePointAct);
            menu.addAction (DifferenceToReferencePointAct);
        } else {
            menu.addAction (SetReferencePointAct);
        }
    }
    if (m_Data->xy_view_flag) {
        SwitchTimeXYViewAct->setText("&switch from xy to time view");
    } else {
        SwitchTimeXYViewAct->setText("&switch from time to xy view");
    }
    menu.addAction (SwitchTimeXYViewAct);
    menu.addAction (ClearAct);

    menu.exec(event->globalPos());
}

void OscilloscopeDrawArea::mouseReleaseEvent(QMouseEvent * event)
{
    switch (event->button ()) {
    case Qt::RightButton:
        break;
    case Qt::LeftButton:
        if (m_MouseGrabbed) {
            releaseMouse();
            DEBUG_PRINT (this, "delete zoom\n");
            m_OnlyUpdateZoomRectFlag = LAST_ZOOM_RECT;
            //this->repaint();
            this->update();
            //m_OnlyUpdateZoomRectFlag = 0;
            m_MouseGrabbed = 0;
            event->accept();

            QMenu menu(this);
            menu.addAction (YZoomAct);
            menu.addAction (TimeZoomAct);
            menu.addAction (YandTimeZoomAct);
            menu.addAction (NoZoomAct);

            menu.exec(GetEventGlobalPos(event));

            update();
        }
        break;
    default:
        break;
    }
}

void OscilloscopeDrawArea::mousePressEvent(QMouseEvent * event)
{
    switch (event->button ()) {
    case Qt::RightButton:
        break;
    case Qt::LeftButton:
        if ((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier) {
            // Zoom
            if (!m_Data->xy_view_flag) {
                grabMouse();
                m_MouseGrabbed = 1;
                m_x2 = m_x1 = GetEventXPos(event);
                m_y2 = m_y1 = GetEventYPos(event);
                m_OnlyUpdateZoomRectFlag = FIRST_ZOOM_RECT;
                DEBUG_PRINT (this, "first paint zoom\n");
                //this->repaint();
                this->update();
                //m_OnlyUpdateZoomRectFlag = 0;
                event->accept();
            }
        } else {
            DEBUG_PRINT (this, "setze Cursor\n");
            if (m_Data->xy_view_flag) {
                uint64_t Time;
                if (PickingXYPoint(GetEventXPos(event), GetEventYPos(event), &Time)) {
                    m_OscilloscopeWidget->set_t_cursor(Time, false);
                }
            } else {
                int x_pos = GetEventXPos(event);
                m_OscilloscopeWidget->SetCursorAll (m_Data->t_window_start + my_umuldiv64(static_cast<uint64_t>(x_pos), m_Data->t_window_end - m_Data->t_window_start, static_cast<uint64_t>(width())), true);
            }
            event->accept();
        }
        break;
    default:
        break;
    }
}


void OscilloscopeDrawArea::mouseMoveEvent(QMouseEvent * event)
{
    if (m_MouseGrabbed) {
        int x = GetEventXPos(event);
        int y = GetEventYPos(event);
        if ((m_x2 != x) || (m_y2 != y)) {
            DEBUG_PRINT (this, "delete old zoom\n");
            m_OnlyUpdateZoomRectFlag = NEXT_ZOOM_RECT;
            //this->repaint();
            this->update();
            //m_OnlyUpdateZoomRectFlag = 0;
            m_x2 = x;
            m_y2 = y;
            DEBUG_PRINT (this, "  than paint new zoom\n");
            m_OnlyUpdateZoomRectFlag = NEXT_ZOOM_RECT;
            //this->repaint();
            this->update();
            //m_OnlyUpdateZoomRectFlag = 0;
        }
        event->accept();
    }
}

void OscilloscopeDrawArea::UpdateCursor ()
{
    //m_OnlyUpdateCursorFlag = FIRST_CURSOR;
    //repaint ();
    this->update();
    //m_OnlyUpdateCursorFlag = NO_CURSOR;
}

bool OscilloscopeDrawArea::SetBackgroundImage(QString &FileName)
{
    bool Ret = false;
    if (FileName.length() > 0) {
        if (m_BackgroundImage == nullptr) {
            m_BackgroundImage = new QImage();
        }
        if (m_BackgroundImage != nullptr) {
            if (!m_BackgroundImage->load(FileName)) {
                delete m_BackgroundImage;
                m_BackgroundImage = nullptr;
                ThrowError (1, "cannot load background image \"%s\" for oscilloscope window \"%s\"",
                           QStringToConstChar(FileName), QStringToConstChar(m_OscilloscopeWidget->GetWindowTitle()));
            } else {
                m_ClearFlag = true;
                Ret = true;
            }
        }
    } else {
        if (m_BackgroundImage != nullptr) {
            delete m_BackgroundImage;
            m_BackgroundImage = nullptr;
            m_ClearFlag = true;
            Ret = true;
        }
    }
    return Ret;
}

// only for debugging
#ifdef DEBUG_PRINT_TO_FILE
void OscilloscopeDrawAreaDebugPrint (OscilloscopeDrawArea *DrawArea, const char *txt, ...)
{
    va_list args;
    va_start (args, txt);

    if (DrawArea->m_FileHandle == nullptr) {
        char Name[MAX_PATH];
#ifdef _WIN32
        PrintFormatToString (Name, sizeof(Name), "c:\\tmp\\%s_log.txt", DrawArea->GetOscilloscopeWidget()->GetWindowTitle().toLatin1().data());
#else
        PrintFormatToString (Name, sizeof(Name), "/tmp/%s_log.txt", DrawArea->GetOscilloscopeWidget()->GetWindowTitle().toLatin1().data());
#endif
        DrawArea->m_FileHandle = fopen (Name, "w");
        if (DrawArea->m_FileHandle != nullptr) {
            setvbuf (DrawArea->m_FileHandle, nullptr, _IOLBF, 1000);
        }
    }
    if (DrawArea->m_FileHandle != nullptr) {
        //fputs (txt, m_FileHandle);
        //fputs (Buffer, m_FileHandle);
        vfprintf (DrawArea->m_FileHandle, txt, args);
        fflush(DrawArea->m_FileHandle);
    }
    va_end (args);
}
#endif


