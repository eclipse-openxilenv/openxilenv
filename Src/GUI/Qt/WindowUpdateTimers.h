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


#ifndef WINDOWUPDATETIMERS_H
#define WINDOWUPDATETIMERS_H

#include <QObject>
#include <QTimer>
#include "InterfaceWidgetPlugin.h"

class WindowUpdateTimers : public QObject
{
    Q_OBJECT
public:
    explicit WindowUpdateTimers(QObject *parent = nullptr);
    ~WindowUpdateTimers();
    QTimer* updateTimer(InterfaceWidgetPlugin::UpdateRate arg_updateRate);
    void stopAllTimer();

signals:

public slots:

private:
    QTimer *m_fastTimer;
    QTimer *m_mediumTimer;
    QTimer *m_slowTimer;
    QTimer *m_noTimer;
};

#endif // WINDOWUPDATETIMERS_H
