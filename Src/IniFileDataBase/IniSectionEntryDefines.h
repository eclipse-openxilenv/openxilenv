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


#ifndef INISECTIONENTRYDEFINES_H
#define INISECTIONENTRYDEFINES_H

// INI sections/entys/defaults
#define OPT_FILE_INFOS      "FileInfos"
#define OPT_FILE_VERSION    "Version"
#define OPT_FILE_TYPE       "Type"

#define SCHED_INI_SECTION     "Scheduler"
#define PERIODE_INI_ENTRY     "Period"
#define LI_TIMEOUT_INI_ENTRY  "LoginTimeout"

#define SCHED_SECTION       "Scheduler"
#define SCHED_PERIODE_TEXT  "Period"
#define SCHED_PERIODE_DEF   "0.01"

#define OPT_SECTION         "BasicSettings"
#define OPT_PROJECT_TEXT    "ProjectName"
#define OPT_PROJECT_DEF     "DefaultTitle"
#define OPT_WORKDIR_TEXT    "WorkDir"
#define OPT_WORKDIR_DEF     ""
#define OPT_XDIM_TEXT       "xdim"
#define OPT_XDIM_DEF        800
#define OPT_YDIM_TEXT       "ydim"
#define OPT_YDIM_DEF        600
#define OPT_MAXIMIZED_TEXT  "maximized"
#define OPT_MAXIMIZED_DEF   0
#define OPT_CPNL_XPOS_TEXT  "ControlpanelXpos"
#define OPT_CPNL_XPOS_DEF   100
#define OPT_CPNL_YPOS_TEXT  "ControlpanelYpos"
#define OPT_CPNL_YPOS_DEF   100
#define OPT_CYCLES_TEXT     "CountofCycles"
#define OPT_CYCLES_DEF      1
#define OPT_ACT_CTRL_TEXT   "SelectedControl"
#define OPT_ACT_CTRL_DEF    0
#define OPT_BB_SIZE_TEXT    "BlackBoardSize"
#define OPT_BB_SIZE_DEF     32768
#define OPT_ED_PATH_TEXT    "Editor"
#define OPT_ED_X_PATH_TEXT    "EditorX"
#define OPT_ED_PATH_DEF     ""
#define OPT_SCRMSG_TEXT     "ScriptMessages"
#define OPT_SCRMSG_DEF      0

#define LAST_10_BP_SECT   "GUI/Last_10_Breakpoints"
#define LAST_10_SCR_SECT  "GUI/Last_10_ScriptFiles"
#define LAST_10_GEN_SECT  "GUI/Last_10_GeneratorFiles"
#define LAST_10_REC_SECT  "GUI/Last_10_RecorderFiles"
#define LAST_10_PLY_SECT  "GUI/Last_10_PlayerFiles"
#define LAST_10_TRG_SECT  "GUI/Last_10_EquationFiles"

#define OPN_WND_TYP_TEXT  "type"

#define DEFAULT_INI_FILE  "DEFAULT.INI"

#endif
