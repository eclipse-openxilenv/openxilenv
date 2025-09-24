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


#if defined(_WIN32) && !defined(__GNUC__)
#ifndef _M_X64
#error should compiled with 64bit target
#endif
#endif

#include <QSettings>

#include <locale.h>

#include <QtWidgets/QApplication>
#include "MainWindow.h"
#include "DarkMode.h"
#include "BlackboardObserver.h"
#include "ErrorDialog.h"
#include "DarkMode.h"

extern "C" {
    #include "MainValues.h"
    #include "StartupInit.h"
    #include "ParseCommandLine.h"
    #include "Scheduler.h"
    #include "RpcSocketServer.h"
    extern int rm_Terminate(void);
    extern void rm_Kill(void);
}

bool SystemDarkMode(QApplication *par_Application)
{
#if _WIN32
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                       QSettings::NativeFormat);
    if(settings.value("AppsUseLightTheme") == 0) {
        SetDarkMode(par_Application, true);
        return true;
    }
#endif
    return false;
}

int main(int argc, char *argv[])
{
#ifndef _WIN32
    // Store the command line so we can access it later
    SaveCommandArgs(argc, argv);
#endif

    QApplication Application(argc, argv);

    SystemDarkMode(&Application);

    // Use always "C" location to avoid use of ',' instead of '.'!
    QLocale::setDefault(QLocale::C);
    setlocale(LC_ALL, "C");

    Application.setStartDragDistance(20);

    if (StartupInit (static_cast<void*>(&Application))) {
        return 1;
    }

    StartBlackboardObserver();

    SetDarkMode(&Application, s_main_ini_val.DarkMode);

    if (strlen(s_main_ini_val.SelectStartupAppStyle)) {  // if a style is defined inside INI file?
        if (!s_main_ini_val.DarkMode) {
            Application.setStyle(QString(s_main_ini_val.SelectStartupAppStyle));
        }
    }

    MainWindow *Window = new MainWindow(QSize (s_main_ini_val.MainWindowWidth, s_main_ini_val.MainWindowHeight));

    SetSyncWithGuiThreadIsInitAndRunning();

    Window->resize (QSize (s_main_ini_val.MainWindowWidth, s_main_ini_val.MainWindowHeight));
    if (ShouldStartedAsIcon() || OpenWithNoGui ()) {
        Window->showMinimized();
    } else if (s_main_ini_val.Maximized) {
        Window->showMaximized();
    } else {
        Window->show();
    }
    Window->MoveControlPanelToItsStoredPosition(s_main_ini_val.Maximized);

    if (GetSchedulerStopByRpc()) {
        disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_RPC, nullptr, nullptr);
    }
    if (GetSchedulerStopByUser()) {
        disable_scheduler_at_end_of_cycle (SCHEDULER_CONTROLED_BY_USER, nullptr, nullptr);
    }

    // Now the schedulers can run
    SchedulersStartingShot();

    StartRemoteProcedureCallThread (s_main_ini_val.RpcOverSocketOrNamedPipe, s_main_ini_val.InstanceName, s_main_ini_val.RpcSocketPort);

    // Starts the Qt event loop
    Application.exec();

    int ExitCode = Window->GetExitCode();

    // The destructor must be called before TerminateSelf
    delete Window;

    TerminateSelf();

    return ExitCode;
}
