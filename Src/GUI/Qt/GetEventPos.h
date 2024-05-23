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


#pragma once

#include <QPoint>
#include <QMouseEvent>
#include <QDropEvent>

// this file is only necessary to switch between Qt 5.x to 6.x

static inline int GetEventXPos(QMouseEvent *par_Event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return (int)par_Event->position().x();
#else
    return (int)par_Event->pos().x();
#endif
}

static inline int GetEventYPos(QMouseEvent *par_Event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return (int)par_Event->position().y();
#else
    return (int)par_Event->pos().y();
#endif
}


static QPoint GetEventGlobalPos(QMouseEvent *par_Event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF PointF = par_Event->globalPosition();
    QPoint Point(PointF.x(), PointF.y());
    return Point;
#else
    return par_Event->globalPos();
#endif
}

static inline int GetDropEventXPos(QDropEvent *par_Event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return (int)par_Event->position().x();
#else
    return (int)par_Event->pos().x();
#endif
}

static inline int GetDropEventYPos(QDropEvent *par_Event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return (int)par_Event->position().y();
#else
    return (int)par_Event->pos().y();
#endif
}
