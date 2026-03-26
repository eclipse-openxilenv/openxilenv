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


#ifndef MAINVALUES_H
#define MAINVALUES_H
#include "Platform.h"
#include "Blackboard.h"

#define MAX_CONFIGURABLE_PREFIXES 32

typedef struct
{
  char            ProjectName[1024];      // Project name (not displayed)
  char            TempProjectName[1024];  // Project name (not displayed) will not write to INI file
  char            WorkDir[MAX_PATH];      // Working folder set at startup
  int             MainWindowWidth;
  int             MainWindowHeight;
  int             Maximized;              // Main window should be maximized
  int             ControlPanelXPos;       // X-Position of the control panel
  int             ControlPanelYPos;       // Y-Position of the control panel
  unsigned int    NumberOfNextCycles;     // Number of cycles filled into the control panel
  int             ControlPanelSelectedInternalProcess;
                                        // internal processes can be
                                        // Script = 0
                                        // Generator = 1
                                        // Stimuli = 2
                                        // Trigger = 3
                                        // Trace = 4
  int             ScriptMessageDialogVisable;  // Script message dialog is visable
                                               // 1 -> visable, 0 -> not visable
  int             ScriptDebugDialogVisable;    // Script debug dialog is visable
                                               // 1 -> visable, 0 -> not visable
  int             BlackboardSize;       // Blackboard size (max numbers of variables)
  char            Editor[MAX_PATH];     // Path to a text editor
  char            EditorX[MAX_PATH];     // Path to a text editor for Linux
  double          SchedulerPeriode;     // Scheduler periode
  int             SyncWithFlexray;      // Scheduler mit Flexray Synchronisieren
  int             WriteProtectIniProcessList;
  int             SwitchAutomaticSaveIniOff;
  int             AskSaveIniAtExit;
  int             RememberReferencedLabels;
  int             NotFasterThanRealTime;
  char            Priority[64];
  int             DontCallSleep;
  int             StatusbarCar;
  char            SpeedStatusbarCar[1024];
  char            StartDirectory[MAX_PATH];     // Working directory after starting XilEnv
  int             NoCaseSensitiveFilters;
  int             AsapCombatibleLabelnames;
  int             ReplaceAllNoneAsapCombatibleChars;
                                   // if 1 than ._[xx]
                                   // if 2 than ._[xx]._
  int             ViewStaticSymbols;
  int             ExtendStaticLabelsWithFilename;
  int             SaveSortedINIFiles;
  int             MakeBackupBeforeSaveINIFiles;
  int             DefaultMinMaxValues;
#define DEFAULT_MIN_0_MAX_1                        0
#define DEFAULT_MINMAX_DATATYPE                    1
#define DEFAULT_MINMAX_DATATYPE_BUT_NOT_MORE_THAN  2
  double          MinMinValues;
  double          MaxMaxValues;
  int             DontMakeScriptDebugFile;
  char            TerminateScript[MAX_PATH];
  int             TerminateScriptFlag;
  int             ScriptStartExeAsIcon;

  int             ScriptStopIfCcpError;
  int             ScriptStopIfXcpError;

  int             StopPlayerIfScriptStopped;
  int             StopRecorderIfScriptStopped;
  int             StopGeneratorIfScriptStopped;
  int             StopEquationIfScriptStopped;

  int             HideControlPanel;
  int             HideControlPanelLock;

  int             ShouldStopSchedulerWhileDialogOpen;
  int             StopSchedulerWhileDialogOpen; // 1 = Stop / 0 = Run
  int             SeparateCyclesForRefAndInitFunction;

  int             UseRelativePaths;
  int             SuppressDisplayNonExistValues;
  int             DontUseInitValuesForExistingCanVariables;

  int             ScrictAtomicEndAtomicCheck;

  char            CopyBB2ProcOnlyIfWrFlagSet;
  char            AllowBBVarsWithSameAddr;

  char            DontWaitForResponseOfStartRecorderRPC;

  int             SaveRefListInNewFormatAsDefault;

  struct {
      char RefLists;
      // CAN, ...
  } UseTempIniForFunc;
  char            ScriptOutputFilenamesPrefix[MAX_PATH];

  char            Script_ADD_BBVARI_DefaultUnit[BBVARI_UNIT_SIZE];
  int             LogFileForAccessedFilesFlag;
  char            LogFileForAccessedFilesName[MAX_PATH];

  int             AvoidCloseTemporarily;

  char            InstanceName[MAX_PATH];    // This will be set by the parameter -Instance, default are an empty string

  int             ExternProcessLoginTimeout;

  char            SelectStartupAppStyle[MAX_PATH];

  int             TextDefaultShowUnitColumn;   // is 1 if nothing defined
  int             TextDefaultShowDispayTypeColumn; // is 1 if nothing defined
  char            TextDefaultFont[MAX_PATH];  // is an empty string if nothing defined
  int             TextDefaultFontSize;    // is 0 if nothing defined

  char            OscilloscopeDefaultFont[MAX_PATH]; // is an empty string if nothing defined
  int             OscilloscopeDefaultFontSize;  // is 0 if nothing defined
  int             OscilloscopeDefaultBufferDepth;

  int             VersionIniFileWasWritten;       // -1 unknown
  int             PatchNumberIniFileWasWritten;   // negative is "pre-" 0 correspond "final" positive correspond "patch-"

  int             ShouldConnectToRemoteMaster;
  int             ConnectToRemoteMaster;
  char            RemoteMasterName[64];
  int             RemoteMasterPort;
  int             CopyAndStartRemoteMaster;
  char            RemoteMasterExecutable[MAX_PATH];
  char            RemoteMasterCopyTo[MAX_PATH];
  char*           RemoteMasterExecutableResoved;

  int             DontWriteBlackbardVariableInfosToIni;
  int             DontSaveBlackbardVariableInfosIniSection;

  int             RpcOverSocketOrNamedPipe;   // 0 -> Named pipe 1 -> Socket
  int             RpcDebugLoggingFlags;
  int             RpcSocketPort;

  int             StopScriptIfLabelDosNotExistInsideSVL;
  int             StopScriptIfLabelDosNotExistInsideSVLIni;

  unsigned int    ExternProcessLoginSocketPort;
  unsigned int    ExternProcessLoginSocketPortIni;

  int             RunningInsideAJob;   // is 1 if XilEnv running inside a job, than it can be terminated if the job will be closed

  int             TerminateSchedulerExternProcessTimeout;

  int             WriteMemoryLeakFile;

  int             ShouldUseDarkModeIni;
  int             DarkModeWasSetByUser;
  int             DarkMode;
  void*           QtApplicationPointer;

  int             RenameExecutableActive;
  char*           ConfigurablePrefix[32];

  struct {
      char* From;
      char *To;
  } *             RenameProcessFromTo;
  int             RenameProcessFromToSize;
  int             RenameProcessFromToPos;

  int             EnableLegacyEnvironmentVariables;

  char*           IniFilterProgram;
  int             IniFilterProgramFlags;
#define INI_FILTER_PROGRAM_INPUT                   0x1
#define INI_FILTER_PROGRAM_OUTPUT                  0x2
#define INI_FILTER_PROGRAM_NO_FILE_VERSION_CKECK   0x8

} MAIN_INI_VAL;


extern MAIN_INI_VAL s_main_ini_val;
extern int32_t CycleCounterVid;
extern int DisplayUnitForNonePhysicalValues;

const char *RenameProcessByBasicSettingsTable(const char *par_From);

int ReadBasicConfigurationFromIni(MAIN_INI_VAL  *sp_main_ini);
int WriteBasicConfigurationToIni(MAIN_INI_VAL  *sp_main_ini);

double GetSchedulingPeriode (void);

int SetProjectName (char *Name);

char *GetRemoteMasterExecutable(void);

void SetExternProcessLoginSocketPort(const char *PortString);
unsigned int GetExternProcessLoginSocketPort(void);
int IsExternProcessLoginSocketPortConfigured(void);

void InitMainSettings(void);
void SetProgramNameIntoMainSettings(const char *par_ProgramName);
void SetIniFilterProgram(const char *par_FilterProgramName, int par_Flags);
void SetDarkModeIntoMainSettings(int par_DarkMode);
void SetEnableLegacyEnvironmentVariables(void);

#endif
