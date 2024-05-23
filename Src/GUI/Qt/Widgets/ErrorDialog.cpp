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


#include "ErrorDialog.h"

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include "Platform.h"
#include <QMessageBox>
#include <QStatusBar>
#include <QPushButton>
#include "MainWindow.h"

#define NO_ERROR_FUNCTION_DECLARATION

extern "C" {
#include "ThrowError.h"
#include "MyMemory.h"
#include "ConfigurablePrefix.h"
#include "Scheduler.h"
#include "ScriptMessageFile.h"
}

typedef struct {
    int InUseFlag;
    int Level;
    int ThreadId;
    CRITICAL_SECTION CriticalSection;
    CONDITION_VARIABLE ConditionVariable;
    const char *HeaderString;
    const char *ErrorString;
    int Pid;
    uint64_t Cycle;
    int Waiting;
    int ReturnValue;
} ERROR_EVENT;

static int TranslateReturnCode (int par_Ret)
{
    switch (par_Ret) {
    default:
    case QMessageBox::Ok:     // 0x00000400
        return IDOK;
    case QMessageBox::Cancel: // 0x00400000
        return IDCANCEL;
    case QMessageBox::YesToAll: // 0x00008000
        return IDOKALL;
    case QMessageBox::NoToAll:  // 0x00020000
        return IDCANCELALL;
    case QMessageBox::Save:     // 0x00000800
        return IDSAVE;
    case QMessageBox::Ignore:  // 0x00100000
        return IDIGNORE;
    }
    /*
    Save               = 0x00000800,
    SaveAll            = 0x00001000,
    Open               = 0x00002000,
    Yes                = 0x00004000,
    No                 = 0x00010000,
    Abort              = 0x00040000,
    Retry              = 0x00080000,
    Ignore             = 0x00100000,
    Close              = 0x00200000,
    Discard            = 0x00800000,
    Help               = 0x01000000,
    Apply              = 0x02000000,
    Reset              = 0x04000000,
    */
}

static ERROR_EVENT ErrorEvents[MAX_SIMULTANEOUSLY_ERROR_EVENTS];
static CRITICAL_SECTION ErrorEventsCriticalSection;

static CRITICAL_SECTION LockErrorPopUpMessageCriticalSection;
static CONDITION_VARIABLE LockErrorPopUpMessageConditionVariable;

static DWORD MainThreadId;


static ErrorMessagesFromOtherThread ErrorMessagesFromOtherThreadInst;
static ErrorMessagesGuiThread ErrorMessagesGuiThreadInst;

static int SyncWithGuiThreadIsInitAndRunning;

void SetSyncWithGuiThreadIsInitAndRunning(void)
{
    SyncWithGuiThreadIsInitAndRunning = 1;
}

#define CALL_MAIN_WINDOW(f) \
    if (MainWindow::GetPtrToMainWindow() != NULL) MainWindow::GetPtrToMainWindow()->f();

static void Error2MessageOut(int par_Level, const char *par_HeaderString, const char *par_ErrorString, ErrorMessagesGuiThread *par_ErrorMessagesGuiThread)
{
    FILE *fh;
    char Path[2*MAX_PATH + 16];
    sprintf (Path, "%s\\%s\\%s.err", s_main_ini_val.StartDirectory, s_main_ini_val.ScriptOutputFilenamesPrefix,
             GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_ERROR_FILE));
    if ((fh = open_file (Path, "a")) != nullptr) {
        fprintf (fh, "%s\n  %s\n", par_HeaderString, par_ErrorString);
        close_file (fh);
    }
    AddScriptMessage (par_HeaderString);
    AddScriptMessage (par_ErrorString);
    if (par_ErrorMessagesGuiThread != nullptr) {
        par_ErrorMessagesGuiThread->AddErrorToStatusBar(par_Level, par_HeaderString, par_ErrorString);
    }
}

static int ErrorMessageInnerFunction(int par_Level,
                                     const char *par_HeaderString,
                                     const char *par_ErrorString,
                                     int par_Pid,
                                     uint64_t par_Cycle,
                                     ErrorMessagesGuiThread *par_ErrorMessagesGuiThread)
{
    int ReturnValue;

    switch(par_Level) {
    default:
    case MESSAGE_STOP:   // all with an OK button
    case ERROR_NO_CRITICAL_STOP:
    case ERROR_CRITICAL:
    case ERROR_SYSTEM_CRITICAL:
    {
        if (GetError2MessageFlag()) {
            Error2MessageOut(par_Level, par_HeaderString, par_ErrorString, par_ErrorMessagesGuiThread);
            ReturnValue = QMessageBox::Ok;   // If error should be redirect confirm all wit OK
        } else {
            QMessageBox *msgBox = new QMessageBox();
            msgBox->setWindowFlags(msgBox->windowFlags() | Qt::WindowStaysOnTopHint);
            msgBox->setText(QString (par_HeaderString));
            msgBox->setInformativeText(QString (par_ErrorString));
            msgBox->setStandardButtons(QMessageBox::Ok);
            msgBox->setDefaultButton(QMessageBox::Ok);
            switch(par_Level) {
            default:
            case MESSAGE_STOP:
                 msgBox->setIcon(QMessageBox::Information);
                 break;
            case ERROR_NO_CRITICAL_STOP:
                msgBox->setIcon(QMessageBox::Warning);
                break;
            case ERROR_CRITICAL:
                msgBox->setIcon(QMessageBox::Critical);
                break;
            case ERROR_SYSTEM_CRITICAL:
                msgBox->setIcon(QMessageBox::Critical);
                break;
            }
            int DisableFlag = 0;
            if (s_main_ini_val.StopSchedulerWhileDialogOpen) {
                if (disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_SYSTEM, nullptr, nullptr) == 0) {
                    DisableFlag = 1;
                }
            }
            CALL_MAIN_WINDOW(removeTopHintFlagControlPanel);
            msgBox->exec();
            if (DisableFlag && s_main_ini_val.StopSchedulerWhileDialogOpen) {
                enable_scheduler (SCHEDULER_CONTROLED_BY_SYSTEM);
            }
            CALL_MAIN_WINDOW(setTopHintFlagControlPanel);
            delete msgBox;
        }
        ReturnValue = QMessageBox::Ok;
        break;
    }
    case ERROR_OKCANCEL:   // all with an OK and a Cancel button
    case QUESTION_OKCANCEL:
    case MESSAGE_OKCANCEL:
    {
        if (GetError2MessageFlag()) {
            Error2MessageOut(par_Level, par_HeaderString, par_ErrorString, par_ErrorMessagesGuiThread);
            ReturnValue = QMessageBox::Ok;   // If error should be redirect confirm all wit OK
        } else {
            QMessageBox msgBox;
            msgBox.setText(QString (par_HeaderString));
            msgBox.setInformativeText(QString (par_ErrorString));
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Question);
            if (s_main_ini_val.StopSchedulerWhileDialogOpen) {
                disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_SYSTEM, nullptr, nullptr);
            }
            CALL_MAIN_WINDOW(removeTopHintFlagControlPanel);
            ReturnValue = msgBox.exec();
            if (s_main_ini_val.StopSchedulerWhileDialogOpen) {
                enable_scheduler (SCHEDULER_CONTROLED_BY_SYSTEM);
            }
            CALL_MAIN_WINDOW(setTopHintFlagControlPanel);
        }
        break;
    }
    case ERROR_OK_OKALL_CANCEL:
    {
        if (GetError2MessageFlag()) {
            Error2MessageOut(par_Level, par_HeaderString, par_ErrorString, par_ErrorMessagesGuiThread);
            ReturnValue = QMessageBox::Ok;   // If error should be redirect confirm all wit OK
        } else {
            QMessageBox msgBox;
            msgBox.setText(QString (par_HeaderString));
            msgBox.setInformativeText(QString (par_ErrorString));
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::YesAll | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Question);
            if (s_main_ini_val.StopSchedulerWhileDialogOpen) {
                disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_SYSTEM, nullptr, nullptr);
            }
            CALL_MAIN_WINDOW(removeTopHintFlagControlPanel);
            ReturnValue = msgBox.exec();
            if (s_main_ini_val.StopSchedulerWhileDialogOpen) {
                enable_scheduler (SCHEDULER_CONTROLED_BY_SYSTEM);
            }
            CALL_MAIN_WINDOW(setTopHintFlagControlPanel);
        }
        break;
    }
    case ERROR_OK_CANCEL_CANCELALL:
    {
        if (GetError2MessageFlag()) {
            Error2MessageOut(par_Level, par_HeaderString, par_ErrorString, par_ErrorMessagesGuiThread);
            ReturnValue = QMessageBox::Ok;   // If error should be redirect confirm all wit OK
        } else {
            QMessageBox msgBox;
            msgBox.setText(QString (par_HeaderString));
            msgBox.setInformativeText(QString (par_ErrorString));
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel | QMessageBox::NoToAll);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Question);
            if (s_main_ini_val.StopSchedulerWhileDialogOpen) {
                disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_SYSTEM, nullptr, nullptr);
            }
            CALL_MAIN_WINDOW(removeTopHintFlagControlPanel);
            ReturnValue = msgBox.exec();
            if (s_main_ini_val.StopSchedulerWhileDialogOpen) {
                enable_scheduler (SCHEDULER_CONTROLED_BY_SYSTEM);
            }
            CALL_MAIN_WINDOW(setTopHintFlagControlPanel);
        }
        break;
    }
    case ERROR_OK_OKALL_CANCEL_CANCELALL:
    {
        if (GetError2MessageFlag()) {
            Error2MessageOut(par_Level, par_HeaderString, par_ErrorString, par_ErrorMessagesGuiThread);
            ReturnValue = QMessageBox::Ok;   // If error should be redirect confirm all wit OK
        } else {
            QMessageBox msgBox;
            msgBox.setText(QString (par_HeaderString));
            msgBox.setInformativeText(QString (par_ErrorString));
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::YesAll | QMessageBox::Cancel | QMessageBox::NoToAll);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Question);
            if (s_main_ini_val.StopSchedulerWhileDialogOpen) {
                disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_SYSTEM, nullptr, nullptr);
            }
            CALL_MAIN_WINDOW(removeTopHintFlagControlPanel);
            ReturnValue = msgBox.exec();
            if (s_main_ini_val.StopSchedulerWhileDialogOpen) {
                enable_scheduler (SCHEDULER_CONTROLED_BY_SYSTEM);
            }
            CALL_MAIN_WINDOW(setTopHintFlagControlPanel);
        }
        break;
    }
    case QUESTION_SAVE_IGNORE_CANCLE:
    {
        if (GetError2MessageFlag()) {
            Error2MessageOut(par_Level, par_HeaderString, par_ErrorString, nullptr);
            ReturnValue = QMessageBox::Ok;   // If error should be redirect confirm all wit OK
        } else {
            QMessageBox msgBox;
            msgBox.setText(QString (par_HeaderString));
            msgBox.setInformativeText(QString (par_ErrorString));
            msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Ignore | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Save);
            msgBox.setIcon(QMessageBox::Question);
            if (s_main_ini_val.StopSchedulerWhileDialogOpen) {
                disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_SYSTEM, nullptr, nullptr);
            }
            CALL_MAIN_WINDOW(removeTopHintFlagControlPanel);
            ReturnValue = msgBox.exec();
            if (s_main_ini_val.StopSchedulerWhileDialogOpen) {
                enable_scheduler (SCHEDULER_CONTROLED_BY_SYSTEM);
            }
            CALL_MAIN_WINDOW(setTopHintFlagControlPanel);
        }
        break;
    }

    case RT_INFO_MESSAGE:
        AddScriptMessage (par_HeaderString);
        AddScriptMessage (par_ErrorString);
        ReturnValue = QMessageBox::Ok;
        break;
    case RT_ERROR:
        AddScriptMessage (par_HeaderString);
        AddScriptMessage (par_ErrorString);
        if (par_ErrorMessagesGuiThread != nullptr) {
            par_ErrorMessagesGuiThread->AddNoneModalErrorMessage (par_Level, par_Cycle, par_Pid, par_ErrorString);
        }
        ReturnValue = QMessageBox::Ok;
        break;

    case INFO_NO_STOP:
        AddScriptMessage (par_HeaderString);
        AddScriptMessage (par_ErrorString);
        if (par_ErrorMessagesGuiThread != nullptr) {
            par_ErrorMessagesGuiThread->AddNoneModalErrorMessage (par_Level, par_Cycle, par_Pid, par_ErrorString);
        }
        ReturnValue = QMessageBox::Ok;
        break;
    case WARNING_NO_STOP:
        AddScriptMessage (par_HeaderString);
        AddScriptMessage (par_ErrorString);
        if (par_ErrorMessagesGuiThread != nullptr) {
            par_ErrorMessagesGuiThread->AddNoneModalErrorMessage (par_Level, par_Cycle, par_Pid, par_ErrorString);
        }
        ReturnValue = QMessageBox::Ok;
        break;
    case ERROR_NO_STOP:
        AddScriptMessage (par_HeaderString);
        AddScriptMessage (par_ErrorString);
        if (par_ErrorMessagesGuiThread != nullptr) {
            par_ErrorMessagesGuiThread->AddNoneModalErrorMessage (par_Level, par_Cycle, par_Pid, par_ErrorString);
        }
        ReturnValue = QMessageBox::Ok;
        break;
    case INFO_NOT_WRITEN_TO_MSG_FILE:
        if (par_ErrorMessagesGuiThread != nullptr) {
            par_ErrorMessagesGuiThread->AddNoneModalErrorMessage (par_Level, par_Cycle, par_Pid, par_ErrorString);
        }
        ReturnValue = QMessageBox::Ok;
        break;
    }
    return ReturnValue;
}


int ErrorMessagesFromOtherThread::Error(int par_Level, uint64_t par_Cycle, const char *par_HeaderString, const char *par_ErrorString, int par_IsGuiMainThreadFlag)
{
    int x;
    int Ret;

    EnterCriticalSection (&ErrorEventsCriticalSection);
    for (x = 0; x < MAX_SIMULTANEOUSLY_ERROR_EVENTS; x++) {
        if (!ErrorEvents[x].InUseFlag)  {
            ErrorEvents[x].InUseFlag = 1;
            ErrorEvents[x].Level = par_Level;
            ErrorEvents[x].HeaderString = par_HeaderString;
            ErrorEvents[x].ErrorString = par_ErrorString;
            ErrorEvents[x].Cycle = par_Cycle;
            ErrorEvents[x].Pid = GetCurrentPid();
            ErrorEvents[x].ReturnValue = 0;
            break;
        }
    }
    LeaveCriticalSection (&ErrorEventsCriticalSection);
    // Transfer it to the GUI thread
    if (x < MAX_SIMULTANEOUSLY_ERROR_EVENTS) {
        if (par_IsGuiMainThreadFlag) {
            // call the slot direct
            ErrorMessagesGuiThreadInst.ErrorMessagesFromOtherThreadSlot (x);
            EnterCriticalSection (&ErrorEventsCriticalSection);
            my_free (ErrorEvents[x].ErrorString);
            my_free (ErrorEvents[x].HeaderString);
            Ret = ErrorEvents[x].ReturnValue;
            ErrorEvents[x].InUseFlag = 0;
            LeaveCriticalSection (&ErrorEventsCriticalSection);
        } else {
            EnterCriticalSection (&(ErrorEvents[x].CriticalSection));
            if (SyncWithGuiThreadIsInitAndRunning) {
                emit ErrorMessagesFromOtherThreadSignal (x);
            } else {
                ErrorEvents[x].Waiting = 1;
            }
            SleepConditionVariableCS_INFINITE(&(ErrorEvents[x].ConditionVariable), &(ErrorEvents[x].CriticalSection));
            LeaveCriticalSection (&(ErrorEvents[x].CriticalSection));
            EnterCriticalSection (&ErrorEventsCriticalSection);
            my_free (ErrorEvents[x].ErrorString);
            my_free (ErrorEvents[x].HeaderString);
            Ret = ErrorEvents[x].ReturnValue;
            ErrorEvents[x].InUseFlag = 0;
            LeaveCriticalSection (&ErrorEventsCriticalSection);
        }
    } else {
        Ret = -1;
    }
    return Ret;
}

extern "C" {
void InitErrorDialog (void)
{
    int x;

    MainThreadId = GetCurrentThreadId();  // Store the main thread ID
    InitializeCriticalSection (&LockErrorPopUpMessageCriticalSection);
    InitializeConditionVariable (&LockErrorPopUpMessageConditionVariable);
    InitializeCriticalSection (&ErrorEventsCriticalSection);
    for (x = 0; x < MAX_SIMULTANEOUSLY_ERROR_EVENTS; x++) {
        InitializeCriticalSection (&(ErrorEvents[x].CriticalSection));
        InitializeConditionVariable (&(ErrorEvents[x].ConditionVariable));
    }
    ErrorMessagesGuiThreadInst.Init();
}

// ensure that no error message would be accepted after start terminating
void TerminateErrorDialog (void)
{
    int x;

    WakeAllConditionVariable (&LockErrorPopUpMessageConditionVariable);
    for (x = 0; x < MAX_SIMULTANEOUSLY_ERROR_EVENTS; x++) {
        WakeAllConditionVariable (&(ErrorEvents[x].ConditionVariable));
    }
}

}

ErrorMessagesGuiThread::ErrorMessagesGuiThread()
{
    m_ExtErrorDialog = nullptr;
}

void ErrorMessagesGuiThread::Init()
{
    bool Ret =connect (&ErrorMessagesFromOtherThreadInst, SIGNAL(ErrorMessagesFromOtherThreadSignal(int)),
                       this, SLOT(ErrorMessagesFromOtherThreadSlot(int)));
    if (Ret == false) {
        printf ("connect error in error message");
    }
}




void ErrorMessagesGuiThread::ErrorMessagesFromOtherThreadSlot(int par_Index)
{
    EnterCriticalSection (&(ErrorEvents[par_Index].CriticalSection));

    ErrorEvents[par_Index].ReturnValue = ErrorMessageInnerFunction(ErrorEvents[par_Index].Level,
                                                                   ErrorEvents[par_Index].HeaderString,
                                                                   ErrorEvents[par_Index].ErrorString,
                                                                   ErrorEvents[par_Index].Pid,
                                                                   ErrorEvents[par_Index].Cycle,
                                                                   this);

    WakeAllConditionVariable (&(ErrorEvents[par_Index].ConditionVariable));
    LeaveCriticalSection (&(ErrorEvents[par_Index].CriticalSection));
}

extern "C" void CheckErrorMessagesBeforeMainWindowIsOpen(void)
{
    int x;
    EnterCriticalSection (&ErrorEventsCriticalSection);
    for (x = 0; x < MAX_SIMULTANEOUSLY_ERROR_EVENTS; x++) {
        if (ErrorEvents[x].InUseFlag)  {
            EnterCriticalSection (&(ErrorEvents[x].CriticalSection));
            if (ErrorEvents[x].Waiting)  {
                ErrorEvents[x].Waiting = 0;
                ErrorEvents[x].ReturnValue = ErrorMessageInnerFunction(ErrorEvents[x].Level,
                                                                       ErrorEvents[x].HeaderString,
                                                                       ErrorEvents[x].ErrorString,
                                                                       ErrorEvents[x].Pid,
                                                                       ErrorEvents[x].Cycle,
                                                                       nullptr);
                WakeAllConditionVariable (&(ErrorEvents[x].ConditionVariable));
            }
            LeaveCriticalSection (&(ErrorEvents[x].CriticalSection));
        }
    }
    LeaveCriticalSection (&ErrorEventsCriticalSection);
}

void ErrorMessagesGuiThread::AddNoneModalErrorMessage(int Level, uint64_t Cycle, int Pid, const char *Message)
{
    if (!m_TerminatedFlag) {
        if (m_ExtErrorDialog == nullptr) {
            m_ExtErrorDialog = new ExtendedErrorDialog();
            m_ExtErrorDialog->show();
        }
        if (m_ExtErrorDialog != nullptr) {
            if (!m_ExtErrorDialog->IsVisable()) {
                m_ExtErrorDialog->show();
            }
            m_ExtErrorDialog->AddMessage (Level, Cycle, Pid, Message);
        }
    }
}

void ErrorMessagesGuiThread::AddErrorToStatusBar(int Level, const char *Header, const char *Message)
{
    MainWindow *PtrToMainWindow = MainWindow::GetPtrToMainWindow();
    if (PtrToMainWindow != nullptr) {
        static enum {SUPPRESSED_NOTHING, SUPPRESSED_WARNING, SUPPRESSED_ERROR} State, OldState;
        switch (Level) {
        case MESSAGE_ONLY:
        case MESSAGE_STOP:
        case ERROR_NO_CRITICAL:
        case ERROR_NO_CRITICAL_STOP:
            State = SUPPRESSED_WARNING;
            break;
        case ERROR_CRITICAL:
        case ERROR_SYSTEM_CRITICAL:
        case ERROR_OKCANCEL:
        case RT_ERROR:
        case ERROR_OK_OKALL_CANCEL:
            State = SUPPRESSED_ERROR;
            break;
        case INFO_NO_STOP:
        case WARNING_NO_STOP:
            State = SUPPRESSED_WARNING;
            break;
        case ERROR_NO_STOP:
            State = SUPPRESSED_ERROR;
            break;
        case RT_INFO_MESSAGE:
            State = SUPPRESSED_WARNING;
            break;
        case ERROR_OK_OKALL_CANCEL_CANCELALL:
            State = SUPPRESSED_ERROR;
            break;
        case INFO_NOT_WRITEN_TO_MSG_FILE:
            State = SUPPRESSED_WARNING;
            break;
        case ERROR_OK_CANCEL_CANCELALL:
            State = SUPPRESSED_ERROR;
            break;
        case QUESTION_OKCANCEL:
        case MESSAGE_OKCANCEL:
            State = SUPPRESSED_WARNING;
            break;
        default:
            State = SUPPRESSED_WARNING;
            break;
        }
        if (OldState != SUPPRESSED_ERROR) {
            if ((OldState == SUPPRESSED_NOTHING) && (State != SUPPRESSED_NOTHING)) {
                PtrToMainWindow->installEventFilter(new SatatusbarEventFilter);
                QPushButton *PushButtom = new QPushButton("open");
                PtrToMainWindow->statusBar()->insertPermanentWidget(0,PushButtom);
                connect(PushButtom, SIGNAL(clicked()), PtrToMainWindow, SLOT(toolbarOpenErrorFileWithEditor()));
            }
            OldState = State;
        }
        QString Line;
        Line.append("suppressed error/warning (");
        Line.append(Header);
        Line.append(" / ");
        Line.append(Message);
        Line.append("), you can see all suppressed error/warning here: ");

        Line.append(GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_ERROR_FILE));
        Line.append(".err");
        if (State == SUPPRESSED_ERROR) PtrToMainWindow->statusBar()->setStyleSheet("background-color: rgb(255, 192, 0);color: rgb(0, 0, 0);");
        else if (State == SUPPRESSED_WARNING) PtrToMainWindow->statusBar()->setStyleSheet("background-color: rgb(255, 255, 0);color: rgb(0, 0, 0);");
        PtrToMainWindow->statusBar()->showMessage(Line);
    }
}


void ErrorMessagesGuiThread::TerminateExtErrorMessageWindow (void)
{
    m_TerminatedFlag = true;   // XilEnv will be terminated -> do not open any error message windows
    if (m_ExtErrorDialog != nullptr) {
        if (m_ExtErrorDialog->IsVisable()) {
            m_ExtErrorDialog->close();
        }
    }
}

void TerminateExtErrorMessageWindow (void)
{
    ErrorMessagesGuiThreadInst.TerminateExtErrorMessageWindow ();
}

extern "C" {
int ErrorMessagesFromOtherThreadInst_Error(int par_Level, uint64_t par_Cycle, const char *par_HeaderString, const char *par_ErrorString, int par_IsGuiMainThreadFlag)
{
    return TranslateReturnCode(ErrorMessagesFromOtherThreadInst.Error(par_Level, par_Cycle, par_HeaderString, par_ErrorString, par_IsGuiMainThreadFlag));
}

int IsMainThread(void)
{
    return (MainThreadId == GetCurrentThreadId());
}
}


bool SatatusbarEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::StatusTip) {
        return true;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}
