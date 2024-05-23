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


#include "StatusBar.h"

#include <QPainter>
#include <QIcon>
#include <QDate>
#include <QResizeEvent>

#define OVERSIZE_CAR  4
#define OFFSET_CAR    (-2)
#define SWITCH_IMAGE_CYCLE  4

void StatusBar::BuildPixmaps()
{
    m_CarHeight = m_Height + OVERSIZE_CAR;
    m_CarWidth = m_Height + OVERSIZE_CAR;
    QSize Size(m_CarWidth, m_CarHeight);
    QDate loc_date = QDate::currentDate();
    if(((loc_date.month() == 12) && (loc_date.day() >= 10)) || ((loc_date.month() == 1) &&(loc_date.day() <= 6))) {
        m_Pixmaps[0] = QIcon (":/Icons/XMas_1.ico").pixmap(Size);
        m_Pixmaps[1] = QIcon (":/Icons/XMas_2.ico").pixmap(Size);
        m_Pixmaps[2] = QIcon (":/Icons/XMas_3.ico").pixmap(Size);
        m_Pixmaps[3] = QIcon (":/Icons/XMas_4.ico").pixmap(Size);
        m_ImagesType = MOVING_XMAS_IMAGES;
    } else {
        m_Pixmaps[0] = QIcon (":/Icons/Car_1.ico").pixmap(Size);
        m_Pixmaps[1] = QIcon (":/Icons/Car_2.ico").pixmap(Size);
        m_Pixmaps[2] = QIcon (":/Icons/Car_3.ico").pixmap(Size);
        m_Pixmaps[3] = QIcon (":/Icons/Car_4.ico").pixmap(Size);
        m_ImagesType = MOVING_CAR_IMAGES;
    }
}

StatusBar::StatusBar(QWidget *parent)
    : QStatusBar{parent}
{
    m_ImageNo = 0;
    m_CurrentPos = 0.0;
    m_Height = 32;
    m_CarHeight = 32;
    m_CarWidth = 32;
    m_ImageSwitchCounter = 0;

    m_CarIsVisable = true;
    m_FirstPaint = true;
    BuildPixmaps();
}

void StatusBar::SetSpeedPerCent(double par_Speed)
{
    if (par_Speed < -100.0) {
        m_Speed = -100.0;
    } else if (par_Speed > 100.0) {
        m_Speed = 100.0;
    } else {
        m_Speed = par_Speed;
    }
}

void StatusBar::SetEnableCar(bool par_Enable)
{
    m_CarIsVisable = par_Enable;
}

bool StatusBar::IsEnableCar()
{
    return m_CarIsVisable;
}

void StatusBar::MoveCar()
{
    if (m_Speed < 0.0) {
        // drive backwarts
        int OldPos = (int)m_CurrentPos;
        m_CurrentPos = m_CurrentPos - m_Speed * 10.0;
        int Pos = m_CurrentPos;
        update(OldPos, 0, (Pos - OldPos) + m_CarWidth, m_Height);
        if(m_CurrentPos > (m_Width + m_CarWidth)) {
            m_CurrentPos = -m_CarWidth;
        }
        m_ImageNo++;
        switch(m_ImagesType) {
        case MOVING_CAR_IMAGES:
            if (m_ImageNo >= 3) m_ImageNo = 0;
            break;
        case MOVING_XMAS_IMAGES:
            if (m_ImageNo >= 4) m_ImageNo = 0;
            break;
        }
    } else if (m_Speed > 0.0) {
        // drive forwarts
        int OldPos = (int)m_CurrentPos;
        m_CurrentPos = m_CurrentPos - m_Speed * 10.0;
        int Pos = m_CurrentPos;
        update(m_CurrentPos, 0, (OldPos - Pos) + m_CarWidth + 1, m_Height);
        if(m_CurrentPos < -m_CarWidth) {
            m_CurrentPos = m_Width + m_CarWidth;
        }
        m_ImageSwitchCounter++;
        if (m_ImageSwitchCounter > SWITCH_IMAGE_CYCLE) {
            m_ImageSwitchCounter = 0;
            m_ImageNo++;
        }
        switch(m_ImagesType) {
        case MOVING_CAR_IMAGES:
            if (m_ImageNo >= 3) m_ImageNo = 0;
            break;
        case MOVING_XMAS_IMAGES:
            if (m_ImageNo >= 4) m_ImageNo = 0;
            break;
        }
    } else {
        m_ImageNo = 3;
    }
}

void StatusBar::paintEvent(QPaintEvent *event)
{
    QStatusBar::paintEvent(event);
    if (m_CarIsVisable && !m_FirstPaint) {
        QPainter Painter(this);
        Painter.drawPixmap((int)m_CurrentPos, OFFSET_CAR, m_Pixmaps[m_ImageNo]);
    }
    m_FirstPaint = false;
}

void StatusBar::resizeEvent(QResizeEvent *event)
{
    if (m_Height != event->size().height()) {
        m_Height = event->size().height();
        BuildPixmaps();
    }
    m_Width = event->size().width();
    QStatusBar::resizeEvent(event);
}
