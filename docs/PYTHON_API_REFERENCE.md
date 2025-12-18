# openXilEnv Python API

Welcome to the **openXilEnv Python API**.

## Table of Contents

- [Getting Started](#getting-started)
- [Example Usage](#example-usage)
- [API Reference](#api-reference)
    - [API Query](#api-query)
    - [Connection](#connection)
    - [Scheduler](#scheduler)
    - [Internal Processes](#internal-processes)
    - [GUI](#gui)
    - [Blackboard](#blackboard)
    - [Calibration](#calibration)
    - [CAN](#can)
    - [CCP](#ccp)
    - [XCP](#xcp)
    - [CAN bit error](#can-bit-error)
    - [CAN Recorder](#can-recorder)
    - [A2lLink](#a2llink)

## Getting Started

We recommend to use python in a virtual environment.
Open a terminal from the root.

To get set-up, make sure python is accessable via CLI.


```cmd
python3 --version
```

First, create a virtual environment

```cmd
python3 -m venv .venv
```

Then activate it

```cmd
source .\venv\bin\activate     # Linux
.\venv\Scripts\activate        # windows
```

Then update and install all dependencies

```cmd
python3 -m pip install --upgrade pip
python3 -m pip install .\python-api
```

Note that **.\python-api** is a directory to the python-api folder inside this repository and not a PyPI package.

That's it. You are now ready to run your first example.

## Example Usage

The following is a minimal example how to start **openXilEnv** from a python script.

```python
from pathlib import Path
from openXilEnv import XilEnv

xilEnv_insatllation_path = Path("path/to/opeXilEnv")
ini_file_path = Path("path/to/an/ini_file.ini")

# initialize xil
xil = XilEnv(xilEnv_installation_path)

# start openXilEnv with gui
xil.startWithGui(ini_file_path)

# verify connection
print(xil.isConnected()) # >>> True
```

## API Reference

The following documents the API of **xilEnv**.

### API Query

---

#### GetAPIVersion

##### Signature

```python
def GetAPIVersion(self) -> int:
```

##### Description

Get the API version of the loaded openXilEnv library.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| int | API version number. |

---

#### GetAPIAlternativeVersion

##### Signature

```python
def GetAPIAlternativeVersion(self) -> int:
```

##### Description

Get the alternative API version of the loaded openXilEnv library.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Alternative API version number. |

---

#### GetAPIModulePath

##### Signature

```python
def GetAPIModulePath(self) -> bytes:
```

##### Description

Get the file system path to the loaded openXilEnv API module.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| bytes | Path to the API module as a bytes object. |

---

### Connection

---

#### startWithGui

##### Signature

```python
def startWithGui(self, iniFilePath: str, timeoutInSec: int = 5) -> None:
```

##### Description

Start openXilEnv with the GUI using the specified INI file.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| iniFilePath | str | Path to the INI configuration file. |
| timeoutInSec | int, optional | Number of seconds to wait for connection (default is 5). |

##### Return

None

---

#### startWithoutGui

##### Signature

```python
def startWithoutGui(self, iniFilePath: str, timeoutInSec: int = 5) -> None:
```

##### Description

Start openXilEnv without the GUI using the specified INI file.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| iniFilePath | str | Path to the INI configuration file. |
| timeoutInSec | int, optional | Number of seconds to wait for connection (default is 5). |

##### Return

None

---

#### disconnectAndCloseXil

##### Signature

```python
def disconnectAndCloseXil(self) -> None:
```

##### Description

Disconnect from openXilEnv and close the process, cleaning up attached variables.

##### Parameter

None

##### Return

None

---

#### ConnectTo

##### Signature

```python
def ConnectTo(self, Address: str) -> int:
```

##### Description

Connect to a remote XilEnv instance by address.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Address | str | Hostname or IP address of the remote XilEnv instance. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if connection is successful |
| -1 | otherwise |

---

#### ConnectToInstance

##### Signature

```python
def ConnectToInstance(self, Address: str, Instance: str) -> int:
```

##### Description

Connect to a named XilEnv instance at a given address.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Address | str | Hostname or IP address of the remote XilEnv instance. |
| Instance | str | Name of the XilEnv instance to connect to. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if connection is successful |
| -1 | otherwise |

---

#### isConnected

##### Signature

```python
def isConnected(self) -> bool:
```

##### Description

Check if connected to a XilEnv instance.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| True | if connected |
| False | otherwise |

---

#### DisconnectFrom

##### Signature

```python
def DisconnectFrom(self) -> int:
```

##### Description

Disconnect from the current XilEnv instance.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if disconnection is successful |
| -1 | otherwise |

---

#### DisconnectAndClose

##### Signature

```python
def DisconnectAndClose(self, SetErrorLevelFlag: int, ErrorLevel: int) -> int:
```

##### Description

Disconnect and close the XilEnv instance.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| SetErrorLevelFlag | int | If nonzero, set the error level after disconnecting. |
| ErrorLevel | int | Error level to set if SetErrorLevelFlag is nonzero. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if disconnection is successful |
| -1 | otherwise |

---

#### CreateFileWithContent

##### Signature

```python
def CreateFileWithContent(self, Filename: str, Content: str) -> int:
```

##### Description

Create a file with the specified content on the remote XilEnv system.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Filename | str | A Path to the file to create. |
| Content | str | Content to write into the file. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### GetEnvironVar

##### Signature

```python
def GetEnvironVar(self, EnvironVar: str) -> str:
```

##### Description

Get the value of an environment variable from the remote XilEnv system.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| EnvironVar | str | Name of the environment variable. |

##### Return

| Return Value | Description |
|--------------|-------------|
| str | Value of the environment variable, or an empty string if not connected. |

---

#### SetEnvironVar

##### Signature

```python
def SetEnvironVar(self, EnvironVar: str, EnvironValue: str) -> int:
```

##### Description

Set the value of an environment variable on the remote XilEnv system.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| EnvironVar | str | Name of the environment variable. |
| EnvironValue | str | Value to set for the environment variable. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### ChangeSettings

##### Signature

```python
def ChangeSettings(self, SettingName: str, SettingValue: str) -> int:
```

##### Description

Change a setting on the remote XilEnv system.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| SettingName | str | Name of the setting to change. |
| SettingValue | str | Value to set for the setting. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### TextOut

##### Signature

```python
def TextOut(self, TextOut: str) -> int:
```

##### Description

Output text to the remote XilEnv system's standard output.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| TextOut | str | Text to output. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### ErrorTextOut

##### Signature

```python
def ErrorTextOut(self, ErrLevel: int, TextOut: str) -> int:
```

##### Description

Output error text to the remote XilEnv system's standard error with a specified error level.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| ErrLevel | int | Error level for the output. |
| TextOut | str | Error text to output. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

### Scheduler

---

#### disableRealTimeFactorSwitch

##### Signature

```python
def disableRealTimeFactorSwitch(self) -> None:
```

##### Description

Switch realtime simulation speed off. openXilEnv will simulate as fast as possible.

##### Parameter

None

##### Return

None

---

#### enableRealTimeFactorSwitch

##### Signature

```python
def enableRealTimeFactorSwitch(self) -> None:
```

##### Description

Switch realtime simulation speed on. openXilEnv tries to match simulation speed to real time.

##### Parameter

None

##### Return

None

---

#### StopScheduler

##### Signature

```python
def StopScheduler(self) -> None:
```

##### Description

Stop the XilEnv scheduler if connected.

##### Parameter

None

##### Return

None

---

#### ContinueScheduler

##### Signature

```python
def ContinueScheduler(self) -> None:
```

##### Description

Continue the XilEnv scheduler if connected.

##### Parameter

None

##### Return

None

---

#### SchedulerRunning

##### Signature

```python
def SchedulerRunning(self) -> int:
```

##### Description

Check if the XilEnv scheduler is running.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 1 | if running |
| 0 | if stopped |
| -1 | if not connected |

---

#### StartProcess

##### Signature

```python
def StartProcess(self, Name: str) -> int:
```

##### Description

Start a process by name on the remote XilEnv system.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Name | str | Name of the process to start. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### StartProcessAndLoadSvl

##### Signature

```python
def StartProcessAndLoadSvl(self, ProcessName: str, SvlName: str) -> int:
```

##### Description

Start a process and load an SVL file on the remote XilEnv system.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| ProcessName | str | Name of the process to start. |
| SvlName | str | Name of the SVL file to load. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### StartProcessEx

##### Signature

```python
def StartProcessEx(
    self,
    ProcessName: str,
    Prio: int,
    Cycle: int,
    Delay: int,
    Timeout: int,
    SVLFile: str,
    BBPrefix: str,
    UseRangeControl: int,
    RangeControlBeforeActiveFlags: int,
    RangeControlBehindActiveFlags: int,
    RangeControlStopSchedFlag: int,
    RangeControlOutput: int,
    RangeErrorCounterFlag: int,
    RangeErrorCounter: str,
    RangeControlVarFlag: int,
    RangeControl: str,
    RangeControlPhysFlag: int,
    RangeControlLimitValues: int
) -> int:
```

##### Description

Start a process with extended options on the remote XilEnv system.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| ProcessName | str | Name of the process to start. |
| Prio | int | Priority of the process. |
| Cycle | int | Cycle time. |
| Delay | int | Delay before start. |
| Timeout | int | Timeout for the process. |
| SVLFile | str | SVL file to load. |
| BBPrefix | str | Blackboard prefix. |
| UseRangeControl | int | Use range control flag. |
| RangeControlBeforeActiveFlags | int | Range control before active flags. |
| RangeControlBehindActiveFlags | int | Range control behind active flags. |
| RangeControlStopSchedFlag | int | Range control stop scheduler flag. |
| RangeControlOutput | int | Range control output flag. |
| RangeErrorCounterFlag | int | Range error counter flag. |
| RangeErrorCounter | str | Range error counter variable. |
| RangeControlVarFlag | int | Range control variable flag. |
| RangeControl | str | Range control variable. |
| RangeControlPhysFlag | int | Range control physical flag. |
| RangeControlLimitValues | int | Range control limit values flag. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### StartProcessEx2

##### Signature

```python
def StartProcessEx2(
    self,
    ProcessName: str,
    Prio: int,
    Cycle: int,
    Delay: int,
    Timeout: int,
    SVLFile: str,
    A2LFile: str,
    A2LFlags: int,
    BBPrefix: str,
    UseRangeControl: int,
    RangeControlBeforeActiveFlags: int,
    RangeControlBehindActiveFlags: int,
    RangeControlStopSchedFlag: int,
    RangeControlOutput: int,
    RangeErrorCounterFlag: int,
    RangeErrorCounter: str,
    RangeControlVarFlag: int,
    RangeControl: str,
    RangeControlPhysFlag: int,
    RangeControlLimitValues: int
) -> int:
```

##### Description

Start a process with extended options and A2L file on the remote XilEnv system.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| ProcessName | str | Name of the process to start. |
| Prio | int | Priority of the process. |
| Cycle | int | Cycle time. |
| Delay | int | Delay before start. |
| Timeout | int | Timeout for the process. |
| SVLFile | str | SVL file to load. |
| A2LFile | str | A2L file to load. |
| A2LFlags | int | A2L flags. |
| BBPrefix | str | Blackboard prefix. |
| UseRangeControl | int | Use range control flag. |
| RangeControlBeforeActiveFlags | int | Range control before active flags. |
| RangeControlBehindActiveFlags | int | Range control behind active flags. |
| RangeControlStopSchedFlag | int | Range control stop scheduler flag. |
| RangeControlOutput | int | Range control output flag. |
| RangeErrorCounterFlag | int | Range error counter flag. |
| RangeErrorCounter | str | Range error counter variable. |
| RangeControlVarFlag | int | Range control variable flag. |
| RangeControl | str | Range control variable. |
| RangeControlPhysFlag | int | Range control physical flag. |
| RangeControlLimitValues | int | Range control limit values flag. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### StopProcess

##### Signature

```python
def StopProcess(self, Name: str) -> int:
```

##### Description

Stop a process by name on the remote XilEnv system.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Name | str | Name of the process to stop. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### GetNextProcess

##### Signature

```python
def GetNextProcess(self, Flag: int, Filter: str) -> str | int:
```

##### Description

Get the next process matching the filter and flag.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Flag | int | Filter flag for process selection. |
| Filter | str | Filter string for process selection. |

##### Return

| Return Value | Description |
|--------------|-------------|
| str | Name of the next process as a string |
| -1 | if not connected |

---

#### GetProcessState

##### Signature

```python
def GetProcessState(self, Name: str) -> int:
```

##### Description

Get the state of a process by name.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Name | str | Name of the process. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | State of the process |
| -1 | if not connected |

---

#### DoNextCycles

##### Signature

```python
def DoNextCycles(self, Cycles: int) -> None:
```

##### Description

Execute the next N scheduler cycles.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Cycles | int | Number of cycles to execute. |

##### Return

None

---

#### DoNextCyclesAndWait

##### Signature

```python
def DoNextCyclesAndWait(self, Cycles: int) -> None:
```

##### Description

Execute the next N scheduler cycles and wait for completion.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Cycles | int | Number of cycles to execute. |

##### Return

None

---

#### AddBeforeProcessEquationFromFile

##### Signature

```python
def AddBeforeProcessEquationFromFile(self, Nr: int, ProcName: str, EquFile: str) -> int:
```

##### Description

Add an equation from file to be executed before a process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Nr | int | Equation number or identifier. |
| ProcName | str | Name of the process. |
| EquFile | str | Path to the equation file. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### AddBehindProcessEquationFromFile

##### Signature

```python
def AddBehindProcessEquationFromFile(self, Nr: int, ProcName: str, EquFile: str) -> int:
```

##### Description

Add an equation from file to be executed after a process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Nr | int | Equation number or identifier. |
| ProcName | str | Name of the process. |
| EquFile | str | Path to the equation file. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### DelBeforeProcessEquations

##### Signature

```python
def DelBeforeProcessEquations(self, Nr: int, ProcName: str) -> None:
```

##### Description

Delete all equations to be executed before a process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Nr | int | Equation number or identifier. |
| ProcName | str | Name of the process. |

##### Return

None

---

#### DelBehindProcessEquations

##### Signature

```python
def DelBehindProcessEquations(self, Nr: int, ProcName: str) -> None:
```

##### Description

Delete all equations to be executed after a process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Nr | int | Equation number or identifier. |
| ProcName | str | Name of the process. |

##### Return

None

---

#### WaitUntil

##### Signature

```python
def WaitUntil(self, Equation: str, Cycles: int) -> int:
```

##### Description

Wait until the given equation is true or a number of cycles have passed.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Equation | str | Equation to evaluate. |
| Cycles | int | Maximum number of cycles to wait. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if the condition was met |
| -1 | otherwise |

---

### Internal Processes

---

#### StartScript

##### Signature

```python
def StartScript(self, ScriptFile: str) -> int:
```

##### Description

Start a script on the remote XilEnv system.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| ScriptFile | str | Path to the script file to start. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### StopScript

##### Signature

```python
def StopScript(self) -> int:
```

##### Description

Stop the currently running script on the remote XilEnv system.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### StartRecorder

##### Signature

```python
def StartRecorder(self, CfgFile: str) -> int:
```

##### Description

Start the recorder with the specified configuration file.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| CfgFile | str | Path to the recorder configuration file. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### RecorderAddComment

##### Signature

```python
def RecorderAddComment(self, Comment: str) -> int:
```

##### Description

Add a comment to the recorder output.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Comment | str | Comment text to add. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### StopRecorder

##### Signature

```python
def StopRecorder(self) -> int:
```

##### Description

Stop the recorder on the remote XilEnv system.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### StartPlayer

##### Signature

```python
def StartPlayer(self, CfgFile: str) -> int:
```

##### Description

Start the player with the specified configuration file.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| CfgFile | str | Path to the player configuration file. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### StopPlayer

##### Signature

```python
def StopPlayer(self) -> int:
```

##### Description

Stop the player on the remote XilEnv system.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### StartEquations

##### Signature

```python
def StartEquations(self, EquFile: str) -> int:
```

##### Description

Start equations from a specified file on the remote XilEnv system.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| EquFile | str | Path to the equation file to load and start. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### StopEquations

##### Signature

```python
def StopEquations(self) -> int:
```

##### Description

Stops the currently running equations in the openXilEnv instance.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### StartGenerator

##### Signature

```python
def StartGenerator(self, GenFile: str) -> int:
```

##### Description

Starts the generator using the specified generator file.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| GenFile | str | Path to the generator configuration file. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### StopGenerator

##### Signature

```python
def StopGenerator(self) -> int:
```

##### Description

Stops the currently running generator in the openXilEnv instance.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

### GUI

---

#### LoadDesktop

##### Signature

```python
def LoadDesktop(self, DskFile: str) -> int:
```

##### Description

Load a desktop layout from a file.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| DskFile | str | Path to the desktop file. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### SaveDesktop

##### Signature

```python
def SaveDesktop(self, DskFile: str) -> int:
```

##### Description

Save the current desktop layout to a file.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| DskFile | str | Path to the desktop file. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### ClearDesktop

##### Signature

```python
def ClearDesktop(self) -> int:
```

##### Description

Clear the current desktop layout.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### CreateDialog

##### Signature

```python
def CreateDialog(self, DialogName: str) -> int:
```

##### Description

Create a dialog window.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| DialogName | str | Name of the dialog. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### AddDialogItem

##### Signature

```python
def AddDialogItem(self, Description: str, VariName: str) -> int:
```

##### Description

Add an item to a dialog window.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Description | str | Description of the dialog item. |
| VariName | str | Name of the variable associated with the item. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### ShowDialog

##### Signature

```python
def ShowDialog(self) -> int:
```

##### Description

Show the created dialog window.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### IsDialogClosed

##### Signature

```python
def IsDialogClosed(self) -> int:
```

##### Description

Check if the dialog window is closed.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 1 | if closed |
| 0 | if open |
| -1 | if not connected or on failure |

---

#### SelectSheet

##### Signature

```python
def SelectSheet(self, SheetName: str) -> int:
```

##### Description

Select a sheet by name.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| SheetName | str | Name of the sheet to select. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### AddSheet

##### Signature

```python
def AddSheet(self, SheetName: str) -> int:
```

##### Description

Add a new sheet by name.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| SheetName | str | Name of the sheet to add. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### DeleteSheet

##### Signature

```python
def DeleteSheet(self, SheetName: str) -> int:
```

##### Description

Delete a sheet by name.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| SheetName | str | Name of the sheet to delete. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### RenameSheet

##### Signature

```python
def RenameSheet(self, OldSheetName: str, NewSheetName: str) -> int:
```

##### Description

Rename a sheet.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| OldSheetName | str | Current name of the sheet. |
| NewSheetName | str | New name for the sheet. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### OpenWindow

##### Signature

```python
def OpenWindow(self, WindowName: str) -> int:
```

##### Description

Open a window by name.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| WindowName | str | Name of the window to open. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### CloseWindow

##### Signature

```python
def CloseWindow(self, WindowName: str) -> int:
```

##### Description

Close a window by name.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| WindowName | str | Name of the window to close. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### DeleteWindow

##### Signature

```python
def DeleteWindow(self, WindowName: str) -> int:
```

##### Description

Delete a window by name.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| WindowName | str | Name of the window to delete. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### ImportWindow

##### Signature

```python
def ImportWindow(self, WindowName: str, FileName: str) -> int:
```

##### Description

Import a window from a file.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| WindowName | str | Name of the window. |
| FileName | str | Path to the file to import. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

#### ExportWindow

##### Signature

```python
def ExportWindow(self, WindowName: str, FileName: str) -> int:
```

##### Description

Export a window to a file.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| WindowName | str | Name of the window. |
| FileName | str | Path to the file to export. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | if not connected or on failure |

---

### Blackboard

---

#### readMultipleSignals

##### Signature

```python
def readMultipleSignals(self, signalNames: list[str]) -> list[float]:
```

##### Description

Read multiple signal values by their names.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| signalNames | list[str] | List of signal names to read. |

##### Return

| Return Value | Description |
|--------------|-------------|
| list[float] | List of signal values. |

---

#### readSignal

##### Signature

```python
def readSignal(self, signalName: str) -> float:
```

##### Description

Read a single signal value by its name.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| signalName | str | Name of the signal to read. |

##### Return

| Return Value | Description |
|--------------|-------------|
| float | Value of the signal. |

---

#### writeMultipleSignals

##### Signature

```python
def writeMultipleSignals(self, signalNames: list[str], signalValues: list[float]) -> None:
```

##### Description

Write multiple signal values by their names.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| signalNames | list[str] | List of signal names to write. |
| signalValues | list[float] | List of values to write. |

##### Return

None

---

#### writeSignal

##### Signature

```python
def writeSignal(self, signalName: str, rawValue: float) -> None:
```

##### Description

Write a single signal value by its name.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| signalName | str | Name of the signal to write. |
| rawValue | float | Value to write. |

##### Return

None

---

#### attachVariables

##### Signature

```python
def attachVariables(self, signalNames: list[str]) -> None:
```

##### Description

Attach a list of signal names to the internal variable dictionary.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| signalNames | list[str] | List of signal names to attach. |

##### Return

None

---

#### AddVari

##### Signature

```python
def AddVari(self, Label: str, Type: int, Unit: str) -> int:
```

##### Description

Add a variable to the blackboard.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Label | str | Name of the variable. |
| Type | int | Type identifier. |
| Unit | str | Unit of the variable. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Variable ID if successful |
| -1 | otherwise |

---

#### RemoveVari

##### Signature

```python
def RemoveVari(self, Vid: int) -> int:
```

##### Description

Remove a variable from the blackboard by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### AttachVari

##### Signature

```python
def AttachVari(self, Label: str) -> int:
```

##### Description

Attach a variable by its name and return its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Label | str | Name of the variable to attach. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Variable ID if successful |
| -1 | otherwise |

---

#### Get

##### Signature

```python
def Get(self, Vid: int) -> float:
```

##### Description

Get the value of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |

##### Return

| Return Value | Description |
|--------------|-------------|
| float | Value of the variable, or 0 if not connected. |

---

#### GetPhys

##### Signature

```python
def GetPhys(self, Vid: int) -> float:
```

##### Description

Get the physical value of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |

##### Return

| Return Value | Description |
|--------------|-------------|
| float | Physical value of the variable, or 0 if not connected. |

---

#### Set

##### Signature

```python
def Set(self, Vid: int, Value: float) -> None:
```

##### Description

Set the value of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |
| Value | float | Value to set. |

##### Return

None

---

#### SetPhys

##### Signature

```python
def SetPhys(self, Vid: int, Value: float) -> int:
```

##### Description

Set the physical value of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |
| Value | float | Physical value to set. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### WriteFrame

##### Signature

```python
def WriteFrame(self, vidList: list[int], valueList: list[float]) -> int:
```

##### Description

Write a frame of values to multiple variables by their IDs.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| vidList | list[int] | List of variable IDs. |
| valueList | list[float] | List of values to write. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### ReadFrame

##### Signature

```python
def ReadFrame(self, vidList: list[int]) -> list[float]:
```

##### Description

Read a frame of values from multiple variables by their IDs.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| vidList | list[int] | List of variable IDs. |

##### Return

| Return Value | Description |
|--------------|-------------|
| list[float] | List of variable values |
| -1 | if not connected |

---

#### Equ

##### Signature

```python
def Equ(self, Equ: str) -> float:
```

##### Description

Evaluate an equation string in the XilEnv context.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Equ | str | Equation string to evaluate. |

##### Return

| Return Value | Description |
|--------------|-------------|
| float | Result of the evaluation, or 0 if not connected. |

---

#### WrVariEnable

##### Signature

```python
def WrVariEnable(self, Label: str, Process: str) -> int:
```

##### Description

Enable write access for a variable in a process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Label | str | Variable name. |
| Process | str | Process name. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### WrVariDisable

##### Signature

```python
def WrVariDisable(self, Label: str, Process: str) -> int:
```

##### Description

Disable write access for a variable in a process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Label | str | Variable name. |
| Process | str | Process name. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### IsWrVariEnabled

##### Signature

```python
def IsWrVariEnabled(self, Label: str, Process: str) -> int:
```

##### Description

Check if write access is enabled for a variable in a process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Label | str | Variable name. |
| Process | str | Process name. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 1 | if enabled |
| 0 | if disabled |
| -1 | if not connected |

---

#### LoadRefList

##### Signature

```python
def LoadRefList(self, RefList: str, Process: str) -> int:
```

##### Description

Load a reference list for a process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| RefList | str | Reference list name. |
| Process | str | Process name. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### AddRefList

##### Signature

```python
def AddRefList(self, RefList: str, Process: str) -> int:
```

##### Description

Add a reference list to a process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| RefList | str | Reference list name. |
| Process | str | Process name. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### SaveRefList

##### Signature

```python
def SaveRefList(self, RefList: str, Process: str) -> int:
```

##### Description

Save a reference list for a process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| RefList | str | Reference list name. |
| Process | str | Process name. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### GetVariConversionType

##### Signature

```python
def GetVariConversionType(self, Vid: int) -> int:
```

##### Description

Get the conversion type of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Conversion type if successful |
| -1 | otherwise |

---

#### GetVariConversionString

##### Signature

```python
def GetVariConversionString(self, Vid: int) -> str:
```

##### Description

Get the conversion string of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |

##### Return

| Return Value | Description |
|--------------|-------------|
| str | Conversion string if successful, empty string otherwise. |

---

#### GetVariType

##### Signature

```python
def GetVariType(self, Vid: int) -> int:
```

##### Description

Get the type of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Variable type if successful |
| -1 | otherwise |

---

#### GetVariUnit

##### Signature

```python
def GetVariUnit(self, Vid: int) -> str:
```

##### Description

Get the unit of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |

##### Return

| Return Value | Description |
|--------------|-------------|
| str | Unit string if successful, empty string otherwise. |

---

#### SetVariUnit

##### Signature

```python
def SetVariUnit(self, Vid: int, Unit: str) -> int:
```

##### Description

Set the unit of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |
| Unit | str | Unit string to set. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### GetVariMin

##### Signature

```python
def GetVariMin(self, Vid: int) -> float:
```

##### Description

Get the minimum value of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |

##### Return

| Return Value | Description |
|--------------|-------------|
| float | Minimum value if successful, 0 otherwise. |

---

#### GetVariMax

##### Signature

```python
def GetVariMax(self, Vid: int) -> float:
```

##### Description

Get the maximum value of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |

##### Return

| Return Value | Description |
|--------------|-------------|
| float | Maximum value if successful, 0 otherwise. |

---

#### SetVariMin

##### Signature

```python
def SetVariMin(self, Vid: int, Min: float) -> int:
```

##### Description

Set the minimum value of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |
| Min | float | Minimum value to set. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### SetVariMax

##### Signature

```python
def SetVariMax(self, Vid: int, Max: float) -> int:
```

##### Description

Set the maximum value of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |
| Max | float | Maximum value to set. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### GetNextVari

##### Signature

```python
def GetNextVari(self, Flag: int, Filter: str) -> str:
```

##### Description

Get the next variable matching a filter and flag.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Flag | int | Filter flag. |
| Filter | str | Filter string. |

##### Return

| Return Value | Description |
|--------------|-------------|
| str | Variable name if found, empty string otherwise. |

---

#### GetNextVariEx

##### Signature

```python
def GetNextVariEx(self, Flag: int, Filter: str, Process: str, AccessFlags: int) -> str:
```

##### Description

Get the next variable matching a filter, process, and access flags.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Flag | int | Filter flag. |
| Filter | str | Filter string. |
| Process | str | Process name. |
| AccessFlags | int | Access flags. |

##### Return

| Return Value | Description |
|--------------|-------------|
| str | Variable name if found, empty string otherwise. |

---

#### GetVariEnum

##### Signature

```python
def GetVariEnum(self, Flag: int, Vid: int, Value: float) -> str:
```

##### Description

Get the enumeration string for a variable value.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Flag | int | Enumeration flag. |
| Vid | int | Variable ID. |
| Value | float | Value to enumerate. |

##### Return

| Return Value | Description |
|--------------|-------------|
| str | Enumeration string if found, empty string otherwise. |

---

#### GetVariDisplayFormatWidth

##### Signature

```python
def GetVariDisplayFormatWidth(self, Vid: int) -> int:
```

##### Description

Get the display format width for a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Display format width if successful |
| -1 | otherwise |

---

#### GetVariDisplayFormatPrec

##### Signature

```python
def GetVariDisplayFormatPrec(self, Vid: int) -> int:
```

##### Description

Get the display format precision for a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Display format precision if successful |
| -1 | otherwise |

---

#### SetVariDisplayFormat

##### Signature

```python
def SetVariDisplayFormat(self, Vid: int, Width: int, Prec: int) -> int:
```

##### Description

Set the display format width and precision for a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |
| Width | int | Display width. |
| Prec | int | Display precision. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### ImportVariProperties

##### Signature

```python
def ImportVariProperties(self, Filename: str) -> int:
```

##### Description

Import variable properties from a file.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Filename | str | Path to the properties file. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### EnableRangeControl

##### Signature

```python
def EnableRangeControl(self, ProcessNameFilter: str, VariableNameFilter: str) -> int:
```

##### Description

Enable range control for a variable in a process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| ProcessNameFilter | str | Process name filter. |
| VariableNameFilter | str | Variable name filter. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### DisableRangeControl

##### Signature

```python
def DisableRangeControl(self, ProcessNameFilter: str, VariableNameFilter: str) -> int:
```

##### Description

Disable range control for a variable in a process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| ProcessNameFilter | str | Process name filter. |
| VariableNameFilter | str | Variable name filter. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### ReferenceSymbol

##### Signature

```python
def ReferenceSymbol(self, Symbol: str, DisplayName: str, Process: str, Unit: str, ConversionType: int, Conversion: str, Min: float, Max: float, Color: int, Width: int, Precision: int, Flags: int) -> int:
```

##### Description

Reference a symbol with detailed properties.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Symbol | str | Symbol name. |
| DisplayName | str | Display name. |
| Process | str | Process name. |
| Unit | str | Unit string. |
| ConversionType | int | Conversion type. |
| Conversion | str | Conversion string. |
| Min | float | Minimum value. |
| Max | float | Maximum value. |
| Color | int | Color code. |
| Width | int | Display width. |
| Precision | int | Display precision. |
| Flags | int | Additional flags. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### DereferenceSymbol

##### Signature

```python
def DereferenceSymbol(self, Symbol: str, DisplayName: str, Process: str, Flags: int) -> int:
```

##### Description

Dereference a symbol by its properties.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Symbol | str | Symbol name. |
| DisplayName | str | Display name. |
| Process | str | Process name. |
| Flags | int | Additional flags. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### GetRaw

##### Signature

```python
def GetRaw(self, Vid: int, RetValue: 'BB_VARI') -> int:
```

##### Description

Get the raw value of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |
| RetValue | BB_VARI | BB_VARI structure to receive the value. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### SetRaw

##### Signature

```python
def SetRaw(self, Vid: int, Type: int, Value: 'BB_VARI', Flag: int) -> int:
```

##### Description

Set the raw value of a variable by its ID.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Vid | int | Variable ID. |
| Type | int | Data type. |
| Value | BB_VARI | BB_VARI structure with the value to set. |
| Flag | int | Additional flags. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

### Calibration

---

#### LoadSvl

##### Signature

```python
def LoadSvl(self, SvlFile: str, Process: str) -> int:
```

##### Description

Load an SVL file for a given process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| SvlFile | str | Path to the SVL file. |
| Process | str | Process name. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### SaveSvl

##### Signature

```python
def SaveSvl(self, SvlFile: str, Process: str, Filter: str) -> int:
```

##### Description

Save an SVL file for a given process and filter.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| SvlFile | str | Path to the SVL file. |
| Process | str | Process name. |
| Filter | str | Filter string. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### SaveSal

##### Signature

```python
def SaveSal(self, SalFile: str, Process: str, Filter: str) -> int:
```

##### Description

Save a SAL file for a given process and filter.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| SalFile | str | Path to the SAL file. |
| Process | str | Process name. |
| Filter | str | Filter string. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### GetSymbolRaw

##### Signature

```python
def GetSymbolRaw(self, Symbol: str, Process: str, Flags: int, RetValue: 'BB_VARI') -> int:
```

##### Description

Get the raw value of a symbol for a given process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Symbol | str | Symbol name. |
| Process | str | Process name. |
| Flags | int | Flags for the operation. |
| RetValue | BB_VARI | BB_VARI structure to receive the value. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Data type if successful |
| -1 | otherwise |

---

#### GetSymbolRawEx

##### Signature

```python
def GetSymbolRawEx(self, Symbol: str, Process: str):
```

##### Description

Get the raw value of a symbol for a given process and return the Python value.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Symbol | str | Symbol name. |
| Process | str | Process name. |

##### Return

| Return Value | Description |
|--------------|-------------|
| value | The value of the symbol in the appropriate Python type |
| None | if not connected or unknown type |

---

#### SetSymbolRaw

##### Signature

```python
def SetSymbolRaw(self, Symbol: str, Process: str, Flags: int, DataType: int, Value) -> int:
```

##### Description

Set the raw value of a symbol for a given process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Symbol | str | Symbol name. |
| Process | str | Process name. |
| Flags | int | Flags for the operation. |
| DataType | int | Data type identifier. |
| Value | - | Value to set (type depends on DataType). |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | if successful |
| -1 | otherwise |

---

#### SetSymbolRawEx

##### Signature

```python
def SetSymbolRawEx(self, Symbol: str, Process: str, Value) -> None:
```

##### Description

Set the raw value of a symbol for a given process using Python value type.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Symbol | str | Symbol name. |
| Process | str | Process name. |
| Value | - | Value to set (type will be mapped to BB_VARI). |

##### Return

None

---

### CAN

---

#### LoadCanVariante

##### Signature

```python
def LoadCanVariante(self, CanFile: str, Channel: int) -> int:
```

##### Description

Load a CAN variant file for a specific channel.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| CanFile | str | Path to the CAN variant file. |
| Channel | int | Channel number. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### LoadAndSelCanVariante

##### Signature

```python
def LoadAndSelCanVariante(self, CanFile: str, Channel: int) -> int:
```

##### Description

Load and select a CAN variant file for a specific channel.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| CanFile | str | Path to the CAN variant file. |
| Channel | int | Channel number. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### AppendCanVariante

##### Signature

```python
def AppendCanVariante(self, CanFile: str, Channel: int) -> int:
```

##### Description

Append a CAN variant file to a specific channel.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| CanFile | str | Path to the CAN variant file. |
| Channel | int | Channel number. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### DelAllCanVariants

##### Signature

```python
def DelAllCanVariants(self) -> None:
```

##### Description

Delete all loaded CAN variants.

##### Parameter

None

##### Return

None

---

#### SetCanChannelCount

##### Signature

```python
def SetCanChannelCount(self, ChannelCount: int) -> int:
```

##### Description

Set the number of CAN channels.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| ChannelCount | int | Number of CAN channels. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### SetCanChannelStartupState

##### Signature

```python
def SetCanChannelStartupState(self, Channel: int, State: int) -> int:
```

##### Description

Set the startup state for a CAN channel.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Channel | int | Channel number. |
| State | int | Startup state value. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### TransmitCAN

##### Signature

```python
def TransmitCAN(
    self,
    Id: int,
    Ext: int,
    Size: int,
    Data0: int,
    Data1: int,
    Data2: int,
    Data3: int,
    Data4: int,
    Data5: int,
    Data6: int,
    Data7: int
) -> int:
```

##### Description

Transmit a standard CAN message.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Id | int | CAN message ID. |
| Ext | int | Extended ID flag. |
| Size | int | Data length. |
| Data0-Data7 | int | Data bytes. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### OpenCANQueue

##### Signature

```python
def OpenCANQueue(self, Depth: int) -> int:
```

##### Description

Open a CAN message queue (FIFO).

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Depth | int | Queue depth. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Queue handle |
| -1 | on failure |

---

#### OpenCANFdQueue

##### Signature

```python
def OpenCANFdQueue(self, Depth: int, FdFlag: int) -> int:
```

##### Description

Open a CAN FD message queue (FIFO).

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Depth | int | Queue depth. |
| FdFlag | int | FD flag. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Queue handle |
| -1 | on failure |

---

#### SetCANAcceptanceWindows

##### Signature

```python
def SetCANAcceptanceWindows(self, Size: int, AcceptanceWindows) -> int:
```

##### Description

Set CAN acceptance windows.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Size | int | Number of acceptance windows. |
| AcceptanceWindows | - | Acceptance window configuration. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### FlushCANQueue

##### Signature

```python
def FlushCANQueue(self, Flags: int) -> int:
```

##### Description

Flush the CAN message queue.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Flags | int | Flush flags. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### ReadCANQueue

##### Signature

```python
def ReadCANQueue(self, ReadMaxElements: int, Elements) -> int:
```

##### Description

Read messages from the CAN queue.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| ReadMaxElements | int | Maximum number of elements to read. |
| Elements | - | Buffer to store read elements. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Number of elements read |
| -1 | on failure |

---

#### ReadCANFdQueue

##### Signature

```python
def ReadCANFdQueue(self, ReadMaxElements: int, Elements) -> int:
```

##### Description

Read messages from the CAN FD queue.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| ReadMaxElements | int | Maximum number of elements to read. |
| Elements | - | Buffer to store read elements. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Number of elements read |
| -1 | on failure |

---

#### TransmitCANQueue

##### Signature

```python
def TransmitCANQueue(self, WriteElements: int, Elements) -> int:
```

##### Description

Transmit messages to the CAN queue.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| WriteElements | int | Number of elements to write. |
| Elements | - | Buffer containing elements to transmit. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Number of elements written |
| -1 | on failure |

---

#### TransmitCANFdQueue

##### Signature

```python
def TransmitCANFdQueue(self, WriteElements: int, Elements) -> int:
```

##### Description

Transmit messages to the CAN FD queue.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| WriteElements | int | Number of elements to write. |
| Elements | - | Buffer containing elements to transmit. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Number of elements written |
| -1 | on failure |

---

#### CloseCANQueue

##### Signature

```python
def CloseCANQueue(self) -> int:
```

##### Description

Close the CAN message queue.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

### CCP

---

#### LoadCCPConfig

##### Signature

```python
def LoadCCPConfig(self, Connection: int, CcpFile: str) -> int:
```

##### Description

Load a CCP configuration file for a given connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |
| CcpFile | str | Path to the CCP configuration file. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StartCCPBegin

##### Signature

```python
def StartCCPBegin(self, Connection: int) -> int:
```

##### Description

Begin a CCP session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StartCCPAddVar

##### Signature

```python
def StartCCPAddVar(self, Connection: int, Variable: str) -> int:
```

##### Description

Add a variable to the CCP session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |
| Variable | str | Name of the variable to add. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StartCCPEnd

##### Signature

```python
def StartCCPEnd(self, Connection: int) -> int:
```

##### Description

End the CCP session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StopCCP

##### Signature

```python
def StopCCP(self, Connection: int) -> int:
```

##### Description

Stop the CCP session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StartCCPCalBegin

##### Signature

```python
def StartCCPCalBegin(self, Connection: int) -> int:
```

##### Description

Begin a CCP calibration session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StartCCPCalAddVar

##### Signature

```python
def StartCCPCalAddVar(self, Connection: int, Variable: str) -> int:
```

##### Description

Add a variable to the CCP calibration session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |
| Variable | str | Name of the variable to add. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StartCCPCalEnd

##### Signature

```python
def StartCCPCalEnd(self, Connection: int) -> int:
```

##### Description

End the CCP calibration session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StopCCPCal

##### Signature

```python
def StopCCPCal(self, Connection: int) -> int:
```

##### Description

Stop the CCP calibration session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

### XCP

---

#### LoadXCPConfig

##### Signature

```python
def LoadXCPConfig(self, Connection: int, XcpFile: str) -> int:
```

##### Description

Load an XCP configuration file for a given connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |
| XcpFile | str | Path to the XCP configuration file. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StartXCPBegin

##### Signature

```python
def StartXCPBegin(self, Connection: int) -> int:
```

##### Description

Begin an XCP session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StartXCPAddVar

##### Signature

```python
def StartXCPAddVar(self, Connection: int, Variable: str) -> int:
```

##### Description

Add a variable to the XCP session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |
| Variable | str | Name of the variable to add. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StartXCPEnd

##### Signature

```python
def StartXCPEnd(self, Connection: int) -> int:
```

##### Description

End the XCP session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StopXCP

##### Signature

```python
def StopXCP(self, Connection: int) -> int:
```

##### Description

Stop the XCP session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StartXCPCalBegin

##### Signature

```python
def StartXCPCalBegin(self, Connection: int) -> int:
```

##### Description

Begin an XCP calibration session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StartXCPCalAddVar

##### Signature

```python
def StartXCPCalAddVar(self, Connection: int, Variable: str) -> int:
```

##### Description

Add a variable to the XCP calibration session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |
| Variable | str | Name of the variable to add. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StartXCPCalEnd

##### Signature

```python
def StartXCPCalEnd(self, Connection: int) -> int:
```

##### Description

End the XCP calibration session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StopXCPCal

##### Signature

```python
def StopXCPCal(self, Connection: int) -> int:
```

##### Description

Stop the XCP calibration session for the specified connection.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Connection | int | Connection handle or identifier. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

### CAN bit error

---

#### SetCanErr

##### Signature

```python
def SetCanErr(self, Channel: int, Id: int, Startbit: int, Bitsize: int, ByteOrder: str, Cycles: int, BitErrValue: int) -> int:
```

##### Description

Inject a bit error into a CAN message by bit position.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Channel | int | CAN channel number. |
| Id | int | CAN message ID. |
| Startbit | int | Start bit position. |
| Bitsize | int | Number of bits to affect. |
| ByteOrder | str | Byte order (e.g., 'Intel' or 'Motorola'). |
| Cycles | int | Number of cycles to apply the error. |
| BitErrValue | int | Value to set for the bit error. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### SetCanErrSignalName

##### Signature

```python
def SetCanErrSignalName(self, Channel: int, Id: int, Signalname: str, Cycles: int, BitErrValue: int) -> int:
```

##### Description

Inject a bit error into a CAN message by signal name.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Channel | int | CAN channel number. |
| Id | int | CAN message ID. |
| Signalname | str | Name of the signal. |
| Cycles | int | Number of cycles to apply the error. |
| BitErrValue | int | Value to set for the bit error. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### ClearCanErr

##### Signature

```python
def ClearCanErr(self) -> int:
```

##### Description

Clear all CAN bit errors.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### SetCanSignalConversion

##### Signature

```python
def SetCanSignalConversion(self, Channel: int, Id: int, Signalname: str, Conversion: str) -> int:
```

##### Description

Set a conversion formula for a CAN signal.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Channel | int | CAN channel number. |
| Id | int | CAN message ID. |
| Signalname | str | Name of the signal. |
| Conversion | str | Conversion formula or string. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### ResetCanSignalConversion

##### Signature

```python
def ResetCanSignalConversion(self, Channel: int, Id: int, Signalname: str) -> int:
```

##### Description

Reset the conversion formula for a specific CAN signal.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Channel | int | CAN channel number. |
| Id | int | CAN message ID. |
| Signalname | str | Name of the signal. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

### CAN Recorder

---

#### StartCANRecorder

##### Signature

```python
def StartCANRecorder(
    self,
    FileName: str,
    TriggerEqu: str,
    DisplayColumnCounterFlag: int,
    DisplayColumnTimeAbsoluteFlag: int,
    DisplayColumnTimeDiffFlag: int,
    DisplayColumnTimeDiffMinMaxFlag: int,
    AcceptanceWindows
) -> int:
```

##### Description

Start recording CAN messages to a file with specified display and trigger options.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| FileName | str | Output file name for the recording. |
| TriggerEqu | str | Trigger equation string. |
| DisplayColumnCounterFlag | int | Show counter column flag. |
| DisplayColumnTimeAbsoluteFlag | int | Show absolute time column flag. |
| DisplayColumnTimeDiffFlag | int | Show time difference column flag. |
| DisplayColumnTimeDiffMinMaxFlag | int | Show min/max time diff column flag. |
| AcceptanceWindows | - | Acceptance window configuration. |

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

#### StopCANRecorder

##### Signature

```python
def StopCANRecorder(self) -> int:
```

##### Description

Stop the CAN message recording.

##### Parameter

None

##### Return

| Return Value | Description |
|--------------|-------------|
| 0 | on success |
| -1 | on failure |

---

### A2lLink

---

#### GetA2lLinkNo

##### Signature

```python
def GetA2lLinkNo(self, Process: str) -> int:
```

##### Description

Get the A2L link number for an external process.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| Process | str | Name of the external process. |

##### Return

| Return Value | Description |
|--------------|-------------|
| int | Link number |
| -1 | if not found |

---

#### FetchA2lData

##### Signature

```python
def FetchA2lData(self, LinkNo: int, Label: str, Flags: int, TypeMask: int = 0xFFFF):
```

##### Description

Fetch A2L data for a given link and label.

##### Parameter

| Parameter | Type | Description |
|-----------|------|-------------|
| LinkNo | int | Link number. |
| Label | str | Name of the label to fetch. |
| Flags | int | Flags for data retrieval. |
| TypeMask | int, optional | Type mask for filtering. Defaults to 0xFFFF. |

##### Return

| Return Value | Description |
|--------------|-------------|
| A2LData | The fetched A2LData object |
| None | if not found or error |