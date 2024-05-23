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


#include <math.h>

extern "C" {
#include "Config.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "MainValues.h"
#include "Scheduler.h"
#include "GetNotANumber.h"
#include "my_udiv128.h"
}
#include "OscilloscopeINI.h"
#include "OscilloscopeWidget.h"
#include "IsWindowNameValid.h"
#include "WindowNameAlreadyInUse.h"
#include "ColorHelpers.h"
#include "OscilloscopeConfigDialog.h"
#include "MdiWindowType.h"
#include "Oscilloscope.h"
#include "OszilloscopeDialogFrame.h"
#include "OscilloscopeSelectVariableDialog.h"
#include "StringHelpers.h"

#define UNIFORM_DIALOGE

OscilloscopeWidget::OscilloscopeWidget(QString par_WindowTitle, MdiSubWindow* par_SubWindow, MdiWindowType *par_Type, QWidget *parent) : MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, parent)
{
    m_UpdateDescsCounter = 0;
    m_UpdateDescsRate = 10;

    m_Data = &m_DataStore;

    memset (m_Data, 0, sizeof (m_DataStore));

    m_Data->NotANumber = GetNotANumber();

    m_Data->CriticalSectionNumber = AllocOsciWidgetCriticalSection ();
    if (m_Data->CriticalSectionNumber < 0) {
        ThrowError (1, "too many oscilloscopes open (max. %i allowed)", MAX_OSCILLOSCOPE_WINDOWS);
        return;
    }

    m_Layout = new QGridLayout(this);
    m_Layout->setContentsMargins(0, 0, 0, 0);;
    m_Layout->setSpacing(0);

    this->setLayout(m_Layout);

    m_RightDescFrame = nullptr;
    m_LeftDescFrame = nullptr;
    m_RightYAxis = nullptr;
    m_LeftYAxis = nullptr;
    m_RightStatus = nullptr;
    m_LeftStatus = nullptr;

    m_TimeAxis = new OscilloscopeTimeAxis (this, m_Data, this);
    m_DrawArea = new OscilloscopeDrawArea (this, m_Data, this);

    m_Layout->addWidget (m_DrawArea,       0, 2, 1, 1);
    m_Layout->addWidget (m_TimeAxis,       1, 2, 1, 1);

    m_Data->state = 1;     // on

    m_Data->left_axis_on = 1;
    m_Data->right_axis_on = 1;

    m_Data->buffer_depth = s_main_ini_val.OscilloscopeDefaultBufferDepth;

    m_Data->t_step = get_sched_periode_timer_clocks64();

    if (m_Data->t_step <= 0) {
        m_Data->t_step = static_cast<uint64_t>(TIMERCLKFRQ / 100);
        ThrowError (1, "abt_per variable is smaller or equal zero! set the sampel period to 0.01s");
    }
    m_Data->t_current_buffer_end = 0.0;
    m_Data->pixelpersec = 1.0 / (static_cast<double>(m_Data->t_step) * TIMERCLKFRQ);
    m_Data->y_zoom[0] = 1.0;

    m_Data->t_window_start = GetSimulatedTimeInNanoSecond();
    m_Data->t_window_end =  m_Data->t_window_start + static_cast<uint64_t>(TIMERCLKFRQ * 10);
    m_Data->window_step_width = 20;
    m_Data->t_step_win = my_umuldiv64(m_Data->t_window_end - m_Data->t_window_start, static_cast<uint64_t>(m_Data->window_step_width), 100);

    int x;
    for (x = 0; x < 20; x++) {
        m_Data->max_left[x] = 1.0;
        m_Data->min_left[x] = -1.0;
        m_Data->max_right[x] = 1.0;
        m_Data->min_right[x] = -1.0;
    }
    readFromIni();

    // Font will not saved int the INI data base

    m_Data->FontAverageCharWidth = this->fontMetrics().averageCharWidth();

    m_Data->t_window_width[0] = m_Data->t_window_end - m_Data->t_window_start;

    m_Data->xy_view_flag = 0;   // default x axis == time
    m_Data->BackgroundImage[0] = 0;
    m_Data->BackgroundImageLoaded = NO_BACKGROUND_IMAGE;

    RegisterOscilloscopeToCyclic (m_Data);
    if (update_vid_owc (m_Data)) {
        close ();
    }

    OnOffDescrFrame();

    setFocusPolicy (Qt::StrongFocus);

    m_MaxUpdateTimeWidth = 0.0;

}

OscilloscopeWidget::~OscilloscopeWidget()
{
    int i;

    writeToIni();
    EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);

    if (m_Data->trigger_vid > 0) remove_bbvari_unknown_wait (m_Data->trigger_vid);
    if (m_Data->name_trigger != nullptr) my_free (m_Data->name_trigger);
    m_Data->name_trigger = nullptr;
    for (i = 0; i < 21; i++) {
        if (m_Data->vids_left[i]) remove_bbvari_unknown_wait (m_Data->vids_left[i]);
        m_Data->vids_left[i] = 0;
        if (m_Data->name_left[i] != nullptr) my_free (m_Data->name_left[i]);
        m_Data->name_left[i] = nullptr;
        if (m_Data->vids_right[i]) remove_bbvari_unknown_wait (m_Data->vids_right[i]);
        m_Data->vids_right[i] = 0;
        if (m_Data->name_right[i] != nullptr) my_free (m_Data->name_right[i]);
        m_Data->name_right[i] = nullptr;
    }
    LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
    update_vid_owc (m_Data);
    EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);

    for (i = 0; i < 21; i++) {
        if (m_Data->buffer_left[i] != nullptr) my_free (m_Data->buffer_left[i]);
        m_Data->buffer_left[i] = nullptr;
        if (m_Data->buffer_right[i] != nullptr) my_free (m_Data->buffer_right[i]);
        m_Data->buffer_right[i] = nullptr;
    }

    if (m_Data->IncExcFilter != nullptr) {
        FreeIncludeExcludeFilter (m_Data->IncExcFilter);
        m_Data->IncExcFilter = nullptr;
    }

    LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);

    UnregisterOscilloscopeFromCyclic (m_Data);

    DeleteOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
    m_Data = nullptr;
}

void OscilloscopeWidget::ClearAllData()
{
    EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
    for (int x = 0; x < 21; x++) {
        m_Data->wr_poi_left[x] = 0;
        m_Data->depth_left[x] = 0;
        m_Data->wr_poi_right[x] = 0;
        m_Data->depth_right[x] = 0;
        m_Data->new_since_last_draw_left[x] = 0;
        m_Data->new_since_last_draw_right[x] = 0;
    }
    LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
}

void OscilloscopeWidget::UpdateDrawArea (void)
{
    m_DrawArea->update ();
}

void OscilloscopeWidget::SetCursorAll(uint64_t par_NewTCursor, bool par_MoveClippingFlag)
{
    QList<OscilloscopeWidget*>AllQscilloscopes = static_cast<OscilloscopeType*>(this->GetMdiWindowType())->GetAllOpenOscilloscopeWidgets();
    foreach (OscilloscopeWidget* Item, AllQscilloscopes) {
        if (!Item->IsOnline()) {
            Item->set_t_cursor(par_NewTCursor, par_MoveClippingFlag);
        }
    }
}

void OscilloscopeWidget::SetReferencePoint()
{
    QList<OscilloscopeWidget*>AllQscilloscopes = static_cast<OscilloscopeType*>(this->GetMdiWindowType())->GetAllOpenOscilloscopeWidgets();
    foreach (OscilloscopeWidget* Item, AllQscilloscopes) {
        if (!Item->IsOnline()) {
            Item->set_reference_point();
        }
    }
}

void OscilloscopeWidget::set_reference_point()
{
    if (!m_Data->ref_point_flag) {
        m_Data->time_step_ref_point = m_Data->time_step_cursor;
        m_Data->t_ref_point = m_Data->t_cursor;
        m_Data->ref_point_flag = 1;
    } else {
        m_Data->ref_point_flag = 0;
        m_Data->diff_to_ref_point_flag = 0;
    }
    this->UpdateDrawArea();
}

void OscilloscopeWidget::UpdateTimeAxis (void)
{
    m_TimeAxis->update ();
}

 void OscilloscopeWidget::UpdateAllUsedDescs (bool OnlyUsed)
{
     if (m_RightDescFrame != nullptr) m_RightDescFrame->UpdateAllUsedDescs(OnlyUsed);
     if (m_LeftDescFrame != nullptr) m_LeftDescFrame->UpdateAllUsedDescs(OnlyUsed);
}

bool OscilloscopeWidget::writeToIni()
{
    QPoint Pos = pos ();
    QString SectionName = GetIniSectionPath();
    write_osziwin_ini (QStringToConstChar(SectionName), m_Data, Pos.x(), Pos.y(), width (), height (), isMinimized ());
    return true;
}

bool OscilloscopeWidget::readFromIni()
{
    QString SectionName = GetIniSectionPath();
    read_osziwin_ini (QStringToConstChar(SectionName), m_Data);
    if (m_Data->BackgroundImageLoaded == SHOULD_LOAD_BACKGROUND_IMAGE) {
        QString Name = QString(m_Data->BackgroundImage);
        m_DrawArea->SetBackgroundImage(Name);
        m_Data->BackgroundImageLoaded = BACKGROUND_IMAGE_LOADED;
    }
    return true;
}


void OscilloscopeWidget::CheckTrigger()
{
    bool trigger = false;

    m_Data->t_current_to_update_end = m_Data->t_current_buffer_end;

    for (uint64_t t = m_Data->t_current_updated_end; t < m_Data->t_current_to_update_end; t += m_Data->t_step) {
        double Value = FiFoPosLeft (m_Data, 20, t);
        if (!_isnan(Value)) {
            if (m_Data->trigger_flank == '>') {
                if (m_Data->trigger_state) {
                    if (Value > m_Data->trigger_value) {
                        m_Data->pre_trigger_counter = m_Data->buffer_depth - m_Data->pre_trigger;
                        trigger = true;
                        break; // for
                    }
                } else {
                    if (Value < m_Data->trigger_value) {
                        m_Data->trigger_state = 1;  // was smaller than the threshold
                    }
                }
            } else if (m_Data->trigger_flank == '<') {
                if (m_Data->trigger_state) {
                    if (Value < m_Data->trigger_value) {
                        m_Data->pre_trigger_counter = m_Data->buffer_depth - m_Data->pre_trigger;
                        trigger = true;
                        break;  // for
                    }
                } else {
                    if (Value > m_Data->trigger_value) {
                        m_Data->trigger_state = 1;  // was larger than the threshold
                    }
                }
            }
        }
    }

    m_Data->t_current_updated_end = m_Data->t_current_to_update_end;

    if (trigger) {   // If triggered count the signls
        int x = 0;
        for (int i = 0; i < 21; i++) {
            if (m_Data->vids_left[i] > 0) x++;
            if (m_Data->vids_right[i] > 0) x++;
        }
        m_Data->pre_trigger_counter *= x; // And multiplay pre-trigger counter
                                          // with the nummer of the signals.
                                          // insiede stamp2oszibuffers() this
                                          // will be decrement for each signal
        m_Data->state = 1;
        this->UpdateStatus();
    }
}

static int CheckIfSignalIsSelectedRight(OSCILLOSCOPE_DATA *m_Data)
{
    // first seach upwards on the right side
    for(int x = m_Data->sel_pos_right; x >= 0; x--) {
        if ((m_Data->vids_right[x] > 0) &&
            !m_Data->vars_disable_right[x]) {
             return x;
        }
    }
    // second seach downwards on the right side
    for(int x = m_Data->sel_pos_right; x< 20; x++) {
        if ((m_Data->vids_right[x] > 0) &&
            !m_Data->vars_disable_right[x]) {
             return x;
        }
    }
    return -1;
}

static int CheckIfSignalIsSelectedLeft(OSCILLOSCOPE_DATA *m_Data)
{
    // first seach upwards on the left side
    for(int x = m_Data->sel_pos_left; x >= 0; x--) {
        if ((m_Data->vids_left[x] > 0) &&
            !m_Data->vars_disable_left[x]) {
             return x;
        }
    }
    // second seach downwards on the left side
    for(int x = m_Data->sel_pos_left; x< 20; x++) {
        if ((m_Data->vids_left[x] > 0) &&
            !m_Data->vars_disable_left[x]) {
             return x;
        }
    }
    return -1;
}
void OscilloscopeWidget::CheckIfSignalIsSelected()
{
    if (m_Data->sel_left_right) {
        if ((m_Data->vids_right[m_Data->sel_pos_right] <= 0) ||    // there is no valid variable selected
            m_Data->vars_disable_right[m_Data->sel_pos_right]) {   // or not displaied
            int NewPos = CheckIfSignalIsSelectedRight(m_Data);
            if (NewPos >= 0) {
                m_RightDescFrame->SetSelectedSignal(Right, NewPos);
            } else {
                NewPos = CheckIfSignalIsSelectedLeft(m_Data);
                if (NewPos >= 0) {
                    m_LeftDescFrame->SetSelectedSignal(Left, NewPos);
                }
            }
        }
    } else {
        if ((m_Data->vids_left[m_Data->sel_pos_left] <= 0) ||    // there is no valid variable selected
            m_Data->vars_disable_left[m_Data->sel_pos_left]) {   // or not displaied
            int NewPos = CheckIfSignalIsSelectedLeft(m_Data);
            if (NewPos >= 0) {
                m_LeftDescFrame->SetSelectedSignal(Left, NewPos);
            } else {
                NewPos = CheckIfSignalIsSelectedRight(m_Data);
                if (NewPos >= 0) {
                    m_RightDescFrame->SetSelectedSignal(Right, NewPos);
                }
            }
        }
    }
}


void OscilloscopeWidget::CyclicUpdate ()
{
    if (m_Data->state) {  // Only if oscilloscope is active cyclic update
        EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
        if (m_Data->state == 3) {
            CheckTrigger();
            LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
        } else {
            if (m_Data->xy_view_flag == 0) {
                // Time view
                // If online and we are at the window frame scroll with time step with forward
                if ((m_Data->state) && (m_Data->t_current_buffer_end > m_Data->t_window_end)) {

                    m_Data->t_window_end = m_Data->t_current_buffer_end + m_Data->t_step_win;

                    m_Data->t_window_start = m_Data->t_window_end - m_Data->t_window_size;
                    #ifdef DEBUG_PRINT_TO_FILE
                    m_DrawArea->DebugPrint ("  update: t_window_start = %" PRIu64 ", t_window_end = %" PRIu64 ", t_current_to_update_end = %" PRIu64 ", t_current_buffer_end = %" PRIu64 "\n",
                                            m_Data->t_window_start, m_Data->t_window_end, m_Data->t_current_to_update_end, m_Data->t_current_buffer_end);
                    #endif

                    LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);

                    // Update time axis
                    m_TimeAxis->update();
                    // Update signal curves
                    m_DrawArea->update();

                } else {
                    // XY view
                    int UpdateStartX;
                    int UpdateEndX;
                    int Width = m_DrawArea->width();

                    m_Data->t_current_buffer_end_save = m_Data->t_current_buffer_end;

                    // Only paint the clipping
                    // Calculate the clipping to be updated
                    UpdateStartX = static_cast<int>(my_umuldiv64(m_Data->t_current_updated_end - m_Data->t_window_start - m_Data->t_current_buffer_end_diff, static_cast<uint64_t>(Width), m_Data->t_window_size));
                    UpdateEndX = static_cast<int>(my_umuldiv64(m_Data->t_current_buffer_end - m_Data->t_window_start, static_cast<uint64_t>(Width), m_Data->t_window_size));
                    m_Data->t_current_buffer_end_diff = 0;

                    // Have we to update anything?
                    if (((UpdateStartX <= 0) && (UpdateEndX <= 0)) ||
                        ((UpdateStartX >= Width) && (UpdateEndX >= Width)) ||
                        (UpdateStartX == UpdateEndX)) {
                        // No update because we are are outside the window frame
                        LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
                    } else {
                        #ifdef DEBUG_PRINT_TO_FILE
                        m_DrawArea->DebugPrint ("repaint: t_window_start = %" PRIu64 ", t_window_end = %" PRIu64 ", t_current_to_update_end = %" PRIu64 ", t_current_buffer_end = %" PRIu64 "\n",
                                                m_Data->t_window_start, m_Data->t_window_end, m_Data->t_current_to_update_end, m_Data->t_current_buffer_end);
                        #endif
                        LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);

                        m_DrawArea->update (UpdateStartX-1, 0, UpdateEndX - UpdateStartX + 1, m_DrawArea->height());
                    }
                }
            } else {
                // XY view
                m_Data->t_current_to_update_end = m_Data->t_current_buffer_end;
                LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
                m_DrawArea->update();
                m_Data->t_current_updated_end = m_Data->t_current_to_update_end;
            }

            if (m_UpdateDescsCounter++ > m_UpdateDescsRate) {
                m_UpdateDescsCounter = 0;
                UpdateAllUsedDescs (true);
            }
        }
    }
}

void OscilloscopeWidget::SelectNextSignal()
{
    if (m_Data->sel_left_right) {
        if (m_Data->sel_pos_right < 18) {
            m_RightDescFrame->SetSelectedSignal(Right, m_Data->sel_pos_right + 1);
        }
    } else {
        if (m_Data->sel_pos_left < 18) {
            m_LeftDescFrame->SetSelectedSignal(Left, m_Data->sel_pos_left + 1);
        }
    }
}

void OscilloscopeWidget::SelectPreviosSignal()
{
    if (m_Data->sel_left_right) {
        if (m_Data->sel_pos_right > 0) {
            m_RightDescFrame->SetSelectedSignal(Right, m_Data->sel_pos_right - 1);
        }
    } else {
        if (m_Data->sel_pos_left > 0) {
            m_LeftDescFrame->SetSelectedSignal(Left, m_Data->sel_pos_left - 1);
        }
    }
}

void OscilloscopeWidget::DeleteSignal()
{
    if (m_Data->sel_left_right) {
        EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
        if (m_Data->vids_right[m_Data->sel_pos_right] > 0) {
            remove_bbvari_unknown_wait (m_Data->vids_right[m_Data->sel_pos_right]);
        }
        m_Data->vids_right[m_Data->sel_pos_right] = 0;
        if (m_Data->buffer_right[m_Data->sel_pos_right] != nullptr) my_free (m_Data->buffer_right[m_Data->sel_pos_right]);
        m_Data->buffer_right[m_Data->sel_pos_right] = nullptr;
        m_Data->LineSize_right[m_Data->sel_pos_right] = 1;
        m_Data->color_right[m_Data->sel_pos_right] = 0;
        LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);

        // Only for updating the display
         m_RightDescFrame->SetSelectedSignal(Right, m_Data->sel_pos_right);
    } else {
        EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
        if (m_Data->vids_left[m_Data->sel_pos_left] > 0) {
            remove_bbvari_unknown_wait (m_Data->vids_left[m_Data->sel_pos_left]);
        }
        m_Data->vids_left[m_Data->sel_pos_left] = 0;
        if (m_Data->buffer_left[m_Data->sel_pos_left] != nullptr) my_free (m_Data->buffer_left[m_Data->sel_pos_left]);
        m_Data->buffer_left[m_Data->sel_pos_left] = nullptr;
        m_Data->LineSize_left[m_Data->sel_pos_left] = 1;
        m_Data->color_left[m_Data->sel_pos_left] = 0;
        LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);

        // Only for updating the display
        m_LeftDescFrame->SetSelectedSignal(Left, m_Data->sel_pos_left - 1);
    }
    if (update_vid_owc (m_Data)) {
        ThrowError (1, "cannot update variable list");
    }
}

void OscilloscopeWidget::EditSelectedSignal()
{
}

void OscilloscopeWidget::SelectLeftSide()
{
    if (m_Data->left_axis_on) {
        m_LeftDescFrame->SetSelectedSignal(Left, m_Data->sel_pos_left);
    }
}

void OscilloscopeWidget::SelectRightSide()
{
    if (m_Data->right_axis_on) {
        m_RightDescFrame->SetSelectedSignal(Right, m_Data->sel_pos_right);
    }
}

void OscilloscopeWidget::changeColor(QColor arg_color)
{
    Q_UNUSED(arg_color)
    // NOTE: required by inheritance but not used
}

void OscilloscopeWidget::changeFont(QFont arg_font)
{
    Q_UNUSED(arg_font)
    // NOTE: required by inheritance but not used
}

void OscilloscopeWidget::changeWindowName(QString arg_name)
{
    Q_UNUSED(arg_name)
    // NOTE: required by inheritance but not used
}

void OscilloscopeWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    Q_UNUSED(arg_variable)
    Q_UNUSED(arg_visible)
    // NOTE: required by inheritance but not used
}

void OscilloscopeWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    Q_UNUSED(arg_variables)
    Q_UNUSED(arg_visible)
    // NOTE: required by inheritance but not used
}

void OscilloscopeWidget::resetDefaultVariables(QStringList arg_variables)
{
    Q_UNUSED(arg_variables)
    // NOTE: required by inheritance but not used
}


void OscilloscopeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    Q_UNUSED(event);
}

void OscilloscopeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    Q_UNUSED(event);
}

void OscilloscopeWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
}

void OscilloscopeWidget::dropEvent(QDropEvent *event)
{
    Q_UNUSED(event);
}

void OscilloscopeWidget::keyPressEvent(QKeyEvent * event)
{
    if (event->matches(QKeySequence::MoveToNextChar)) {
        SetCursorAll (m_Data->t_cursor + m_Data->t_step, true);
    } else if (event->matches(QKeySequence::MoveToPreviousChar)) {
        SetCursorAll (m_Data->t_cursor - m_Data->t_step, true);
    } else if (event->matches(QKeySequence::MoveToNextPage)) {
        SetCursorAll (m_Data->t_cursor - m_Data->t_window_size, false);
    } else if (event->matches(QKeySequence::MoveToPreviousPage)) {
        SetCursorAll (m_Data->t_cursor + m_Data->t_window_size, false);
    } else if (event->matches(QKeySequence::MoveToNextLine)) {
        SelectNextSignal();
    } else if (event->matches(QKeySequence::MoveToPreviousLine)) {
        SelectPreviosSignal();
    } else if (event->matches(QKeySequence::Delete)) {
        DeleteSignal();
    } else if (event->matches(QKeySequence::InsertParagraphSeparator)) {  // Enter
        EditSelectedSignal();
    } else if (event->matches(QKeySequence::SelectStartOfLine)) {
        SelectLeftSide();
    } else if (event->matches(QKeySequence::SelectEndOfLine)) {
        SelectRightSide();
    }
}


void OscilloscopeWidget::UpdateLeftYAxis ()
{
    if (m_Data->left_axis_on) {
        m_LeftYAxis->update();
    }
}

void OscilloscopeWidget::UpdateRightYAxis ()
{
    if (m_Data->right_axis_on) {
        m_RightYAxis->update();
    }
}

void OscilloscopeWidget::UpdateYAxises ()
{
    if (m_Data->right_axis_on) {
        m_RightYAxis->update();
    }
    if (m_Data->left_axis_on) {
        m_LeftYAxis->update();
    }
}

void OscilloscopeWidget::UpdateStatus ()
{
    if (m_Data->left_axis_on) {
        m_LeftStatus->update();
    }
    if (m_Data->right_axis_on) {
        m_RightStatus->update();
    }
}


void OscilloscopeWidget::GoOnline (uint64_t par_StartTime)
{
    EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
    int StateSave = m_Data->state;
    if (!StateSave) {
        if (m_Data->trigger_flag) {
            m_Data->state = 3;
            m_Data->trigger_state = 0;
        } else m_Data->state = 1;
    }
    SwitchOscilloscopeOnline (m_Data, par_StartTime, 0);
    LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
    if (!StateSave) {
        // repaint following child windows
        m_DrawArea->update ();
        UpdateTimeAxis ();
        UpdateAllUsedDescs (true);
        UpdateStatus ();
    }
}

void OscilloscopeWidget::GoOffline (bool par_WithTime, uint64_t par_EndTime)
{
    EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
    int StateSave = m_Data->state;
    if (StateSave || par_WithTime) {
        m_Data->state = 0;
        if (par_WithTime) {
            m_Data->t_current_to_update_end = par_EndTime;
        } else {
            m_Data->t_current_to_update_end = m_Data->t_current_buffer_end;
        }
        m_Data->t_current_updated_end = m_Data->t_current_to_update_end;
        m_Data->t_cursor = m_Data->t_current_to_update_end;
        if (m_Data->t_current_to_update_end < m_Data->t_window_size) {
            set_t_window_start_end (0,
                                    m_Data->t_window_size);
        } else {
            set_t_window_start_end (m_Data->t_current_to_update_end - m_Data->t_window_size,
                                    m_Data->t_current_to_update_end);
        }
    }
    LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
    if (StateSave) {
        // repaint following child windows
        m_DrawArea->update ();
        UpdateTimeAxis ();
        UpdateAllUsedDescs (true);
        UpdateStatus ();
    }
}

void OscilloscopeWidget::GoAllOnline()
{
    QList<OscilloscopeWidget*>AllQscilloscopes = static_cast<OscilloscopeType*>(this->GetMdiWindowType())->GetAllOpenOscilloscopeWidgets();
    uint64_t Time = GetSimulatedTimeSinceStartedInNanoSecond();
    EnterOsziCycleCS();
    foreach (OscilloscopeWidget* Item, AllQscilloscopes) {
        Item->GoOnline(Time);
    }
    LeaveOsziCycleCS();
}


void OscilloscopeWidget::GoAllOffline()
{
    SwitchAllSyncronOffline();
    QList<OscilloscopeWidget*>AllQscilloscopes = static_cast<OscilloscopeType*>(this->GetMdiWindowType())->GetAllOpenOscilloscopeWidgets();
    foreach (OscilloscopeWidget* Item, AllQscilloscopes) {
        Item->UpdateDrawArea();
        Item->UpdateTimeAxis();
    }
}

int OscilloscopeWidget::IsOnline ()
{
    return m_Data->state;
}

//  Set the cursor always to a defined sample time
void OscilloscopeWidget::set_t_cursor (uint64_t par_NewTCursor, bool par_MoveClippingFlag)
{
    uint64_t dt;
    uint64_t t_cursor_save = m_Data->t_cursor;

    m_DrawArea->UpdateCursor ();  // delete old one  (repaint with XOR)

    CheckIfSignalIsSelected();    // be sure one signal is selected

    m_Data->t_cursor = par_NewTCursor;
    dt = m_Data->t_cursor % m_Data->t_step;
    if (dt > (m_Data->t_step / 2))
        m_Data->t_cursor  += m_Data->t_step - dt;
    else m_Data->t_cursor -= dt;

    uint64_t latest_valid_time = static_cast<uint64_t>(m_Data->buffer_depth) * m_Data->t_step;
    if (latest_valid_time < m_Data->t_current_buffer_end) {
        latest_valid_time = m_Data->t_current_buffer_end - latest_valid_time;
    } else {
        latest_valid_time = 0;
    }
    if (m_Data->t_cursor < latest_valid_time) {
        m_Data->t_cursor = latest_valid_time;
    }
    if (m_Data->t_cursor > m_Data->t_current_buffer_end) m_Data->t_cursor = m_Data->t_current_buffer_end;

    if (par_MoveClippingFlag) {
        if (m_Data->t_cursor >= m_Data->t_window_end) {
            SetWindowEndTime (m_Data->t_window_end + m_Data->t_step_win);
        } else if (m_Data->t_cursor <= m_Data->t_window_start) {
            SetWindowStartTime (m_Data->t_window_start - m_Data->t_step_win);
        } else {
            m_DrawArea->UpdateCursor ();  // Paint the cursor (with XOR)
            UpdateStatus ();              // Paint the time
            UpdateAllUsedDescs (true);    // Paint all descriptions
        }
    } else {
        if (m_Data->t_cursor > t_cursor_save) {
            SetWindowEndTime (m_Data->t_window_end + (m_Data->t_cursor - t_cursor_save));
        } else if (m_Data->t_cursor < t_cursor_save) {
            SetWindowStartTime (m_Data->t_window_start - (t_cursor_save - m_Data->t_cursor));
        }
    }
}

//  Set window start time
void OscilloscopeWidget::SetWindowStartTime (uint64_t par_WindowStartTime)
{
    uint64_t dt;
    dt = par_WindowStartTime % m_Data->t_step;
    if (dt > (m_Data->t_step / 2))
        par_WindowStartTime += m_Data->t_step - dt;
    else par_WindowStartTime -= dt;

    if (par_WindowStartTime < 0.0) {
        par_WindowStartTime = 0.0;
    }
    // Move window start time not behind the buffer depth
    if (par_WindowStartTime < (m_Data->t_current_buffer_end - static_cast<uint64_t>(m_Data->buffer_depth) * m_Data->t_step)) {
        par_WindowStartTime = m_Data->t_current_buffer_end - static_cast<uint64_t>(m_Data->buffer_depth) * m_Data->t_step;
    }
    // Move window end time not to the future
    if ((par_WindowStartTime + m_Data->t_window_size) > m_Data->t_current_buffer_end) {
        par_WindowStartTime = m_Data->t_current_buffer_end -  m_Data->t_window_size;
    }
    m_Data->t_window_start = par_WindowStartTime;
    m_Data->t_window_end = par_WindowStartTime + m_Data->t_window_size;
    UpdateStatus ();
    UpdateAllUsedDescs (true);
    m_TimeAxis->update();
    m_DrawArea->update();
}

//  Set window end time
void OscilloscopeWidget::SetWindowEndTime (uint64_t par_WindowEndTime)
{
    uint64_t dt;
    dt = par_WindowEndTime % m_Data->t_step;
    if (dt > (m_Data->t_step / 2))
        par_WindowEndTime += m_Data->t_step - dt;
    else par_WindowEndTime -= dt;

    // Move window end time not to the future
    if (par_WindowEndTime > m_Data->t_current_buffer_end) {
        par_WindowEndTime = m_Data->t_current_buffer_end;
    }
    // Move window start time not behind the buffer depth
    if ((par_WindowEndTime - m_Data->t_window_size) < m_Data->t_current_buffer_end - static_cast<uint64_t>(m_Data->buffer_depth) * m_Data->t_step) {
        par_WindowEndTime = m_Data->t_current_buffer_end - static_cast<uint64_t>(m_Data->buffer_depth) * m_Data->t_step + m_Data->t_window_size;
    }
    m_Data->t_window_end = par_WindowEndTime;
    m_Data->t_window_start = par_WindowEndTime - m_Data->t_window_size;

    UpdateStatus ();
    UpdateAllUsedDescs (true);
    m_TimeAxis->update();
    m_DrawArea->update();
}

void OscilloscopeWidget::set_t_window_start_end (uint64_t ts, uint64_t te)
{
    uint64_t dt;

    // end time of the window must be larger than start time (by one time step)
    if (m_Data->t_window_end <= (m_Data->t_window_start + m_Data->t_step)) {
        m_Data->t_window_end = m_Data->t_window_start + m_Data->t_step;
    }

    m_Data->t_window_start = ts;
    dt = m_Data->t_window_start % m_Data->t_step;
    if (dt > (m_Data->t_step / 2))
        m_Data->t_window_start += m_Data->t_step - dt;
    else m_Data->t_window_start -= dt;

    m_Data->t_window_end = te;
    dt = m_Data->t_window_end % m_Data->t_step;
    if (dt > (m_Data->t_step / 2))
        m_Data->t_window_end  += m_Data->t_step - dt;
    else m_Data->t_window_end -= dt;

    m_Data->t_window_size = m_Data->t_window_end - m_Data->t_window_start;
    if (m_Data->t_window_size <= m_Data->t_step) {
        m_Data->t_window_size = m_Data->t_step;
        m_Data->t_window_end = m_Data->t_window_start + m_Data->t_window_size;
    }
    m_Data->draw_widget_width = m_DrawArea->width();
    m_Data->pixelpersec = static_cast<double>(m_Data->draw_widget_width) / static_cast<double>(m_Data->t_window_end - m_Data->t_window_start);
    m_Data->t_step_win = my_umuldiv64(m_Data->t_window_end - m_Data->t_window_start, static_cast<uint64_t>(m_Data->window_step_width), 100);
}

void OscilloscopeWidget::ZoomIn (void)
{
    if (--(m_Data->zoom_pos) < 0) m_Data->zoom_pos = 0;
    else {
        if (m_Data->state)
             set_t_window_start_end (m_Data->t_window_end - m_Data->t_window_width[m_Data->zoom_pos],
                                     m_Data->t_window_end);
        else set_t_window_start_end (m_Data->t_window_base[m_Data->zoom_pos],
                                     m_Data->t_window_base[m_Data->zoom_pos] + m_Data->t_window_width[m_Data->zoom_pos]);
        m_DrawArea->update ();
        UpdateYAxises();
        UpdateTimeAxis();
    }
}

void OscilloscopeWidget::ZoomOut (void)
{
    if (++(m_Data->zoom_pos) > 9) m_Data->zoom_pos = 9;
    else {
        if (m_Data->y_zoom[m_Data->zoom_pos] <= 0.0)
            m_Data->zoom_pos--;
        if (m_Data->state)
            set_t_window_start_end (m_Data->t_window_end - m_Data->t_window_width[m_Data->zoom_pos],
                                    m_Data->t_window_end);
        else set_t_window_start_end (m_Data->t_window_base[m_Data->zoom_pos],
                                     m_Data->t_window_base[m_Data->zoom_pos] + m_Data->t_window_width[m_Data->zoom_pos]);
        m_DrawArea->update ();
        UpdateYAxises();
        UpdateTimeAxis();
    }
}

void OscilloscopeWidget::ZoomReset (void)
{
    m_Data->zoom_pos = 0;
    m_Data->y_off[0] = 0.0;
    m_Data->y_zoom[0] = 1.0;
    for (int x = 1; x < 10; x++) {
         m_Data->y_zoom[x] = 0.0;
    }
    m_Data->t_window_size = static_cast<uint64_t>(m_Data->t_window_size_configered * TIMERCLKFRQ);
    if (m_Data->t_current_buffer_end >= m_Data->t_window_size) {
        set_t_window_start_end (m_Data->t_current_buffer_end - m_Data->t_window_size,
                                m_Data->t_current_buffer_end);
    } else {
        set_t_window_start_end (0,
                                m_Data->t_window_size);
    }
    m_DrawArea->update ();
    UpdateYAxises();
    UpdateTimeAxis();
}

void OscilloscopeWidget::ZoomIndex(int par_Index)
{
    if ((par_Index >= 0) && (par_Index < 10)) {
        m_Data->zoom_pos = par_Index;
        if (m_Data->state)
            set_t_window_start_end (m_Data->t_window_end - m_Data->t_window_width[m_Data->zoom_pos],
                                    m_Data->t_window_end);
        else set_t_window_start_end (m_Data->t_window_base[m_Data->zoom_pos],
                                     m_Data->t_window_base[m_Data->zoom_pos] + m_Data->t_window_width[m_Data->zoom_pos]);
        m_DrawArea->update ();
        UpdateYAxises();
        UpdateTimeAxis();
    }
}

void OscilloscopeWidget::new_zoom (void)
{
    int x, i;

    i = m_Data->zoom_pos;
    for (x = 0; x < 10; x++) {
        if (i < 10) {
            m_Data->y_off[x] = m_Data->y_off[i];
            m_Data->y_zoom[x] = m_Data->y_zoom[i];
            m_Data->t_window_base[x] = m_Data->t_window_base[i];
            m_Data->t_window_width[x] = m_Data->t_window_width[i];
        } else {
            m_Data->y_off[x] = 0.0;
            m_Data->y_zoom[x] = 0.0;
            m_Data->t_window_base[x] = 0.0;
            m_Data->t_window_width[x] = 0.0;
        }
        i++;
    }
    m_Data->zoom_pos = 0;
    for (i = 9; i > 0; i--) {
        m_Data->y_off[i] = m_Data->y_off[i-1];
        m_Data->y_zoom[i] = m_Data->y_zoom[i-1];
        m_Data->t_window_base[i] = m_Data->t_window_base[i-1];
        m_Data->t_window_width[i] = m_Data->t_window_width[i-1];
    }
    if (--(m_Data->zoom_pos) < 0) m_Data->zoom_pos = 0;
}

void OscilloscopeWidget::NewZoom (int par_YZoomFlag, int par_TimeZoomFlag,
                                  int par_WinWidth, int par_WinHeight,
                                  int par_x1, int par_y1,
                                  int par_x2, int par_y2)
{
    m_Data->pixelpersec = static_cast<double>(par_WinWidth) / (m_Data->t_window_end - m_Data->t_window_start);
    if (par_YZoomFlag && !par_TimeZoomFlag)  { /* Y-Zoom */
        new_zoom ();
        m_Data->y_off[m_Data->zoom_pos] += (static_cast<double>(par_WinHeight - ((par_y1 > par_y2) ? par_y1 : par_y2)) / static_cast<double>(par_WinHeight)) / m_Data->y_zoom[m_Data->zoom_pos];
        m_Data->y_zoom[m_Data->zoom_pos] *= static_cast<double>(par_WinHeight) / static_cast<double>(abs (par_y1 - par_y2));
    } else if (!par_YZoomFlag && par_TimeZoomFlag)  {  /* Zeit-Zoom */
        m_Data->t_window_base[m_Data->zoom_pos] = m_Data->t_window_start;
        m_Data->t_window_width[m_Data->zoom_pos] = m_Data->t_window_end - m_Data->t_window_start;
        new_zoom ();
        set_t_window_start_end (m_Data->t_window_start + static_cast<uint64_t>(static_cast<double>((par_x1 < par_x2) ? par_x1 : par_x2) / m_Data->pixelpersec),
                                m_Data->t_window_start + static_cast<uint64_t>(static_cast<double>((par_x1 > par_x2) ? par_x1 : par_x2) / m_Data->pixelpersec));
        m_Data->t_step_win = my_umuldiv64(m_Data->t_window_end - m_Data->t_window_start, static_cast<uint64_t>(m_Data->window_step_width), 100);
        m_Data->t_window_base[m_Data->zoom_pos] = m_Data->t_window_start;
        m_Data->t_window_width[m_Data->zoom_pos] = m_Data->t_window_end - m_Data->t_window_start;
    } else if (par_YZoomFlag && par_TimeZoomFlag)  {    /* beides */
        m_Data->t_window_base[m_Data->zoom_pos] = m_Data->t_window_start;
        m_Data->t_window_width[m_Data->zoom_pos] = m_Data->t_window_end - m_Data->t_window_start;
        new_zoom ();
        m_Data->y_off[m_Data->zoom_pos] += (static_cast<double>(par_WinHeight - ((par_y1 > par_y2) ? par_y1 : par_y2)) / static_cast<double>(par_WinHeight)) / m_Data->y_zoom[m_Data->zoom_pos];
        m_Data->y_zoom[m_Data->zoom_pos] *= static_cast<double>(par_WinHeight) / static_cast<double>(abs (par_y1 - par_y2));
        set_t_window_start_end (m_Data->t_window_start + static_cast<uint64_t>(static_cast<double>((par_x1 < par_x2) ? par_x1 : par_x2) / m_Data->pixelpersec),
                                m_Data->t_window_start + static_cast<uint64_t>(static_cast<double>((par_x1 > par_x2) ? par_x1 : par_x2) / m_Data->pixelpersec));
        m_Data->t_step_win = my_umuldiv64(m_Data->t_window_end - m_Data->t_window_start, static_cast<uint64_t>(m_Data->window_step_width), 100);
        m_Data->t_window_base[m_Data->zoom_pos] = m_Data->t_window_start;
        m_Data->t_window_width[m_Data->zoom_pos] = m_Data->t_window_end - m_Data->t_window_start;
    }
}

void OscilloscopeWidget::ConfigDialog (void)
{
    OscilloscopeConfigDialog Dlg (m_Data, GetWindowTitle());

    if (Dlg.exec() == QDialog::Accepted) {
        if (Dlg.WindowTitleChanged()) {
            RenameWindowTo (Dlg.GetWindowTitle());
        }
        QString Name;
        if (m_Data->BackgroundImageLoaded == SHOULD_LOAD_BACKGROUND_IMAGE) {
            Name = QString(m_Data->BackgroundImage);
            if (m_DrawArea->SetBackgroundImage(Name)) {
                m_Data->BackgroundImageLoaded = BACKGROUND_IMAGE_LOADED;
            } else {
            }
        } else if (m_Data->BackgroundImageLoaded == SHOULD_DELETE_BACKGROUND_IMAGE) {
            Name = QString("");
            m_DrawArea->SetBackgroundImage(Name);
            m_Data->BackgroundImageLoaded = NO_BACKGROUND_IMAGE;
        }
        if (Dlg.OnOffDescrFrameChanged()) {
            OnOffDescrFrame ();
        }
    }
}

void OscilloscopeWidget::ResizeDescFrames (void)
{
    if (m_Data->left_axis_on) {
        if (m_LeftDescFrame->width() != m_Data->width_all_left_desc * (m_Data->FontAverageCharWidth << 1)) {
            m_LeftDescFrame->setMaximumWidth (m_Data->width_all_left_desc * (m_Data->FontAverageCharWidth << 1));
        }
    }
    if (m_Data->right_axis_on) {
        if (m_RightDescFrame->width() != m_Data->width_all_right_desc * (m_Data->FontAverageCharWidth << 1)) {
            m_RightDescFrame->setMaximumWidth (m_Data->width_all_right_desc * (m_Data->FontAverageCharWidth << 1));
        }
    }
}

int OscilloscopeWidget::DropSignal(enum Side par_Side, int par_Pos, DragAndDropInfos *par_DropInfos)
{
    if (m_Data != nullptr) {
        if (par_Pos < 0) {
            // If position is negative than search a free one
            if (m_Data->left_axis_on) {
                for (int x = 0; x < 20; x++) {
                    if (m_Data->vids_left[x] <= 0) {
                        par_Pos = x;
                        par_Side = Left;
                        goto __FOUND;
                    }
                }
            }
            if (m_Data->right_axis_on) {
                for (int x = 0; x < 20; x++) {
                    if (m_Data->vids_right[x] <= 0) {
                        par_Pos = x;
                        par_Side = Right;
                        goto __FOUND;
                    }
                }
            }
        } else {
__FOUND:
            double Min, Max;
            bool MinMaxValid = par_DropInfos->GetMinMaxValue(&Min, &Max);
            QString Name = par_DropInfos->GetName();
            return AddSignal (par_Side, par_Pos,
                              Name,
                              MinMaxValid, Min, Max,
                              par_DropInfos->GetColor(),
                              par_DropInfos->GetLineWidth(),
                              par_DropInfos->GetDisplayMode(),
                              par_DropInfos->GetAlignment());
        }
    }
    return -1;
}

int OscilloscopeWidget::AddSignal(enum Side par_Side, int par_Pos, QString &par_Name,
                                  bool par_MinMaxVaild, double par_Min, double par_Max,
                                  QColor par_Color, int par_LineWidth,
                                  enum DragAndDropDisplayMode par_DisplayMode, enum DragAndDropAlignment par_Alignment)
{
    int vid;
    char *variname;

    if (par_LineWidth < 1) par_LineWidth = 1;  // Linie have to be at least 1 pixel width

    if (par_Side == Right) {
        vid = m_Data->vids_right[par_Pos];
        variname = m_Data->name_right[par_Pos];
    } else {
        vid = m_Data->vids_left[par_Pos];
        variname = m_Data->name_left[par_Pos];
    }
    if (vid > 0) {
        if (variname != nullptr) {
            if (par_Name.compare(QString(variname))) {
                remove_bbvari_unknown_wait (vid);   // First delete all variables
            }
        }
    }
    // Now ann new variables
    vid = add_bbvari (QStringToConstChar(par_Name), BB_UNKNOWN_WAIT, nullptr);
    if (vid <= 0) {
        ThrowError (1, "cannot attach variable \"%s\"", QStringToConstChar(par_Name));
        return -1;
    }
    if (!par_MinMaxVaild) {
        par_Min = get_bbvari_min(vid);
        par_Max = get_bbvari_max(vid);
    }
    if (!par_Color.isValid()) {
        int IntColor = get_bbvari_color(vid);
        if (IntColor < 0) {
            QList<QColor> UsedColors = GetAllUsedColors();
            par_Color = GetColorProposal(UsedColors);
        } else {
            par_Color = QColor ((IntColor >> 0) & 0xFF, (IntColor >> 8) & 0xFF, (IntColor >> 16) & 0xFF);
        }
    }
    if (par_Side == Right) {
        EnterOsziCycleCS ();                                            // 1.  The order is important
        EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber); // 2.
        m_Data->vids_right[par_Pos] = vid;
        m_Data->name_right[par_Pos] = ReallocCopyString(m_Data->name_right[par_Pos], par_Name);
        if (m_Data->buffer_right[par_Pos] != nullptr) my_free (m_Data->buffer_right[par_Pos]);
        if ((m_Data->buffer_right[par_Pos] = static_cast<double*>(my_calloc (static_cast<size_t>(m_Data->buffer_depth), sizeof(double)))) == nullptr) {
            ThrowError (1, "out of memory");
        }
        m_Data->wr_poi_right[par_Pos] = 0;
        m_Data->depth_right[par_Pos] = 0;
        LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber); // 2.
        LeaveOsziCycleCS ();                                            // 1.
    } else {
        EnterOsziCycleCS ();                                            // 1.   The order is important
        EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber); // 2.
        m_Data->vids_left[par_Pos] = vid;
        m_Data->name_left[par_Pos] = ReallocCopyString(m_Data->name_left[par_Pos], par_Name);
        if (m_Data->buffer_left[par_Pos] != nullptr) my_free (m_Data->buffer_left[par_Pos]);
        if ((m_Data->buffer_left[par_Pos] = static_cast<double*>(my_calloc (static_cast<size_t>(m_Data->buffer_depth), sizeof(double)))) == nullptr) {
            ThrowError (1, "out of memory");
        }
        m_Data->wr_poi_left[par_Pos] = 0;
        m_Data->depth_left[par_Pos] = 0;
        LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber); // 2.
        LeaveOsziCycleCS ();                                            // 1.
    }

    if (par_Min >= par_Max) par_Max = par_Min + 1.0;
    if (par_Side == Right) {
        m_Data->min_right[par_Pos] = par_Min;
        m_Data->max_right[par_Pos] = par_Max;
        m_Data->color_right[par_Pos] = par_Color.red() + (par_Color.green() << 8) + (par_Color.blue() << 16);
        m_Data->LineSize_right[par_Pos] = par_LineWidth;
    } else {
        m_Data->min_left[par_Pos] = par_Min;
        m_Data->max_left[par_Pos] = par_Max;
        m_Data->color_left[par_Pos] = par_Color.red() + (par_Color.green() << 8) + (par_Color.blue() << 16);
        m_Data->LineSize_left[par_Pos] = par_LineWidth;
    }

    if (par_DisplayMode == DISPLAY_MODE_UNKNOWN) {
        switch (get_bbvari_conversiontype(vid)) {
        case BB_CONV_FORMULA:
        case BB_CONV_TEXTREP:
            par_DisplayMode = DISPLAY_MODE_PHYS;
            break;
        default:
            break;
        }
    }
    switch (par_DisplayMode) {
    default:
    case DISPLAY_MODE_DEC:
        if (par_Side == Right) {
            m_Data->dec_phys_right[par_Pos] = 0;
            m_Data->dec_hex_bin_right[par_Pos] = 0;
        } else {
            m_Data->dec_phys_left[par_Pos] = 0;
            m_Data->dec_hex_bin_left[par_Pos] = 0;
        }
        break;
    case DISPLAY_MODE_PHYS:
        if (par_Side == Right) {
            m_Data->dec_phys_right[par_Pos] = 1;
            m_Data->dec_hex_bin_right[par_Pos] = 0;
        } else {
            m_Data->dec_phys_left[par_Pos] = 1;
            m_Data->dec_hex_bin_left[par_Pos] = 0;
        }
        break;
    case DISPLAY_MODE_HEX:
        if (par_Side == Right) {
            m_Data->dec_phys_right[par_Pos] = 0;
            m_Data->dec_hex_bin_right[par_Pos] = 1;
        } else {
            m_Data->dec_phys_left[par_Pos] = 0;
            m_Data->dec_hex_bin_left[par_Pos] = 1;
        }
        break;
    case DISPLAY_MODE_BIN:
        if (par_Side == Right) {
            m_Data->dec_phys_right[par_Pos] = 0;
            m_Data->dec_hex_bin_right[par_Pos] = 2;
        } else {
            m_Data->dec_phys_left[par_Pos] = 0;
            m_Data->dec_hex_bin_left[par_Pos] = 2;
        }
        break;
    }

    switch (par_Alignment) {
    default:
    case DISPLAY_LEFT_ALIGNED:
        if (par_Side == Right) {
            m_Data->txt_align_right[par_Pos] = 0;
        } else {
            m_Data->txt_align_left[par_Pos] = 0;
        }
        break;
    case DISPLAY_RIGHT_ALIGNED:
        if (par_Side == Right) {
            m_Data->txt_align_right[par_Pos] = 1;
        } else {
            m_Data->txt_align_left[par_Pos] = 1;
        }
        break;
    }
    if (par_Side == Right) {
        m_Data->presentation_right[par_Pos] = 0;
    } else {
        m_Data->presentation_left[par_Pos] = 0;
    }

    if (update_vid_owc (m_Data)) {
        ThrowError (1, "cannot update variable list");
    }
    return 0;
}

CustomDialogFrame* OscilloscopeWidget::dialogSettings(QWidget* arg_parent)
{
    OszilloscopeDialogFrame *loc_frame = new OszilloscopeDialogFrame(m_Data, arg_parent);
    QList<QColor> Colors = GetAllUsedColors();
    loc_frame->SetUsedColors(Colors);
    arg_parent->setWindowTitle("Oszilloscpe signal");
    return loc_frame;
}

void OscilloscopeWidget::DeleteSignal(Side par_Side, int par_Number)
{
    if (par_Side == Left) {
        m_Data->sel_left_right = 0; /* 1 -> right 0 -> left */
        m_Data->sel_pos_left = par_Number;
    } else {
        m_Data->sel_left_right = 1; /* 1 -> right 0 -> left */
        m_Data->sel_pos_right = par_Number;
    }
    DeleteSignal();
}

QList<QColor> OscilloscopeWidget::GetAllUsedColors()
{
    QList<QColor> Ret;
    if (m_Data->right_axis_on) {
        for(int x = 0; x < 20; x++) {
            if (m_Data->vids_right[x] > 0) {
                int IntColor = m_Data->color_right[x];
                Ret.append(QColor ((IntColor >> 0) & 0xFF, (IntColor >> 8) & 0xFF, (IntColor >> 16) & 0xFF));
            }
        }
    }
    if (m_Data->left_axis_on) {
        for(int x = 0; x < 20; x++) {
            if (m_Data->vids_left[x] > 0) {
                int IntColor = m_Data->color_left[x];
                Ret.append(QColor ((IntColor >> 0) & 0xFF, (IntColor >> 8) & 0xFF, (IntColor >> 16) & 0xFF));
            }
        }
    }
    return Ret;
}

void OscilloscopeWidget::OnOffDescrFrame()
{
    if (m_Data->right_axis_on) {
        if (m_RightDescFrame == nullptr) {
            m_RightDescFrame = new OscilloscopeDescFrame (this, m_Data, Right, this);
            m_RightDescFrame->setMaximumWidth (m_Data->width_all_right_desc * (m_Data->FontAverageCharWidth <<1));
            m_Layout->addWidget (m_RightDescFrame, 0, 4, 1, 1);
#ifdef UNIFORM_DIALOGE
            connect(m_RightDescFrame, SIGNAL(variableForStandardDialog(QStringList,bool,bool,QColor,QFont)), this, SIGNAL(openStandardDialog(QStringList,bool,bool,QColor,QFont)));
#endif
        }
        if (m_RightYAxis == nullptr) {
            m_RightYAxis = new OscilloscopeYAxis (this, m_Data, OscilloscopeYAxis::Side::Right, this);
            m_Layout->addWidget (m_RightYAxis,     0, 3, 1, 1);
        }
        if (m_RightStatus == nullptr) {
            m_RightStatus = new OscilloscopeStatus (m_Data, this);
            m_Layout->addWidget (m_RightStatus,    1, 3, 1, 2);
        }
    } else {
        if (m_RightDescFrame != nullptr) {
            delete m_RightDescFrame;
            m_RightDescFrame = nullptr;
        }
        if (m_RightYAxis != nullptr) {
            delete m_RightYAxis;
            m_RightYAxis = nullptr;
        }
        if (m_RightStatus != nullptr) {
            delete m_RightStatus;
            m_RightStatus = nullptr;
        }
    }
    if (m_Data->left_axis_on) {
        if (m_LeftDescFrame == nullptr) {
            m_LeftDescFrame = new OscilloscopeDescFrame (this, m_Data, Left, this);
            m_LeftDescFrame->setMaximumWidth (m_Data->width_all_left_desc * (m_Data->FontAverageCharWidth << 1));
            m_Layout->addWidget (m_LeftDescFrame,  0, 0, 1, 1);
#ifdef UNIFORM_DIALOGE
            connect(m_LeftDescFrame, SIGNAL(variableForStandardDialog(QStringList,bool,bool,QColor,QFont)), this, SIGNAL(openStandardDialog(QStringList,bool,bool,QColor,QFont)));
#endif
        }
        if (m_LeftYAxis == nullptr) {
            m_LeftYAxis = new OscilloscopeYAxis (this, m_Data, OscilloscopeYAxis::Side::Left, this);
            m_Layout->addWidget (m_LeftYAxis,      0, 1, 1, 1);
        }
        if (m_LeftStatus == nullptr) {
            m_LeftStatus = new OscilloscopeStatus (m_Data, this);
            m_Layout->addWidget (m_LeftStatus,     1, 0, 1, 2);
        }
    } else {
        if (m_LeftDescFrame != nullptr) {
            delete m_LeftDescFrame;
            m_LeftDescFrame = nullptr;
        }
        if (m_LeftYAxis != nullptr) {
            delete m_LeftYAxis;
            m_LeftYAxis = nullptr;
        }
        if (m_LeftStatus != nullptr) {
            delete m_LeftStatus;
            m_LeftStatus = nullptr;
        }
    }
}
