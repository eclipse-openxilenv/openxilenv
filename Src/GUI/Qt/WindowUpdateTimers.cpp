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


#include "WindowUpdateTimers.h"

WindowUpdateTimers::WindowUpdateTimers(QObject *parent) : QObject(parent)
{
    m_fastTimer = new QTimer(this);
    m_mediumTimer = new QTimer(this);
    m_slowTimer = new QTimer(this);
    m_noTimer = new QTimer(this);
    m_fastTimer->setInterval(100);
    m_mediumTimer->setInterval(500);
    m_slowTimer->setInterval(1000);

    m_slowTimer->start();
    m_mediumTimer->start();
    m_fastTimer->start();
}

WindowUpdateTimers::~WindowUpdateTimers()
{
}

QTimer* WindowUpdateTimers::updateTimer(InterfaceWidgetPlugin::UpdateRate arg_updateRate)
{
    switch (arg_updateRate){
    case InterfaceWidgetPlugin::FastUpdateWindow:
        return m_fastTimer;
    case InterfaceWidgetPlugin::MediumUpdateWindow:
        return m_mediumTimer;
    case InterfaceWidgetPlugin::SlowUpdateWindow:
        return m_slowTimer;
    case InterfaceWidgetPlugin::NoneUpdateWindow:
        return m_noTimer;
    }
    return m_noTimer;
}

void WindowUpdateTimers::stopAllTimer() {
    m_slowTimer->stop();
    m_mediumTimer->stop();
    m_fastTimer->stop();
}
