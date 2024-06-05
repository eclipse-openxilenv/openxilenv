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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifdef __cplusplus
#include "ProgressBar.h"
#include <QMainWindow>
#include <QLabel>
#include <QComboBox>
#include <QTimer>
#include <QTime>
#include <QPoint>
#include <QMap>
#include <QMenu>
#include <QMdiArea>
#include <QCloseEvent>
#include <QPixmap>
#include <QMutex>
#include <QWaitCondition>
#include <QVBoxLayout>
#include <QFile>
#include <QFileDialog>
#include <QUrl>
#include <QPluginLoader>
#include <QFrame>

#include "HotkeyHandler.h"
#include "ControlPanel.h"
#include "MdiSubWindow.h"

#include "LampsWidget.h"
#include "NewSheetDialog.h"
#include "OpenElementDialog.h"
#include "BasicSettingsDialog.h"
#include "HotkeysDialog.h"
#include "LoadValuesDialog.h"
#include "LoadSvlFileDialog.h"
#include "SaveProcessesDialog.h"
#include "ImportReferenceListDialog.h"
#include "ExportReferenceListDialog.h"
#include "GlobalSectionFilterDialog.h"
#include "WritesSctionToExeDialog.h"
#include "ImportVariableProperties.h"
#include "ExportVariablePropertiesDialog.h"
#include "UserDefinedEnvironmentVariablesDialog.h"
#include "CcpConfigDialog.h"
#include "StartStopCppDialog.h"
#include "StartStopCcpCalDialog.h"
#include "LoadCcpConfigDialog.h"
#include "SaveCcpConfigDialog.h"
#include "XcpConfigDialog.h"
#include "StartStopXcpDialog.h"
#include "StartStopXcpCalDialog.h"
#include "LoadXcpConfigDialog.h"
#include "SaveXcpConfigDialog.h"
#include "AboutDialog.h"
#include "ImportSheetsDialog.h"
#include "ExportSheetsDialog.h"
#include "InternalsDialog.h"
#include "ConfigureXcpOverEthernetDialog.h"
#include "UserDefinedBlackboardVariablesDialog.h"

#include "ScriptUserDialog.h"
#include "BlackboardVariableModel.h"
#include "DefaultElementDialog.h"

extern "C"{
#include <LoadSaveToFile.h>
#include <SectionFilter.h>
#include <ImExportVarProperties.h>
#include "CanDataBase.h"
#include <ImExportDskFile.h>
#include <BlackboardIniCleaner.h>
#include <EquationList.h>
#include "IniDataBase.h"
#include "MainValues.h"
#include "FileExtensions.h"
#include "BlackboardAccess.h"
#include <ExportA2L.h>
#include <MainValues.h>
#include <StartupInit.h>
}
#include <QMessageBox>
#include "Sheets.h"

#include "MdiWindowType.h"
#include "MdiWindowWidget.h"

namespace Ui {
class MainWindow;
}

class ScriptUserDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QSize par_NormalSize, QWidget *parent = nullptr);
    ~MainWindow();

    int GetExitCode();

    bool writeToIni();

    cHotkeyHandler *get_hotkeyHandler();
    void set_hotkeyHandler(QStringList list);

    QSize sizeHint(void) const;

    int IsWindowOpen (QString &par_WindowName, bool par_OnlyActiveSheet);
    int IsWindowOpen (char *par_WindowName);
    QString GetScriptFilenameFromControlPannel();

    static MainWindow *PointerToMainWindow;
    static bool MainWindowIsOpen;

    QStringList GetSheetsInsideWindowIsOpen (QString par_WindowName);
    void SelectSheetAndWindow (QString par_SheetName, QString par_WindowName);

    QString GetCurrentSheet();
    QList<Sheets *> GetAllSheets();
    QMap<QString, Sheets *> *GetRefToAllSheets();

    void ReadAllSheetsFromIni();
    void ReadOneSheetFromIni(QString &par_Sheet, QString &par_ImportSection);

    static bool IsMainWindowOpen();
    static MainWindow *GetPtrToMainWindow();

    static BlackboardVariableModel *GetBlackboardVariableModel();
    void setTopHintFlagControlPanel();
    void removeTopHintFlagControlPanel();

    void MoveControlPanelToItsStoredPosition(bool par_Maximized);

private:
    void populateToolbar();
    MdiSubWindow* addNewSubwindow();
    void closeEvent(QCloseEvent *event);
    int addNewTab(QString name, QString arg_section, bool par_Selected);
    void loadAllPlugins();
    void CloseWindowByName(char *par_WindowName);
    void changeEvent(QEvent *arg_event);

signals:
    void SetProgressBarSignal(int value, unsigned int par_progressID, bool par_CalledFromNonGuiThread);

private Q_SLOTS:
    bool close();
    void addNewTab();
    void openElement();
    void openUserDefinedBBVarsDialog();
    void newElement();
    void closeTab(int index, bool AskFlag = true);
    int renameTab(int index, QString OldName, QString NewName);
    void closeAllTabs();
    void unhook(const QPoint);
    void hook(const QPoint);
    void closeCurrentTab();
    void changeTabFromComboBox(int index);
    void changeComboBoxFromTab(int index);
    void closeSubwindows();
    void hideSubwindows();
    void setStatusBarTime();
    void makeFullScreen(bool full);
    void openConfigureXCPOverEthernetDialog();
    void openGlobalSectionFilterDialog();
    void openSaveValueDialog();
    void openWriteSectionToExeDialog();
    void openExportVariablePropertiesDialog();
    void openImportVariablePropertiesDialog();
    void openUserDefinedEnvVarsDialog();
    void openCcpConfigDialog();
    void openCcpStartStopDialog();
    void openCcpStartStopCalibrationDialog();
    void openCcpImportConfigDialog();
    void openCcpExportConfigDialog();
    void openXcpConfigDialog();
    void openXcpStartStopDialog();
    void openXcpStartStopCalibrationDialog();
    void openXcpImportConfigDialog();
    void openXcpExportConfigDialog();
    void openCanInsertErrorDialog();
    void openExportVariableReferenceListDialog();
    void openImportVariableReferenceListDialog();
    void openSearchWindowWithVariableDialog();
    void openLoadSvlDialog();
    void openClearDesktopDialog();
    void openImportSheetDialog();
    void openExportSheetDialog();
    void openCleanAllBlackboardVariablesDialog();
    void openActiveEquationWindowDialog();
    void openBasicSettingsDialog();
    void openSaveDesktopDialog();
    void openLoadDesktopDialog();
    void openAboutDialog();
    void openInternalsDialog();
    void openLoadedDebugInfosDialog();

    void openCanConfigDialog();
    void openCanImportDialog();

    void menuClearDesktop();
    void menuExportA2L();
    void menuCleanBBIniSection();
    void menuExportHotKeys();
    void menuImportHotKeys();
    void menuFunctionHotKeys();
    void menuBlackboardInternalInfos();
    void OpenRemoteMasterCallStatisticsDialog();
    void menuHomepage();
    void menuSaveIniAs();
    void toolbarNotFasterThanRealtime(bool rtState);
    void toolbarHelp();
    void toolbarContinueScheduler();
    void toolbarStopScheduler();
    void toolbarNextOneScheduler();
    void toolbarSuppressNonExistValues(bool existValue);
    void toolbarSaveIniFile();
    void toolbarControlPanelShow(bool showPanel);
    void toolbarControlPanelButtonDeaktivate();
    void toolbarControlPanelMoveToTopLeftCorner();
    void toolbarControlContinue();
    void toolbarControlStop();
    void toolbarEditDebug();
    void toolbarEditError();
    void toolbarEditMessage();
    void toolbarHTMLreport();
    void toolbarOpenErrorFileWithEditor();
    void writeSheetsToIni();
    void loadPlugin();
    void closeAllSheets();
    void saveAllSheets(QString &par_SelectedSheet);
    void ExitWithoutSaveIni();
    void BarrierLogging();
    void ExternProcessCopyLists();
    void styleChange();

    void cascadeSubWindows();
    void tileSubWindows();

    // This methods can be called from other threads
    void closeFromOtherThread(int par_ExitCode, int par_ExitCodeValid);
    void ClearDesktopFromOtherThread();
    void LoadDesktopFromOtherThread(QString par_Filename);
    void SaveDesktopFromOtherThread(QString par_Filename);

    // This synchronisation methods are already inside MainWinowSyncWithOtherThreads.h
    void SchedulerStateChangedSlot(int par_ThreadId, int par_State);
    void OpenWindowByNameSlot(int par_ThreadId, char *par_WindowName, bool par_PosFlag, int par_XPos, int par_YPos, bool par_SizeFlag, int par_Width, int par_Hight);
    void IsWindowOpenSlot (int par_ThreadId, char *par_WindowName);
    void SaveAllConfigToIniDataBaseSlot (int par_ThreadId);
    void CloseWindowByNameSlot (int par_ThreadId, char *par_WindowName);
    void SelectSheetSlot (int par_ThreadId, char *par_SheetName);
    void AddSheetSlot (int par_ThreadId, char *par_SheetName);
    void DeleteSheetSlot (int par_ThreadId, char *par_SheetName);
    void RenameSheetSlot (int par_ThreadId, char *par_OldSheetName, char *par_NewSheetName);

    // Script-Commands: CREATE_DLG/ADD_DLG_ITEM/SHOW_DLG
    void ScriptCreateDialogSlot(int par_ThreadId, QString par_Headline);
    void ScriptAddDialogItemSlot(int par_ThreadId, QString par_Element, QString par_Label);
    void ScriptShowDialogSlot(int par_ThreadId);
    void ScriptDestroyDialogSlot(int par_ThreadId);
    void IsScriptDialogClosedSlot(int par_ThreadId);

    void StopOscilloscopeSlot(void* par_OscilloscopeWidget, uint64_t par_Time);

    // Socket based remote API
    void RemoteProcedureCallRequestSlot(int par_ThreadId, void *par_Connection, const void *par_In, void *ret_Out);


    void ImportHotkeySlot(int par_ThreadId, QString Filename);
    void ExportHotkeySlot(int par_ThreadId, QString Filename);

    void SetNotFasterThanRealtimeStateSlot(int par_State);
    void SetSuppressDisplayNonExistValuesStateSlot(int par_State);
    void DeactivateBreakpoinSlot();

    void AddNewScriptErrorMessageSlot(int par_ThreadId, int par_Level, int par_LineNr, const char *par_Filename, const char *par_Message);


    void CheckTerminatingSlot();

    void resizeEvent(QResizeEvent * event);
    void showBlackboardList(bool arg_visible);

public Q_SLOTS:
    void OpenProgressBarSlot(int par_Id, int par_ThreadId, QString par_ProgressName);
    void SetProgressBarSlot(int par_ThreadId, int par_ProgressBarID, int par_Value, bool par_CalledFromGuiThread);
    void CloseProgressBarSlot(int par_ThreadId, int par_ProgressBarID);

    void OpenOpenWindowMenuAt(const QPoint &par_Point);

    void openElementDialog(QStringList arg_selectedVaraibles, bool arg_allVaraibles, bool arg_multiselect, QColor arg_color, QFont arg_font, bool arg_TitleEditable);
private:
    void SwitchToolboxShowHideControlPanelIcon(bool par_Show);
    void ClearDesktop();
    void LoadDesktop(QString par_Filename);
    void SaveDesktop(QString par_Filename);
    void UpdateWindowTitle();

    void RePositioningProcessBars();

    void AddWindowType(MdiWindowType *arg_WindowType);

    Ui::MainWindow *ui;
    QMap<QString, MdiWindowType* > *m_allWindowTypes;
    QMap<QString, Sheets* > *m_allSheets;
    QLabel *d_labelRtFactor;
    QComboBox *d_comboBoxSheets;
    QMap<QWidget*, MdiSubWindow*> *d_widgetSubMap;
    ControlPanel *d_controlPanel;
    WindowUpdateTimers *m_updateTimers;
    QMap<int, ProgressBar*> m_progressBars;

    enum {NORMAL_SIZE, MAXIMIZED_SIZE, MINIMIZED_SIZE} m_WindowSizeState;
    QSize m_NormalSize;  // Stores if the window size is not maximized nor minimized

    cHotkeyHandler *hotkeyHandler;

    ScriptUserDialog *m_ScriptUserDialog;
    BlackboardVariableModel *m_blackboardModel;
    QSortFilterProxyModel *m_blackboardFilterModel;
    DefaultElementDialog *m_elementDialog;

    bool m_OpenWindowPosValid;
    QPoint m_OpenWindowPos;

    bool m_DoNotChangeNotFasterThanRealtime;

    bool m_ExitCodeSeeded;
    int m_ExitCode;
};


class SignalsFromOtherThread : public QObject
{
    Q_OBJECT
public:
    void CloseXilEnv (int par_ExitCode, int par_ExitCodeValid);
    void ClearDesktopFromOtherThread();
    void LoadDesktopFromOtherThread(char *par_Filename);
    void SaveDesktopFromOtherThread(char *par_Filename);
    void RpcCallRequestFromOtherThread();

signals:
    void CloseXilEnvSignal (int par_ExitCode, int par_ExitCodeValid);
    void ClearDesktopFromOtherThreadSignal();
    void LoadDesktopFromOtherThreadSignal(QString par_Filename);
    void SaveDesktopFromOtherThreadSignal(QString par_Filename);
};

QString GetNewUniqueWindowTitleStartWith (QString &Prefix);
QString GetScriptFilenameFromControlPannel();

#else

void CloseFromOtherThread (int par_ExitCode, int par_ExitCodeValid);
void ClearDesktopFromOtherThread();
void LoadDesktopFromOtherThread(char *par_Filename);
void SaveDesktopFromOtherThread(char *par_Filename);
void RpcCallRequestFromOtherThread (void);

#endif // __cplusplus

#endif // MAINWINDOW_H
