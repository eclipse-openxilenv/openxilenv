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


#ifndef OSCILLOSCOPESTATUS_H
#define OSCILLOSCOPESTATUS_H

#include <QWidget>
#include <QPainter>

#include "OscilloscopeData.h"

class OscilloscopeStatus : public QWidget
{
    Q_OBJECT
public:
    explicit OscilloscopeStatus(OSCILLOSCOPE_DATA *par_Data, QWidget *parent = nullptr);
    ~OscilloscopeStatus() Q_DECL_OVERRIDE;

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    OSCILLOSCOPE_DATA *m_Data;

};

#endif // OSCILLOSCOPESTATUS_H
