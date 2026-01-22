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


#ifndef OSCILLOSCOPEDRAWAREA_H
#define OSCILLOSCOPEDRAWAREA_H

#include <stdio.h>
#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QMenu>
#include <QContextMenuEvent>
#include <QApplication>

#include "OscilloscopeData.h"


class OscilloscopeWidget;

class OscilloscopeDrawArea : public QWidget
{
    Q_OBJECT
public:
    explicit OscilloscopeDrawArea(OscilloscopeWidget *par_OscilloscopeWidget, OSCILLOSCOPE_DATA *par_Data, QWidget *parent = nullptr);
    ~OscilloscopeDrawArea() Q_DECL_OVERRIDE;

    void UpdateCursor ();

    bool SetBackgroundImage(QString &FileName);

    void paint(QPainter &painter);
    void paint_cursor(QPainter *painter);

public slots:
    void OnlineSlot (void);
    void OfflineSlot (void);
    void AllOnlineSlot (void);
    void AllOfflineSlot (void);
    void ConfigSlot (void);
    void ZoomInSlot (void);
    void ZoomOutSlot (void);
    void ZoomHistorySlot (void);
    void ZoomResetSlot (void);
#ifdef QT_SVG_LIB
    void MetafileToClipboardSlot (void);
#endif
    void SaveToFileSlot (void);
    void ResetReferencePointSlot (void);
    void DifferenceToReferencePointSlot (void);
    void SetReferencePointSlot (void);
    void SwitchTimeXYViewSlot(void);
    void ClearSlot(void);

    void YZoomSlot (void);
    void TimeZoomSlot (void);
    void YandTimeZoomSlot (void);
    void NoZoomSlot (void);
    OscilloscopeWidget *GetOscilloscopeWidget() { return m_OscilloscopeWidget; }

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;

    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent * event) Q_DECL_OVERRIDE;

    void PaintZoomRectangle (QPainter *painter);
    void PaintCursor (QPainter *painter, bool xor_flag);

private:
    void PaintTimeLineToPixmap(QPainter &painter, QRect par_Rec);
    void PaintXYToPixmap(QPainter &painter);
    bool PickingXYPoint(int x, int y, uint64_t *ret_Time);
    QPen CursorPen;
    QPen RefCursorPen;

    void createActions (void);
    QAction *OnlineAct;
    QAction *OfflineAct;
    QAction *AllOnlineAct;
    QAction *AllOfflineAct;
    QAction *ConfigAct;
    QAction *ZoomInAct;
    QAction *ZoomOutAct;
    QAction *ZoomHistoryAct;
    QAction *ZoomResetAct;
    QAction *MetafileToClipboardAct;
    QAction *SaveToFileAct;
    QAction *SetReferencePointAct;
    QAction *ResetReferencePointAct;
    QAction *DifferenceToReferencePointAct;
    QAction *SwitchTimeXYViewAct;
    QAction *ClearAct;

    QAction *YZoomAct;
    QAction *TimeZoomAct;
    QAction *YandTimeZoomAct;
    QAction *NoZoomAct;


    OSCILLOSCOPE_DATA *m_Data;

    OscilloscopeWidget *m_OscilloscopeWidget;

    int m_MouseGrabbed;
    int m_x1;
    int m_y1;
    int m_x2;
    int m_y2;
    enum OnlyUpdateZoomRectFlag {NO_ZOOM_RECT, FIRST_ZOOM_RECT, NEXT_ZOOM_RECT, LAST_ZOOM_RECT} m_OnlyUpdateZoomRectFlag;
    bool m_IsX11;

    QColor m_BackgroundColor;

    QPixmap *m_BufferPixMap;  // Buffer only for xy view tis will be deleted if the window size will be changed
    bool m_ClearFlag;         // If this is true delete m_BufferPixMap and paint everything new
    QImage *m_BackgroundImage;

public:
    FILE *m_FileHandle;
};

#endif // OSCILLOSCOPEDRAWAREA_H
