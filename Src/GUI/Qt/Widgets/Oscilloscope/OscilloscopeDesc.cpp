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

#include <inttypes.h>
#include <QEvent>
#include <QMimeData>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QApplication>
#include <QMenu>
#include <QAction>


#include "OscilloscopeDesc.h"
#include "OscilloscopeCyclic.h"
#include "OscilloscopeDescFrame.h"
#include "OscilloscopeSelectVariableDialog.h"
#include "DragAndDrop.h"
#include "BlackboardInfoDialog.h"
#include "GetEventPos.h"
#include "ColorHelpers.h"
#include "StringHelpers.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "StringMaxChar.h"
#include "PrintFormatToString.h"
#include "Blackboard.h"
#include "BlackboardAccess.h"
#include "ThrowError.h"
#include "MainValues.h"

#define UNIFORM_DIALOGE

extern int DisplayUnitForNonePhysicalValues;

#ifdef __cplusplus
}
#endif

OscilloscopeDesc::OscilloscopeDesc(OscilloscopeDescFrame *par_OscilloscopeDescFrame, OSCILLOSCOPE_DATA *par_Data, enum Side par_Side, int par_Number, QWidget *parent) : QWidget(parent)
{
    m_OscilloscopeDescFrame = par_OscilloscopeDescFrame;
    m_Data = par_Data;
    m_Side = par_Side;
    m_Number = par_Number;

    setMaximumHeight(OSCILLOSCOPE_DESC_HIGHT);
    setMinimumHeight(OSCILLOSCOPE_DESC_HIGHT);

    setBackgroundRole(QPalette::Window);
    setAutoFillBackground(true);
    setAcceptDrops(true);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customContextMenu(QPoint)));
}

OscilloscopeDesc::~OscilloscopeDesc()
{
}

void OscilloscopeDesc::openDialog()
{
#ifdef UNIFORM_DIALOGE
    QStringList loc_variable;
    char loc_name[BBVARI_NAME_SIZE];
    QColor loc_color;
    if(m_Data->sel_left_right) {
        if (m_Data->vids_right[m_Data->sel_pos_right] > 0) {
            GetBlackboardVariableName(m_Data->vids_right[m_Data->sel_pos_right], loc_name, sizeof(loc_name));
            loc_variable.append(QString(loc_name));
        }
        loc_color = QColor((m_Data->color_right[m_Data->sel_pos_right]&0x00FF0000)>>16, (m_Data->color_right[m_Data->sel_pos_right]&0x0000FF00)>>8, m_Data->color_right[m_Data->sel_pos_right]&0x000000FF);
    } else {
        if (m_Data->vids_right[m_Data->sel_pos_right] > 0) {
            GetBlackboardVariableName(m_Data->vids_right[m_Data->sel_pos_right], loc_name, sizeof(loc_name));
            loc_variable.append(QString(loc_name));
        }
        loc_color = QColor((m_Data->color_left[m_Data->sel_pos_left]&0x00FF0000)>>16, (m_Data->color_left[m_Data->sel_pos_left]&0x0000FF00)>>8, m_Data->color_left[m_Data->sel_pos_left]&0x000000FF);
    }
    emit variableForStandardDialog(loc_variable, true, false, loc_color);
#else
    OscilloscopeSelectVariableDialog Dlg (m_Data);
    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
#endif
}

void OscilloscopeDesc::deleteSignal()
{
    m_OscilloscopeDescFrame->DeleteSignal(m_Number);
    update();
}

void OscilloscopeDesc::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText()) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
        }
    } else {
        event->ignore();
    }
}

void OscilloscopeDesc::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasText()) {
        const QMimeData *mime = event->mimeData();
        DragAndDropInfos Infos (mime->text());

        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        } else {
            event->acceptProposedAction();
            m_OscilloscopeDescFrame->DropSignal(m_Side, m_Number, &Infos);
        }
    } else {
        event->ignore();
    }
}

void OscilloscopeDesc::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_OscilloscopeDescFrame->SetSelectedSignal(m_Side, m_Number);
        m_startDragPosition = GetEventGlobalPos(event);
    } else if (event->button() == Qt::RightButton) {
        m_OscilloscopeDescFrame->SetSelectedSignal(m_Side, m_Number);
    }
}

void OscilloscopeDesc::mouseMoveEvent(QMouseEvent* event)
{
    if(!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    if((GetEventGlobalPos(event) - m_startDragPosition).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }
    if (((m_Side == Left) && (m_Data->vids_left[m_Number] > 0)) ||
        ((m_Side == Right) && (m_Data->vids_right[m_Number] > 0))) {
        char loc_variableName[BBVARI_NAME_SIZE];
        int vid;
        double min, max;
        int IntColor;
        int LineWidth;
        enum DragAndDropAlignment Alignment;
        enum DragAndDropDisplayMode DisplayMode;

        if (m_Side == Left) {
            vid = m_Data->vids_left[m_Number];
            min = m_Data->min_left[m_Number];
            max = m_Data->max_left[m_Number];
            IntColor = m_Data->color_left[m_Number];
            LineWidth = m_Data->LineSize_left[m_Number];
            if (m_Data->txt_align_left[m_Number]) {
                Alignment = DISPLAY_RIGHT_ALIGNED;
            } else {
                Alignment = DISPLAY_LEFT_ALIGNED;
            }
            if (m_Data->dec_phys_left[m_Number]) {
                DisplayMode = DISPLAY_MODE_PHYS;
            } else {
                switch (m_Data->dec_hex_bin_left[m_Number]){
                default:
                case 0:
                    DisplayMode = DISPLAY_MODE_DEC;
                    break;
                case 1:
                    DisplayMode = DISPLAY_MODE_HEX;
                    break;
                case 2:
                    DisplayMode = DISPLAY_MODE_BIN;
                    break;
                }
            }
        } else {
            vid = m_Data->vids_right[m_Number];
            min = m_Data->min_right[m_Number];
            max = m_Data->max_right[m_Number];
            IntColor = m_Data->color_right[m_Number];
            LineWidth = m_Data->LineSize_right[m_Number];
            if (m_Data->txt_align_right[m_Number]) {
                Alignment = DISPLAY_RIGHT_ALIGNED;
            } else {
                Alignment = DISPLAY_LEFT_ALIGNED;
            }
            if (m_Data->dec_phys_right[m_Number]) {
                DisplayMode = DISPLAY_MODE_PHYS;
            } else {
                switch (m_Data->dec_hex_bin_right[m_Number]){
                default:
                case 0:
                    DisplayMode = DISPLAY_MODE_DEC;
                    break;
                case 1:
                    DisplayMode = DISPLAY_MODE_HEX;
                    break;
                case 2:
                    DisplayMode = DISPLAY_MODE_BIN;
                    break;
                }
            }
        }
        GetBlackboardVariableName(vid, loc_variableName, sizeof(loc_variableName));

        int r = IntColor & 0xFF;
        int g = (IntColor >> 8) & 0xFF;
        int b = (IntColor >> 16) & 0xFF;
        QColor Color (r, g, b);

        DragAndDropInfos Infos;
        QString Name(loc_variableName);
        Infos.SetName(Name);
        Infos.SetColor(Color);
        Infos.SetMinMaxValue(min, max);
        Infos.SetDisplayMode(DisplayMode);
        Infos.SetAlignment(Alignment);
        Infos.SetLineWidth(LineWidth);
        QDrag *loc_dragObject = buildDragObject(this, &Infos);
        loc_dragObject->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
    }
}

void OscilloscopeDesc::BuildHexString (const char *Prefix, uint64_t Value, int Type, char *String)
{
    switch (Type) {
    case BB_BYTE:
    case BB_UBYTE:
        PrintFormatToString (String, sizeof(String), "%s 0x%02X", Prefix, static_cast<uint32_t>(Value));
        break;
    case BB_WORD:
    case BB_UWORD:
        PrintFormatToString (String, sizeof(String), "%s 0x%04X", Prefix, static_cast<uint32_t>(Value));
        break;
    default:
    case BB_DWORD:
    case BB_UDWORD:
        PrintFormatToString (String, sizeof(String), "%s 0x%08X", Prefix, static_cast<uint32_t>(Value));
        break;
    case BB_QWORD:
    case BB_UQWORD:
        PrintFormatToString (String, sizeof(String), "%s 0x%016" PRIX64, Prefix, Value);
        break;
    }
}

void OscilloscopeDesc::BuildBinaryString (const char *Prefix, uint64_t Value, int Type, char *String)
{
    int x;
    int Size;

    switch (Type) {
    case BB_BYTE:
    case BB_UBYTE:
        Size = 8;
        break;
    case BB_WORD:
    case BB_UWORD:
        Size = 16;
        break;
    default:
    case BB_DWORD:
    case BB_UDWORD:
        Size = 32;
        break;
    case BB_QWORD:
    case BB_UQWORD:
        Size = 64;
        break;    }

    while (*Prefix != 0) {
        *String++ = *Prefix++;
    }
    *String++ = ' ';

    for (x = Size - 1; x >= 0; x--) {
        if (Value & (1ULL << x)) {
            *String++ = '1';
        } else {
            *String++ = '0';
        }
    }
    *String++ = 'b';
    *String = 0;
}

int OscilloscopeDesc::CalcDiffEnumString (double Value, int left_right, int pos,
                                          char *DiffEnumStr, int maxc,  int *pcolor)
{
    int help;
    double RefValue;

    if (left_right) {  // rechts
        RefValue = FiFoPosRight (m_Data, pos, m_Data->t_ref_point);
    } else {
        RefValue = FiFoPosLeft (m_Data, pos, m_Data->t_ref_point);
    }
    if (!_isnan (RefValue)) {
        if (left_right) {
            convert_value_textreplace (m_Data->vids_right[pos],
                                       static_cast<int32_t>(RefValue), DiffEnumStr, maxc - 4, pcolor);
            StringAppendMaxCharTruncate (DiffEnumStr, " -> ", maxc);
            help = static_cast<int>(strlen (DiffEnumStr));
            convert_value_textreplace (m_Data->vids_right[pos],
                                       static_cast<int32_t>(Value), DiffEnumStr + help, maxc - help - 1, pcolor);
        } else {
            convert_value_textreplace (m_Data->vids_left[pos],
                                       static_cast<int32_t>(RefValue), DiffEnumStr, maxc - 4, pcolor);
            StringAppendMaxCharTruncate (DiffEnumStr, " -> ", maxc);
            help = static_cast<int>(strlen (DiffEnumStr));
            convert_value_textreplace (m_Data->vids_left[pos],
                                       static_cast<int32_t>(Value), DiffEnumStr + help, maxc - help - 1, pcolor);
        }
        return 0;
    } else {
        StringCopyMaxCharTruncate (DiffEnumStr, "no valid ref point", maxc);
        return -1;
    }
}

int OscilloscopeDesc::ValueToString (int Pos,
                                     const char *Prefix, int left_right, int Vid, double Value,
                                     int dec_phys_flag, int dec_hex_bin_flag, int Type,
                                     char *String, int maxc, int *pcolor)
{
    if (_isnan (Value)) {
        StringCopyMaxCharTruncate (String, "doesn't exist", maxc);
        return -1;
    } else {
        if ((get_bbvari_conversiontype (Vid) == 2) && dec_phys_flag) {
            if (m_Data->diff_to_ref_point_flag) {
                CalcDiffEnumString (Value, left_right, Pos,
                                    String, 100, pcolor);
            } else {
                convert_value_textreplace (Vid, static_cast<int32_t>(Value), String, 100, pcolor);
            }
       } else {
            if (m_Data->diff_to_ref_point_flag) {
                double RefValue;
                if (left_right) {  /* rechts */
                    RefValue = FiFoPosRight (m_Data, Pos, m_Data->t_ref_point);
                } else {
                    RefValue = FiFoPosLeft (m_Data, Pos, m_Data->t_ref_point);
                }
                if (!_isnan (RefValue)) {
                    Value -= RefValue;
                }
            }
            if (dec_phys_flag || (Type == BB_FLOAT) || (Type == BB_DOUBLE)) {
                PrintFormatToString (String, sizeof(String), "%s %*.*lf", Prefix,
                         get_bbvari_format_width (Vid),
                         get_bbvari_format_prec (Vid),
                         Value);
            } else {
                switch (dec_hex_bin_flag) {
                case 2:  // bin
                    BuildBinaryString (Prefix, static_cast<uint64_t>(Value), Type, String);
                    break;
                case 1:
                    BuildHexString (Prefix, static_cast<uint64_t>(Value), Type, String);
                    break;
                case 0:
                default:
                    PrintFormatToString (String, sizeof(String), "%s %0.0f", Prefix, Value);
                    break;
                }
            }
        }
       return 0;
    }
}

int OscilloscopeDesc::read_value_desc_txt (char *txt, int maxc, int left_right, int pos, int *pcolor)
{
    int ret;
    double value;
    uint64_t t;
    const char *Prefix = "";

    if (m_Data->state) { // on -> t_current
        t = m_Data->t_current_to_update_end;
    } else {                // off -> t_cursor
        t = m_Data->t_cursor;
    }
    if (left_right) {  /* right side */
        if (m_Data->buffer_right[pos] != nullptr) {
            value = FiFoPosRight (m_Data, pos, t);
            if (!_isnan (value)) {
                ret = ValueToString (pos,
                                     Prefix, left_right, m_Data->vids_right[pos], value,
                                     m_Data->dec_phys_right[pos], m_Data->dec_hex_bin_right[pos],
                                     get_bbvaritype (m_Data->vids_right[pos]), txt, maxc, pcolor);
            } else {
                StringCopyMaxCharTruncate (txt, "doesn't exist", maxc);
                ret = 0;
            }
        } else {
            txt[0] = 0;
            ret = 0;
        }
    } else {                     // left side
        if (m_Data->buffer_left[pos] != nullptr) {
            value = FiFoPosLeft (m_Data, pos, t);
            if (!_isnan (value)) {
                ret = ValueToString (pos,
                                     Prefix, left_right, m_Data->vids_left[pos], value,
                                     m_Data->dec_phys_left[pos], m_Data->dec_hex_bin_left[pos],
                                     get_bbvaritype (m_Data->vids_left[pos]), txt, maxc, pcolor);
            } else {
                StringCopyMaxCharTruncate (txt, "doesn't exist", maxc);
                ret = 0;
            }
        } else {
            txt[0] = 0;
            ret = 0;
        }
    }
    return ret;
}

void OscilloscopeDesc::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);
    paint(painter, true);
}

void OscilloscopeDesc::paint(QPainter &painter, bool border_flag)
{
    char vari_name[4+BBVARI_NAME_SIZE];
    int sel_pos;
    int font_height;
    int string_width;
    char value_string[BBVARI_NAME_SIZE];
    char unit[BBVARI_UNIT_SIZE];
    int win_width, win_height, wh3, wh8;
    int end_pos;
    int visable = 1;
    int color;

    // Only paint if it is complete visable
    end_pos = (parentWidget()->height() / height()) - 1;
    if ((m_Number > end_pos) || (m_Number > 19)) {
        return;
    }

    EnterOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);

    int SelectedFlag = 0;
    if (m_Side == Left) {
        if (m_Data->sel_left_right == 0) {
            if (m_Number == m_Data->sel_pos_left) {
                SelectedFlag = 1;
            }
        }
    } else if (m_Side == Right) {
        if (m_Data->sel_pos_right == 0) {
            if (m_Number == m_Data->sel_pos_right) {
                SelectedFlag = 1;
            }
        }
    }

    QString Value;
    if (m_Side == Left) {
        if (m_Data->vids_left[m_Number] > 0) {
            if (m_Data->xy_view_flag) {
                if ((m_Number & 0x1) == 0x1) {
                    STRING_COPY_TO_ARRAY(vari_name, "x = ");
                } else  {
                    STRING_COPY_TO_ARRAY(vari_name, "y = ");
                }
            } else vari_name[0] = 0;
            if (m_Data->name_left[m_Number] != nullptr) STRING_APPEND_TO_ARRAY (vari_name, m_Data->name_left[m_Number]);
            if (m_Data->vars_disable_left[m_Number]) {
                value_string[0] = 0;
            } else {
                visable = read_value_desc_txt (value_string, sizeof(value_string), 0, m_Number, &color) == 0;
                Value = QString(value_string);
                if ((m_Data->dec_phys_left[m_Number])  || DisplayUnitForNonePhysicalValues) {
                    get_bbvari_unit (m_Data->vids_left[m_Number], unit, sizeof (unit));
                    Value.append(" ");
                    Value.append(CharToQString(unit));
                }
            }
        } else {
            vari_name[0] = '\0';
            value_string[0] = '\0';
            visable = 0;
        }
    } else {
        if (m_Data->vids_right[m_Number] > 0) {
            if (m_Data->xy_view_flag) {
                if ((m_Number & 0x1) == 0x1) {
                    STRING_COPY_TO_ARRAY(vari_name, "x = ");
                } else  {
                    STRING_COPY_TO_ARRAY(vari_name, "y = ");
                }
            } else vari_name[0] = 0;
            if (m_Data->name_right[m_Number] != nullptr) STRING_APPEND_TO_ARRAY (vari_name, m_Data->name_right[m_Number]);
            if (m_Data->vars_disable_right[m_Number]) {
                value_string[0] = 0;
            } else {
                visable = read_value_desc_txt (value_string, sizeof(value_string), 1, m_Number, &color) == 0;
                Value = QString(value_string);
                if ((m_Data->dec_phys_right[m_Number])  || DisplayUnitForNonePhysicalValues) {
                    get_bbvari_unit (m_Data->vids_right[m_Number], unit, sizeof (unit));
                    Value.append(" ");
                    Value.append(QString().fromLatin1(unit));
                }
            }
        } else {
            vari_name[0] = '\0';
            value_string[0] = '\0';
            visable = 0;
        }
    }

    QString VariableName(vari_name);
    if (m_Side == Left) {
        sel_pos = m_Data->sel_pos_left;
    } else {
        sel_pos = m_Data->sel_pos_right;
    }
    font_height = 11;

    // calculate the size
    win_width = width ();
    win_height = height ();
    wh3 = win_height/3;
    wh8 = win_height/8;

    QPen PenSave = painter.pen();
    QPen Pen(Qt::darkGray);
    Pen.setWidth(2);
    painter.setPen(Pen);

    if (m_Side == Left) {
        if (border_flag) {
            // Paint the border
            if (!m_Data->xy_view_flag || ((m_Number & 0x1) == 0x0)) {
                if (m_Number == 0) painter.drawLine (win_width-1, 0, 0, 0);
                else painter.drawLine (win_width-wh3, 0, 0, 0);
            }
            painter.drawLine (0, 0, 0, win_height-1);
            if (!m_Data->xy_view_flag || ((m_Number & 0x1) == 0x1)) {
                if (m_Number == end_pos) {
                    painter.drawLine (0, win_height-1, win_width-1, win_height-1);
                    if (m_Data->xy_view_flag) {
                        if ((m_Number/2) != (sel_pos/2)) painter.drawLine (win_width-1, win_height-1, win_width-1, wh3);
                    } else {
                        if (m_Number != sel_pos) painter.drawLine (win_width-1, win_height-1, win_width-1, wh3);
                    }
                } else {
                    painter.drawLine (0, win_height-1, win_width-wh3, win_height-1);
                }
            }
            bool Before;
            bool Behind;
            if (m_Data->xy_view_flag) {
                Before = ((sel_pos/2) < (m_Number/2));
                Behind = ((sel_pos/2) > (m_Number/2));
            } else {
                Before = (sel_pos < m_Number);
                Behind = (sel_pos > m_Number);
            }
            if (Before) {
                if (!m_Data->xy_view_flag || ((m_Number & 0x1) == 0x0)) {
                    painter.drawLine (win_width-wh3, 0, win_width-1, wh3);  /* \ */
                }
                painter.drawLine (win_width-1, wh3, win_width-1, win_height-1);
                if (m_Data->xy_view_flag) {
                    if ((sel_pos/2) != ((m_Number-1)/2)) {
                        painter.drawLine(win_width-1, 0, win_width-1, wh3);
                    }
                } else {
                    if (sel_pos != m_Number-1) {
                        painter.drawLine(win_width-1, 0, win_width-1, wh3);
                    }
                }
            } else if (Behind) {
                if (m_Number == end_pos) {
                    painter.drawLine (win_width-1, 0, win_width-1, win_height-1);
                } else {
                    painter.drawLine (win_width-1, 0, win_width-1, win_height-wh3);
                    if (!m_Data->xy_view_flag || ((m_Number & 0x1) == 0x1)) {
                        painter.drawLine (win_width-1, win_height-wh3, win_width-wh3, win_height-1);  /* \ */
                    }
                }
                if (m_Data->xy_view_flag) {
                    if ((sel_pos/2) != ((m_Number+1)/2)) {
                        painter.drawLine (win_width-1, win_height, win_width-1, win_height-wh3);
                    }
                } else {
                    if (sel_pos != m_Number+1) {
                        painter.drawLine (win_width-1, win_height, win_width-1, win_height-wh3);
                    }
                }
            }
        }
        if (visable || !s_main_ini_val.SuppressDisplayNonExistValues) {
            if (m_Data->vars_disable_left[m_Number]) {
                // crossed out the variable
                QFontMetrics FontMetrics = painter.fontMetrics ();
                font_height = FontMetrics.height();
                string_width = FontMetrics.boundingRect(VariableName).width();
                string_width = (string_width * 13) / 10;
                if (m_Data->txt_align_left[m_Number]) {
                    painter.drawLine (win_width-wh3, wh3-font_height/2, win_width-wh3-string_width, wh3-font_height/2);
                } else {
                    painter.drawLine (wh3, wh3-font_height/2, string_width, wh3-font_height/2);
                }
            }
            // paint a solid line with the corresponding color
            if (visable) {
                QPen SavePen = painter.pen();
                QPen Pen;
                Pen.setWidth (m_Data->LineSize_left[m_Number]);
                int c = m_Data->color_left[m_Number];
                int r = c & 0xFF;
                int g = (c >> 8) & 0xFF;
                int b = (c >> 16) & 0xFF;
                QColor Color (r, g, b);
                Pen.setColor (Color);
                painter.setPen (Pen);
                painter.drawLine (wh3, wh3*2+wh3/2, wh3*3, wh3*2+wh3/2);
                painter.setPen (SavePen);
            }
            if (m_Number == m_Data->sel_pos_left) {
                QFont Font = painter.font();
                Font.setBold(true);
                painter.setFont(Font);
            }
            if (!border_flag || !s_main_ini_val.DarkMode) {
                painter.setPen (Qt::black);
            } else {
                painter.setPen (Qt::white);
            }
            // Print variable name and value
            if (m_Data->txt_align_left[m_Number]) {
                painter.drawText (wh8, wh8, win_width-wh8*2, font_height+wh8, Qt::AlignRight, VariableName);
            } else {
                painter.drawText (wh8, wh8, win_width-wh8*2, font_height+wh8, Qt::AlignLeft, VariableName);
            }
            if (m_Number == m_Data->sel_pos_left) {
                QFont Font = painter.font();
                Font.setBold(false);
                painter.setFont(Font);
            }
            painter.drawText (wh3, wh3 + font_height + font_height / 4 + wh8, Value);
        }
    } else { // (m_Side == Right)
        if (border_flag) {
            // Paint the border
            if (!m_Data->xy_view_flag || ((m_Number & 0x1) == 0x0)) {
                if (m_Number == 0) painter.drawLine (0, 0, win_width-1, 0);
                else painter.drawLine (wh3, 0, win_width-1, 0);
            }
            painter.drawLine (win_width-1, 0, win_width-1, win_height-1);
            if (!m_Data->xy_view_flag || ((m_Number & 0x1) == 0x1)) {
                if (m_Number == end_pos) {
                    painter.drawLine (win_width-1, win_height-1, 0, win_height-1);
                    if (m_Data->xy_view_flag) {
                        if ((m_Number/2) != (sel_pos/2)) painter.drawLine (0, win_height-1, 0, wh3);
                    } else {
                        if (m_Number != sel_pos) painter.drawLine (0, win_height-1, 0, wh3);
                    }
                } else painter.drawLine (win_width-1, win_height-1, wh3, win_height-1);
            }
            bool Before;
            bool Behind;
            if (m_Data->xy_view_flag) {
                Before = ((sel_pos/2) < (m_Number/2));
                Behind = ((sel_pos/2) > (m_Number/2));
            } else {
                Before = (sel_pos < m_Number);
                Behind = (sel_pos > m_Number);
            }
            if (Before) {
                if (!m_Data->xy_view_flag || ((m_Number & 0x1) == 0x0)) {
                    painter.drawLine (wh3, 0, 0, wh3);  /* / */
                }
                painter.drawLine (0, wh3, 0, win_height-1);
                if (m_Data->xy_view_flag) {
                    if ((sel_pos/2) != ((m_Number-1)/2)) {
                        painter.drawLine (0, 0, 0, wh3);
                    }
                } else {
                    if (sel_pos != m_Number-1) {
                        painter.drawLine (0, 0, 0, wh3);
                    }
                }
            } else if (Behind) {
                if (m_Number == end_pos) {
                    painter.drawLine (0, 0, 0, win_height-1);
                } else {
                    painter.drawLine (0, 0, 0, win_height-wh3);
                    if (!m_Data->xy_view_flag || ((m_Number & 0x1) == 0x1)) {
                        painter.drawLine (0, win_height-wh3, wh3, win_height-1);  /* \ */
                    }
                }
                if (m_Data->xy_view_flag) {
                    if ((sel_pos/2) != ((m_Number+1)/2)) {
                        painter.drawLine (0, win_height, 0, win_height-wh3);
                    }
                } else {
                    if (sel_pos != m_Number+1) {
                        painter.drawLine (0, win_height, 0, win_height-wh3);
                    }
                }
            }
        }
        if (visable || !s_main_ini_val.SuppressDisplayNonExistValues) {
            if (m_Data->vars_disable_right[m_Number]) {
                // crossed out the variable
                QFontMetrics FontMetrics = painter.fontMetrics ();
                font_height = FontMetrics.height();
                string_width = FontMetrics.boundingRect(VariableName).width();
                string_width = (string_width * 13) / 10;
                if (m_Data->txt_align_right[m_Number]) {
                    painter.drawLine (win_width-wh3, wh3-font_height/2, win_width-wh3-string_width, wh3-font_height/2);
                } else {
                    painter.drawLine (wh3, wh3-font_height/2, wh3+string_width, wh3-font_height/2);
                }
            }
            // paint a solid line with the corresponding color
            if (visable) {
                QPen SavePen = painter.pen();
                QPen Pen;
                Pen.setWidth (m_Data->LineSize_right[m_Number]);
                int c = m_Data->color_right[m_Number];
                int r = c & 0xFF;
                int g = (c >> 8) & 0xFF;
                int b = (c >> 16) & 0xFF;
                QColor Color (r, g, b);
                Pen.setColor (Color);
                painter.setPen (Pen);
                painter.drawLine (2*wh3, wh3*2+wh3/2, wh3*5, wh3*2+wh3/2);
                painter.setPen (SavePen);
            }
            if (m_Number == m_Data->sel_pos_right) {
                QFont Font = painter.font();
                Font.setBold(true);
                painter.setFont(Font);
            }
            if (!border_flag || !s_main_ini_val.DarkMode) {
                painter.setPen (Qt::black);
            } else {
                painter.setPen (Qt::white);
            }
            // Print variable name and value
            if (m_Data->txt_align_right[m_Number]) {
                painter.drawText (wh8, wh8, win_width-wh8*2, font_height+wh8, Qt::AlignRight, VariableName);
            } else {
                painter.drawText (wh8, wh8, win_width-wh8*2, font_height+wh8, Qt::AlignLeft, VariableName);
            }
            if (m_Number == m_Data->sel_pos_right) {
                QFont Font = painter.font();
                Font.setBold(false);
                painter.setFont(Font);
            }
            painter.drawText (wh3, wh3 + font_height + font_height / 4 + wh8, Value);
        }
    }
    painter.setPen(PenSave);
    LeaveOsciWidgetCriticalSection (m_Data->CriticalSectionNumber);
}


void OscilloscopeDesc::customContextMenu(QPoint arg_point)
{
    QMenu loc_contextMenu(tr("config signal"), this);
    QAction loc_openDialog(tr("config signal"), this);
    QAction loc_delete(tr("delete signal"), this);
    QAction loc_blackboardInfos("blackboard &variable info");
    connect(&loc_openDialog, SIGNAL(triggered()), this, SLOT(openDialog()));
    loc_contextMenu.addAction(&loc_openDialog);
    if (((m_Side == Left) && (m_Data->vids_left[m_Number] > 0)) ||
        ((m_Side == Right) && (m_Data->vids_right[m_Number] > 0))) {
        connect(&loc_delete, SIGNAL(triggered()), this, SLOT(deleteSignal()));
        loc_contextMenu.addAction(&loc_delete);
        connect(&loc_blackboardInfos, SIGNAL(triggered()), this, SLOT(BlackboardInfos()));
        loc_contextMenu.addAction(&loc_blackboardInfos);
    }
    loc_contextMenu.exec(mapToGlobal(arg_point));
}

void OscilloscopeDesc::mouseReleaseEvent(QMouseEvent * event)
{
    Q_UNUSED(event);
}

void OscilloscopeDesc::mouseDoubleClickEvent(QMouseEvent *arg_event)
{
    Q_UNUSED(arg_event);
    openDialog();
}

void OscilloscopeDesc::BlackboardInfos()
{
    QString Name;
    if (m_Side == Left) {
        if (m_Data->vids_left[m_Number] > 0) {
            Name = QString(m_Data->name_left[m_Number]);
        }
    } else if (m_Side == Right) {
        if (m_Data->vids_right[m_Number] > 0) {
            Name = QString(m_Data->name_right[m_Number]);
        }
    }
    if (!Name.isEmpty()) {
        BlackboardInfoDialog Dlg;
        Dlg.editCurrentVariable(Name);

        if (Dlg.exec() == QDialog::Accepted) {
            ;
        }
    }
}

