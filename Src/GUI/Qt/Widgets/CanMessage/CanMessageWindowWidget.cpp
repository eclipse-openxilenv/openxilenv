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


#include "CanMessageWindowWidget.h"
#include "CanMessageWindowConfigDialog.h"

#include "StringHelpers.h"
#include "QtIniFile.h"

extern "C" {
#include "EquationParser.h"
#include "Files.h"
#include "MyMemory.h"
#include "ThrowError.h"
#include "IniDataBase.h"
#include "CanFifo.h"
#include "MainValues.h"
#include "RemoteMasterControlProcess.h"
}

CANMessageWindowWidget::CANMessageWindowWidget (QString par_WindowTitle, MdiSubWindow *par_SubWindow, MdiWindowType *par_Type,  QWidget *parent)
    : MdiWindowWidget(par_WindowTitle, par_SubWindow, par_Type, parent)
{
    m_OnOff = 0;
    m_cam_count = 0;
    m_DisplayFormat = DISPLAY_HEX;
    m_Record2Disk = 0;
    m_fh = nullptr;
    m_TriggerActiv = 0;
    m_Triggered = 0;
    m_MaxCanMsgRec = 0;

    m_RefreshCounter = 0;
    m_AbtastPeriodeInMs = 0.0;
    m_DisplayColumnCounterFlag = 0;
    m_DisplayColumnTimeAbsoluteFlag = 0;
    m_DisplayColumnTimeDiffFlag = 0;
    m_DisplayColumnTimeDiffMinMaxFlag = 0;

    m_DisplayColumnCounterWidth = 0;
    m_DisplayColumnTimeAbsoluteWidth = 0;
    m_DisplayColumnTimeDiffWidth = 0;
    m_DisplayColumnTimeDiffMinMaxWidth = 0;
    m_DisplayColumnDirWidth = 0;
    m_DisplayColumnChannelWidth = 0;
    m_DisplayColumnIdWidth = 0;
    m_DisplayColumnSizeWidth = 0;
    m_DisplayColumnDataWidth = 0;

    m_WaitForFirstMessageFlag = 0.0;
    m_TsOffset = 0.0;

    if (s_main_ini_val.ConnectToRemoteMaster) {
        m_CanFiFoHandle = CreateCanFifos (10000, 0x1);
    } else {
        m_CanFiFoHandle = CreateCanFifos (100000, ((uint32_t)-1 << 16) | 0x1); // do not lose any message ((uint32_t)-1 << 16)
    }
    readFromIni ();
    m_AbtastPeriodeInMs = GetSchedulingPeriode () * 1000.0;
    SetAcceptMask4CanFifo (m_CanFiFoHandle, m_cam, m_cam_count);
    FlushMessageQueue ();

    m_TreeView = new CANMessageTreeView (this, this);

    m_Model = new CANMessageWindowModel (m_AbtastPeriodeInMs,
                                         m_TreeView);
    m_TreeView->setModel(m_Model);

    SwitchColumnsOnOff();
    SetAllColumnsWidth();
    FitNewColumnsToHeader();

}

CANMessageWindowWidget::~CANMessageWindowWidget()
{
    if (m_fh != nullptr) {
        ReadAndProcessCANMessages();
        close_file(m_fh);
        m_fh = nullptr;
    }
    writeToIni();
    DeleteCanFifos (m_CanFiFoHandle);
}

void CANMessageWindowWidget::FlushMessageQueue ()
{
    CAN_FIFO_ELEM CANMessage[64];
    while (ReadCanMessagesFromFifo2Process (m_CanFiFoHandle, CANMessage, 64) >= 64);
}

bool CANMessageWindowWidget::writeToIni()
{
    int x;
    char txt[256];
    char entry[32];
    int Fd = GetMainFileDescriptor();

    QString SectiopnPath = GetIniSectionPath();

    for (x = 0; x < (MAX_CAN_ACCEPTANCE_MASK_ELEMENTS - 1); x++) {
        sprintf (entry, "cam%i", x);
        if (x < (m_cam_count-1)) {
            sprintf (txt, "%i:0x%X,0x%X", m_cam[x].Channel,
                     m_cam[x].Start, m_cam[x].Stop);
            ScQt_IniFileDataBaseWriteString (SectiopnPath, entry, txt, Fd);
        } else ScQt_IniFileDataBaseWriteString (SectiopnPath, entry, nullptr, Fd);
    }
    sprintf (txt, "%i", m_Record2Disk);
    ScQt_IniFileDataBaseWriteString(SectiopnPath, "Record2Disk", txt, Fd);
    sprintf (txt, "%i", m_TriggerActiv);
    ScQt_IniFileDataBaseWriteString(SectiopnPath, "TriggerActiv", txt, Fd);
    ScQt_IniFileDataBaseWriteString(SectiopnPath, "FileName", QStringToConstChar(m_RecorderFileName), Fd);
    ScQt_IniFileDataBaseWriteString(SectiopnPath, "TriggerEquation", QStringToConstChar(m_TriggerEquation), Fd);

    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnCounter", m_DisplayColumnCounterFlag, Fd);
    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnTimeAbsolute", m_DisplayColumnTimeAbsoluteFlag, Fd);
    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnTimeDiff", m_DisplayColumnTimeDiffFlag, Fd);
    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnTimeDiffMinMax", m_DisplayColumnTimeDiffMinMaxFlag, Fd);

    GetAllColumnsWidth();
    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnCounterWidth", m_DisplayColumnCounterWidth, Fd);
    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnTimeAbsoluteWidth", m_DisplayColumnTimeAbsoluteWidth, Fd);
    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnTimeDiffWidth", m_DisplayColumnTimeDiffWidth, Fd);
    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnTimeDiffMinMaxWidth", m_DisplayColumnTimeDiffMinMaxWidth, Fd);
    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnDirWidth", m_DisplayColumnDirWidth, Fd);
    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnChannelWidth", m_DisplayColumnChannelWidth, Fd);
    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnIdentifierWidth", m_DisplayColumnIdWidth, Fd);
    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnSizeWidth", m_DisplayColumnSizeWidth, Fd);
    ScQt_IniFileDataBaseWriteInt (SectiopnPath, "DisplayColumnDataWidth", m_DisplayColumnDataWidth, Fd);

    return true;
}

bool CANMessageWindowWidget::readFromIni()
{
    int x;
    char txt[256];
    char entry[32];
    char Line[INI_MAX_LINE_LENGTH];
    char *p, *endptr;
    int32_t channel;
    uint32_t start, stop;
    int Fd = ScQt_GetMainFileDescriptor();

    QString SectiopnPath = GetIniSectionPath();

    for (x = 0; x < (MAX_CAN_ACCEPTANCE_MASK_ELEMENTS - 1); x++) {
        sprintf (entry, "cam%i", x);
        if (!ScQt_IniFileDataBaseReadString(SectiopnPath, entry, "", txt, sizeof (txt), Fd)) {
            break;
        }
        p = txt;
        // channel:   Start-ID  /  Stop-ID
        channel = strtol (p, &endptr, 10);
        if ((endptr == nullptr) || *endptr != ':') break;
        p = endptr + 1;
        start = strtoul (p, &endptr, 0);
        if ((endptr == nullptr) || *endptr != ',') break;
        p = endptr + 1;
        stop = strtoul (p, &endptr, 0);
        m_cam[x].Channel = channel;
        m_cam[x].Start = start;
        m_cam[x].Stop = stop;
    }
    m_cam[x].Channel = -1;
    m_cam[x].Start = 0;
    m_cam[x].Stop = 0;
    m_cam_count = x+1;
    m_Record2Disk = ScQt_IniFileDataBaseReadInt(SectiopnPath, "Record2Disk", 0, Fd);
    m_TriggerActiv = ScQt_IniFileDataBaseReadInt(SectiopnPath, "TriggerActiv", 0, Fd);
    ScQt_IniFileDataBaseReadString(SectiopnPath, "FileName", "", Line, sizeof (Line), Fd);
    m_RecorderFileName = CharToQString(Line);
    ScQt_IniFileDataBaseReadString(SectiopnPath, "TriggerEquation", "", Line, sizeof (Line), Fd);
    m_TriggerEquation = CharToQString(Line);

    m_DisplayColumnCounterFlag = ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnCounter", 1, Fd);
    m_DisplayColumnTimeAbsoluteFlag = ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnTimeAbsolute", 0, Fd);
    m_DisplayColumnTimeDiffFlag =  ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnTimeDiff", 0, Fd);
    m_DisplayColumnTimeDiffMinMaxFlag =  ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnTimeDiffMinMax", 0, Fd);

    m_DisplayColumnCounterWidth = ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnCounterWidth", 0, Fd);
    m_DisplayColumnTimeAbsoluteWidth = ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnTimeAbsoluteWidth", 0, Fd);
    m_DisplayColumnTimeDiffWidth = ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnTimeDiffWidth", 0, Fd);
    m_DisplayColumnTimeDiffMinMaxWidth = ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnTimeDiffMinMaxWidth", 0, Fd);
    m_DisplayColumnDirWidth = ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnDirWidth", 0, Fd);
    m_DisplayColumnChannelWidth = ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnChannelWidth", 0, Fd);
    m_DisplayColumnIdWidth = ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnIdentifierWidth", 0, Fd);
    m_DisplayColumnSizeWidth = ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnSizeWidth", 0, Fd);
    m_DisplayColumnDataWidth = ScQt_IniFileDataBaseReadInt (SectiopnPath, "DisplayColumnDataWidth", 0, Fd);

    return true;
}

// return value are milliseconds
double CANMessageWindowWidget::TimestampCalc (CAN_FD_FIFO_ELEM *pCanMessage)
{
    double Ts;

    if (pCanMessage->node) {  // from oneself
        // transmit messages: the timestampare are the cycle counter
        Ts = static_cast<double>(pCanMessage->timestamp) * m_AbtastPeriodeInMs;
    } else {
        if (s_main_ini_val.ConnectToRemoteMaster) {
            uint32_t Cycle;
            uint16_t T4Master, T4;
            switch (GetCanCardType (pCanMessage->channel)) {
            default:
                Ts = static_cast<double>(pCanMessage->timestamp) * 0.000001;
                break;
            case 1:   // I_PCI-165
            case 2:   // I_PCI-XC16
                Cycle = static_cast<uint32_t>(pCanMessage->timestamp >> 32);
                T4Master = static_cast<uint16_t>(static_cast<uint32_t>(pCanMessage->timestamp) >> 16);
                T4 = static_cast<uint16_t>(pCanMessage->timestamp);
                Ts = static_cast<double>(Cycle) * m_AbtastPeriodeInMs + static_cast<double>(static_cast<int16_t>(T4 - T4Master)) / 1250.0;
                break;
            case 6: // Flexcard
                 // High DWord ist der UCx, LowDWord ist der FRCx
                Ts = static_cast<double>((pCanMessage->timestamp << 32) | (pCanMessage->timestamp >> 32)) / 80000.0;
                break;
            case 8: // SocketCAN
                Ts = static_cast<double>(pCanMessage->timestamp) / 1000.0;
                break;
            }
        } else {
            // receiving CAN messages are 1ns resolution
            Ts = static_cast<double>(pCanMessage->timestamp) * 0.000001;
        }
    }
    return Ts;

}

void CANMessageWindowWidget::GetAllColumnsWidth()
{
    m_DisplayColumnCounterWidth = m_TreeView->columnWidth(0);
    m_DisplayColumnTimeAbsoluteWidth = m_TreeView->columnWidth(1);
    m_DisplayColumnTimeDiffWidth = m_TreeView->columnWidth(2);
    m_DisplayColumnTimeDiffMinMaxWidth = m_TreeView->columnWidth(3);
    m_DisplayColumnDirWidth = m_TreeView->columnWidth (4);
    m_DisplayColumnChannelWidth = m_TreeView->columnWidth (5);
    m_DisplayColumnIdWidth = m_TreeView->columnWidth (6);
    m_DisplayColumnSizeWidth = m_TreeView->columnWidth (7);
    m_DisplayColumnDataWidth = m_TreeView->columnWidth (8);
}

void CANMessageWindowWidget::SetAllColumnsWidth()
{
    m_TreeView->setColumnWidth (0, m_DisplayColumnCounterWidth);
    m_TreeView->setColumnWidth (1, m_DisplayColumnTimeAbsoluteWidth);
    m_TreeView->setColumnWidth (2, m_DisplayColumnTimeDiffWidth);
    m_TreeView->setColumnWidth (3, m_DisplayColumnTimeDiffMinMaxWidth);
    m_TreeView->setColumnWidth (4, m_DisplayColumnDirWidth);
    m_TreeView->setColumnWidth (5, m_DisplayColumnChannelWidth);
    m_TreeView->setColumnWidth (6, m_DisplayColumnIdWidth);
    m_TreeView->setColumnWidth (7, m_DisplayColumnSizeWidth);
    m_TreeView->setColumnWidth (8, m_DisplayColumnDataWidth);
    FitNewColumnsToHeader();
}

void CANMessageWindowWidget::FitNewColumnsToHeader()
{
    if (m_DisplayColumnCounterWidth <= 0) m_TreeView->resizeColumnToContents(0);
    if (m_DisplayColumnTimeAbsoluteWidth <= 0) m_TreeView->resizeColumnToContents(1);
    if (m_DisplayColumnTimeDiffWidth <= 0) m_TreeView->resizeColumnToContents(2);
    if (m_DisplayColumnTimeDiffMinMaxWidth <= 0) m_TreeView->resizeColumnToContents(3);
    if (m_DisplayColumnDirWidth <= 0) m_TreeView->resizeColumnToContents(4);
    if (m_DisplayColumnChannelWidth <= 0) m_TreeView->resizeColumnToContents(5);
    if (m_DisplayColumnIdWidth <= 0) m_TreeView->resizeColumnToContents(6);
    if (m_DisplayColumnSizeWidth <= 0) m_TreeView->resizeColumnToContents(7);
    if (m_DisplayColumnDataWidth <= 0) m_TreeView->resizeColumnToContents(8);
}


void CANMessageWindowWidget::SwitchColumnsOnOff()
{
    m_TreeView->setColumnHidden(0, !m_DisplayColumnCounterFlag);
    m_TreeView->setColumnHidden(1, !m_DisplayColumnTimeAbsoluteFlag);
    m_TreeView->setColumnHidden(2, !m_DisplayColumnTimeDiffFlag);
    m_TreeView->setColumnHidden(3, !m_DisplayColumnTimeDiffMinMaxFlag);
}

void CANMessageWindowWidget::changeColor(QColor arg_color)
{
    Q_UNUSED(arg_color)
    // NOTE: required by inheritance but not used
}

void CANMessageWindowWidget::changeFont(QFont arg_font)
{
    Q_UNUSED(arg_font)
    // NOTE: required by inheritance but not used
}

void CANMessageWindowWidget::changeWindowName(QString arg_name)
{
    Q_UNUSED(arg_name)
    // NOTE: required by inheritance but not used
}

void CANMessageWindowWidget::changeVariable(QString arg_variable, bool arg_visible)
{
    Q_UNUSED(arg_variable)
    Q_UNUSED(arg_visible)
    // NOTE: required by inheritance but not used
}

void CANMessageWindowWidget::changeVaraibles(QStringList arg_variables, bool arg_visible)
{
    Q_UNUSED(arg_variables)
    Q_UNUSED(arg_visible)
    // NOTE: required by inheritance but not used
}

void CANMessageWindowWidget::resetDefaultVariables(QStringList arg_variables)
{
    Q_UNUSED(arg_variables)
    // NOTE: required by inheritance but not used
}

void CANMessageWindowWidget::BuildAbsoluteTimeString (double Ts, char *Txt)
{
    uint32_t Hour;
    uint32_t Minute;
    double T = Ts + m_TsOffset;

    Hour = static_cast<uint32_t>(T / 3600000.0);  // 1 hour has 3600s
    T -= static_cast<double>(Hour) * 3600000.0;
    Minute = static_cast<uint32_t>(T / 60000.0);  // 1 minute has 60s
    T -= static_cast<double>(Minute) * 60000.0;
    T *= 0.001;
    sprintf (Txt, "%02i:%02i:%09.6f", Hour, Minute, T);
}

int CANMessageWindowWidget::ReadAndProcessCANMessages (void)
{
    CAN_FD_FIFO_ELEM *pCANMessage, CANMessages[64];
    int i, x;
    int ReadElements;
    double Ts;
    char Txt[64];

    do {
        ReadElements = ReadCanFdMessagesFromFifo2Process (m_CanFiFoHandle, CANMessages, 64);   // read max 64 CAN object
        if (m_Record2Disk) {
            if (m_TriggerActiv) {
                if (!m_Triggered) {
                    m_Triggered = static_cast<int>(direct_solve_equation_no_error_message (QStringToConstChar(m_TriggerEquation))) != 0;
                }
            // it there is no trigger active start immediately with the recording
            } else m_Triggered = 1;

            if (m_Triggered) {
                if (m_fh == nullptr) {
                    if ((m_fh = open_file (QStringToConstChar(m_RecorderFileName), "wt")) == nullptr) {
                        m_Record2Disk = 0;
                        ThrowError (1, "cannot open file %s", QStringToConstChar(m_RecorderFileName));
                    } else {
                        m_WaitForFirstMessageFlag = 1;
                        fprintf (m_fh, "Time\t");
                        fprintf (m_fh, "Dir\t");
                        fprintf (m_fh, "C\t");
                        fprintf (m_fh, "Id\t");
                        fprintf (m_fh, "Size\t");
                        fprintf (m_fh, "Data[0...7/63]\n");
                        goto __WRITE_A_LINE;
                    }
                } else {
                  __WRITE_A_LINE:
                    for (x = 0; x < ReadElements; x++) {
                        pCANMessage = &CANMessages[x];
                        Ts = TimestampCalc (pCANMessage);
                        if (m_WaitForFirstMessageFlag) {
#ifdef _WIN32
                            GetLocalTime (&(m_SystemTimeStartCANLogging));
                            m_WaitForFirstMessageFlag = 0;

                            m_TsOffset = static_cast<double>(m_SystemTimeStartCANLogging.wHour) * 3600000.0 +
                                         static_cast<double>(m_SystemTimeStartCANLogging.wMinute) * 60000.0 +
                                         static_cast<double>(m_SystemTimeStartCANLogging.wSecond) * 1000.0 +
                                         static_cast<double>(m_SystemTimeStartCANLogging.wMilliseconds);
#else
                            clock_gettime(CLOCK_MONOTONIC, &m_SystemTimeStartCANLogging);
                            m_TsOffset = m_SystemTimeStartCANLogging.tv_sec * 1000.0 + m_SystemTimeStartCANLogging.tv_nsec / 1000000.0;
#endif
                            m_TsOffset -= Ts;
                        }
                        BuildAbsoluteTimeString (Ts, Txt);
                        fprintf (m_fh, "%s\t", Txt);
                        fprintf (m_fh, "%s\t", (pCANMessage->node) ? "->" : "<-");
                        fprintf (m_fh, "%i\t", static_cast<int>(pCANMessage->channel));
                        fprintf (m_fh, "0x%X", static_cast<int>(pCANMessage->id));
                        switch (pCANMessage->ext) {
                        case 0:
                            fprintf (m_fh, "n\t");
                            break;
                        case 1:
                            fprintf (m_fh, "e\t");
                            break;
                        case 2:
                            fprintf (m_fh, "bn\t");
                            break;
                        case 3:
                            fprintf (m_fh, "be\t");
                            break;
                        case 4:
                            fprintf (m_fh, "fn\t");
                            break;
                        case 5:
                            fprintf (m_fh, "fe\t");
                            break;
                        case 6:
                            fprintf (m_fh, "fbn\t");
                            break;
                        case 7:
                            fprintf (m_fh, "fbe\t");
                            break;
                        }

                        fprintf (m_fh, "%i\t", static_cast<int>(pCANMessage->size));

                        for (i = 0; (i < pCANMessage->size) && (i < 64); i++) {
                            fprintf (m_fh, "0x%02X ", static_cast<int>(pCANMessage->data[i]));
                        }
                        fprintf (m_fh, "\n");
                    }
                }
            }
        } else if (m_fh != nullptr) {
            close_file (m_fh);
            m_fh = nullptr;
        }

        m_Model->AddNewCANMessages (ReadElements, CANMessages);

    } while (ReadElements == 64);   // repeat till fifo is empty
    return 0;
}


void CANMessageWindowWidget::CyclicUpdate()
{
    ReadAndProcessCANMessages();
    m_TreeView->reset();
    if ((m_fh != nullptr) && (m_RefreshCounter)) {
        m_RefreshCounter = 0;
        QString Title = GetWindowTitle().append(QString (" (recording)"));
        this->setWindowTitle(Title);
    } else {
        m_RefreshCounter = 1;
        QString Title = GetWindowTitle();
        this->setWindowTitle(Title);
    }
}

void CANMessageWindowWidget::ConfigDialogSlot()
{
    CANMessageWindowConfigDialog Dlg (GetWindowTitle(),
                                      m_cam_count,
                                      m_cam,
                                      m_DisplayColumnCounterFlag,
                                      m_DisplayColumnTimeAbsoluteFlag,
                                      m_DisplayColumnTimeDiffFlag,
                                      m_DisplayColumnTimeDiffMinMaxFlag,
                                      m_Record2Disk,
                                      m_RecorderFileName,
                                      m_TriggerActiv,
                                      m_TriggerEquation);
    if (Dlg.exec() == QDialog::Accepted) {
        if (Dlg.WindowTitleChanged()) {
            RenameWindowTo (Dlg.GetWindowTitle());
        }
        m_cam_count = Dlg.GetAcceptanceMask(m_cam);

        Dlg.GetDisplayFlags (&m_DisplayColumnCounterFlag,
                             &m_DisplayColumnTimeAbsoluteFlag,
                             &m_DisplayColumnTimeDiffFlag,
                             &m_DisplayColumnTimeDiffMinMaxFlag);
        m_RecorderFileName = Dlg.GetRecorderFilename (&m_Record2Disk);
        m_TriggerEquation = Dlg.GetTrigger (&m_TriggerActiv);

        m_Model->Clear();

        m_TreeView->reset();

        SwitchColumnsOnOff();

        SetAcceptMask4CanFifo (m_CanFiFoHandle, m_cam, m_cam_count);
        FlushMessageQueue ();
    }
}

void CANMessageWindowWidget::ClearSlot()
{
    m_Model->Clear();
    m_TreeView->reset();
    FlushMessageQueue ();
}


void CANMessageWindowWidget::resizeEvent(QResizeEvent * /* event */)
{
    m_TreeView->resize (width(), height());
}

CustomDialogFrame* CANMessageWindowWidget::dialogSettings(QWidget* arg_parent)
{
    Q_UNUSED(arg_parent);
    // NOTE: required by inheritance but not used
    return nullptr;
}
