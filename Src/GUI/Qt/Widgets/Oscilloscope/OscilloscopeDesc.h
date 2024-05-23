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


#ifndef OSCILLOSCOPEDESC_H
#define OSCILLOSCOPEDESC_H

#include <QWidget>
#include <QPainter>
#include <QEvent>

#include "OscilloscopeData.h"

#define OSCILLOSCOPE_DESC_HIGHT  45

class OscilloscopeDescFrame;

enum Side { Left, Right};

class OscilloscopeDesc : public QWidget
{
    Q_OBJECT
public:
    explicit OscilloscopeDesc(OscilloscopeDescFrame *par_OscilloscopeDescFrame, OSCILLOSCOPE_DATA *par_Data, enum Side par_Side, int par_Number, QWidget *parent = nullptr);
    ~OscilloscopeDesc() Q_DECL_OVERRIDE;

signals:
    void variableForStandardDialog(QStringList arg_variableName, bool arg_showAllVariable, bool arg_multiSelect, QColor arg_color, QFont arg_font = QFont());
public slots:
    void openDialog();
    void deleteSignal();
    void customContextMenu(QPoint arg_point);

protected:
    // Drag&Drop
    virtual void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent* event) Q_DECL_OVERRIDE;

    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent * event) Q_DECL_OVERRIDE;

    void mouseDoubleClickEvent(QMouseEvent *arg_event) Q_DECL_OVERRIDE;

    int read_value_desc_txt (char *txt, int left_right, int pos, int *pcolor);
    int ValueToString (int Pos,
                       const char *Prefix, int left_right, int Vid, double Value,
                       int dec_phys_flag, int dec_hex_bin_flag, int Type,
                       char *String, int *pcolor);
    int CalcDiffEnumString (double Value, int left_right, int pos,
                            char *DiffEnumStr, int maxc,  int *pcolor);
    void BuildBinaryString (const char *Prefix, uint64_t Value, int Type, char *String);
    void BuildHexString (const char *Prefix, uint64_t Value, int Type, char *String);

private slots:
    void BlackboardInfos();

private:
    int m_Number;
    enum Side m_Side;
    OSCILLOSCOPE_DATA *m_Data;

    OscilloscopeDescFrame *m_OscilloscopeDescFrame;

    QPoint m_startDragPosition;

};

#endif // OSCILLOSCOPEDESC_H
