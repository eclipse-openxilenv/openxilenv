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


#include "ScriptDebuggingDialog.h"
#include "ui_ScriptDebuggingDialog.h"
#include "StringHelpers.h"

#include <QTimer>
#include <QMenu>
#include <QPoint>

extern "C" {
#include "InterfaceToScript.h"
#include "MyMemory.h"
#include "IniDataBase.h"
#include "EnvironmentVariables.h"
}
#include "Script.h"

static bool IsScriptDebuggingWindowOpen;
static bool IsScriptDebuggingWindowVisable;
static bool StopScriptAtStartDebuggingFlag;


ScriptDebuggingDialog::ScriptDebuggingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ScriptDebuggingDialog)
{
    ui->setupUi(this);
    ui->EditPushButton->setDisabled(true);

    m_Timer = new QTimer(this);
    m_Timer->setInterval(100);
    m_Timer->start();
    connect(m_Timer, SIGNAL(timeout()), this, SLOT(CyclicUpdate()));
    m_ScriptFileCount = 0;
    m_DisplayedScriptNumber = -1;
    m_EnvironmentVariableListBuffer = nullptr;
    m_SizeOfEnvironmentVariableListBuffer = 0;
    m_EnvironmentVariableListRefBuffer = nullptr;
    m_SizeOfEnvironmentVariableListRefBuffer = 0;
    m_StackChangedCounter = 0;
    m_ParameterListBuffer = nullptr;
    m_SizeOfParameterListBuffer = 0;
    m_ParameterListRefBuffer = nullptr;
    m_SizeOfParameterListRefBuffer = 0;
    m_BreakpointChangedCounter = 0;

    m_CyclicUpdateLastFile = -1;
    m_CyclicUpdateLastLine = -1;
    m_CyclicUpdateLasDelayCounter = -1;

    m_FileChangedByUserFileNo = -1;
    m_WillChangedNoFromUser = false;
    m_CurrentFileNo = -1;
    m_CurrentLineNo = -1;
    m_CurrentFileNoLastCycle = -1;
    m_CurrentLineNoLastCycle = -1;

    ui->ScriptFilePlainTextEdit->setCenterOnScroll(true);
    ui->ScriptFilePlainTextEdit->setContextMenuPolicy (Qt::CustomContextMenu);
    bool Ret;
    Ret = connect (ui->ScriptFilePlainTextEdit, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OpenContextMenuSlot(QPoint)));
    if (!Ret) {
        ThrowError (1, "connect");
    }

    m_AddBereakpointAct = new QAction(tr("&add new breakpoint"), this);
    m_AddBereakpointAct->setStatusTip(tr("add new breakboint at this line"));
    Ret = connect(m_AddBereakpointAct, SIGNAL(triggered()), this, SLOT(AddBreakpointSlot()));
    if (!Ret) {
        ThrowError (1, "connect");
    }
    m_DeleteBereakpointAct = new QAction(tr("&delete breakpoint"), this);
    m_DeleteBereakpointAct->setStatusTip(tr("delete breakboint at this line"));
    Ret = connect(m_DeleteBereakpointAct, SIGNAL(triggered()), this, SLOT(DeleteBreakpointSlot()));
    if (!Ret) {
        ThrowError (1, "connect");
    }
}

ScriptDebuggingDialog::~ScriptDebuggingDialog()
{
    delete ui;
}

void ScriptDebuggingDialog::CyclicUpdate()
{
    if (ScriptEnterCriticalSection()) {
        int ChangedCounter = cFileCache::GetChangedCounter();
        if (m_ChangedCounter != ChangedCounter) {  // Are there changes inside the script file cache
            m_ChangedCounter = ChangedCounter;
            int ScriptFileCount = cFileCache::GetNumberOfCachedFiles();
            ui->ScriptFileComboBox->clear();
            for (int x = 0; x < ScriptFileCount; x++) {
                cFileCache *ScriptFile = cFileCache::IsFileCached (x);
                if (ScriptFile != nullptr) {
                    m_WillChangedNoFromUser = true;
                    ui->ScriptFileComboBox->addItem(QString (ScriptFile->GetFilename()));
                    m_WillChangedNoFromUser = false;
                }
            }
            m_FileChangedByUserFileNo = -1;
        }
        int StartSelOffset;
        int EndSelOffset;
        int FileNr;
        int LineNr;
        int DelayCounter;
        cExecutor *Executor;
        cParser *Parser;

        m_CurrentFileNoLastCycle = m_CurrentFileNo;
        m_CurrentLineNoLastCycle = m_CurrentLineNo;

        if (ScriptDebugGetCurrentState (&StartSelOffset, &EndSelOffset, &m_CurrentFileNo, &m_CurrentLineNo,
                                        &DelayCounter, &Executor, &Parser) == 0) {
            bool AutoScroll = false;
            if ((m_FileChangedByUserFileNo >= 0) &&               // the user has changed the displaying file
                (m_CurrentFileNoLastCycle == m_CurrentFileNo) &&  // and the debug positions has not changed
                (m_CurrentLineNoLastCycle == m_CurrentLineNo)) {
                FileNr = m_FileChangedByUserFileNo;
                LineNr = m_CurrentLineNo;
                StartSelOffset = -1;
                EndSelOffset = -1;
            } else {
                FileNr = m_CurrentFileNo;
                LineNr = m_CurrentLineNo;
                m_FileChangedByUserFileNo = -1;  // switch back to current file
            }
            if (m_DisplayedScriptNumber != FileNr) {   // The script file has changed
                ui->ScriptFilePlainTextEdit->clear();
                cFileCache *ScriptFile = cFileCache::IsFileCached (FileNr);
                if (ScriptFile != nullptr) {
                    m_WillChangedNoFromUser = true;
                    ui->ScriptFileComboBox->setCurrentText(QString(ScriptFile->GetFilename()));
                    m_WillChangedNoFromUser = false;
                    ui->ScriptFilePlainTextEdit->appendPlainText(QString ().fromLatin1(ScriptFile->GetPtr(0)));
                }
                m_DisplayedScriptNumber = FileNr;
                AutoScroll = true;
            }

            // Update if the file has changed
            if (m_CyclicUpdateLastFile != FileNr) {
                m_CyclicUpdateLastFile = FileNr;
                m_CyclicUpdateLastLine = -1;
                m_CyclicUpdateLasDelayCounter = -1;
            }
            if (m_CyclicUpdateLasDelayCounter != DelayCounter) {
                m_CyclicUpdateLasDelayCounter = DelayCounter;
                ui->DelayLineEdit->setText(QString().number(DelayCounter));
            }
            int BreakpointChangedCounter = Executor->GetBreakpointChangedCounter();
            if (m_BreakpointChangedCounter != BreakpointChangedCounter) {
                m_BreakpointChangedCounter = BreakpointChangedCounter;
                m_CyclicUpdateLastLine = -1;
                ui->BreakpointsListWidget->clear();
                char BreakpointLine[1024];
                for (int x = 0; ; x++) {
                    if (Executor->GetBreakpointString (x, BreakpointLine)) break;
                    ui->BreakpointsListWidget->addItem(CharToQString(BreakpointLine));
                }
            } else {
                AutoScroll = true;
            }
            if (m_CyclicUpdateLastLine != LineNr) {
                m_CyclicUpdateLastLine = LineNr;

                ui->LineLineEdit->setText(QString().number(LineNr));
                QList<QTextEdit::ExtraSelection> Selections;
                QTextEdit::ExtraSelection Selection;

                // Breakpoints:
                int ActiveValid;
                int Ip;
                int CurrentPosFlag = 1;
                for (int Idx = Executor->GetNextBreakpoint (-1, FileNr, &Ip, &LineNr, &ActiveValid);
                    Idx > -1;
                    Idx = Executor->GetNextBreakpoint (Idx, FileNr, &Ip, &LineNr, &ActiveValid)) {
                    int BpStartSelOffset = Executor->GetCmdFileOffset (Ip);
                    int BpEndSelOffset = Executor->GetCmdBeginOfParamOffset (Ip);
                    Selection.cursor = QTextCursor(ui->ScriptFilePlainTextEdit->document());
                    Selection.cursor.setPosition(BpStartSelOffset);
                    Selection.cursor.setPosition(BpEndSelOffset, QTextCursor::KeepAnchor);
                    if (BpStartSelOffset == StartSelOffset) {
                        Selection.format.setBackground (Qt::green);
                        CurrentPosFlag = 0;
                    } else {
                        Selection.format.setBackground (Qt::red);
                    }
                    Selections.append(Selection);
                }
                // Current position
                if (CurrentPosFlag) {
                    Selection.cursor = QTextCursor(ui->ScriptFilePlainTextEdit->document());
                    Selection.cursor.setPosition(StartSelOffset);
                    Selection.cursor.setPosition(EndSelOffset, QTextCursor::KeepAnchor );
                    Selection.format.setBackground (Qt::cyan);
                    Selections.append(Selection);

                }
                ui->ScriptFilePlainTextEdit->setExtraSelections(Selections);
                // Automatic scrolling
                if (AutoScroll) {
                    QTextCursor Cursor = ui->ScriptFilePlainTextEdit->textCursor();
                    Cursor.setPosition(StartSelOffset);
                    ui->ScriptFilePlainTextEdit->setTextCursor(Cursor);
                    ui->ScriptFilePlainTextEdit->ensureCursorVisible();
                }
            }
            int NewStackChangedCounter = Executor->Stack.GetStackChangedCounter();
            if (m_StackChangedCounter != NewStackChangedCounter) {
                m_StackChangedCounter = NewStackChangedCounter;
                char StackLine[1024];
                ui->StackListWidget->clear();
                for (int x = 0; ; x++) {
                    if (!Executor->Stack.PrintOneStackToString(x, StackLine, sizeof (StackLine))) break;
                    ui->StackListWidget->addItem(CharToQString(StackLine));
                }
            }
            if (GetEnvironmentVariableList (&m_EnvironmentVariableListBuffer, &m_SizeOfEnvironmentVariableListBuffer,
                                            &m_EnvironmentVariableListRefBuffer, &m_SizeOfEnvironmentVariableListRefBuffer)) {
                ui->EnvironmentListWidget->clear();
                if (m_EnvironmentVariableListBuffer != nullptr) {
                     char *p = m_EnvironmentVariableListBuffer;
                     while (*p != 0) {
                         ui->EnvironmentListWidget->addItem(CharToQString(p));
                         while (*p != 0) p++;
                         p++;
                     }
                }
            }
            if (Parser->GetParamListString (&m_ParameterListBuffer, &m_SizeOfParameterListBuffer,
                                            &m_ParameterListRefBuffer, &m_SizeOfParameterListRefBuffer)) {
                ui->ParameterListListWidget->clear();
                if (m_ParameterListBuffer != nullptr) {
                     char *p = m_ParameterListBuffer;
                     while (*p != 0) {
                         ui->ParameterListListWidget->addItem(CharToQString(p));
                         while (*p != 0) p++;
                         p++;
                     }
                }
            }
        }
        ScriptLeaveCriticalSection();
    }
}


void ScriptDebuggingDialog::WriteToIni()
{
    IniFileDataBaseWriteInt ("BasicSettings", "ScriptDebugWindowXPosition", this->x(), GetMainFileDescriptor());
    IniFileDataBaseWriteInt ("BasicSettings", "ScriptDebugWindowYPosition", this->y(), GetMainFileDescriptor());
    IniFileDataBaseWriteInt ("BasicSettings", "ScriptDebugWindowXSize", this->width(), GetMainFileDescriptor());
    IniFileDataBaseWriteInt ("BasicSettings", "ScriptDebugWindowYSize", this->height(), GetMainFileDescriptor());
}

void ScriptDebuggingDialog::AddBreakpoint()
{
    QTextCursor Cursor = ui->ScriptFilePlainTextEdit->textCursor();
    int CurrentLine = Cursor.blockNumber() + 1;
    QString Filename = ui->ScriptFileComboBox->currentText();
    int FileNr;
    int Ip;

    if (ScriptEnterCriticalSection()) {
        if (CheckIfBreakPointIsValid (QStringToConstChar(Filename), CurrentLine, &FileNr, &Ip)) {
            AddBreakPoint (1, FileNr, QStringToConstChar(Filename),
                           CurrentLine, Ip, 0, nullptr);
        }
        ScriptLeaveCriticalSection();
    }
}

void ScriptDebuggingDialog::DeleteBreakpoint(QString &Filename, int LineNr)
{
    if (ScriptEnterCriticalSection()) {
        DelBreakPoint (QStringToConstChar(Filename), LineNr);
        ScriptLeaveCriticalSection();
    }
}

int ScriptDebuggingDialog::IsABreakpointOnThisLine(QString &Filename, int LineNr)
{
    int Ret = 0;
    if (ScriptEnterCriticalSection()) {
        Ret = IsThereABreakPoint (QStringToConstChar(Filename), LineNr);
        ScriptLeaveCriticalSection();
    }
    return Ret;
}

void ScriptDebuggingDialog::closeEvent(QCloseEvent *event)
{
    WriteToIni();
    IsScriptDebuggingWindowVisable = false;
    this->setVisible(false);
    emit SetScriptDebuggingWindowStateSignal(false);  // Clear checkbox inside control panel
    event->ignore();                                  // Window will not be closed really it be only hide
}


void ScriptDebuggingDialog::on_StopPushButton_clicked()
{
    DebugStopScriptExecution();
}

void ScriptDebuggingDialog::on_ContinuePushButton_clicked()
{
    DebugContScriptExecution();
}

void ScriptDebuggingDialog::on_StepOverPushButton_clicked()
{
    DebugStepOverScriptExecution();
}

void ScriptDebuggingDialog::on_StepIntoPushButton_clicked()
{
    DebugStepIntoScriptExecution();
}

void ScriptDebuggingDialog::on_StepOutpushButton_clicked()
{
    DebugStepOutScriptExecution();
}


// Use signal/slot for synchronizing from other threads
static ScriptDebuggingToGuiThread ScriptDebuggingToGuiThreadInst;

void ScriptDebuggingToGuiThread::OpenWindow(ControlPanel *par_ControlPanel, QWidget *par_Parent)
{
    if (!IsScriptDebuggingWindowOpen) {
        IsScriptDebuggingWindowOpen = true;
        m_Parent = par_Parent;
        m_ControlPanel = par_ControlPanel;
        if (m_ScriptDebuggingWindow == nullptr) {
            m_ScriptDebuggingWindow = new ScriptDebuggingDialog(m_Parent);
        }
        int xpos = IniFileDataBaseReadInt ("BasicSettings", "ScriptDebugWindowXPosition", -1, GetMainFileDescriptor());
        int ypos = IniFileDataBaseReadInt ("BasicSettings", "ScriptDebugWindowYPosition", -1, GetMainFileDescriptor());
        // Are this valid positions
        if (xpos < 100) xpos = 100;
        if (ypos < 0) ypos = 0;

        // Do not set outside desktop
#ifdef _WIN32
        RECT rc;

        GetWindowRect (GetDesktopWindow (), &rc);
        int max_xpos = rc.right - 100;
        int max_ypos = rc.bottom - 100;
        if (xpos > max_xpos) xpos = max_xpos;
        if (ypos > max_ypos) ypos = max_ypos;
        m_ScriptDebuggingWindow->move (xpos, ypos);
#else
#endif
        bool Ret = connect(m_ScriptDebuggingWindow, SIGNAL(SetScriptDebuggingWindowStateSignal(bool)), m_ControlPanel, SLOT(SetScriptDebugWindowStateSlot(bool)));
        printf ("%i", Ret);
    }
    if (!IsScriptDebuggingWindowVisable) {
        IsScriptDebuggingWindowVisable = true;
        m_ScriptDebuggingWindow->show();
    }
}

void ScriptDebuggingToGuiThread::CloseWindow()
{
    if (m_ScriptDebuggingWindow != nullptr) {
        if (IsScriptDebuggingWindowVisable) {
            IsScriptDebuggingWindowVisable = false;
            StopScriptAtStartDebuggingFlag = false;
            m_ScriptDebuggingWindow->close();
        }
    }
}

void InitScriptDebuggingWindow(ControlPanel *par_ControlPanel, QWidget *parent)
{
    ScriptDebuggingToGuiThreadInst.OpenWindow(par_ControlPanel, parent);
}

void CloseScriptDebuggingWindow()
{
    ScriptDebuggingToGuiThreadInst.CloseWindow();
}

int IsScriptDebugWindowOpen()
{
    return IsScriptDebuggingWindowVisable;
}

int GetStopScriptAtStartDebuggingFlag ()
{
    return StopScriptAtStartDebuggingFlag;
}

void ScriptDebuggingDialog::on_AddPushButton_clicked()
{
    AddBreakpoint();
}

void ScriptDebuggingDialog::OpenContextMenuSlot(const QPoint &pos)
{
    QTextCursor Cursor = ui->ScriptFilePlainTextEdit->textCursor();
    int CurrentLine = Cursor.blockNumber() + 1;
    QString Filename = ui->ScriptFileComboBox->currentText();

    if (IsThereABreakPoint(QStringToConstChar(Filename), CurrentLine)) {
        m_AddBereakpointAct->setDisabled(true);
        m_DeleteBereakpointAct->setEnabled(true);
    } else {
        m_AddBereakpointAct->setEnabled(true);
        m_DeleteBereakpointAct->setDisabled(true);
    }
    QMenu *Menu = ui->ScriptFilePlainTextEdit->createStandardContextMenu();
    Menu->addAction(m_AddBereakpointAct);
    Menu->addAction(m_DeleteBereakpointAct);
    Menu->popup(ui->ScriptFilePlainTextEdit->viewport()->mapToGlobal(pos));
}

void ScriptDebuggingDialog::AddBreakpointSlot()
{
    AddBreakpoint();
}

void ScriptDebuggingDialog::DeleteBreakpointSlot()
{
    QTextCursor Cursor = ui->ScriptFilePlainTextEdit->textCursor();
    int CurrentLine = Cursor.blockNumber() + 1;
    QString Filename = ui->ScriptFileComboBox->currentText();
    DeleteBreakpoint(Filename, CurrentLine);
}

void ScriptDebuggingDialog::on_ScriptFileComboBox_currentIndexChanged(const QString &arg1)
{
    if (!m_WillChangedNoFromUser) {
        m_SelectedScriptFile = arg1;
        if (cFileCache::IsFileCached (QStringToConstChar(m_SelectedScriptFile), &m_FileChangedByUserFileNo) == nullptr) {
            m_FileChangedByUserFileNo = -1;
        }
        m_DisplayedScriptNumber = -1;  // Load new script file
    }
}

void ScriptDebuggingDialog::on_DeletePushButton_clicked()
{
    QListWidgetItem *Item = ui->BreakpointsListWidget->currentItem();
    if (Item != nullptr) {
        QString Breakpoint = Item->data(Qt::DisplayRole).toString();
        QString Filename = ParseBreakpointLineFileName (Breakpoint);
        int LineNr = ParseBreakpointLineLineNr(Breakpoint);
        DeleteBreakpoint(Filename, LineNr);
    }
}

QString ScriptDebuggingDialog::ParseBreakpointLineFileName (QString &Line)
{
    QStringList List = Line.split('\t');
    return List.at(1);
}

int ScriptDebuggingDialog::ParseBreakpointLineLineNr (QString &Line)
{
    QStringList List = Line.split('\t');
    return List.at(2).toInt();
}

void ScriptDebuggingDialog::on_StayOnTopCheckBox_clicked(bool checked)
{
    Qt::WindowFlags flags = this->windowFlags();
    if (checked) this->setWindowFlags(flags | Qt::WindowStaysOnTopHint);
    else this->setWindowFlags(flags & ~Qt::WindowStaysOnTopHint);
    this->show();
}

void ScriptDebuggingDialog::on_StopAtStartCheckBox_clicked(bool checked)
{
    StopScriptAtStartDebuggingFlag = checked;
}
