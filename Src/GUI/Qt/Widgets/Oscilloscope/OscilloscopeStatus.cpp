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


#include "OscilloscopeStatus.h"
extern "C" {
#include "Config.h"
#include "Blackboard.h"
}

OscilloscopeStatus::OscilloscopeStatus(OSCILLOSCOPE_DATA *par_Data, QWidget *parent) : QWidget(parent)
{
    m_Data = par_Data;
    setMaximumHeight(OSCILLOSCOPE_TIME_AXIS_HIGHT);
    setBackgroundRole(QPalette::Window);
    setAutoFillBackground(true);
}

OscilloscopeStatus::~OscilloscopeStatus()
{

}

void OscilloscopeStatus::paintEvent(QPaintEvent * /* event */)
{
    int win_width, win_height;
    char txt[BBVARI_NAME_SIZE+100];
    double time;

    QPainter painter(this);

    win_width = width ();
    win_height = height ();
    switch (m_Data->state) {
    case 2:
    case 3:
        sprintf (txt, "%s %c %.2lf",
                 m_Data->name_trigger,
                 m_Data->trigger_flank,
                 m_Data->trigger_value);
        break;
    case 1:
        strcpy (txt, "Online");
        break;
    default:
        if (m_Data->ref_point_flag) {
            if (m_Data->t_cursor >= m_Data->t_ref_point) {
                time = static_cast<double>(m_Data->t_cursor - m_Data->t_ref_point) / TIMERCLKFRQ;
            } else {
                time = -static_cast<double>(m_Data->t_ref_point - m_Data->t_cursor) / TIMERCLKFRQ;
            }
        } else {
            time = static_cast<double>(m_Data->t_cursor) / TIMERCLKFRQ;
        }
        if (m_Data->t_step < 1000) {
            sprintf (txt, "Offline %.6lfs", time);
        } else {
            sprintf (txt, "Offline %.4lfs", time);
        }
    }
    painter.drawText (rect(), Qt::AlignCenter, tr(txt));
    painter.setPen(Qt::darkGray);
    painter.drawLine (win_width-1, 0, 0, 0);
    painter.drawLine (0, 0, 0, win_height-1);
    painter.drawLine (0, win_height-1, win_width-1, win_height-1);
    painter.drawLine (win_width-1, win_height-1, win_width-1, 0);
}
