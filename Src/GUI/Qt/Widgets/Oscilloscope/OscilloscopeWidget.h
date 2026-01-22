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


#ifndef OSCILLOSCOPEWIDGET_H
#define OSCILLOSCOPEWIDGET_H

#ifdef __cplusplus

#include <QWidget>
#include <QGridLayout>

#include "OscilloscopeCyclic.h"
#include "OscilloscopeDescFrame.h"
#include "OscilloscopeDrawArea.h"
#include "OscilloscopeStatus.h"
#include "OscilloscopeTimeAxis.h"
#include "OscilloscopeYAxis.h"
#include "OscilloscopeData.h"
#include "MdiWindowWidget.h"
#include "DragAndDrop.h"


class OscilloscopeWidget : public MdiWindowWidget
{
    Q_OBJECT
public:
    explicit OscilloscopeWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent = nullptr);
    ~OscilloscopeWidget() Q_DECL_OVERRIDE;

    void ClearAllData();
    void GoOnline (uint64_t par_StartTime);
    void GoOffline (bool par_WithTime = false, uint64_t par_EndTime = 0);
    void GoAllOnline ();
    void GoAllOffline ();
    int IsOnline ();
    void SetState(int par_NewState) { m_Data->state = par_NewState; }

    void UpdateTimeAxis (void);
    virtual bool writeToIni() Q_DECL_OVERRIDE;
    virtual bool readFromIni() Q_DECL_OVERRIDE;

    void UpdateLeftYAxis ();
    void UpdateRightYAxis ();
    void UpdateYAxises ();

    void UpdateAllUsedDescs (bool OnlyUsed);
    void UpdateStatus ();
    void UpdateDrawArea ();

    void SetReferencePoint();
    void set_reference_point();

    void SetCursorAll (uint64_t par_NewTCursor, bool par_MoveClippingFlag);
    void set_t_cursor (uint64_t par_NewTCursor, bool par_MoveClippingFlag);

    void set_t_window_start_end (uint64_t ts, uint64_t te);

    void ZoomIn (void);
    void ZoomOut (void);
    void ZoomReset (void);
    void ZoomIndex (int par_Index);
    void NewZoom (int par_YZoomFlag, int par_TimeZoomFlag, int par_WinWidth, int par_WinHeight, int par_x1, int par_y1, int par_x2, int par_y2);
    void ConfigDialog (void);

    void ResizeDescFrames (void);

    int DropSignal (enum Side par_Side, int par_Pos, DragAndDropInfos *par_DropInfos);
    int AddSignal (enum Side par_Side, int par_Pos, QString &par_Name, bool par_MinMaxVaild, double par_Min, double par_Max,
                   QColor par_Color, int par_LineWidth, DragAndDropDisplayMode par_DisplayMode, DragAndDropAlignment par_Alignment);
    virtual CustomDialogFrame* dialogSettings(QWidget *arg_parent) Q_DECL_OVERRIDE;

    void DeleteSignal(enum Side par_Side, int par_Number);

    QList<QColor> GetAllUsedColors();
#ifdef QT_SVG_LIB
    void PrintSvgToClipboard();
#endif

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    virtual void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    virtual void keyPressEvent(QKeyEvent * event) Q_DECL_OVERRIDE;

signals:

public slots:
    virtual void CyclicUpdate() Q_DECL_OVERRIDE;

private slots:
    virtual void changeColor(QColor arg_color) Q_DECL_OVERRIDE;
    virtual void changeFont(QFont arg_font) Q_DECL_OVERRIDE;
    virtual void changeWindowName(QString arg_name) Q_DECL_OVERRIDE;
    virtual void changeVariable(QString arg_variable, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void changeVaraibles(QStringList arg_variables, bool arg_visible) Q_DECL_OVERRIDE;
    virtual void resetDefaultVariables(QStringList arg_variables) Q_DECL_OVERRIDE;

private:
    void SelectNextSignal();
    void SelectPreviosSignal();
    void DeleteSignal();
    void EditSelectedSignal();
    void SelectLeftSide();
    void SelectRightSide();

    void new_zoom (void);
    void SetWindowStartTime (uint64_t par_WindowStartTime);
    void SetWindowEndTime (uint64_t par_WindowEndTime);
    void OnOffDescrFrame();

    void CheckTrigger();

    void CheckIfSignalIsSelected();

    OscilloscopeDescFrame *m_RightDescFrame;
    OscilloscopeDescFrame *m_LeftDescFrame;
    OscilloscopeYAxis *m_RightYAxis;
    OscilloscopeYAxis *m_LeftYAxis;
    OscilloscopeStatus *m_RightStatus;
    OscilloscopeStatus *m_LeftStatus;
    OscilloscopeTimeAxis *m_TimeAxis;
    OscilloscopeDrawArea *m_DrawArea;

    QGridLayout *m_Layout;

    // This structure will use also inside OszilloscopeCycic.c
    OSCILLOSCOPE_DATA *m_Data;
    OSCILLOSCOPE_DATA m_DataStore;

    int m_UpdateDescsCounter;
    int m_UpdateDescsRate;

    double m_MaxUpdateTimeWidth;  // Only for debugging

};
#endif

#endif // OSCILLOSCOPEWIDGET_H
