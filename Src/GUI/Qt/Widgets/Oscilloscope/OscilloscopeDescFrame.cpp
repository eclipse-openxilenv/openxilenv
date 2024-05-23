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


#include "OscilloscopeDescFrame.h"
#include "OscilloscopeWidget.h"


OscilloscopeDescFrame::OscilloscopeDescFrame(OscilloscopeWidget *par_OscilloscopeWidget, OSCILLOSCOPE_DATA *par_Data, enum Side par_Side, QWidget *parent) : QWidget(parent)
{
    m_OscilloscopeWidget = par_OscilloscopeWidget;
    m_Data = par_Data;
    m_Side = par_Side;
    setMaximumWidth(OSCILLOSCOPE_DESC_FRAME_WIDTH);
    setBackgroundRole(QPalette::Window);
    setAutoFillBackground(true);

    for (int y = 0; y < MAX_SIGNALS_INSIDE_FRAME; y++) {
        m_Desc[y] = new OscilloscopeDesc (this, m_Data, m_Side, y, this);
        m_Desc[y]->move (0, y * OSCILLOSCOPE_DESC_HIGHT);
        connect(m_Desc[y], SIGNAL(variableForStandardDialog(QStringList,bool,bool,QColor,QFont)), this, SIGNAL(variableForStandardDialog(QStringList,bool,bool,QColor,QFont)));
    }
}

OscilloscopeDescFrame::~OscilloscopeDescFrame()
{

}

void OscilloscopeDescFrame::resizeEvent(QResizeEvent * /* event */)
{
    int WinWidth =  width ();
    for (int y = 0; y < MAX_SIGNALS_INSIDE_FRAME; y++) {
        m_Desc[y]->resize (WinWidth, OSCILLOSCOPE_DESC_HIGHT);
    }
}


 void OscilloscopeDescFrame::UpdateAllUsedDescs(bool OnlyUsed)
 {
     int x;
     for (x = 0; x < MAX_SIGNALS_INSIDE_FRAME; x++) {
         if (!OnlyUsed ||
             ((m_Side == Left) && (m_Data->vids_left[x] > 0)) ||
             ((m_Side == Right) && (m_Data->vids_right[x] > 0))) {
             m_Desc[x]->update ();
         }
     }
 }

void OscilloscopeDescFrame::UpdateYAxis ()
{
    if (m_Side == Left) {
        m_OscilloscopeWidget->UpdateLeftYAxis();
    } else if (m_Side == Right) {
        m_OscilloscopeWidget->UpdateRightYAxis();
    }
}

int OscilloscopeDescFrame::DropSignal(Side par_Side, int par_Pos, DragAndDropInfos *par_DropInfos)
{
    return m_OscilloscopeWidget->DropSignal(par_Side, par_Pos, par_DropInfos);
}

void OscilloscopeDescFrame::SetSelectedSignal(enum Side par_Side, int par_Number)
{
    if ((m_Data->sel_left_right != par_Side) ||
        ((m_Data->sel_left_right == 0) && (m_Data->sel_pos_left != par_Number)) ||
        ((m_Data->sel_left_right == 1) && (m_Data->sel_pos_right != par_Number))) {
        if (par_Side == Left) {
            m_Data->sel_left_right = 0; /* 1 -> right 0 -> left */
            m_Data->sel_pos_left = par_Number;
        } else {
            m_Data->sel_left_right = 1; /* 1 -> right 0 -> left */
            m_Data->sel_pos_right = par_Number;
        }
        if (m_Data->state == 0) {  // If offline than because new cursor position
            m_OscilloscopeWidget->UpdateDrawArea();
        }
        // Repaint all descriptions
        QObjectList ChildWindows = children ();
        for (int i = 0; i < ChildWindows.size(); ++i) {
            static_cast<QWidget*>(ChildWindows.at (i))->update();
        }
        UpdateYAxis ();
        if (m_Data->xy_view_flag) {
            m_OscilloscopeWidget->UpdateTimeAxis();
        }
    }
}

void OscilloscopeDescFrame::DeleteSignal(int par_Numer)
{
    if (m_OscilloscopeWidget != nullptr) {
        m_OscilloscopeWidget->DeleteSignal(m_Side, par_Numer);
    }
}
