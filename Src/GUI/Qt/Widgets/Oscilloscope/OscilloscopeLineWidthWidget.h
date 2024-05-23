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


#ifndef OSCILLOSCOPELINEWIDTHWIDGET_H
#define OSCILLOSCOPELINEWIDTHWIDGET_H

#include <QWidget>

class OscilloscopeLineWidthWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OscilloscopeLineWidthWidget(QWidget *parent = nullptr);
    ~OscilloscopeLineWidthWidget() Q_DECL_OVERRIDE;
    void SetLineWidthAndColor (int par_LineWidth, QColor &par_Color);

    QSize sizeHint() const Q_DECL_OVERRIDE;

signals:

public slots:
    void setLineWidth(int arg_lineWidth);
    void setLineColor(QColor arg_lineColor);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

private:
    int m_LineWidth;
    QColor m_Color;

};

#endif // OSCILLOSCOPELINEWIDTHWIDGET_H
