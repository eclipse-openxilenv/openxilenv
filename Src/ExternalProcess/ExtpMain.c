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


#include <stdio.h>
#include <stdlib.h>
#include "Platform.h"

#include "ExtpBaseMessages.h"
#include "ExtpProcessAndTaskInfos.h"
#include "ExtpParseCmdLine.h"
#include "XilEnvExtProc.h"

#ifndef EXTPROC_DLL_EXPORTS
extern void reference_varis (void);
extern int init_test_object (void);
extern void cyclic_test_object (void);
extern void terminate_test_object (void);

static EXTERN_PROCESS_TASKS_LIST ExternProcessTasksList[1] = {{"", reference_varis, init_test_object, cyclic_test_object, terminate_test_object}};
static int ExternProcessTasksListElementCount = 1;
#endif

struct WM_CREATE_PARAM {
    int ExternProcessTasksListElementCount;
    EXTERN_PROCESS_TASKS_LIST ExternProcessTasksList[MAX_PROCESSES_INSIDE_ONE_EXECUTABLE];
};

#ifdef _WIN32
LRESULT FAR PASCAL WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_CREATE:
        CheckIfConnectedTo (((struct WM_CREATE_PARAM*)((CREATESTRUCTA*)(lParam))->lpCreateParams)->ExternProcessTasksList, 
                                   ((struct WM_CREATE_PARAM*)((CREATESTRUCTA*)(lParam))->lpCreateParams)->ExternProcessTasksListElementCount);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        break;
    case WM_ACTIVATE:
        ShowWindow (hwnd, SW_MINIMIZE);
        break;
    case WM_COMMAND:
        switch (wParam) {
        case 101:  ;
        }
        break;
    }
    return DefWindowProc (hwnd, message, wParam, lParam) ;
}

static DWORD WINAPI XilEnvInternal_WindowMainLoopThreadFunction (LPVOID lpParam)
{
    WNDCLASS wndclass;
    HWND hwnd;
    MSG msg;
    char app_name[MAX_PATH];
    HINSTANCE h_instance = GetModuleHandle(NULL);

    GetModuleFileName (h_instance, app_name, sizeof (app_name));

    // First instance -> register window class
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = h_instance;
    wndclass.hIcon          = LoadIcon (h_instance, "EXTP_ICON");
    wndclass.hCursor        = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH)COLOR_BACKGROUND;
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = app_name;

    RegisterClass (&wndclass);

    // Create window
    hwnd = CreateWindow (app_name, app_name,
                         WS_OVERLAPPEDWINDOW | WS_MINIMIZE,
                         CW_USEDEFAULT, CW_USEDEFAULT,
                         200, 100,
                         NULL, NULL, h_instance, lpParam);
    // and show it
    ShowWindow (hwnd, SW_MINIMIZE);

    SetHwndMainWindow (hwnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage (&msg) ;
        DispatchMessage (&msg) ;
    }
    exit(0);
    return (DWORD)msg.wParam;
}
#endif

EXPORT_OR_IMPORT int __FUNC_CALL_CONVETION__ ExternalProcessMain (int par_GuiOrConsole, EXTERN_PROCESS_TASKS_LIST *par_ExternProcessTasksList, int par_ExternProcessTasksListElementCount)
{
    struct WM_CREATE_PARAM *WmCreateParam;
    EXTERN_PROCESS_TASK_HANDLE ProcessHandle;
    int Ret;

#ifdef _WIN32
    if (par_GuiOrConsole) {
        // GUI
        char *CommandLine;
        int NoGui = 0;
        int Err2Msg = 0;
        int NoXcp = 0;
        int Status;
        char NoGuiString[64];
        EXTERN_PROCESS_INFOS_STRUCT *ProcessInfos = XilEnvInternal_GetGlobalProcessInfos ();

        // before check if -nogui is set
        CommandLine = GetCommandLine ();
        XilEnvInternal_ParseCmdLine (CommandLine, NULL, 0, NULL, 0, NULL, 0,
                                      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &NoGui, &Err2Msg, &NoXcp, NULL, 0);
        // Or the environment variable is set
        Status = GetEnvironmentVariable("XilEnv_NoGui", NoGuiString, sizeof(NoGuiString));
        if ((Status > 0) && (Status < sizeof(NoGuiString))) {
            if (!strcmpi(NoGuiString, "yes")) {
                NoGui = 1;
            }
        }
        Status = GetEnvironmentVariable("XILENV_NO_XCP", NoGuiString, sizeof(NoGuiString));
        if ((Status > 0) && (Status < sizeof(NoGuiString))) {
            if (!strcmpi(NoGuiString, "yes")) {
                NoXcp = 1;
            }
        }

        // Save this into the global structure
        ProcessInfos->NoGuiFlag = NoGui; 
        ProcessInfos->Err2Msg = Err2Msg; 
        ProcessInfos->NoXcpFlag = NoXcp;

        // switch off the FMU internal xcp driver use the XilEnv one instead
        if (!SetEnvironmentVariable("SWITCH_OFF_XCP_IN_PROCESS", "yes")) {
            ThrowError (1, "cannot set environment variable \"SWITCH_OFF_XCP_IN_PROCESS\"");
        }

        if (NoXcp) {
            if (!SetEnvironmentVariable("XilEnv_NoXcp", "yes")) {
                ThrowError (1, "cannot set environment variable \"XilEnv_NoXcp\" (-NoXcp will be ignored)");
            }
        }

        if (NoGui) {
            if (!SetEnvironmentVariable("XilEnv_NoGui", "yes")) {
                ThrowError (1, "cannot set environment variable \"XilEnv_NoGui\" (-nogui will be ignored)");
            }
        } else {
            DWORD dwThreadId;
            int x;
            WmCreateParam = (struct WM_CREATE_PARAM*)malloc (sizeof(struct WM_CREATE_PARAM));
            if (WmCreateParam == NULL) {
                ThrowError (1, "out of memory");
            } else {
                for (x = 0; (x < MAX_PROCESSES_INSIDE_ONE_EXECUTABLE) && (x < par_ExternProcessTasksListElementCount); x++) {
                    WmCreateParam->ExternProcessTasksList[x] = par_ExternProcessTasksList[x];
                }
                WmCreateParam->ExternProcessTasksListElementCount = x;

                CreateThread (NULL,              // no security attribute
                              0,                 // default stack size
                              XilEnvInternal_WindowMainLoopThreadFunction,    // thread proc
                              (LPVOID)WmCreateParam,    // thread parameter
                              0,                 // not suspended
                              &dwThreadId);      // returns thread ID
            }
        }
    } else {
        EXTERN_PROCESS_INFOS_STRUCT *ProcessInfos = XilEnvInternal_GetGlobalProcessInfos ();
        ProcessInfos->NoGuiFlag = 1;
    }
#endif
    if (CheckIfConnectedToEx (par_ExternProcessTasksList, par_ExternProcessTasksListElementCount, &ProcessHandle) == 0) {
        EXTERN_PROCESS_INFOS_STRUCT *ProcessInfos = XilEnvInternal_GetGlobalProcessInfos ();
        int x;
        int AllThreadStoped;
        // This is the execution loop, the function will return only if the external process will be terminated
        EnterAndExecuteOneProcessLoop (ProcessHandle);
        // The main thread should wait till all other threads are terminated!
        do {
            AllThreadStoped = 1;
            for (x = 0; x < ProcessInfos->ThreadCount; x++) {
                if (ProcessInfos->TasksInfos[x] != NULL) {
                    if (ProcessInfos->TasksInfos[x]->State) AllThreadStoped = 0;
                }
            }
            if (!AllThreadStoped) {
#ifdef _WIN32
                Sleep(10);
#else
                usleep(10*1000);
#endif
            }
        } while(!AllThreadStoped);

        Ret = 0;
    } else {
        Ret = 1;
    }
    return Ret;
}

#ifndef EXTPROC_DLL_EXPORTS
int main(void)
{
#if defined _WIN32 && defined __GNUC__
	// beim mingw funktioniert es nicht die WinMain in eine Library zu packen
    return ExternalProcessMain(1, ExternProcessTasksList, ExternProcessTasksListElementCount);
#else
    return ExternalProcessMain(0, ExternProcessTasksList, ExternProcessTasksListElementCount);
#endif
}


#ifdef _WIN32
int WINAPI WinMain (HINSTANCE h_instance,
                    HINSTANCE h_prev_instance,
                    LPSTR lpsz_cmdline,
                    int i_cmd_show)
{
    return ExternalProcessMain(1, ExternProcessTasksList, ExternProcessTasksListElementCount);
}
#endif
#endif
