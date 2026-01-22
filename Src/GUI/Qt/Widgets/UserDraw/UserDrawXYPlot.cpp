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


#include "UserDrawXYPlot.h"
#include "UserDrawSplitParameters.h"
#include "StringHelpers.h"

extern "C" {
#include "Config.h"
#include "MemZeroAndCopy.h"
#include "OscilloscopeCyclic.h"
#include "ThrowError.h"
#include "MyMemory.h"
#include "GetNotANumber.h"
#include "my_udiv128.h"
}

UserDrawXYPlot::UserDrawXYPlot(int par_Pos, QString &par_ParameterString, UserDrawElement *par_Parent) : UserDrawElement(par_Parent)
{
    STRUCT_ZERO_INIT (m_Data, OSCILLOSCOPE_DATA);
    m_Data.NotANumber = GetNotANumber();
    m_Data.CriticalSectionNumber = AllocOsciWidgetCriticalSection ();
    if (m_Data.CriticalSectionNumber < 0) {
        ThrowError (1, "too many oscilloscopes open (max. %i allowed)", MAX_OSCILLOSCOPE_WINDOWS);
    }
    RegisterOscilloscopeToCyclic(&m_Data);
    InitParameter(par_Pos, par_ParameterString);
}

UserDrawXYPlot::~UserDrawXYPlot()
{
    ResetToDefault();
    UnregisterOscilloscopeFromCyclic(&m_Data);
    DeleteOsciWidgetCriticalSection (m_Data.CriticalSectionNumber);
}

void UserDrawXYPlot::ResetToDefault()
{
    UserDrawElement::ResetToDefault();

    m_Properties.Add(QString("buffer"), QString("1024"));
    m_Properties.Add(QString("points"), QString(), UserDrawPropertiesListElem::UserDrawPropertyXYPlotSignalType);

    int i;

    EnterOsciWidgetCriticalSection (m_Data.CriticalSectionNumber);

    /* bein Oszi-Prozess alle Varis abmelden und Speicher freigeben */
    if (m_Data.trigger_vid > 0) remove_bbvari_unknown_wait (m_Data.trigger_vid);
    if (m_Data.name_trigger != nullptr) my_free (m_Data.name_trigger);
    m_Data.name_trigger = nullptr;
    for (i = 0; i < 21; i++) {
        if (m_Data.vids_left[i]) remove_bbvari_unknown_wait (m_Data.vids_left[i]);
        m_Data.vids_left[i] = 0;
        if (m_Data.name_left[i] != nullptr) my_free (m_Data.name_left[i]);
        m_Data.name_left[i] = nullptr;
        if (m_Data.vids_right[i]) remove_bbvari_unknown_wait (m_Data.vids_right[i]);
        m_Data.vids_right[i] = 0;
        if (m_Data.name_right[i] != nullptr) my_free (m_Data.name_right[i]);
        m_Data.name_right[i] = nullptr;
    }
    LeaveOsciWidgetCriticalSection (m_Data.CriticalSectionNumber);
    update_vid_owc (&m_Data);
    EnterOsciWidgetCriticalSection (m_Data.CriticalSectionNumber);

    for (i = 0; i < 21; i++) {
        /* Spuren-Puffer loeschen */
        if (m_Data.buffer_left[i] != nullptr) my_free (m_Data.buffer_left[i]);
        m_Data.buffer_left[i] = nullptr;
        if (m_Data.buffer_right[i] != nullptr) my_free (m_Data.buffer_right[i]);
        m_Data.buffer_right[i] = nullptr;
    }

    LeaveOsciWidgetCriticalSection (m_Data.CriticalSectionNumber);
}



void UserDrawXYPlot::ParseOneSignal(QString &par_Signal)
{
    QStringList InList = SplitParameters(par_Signal);
    if (InList.size() >= 11) {
        QString ValueString;
        BaseValue Value;
        // X
        m_XYIns.append(InList.at(0).trimmed());
        ValueString = InList.at(1).trimmed();
        m_XYPhys.append(ValueString.compare("phys", Qt::CaseInsensitive) == 0);
        ValueString = InList.at(2).trimmed();
        Value.Parse(ValueString, true);
        m_XYMin.append(Value.Get());
        ValueString = InList.at(3).trimmed();
        Value.Parse(ValueString, true);
        m_XYMax.append(Value.Get());
        // Y
        m_XYIns.append(InList.at(4).trimmed());
        ValueString = InList.at(5).trimmed();
        m_XYPhys.append(ValueString.compare("phys", Qt::CaseInsensitive) == 0);
        ValueString = InList.at(6).trimmed();
        Value.Parse(ValueString, true);
        m_XYMin.append(Value.Get());
        ValueString = InList.at(7).trimmed();
        Value.Parse(ValueString, true);
        m_XYMax.append(Value.Get());
        ValueString = InList.at(8).trimmed();
        ColorValue Color;
        Color.Parse(ValueString, true);
        m_Color.append(Color.GetInt());
        ValueString = InList.at(9).trimmed();
        m_LineOrPoints.append(ValueString.compare("point", Qt::CaseInsensitive) == 0);
        ValueString = InList.at(10).trimmed();
        Value.Parse(ValueString, true);
        m_Width.append(Value.Get());
    }
}


bool UserDrawXYPlot::ParseOneParameter(QString &par_Value)
{
    BaseValue Value;
    QStringList List = par_Value.split('=');
    if (List.size() == 2) {
        QString Key = List.at(0).trimmed();
        QString ValueString = List.at(1).trimmed();
        if (!Key.compare("buffer", Qt::CaseInsensitive)) {
            Value.Parse(ValueString);
            m_BufferDepth = (int)Value.Get();
            m_Properties.Add(QString("buffer"), ValueString);
        } else if (!Key.compare("points", Qt::CaseInsensitive) ||
                   !Key.compare("p", Qt::CaseInsensitive)) {
            int Len = ValueString.length();
            if ((Len > 0) && (ValueString.at(0) != '(')) {
                ThrowError (1, "expecting a '(' in \"%s\"", QStringToConstChar(ValueString));
                return false;
            }
            int BraceCounter = 0;
            int StartParameter = 0;
            for (int x = 0; x < Len; x++) {
                QChar c = ValueString.at(x);
                if (c == '(') {
                    BraceCounter++;
                    if (BraceCounter == 1) {
                        StartParameter = x;
                    }
                } else if (c == ')') {
                    if ((BraceCounter == 0) && (x < (Len - 1))) {
                        ThrowError (1, "missing '(' in \"%s\"", QStringToConstChar(ValueString));
                        return false;
                    }
                    if (BraceCounter == 1) {
                        QString Parameter = ValueString.mid(StartParameter+1, x - StartParameter-1);
                        ParseOneSignal(Parameter);
                        StartParameter = x;
                    }
                    BraceCounter--;
                } else if (c == ',') {
                    if (BraceCounter == 1) {
                        QString Parameter = ValueString.mid(StartParameter+1, x - StartParameter-1);
                        ParseOneSignal(Parameter);
                        StartParameter = x;
                    }
                }
            }
            m_Properties.Add("points", ValueString, UserDrawPropertiesListElem::UserDrawPropertyXYPlotSignalType);
        } else {
            return ParseOneBaseParameter(Key, ValueString);
        }
        return true;
    }
    return false;
}

void UserDrawXYPlot::Paint(QPainter *par_Painter, QList<UserDrawImageItem*> *par_Images, DrawParameter &par_Draw)
{
    Q_UNUSED(par_Images)
    if (m_Visible.Get() >= 0.5) {
        SetColor(par_Painter);
        TranslateParameter P(m_X.Get(),
                             m_Y.Get(),
                             m_Scale.Get(),
                             m_Rot.Get(),
                             par_Draw);
        for (int x = 0; x < m_SignalCount; x+=2) {
            if (x < 20) {
                // Left side
                int i = x;
                if (((m_Data.vids_left[i] > 0) && (!m_Data.vars_disable_left[i])) &&     // for a valid xy view there must be 2 signals in a row
                    ((m_Data.vids_left[i+1] > 0) && (!m_Data.vars_disable_left[i+1]))) {
                    QPen Pen;
                    Pen.setWidth (m_Data.LineSize_left[i]);
                    int c = m_Data.color_left[i];
                    int r = c & 0xFF;
                    int g = (c >> 8) & 0xFF;
                    int b = (c >> 16) & 0xFF;
                    QColor Color (r, g, b);
                    Pen.setColor (Color);
                    par_Painter->setPen (Pen);

                    bool valid_m1 = false;
                    double x_m1_d, y_m1_d;

                    double YFactor = 1.0 / (m_Data.max_left[i] - m_Data.min_left[i]);
                    double YOffset = -m_Data.min_left[i] * YFactor;
                    double XFactor = 1.0 / (m_Data.max_left[i+1] - m_Data.min_left[i+1]);
                    double XOffset = -m_Data.min_left[i+1] * XFactor;

                    int new_since_last_draw = m_Data.new_since_last_draw_left[i] + 2;
                    int ToPaintPoints =  (m_Data.depth_left[i] < m_Data.depth_left[i+1]) ? m_Data.depth_left[i] : m_Data.depth_left[i+1];
                    if (!m_ClearFlag) {
                        ToPaintPoints = (new_since_last_draw < ToPaintPoints) ? new_since_last_draw : ToPaintPoints;
                    }
                    m_Data.new_since_last_draw_left[i] = 0;
                    for (int Idx = 1; Idx < ToPaintPoints; Idx++) {
                        int IdxY = m_Data.wr_poi_left[i] - Idx;
                        if (IdxY < 0) IdxY = m_Data.buffer_depth + IdxY;
                        int IdxX = m_Data.wr_poi_left[i+1] - Idx;
                        if (IdxX < 0) IdxX = m_Data.buffer_depth + IdxX;

                        double ValueY = m_Data.buffer_left[i][IdxY];
                        double ValueX = m_Data.buffer_left[i+1][IdxX];
                        if (!_isnan (ValueX) && !_isnan (ValueY)) {

                            double y_d = ValueY * YFactor + YOffset;
                            double x_d = ValueX * XFactor + XOffset;

                            Translate(&x_d, &y_d, P);

                            if (m_Data.presentation_left[i] == 1) {  // only the y axis will define this
                                QPointF Point(x_d, y_d);
                                par_Painter->drawPoint(Point);
                            } else {
                                if (valid_m1) { // Value before was also valid
                                    QLineF Line (x_m1_d, y_m1_d, x_d, y_d);
                                    par_Painter->drawLine (Line);
                                    x_m1_d = x_d;
                                    y_m1_d = y_d;
                                } else {
                                    // Value before was not valid only stor the current value
                                    x_m1_d = x_d;
                                    y_m1_d = y_d;
                                }
                            }
                            valid_m1 = true;
                        } else {
                            valid_m1 = false;
                        }
                    }
                }
            } else if (x < 40) {
                // Right side
                int i = x - 20;
                if (((m_Data.vids_right[i] > 0) && (!m_Data.vars_disable_right[i])) &&     // for a valid xy view there must be 2 signals in a row
                    ((m_Data.vids_right[i+1] > 0) && (!m_Data.vars_disable_right[i+1]))) {
                    QPen Pen;
                    Pen.setWidth (m_Data.LineSize_right[i]);
                    int c = m_Data.color_right[i];
                    int r = c & 0xFF;
                    int g = (c >> 8) & 0xFF;
                    int b = (c >> 16) & 0xFF;
                    QColor Color (r, g, b);
                    Pen.setColor (Color);
                    par_Painter->setPen (Pen);

                    bool valid_m1 = false;
                    double x_m1_d, y_m1_d;

                    double YFactor = 1.0 / (m_Data.max_right[i] - m_Data.min_right[i]);
                    double YOffset = -m_Data.min_right[i] * YFactor;
                    double XFactor = 1.0 / (m_Data.max_right[i+1] - m_Data.min_right[i+1]);
                    double XOffset = -m_Data.min_right[i+1] * XFactor;

                    int new_since_last_draw = m_Data.new_since_last_draw_right[i] + 2;
                    int ToPaintPoints =  (m_Data.depth_right[i] < m_Data.depth_right[i+1]) ? m_Data.depth_right[i] : m_Data.depth_right[i+1];
                    if (!m_ClearFlag) {
                        ToPaintPoints = (new_since_last_draw < ToPaintPoints) ? new_since_last_draw : ToPaintPoints;
                    }
                    m_Data.new_since_last_draw_right[i] = 0;
                    for (int Idx = 1; Idx < ToPaintPoints; Idx++) {
                        int IdxY = m_Data.wr_poi_right[i] - Idx;
                        if (IdxY < 0) IdxY = m_Data.buffer_depth + IdxY;
                        int IdxX = m_Data.wr_poi_right[i+1] - Idx;
                        if (IdxX < 0) IdxX = m_Data.buffer_depth + IdxX;

                        double ValueY = m_Data.buffer_right[i][IdxY];
                        double ValueX = m_Data.buffer_right[i+1][IdxX];
                        if (!_isnan (ValueX) && !_isnan (ValueY)) {

                            double y_d = ValueY * YFactor + YOffset;
                            double x_d = ValueX * XFactor + XOffset;

                            Translate(&x_d, &y_d, P);

                            if (m_Data.presentation_right[i] == 1) {  // only the y axis will define this
                                QPointF Point(x_d, y_d);
                                par_Painter->drawPoint(Point);
                            } else {
                                if (valid_m1) { // Wert davor war auch gueltig
                                    QLineF Line (x_m1_d, y_m1_d, x_d, y_d);
                                    par_Painter->drawLine (Line);
                                    x_m1_d = x_d;
                                    y_m1_d = y_d;
                                } else {
                                    // Value before was not valid only stor the current value
                                    x_m1_d = x_d;
                                    y_m1_d = y_d;
                                }
                            }
                            valid_m1 = true;
                        } else {
                            valid_m1 = false;
                        }
                    }
                }
            }
        }
    }
}

bool UserDrawXYPlot::Init()
{
    // Init. data
    m_SignalCount = m_XYIns.count();
    m_ClearFlag = true; // paint all new

    InitDataStruct();
    return true;
}

QString UserDrawXYPlot::GetTypeString()
{
    return QString("XYPlot");
}

int UserDrawXYPlot::Is(QString &par_Line)
{
    if (par_Line.startsWith("XYPlot", Qt::CaseInsensitive)) {
        return 6;
    } else {
        return 0;
    }
}

void UserDrawXYPlot::SetDefaultParameterString()
{
    m_ParameterLine = QString("(buffer=1024)");
}

bool UserDrawXYPlot::InitDataStruct()
{
    m_Data.xy_view_flag = 1;
    m_Data.BackgroundImageLoaded = NO_BACKGROUND_IMAGE;
    m_Data.left_axis_on = 1;
    m_Data.right_axis_on = 1;
    m_Data.buffer_depth = m_BufferDepth;
    if (m_Data.buffer_depth < 1024) m_Data.buffer_depth = 1024;
    if (m_Data.buffer_depth > 1000000) m_Data.buffer_depth = 1000000;
    m_Data.sample_steps =1;
    m_Data.window_step_width = 20;

    m_Data.t_window_size_configered = 10.0;
    if (m_Data.t_window_size_configered < 0.001) m_Data.t_window_size_configered = 0.001;
    if (m_Data.t_window_size_configered > 1000.0) m_Data.t_window_size_configered = 1000.0;
    m_Data.t_window_size = (uint64_t)(m_Data.t_window_size_configered * TIMERCLKFRQ);
    m_Data.t_window_start = 0;
    m_Data.t_window_end = m_Data.t_window_size;

    m_Data.t_step_win = my_umuldiv64(m_Data.t_window_end - m_Data.t_window_start, (uint64_t)m_Data.window_step_width, 100);

    // no trigger
    m_Data.trigger_flag = 0;
    m_Data.trigger_value = 0.0;
    m_Data.trigger_vid = 0;
    m_Data.pre_trigger = 0;

    ENTER_GCS ();
    for (int s = 0; s < m_SignalCount; s++) {
        if (s < 20) {  // first 20 signals are on the left side of the oscilloscope
            int x = s;
            m_Data.LineSize_left[x]  = 1;
            // Add variable to blackboard
            QString SignalName = m_XYIns.at(s);
            int vid = add_bbvari(QStringToConstChar(SignalName), BB_UNKNOWN_WAIT, NULL);
            if (vid > 0) {
                m_Data.vids_left[x] = vid;
                m_Data.name_left[x] = ReallocCopyString(m_Data.name_left[x], SignalName);
                if ((m_Data.buffer_left[x] = (double*)my_calloc ((size_t)m_Data.buffer_depth, sizeof(double))) == NULL) {
                    ThrowError (1, "no free memory");
                    m_Data.vids_left[x] = 0;
                }
                // min
                m_Data.min_left[x] = m_XYMin.at(s);
                // max
                m_Data.max_left[x] = m_XYMax.at(s);
                if (m_Data.max_left[x] <= m_Data.min_left[x]) m_Data.max_left[x] = m_Data.min_left[x] + 1.0;
                // Color
                m_Data.color_left[x] = m_Color.at(s/2); // only half the size!
                // Phys. or dec.
                m_Data.dec_phys_left[x] = m_XYPhys.at(s);
                m_Data.dec_hex_bin_left[x] = 0;
                // Line width
                m_Data.LineSize_left[x] = m_Width.at(s/2); // only half the size!
                // disable (crossed out)
                m_Data.vars_disable_left[x] = 0;
                // Lines or points
                m_Data.presentation_left[x] = m_LineOrPoints.at(s/2); // only half the size!
            }
        } else if (s < 40) {  // the next 20 signals are on the right side of the oscilloscope
            int x = s - 20;
            m_Data.LineSize_right[x]  = 1;
            // Add variable to blackboard
            QString SignalName = m_XYIns.at(s);
            int vid = add_bbvari(QStringToConstChar(SignalName), BB_UNKNOWN_WAIT, NULL);
            if (vid > 0) {
                m_Data.vids_right[x] = vid;
                m_Data.name_right[x] = ReallocCopyString(m_Data.name_right[x], SignalName);
                if ((m_Data.buffer_right[x] = (double*)my_calloc ((size_t)m_Data.buffer_depth, sizeof(double))) == NULL) {
                    ThrowError (1, "no free memory");
                    m_Data.vids_right[x] = 0;
                }
                // min
                m_Data.min_right[x] = m_XYMin.at(s);
                // max
                m_Data.max_right[x] = m_XYMax.at(s);
                if (m_Data.max_right[x] <= m_Data.min_right[x]) m_Data.max_right[x] = m_Data.min_right[x] + 1.0;
                // Color
                m_Data.color_right[x] = m_Color.at(s/2); // only half the size!
                // Phys. or dec.
                m_Data.dec_phys_right[x] = m_XYPhys.at(s);
                m_Data.dec_hex_bin_right[x] = 0;
                // Line width
                m_Data.LineSize_right[x] = m_Width.at(s/2); // only half the size!
                // disable (crossed out)
                m_Data.vars_disable_right[x] = 0;
                // Lines or points
                m_Data.presentation_right[x] = m_LineOrPoints.at(s/2); // only half the size!
            }
        }
    }
    m_Data.state = 1;
    LEAVE_GCS();
    update_vid_owc (&m_Data);
    return true;
}
