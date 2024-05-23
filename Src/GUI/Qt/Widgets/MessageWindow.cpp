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


#include "MessageWindow.h"
#include "ui_MessageWindow.h"
#include "QtIniFile.h"

extern "C" {
#include "MyMemory.h"
#include "MainValues.h"
}

static bool IsMessageWindowOpen;
static bool IsMessageWindowVisable;

static int64_t MessageToPrintCounter;
static int64_t MessagePrintedCounter;
static int WaitForEventFlag;

static CRITICAL_SECTION MessageIsPrintedCriticalSection;
static CONDITION_VARIABLE MessageIsPrintedConditionVariable;


MessageWindow::MessageWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MessageWindow)
{
    ui->setupUi(this);
    m_StayOnTop = ScQt_IniFileDataBaseReadYesNo ("BasicSettings", "MessageWindowStayOnTop", 0, ScQt_GetMainFileDescriptor());
    ui->StayOnTopCheckBox->setChecked(m_StayOnTop);
    Qt::WindowFlags flags = this->windowFlags();
    if (m_StayOnTop) {
        this->setWindowFlags(flags | Qt::WindowStaysOnTopHint);
    } else {
        this->setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);
    }

}

MessageWindow::~MessageWindow()
{
    delete ui;
    WriteToIni();
    IsMessageWindowOpen = false;
}

void MessageWindow::SetMaxHistoryLines(int par_MaxLines)
{
    ui->MessagePlainTextEdit->setMaximumBlockCount (par_MaxLines);
}

void MessageWindow::WriteToIni()
{
    int Fd = ScQt_GetMainFileDescriptor();
    ScQt_IniFileDataBaseWriteInt ("BasicSettings", "MessageWindowXPosition", this->x(), Fd);
    ScQt_IniFileDataBaseWriteInt ("BasicSettings", "MessageWindowYPosition", this->y(), Fd);
    ScQt_IniFileDataBaseWriteInt ("BasicSettings", "MessageWindowXSize", this->width(), Fd);
    ScQt_IniFileDataBaseWriteInt ("BasicSettings", "MessageWindowYSize", this->height(), Fd);
    ScQt_IniFileDataBaseWriteYesNo ("BasicSettings", "MessageWindowStayOnTop", this->m_StayOnTop, Fd);
}

void MessageWindow::AddMessage(QString par_MessageString)
{
    ui->MessagePlainTextEdit->appendPlainText(par_MessageString);
}

void MessageWindow::on_SelectAllpushButton_clicked()
{
    ui->MessagePlainTextEdit->selectAll();
}

void MessageWindow::on_CopyPushButton_clicked()
{
    ui->MessagePlainTextEdit->copy();
}

void MessageWindow::on_ClosePushButton_clicked()
{
    this->close();
}

void MessageWindow::closeEvent(QCloseEvent *event)
{
    WriteToIni();
    ui->MessagePlainTextEdit->clear();
    IsMessageWindowVisable = false;
    this->setVisible(false);
    emit SetMessageWindowStateSignal(false);  // Reset the checkbox inside the ControlPanel
    event->ignore();                          // The window will not close it will only hide
}


// Use signal/slot for synchronizing from other threads
static MessagesFromOtherThread MessagesFromOtherThreadInst;
static MessagesToGuiThread MessagesToGuiThreadInst;

void MessagesFromOtherThread::AddMessages(QString par_Messsage)
{
    emit AddMessagesFromOtherThreadSignal(par_Messsage);
}

void MessagesFromOtherThread::AddMessagesChar(const char *par_Message)
{
    EnterCriticalSection (&MessageIsPrintedCriticalSection);
    MessageToPrintCounter++;
    LeaveCriticalSection (&MessageIsPrintedCriticalSection);
    emit AddMessagesFromOtherThreadSignal(QString().fromLatin1(par_Message));
    EnterCriticalSection (&MessageIsPrintedCriticalSection);
    int64_t Diff = (MessageToPrintCounter - MessagePrintedCounter);
    // If more than 100 messages not displayed wait till rhey can displayed
    if (Diff > 100) {
        // Wait max. 100ms
        WaitForEventFlag = 1;
        SleepConditionVariableCS (&MessageIsPrintedConditionVariable, &MessageIsPrintedCriticalSection, 100);
        WaitForEventFlag = 0;
    }
    LeaveCriticalSection (&MessageIsPrintedCriticalSection);
}

void AddMessages (QString par_Messsage)
{
    MessagesFromOtherThreadInst.AddMessages (par_Messsage);
}

void AddMessagesChar (const char *par_Messsage)
{
    MessagesFromOtherThreadInst.AddMessagesChar (par_Messsage);
}


void MessagesToGuiThread::Init(ControlPanel *par_ControlPanel, QWidget *par_Parent)
{
    m_Parent = par_Parent;
    m_ControlPanel = par_ControlPanel;
    connect (&MessagesFromOtherThreadInst, SIGNAL(AddMessagesFromOtherThreadSignal(QString)), this, SLOT(AddMessagesToGuiThreadSlot(QString)));
}

void MessagesToGuiThread::Terminate()
{
    if (m_MessageWindow != nullptr) {
        if (IsMessageWindowVisable) {
            m_MessageWindow->close();
        }
    }
}

void MessagesToGuiThread::AddMessagesToGuiThreadSlot(QString par_Message)
{
    // Should script messages should be displayed
    if (s_main_ini_val.ScriptMessageDialogVisable) {
        if (IsMessageWindowOpen == false) {
            IsMessageWindowOpen = true;
            // Open message window if it is not open
            m_MessageWindow = new MessageWindow(m_Parent);
            int Fd = ScQt_GetMainFileDescriptor();
            m_MessageWindow->SetMaxHistoryLines (ScQt_IniFileDataBaseReadInt ("BasicSettings", "MessageWindowMaxLines", 1000, Fd));
            int xpos = ScQt_IniFileDataBaseReadInt ("BasicSettings", "MessageWindowXPosition", -1, Fd);
            int ypos = ScQt_IniFileDataBaseReadInt ("BasicSettings", "MessageWindowYPosition", -1, Fd);
            // Are the position valid
            if (xpos < 100) xpos = 100;
            if (ypos < 0) ypos = 0;

            // Don't set the window outside the desktop
#ifdef _WIN32
            RECT rc;

            GetWindowRect (GetDesktopWindow (), &rc);
            int max_xpos = rc.right - 100;
            int max_ypos = rc.bottom - 100;
            if (xpos > max_xpos) xpos = max_xpos;
            if (ypos > max_ypos) ypos = max_ypos;
            m_MessageWindow->move (xpos, ypos);
#else
#endif
            bool Ret = connect(m_MessageWindow, SIGNAL(SetMessageWindowStateSignal(bool)), m_ControlPanel, SLOT(SetMessageWindowStateSlot(bool)));
            printf ("%i", Ret);
        }
        if (!IsMessageWindowVisable) {
            IsMessageWindowVisable = true;
            m_MessageWindow->show();
        }
        if (m_MessageWindow != nullptr) {
            m_MessageWindow->AddMessage(par_Message);
        }
    }
    EnterCriticalSection (&MessageIsPrintedCriticalSection);
    MessagePrintedCounter++;
    if (WaitForEventFlag) {
        int64_t Diff = (MessageToPrintCounter - MessagePrintedCounter);
        if (Diff <= 0) {
            LeaveCriticalSection (&MessageIsPrintedCriticalSection);
            WakeAllConditionVariable (&MessageIsPrintedConditionVariable);
            EnterCriticalSection (&MessageIsPrintedCriticalSection);
        }
    }
    LeaveCriticalSection (&MessageIsPrintedCriticalSection);
}

void InitMessageWindow(ControlPanel *par_ControlPanel, QWidget *parent)
{
    MessagesToGuiThreadInst.Init(par_ControlPanel, parent);
}

void CloseMessageWindow()
{
    MessagesToGuiThreadInst.Terminate();
}

extern "C" {
void AddMessages (char *par_Messsage)
{
    MessagesFromOtherThreadInst.AddMessagesChar (par_Messsage);
}

void InitMessageWindowCriticalSections (void)
{
    InitializeCriticalSection (&MessageIsPrintedCriticalSection);
    InitializeConditionVariable (&MessageIsPrintedConditionVariable);
}
}

void MessageWindow::on_StayOnTopCheckBox_clicked(bool checked)
{
    Qt::WindowFlags flags = this->windowFlags();
    if (checked) {
        this->setWindowFlags(flags | Qt::WindowStaysOnTopHint);
        m_StayOnTop = true;
    } else {
        this->setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);
        m_StayOnTop = false;
    }
    this->show();
}
