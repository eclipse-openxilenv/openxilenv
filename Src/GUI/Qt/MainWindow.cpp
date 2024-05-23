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

#include "Platform.h"  // because access!
#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "A2LCalMap/A2LCalMapType.h"
#include "A2LCalSingle/A2LCalSingleType.h"
#include "CalibrationTree/CalibrationTreeType.h"
#include "Oscilloscope.h"
#include "LampsType.h"
#include "EnumType.h"
#include "SliderType.h"
#include "UserDrawType.h"
#include "UserControlType.h"
#include "TextType.h"
#include "TachoType.h"
#include "BargraphType.h"
#include "KnobType.h"
#include "CanMessageWindowType.h"
#include "WindowNameAlreadyInUse.h"
#include "GetNewUniqueWindowTitleStartWith.h"
#include "BarrierHistoryLoggingDialog.h"
#include "ExternProcessCopyListDialog.h"
#include "BlackboardInternalDialog.h"
#include "RemoteMasterCallStatitsticsDialog.h"
#include "ActiveEquationsDialog.h"
#include "SearchWindowIncludedVariableDialog.h"
#include "CanConfigDialog.h"
#include "InsertCANErrorDialog.h"
#include "DbcImportDialog.h"
#include "LoadedDebugInfosDialog.h"
#include "BinaryCompareDialog.h"
#include "FileDialog.h"
#include "MessageWindow.h"
#include "ErrorDialog.h"
#include "ScriptErrorDialog.h"
#include "MainWinowSyncWithOtherThreads.h"
#include "StringHelpers.h"
#include "QtIniFile.h"

#include <QStyleFactory>
#include <QThread>
#include <QShortcut>
#include <QKeySequence>
#include <QScreen>
#include <QGuiApplication>

extern "C" {
#include "MyMemory.h"
#include "ThrowError.h"
#include "StringMaxChar.h"
#include "ConfigurablePrefix.h"
#include "InitProcess.h"
#include "MainValues.h"
#include "ParseCommandLine.h"
#include "EquationParser.h"
#include "EnvironmentVariables.h"
#include "InterfaceToScript.h"
#include "Scheduler.h"
#include "ReadDspConfig.h"
#include "RemoteMasterBlackboard.h"
}

static SignalsFromOtherThread SignalsFromOtherThreadInst;

void SignalsFromOtherThread::CloseXilEnv (int par_ExitCode, int par_ExitCodeValid)
{
    emit CloseXilEnvSignal (par_ExitCode, par_ExitCodeValid);
}

void SignalsFromOtherThread::ClearDesktopFromOtherThread(void)
{
    emit ClearDesktopFromOtherThreadSignal();
}

void SignalsFromOtherThread::LoadDesktopFromOtherThread(char *par_Filename)
{
    emit LoadDesktopFromOtherThreadSignal(QString (par_Filename));
}

void SignalsFromOtherThread::SaveDesktopFromOtherThread(char *par_Filename)
{
    emit SaveDesktopFromOtherThreadSignal(QString (par_Filename));
}

extern "C" {
void CloseFromOtherThread (int par_ExitCode, int par_ExitCodeValid)
{
    SignalsFromOtherThreadInst.CloseXilEnv(par_ExitCode, par_ExitCodeValid);
}
void ClearDesktopFromOtherThread (void)
{
    SignalsFromOtherThreadInst.ClearDesktopFromOtherThread();
}
void LoadDesktopFromOtherThread (char *par_Filename)
{
    //UnlockErrorPopUpMessage();
    SignalsFromOtherThreadInst.LoadDesktopFromOtherThread(par_Filename);
}
void SaveDesktopFromOtherThread (char *par_Filename)
{
    SignalsFromOtherThreadInst.SaveDesktopFromOtherThread(par_Filename);
}
}

void MainWindow::closeFromOtherThread(int par_ExitCode, int par_ExitCodeValid)
{
    if (!m_ExitCodeSeeded) {    // set Exit Code only once!
        if (par_ExitCodeValid) {
            m_ExitCode = par_ExitCode;
            m_ExitCodeSeeded = true;
        }
    }
    this->close();
}

void MainWindow::ClearDesktopFromOtherThread()
{
    ClearDesktop();
}

void MainWindow::LoadDesktopFromOtherThread(QString par_Filename)
{
    LoadDesktop(par_Filename);
}

void MainWindow::SaveDesktopFromOtherThread(QString par_Filename)
{
    SaveDesktop(par_Filename);
}

void MainWindow::SchedulerStateChangedSlot(int par_ThreadId, int par_State)
{
    Q_UNUSED(par_State)
    SchedulerStateChangedAck(par_ThreadId);
}

void MainWindow::OpenWindowByNameSlot(int par_ThreadId, char *par_WindowName, bool par_PosFlag, int par_XPos, int par_YPos, bool par_SizeFlag, int par_Width, int par_Hight)
{
    // current selected sheet
    Sheets *Sheet = m_allSheets->value(ui->centralwidget->tabText(ui->centralwidget->currentIndex()));
    QStringList OpenWindowNameList(par_WindowName);
    QPoint Point;
    Point.setX(par_XPos);
    Point.setY(par_YPos);
    QList<MdiWindowWidget*> OpendWidgets = Sheet->OpenWindows(OpenWindowNameList, m_allWindowTypes, m_updateTimers, par_PosFlag, Point, par_SizeFlag, par_Width, par_Hight);
    foreach (MdiWindowWidget* OpenWidget, OpendWidgets) {
        connect(OpenWidget, SIGNAL(openStandardDialog(QStringList,bool,bool,QColor,QFont,bool)), this, SLOT(openElementDialog(QStringList,bool,bool,QColor,QFont,bool)));
    }
    OpenWindowByNameFromOtherThreadAck(par_ThreadId, 0);
}

void MainWindow::IsWindowOpenSlot(int par_ThreadId, char *par_WindowName)
{
    QString WindowNameHelp = QString(par_WindowName);
    int Ret = IsWindowOpen (WindowNameHelp, true);
    IsWindowOpenFromOtherThreadAck(par_ThreadId, Ret);
}

void MainWindow::SaveAllConfigToIniDataBaseSlot(int par_ThreadId)
{
    writeSheetsToIni();
    SaveAllConfigToIniDataBaseFromOtherThreadAck(par_ThreadId);
}

void MainWindow::CloseWindowByNameSlot(int par_ThreadId, char *par_WindowName)
{
    this->CloseWindowByName (par_WindowName);
    CloseWindowByNameFromOtherThreadAck(par_ThreadId, 0);
}

void MainWindow::SelectSheetSlot(int par_ThreadId, char *par_SheetName)
{
    QString SheetName = QString(par_SheetName);
    int TabCount = ui->centralwidget->count();
    int Ret = -1;

    if (!SheetName.compare("default")) {
       SheetName = "Default";
    }
    for(int i = 0; i < TabCount; i++) {
        if(ui->centralwidget->tabText(i).compare(SheetName) == 0) {
            Sheets *Sheet1 = m_allSheets->value(ui->centralwidget->tabText(ui->centralwidget->currentIndex()));
            Sheets *Sheet2 = m_allSheets->value(SheetName);
            if (Sheet1 != Sheet2) {
                // If the last sheet should not be the selected one
                ui->centralwidget->setCurrentIndex(i);
                d_comboBoxSheets->setCurrentIndex(i);
                // Sheet have to be activate (to fetch back all sub windows from the other sheets)
                Sheet2->Activating();
            }
            Ret = 0;
            break;
        }
    }
    SelectSheetFromOtherThreadAck(par_ThreadId, Ret);
}

void MainWindow::AddSheetSlot(int par_ThreadId, char *par_SheetName)
{
    QString SheetName = QString(par_SheetName);

    if (!SheetName.compare("default")) {
       SheetName = "Default";
    }
    int Ret = addNewTab(QString(SheetName), "", true);
    AddSheetFromOtherThreadAck(par_ThreadId, Ret);
}

void MainWindow::DeleteSheetSlot(int par_ThreadId, char *par_SheetName)
{
    QString SheetName = QString(par_SheetName);
    int TabCount = ui->centralwidget->count();
    int Ret = -1;

    for(int i = 0; i < TabCount; i++) {
        if(ui->centralwidget->tabText(i).compare(SheetName) == 0) {
            Sheets *Sheet1 = m_allSheets->value(ui->centralwidget->tabText(ui->centralwidget->currentIndex()));
            Sheets *Sheet2 = m_allSheets->value(SheetName);
            if (Sheet1 != Sheet2) {
                closeTab(i, false);
            }
            Ret = 0;
            break;
        }
    }
    DeleteSheetFromOtherThreadAck(par_ThreadId, Ret);
}

void MainWindow::RenameSheetSlot(int par_ThreadId, char *par_OldSheetName, char *par_NewSheetName)
{
    QString SheetName = QString(par_OldSheetName);
    int TabCount = ui->centralwidget->count();
    int Ret = -1;

    if (!SheetName.compare("default")) {
       SheetName = "Default";
    }
    for(int i = 1; i < TabCount; i++) {  // The first sheet named always "default" and cannot be renamed
        if(ui->centralwidget->tabText(i).compare(SheetName) == 0) {
            Sheets *Sheet1 = m_allSheets->value(ui->centralwidget->tabText(ui->centralwidget->currentIndex()));
            Sheets *Sheet2 = m_allSheets->value(SheetName);
            if (Sheet1 == Sheet2) {
                Ret = renameTab(i, QString(par_OldSheetName), QString(par_NewSheetName));
            }
            break;
        }
    }
    DeleteSheetFromOtherThreadAck(par_ThreadId, Ret);
}

void MainWindow::ScriptCreateDialogSlot(int par_ThreadId, QString par_Headline)
{
    int Ret = 0;
    if (m_ScriptUserDialog == nullptr) {
        m_ScriptUserDialog = new ScriptUserDialog(par_Headline);
        if (m_ScriptUserDialog == nullptr) {
            Ret = -1;
        }
    } else {
        m_ScriptUserDialog->ReInit(par_Headline);
    }
    ScriptCreateDialogFromOtherThreadAck(par_ThreadId, Ret);
}

void MainWindow::ScriptAddDialogItemSlot(int par_ThreadId, QString par_Element, QString par_Label)
{
    int Ret = 0;
    if (m_ScriptUserDialog != nullptr) {
        Ret = m_ScriptUserDialog->AddItem(par_Element, par_Label);
    } else Ret = -1;
    ScriptAddDialogItemFromOtherThreadAck(par_ThreadId, Ret);
}

void MainWindow::ScriptShowDialogSlot(int par_ThreadId)
{
    int Ret = 0;
    if (m_ScriptUserDialog != nullptr) {
        Ret = m_ScriptUserDialog->Show();
    } else Ret = -1;
    ScriptShowDialogFromOtherThreadAck(par_ThreadId, Ret);
}

void MainWindow::ScriptDestroyDialogSlot(int par_ThreadId)
{
    int Ret = 0;
    if (m_ScriptUserDialog != nullptr) {
        delete (m_ScriptUserDialog);
        m_ScriptUserDialog = nullptr;
    } else Ret = -1;
    ScriptDestroyDialogFromOtherThreadAck(par_ThreadId, Ret);
}

void MainWindow::IsScriptDialogClosedSlot(int par_ThreadId)
{
    int Ret = 0;
    if (m_ScriptUserDialog == nullptr) {
        Ret = 1;
    } else {
        if (m_ScriptUserDialog->IsClosed()) {
            Ret = 1;
            delete (m_ScriptUserDialog);
            m_ScriptUserDialog = nullptr;
        }
    }
    IsScriptDialogClosedFromOtherThreadAck(par_ThreadId, Ret);
}

void MainWindow::StopOscilloscopeSlot(void *par_OscilloscopeWidget, uint64_t par_Time)
{
    static_cast<OscilloscopeWidget*>(par_OscilloscopeWidget)->GoOffline(true, par_Time);
}

// From rpc_socket_server.h TODO include the header
extern "C" int RemoteProcedureDecodeMessage(void *par_Connection, const void *par_In, void *ret_Out);

void MainWindow::RemoteProcedureCallRequestSlot(int par_ThreadId, void *par_Connection, const void *par_In, void *ret_Out)
{
    int Ret = RemoteProcedureDecodeMessage(par_Connection, par_In, ret_Out);
    RemoteProcedureCallRequestFromOtherThreaddAck(par_ThreadId, Ret);
}


void MainWindow::ImportHotkeySlot(int par_ThreadId, QString Filename)
{
    int Ret;

    int Fd = ScQt_IniFileDataBaseOpen(Filename);
    if (Fd <= 0) {
        Ret = -1;
    } else {
        QString Section("GUI/FunctionHotkeys");
        Ret = ScQt_IniFileDataBaseCopySectionSameName(GetMainFileDescriptor(), Fd, Section);
    }
    ScQt_IniFileDataBaseClose(Fd);

    delete hotkeyHandler;
    hotkeyHandler = new cHotkeyHandler;
    hotkeyHandler->register_hotkeys(this);

    ImportHotkeyFromOtherThreadAck(par_ThreadId, Ret);
}

void MainWindow::ExportHotkeySlot(int par_ThreadId, QString Filename)
{
    int Ret;

    int Fd = ScQt_IniFileDataBaseOpen(Filename);
    if (Fd <= 0) {
        Fd = ScQt_IniFileDataBaseCreateAndOpenNewIniFile(Filename);
    }
    if (Fd > 0) {
        QString Section("GUI/FunctionHotkeys");
        Ret = ScQt_IniFileDataBaseCopySectionSameName(Fd, GetMainFileDescriptor(), Section);
        ScQt_IniFileDataBaseSave(Fd, Filename, 2);
        ExportHotkeyFromOtherThreadAck(par_ThreadId, Ret);
    }
}

void MainWindow::RePositioningProcessBars()
{
    int x = width();
    int y = height() - 32;

    QMapIterator<int, ProgressBar*> i(m_progressBars);
    while (i.hasNext()) {
        i.next();
        ProgressBar *bar = i.value();
        x -= bar->width();
        bar->move(x, y);
    }
}

void MainWindow::AddWindowType(MdiWindowType *arg_WindowType)
{
    ui->menuNew->addAction(arg_WindowType->GetIcon(), arg_WindowType->GetMenuEntryName(), this, SLOT(newElement()));
    ui->menuOpen->addAction(arg_WindowType->GetIcon(), arg_WindowType->GetMenuEntryName(), this, SLOT(openElement()));
    m_allWindowTypes->insert(arg_WindowType->GetMenuEntryName(), arg_WindowType);
}


void MainWindow::OpenProgressBarSlot(int par_Id, int par_ThreadId, QString par_ProgressName)
{
    Q_UNUSED(par_ThreadId);
    ProgressBar *loc_progress = new ProgressBar(par_Id, par_ProgressName, this);
    m_progressBars.insert(loc_progress->getProgressID(), loc_progress);

    RePositioningProcessBars();

    loc_progress->show();
}

void MainWindow::SetProgressBarSlot(int par_ThreadId, int par_ProgressBarID, int par_Value, bool par_CalledFromGuiThread)
{
    Q_UNUSED(par_ThreadId);
    if (m_progressBars.contains(par_ProgressBarID)) {
        ProgressBar *bar = m_progressBars.value(par_ProgressBarID);
        if (bar != nullptr) {
            bar->valueChanged(par_Value, par_ProgressBarID, par_CalledFromGuiThread);
        }
    }
}

void MainWindow::CloseProgressBarSlot(int par_ThreadId, int par_ProgressBarID)
{
    Q_UNUSED(par_ThreadId);
    if (m_progressBars.contains(par_ProgressBarID)) {
        m_progressBars.value(par_ProgressBarID)->close();
        m_progressBars.value(par_ProgressBarID)->~ProgressBar();  // call destructor
        m_progressBars.remove(par_ProgressBarID);
    }
}

void MainWindow::OpenOpenWindowMenuAt(const QPoint &par_Point)
{
    QMenu ContextMenu(this);

    QList<QMenu*> Menus = menuBar()->findChildren<QMenu*>();
    foreach (QMenu* Menu, Menus) {
        if (!Menu->title().compare("&Display")) {
            Menus = Menu->findChildren<QMenu*>();  // Sub menues
            foreach (QMenu* Menu, Menus) {
                QString Name = Menu->title();
                if (!Name.compare("New")) {
                    QMenu *ContexOpenMenu = ContextMenu.addMenu("New");
                    QList<QAction*> NewActions = Menu->actions();  // Sub menues
                    foreach (QAction *ActionMember, NewActions) {
                        QString Name = ActionMember->text();
                        MdiWindowType* Window = m_allWindowTypes->value(Name);
                        ContexOpenMenu->addAction(Window->GetIcon(), Window->GetMenuEntryName(), this, SLOT(newElement()));
                    }
                } else if (!Name.compare("Open")) {
                    QMenu *ContexNewMenu = ContextMenu.addMenu("Open");
                    QList<QAction*> OpenActions = Menu->actions();  // Sub-Menues
                    foreach (QAction *ActionMember, OpenActions) {
                        QString Name = ActionMember->text();
                        MdiWindowType* Window = m_allWindowTypes->value(Name);
                        ContexNewMenu->addAction(Window->GetIcon(), Window->GetMenuEntryName(), this, SLOT(openElement()));
                    }
                }
            }
        }
    }
    m_OpenWindowPosValid = true;
    m_OpenWindowPos = par_Point;

    ContextMenu.exec(par_Point);

    m_OpenWindowPosValid = false;
}

void MainWindow::SetNotFasterThanRealtimeStateSlot(int par_State)
{
    m_DoNotChangeNotFasterThanRealtime = true;
    ui->actionRealtime->setChecked(par_State);
    m_DoNotChangeNotFasterThanRealtime = false;
}

void MainWindow::SetSuppressDisplayNonExistValuesStateSlot(int par_State)
{
    ui->actionSuppress_display_non_existing_values->setChecked(par_State);
}

void MainWindow::DeactivateBreakpoinSlot()
{
    d_controlPanel->DeactivateBreakpoint();
}


void MainWindow::AddNewScriptErrorMessageSlot(int par_ThreadId, int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message)
{
    ScriptErrorDialog::ScriptErrorMsgDlgAddMsgGuiThread(par_Level, par_LineNr, par_Filename, par_Message, this);
    AddNewScriptErrorMessageFromOtherThreadAck(par_ThreadId);
}

void MainWindow::CheckTerminatingSlot()
{
    if (CheckIfAllSchedulerAreTerminated()) {  // If all scheduler are terminated send the close event again
        this->close();
    } else {
        if (CheckTerminateAllSchedulerTimeout ()) {
            char *ProcessNames;
            switch (ThrowError (ERROR_OKCANCEL, "cannot terminate all schedulers press OK to terminate %s anyway or cancel to continue waiting\n"
                                "following processe are not stopped:\n%s",
                                GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME), ProcessNames = GetProcessInsideExecutionFunctionSemicolonSeparated())) {
            case IDOK:
                this->close();
                break;
            default:
            case IDCANCEL:
                SetTerminateAllSchedulerTimeout(s_main_ini_val.TerminateSchedulerExternProcessTimeout);
                break;
            }
            if (ProcessNames != nullptr) my_free(ProcessNames);
        }
    }
}

void MainWindow::ClearDesktop()
{
    this->closeAllSheets();
    clear_desktop_ini();
    this->ReadAllSheetsFromIni();
}

void MainWindow::LoadDesktop(QString par_Filename)
{
    this->closeAllSheets();
    clear_desktop_ini();
    load_desktop_file_ini(QStringToConstChar(par_Filename));
    this->ReadAllSheetsFromIni();
}

void MainWindow::SaveDesktop(QString par_Filename)
{
    writeSheetsToIni();
    save_desktop_file_ini(QStringToConstChar(par_Filename));
}

void MainWindow::UpdateWindowTitle()
{
    QString Title = QString(GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME));
    char IniFileName[MAX_PATH];
    IniFileDataBaseGetFileNameByDescriptor(GetMainFileDescriptor(), IniFileName, sizeof(IniFileName));
    if (strlen (s_main_ini_val.InstanceName)) {
        Title.append(QString(" [%1]").arg(s_main_ini_val.InstanceName).append (QString(" \"%1\"").arg(IniFileName)));
    } else {
        Title.append(QString(" \"%1\"").arg(IniFileName));
    }
    if (s_main_ini_val.RunningInsideAJob) {
        Title.append("    !running inside a job!");
    }
    this->setWindowTitle(Title);
}

// Pointer to the main window
MainWindow *MainWindow::PointerToMainWindow;

bool MainWindow::MainWindowIsOpen;


MainWindow::MainWindow(QSize par_NormalSize, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    m_ExitCodeSeeded = false;
    m_ExitCode = 0;

    m_DoNotChangeNotFasterThanRealtime = false;

    m_ScriptUserDialog = nullptr;

    PointerToMainWindow = this;

    m_OpenWindowPosValid = false;

    m_NormalSize = par_NormalSize;

    connect (&SignalsFromOtherThreadInst, SIGNAL(CloseXilEnvSignal(int, int)), this, SLOT(closeFromOtherThread(int, int)));
    connect (&SignalsFromOtherThreadInst, SIGNAL(ClearDesktopFromOtherThreadSignal()), this, SLOT(ClearDesktopFromOtherThread()));
    connect (&SignalsFromOtherThreadInst, SIGNAL(LoadDesktopFromOtherThreadSignal(QString)), this, SLOT(LoadDesktopFromOtherThread(QString)));
    connect (&SignalsFromOtherThreadInst, SIGNAL(SaveDesktopFromOtherThreadSignal(QString)), this, SLOT(SaveDesktopFromOtherThread(QString)));

    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(SchedulerStateChangedSignal(int,int)), this, SLOT(SchedulerStateChangedSlot(int,int)));

    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(OpenWindowByNameSignal(int,char*,bool,int,int,bool,int,int)), this, SLOT(OpenWindowByNameSlot(int,char*,bool,int,int,bool,int,int)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(IsWindowOpenSignal(int,char*)), this, SLOT(IsWindowOpenSlot(int,char*)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(SaveAllConfigToIniDataBaseSignal(int)), this, SLOT(SaveAllConfigToIniDataBaseSlot(int)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(CloseWindowByNameSignal(int,char*)), this, SLOT(CloseWindowByNameSlot(int,char*)));

    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(SelectSheetSignal(int,char*)), this, SLOT(SelectSheetSlot(int,char*)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(AddSheetSignal(int,char*)), this, SLOT(AddSheetSlot(int,char*)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(DeleteSheetSignal(int,char*)), this, SLOT(DeleteSheetSlot(int,char*)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(RenameSheetSignal(int,char*,char*)), this, SLOT(RenameSheetSlot(int,char*,char*)));

    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(AddNewScriptErrorMessageSignal(int,int,int,const char*,const char*)), this, SLOT(AddNewScriptErrorMessageSlot(int,int,int,const char*,const char*)));

    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(ImportHotkeySignal(int,QString)), this, SLOT(ImportHotkeySlot(int,QString)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(ExportHotkeySignal(int,QString)), this, SLOT(ExportHotkeySlot(int,QString)));

    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(OpenProgressBarSignal(int,int,QString)), this, SLOT(OpenProgressBarSlot(int,int,QString)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(SetProgressBarSignal(int,int,int,bool)), this, SLOT(SetProgressBarSlot(int,int,int,bool)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(CloseProgressBarSignal(int,int)), this, SLOT(CloseProgressBarSlot(int,int)));

    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(SetNotFasterThanRealtimeStateSignal(int)), this, SLOT(SetNotFasterThanRealtimeStateSlot(int)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(SetSuppressDisplayNonExistValuesStateSignal(int)), this, SLOT(SetSuppressDisplayNonExistValuesStateSlot(int)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(DeactivateBreakpointSignal()), this, SLOT(DeactivateBreakpoinSlot()));

    // Script-Commands: CREATE_DLG/ADD_DLG_ITEM/SHOW_DLG
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(ScriptCreateDialogSignal(int,QString)), this, SLOT(ScriptCreateDialogSlot(int,QString)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(ScriptAddDialogItemSignal(int,QString,QString)), this, SLOT(ScriptAddDialogItemSlot(int,QString,QString)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(ScriptShowDialogSignal(int)), this, SLOT(ScriptShowDialogSlot(int)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(ScriptDestroyDialogSignal(int)), this, SLOT(ScriptDestroyDialogSlot(int)));
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(IsScriptDialogClosedSignal(int)), this, SLOT(IsScriptDialogClosedSlot(int)));

    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(StopOscilloscopeSignal(void*,uint64_t)), this, SLOT(StopOscilloscopeSlot(void*,uint64_t)));

    // Socket based remote API
    connect (&MainWinowSyncWithOtherThreadsInstance, SIGNAL(RemoteProcedureCallRequestSignal(int, void*, const void*, void *)), this, SLOT(RemoteProcedureCallRequestSlot(int, void*, const void*, void *)));

    ui->setupUi(this);
    UpdateWindowTitle();

    populateToolbar();

    m_allWindowTypes = new QMap<QString, MdiWindowType* >();
    m_allSheets = new QMap<QString, Sheets* >();

    // load all display elemente and existing plugins
    loadAllPlugins();

    // This Map will used for detached display windows which are moved
    // between  MdiSubWindow and the Widget. So the MdiSubWindow will
    // be shown at the same place it was detached before
    d_widgetSubMap = new QMap<QWidget*, MdiSubWindow*>;

    m_updateTimers = new WindowUpdateTimers(this);

    // Always build the control panel (even it is not shown)
    d_controlPanel = new ControlPanel(this);

    if (!s_main_ini_val.HideControlPanel) {
        d_controlPanel->show();
    }

    InitMessageWindow (d_controlPanel, this);

    // Status line
    this->setStyleSheet("QStatusBar::item { border: 0px solid black}; ");

    QTimer *loc_StatusBarTimer = new QTimer(this);
    loc_StatusBarTimer->start(100);
    connect(loc_StatusBarTimer, SIGNAL(timeout()), ui->statusbar, SLOT(MoveCar()));

    foreach(QString loc_styleName, QStyleFactory::keys()) {
        QAction *loc_action = ui->menuFenster_Style->addAction(loc_styleName);
        connect(loc_action, SIGNAL(triggered()), this, SLOT(styleChange()));
    }

    // Open all sheets and windows which are stored inside the INI file
    // and should be displayed
    ReadAllSheetsFromIni(); // NOTE: all plugin must be loaded before

    // the status bar has a slow update rate
    connect(m_updateTimers->updateTimer(InterfaceWidgetPlugin::SlowUpdateWindow), SIGNAL(timeout()), this, SLOT(setStatusBarTime()));
    connect(ui->centralwidget, SIGNAL(currentChanged(int)), this, SLOT(changeComboBoxFromTab(int)));

    // Control panel has a slow update rate
    connect(m_updateTimers->updateTimer(InterfaceWidgetPlugin::SlowUpdateWindow), SIGNAL(timeout()), d_controlPanel, SLOT(CyclicUpdate()));

    // Connect QAction object signals with existing slots
    connect(ui->actionTile, SIGNAL(triggered()), this, SLOT(tileSubWindows()));
    connect(ui->actionCascade, SIGNAL(triggered()), this, SLOT(cascadeSubWindows()));

    // Connect control panel signals with QAction object
    connect(d_controlPanel, SIGNAL(hideControlPanel()), this, SLOT(toolbarControlPanelButtonDeaktivate()));
    connect(d_controlPanel, SIGNAL(runControlContinue()), this, SLOT(toolbarControlContinue()));
    connect(d_controlPanel, SIGNAL(runControlStop()), this, SLOT(toolbarControlStop()));
    connect(d_controlPanel, SIGNAL(openAddBBDialog()), this, SLOT(openUserDefinedBBVarsDialog()));

    // Read hotkeys from ini-file
    hotkeyHandler = new cHotkeyHandler();
    hotkeyHandler->register_hotkeys(this);

    // Some GUI hotkeys
    ui->menuDatei->setTitle("&File");                   //Alt + f
    ui->menuAnsicht->setTitle("&Display");              //Alt + d
    ui->menuSettings->setTitle("&Settings");            //Alt + s
    ui->menuWindow->setTitle("&Window");                //Alt + w
    ui->menuCalibration->setTitle("C&alibration");      //Alt + a
    ui->menuCAN->setTitle("&CAN");                      //Alt + c
    ui->menuAbout->setTitle("A&bout");                  //Alt + b

    QShortcut *shortcut = new QShortcut(QKeySequence::AddTab, this);
    connect (shortcut, SIGNAL(activated()), this, SLOT(addNewTab()));

    m_blackboardModel = new BlackboardVariableModel;

    m_blackboardFilterModel = new QSortFilterProxyModel;
    m_blackboardFilterModel->setSourceModel(m_blackboardModel);
    m_blackboardFilterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_blackboardFilterModel->sort(0);
    if(s_main_ini_val.NoCaseSensitiveFilters) {
        m_blackboardFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    } else {
        m_blackboardFilterModel->setFilterCaseSensitivity(Qt::CaseSensitive);
    }
    ui->listView->setModel(m_blackboardFilterModel);

    ui->listView->setDragEnabled(true);
    ui->actionBlackboard_List->setChecked(false);
    showBlackboardList(false);
    connect(ui->lineEdit, SIGNAL(textChanged(QString)), m_blackboardFilterModel, SLOT(setFilterFixedString(QString)));
}

void MainWindow::populateToolbar()
{
    if (!s_main_ini_val.ConnectToRemoteMaster) {
        d_labelRtFactor = new QLabel("RT-Factor:\nunknown", this);
    }
    d_comboBoxSheets = new QComboBox(this);
    ui->toolBar->addAction(ui->actionSave_INI);
    ui->toolBar->addSeparator();
    if (!s_main_ini_val.ConnectToRemoteMaster) {
        ui->toolBar->addAction(ui->actionContinue);
        ui->toolBar->addAction(ui->actionStop);
        ui->toolBar->addAction(ui->actionNextOne);
        ui->toolBar->addSeparator();
        ui->toolBar->addAction(ui->actionRealtime);
        ui->toolBar->addWidget(d_labelRtFactor);
        ui->toolBar->addSeparator();
    }
    ui->toolBar->addAction(ui->actionSuppress_display_non_existing_values);
    ui->toolBar->addAction(ui->actionControlPnl);
    ui->toolBar->addAction(ui->actionmove_control_panel_to_top_left_position);
    ui->toolBar->addAction(ui->actionBlackboard_List);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->actionEditDebug);
    ui->toolBar->addAction(ui->actionEditError);
    ui->toolBar->addAction(ui->actionEditMessage);
    ui->toolBar->addAction(ui->actionHTMLreport);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->actionAbout);
    ui->toolBar->addSeparator();
    ui->toolBar->addWidget(d_comboBoxSheets);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(ui->actionAddSheet);
    ui->toolBar->addAction(ui->actionRemoveSheet);

    ui->actionContinue->setEnabled(false);
    ui->actionNextOne->setEnabled(false);
    ui->actionControlPnl->setChecked(!s_main_ini_val.HideControlPanel);
    ui->actionRealtime->setChecked(s_main_ini_val.NotFasterThanRealTime);

    // Connect ComboBox signal and the QAction object signals with the corresponding slot method
    connect(d_comboBoxSheets, SIGNAL(currentIndexChanged(int)), this, SLOT(changeTabFromComboBox(int)));
    connect(ui->actionRealtime, SIGNAL(toggled(bool)), this, SLOT(toolbarNotFasterThanRealtime(bool)));
    connect(ui->actionSuppress_display_non_existing_values, SIGNAL(toggled(bool)), this, SLOT(toolbarSuppressNonExistValues(bool)));
    connect(ui->actionControlPnl, SIGNAL(toggled(bool)), this, SLOT(toolbarControlPanelShow(bool)));
    connect(ui->actionmove_control_panel_to_top_left_position, SIGNAL(triggered()), this, SLOT(toolbarControlPanelMoveToTopLeftCorner()));
    connect(ui->actionContinue, SIGNAL(triggered()),this, SLOT(toolbarContinueScheduler()));
    connect(ui->actionStop, SIGNAL(triggered()), this, SLOT(toolbarStopScheduler()));
    connect(ui->actionNextOne, SIGNAL(triggered()), this, SLOT(toolbarNextOneScheduler()));

    connect(ui->actionEditDebug, SIGNAL(triggered()), this, SLOT(toolbarEditDebug()));
    connect(ui->actionEditError, SIGNAL(triggered()), this, SLOT(toolbarEditError()));
    connect(ui->actionEditMessage, SIGNAL(triggered()), this, SLOT(toolbarEditMessage()));
    connect(ui->actionHTMLreport, SIGNAL(triggered()), this, SLOT(toolbarHTMLreport()));
}

void MainWindow::setStatusBarTime()
{
    if(s_main_ini_val.StatusbarCar == 0) {
        if (ui->statusbar->IsEnableCar()) ui->statusbar->SetEnableCar(false);
    } else {
        if (!ui->statusbar->IsEnableCar()) ui->statusbar->SetEnableCar(true);
    }
    if (strlen (s_main_ini_val.SpeedStatusbarCar) == 0) {
        ui->statusbar->SetSpeedPerCent(1.0);
    } else {
        double Steps = direct_solve_equation_no_error_message (s_main_ini_val.SpeedStatusbarCar);
        ui->statusbar->SetSpeedPerCent(Steps);
    }
    if (!s_main_ini_val.ConnectToRemoteMaster) {
        double loc_rtFactor = GetRealtimeFactor();
        QString tmp = "RT-Factor:\n";
        if(loc_rtFactor < 9999.9) {
            tmp.append(QString().setNum(GetRealtimeFactor(),'f',2));
        } else {
            tmp.append("fast");
        }
        d_labelRtFactor->setText(tmp);
    }
    MainWindowIsOpen = true;
}

void MainWindow::addNewTab()
{
    NewSheetDialog d_newSheet;

    d_newSheet.setWindowTitle("New Sheet");

    removeTopHintFlagControlPanel();

    if(d_newSheet.exec() == QDialog::Accepted) {
        addNewTab(d_newSheet.getSheetName(), "", true);
    }

    setTopHintFlagControlPanel();
}

int MainWindow::addNewTab(QString name, QString arg_section, bool par_Selected)
{
    // Sheets need a unique name
    if (m_allSheets->contains(name)) {
        return -1;
    }

    Sheets *loc_Sheet = new Sheets(ui->centralwidget, m_allWindowTypes,
                                   m_updateTimers, arg_section, par_Selected);

    connect(loc_Sheet, SIGNAL(NewOrOpenWindowMenu(const QPoint &)), this, SLOT(OpenOpenWindowMenuAt(const QPoint &)));

    ui->centralwidget->addTab(loc_Sheet, name);
    d_comboBoxSheets->addItem(name);
    m_allSheets->insert(name, loc_Sheet);

    return 0;
}


void MainWindow::closeTab(int index, bool AskFlag)
{
    bool DeleteFlag = false;
    // The "Default" sheet cannot remove
    if(index > 0) {
        if (AskFlag) {
            if (ThrowError (ERROR_OKCANCEL, "Are you sure you want to close the sheet?") == IDOK) {
                DeleteFlag = true;
            }
        } else {
            DeleteFlag = true;
        }
        if (DeleteFlag) {
            d_comboBoxSheets->removeItem(index);
            QString SheetName = ui->centralwidget->tabText(index);
            ui->centralwidget->removeTab(index);
            m_allSheets->remove(SheetName);
        }
    }
}

int MainWindow::renameTab(int index, QString OldName, QString NewName)
{
    if(index > 0) {
        d_comboBoxSheets->removeItem(index);
        d_comboBoxSheets->insertItem(index, NewName);
        ui->centralwidget->setTabText(index, NewName);
        Sheets *Sheet = m_allSheets->value(OldName);
        m_allSheets->remove(OldName);
        m_allSheets->insert(NewName, Sheet);
        return 0;
    }
    return -1;
}

void MainWindow::closeAllTabs()
{
   d_comboBoxSheets->clear();
   ui->centralwidget->clear();

   QMap<QString, Sheets* >::const_iterator i = m_allSheets->constEnd();
   while (i != m_allSheets->constBegin()) {
       --i;
       delete (i.value());
   }
   m_allSheets->clear();
}


void MainWindow::unhook(const QPoint)
{
    QMenu d_menu;
    d_menu.addAction("Floating Widget");
    d_menu.addAction("Close");

    QAction *selectedItem = d_menu.exec(QCursor::pos());

    if(selectedItem) {
         // Move the widget into a ToolWidget tomake it free movable
         // ToolWidgets will not shown as own window inside the task bar
        if(selectedItem->text() == "Floating Widget") {
            QWidget *d_widget = new QWidget(this, Qt::Tool);
            d_widget->setLayout(new QVBoxLayout(d_widget));
            QMdiArea *d_area = dynamic_cast<QMdiArea*>(ui->centralwidget->currentWidget());
            d_widget->setPalette(d_area->currentSubWindow()->widget()->palette());
            d_area->currentSubWindow()->widget()->setParent(d_widget);
            d_widget->layout()->addWidget(d_area->currentSubWindow()->widget());

            d_widgetSubMap->insert(d_widget, dynamic_cast<MdiSubWindow*>(d_area->currentSubWindow()));

            d_widget->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(d_widget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(hook(QPoint)));

            d_widget->setWindowTitle(d_area->currentSubWindow()->windowTitle());
            d_widget->show();
            d_area->currentSubWindow()->hide();
            d_widget->move(QCursor::pos().x() - 20, QCursor::pos().y() - 10);
            QApplication::setActiveWindow(d_widget);
        }

        if(selectedItem->text() == "Close") {
            dynamic_cast<QMdiArea*>(ui->centralwidget->currentWidget())->currentSubWindow()->close();
        }
    }
}

void MainWindow::hook(const QPoint)
{
    QMenu d_menu;
    d_menu.addAction("Docking Widget");
    d_menu.addAction("Close");

    QAction *selectedItem = d_menu.exec(QCursor::pos());

    if(selectedItem) {
        if(selectedItem->text() == "Docking Widget") {
            QWidget *d_widget = QApplication::activeWindow();
            dynamic_cast<QWidget*>(d_widget->children().last())->setParent(d_widgetSubMap->value(d_widget));
            d_widgetSubMap->value(d_widget)->layout()->addWidget(d_widgetSubMap->value(d_widget)->widget());

            d_widgetSubMap->value(d_widget)->showNormal();
            d_widget->close();
        }

        if(selectedItem->text() == "Close") {
            QWidget *d_widget = QApplication::activeWindow();
            d_widgetSubMap->take(d_widget)->close();
            d_widget->close();
        }
    }
}

void MainWindow::closeCurrentTab()
{
    if(ui->centralwidget->currentIndex() > 0) closeTab(ui->centralwidget->currentIndex());
}

void MainWindow::changeTabFromComboBox(int index)
{
    ui->centralwidget->setCurrentIndex(index);
}

void MainWindow::changeComboBoxFromTab(int index)
{
    // Current selected sheet
    Sheets *Sheet = m_allSheets->value(ui->centralwidget->tabText(ui->centralwidget->currentIndex()));
    if (Sheet != nullptr) {
        Sheet->Activating();
    }
    d_comboBoxSheets->setCurrentIndex(index);
}

void MainWindow::closeSubwindows()
{
    QList<QMdiSubWindow*> tmp = dynamic_cast<QMdiArea*>(ui->centralwidget->currentWidget())->subWindowList();
    for(int i = 0; i < tmp.count(); i++)
        tmp.value(i)->close();
}

void MainWindow::hideSubwindows()
{
    QList<QMdiSubWindow*> tmp = dynamic_cast<QMdiArea*>(ui->centralwidget->currentWidget())->subWindowList();
    for(int i = 0; i < tmp.count(); i++)
        tmp.value(i)->showMinimized();
}

void MainWindow::writeSheetsToIni()
{
    // the current selected sheet would be stored into the BasicSettings section inside the INI file
    QString SelectedSheet = ui->centralwidget->tabText(ui->centralwidget->currentIndex());
    IniFileDataBaseWriteString("BasicSettings", "SelectedSheet", QStringToConstChar(SelectedSheet), GetMainFileDescriptor());

    saveAllSheets (SelectedSheet);
}

// The default sheet mybe named "Default" or "default"
static int CompareSheetNames(char *SheetName1, char *SheetName2)
{
    if (((*SheetName1 == 'D') || (*SheetName1 == 'd')) &&
        ((*SheetName2 == 'D') || (*SheetName2 == 'd'))) {
        return strcmp (SheetName1+1, SheetName2+1);
    } else {
        return strcmp (SheetName1, SheetName2);
    }
}

void MainWindow::ReadAllSheetsFromIni()
{
    char section[INI_MAX_SECTION_LENGTH];
    char SelectedSheet[INI_MAX_LINE_LENGTH];
    bool SelectedSheetAreDefined;

    // the current selected sheet would be stored into the BasicSettings section inside the INI file
    if (IniFileDataBaseReadString("BasicSettings", "SelectedSheet", "", SelectedSheet, sizeof(SelectedSheet), GetMainFileDescriptor()) <= 0) {
        strcpy (SelectedSheet, "Default");
        SelectedSheetAreDefined = false;
    } else {
        SelectedSheetAreDefined = true;
    }

    char loc_Sheet[INI_MAX_LINE_LENGTH];
    for (int x = 0; ; x++) {
        sprintf(section, "GUI/OpenWindowsForSheet%i", x);
        if(x == 0) {
            // The first sheet named always Default
            StringCopyMaxCharTruncate (loc_Sheet, "Default", sizeof(loc_Sheet));
        } else {
            if (IniFileDataBaseReadString(section, "SheetName", "", loc_Sheet, sizeof(loc_Sheet), GetMainFileDescriptor()) == 0) {
                break;
            }
        }
        addNewTab(QString(loc_Sheet), section, !CompareSheetNames (SelectedSheet, loc_Sheet));
    }

    // If not the first "Default" sheet should be active
    QString showTab = QString(SelectedSheet);
    int tabs = ui->centralwidget->count();

    for(int i = 0; i < tabs; i++) {
        if(ui->centralwidget->tabText(i).compare(showTab) == 0) {
            Sheets *SheetShouldBeSelected = m_allSheets->value(QString(SelectedSheet));
            if ((SheetShouldBeSelected != nullptr)) {
                // If not the last one should be selected
                ui->centralwidget->setCurrentIndex(i);
                d_comboBoxSheets->setCurrentIndex(i);
                // Sheet have to be activate (to fetch back all sub windows from the other sheets)
                SheetShouldBeSelected->Activating();
            }
            break;
        }
    }
}

void MainWindow::ReadOneSheetFromIni(QString &par_Sheet, QString &par_ImportSection)
{
    addNewTab(par_Sheet, par_ImportSection, false);
}

bool MainWindow::IsMainWindowOpen()
{
    return MainWindowIsOpen;
}

MainWindow *MainWindow::GetPtrToMainWindow()
{
    return PointerToMainWindow;
}

BlackboardVariableModel *MainWindow::GetBlackboardVariableModel()
{
    if (PointerToMainWindow != nullptr) {
        PointerToMainWindow->m_blackboardModel->change_connection_to_observer(true);  // safekeeping the connection to the observer
        return PointerToMainWindow->m_blackboardModel;
    }
    return nullptr;
}

void MainWindow::closeAllSheets ()
{
    writeSheetsToIni();
    closeAllTabs();
}

void MainWindow::saveAllSheets (QString &par_SelectedSheet)
{

    // Fist delete all
    Sheets::CleanIni();
    foreach(QString loc_key, m_allSheets->keys()) {
        bool isSelectedSheet = par_SelectedSheet.compare(loc_key) == 0;
        m_allSheets->value(loc_key)->writeToIni(isSelectedSheet);
    }
}

void MainWindow::ExitWithoutSaveIni()
{
    if (s_main_ini_val.AskSaveIniAtExit == 1) {
        char IniFileName[MAX_PATH];
        IniFileDataBaseGetFileNameByDescriptor(GetMainFileDescriptor(), IniFileName, sizeof(IniFileName));
        if (ThrowError (ERROR_OKCANCEL, "Automatic save of \"%s\" is switched off. Quit %s anyway?",
                        IniFileName, GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_PROGRAM_NAME)) == IDOK) {
            this->close();
        }
    } else {
        s_main_ini_val.SwitchAutomaticSaveIniOff = 1;
        this->close();
    }
}

void MainWindow::BarrierLogging()
{
    BarrierHistoryLoggingDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::ExternProcessCopyLists()
{
    ExternProcessCopyListDialog Dlg(QString(""));
    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}


int MainWindow::IsWindowOpen (QString &par_WindowName, bool par_OnlyActiveSheet)
{
    Q_UNUSED(par_OnlyActiveSheet)
    // Current selected sheet
    Sheets *Sheet = m_allSheets->value(ui->centralwidget->tabText(ui->centralwidget->currentIndex()));
    if (Sheet->WindowNameAlreadyInUse (par_WindowName)) {
        return 1;
    }
    return 0;
}

int MainWindow::IsWindowOpen (char *par_WindowName)
{
    QString aString;
    aString = par_WindowName;
    return WindowNameAlreadyInUse(aString);
}

QString MainWindow::GetScriptFilenameFromControlPannel()
{
    return d_controlPanel->GetActualScriptFilename();
}

QStringList MainWindow::GetSheetsInsideWindowIsOpen(QString par_WindowName)
{
    QStringList Ret;
    foreach(QString loc_key, m_allSheets->keys()) {
        Sheets *Sheet = m_allSheets->value(loc_key);
        if (Sheet->WindowNameAlreadyInUse (par_WindowName)) {
            Ret.append(Sheet->GetName());
        }
    }
    return Ret;
}

void MainWindow::SelectSheetAndWindow(QString par_SheetName, QString par_WindowName)
{
    foreach(QString loc_key, m_allSheets->keys()) {
        Sheets *Sheet = m_allSheets->value(loc_key);
        if (!par_SheetName.compare (Sheet->GetName())) {
            ui->centralwidget->setCurrentWidget(Sheet);
            Sheet->Activating();
            Sheet->SelectWindow(par_WindowName);
        }
    }
}

QList<Sheets *> MainWindow::GetAllSheets()
{
    QList<Sheets*> Sheet;
    foreach(QString loc_key, m_allSheets->keys())
    {
        Sheet.append(m_allSheets->value(loc_key));
    }
    return Sheet;
}

QMap<QString, Sheets *> *MainWindow::GetRefToAllSheets()
{
    return m_allSheets;
}

QString MainWindow::GetCurrentSheet()
{
    return ui->centralwidget->tabText(ui->centralwidget->currentIndex());
}


// If XilEnv would be terminate all open sheets and there open windows
// would be written to the INI file
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (RequestForTermination () == 1) {
        writeToIni();
        closeAllSheets();
        CloseMessageWindow();
        TerminateExtErrorMessageWindow();
        MainWindowIsOpen = false;
        QMainWindow::closeEvent(event);
    } else {
        // Connect the waiting for termination of the schedulers with a timer
        connect(m_updateTimers->updateTimer(InterfaceWidgetPlugin::MediumUpdateWindow), SIGNAL(timeout()), this, SLOT(CheckTerminatingSlot()));

        event->ignore(); // Ignore request for termination of XilEnv
                         // The request will be automatc repeat if all
                         // requirements are fulfil (terminate script, ...)

    }
}

void MainWindow::makeFullScreen(bool full)
{
    if(full) {
        this->showFullScreen();
    } else {
        this->showNormal();
    }
}

void MainWindow::openGlobalSectionFilterDialog()
{
    globalsectionfilterdialog Dlg;
    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::openSaveValueDialog()
{
    saveprocessesdialog Dlg;
    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::openWriteSectionToExeDialog()
{
    writesectiontoexedialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openExportVariablePropertiesDialog()
{
    exportvariableproperties Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openImportVariablePropertiesDialog()
{
    importvariableproperties Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openUserDefinedEnvVarsDialog()
{
    UserDefinedEnvironmentVariableDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openUserDefinedBBVarsDialog()
{
    UserDefinedBBVariableDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openCcpConfigDialog()
{
    CCPConfigDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openCcpStartStopDialog()
{
    StartStopCPPDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openCcpStartStopCalibrationDialog()
{
    StartStopCCPCalDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openCcpImportConfigDialog()
{
    LoadCCPConfigDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openCcpExportConfigDialog()
{
    SaveCCPConfigDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openXcpConfigDialog()
{
    XCPConfigDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openXcpStartStopDialog()
{
    StartStopXCPDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openXcpStartStopCalibrationDialog()
{
    StartStopXCPCalDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openXcpImportConfigDialog()
{
    LoadXCPConfigDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openXcpExportConfigDialog()
{
    SaveXCPConfigDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted)
    {
        ;
    }
}

void MainWindow::openCanInsertErrorDialog()
{
    InsertCANErrorDialog *Dlg;
    Dlg = new InsertCANErrorDialog(this);
    Dlg->show();
}

void MainWindow::openExportVariableReferenceListDialog()
{
    ExportReferenceListDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::openImportVariableReferenceListDialog()
{
    importscreflistdialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::openSearchWindowWithVariableDialog()
{
    SearchWindowIncludedVariableDialog Dlg(this);
    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::openLoadSvlDialog()
{
    loadsvlfiledialog Dlg;
    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::openClearDesktopDialog()
{
    ClearDesktop();
}

void MainWindow::openImportSheetDialog()
{
    ImportSheetsDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::openExportSheetDialog()
{
    ExportSheetsDialog Dlg;
    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::openCleanAllBlackboardVariablesDialog()
{
    CleanVariablesSection();
}

void MainWindow::openActiveEquationWindowDialog()
{
    ActiveEquationsDialog::Open (this);
}

void MainWindow::openBasicSettingsDialog()
{
    BasicSettingsDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        if(s_main_ini_val.NoCaseSensitiveFilters) {
            m_blackboardFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        } else {
            m_blackboardFilterModel->setFilterCaseSensitivity(Qt::CaseSensitive);
        }
    }
}

void MainWindow::openSaveDesktopDialog()
{
    QString fileName = FileDialog::getSaveFileName(this, QString ("Save desktop file"), QString(), QString (DSK_EXT));
    if(!fileName.isEmpty()) {
        SaveDesktop(QStringToConstChar(fileName));
    }
}

void MainWindow::openLoadDesktopDialog()
{
    QString fileName = FileDialog::getOpenFileName(this, QString ("Load desktop file"), QString(), QString (DSK_EXT));
    if(!fileName.isEmpty()) {
        LoadDesktop(QStringToConstChar(fileName));
    }
}

void MainWindow::menuClearDesktop()
{
    ClearDesktop();
}

void MainWindow::menuExportA2L()
{
    QString fileName = FileDialog::getSaveFileName(this, QString ("Export A2L to"), QString(), QString (A2L_EXT));
    if(!fileName.isEmpty()) {
        ExportA2lFile(QStringToConstChar(fileName));
    }
}

void MainWindow::menuCleanBBIniSection()
{
    QMessageBox msg;
    msg.setText("Clean BB Vari");
    msg.setInformativeText("delete all not used blackboard variable description out of the current INI file");
    msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);

    if(msg.exec() == QMessageBox::Ok) {
        CleanVariablesSection();
    }
}

void MainWindow::menuExportHotKeys()
{
    QString FileName = FileDialog::getSaveFileName(this, QString ("Export hotkeys to"), QString(), QString (HOTKEY_EXT));
    if (!FileName.isEmpty()) {
        int Fd = IniFileDataBaseOpen(QStringToConstChar(FileName));
        if (Fd <= 0) {
            Fd = IniFileDataBaseCreateAndOpenNewIniFile(QStringToConstChar(FileName));
        }
        if (Fd > 0) {
            IniFileDataBaseCopySectionSameName(Fd, GetMainFileDescriptor(), "GUI/FunctionHotkeys");
            IniFileDataBaseSave(Fd, nullptr, 2);
        }
    }
}

void MainWindow::menuImportHotKeys()
{
    QString FileName = FileDialog::getOpenFileName(this, QString ("Import hotkeys"), QString(), QString (HOTKEY_EXT));
    if(!FileName.isEmpty()) {
        int Fd = IniFileDataBaseOpen(QStringToConstChar(FileName));
        if (Fd <= 0) {
            IniFileDataBaseCopySectionSameName(GetMainFileDescriptor(), Fd, "GUI/FunctionHotkeys");
            IniFileDataBaseClose(Fd);
            delete hotkeyHandler;
            hotkeyHandler = new cHotkeyHandler;
            hotkeyHandler->register_hotkeys(this);
        }
    }
}

void MainWindow::menuFunctionHotKeys()
{
    HotkeysDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        hotkeyHandler->register_hotkeys(this);
    }
}

void MainWindow::openConfigureXCPOverEthernetDialog()
{
    ConfigureXCPOverEthernetDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::openAboutDialog()
{
    AboutDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::openInternalsDialog()
{
    InternalsDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::menuBlackboardInternalInfos()
{
    BlackboardInternalDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::OpenRemoteMasterCallStatisticsDialog()
{
    RemoteMasterCallStatitsticsDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::openLoadedDebugInfosDialog()
{
    LoadedDebugInfosDialog Dlg;

    if (Dlg.exec() == QDialog::Accepted) {
        ;
    }
}

void MainWindow::openCanConfigDialog()
{
    CanConfigDialog *Dlg;

    Dlg = new CanConfigDialog();
    if (Dlg->exec() == QDialog::Accepted) {
        Dlg->SaveToIni();
    }
    delete Dlg;
}

void MainWindow::openCanImportDialog()
{
    QString fileName = FileDialog::getOpenFileName(this, QString ("Import CAN DBC file"), QString(), QString (DBC_EXT));
    if(!fileName.isEmpty())
    {
        DbcImportDialog *dlg = new DbcImportDialog(fileName);
        dlg->exec();
        delete dlg;
    }
}


void MainWindow::menuHomepage()
{
    // TODO
    ThrowError (1, "not implemented");
}


void MainWindow::menuSaveIniAs()
{
    QString DstFileName = FileDialog::getSaveFileName(this, QString ("Save INI file as"), QString(), QString (INI_EXT));
    if(!DstFileName.isEmpty()) {
        if (CreateNewEmptyIniFile(QStringToConstChar(DstFileName))) {
            ThrowError (1, "cannot write to \"%s\" maybe it is write protected", QStringToConstChar(DstFileName));
        } else {
            writeToIni();
            writeSheetsToIni();

            write_process_list2ini ();
            if (s_main_ini_val.ConnectToRemoteMaster) {
                rm_WriteBackVariableSectionCache();
            }

            if (IniFileDataBaseSave(GetMainFileDescriptor(), QStringToConstChar(DstFileName), INIFILE_DATABAE_OPERATION_RENAME)) {   // 1 -> do not delete from INI-DB only rename it!
                ThrowError(1, "cannot write to \"%s\"", QStringToConstChar(DstFileName));
            }
            set_ini_path(QStringToConstChar(DstFileName));
            UpdateWindowTitle();
        }
    }
}

void MainWindow::toolbarNotFasterThanRealtime(bool rtState)
{
    if (!m_DoNotChangeNotFasterThanRealtime) {
        s_main_ini_val.NotFasterThanRealTime = rtState;
    }
}

void MainWindow::toolbarHelp()
{
#ifdef _WIN32
    char *p, Path[MAX_PATH];
    GetModuleFileName (GetModuleHandle(nullptr), Path, sizeof (Path));
    p = Path;
    while (*p != 0) p++;
    while ((p > Path) && (*p != '\\')) p--;
    p++;
    *p = 0;
    StringCopyMaxCharTruncate (p, "OpenXiL_Userguide.pdf", (int)(sizeof(Path) - (p - Path)));
    if (ShellExecute (nullptr, "open", Path, nullptr,
                      nullptr, SW_SHOW) <= reinterpret_cast<HINSTANCE>(32)) {
        ThrowError (1, "cannot open user guide \"%s\"", Path);
    }
#else
    // TODO
    ThrowError (1, "not implemented");
#endif
}

void MainWindow::toolbarContinueScheduler()
{
    enable_scheduler(SCHEDULER_CONTROLED_BY_USER);
}

void MainWindow::toolbarStopScheduler()
{
    disable_scheduler_at_end_of_cycle(SCHEDULER_CONTROLED_BY_USER, nullptr, nullptr);
}

void MainWindow::toolbarNextOneScheduler()
{
    make_n_next_cycles(SCHEDULER_CONTROLED_BY_USER, 1, nullptr, nullptr);
}

void MainWindow::toolbarSuppressNonExistValues(bool existValue)
{
    s_main_ini_val.SuppressDisplayNonExistValues = existValue;
}


bool MainWindow::writeToIni()
{
    int loc_x_main_window;
    int loc_y_main_window;
    int loc_x_control_panel;
    int loc_y_control_panel;
    bool Ret = true;

    QRect MainWindowGeometry = frameGeometry();
    QRect ControlPanelGeometry = d_controlPanel->frameGeometry();
    loc_x_control_panel = ControlPanelGeometry.x();
    loc_y_control_panel = ControlPanelGeometry.y();
    loc_x_main_window = MainWindowGeometry.x();
    loc_y_main_window = MainWindowGeometry.y();
    if (isMaximized()) {
        QPoint Point(x()+width()/2, y()+height()/2);
        QScreen *MainWindowScreen = QGuiApplication::screenAt(Point);
        if (MainWindowScreen != nullptr) {
            QRect MainWindowScreenGeometry = MainWindowScreen->geometry();
            loc_x_main_window = MainWindowScreenGeometry.x();
            loc_y_main_window = MainWindowScreenGeometry.y();
        } else {
            // this can happen if running without screen (cloud, ...)
            //ThrowError (1, "cannot get screen of application");
            Ret = false;
        }
    }
    s_main_ini_val.Maximized = isMaximized();
    // Stoe the main window size if it is not maximized
    s_main_ini_val.MainWindowWidth = m_NormalSize.width();
    s_main_ini_val.MainWindowHeight = m_NormalSize.height();

    // Store the control panel position and visibility
    if (Ret) {
        s_main_ini_val.ControlPanelXPos = loc_x_control_panel - loc_x_main_window;
        s_main_ini_val.ControlPanelYPos = loc_y_control_panel - loc_y_main_window;
    } else {
        s_main_ini_val.ControlPanelXPos = 0;
        s_main_ini_val.ControlPanelYPos = 0;
    }
    WriteBasicConfigurationToIni (&s_main_ini_val);
    return Ret;
}

void MainWindow::toolbarSaveIniFile()
{
    writeToIni();
    writeSheetsToIni();
    write_process_list2ini ();
    if (s_main_ini_val.ConnectToRemoteMaster) {
        rm_WriteBackVariableSectionCache();
    }
    char IniFileName[MAX_PATH];
    IniFileDataBaseGetFileNameByDescriptor(GetMainFileDescriptor(), IniFileName, sizeof(IniFileName));
    if (IniFileDataBaseSave(GetMainFileDescriptor(), nullptr, 0)) {   // 0 -> do not delete from INI-DB!
        ThrowError (1, "cannot save \"%s\" maybe it is write protected", IniFileName);
    }
}

void MainWindow::toolbarControlPanelShow(bool showPanel)
{
    d_controlPanel->setVisible(showPanel);
    showPanel ? s_main_ini_val.HideControlPanel = 0 : s_main_ini_val.HideControlPanel = 1;
}

void MainWindow::toolbarControlPanelButtonDeaktivate()
{
    ui->actionControlPnl->setChecked(false);
}

void MainWindow::toolbarControlPanelMoveToTopLeftCorner()
{
    QPoint globalCursorPos = QCursor::pos();
    d_controlPanel->move(globalCursorPos);
    toolbarControlPanelShow(true);
}

void MainWindow::toolbarControlContinue()
{
    ui->actionContinue->setEnabled(false);
    ui->actionNextOne->setEnabled(false);
    ui->actionStop->setEnabled(true);
}

void MainWindow::toolbarControlStop()
{
    ui->actionContinue->setEnabled(true);
    ui->actionNextOne->setEnabled(true);
    ui->actionStop->setEnabled(false);
}



int LaunchEditor(const char *szFilename)
{
#ifdef _WIN32
    char exec_string[MAX_PATH];
    char TempString[MAX_PATH];

    if (!strlen(s_main_ini_val.Editor)) {
        ThrowError (1, "no Editor selected");
        return -1;
    }
    SearchAndReplaceEnvironmentStrings (s_main_ini_val.Editor,
                                        TempString,
                                        sizeof(s_main_ini_val.Editor));

    sprintf(exec_string, "%s %s", TempString, szFilename);
    if (WinExec(exec_string, SW_NORMAL) < 32) {
        ThrowError (1, "Can't start Editor '%s' !!! Please check entry in Basic Settings !", TempString);
        return -1;
    }
    return 0;
#else
    Q_UNUSED(szFilename);
    // TODO
    ThrowError (1, "not implemented");
    return 0;
#endif
}


void MainWindow::toolbarEditDebug()
{
#ifdef _WIN32
    char Path[MAX_PATH];
    sprintf (Path, "%s\\%sscript.dbg", s_main_ini_val.StartDirectory, s_main_ini_val.ScriptOutputFilenamesPrefix);
    SearchAndReplaceEnvironmentStrings(Path, Path, sizeof(Path));
    if (access(Path, 0) == 0) {
        LaunchEditor(Path);
    } else {
        ThrowError (1, "cannot open script debug file \"%s\"", Path);
    }
#else
    // TODO
    ThrowError (1, "not implemented");
#endif
}

void MainWindow::toolbarEditError()
{
    char Path[2*MAX_PATH+16];
    sprintf (Path, "%s\\%sscript.err", s_main_ini_val.StartDirectory, s_main_ini_val.ScriptOutputFilenamesPrefix);
    SearchAndReplaceEnvironmentStrings(Path, Path, sizeof(Path));
    if (access(Path, 0) == 0) {
        LaunchEditor(Path);
    } else {
        ThrowError (1, "cannot open script error file \"%s\"", Path);
    }
}

void MainWindow::toolbarEditMessage()
{
    char Path[2*MAX_PATH+16];
    sprintf (Path, "%s\\%sscript.msg", s_main_ini_val.StartDirectory, s_main_ini_val.ScriptOutputFilenamesPrefix);
    SearchAndReplaceEnvironmentStrings(Path, Path, sizeof(Path));
    if (access(Path, 0) == 0) {
        LaunchEditor(Path);
    } else {
        ThrowError (1, "cannot open script message file \"%s\"", Path);
    }

}

void MainWindow::toolbarHTMLreport()
{
#ifdef _WIN32
    char Path[2*MAX_PATH+16];
    sprintf (Path, "%s\\%sreport.html", s_main_ini_val.StartDirectory, s_main_ini_val.ScriptOutputFilenamesPrefix);
    SearchAndReplaceEnvironmentStrings(Path, Path, sizeof(Path));
    if (access(Path, 0) == 0) {
        ShellExecute (nullptr, "open", Path, nullptr, "", SW_SHOW);
    } else {
        ThrowError (1, "cannot open html report file \"%s\"", Path);
    }
#else
    // TODO
    ThrowError (1, "not implemented");
#endif

}

void MainWindow::toolbarOpenErrorFileWithEditor()
{
#ifdef _WIN32
    char Path[2*MAX_PATH+16];
    sprintf (Path, "%s\\%s\\%s.err", s_main_ini_val.StartDirectory, s_main_ini_val.ScriptOutputFilenamesPrefix,
             GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_ERROR_FILE));
    SearchAndReplaceEnvironmentStrings(Path, Path, sizeof(Path));
    if (access(Path, 0) == 0) {
        ShellExecute (nullptr, "open", Path, nullptr, "", SW_SHOW);
    } else {
        ThrowError (1, "cannot open %s.err file \"%s\"", Path, GetConfigurablePrefix(CONFIGURABLE_PREFIX_TYPE_ERROR_FILE));
    }
#else
    // TODO
    ThrowError (1, "not implemented");
#endif
}

void MainWindow::setTopHintFlagControlPanel()
{
}

void MainWindow::removeTopHintFlagControlPanel()
{
}

void MainWindow::MoveControlPanelToItsStoredPosition(bool par_Maximized)
{
    bool Err = false;
    int loc_x_main_window;
    int loc_y_main_window;

    QRect MainWindowGeometry = frameGeometry();
    loc_x_main_window = MainWindowGeometry.x();
    loc_y_main_window = MainWindowGeometry.y();
    if (par_Maximized) {
        QPoint Point(x()+width()/2, y()+height()/2);
        QScreen *MainWindowScreen = QGuiApplication::screenAt(Point);
        if (MainWindowScreen != nullptr) {
            QRect MainWindowScreenGeometry = MainWindowScreen->geometry();
            loc_x_main_window = MainWindowScreenGeometry.x();
            loc_y_main_window = MainWindowScreenGeometry.y();
        } else {
            // this can happen if running without screen (cloud, ...)
            //ThrowError (1, "cannot get screen of application");
            Err = true;
        }
    }
    if (!Err) {
        int loc_x_diff_control_panel = s_main_ini_val.ControlPanelXPos;
        int loc_y_diff_control_panel = s_main_ini_val.ControlPanelYPos;
        int loc_x_control_panel = loc_x_main_window + loc_x_diff_control_panel;
        int loc_y_control_panel = loc_y_main_window + loc_y_diff_control_panel;

        // Check if it is visable
        QPoint Point(loc_x_control_panel, loc_y_control_panel);
        QScreen *ControlPanelScreen = QGuiApplication::screenAt(Point);
        if (ControlPanelScreen == nullptr) {
            // If it is outside all screens
            loc_x_control_panel = 0;
            loc_y_control_panel = 0;
        } else {
            d_controlPanel->move (loc_x_control_panel, loc_y_control_panel);
        }
    }
}

MainWindow::~MainWindow()
{
    MainWindowIsOpen = false;
    m_blackboardModel->change_connection_to_observer(false);
    PointerToMainWindow = nullptr;
    delete ui;
}


QSize MainWindow::sizeHint(void) const
{
    // this will never called
    return m_NormalSize;
}

void MainWindow::openElement()
{
    QAction *act = qobject_cast<QAction*>(sender());
    QStringList ToOpenWindowNames = m_allWindowTypes->value(act->text())->openElementDialog();
    // current selected sheet
    Sheets *Sheet = m_allSheets->value(ui->centralwidget->tabText(ui->centralwidget->currentIndex()));
    QList<MdiWindowWidget*> OpendWidgets = Sheet->OpenWindows(ToOpenWindowNames, m_allWindowTypes, m_updateTimers, m_OpenWindowPosValid, m_OpenWindowPos);
    int dx = 0;
    int dy = 0;
    foreach (MdiWindowWidget* OpenWidget, OpendWidgets) {
        connect(OpenWidget, SIGNAL(openStandardDialog(QStringList,bool,bool,QColor,QFont,bool)), this, SLOT(openElementDialog(QStringList,bool,bool,QColor,QFont,bool)));
        // If context menu is open select this position
        if (m_OpenWindowPosValid) {
            QPoint Point = Sheet->mapFromGlobal(m_OpenWindowPos);
            OpenWidget->GetCustomMdiSubwindow()->move(Point.x() + dx, Point.y() + dy);
            dx += 10;
            dy += 10;
        }

    }
}

void MainWindow::newElement()
{
    QAction *act = qobject_cast<QAction*>(sender());

    Sheets *Sheet = m_allSheets->value(ui->centralwidget->tabText(ui->centralwidget->currentIndex()));

    if (Sheet != nullptr) {
        MdiSubWindow *SubWindow = new MdiSubWindow(Sheet);
        MdiWindowWidget *loc_SC_Widget = m_allWindowTypes->value(act->text())->newElement(SubWindow);

        if (loc_SC_Widget != nullptr) {
            connect(loc_SC_Widget, SIGNAL(openStandardDialog(QStringList,bool,bool,QColor,QFont,bool)), this, SLOT(openElementDialog(QStringList,bool,bool,QColor,QFont,bool)));

            // Add new window to the [all xxxx windows] list
            loc_SC_Widget->GetMdiWindowType()->AddWindowToAllOfTypeIniList (loc_SC_Widget);
            // Set the dafault size of the new window
            QSize Size = loc_SC_Widget->GetMdiWindowType()->GetDefaultSize();
            if (!Size.isNull()) {
                loc_SC_Widget->GetCustomMdiSubwindow()->resize (Size);
            }

            Sheet->addSubWindowToSheet(loc_SC_Widget);

            // If it was open by the contex menu use the position of the contex menu
            if (m_OpenWindowPosValid) {
                QPoint Point = Sheet->mapFromGlobal(m_OpenWindowPos);
                loc_SC_Widget->GetCustomMdiSubwindow()->move(Point.x(), Point.y());
            }

            bool Ret = true;
            switch (m_allWindowTypes->value(act->text())->GetUpdateRate()){
            case InterfaceWidgetPlugin::FastUpdateWindow:
                Ret = connect(m_updateTimers->updateTimer(InterfaceWidgetPlugin::FastUpdateWindow), SIGNAL(timeout()), loc_SC_Widget, SLOT(CyclicUpdate()));
                break;
            case InterfaceWidgetPlugin::MediumUpdateWindow:
                Ret = connect(m_updateTimers->updateTimer(InterfaceWidgetPlugin::MediumUpdateWindow), SIGNAL(timeout()), loc_SC_Widget, SLOT(CyclicUpdate()));
                break;
            case InterfaceWidgetPlugin::SlowUpdateWindow:
                Ret = connect(m_updateTimers->updateTimer(InterfaceWidgetPlugin::SlowUpdateWindow), SIGNAL(timeout()), loc_SC_Widget, SLOT(CyclicUpdate()));
                break;
            case InterfaceWidgetPlugin::NoneUpdateWindow:
                break;
            };
            if (!Ret) {
                ThrowError (1, "cannot connect SIGNAL(timeout()) to SLOT(CyclicUpdate())");
            }
        } else {
            delete loc_SC_Widget;
        }
    }
}

void MainWindow::loadAllPlugins()
{
    AddWindowType(new TextType());
    AddWindowType(new OscilloscopeType());
    AddWindowType(new LampsType());
    AddWindowType(new SliderType());
    AddWindowType(new EnumType());
    AddWindowType(new TachoType());
    AddWindowType(new BargraphType());
    AddWindowType(new KnobType());
    AddWindowType(new CalibrationTreeType());
    AddWindowType(new UserDrawType());
    AddWindowType(new UserControlType());
    AddWindowType(new A2LCalSingleType());
    AddWindowType(new A2LCalMapType());
    AddWindowType(new CANMessageWindowType());

    // Add all further display elements to the menue entry "new" und "open" and into the m_elements QMap
    QDir pluginDir(QDir::currentPath());
    // NOTE: All plugins must be inside the same folder "plugins" which must exist beside the XilEnv executable
    if(pluginDir.cd("plugins")) {
        foreach (QString loc_fileName, pluginDir.entryList(QDir::Files)) {
            QPluginLoader pluginLoader(pluginDir.absoluteFilePath(loc_fileName));
            QObject *loc_plugin = pluginLoader.instance();
            if(loc_plugin) {
                // A Plugin must have the type MdiWindowType
                MdiWindowType *loc_MdiWindowType = qobject_cast<MdiWindowType*>(loc_plugin);
                if(loc_MdiWindowType) {
                    AddWindowType(loc_MdiWindowType);
                }
            }
        }
    }
}

void MainWindow::CloseWindowByName(char *par_WindowName)
{
    foreach(QString loc_key, m_allSheets->keys()) {
        m_allSheets->value(loc_key)->CloseWindowByName (par_WindowName);
    }
}

void MainWindow::changeEvent(QEvent* arg_event)
{
    if(arg_event->type() == QEvent::WindowStateChange) {
        if (OpenWithNoGui ()) {
            showMinimized();   // Window must stay a icon
        }
        QWindowStateChangeEvent *loc_event = static_cast<QWindowStateChangeEvent*>(arg_event);
        if(loc_event->oldState() & Qt::WindowMinimized) {
            // Normal or maximized
            RePositioningProcessBars();
        } else {
            if((loc_event->oldState() == Qt::WindowNoState) && (this->windowState() == Qt::WindowMaximized)) {
                // Maximized
                RePositioningProcessBars();
            }
        }
    }
    QMainWindow::changeEvent(arg_event);
}

bool MainWindow::close()
{
    if (m_ScriptUserDialog != nullptr) {
        delete m_ScriptUserDialog;
        m_ScriptUserDialog = nullptr;
    }
    return QMainWindow::close();
}

void MainWindow::loadPlugin()
{
    QFileDialog dialog(this, tr("Load Plugin"), QDir::currentPath(), tr("Plugins (*.dll)"));
    if(dialog.exec()) {
        foreach(QString loc_fileName, dialog.selectedFiles()) {
            QPluginLoader pluginLoader(dialog.directory().absoluteFilePath(loc_fileName));
            QObject *loc_plugin = pluginLoader.instance();
            if(loc_plugin) {
                // A Plugin must have the type MdiWindowType
                MdiWindowType *loc_MdiWindowType = qobject_cast<MdiWindowType*>(loc_plugin);
                if(loc_MdiWindowType) {
                    ui->menuNew->addAction(loc_MdiWindowType->GetMenuEntryName(), this, SLOT(newElement()));
                    ui->menuOpen->addAction(loc_MdiWindowType->GetMenuEntryName(), this, SLOT(openElement()));
                    m_allWindowTypes->insert(loc_MdiWindowType->GetMenuEntryName(), loc_MdiWindowType);
                }
            }
        }
    }
}


void MainWindow::resizeEvent(QResizeEvent * event)
{
    if (!isMaximized() && !isMinimized()) {
        if (!event->oldSize().isEmpty()) {
            m_NormalSize = event->oldSize();
        }
    }
    RePositioningProcessBars();
    QMainWindow::resizeEvent(event);
}

void MainWindow::showBlackboardList(bool arg_visible)
{
    if(arg_visible) {
        m_blackboardModel->change_connection_to_observer(true); // If there is no connection to the observer
        ui->blackboardDockWidget->show();
    } else {
        ui->blackboardDockWidget->hide();
    }
}

int MainWindow::GetExitCode()
{
    return m_ExitCode;
}

void MainWindow::openElementDialog(QStringList arg_selectedVaraibles, bool arg_allVaraibles, bool arg_multiselect, QColor arg_color, QFont arg_font, bool arg_TitleEditable)
{
    Q_UNUSED(arg_TitleEditable);
    m_elementDialog = new DefaultElementDialog(this);
    if(m_elementDialog) {
        MdiWindowWidget* loc_window = qobject_cast<MdiWindowWidget*>(sender());
        if(arg_allVaraibles) {
            m_elementDialog->setVaraibleList(DefaultDialogFrame::ANY);
        } else {
            m_elementDialog->setVaraibleList(DefaultDialogFrame::ENUM);
        }
        m_elementDialog->setMultiSelect(arg_multiselect);
        m_elementDialog->setCurrentVisibleVariables(arg_selectedVaraibles);
        m_elementDialog->getDefaultFrame()->setCurrentColor(arg_color);
        m_elementDialog->getDefaultFrame()->setCurrentFont(arg_font);
        m_elementDialog->setElementWindowName(loc_window->GetWindowTitle());
        CustomDialogFrame* Frame = loc_window->dialogSettings(m_elementDialog);
        m_elementDialog->addSettings(Frame);
        connect(m_elementDialog->getDefaultFrame(), SIGNAL(colorChanged(QColor)), loc_window, SLOT(changeColor(QColor)));
        connect(m_elementDialog->getDefaultFrame(), SIGNAL(fontChanged(QFont)), loc_window, SLOT(changeFont(QFont)));
        connect(m_elementDialog->getDefaultFrame(), SIGNAL(variableSelectionChanged(QString,bool)), loc_window, SLOT(changeVariable(QString,bool)));
        connect(m_elementDialog->getDefaultFrame(), SIGNAL(variablesSelectionChanged(QStringList,bool)), loc_window, SLOT(changeVaraibles(QStringList,bool)));
        connect(m_elementDialog->getDefaultFrame(), SIGNAL(defaultVariables(QStringList)), loc_window, SLOT(resetDefaultVariables(QStringList)));
        connect(m_elementDialog, SIGNAL(windowNameChanged(QString)), loc_window, SLOT(changeWindowName(QString)));
        if(m_elementDialog->exec() == QDialog::Accepted) {
            // do nothing
        }
        disconnect(m_elementDialog->getDefaultFrame(), SIGNAL(colorChanged(QColor)), loc_window, SLOT(changeColor(QColor)));
        disconnect(m_elementDialog->getDefaultFrame(), SIGNAL(fontChanged(QFont)), loc_window, SLOT(changeFont(QFont)));
        disconnect(m_elementDialog->getDefaultFrame(), SIGNAL(variableSelectionChanged(QString,bool)), loc_window, SLOT(changeVariable(QString,bool)));
        disconnect(m_elementDialog->getDefaultFrame(), SIGNAL(variablesSelectionChanged(QStringList,bool)), loc_window, SLOT(changeVaraibles(QStringList,bool)));
        disconnect(m_elementDialog->getDefaultFrame(), SIGNAL(defaultVariables(QStringList)), loc_window, SLOT(resetDefaultVariables(QStringList)));
        disconnect(m_elementDialog, SIGNAL(windowNameChanged(QString)), loc_window, SLOT(changeWindowName(QString)));
        delete m_elementDialog;
        m_elementDialog = nullptr;
    }
}

void MainWindow::styleChange()
{
    QObject *loc_sender = sender();
    QAction *loc_action = qobject_cast<QAction*>(loc_sender);
    if(loc_action) {
        strncpy (s_main_ini_val.SelectStartupAppStyle, QStringToConstChar(loc_action->text()), sizeof(s_main_ini_val.SelectStartupAppStyle)-1);
        qApp->setStyle(loc_action->text());
    }
}

void MainWindow::cascadeSubWindows()
{
    (dynamic_cast<QMdiArea*>(ui->centralwidget->currentWidget()))->cascadeSubWindows();
}

void MainWindow::tileSubWindows()
{
    (dynamic_cast<QMdiArea*>(ui->centralwidget->currentWidget()))->tileSubWindows();
}

int WindowNameAlreadyInUse(QString &par_WindowName)
{
    if (MainWindow::PointerToMainWindow != nullptr) {
        if (MainWindow::PointerToMainWindow->IsWindowOpen(par_WindowName, false)) {
            return 1;
        }
        QString IniSection("GUI/Widgets/");
        IniSection.append(par_WindowName);
        // Look if there exist a section with this name inside the INI file
        if (IniFileDataBaseLookIfSectionExist(QStringToConstChar(IniSection), GetMainFileDescriptor())) {
            return 1;  // exists inside the INI file
        }
    } else {
        return 1;
    }
    return 0;
}

int IsWindowOpen(QString &par_WindowName, bool par_OnlyActiveSheet)
{
    if (MainWindow::PointerToMainWindow != nullptr) {
        if (MainWindow::PointerToMainWindow->IsWindowOpen(par_WindowName, par_OnlyActiveSheet)) {
            return 1;
        }
    } else {
        return 1;
    }
    return 0;
}

cHotkeyHandler *MainWindow::get_hotkeyHandler()
{
    return hotkeyHandler;
}

void MainWindow::set_hotkeyHandler(QStringList list)
{
    hotkeyHandler->set_shortcutStringList(list);
}


QString GetNewUniqueWindowTitleStartWith (QString &Prefix)
{
    if (!WindowNameAlreadyInUse (Prefix)) return Prefix;
    QString WindowName;
    for (int x = 1; ; x++) {
        WindowName = Prefix;
        WindowName.append(QString(" %1").arg(x));
        if (!WindowNameAlreadyInUse (WindowName)) break;
    }
    return WindowName;
}

QString GetScriptFilenameFromControlPannel()
{
    if (MainWindow::PointerToMainWindow != nullptr) {
        return MainWindow::PointerToMainWindow->GetScriptFilenameFromControlPannel();
    }
    return QString();
}
