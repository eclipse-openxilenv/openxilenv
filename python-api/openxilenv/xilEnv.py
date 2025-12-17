

# Copyright (c) 2024 Contributors to the Eclipse Foundation
#
# This program and the accompanying materials are made available under the
# terms of the Apache License, Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0.
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0

"""
This module provides the XilEnv class, a Python wrapper for the openXilEnv library bindings.
It enables function calls to a running openXilEnv instance from Python code.
"""

import ctypes as ct
import platform
import subprocess
import sys
import time
from pathlib import Path

from openxilenv._can import CAN_ACCEPTANCE_WINDOWS, CAN_FIFO_ELEM, CAN_FD_FIFO_ELEM
from openxilenv._a2l import A2LData


class BB_VARI(ct.Union):
	_fields_ = [('b', ct.c_int8), ('ub', ct.c_uint8), ('w', ct.c_int16), ('uw', ct.c_uint16), ('dw', ct.c_int32), ('udw', ct.c_uint32), ('qw', ct.c_int64), ('uqw', ct.c_uint64), ('f', ct.c_float), ('d', ct.c_double)]


class XilEnv:
	"""
	Python wrapper for the openXilEnv library bindings.

	Provides a minimal interface to control and interact with a running openXilEnv instance from Python.
	"""

	def __init__(self, openXilEnv_path: Path) -> None:
		"""
		Initialize a new XilEnv instance.

		Parameters
		----------
		openXilEnv_path : Path
			Path to the openXilEnv installation directory.
		"""

		# initilaize xilEnv properties
		self.__lib: ct.CDLL|ct.WinDLL|None          = None  # ct.CDLL for Linux, ct.WinDLL for Windows
		self.__SuccessfulConnected: int             = 0     # 0 = not connected, 1 = connected
		self.__ApiVersion: int                      = 0     # API version
		self.__system: str|None                     = None  # OS system
		self.__xilEnvExePath: Path|None             = None  # Path to XilEnv executable
		self.__xilEnvGuiExePath: Path|None          = None  # Path to XilEnvGui executable
		self.__xilEnvProcess: subprocess.Popen|None = None  # XilEnv process handle
		self.__attachedVar: dict[str, int]          = {}    # attached blackboard variables
		self.__defaultXilEnvVariables: list[str]    = [     # default blackboard variables to attach
            "XilEnv.CycleCounter",
            "XilEnv.EquationCalculator",
            "XilEnv.exit",
            "XilEnv.ExitCode",
            "XilEnv.Generator",
            "XilEnv.Realtime",
            "XilEnv.RealtimeFactor",
            "XilEnv.SampleFrequency",
            "XilEnv.SampleTime",
            "XilEnv.Script",
            "XilEnv.StimulusPlayer",
            "XilEnv.StimulusPlayer.suspend",
            "XilEnv.TraceRecorder",
            "XilEnv.Version",
            "XilEnv.Version.Patch"
        ]

		# get lib path
		self.__system = platform.system()
		if self.__system == "Windows":
			lib_name = "XilEnvRpc.dll"
		elif self.__system == "Linux":
			lib_name = "libXilEnvRpc.so"
		else:
			raise OSError(f"Unsupported OS: {self.__system}")
		lib_path = openXilEnv_path.joinpath(lib_name)

		# validate lib path
		if not lib_path.is_file():
			raise FileNotFoundError(f"{lib_name} not found at path: {lib_path}")

		# load lib
		if self.__system == "Windows":
			self.__lib = ct.windll.LoadLibrary(lib_path)
		elif self.__system == "Linux":
			self.__lib = ct.cdll.LoadLibrary(lib_path)

		# path to application
		if self.__system == "Windows":
			self.__xilEnvExePath = openXilEnv_path.joinpath("XilEnv.exe")
			self.__xilEnvGuiExePath = openXilEnv_path.joinpath("XilEnvGui.exe")
		elif self.__system == "Linux":
			self.__xilEnvExePath = openXilEnv_path.joinpath("XilEnv")
			self.__xilEnvGuiExePath = openXilEnv_path.joinpath("XilEnvGui")

		if not self.__xilEnvExePath.is_file():
			raise FileNotFoundError(f"XilEnv application not found at path: {self.__xilEnvExePath}")
		if not self.__xilEnvGuiExePath.is_file():
			raise FileNotFoundError(f"XilEnvGui application not found at path: {self.__xilEnvGuiExePath}")


		# define API prototypes
		
		# Connect
		self.__lib.XilEnv_GetAPIVersion.restype = ct.c_int
		self.__lib.XilEnv_GetAPIVersion.argtypes = None
		self.__ApiVersion = self.__lib.XilEnv_GetAPIVersion()

		self.__lib.XilEnv_GetAPIModulePath.restype = ct.c_char_p
		self.__lib.XilEnv_GetAPIModulePath.argtypes = None

		self.__lib.XilEnv_ConnectTo.restype = ct.c_int
		self.__lib.XilEnv_ConnectTo.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_ConnectToInstance.restype = ct.c_int
		self.__lib.XilEnv_ConnectToInstance.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_IsConnectedTo.restype = ct.c_int
		self.__lib.XilEnv_IsConnectedTo.argtypes = None

		self.__lib.XilEnv_DisconnectFrom.restype = ct.c_int
		self.__lib.XilEnv_DisconnectFrom.argtypes = None

		self.__lib.XilEnv_DisconnectAndClose.restype = ct.c_int
		self.__lib.XilEnv_DisconnectAndClose.argtypes = [ct.c_int, ct.c_int]

		self.__lib.XilEnv_GetVersion.restype = ct.c_int
		self.__lib.XilEnv_GetVersion.argtypes = None
# Other
		self.__lib.XilEnv_CreateFileWithContent.restype = ct.c_int
		self.__lib.XilEnv_CreateFileWithContent.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_GetEnvironVar.restype = ct.c_char_p
		self.__lib.XilEnv_GetEnvironVar.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_SetEnvironVar.restype = ct.c_int
		self.__lib.XilEnv_SetEnvironVar.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_ChangeSettings.restype = ct.c_int
		self.__lib.XilEnv_ChangeSettings.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_TextOut.restype = ct.c_int
		self.__lib.XilEnv_TextOut.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_ErrorTextOut.restype = ct.c_int
		self.__lib.XilEnv_ErrorTextOut.argtypes = [ct.c_int, ct.c_char_p]

# Scheduler
		self.__lib.XilEnv_StopScheduler.restype = None
		self.__lib.XilEnv_StopScheduler.argtypes = None

		self.__lib.XilEnv_ContinueScheduler.restype = None 
		self.__lib.XilEnv_ContinueScheduler.argtypes = None

		self.__lib.XilEnv_IsSchedulerRunning.restype = ct.c_int
		self.__lib.XilEnv_IsSchedulerRunning.argtypes = None

		self.__lib.XilEnv_StartProcess.restype = ct.c_int
		self.__lib.XilEnv_StartProcess.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_StartProcessAndLoadSvl.restype = ct.c_int
		self.__lib.XilEnv_StartProcessAndLoadSvl.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_StartProcessEx.restype = ct.c_int
		self.__lib.XilEnv_StartProcessEx.argtypes = [ct.c_char_p, ct.c_int,  ct.c_int, ct.c_short, ct.c_int, ct.c_char_p, ct.c_char_p, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_int, ct.c_char_p, ct.c_int, ct.c_int]

		self.__lib.XilEnv_StartProcessEx2.restype = ct.c_int
		self.__lib.XilEnv_StartProcessEx2.argtypes = [ct.c_char_p, ct.c_int,  ct.c_int, ct.c_short, ct.c_int, ct.c_char_p, ct.c_char_p, ct.c_uint, ct.c_char_p, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_int, ct.c_char_p, ct.c_int, ct.c_int]

		self.__lib.XilEnv_StopProcess.restype = ct.c_int
		self.__lib.XilEnv_StopProcess.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_GetNextProcess.restype = ct.c_char_p
		self.__lib.XilEnv_GetNextProcess.argtypes = [ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_GetProcessState.restype = ct.c_int
		self.__lib.XilEnv_GetProcessState.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_DoNextCycles.restype = None
		self.__lib.XilEnv_DoNextCycles.argtypes = [ct.c_int]

		self.__lib.XilEnv_DoNextCyclesAndWait.restype = None
		self.__lib.XilEnv_DoNextCyclesAndWait.argtypes = [ct.c_int]

		self.__lib.XilEnv_AddBeforeProcessEquationFromFile.restype = ct.c_int
		self.__lib.XilEnv_AddBeforeProcessEquationFromFile.argtypes = [ct.c_int, ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_AddBehindProcessEquationFromFile.restype = ct.c_int
		self.__lib.XilEnv_AddBehindProcessEquationFromFile.argtypes = [ct.c_int, ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_DelBeforeProcessEquations.restype = None
		self.__lib.XilEnv_DelBeforeProcessEquations.argtypes = [ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_DelBehindProcessEquations.restype = None
		self.__lib.XilEnv_DelBehindProcessEquations.argtypes = [ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_WaitUntil.restype = ct.c_int
		self.__lib.XilEnv_WaitUntil.argtypes = [ct.c_char_p, ct.c_int]

# internal processes
		self.__lib.XilEnv_StartScript.restype = ct.c_int
		self.__lib.XilEnv_StartScript.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_StopScript.restype = ct.c_int
		self.__lib.XilEnv_StopScript.argtypes = None

		self.__lib.XilEnv_StartRecorder.restype = ct.c_int
		self.__lib.XilEnv_StartRecorder.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_RecorderAddComment.restype = ct.c_int
		self.__lib.XilEnv_RecorderAddComment.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_StopRecorder.restype = ct.c_int
		self.__lib.XilEnv_StopRecorder.argtypes = None

		self.__lib.XilEnv_StartPlayer.restype = ct.c_int
		self.__lib.XilEnv_StartPlayer.argtypes = [ct.c_char_p]
		
		self.__lib.XilEnv_StopPlayer.restype = ct.c_int
		self.__lib.XilEnv_StopPlayer.argtypes = None

		self.__lib.XilEnv_StartEquations.restype = ct.c_int
		self.__lib.XilEnv_StartEquations.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_StopEquations.restype = ct.c_int
		self.__lib.XilEnv_StopEquations.argtypes = None

		self.__lib.XilEnv_StartGenerator.restype = ct.c_int
		self.__lib.XilEnv_StartGenerator.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_StopGenerator.restype = ct.c_int
		self.__lib.XilEnv_StopGenerator.argtypes = None

# GUI
		self.__lib.XilEnv_LoadDesktop.restype = ct.c_int
		self.__lib.XilEnv_LoadDesktop.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_SaveDesktop.restype = ct.c_int
		self.__lib.XilEnv_SaveDesktop.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_ClearDesktop.restype = ct.c_int
		self.__lib.XilEnv_ClearDesktop.argtypes = None

		self.__lib.XilEnv_CreateDialog.restype = ct.c_int
		self.__lib.XilEnv_CreateDialog.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_AddDialogItem.restype = ct.c_int
		self.__lib.XilEnv_AddDialogItem.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_ShowDialog.restype = ct.c_int
		self.__lib.XilEnv_ShowDialog.argtypes = None

		self.__lib.XilEnv_IsDialogClosed.restype = ct.c_int
		self.__lib.XilEnv_IsDialogClosed.argtypes = None

		self.__lib.XilEnv_SelectSheet.restype = ct.c_int
		self.__lib.XilEnv_SelectSheet.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_AddSheet.restype = ct.c_int
		self.__lib.XilEnv_AddSheet.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_DeleteSheet.restype = ct.c_int
		self.__lib.XilEnv_DeleteSheet.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_RenameSheet.restype = ct.c_int
		self.__lib.XilEnv_RenameSheet.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_OpenWindow.restype = ct.c_int
		self.__lib.XilEnv_OpenWindow.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_CloseWindow.restype = ct.c_int
		self.__lib.XilEnv_CloseWindow.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_DeleteWindow.restype = ct.c_int
		self.__lib.XilEnv_DeleteWindow.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_ImportWindow.restype = ct.c_int
		self.__lib.XilEnv_ImportWindow.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_ExportWindow.restype = ct.c_int
		self.__lib.XilEnv_ExportWindow.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_char_p]

# Blackboard
		self.__lib.XilEnv_AddVari.restype = ct.c_int
		self.__lib.XilEnv_AddVari.argtypes = [ct.c_char_p, ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_RemoveVari.restype = ct.c_int
		self.__lib.XilEnv_RemoveVari.argtypes = [ct.c_int]

		self.__lib.XilEnv_AttachVari.restype = ct.c_int
		self.__lib.XilEnv_AttachVari.argtypes = [ct.c_char_p]
		
		self.__lib.XilEnv_Get.restype = ct.c_double
		self.__lib.XilEnv_Get.argtypes = [ct.c_int]

		self.__lib.XilEnv_GetPhys.restype = ct.c_double
		self.__lib.XilEnv_GetPhys.argtypes = [ct.c_int]

		self.__lib.XilEnv_Set.restype = None
		self.__lib.XilEnv_Set.argtypes = [ct.c_int, ct.c_double]

		self.__lib.XilEnv_SetPhys.restype = ct.c_int
		self.__lib.XilEnv_SetPhys.argtypes = [ct.c_int, ct.c_double]

		self.__lib.XilEnv_Equ.restype = ct.c_double
		self.__lib.XilEnv_Equ.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_WrVariEnable.restype = ct.c_int
		self.__lib.XilEnv_WrVariEnable.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_WrVariDisable.restype = ct.c_int
		self.__lib.XilEnv_WrVariDisable.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_IsWrVariEnabled.restype = ct.c_int
		self.__lib.XilEnv_IsWrVariEnabled.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_LoadRefList.restype = ct.c_int
		self.__lib.XilEnv_LoadRefList.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_AddRefList.restype = ct.c_int
		self.__lib.XilEnv_AddRefList.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_SaveRefList.restype = ct.c_int
		self.__lib.XilEnv_SaveRefList.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_GetVariConversionType.restype = ct.c_int
		self.__lib.XilEnv_GetVariConversionType.argtypes = [ct.c_int]

		self.__lib.XilEnv_GetVariConversionString.restype = ct.c_char_p
		self.__lib.XilEnv_GetVariConversionString.argtypes = [ct.c_int]

		self.__lib.XilEnv_SetVariConversion.restype = ct.c_int
		self.__lib.XilEnv_SetVariConversion.argtypes = [ct.c_int, ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_GetVariType.restype = ct.c_int
		self.__lib.XilEnv_GetVariType.argtypes = [ct.c_int]

		self.__lib.XilEnv_GetVariUnit.restype = ct.c_char_p
		self.__lib.XilEnv_GetVariUnit.argtypes = [ct.c_int]

		self.__lib.XilEnv_SetVariUnit.restype = ct.c_int
		self.__lib.XilEnv_SetVariUnit.argtypes = [ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_GetVariMin.restype = ct.c_double 
		self.__lib.XilEnv_GetVariMin.argtypes = [ct.c_int]

		self.__lib.XilEnv_GetVariMax.restype = ct.c_double 
		self.__lib.XilEnv_GetVariMax.argtypes = [ct.c_int]

		self.__lib.XilEnv_SetVariMin.restype = ct.c_int
		self.__lib.XilEnv_SetVariMin.argtypes = [ct.c_int, ct.c_double]

		self.__lib.XilEnv_SetVariMax.restype = ct.c_int
		self.__lib.XilEnv_SetVariMax.argtypes = [ct.c_int, ct.c_double]

		self.__lib.XilEnv_GetNextVari.restype = ct.c_char_p
		self.__lib.XilEnv_GetNextVari.argtypes = [ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_GetNextVariEx.restype = ct.c_char_p
		self.__lib.XilEnv_GetNextVariEx.argtypes = [ct.c_int, ct.c_char_p, ct.c_char_p, ct.c_int]

		self.__lib.XilEnv_GetVariEnum.restype = ct.c_char_p
		self.__lib.XilEnv_GetVariEnum.argtypes = [ct.c_int, ct.c_char_p, ct.c_double]

		self.__lib.XilEnv_GetVariDisplayFormatWidth.restype = ct.c_int
		self.__lib.XilEnv_GetVariDisplayFormatWidth.argtypes = [ct.c_int]

		self.__lib.XilEnv_GetVariDisplayFormatPrec.restype = ct.c_int
		self.__lib.XilEnv_GetVariDisplayFormatPrec.argtypes = [ct.c_int]

		self.__lib.XilEnv_SetVariDisplayFormat.restype = ct.c_int
		self.__lib.XilEnv_SetVariDisplayFormat.argtypes = [ct.c_int, ct.c_int, ct.c_int]

		self.__lib.XilEnv_ImportVariProperties.restype = ct.c_int
		self.__lib.XilEnv_ImportVariProperties.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_EnableRangeControl.restype = ct.c_int
		self.__lib.XilEnv_EnableRangeControl.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_DisableRangeControl.restype = ct.c_int
		self.__lib.XilEnv_DisableRangeControl.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_WriteFrame.restype = ct.c_int
		self.__lib.XilEnv_WriteFrame.argtypes = [ct.POINTER(ct.c_int),ct.POINTER(ct.c_double),ct.c_int]

		self.__lib.XilEnv_GetFrame.restype = ct.c_int
		self.__lib.XilEnv_GetFrame.argtypes = [ct.POINTER(ct.c_int),ct.POINTER(ct.c_double),ct.c_int]

		self.__lib.XilEnv_WriteFrameWaitReadFrame.restype = ct.c_int
		self.__lib.XilEnv_WriteFrameWaitReadFrame.argtypes = [ct.POINTER(ct.c_int),ct.POINTER(ct.c_double),ct.c_int,ct.POINTER(ct.c_int),ct.POINTER(ct.c_double),ct.c_int]

		self.__lib.XilEnv_ReferenceSymbol.restype = ct.c_int
		self.__lib.XilEnv_ReferenceSymbol.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_char_p, ct.c_char_p, ct.c_int, ct.c_char_p, ct.c_double, ct.c_double, ct.c_int, ct.c_int, ct.c_int, ct.c_int]

		self.__lib.XilEnv_DereferenceSymbol.restype = ct.c_int
		self.__lib.XilEnv_DereferenceSymbol.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_char_p, ct.c_int]

		self.__lib.XilEnv_GetRaw.restype = ct.c_int
		self.__lib.XilEnv_GetRaw.argtypes = [ct.c_int, ct.POINTER(BB_VARI)]

		self.__lib.XilEnv_SetRaw.restype = ct.c_int
		self.__lib.XilEnv_SetRaw.argtypes = [ct.c_int, ct.c_int, BB_VARI, ct.c_int]

# Calibration
		self.__lib.XilEnv_LoadSvl.restype = ct.c_int
		self.__lib.XilEnv_LoadSvl.argtypes = [ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_SaveSvl.restype = ct.c_int
		self.__lib.XilEnv_SaveSvl.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_SaveSal.restype = ct.c_int
		self.__lib.XilEnv_SaveSal.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_GetSymbolRaw.restype = ct.c_int
		self.__lib.XilEnv_GetSymbolRaw.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_int, ct.POINTER(BB_VARI)]

		self.__lib.XilEnv_SetSymbolRaw.restype = ct.c_int
		self.__lib.XilEnv_SetSymbolRaw.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_int, ct.c_int, BB_VARI]

# CAN
		self.__lib.XilEnv_LoadCanVariante.restype = ct.c_int
		self.__lib.XilEnv_LoadCanVariante.argtypes = [ct.c_char_p, ct.c_int]

		self.__lib.XilEnv_LoadAndSelCanVariante.restype = ct.c_int
		self.__lib.XilEnv_LoadAndSelCanVariante.argtypes = [ct.c_char_p, ct.c_int]

		self.__lib.XilEnv_AppendCanVariante.restype = ct.c_int
		self.__lib.XilEnv_AppendCanVariante.argtypes = [ct.c_char_p, ct.c_int]

		self.__lib.XilEnv_DelAllCanVariants.restype = None
		self.__lib.XilEnv_DelAllCanVariants.argtypes = None

# CCP
		self.__lib.XilEnv_LoadCCPConfig.restype = ct.c_int
		self.__lib.XilEnv_LoadCCPConfig.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_StartCCPBegin.restype = ct.c_int
		self.__lib.XilEnv_StartCCPBegin.argtypes = [ct.c_int]

		self.__lib.XilEnv_StartCCPAddVar.restype = ct.c_int
		self.__lib.XilEnv_StartCCPAddVar.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_StartCCPEnd.restype = ct.c_int
		self.__lib.XilEnv_StartCCPEnd.argtypes = [ct.c_int]

		self.__lib.XilEnv_StopCCP.restype = ct.c_int
		self.__lib.XilEnv_StopCCP.argtypes = [ct.c_int]

		self.__lib.XilEnv_StartCCPCalBegin.restype = ct.c_int
		self.__lib.XilEnv_StartCCPCalBegin.argtypes = [ct.c_int]

		self.__lib.XilEnv_StartCCPCalAddVar.restype = ct.c_int
		self.__lib.XilEnv_StartCCPCalAddVar.argtypes = [ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_StartCCPCalEnd.restype = ct.c_int
		self.__lib.XilEnv_StartCCPCalEnd.argtypes = [ct.c_int]

		self.__lib.XilEnv_StopCCPCal.restype = ct.c_int
		self.__lib.XilEnv_StopCCPCal.argtypes = [ct.c_int]

# XCP
		self.__lib.XilEnv_LoadXCPConfig.restype = ct.c_int
		self.__lib.XilEnv_LoadXCPConfig.argtypes = [ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_StartXCPBegin.restype = ct.c_int
		self.__lib.XilEnv_StartXCPBegin.argtypes = [ct.c_int]

		self.__lib.XilEnv_StartXCPAddVar.restype = ct.c_int
		self.__lib.XilEnv_StartXCPAddVar.argtypes = [ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_StartXCPEnd.restype = ct.c_int
		self.__lib.XilEnv_StartXCPEnd.argtypes = [ct.c_int]

		self.__lib.XilEnv_StopXCP.restype = ct.c_int
		self.__lib.XilEnv_StopXCP.argtypes = [ct.c_int]

		self.__lib.XilEnv_StartXCPCalBegin.restype = ct.c_int
		self.__lib.XilEnv_StartXCPCalBegin.argtypes = [ct.c_int]

		self.__lib.XilEnv_StartXCPCalAddVar.restype = ct.c_int
		self.__lib.XilEnv_StartXCPCalAddVar.argtypes = [ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_StartXCPCalEnd.restype = ct.c_int
		self.__lib.XilEnv_StartXCPCalEnd.argtypes = [ct.c_int]

		self.__lib.XilEnv_StopXCPCal.restype = ct.c_int
		self.__lib.XilEnv_StopXCPCal.argtypes = [ct.c_int]

		self.__lib.XilEnv_TransmitCAN.restype = ct.c_int
		self.__lib.XilEnv_TransmitCAN.argtypes = [ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_ubyte, ct.c_ubyte, ct.c_ubyte, ct.c_ubyte, ct.c_ubyte, ct.c_ubyte, ct.c_ubyte, ct.c_ubyte]

		self.__lib.XilEnv_TransmitCANFd.restype = ct.c_int
		self.__lib.XilEnv_TransmitCANFd.argtypes = [ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.POINTER(ct.c_ubyte)]

# CAN-Message-Queues (FIFO's)
		
		self.__lib.XilEnv_OpenCANQueue.restype = ct.c_int
		self.__lib.XilEnv_OpenCANQueue.argtypes = [ct.c_int]

		self.__lib.XilEnv_OpenCANFdQueue.restype = ct.c_int
		self.__lib.XilEnv_OpenCANFdQueue.argtypes = [ct.c_int, ct.c_int]

		self.__lib.XilEnv_SetCANAcceptanceWindows.restype = ct.c_int
		self.__lib.XilEnv_SetCANAcceptanceWindows.argtypes = [ct.c_int, ct.POINTER(CAN_ACCEPTANCE_WINDOWS)]

		self.__lib.XilEnv_FlushCANQueue.restype = ct.c_int
		self.__lib.XilEnv_FlushCANQueue.argtypes = [ct.c_int]

		self.__lib.XilEnv_ReadCANQueue.restype = ct.c_int
		self.__lib.XilEnv_ReadCANQueue.argtypes = [ct.c_int, ct.POINTER(CAN_FIFO_ELEM)]

		self.__lib.XilEnv_ReadCANFdQueue.restype = ct.c_int
		self.__lib.XilEnv_ReadCANFdQueue.argtypes = [ct.c_int, ct.POINTER(CAN_FD_FIFO_ELEM)]

		self.__lib.XilEnv_TransmitCANQueue.restype = ct.c_int
		self.__lib.XilEnv_TransmitCANQueue.argtypes = [ct.c_int, ct.POINTER(CAN_FIFO_ELEM)]

		self.__lib.XilEnv_TransmitCANFdQueue.restype = ct.c_int
		self.__lib.XilEnv_TransmitCANFdQueue.argtypes = [ct.c_int, ct.POINTER(CAN_FD_FIFO_ELEM)]

		self.__lib.XilEnv_CloseCANQueue.restype = ct.c_int
		self.__lib.XilEnv_CloseCANQueue.argtypes = None

# Other part 2
		self.__lib.XilEnv_SetCanChannelCount.restype = ct.c_int
		self.__lib.XilEnv_SetCanChannelCount.argtypes = [ct.c_int]

		self.__lib.XilEnv_SetCanChannelStartupState.restype = ct.c_int
		self.__lib.XilEnv_SetCanChannelStartupState.argtypes = [ct.c_int, ct.c_int]

# CAN bit error
		self.__lib.XilEnv_SetCanErr.restype = ct.c_int
		self.__lib.XilEnv_SetCanErr.argtypes = [ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_int, ct.c_int64]

		self.__lib.XilEnv_SetCanErrSignalName.restype = ct.c_int
		self.__lib.XilEnv_SetCanErrSignalName.argtypes = [ct.c_int, ct.c_int, ct.c_char_p, ct.c_int, ct.c_int64]

		self.__lib.XilEnv_ClearCanErr.restype = ct.c_int
		self.__lib.XilEnv_ClearCanErr.argtypes = None

		self.__lib.XilEnv_SetCanSignalConversion.restype = ct.c_int
		self.__lib.XilEnv_SetCanSignalConversion.argtypes = [ct.c_int, ct.c_int, ct.c_char_p, ct.c_char_p]

		self.__lib.XilEnv_ResetCanSignalConversion.restype = ct.c_int
		self.__lib.XilEnv_ResetCanSignalConversion.argtypes = [ct.c_int, ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_ResetAllCanSignalConversion.restype = ct.c_int
		self.__lib.XilEnv_ResetAllCanSignalConversion.argtypes = [ct.c_int, ct.c_int]

# CAN Recorder
		self.__lib.XilEnv_StartCANRecorder.restype = ct.c_int
		self.__lib.XilEnv_StartCANRecorder.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.POINTER(CAN_ACCEPTANCE_WINDOWS)]

		self.__lib.XilEnv_StopCANRecorder.restype = ct.c_int
		self.__lib.XilEnv_StopCANRecorder.argtypes = None

# A2LLink
		self.__lib.XilEnv_SetupLinkToExternProcess.restype = ct.c_int
		self.__lib.XilEnv_SetupLinkToExternProcess.argtypes = [ct.c_char_p, ct.c_char_p, ct.c_int]

		self.__lib.XilEnv_GetLinkToExternProcess.restype = ct.c_int
		self.__lib.XilEnv_GetLinkToExternProcess.argtypes = [ct.c_char_p]

		self.__lib.XilEnv_GetIndexFromLink.restype = ct.c_int
		self.__lib.XilEnv_GetIndexFromLink.argtypes = [ct.c_int, ct.c_char_p, ct.c_int]
	
		self.__lib.XilEnv_GetNextSymbolFromLink.restype = ct.c_int
		self.__lib.XilEnv_GetNextSymbolFromLink.argtypes = [ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.POINTER(ct.c_ubyte), ct.c_int]

		self.__lib.XilEnv_GetDataFromLink.restype = ct.c_void_p
		self.__lib.XilEnv_GetDataFromLink.argtypes = [ct.c_int, ct.c_int, ct.c_void_p, ct.c_int, ct.POINTER(ct.c_char_p)]

		self.__lib.XilEnv_SetDataToLink.restype = ct.c_int
		self.__lib.XilEnv_SetDataToLink.argtypes = [ct.c_int, ct.c_int, ct.c_void_p, ct.POINTER(ct.c_char_p)]

		self.__lib.XilEnv_ReferenceMeasurementToBlackboard.restype = ct.c_int
		self.__lib.XilEnv_ReferenceMeasurementToBlackboard = [ct.c_int, ct.c_int, ct.c_int]

# A2LLinks helper
		self.__lib.XilEnv_GetLinkDataType.restype = ct.c_int
		self.__lib.XilEnv_GetLinkDataType.argtypes = [ct.c_void_p]

		self.__lib.XilEnv_GetLinkDataArrayCount.restype = ct.c_int
		self.__lib.XilEnv_GetLinkDataArrayCount.argtypes = [ct.c_void_p]

		self.__lib.XilEnv_GetLinkDataArraySize.restype = ct.c_int
		self.__lib.XilEnv_GetLinkDataArraySize.argtypes = [ct.c_void_p, ct.c_int]

		self.__lib.XilEnv_CopyLinkData.restype = ct.c_void_p
		self.__lib.XilEnv_CopyLinkData.argtypes = [ct.c_void_p]

		self.__lib.XilEnv_FreeLinkData.restype = ct.c_void_p
		self.__lib.XilEnv_FreeLinkData.argtypes = [ct.c_void_p]

# Single values
		self.__lib.XilEnv_GetLinkSingleValueDataType.restype = ct.c_int
		self.__lib.XilEnv_GetLinkSingleValueDataType.argtypes = [ct.c_void_p]

		self.__lib.XilEnv_GetLinkSingleValueTargetDataType.restype = ct.c_int
		self.__lib.XilEnv_GetLinkSingleValueTargetDataType.argtypes = [ct.c_void_p]

		self.__lib.XilEnv_GetLinkSingleValueFlags.restype = ct.c_uint
		self.__lib.XilEnv_GetLinkSingleValueFlags.argtypes = [ct.c_void_p]

		self.__lib.XilEnv_GetLinkSingleValueAddress.restype = ct.c_uint64
		self.__lib.XilEnv_GetLinkSingleValueAddress.argtypes = [ct.c_void_p]

		self.__lib.XilEnv_GetLinkSingleValueDimensionCount.restype = ct.c_int
		self.__lib.XilEnv_GetLinkSingleValueDimensionCount.argtypes = [ct.c_void_p]

		self.__lib.XilEnv_GetLinkSingleValueDimension.restype = ct.c_int
		self.__lib.XilEnv_GetLinkSingleValueDimension.argtypes = [ct.c_void_p, ct.c_int]

		self.__lib.XilEnv_GetLinkSingleValueDataDouble.restype = ct.c_double
		self.__lib.XilEnv_GetLinkSingleValueDataDouble.argtypes = [ct.c_void_p]

		self.__lib.XilEnv_SetLinkSingleValueDataDouble.restype = ct.c_int
		self.__lib.XilEnv_SetLinkSingleValueDataDouble.argtypes = [ct.c_void_p, ct.c_double]

		self.__lib.XilEnv_GetLinkSingleValueDataInt.restype = ct.c_int64
		self.__lib.XilEnv_GetLinkSingleValueDataInt.argtypes = [ct.c_void_p]

		self.__lib.XilEnv_SetLinkSingleValueDataInt.restype = ct.c_int
		self.__lib.XilEnv_SetLinkSingleValueDataInt.argtypes = [ct.c_void_p, ct.c_int64]

		self.__lib.XilEnv_GetLinkSingleValueDataUint.restype = ct.c_uint64
		self.__lib.XilEnv_GetLinkSingleValueDataUint.argtypes = [ct.c_void_p]

		self.__lib.XilEnv_SetLinkSingleValueDataUint.restype = ct.c_int
		self.__lib.XilEnv_SetLinkSingleValueDataUint.argtypes = [ct.c_void_p, ct.c_int64]

		self.__lib.XilEnv_GetLinkSingleValueDataString.restype = ct.c_int
		self.__lib.XilEnv_GetLinkSingleValueDataString.argtypes = [ct.c_void_p, ct.POINTER(ct.c_char), ct.c_int]

		self.__lib.XilEnv_GetLinkSingleValueDataStringPtr.restype = ct.c_char_p
		self.__lib.XilEnv_GetLinkSingleValueDataStringPtr.argtypes = [ct.c_void_p]

		self.__lib.XilEnv_SetLinkSingleValueDataString.restype = ct.c_int
		self.__lib.XilEnv_SetLinkSingleValueDataString.argtypes = [ct.c_void_p, ct.c_char_p]

		self.__lib.XilEnv_GetLinkSingleValueUnit.restype = ct.c_int
		self.__lib.XilEnv_GetLinkSingleValueUnit.argtypes = [ct.c_void_p, ct.POINTER(ct.c_char), ct.c_int]

		self.__lib.XilEnv_GetLinkSingleValueUnitPtr.restype = ct.c_char_p
		self.__lib.XilEnv_GetLinkSingleValueUnitPtr.argtypes = [ct.c_void_p]

# Array of values
		self.__lib.XilEnv_GetLinkArrayValueDataType.restype = ct.c_int
		self.__lib.XilEnv_GetLinkArrayValueDataType.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__lib.XilEnv_GetLinkArrayValueTargetDataType.restype = ct.c_int
		self.__lib.XilEnv_GetLinkArrayValueTargetDataType.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__lib.XilEnv_GetLinkArrayValueFlags.restype = ct.c_uint
		self.__lib.XilEnv_GetLinkArrayValueFlags.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__lib.XilEnv_GetLinkArrayValueAddress.restype = ct.c_uint64
		self.__lib.XilEnv_GetLinkArrayValueAddress.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__lib.XilEnv_GetLinkArrayValueDimensionCount.restype = ct.c_int
		self.__lib.XilEnv_GetLinkArrayValueDimensionCount.argtypes = [ct.c_void_p, ct.c_int]

		self.__lib.XilEnv_GetLinkArrayValueDimension.restype = ct.c_int
		self.__lib.XilEnv_GetLinkArrayValueDimension.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__lib.XilEnv_GetLinkArrayValueDataDouble.restype = ct.c_double
		self.__lib.XilEnv_GetLinkArrayValueDataDouble.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__lib.XilEnv_SetLinkArrayValueDataDouble.restype = ct.c_int
		self.__lib.XilEnv_SetLinkArrayValueDataDouble.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, ct.c_double]

		self.__lib.XilEnv_GetLinkArrayValueDataInt.restype = ct.c_int64
		self.__lib.XilEnv_GetLinkArrayValueDataInt.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__lib.XilEnv_SetLinkArrayValueDataInt.restype = ct.c_int
		self.__lib.XilEnv_SetLinkArrayValueDataInt.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, ct.c_int64]

		self.__lib.XilEnv_GetLinkArrayValueDataUint.restype = ct.c_uint64
		self.__lib.XilEnv_GetLinkArrayValueDataUint.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]

		self.__lib.XilEnv_SetLinkArrayValueDataUint.restype = ct.c_int
		self.__lib.XilEnv_SetLinkArrayValueDataUint.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, ct.c_uint64]

		self.__lib.XilEnv_GetLinkArrayValueDataString.restype = ct.c_int
		self.__lib.XilEnv_GetLinkArrayValueDataString.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, ct.POINTER(ct.c_char), ct.c_int]

		self.__lib.XilEnv_GetLinkArrayValueDataStringPtr.restype = ct.c_char_p
		self.__lib.XilEnv_GetLinkArrayValueDataStringPtr.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]
	
		self.__lib.XilEnv_SetLinkArrayValueDataString.restype = ct.c_int
		self.__lib.XilEnv_SetLinkArrayValueDataString.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, ct.c_char_p]

		self.__lib.XilEnv_GetLinkArrayValueUnit.restype = ct.c_int
		self.__lib.XilEnv_GetLinkArrayValueUnit.argtypes = [ct.c_void_p, ct.c_int, ct.c_int, ct.POINTER(ct.c_char), ct.c_int]

		self.__lib.XilEnv_GetLinkArrayValueUnitPtr.restype = ct.c_char_p
		self.__lib.XilEnv_GetLinkArrayValueUnitPtr.argtypes = [ct.c_void_p, ct.c_int, ct.c_int]
	
		self.__lib.XilEnv_PrintLinkData.restype = None
		self.__lib.XilEnv_PrintLinkData.argtypes = [ct.c_void_p]

# All methods definitions starts here:

# Helper methods

	def c_str(self, string: str) -> ct.c_char_p:
		"""
		Convert a Python string to a C-style char pointer (UTF-8 encoded).

		Parameters
		----------
		string : str
			Python string to convert.

		Returns
		-------
		c_char_p
			C-compatible char pointer.
		"""
		return ct.c_char_p(string.encode('utf-8'))


	def str_c(self, c_string: ct.c_char_p) -> str:
		"""
		Convert a C-style char pointer to a Python string (UTF-8 decoded).

		Parameters
		----------
		c_string : c_char_p
			C-compatible char pointer.

		Returns
		-------
		str
			Decoded Python string.
		"""
		if c_string is None:
			return ""
		else:
			return c_string.decode("utf-8")
		

	def waitSeconds(self, waitTimeInSec: float) -> float:
		"""
		Wait for a specified time in seconds by executing the corresponding number of scheduler cycles.

		Parameters
		----------
		waitTimeInSec : float
			Time to wait in seconds.

		Returns
		-------
		float
			The waited time in seconds.
		"""
		cycles = self.__translateSecondsToCycleCounts(waitTimeInSec)

		self.StopScheduler()
		self.DoNextCyclesAndWait(cycles)
		self.ContinueScheduler()

		return waitTimeInSec


	def __translateSecondsToCycleCounts(self, seconds: float) -> int:
		"""
		Convert a time duration in seconds to scheduler cycle counts.

		Parameters
		----------
		seconds : float
			Time duration in seconds.

		Returns
		-------
		int
			Number of cycles corresponding to the given time.
		"""
		# TODO: How to handle residuals
		sampleFrequency = self.readSignal("XilEnv.SampleFrequency")
		return int(seconds * sampleFrequency)


	def waitUntilValueAlmostMatch(
		self,
		signalName: str,
		expectedSignalValue: float,
		waitTimeInSec: float,
		tolerance: float
	) -> int:
		"""
		Wait until a signal value is within a specified tolerance of the expected value.

		Parameters
		----------
		signalName : str
			Name of the signal to monitor.
		expectedSignalValue : float
			Target value to match.
		waitTimeInSec : float
			Maximum time to wait in seconds.
		tolerance : float
			Allowed deviation from the expected value.

		Returns
		-------
		int
			Remaining cycles after the condition is met or timeout.
		"""
		cycleCount = self.__translateSecondsToCycleCounts(waitTimeInSec)
		minValue = expectedSignalValue - tolerance
		maxValue = expectedSignalValue + tolerance
		equation = f"{signalName} >= {minValue} && {signalName} <= {maxValue}"
		remainingCycles = self.WaitUntil(equation, cycleCount)
		return remainingCycles


	def waitUntilValueIsGreater(
		self,
		signalName: str,
		expectedSignalValue: float,
		waitTimeInSec: float
	) -> int:
		"""
		Wait until a signal value is greater than the expected value.

		Parameters
		----------
		signalName : str
			Name of the signal to monitor.
		expectedSignalValue : float
			Value to exceed.
		waitTimeInSec : float
			Maximum time to wait in seconds.

		Returns
		-------
		int
			Remaining cycles after the condition is met or timeout.
		"""
		cycleCount = self.__translateSecondsToCycleCounts(waitTimeInSec)
		equation = f"{signalName} > {expectedSignalValue}"
		remainingCycles = self.WaitUntil(equation, cycleCount)
		return remainingCycles


	def waitUntilValueIsSmaller(
		self,
		signalName: str,
		expectedSignalValue: float,
		waitTimeInSec: float
	) -> int:
		"""
		Wait until a signal value is smaller than the expected value.

		Parameters
		----------
		signalName : str
			Name of the signal to monitor.
		expectedSignalValue : float
			Value to fall below.
		waitTimeInSec : float
			Maximum time to wait in seconds.

		Returns
		-------
		int
			Remaining cycles after the condition is met or timeout.
		"""
		cycleCount = self.__translateSecondsToCycleCounts(waitTimeInSec)
		equation = f"{signalName} < {expectedSignalValue}"
		remainingCycles = self.WaitUntil(equation, cycleCount)
		return remainingCycles


	def waitUntilValueMatch(
		self,
		signalName: str,
		expectedSignalValue: float,
		waitTimeInSec: float
	) -> int:
		"""
		Wait until a signal value matches the expected value exactly.

		Parameters
		----------
		signalName : str
			Name of the signal to monitor.
		expectedSignalValue : float
			Value to match.
		waitTimeInSec : float
			Maximum time to wait in seconds.

		Returns
		-------
		int
			Remaining cycles after the condition is met or timeout.
		"""
		cycleCount = self.__translateSecondsToCycleCounts(waitTimeInSec)
		equation = f"{signalName} == {expectedSignalValue}"
		remainingCycles = self.WaitUntil(equation, cycleCount)
		return remainingCycles

# API Query

	# The following method can be called without a successful connection to softcar

	def GetAPIVersion(self) -> int:
		"""
		Get the API version of the loaded openXilEnv library.

		Returns
		-------
		int
			API version number.
		"""
		return self.__lib.XilEnv_GetAPIVersion()


	def GetAPIAlternativeVersion(self) -> int:
		"""
		Get the alternative API version of the loaded openXilEnv library.

		Returns
		-------
		int
			Alternative API version number.
		"""
		return self.__lib.XilEnv_GetAPIAlternativeVersion()


	def GetAPIModulePath(self) -> bytes:
		"""
		Get the file system path to the loaded openXilEnv API module.

		Returns
		-------
		bytes
			Path to the API module as a bytes object.
		"""
		return self.__lib.XilEnv_GetAPIModulePath()
	
	
# Connect


	def startWithGui(self, iniFilePath: str, timeoutInSec: int = 5) -> None:
		"""
		Start openXilEnv with the GUI using the specified INI file.

		Parameters
		----------
		iniFilePath : str
			Path to the INI configuration file.
		timeoutInSec : int, optional
			Number of seconds to wait for connection (default is 5).

		Returns
		-------
		None
		"""
		self.__start(iniFilePath, True, timeoutInSec)


	def startWithoutGui(self, iniFilePath: str, timeoutInSec: int = 5) -> None:
		"""
		Start openXilEnv without the GUI using the specified INI file.

		Parameters
		----------
		iniFilePath : str
			Path to the INI configuration file.
		timeoutInSec : int, optional
			Number of seconds to wait for connection (default is 5).

		Returns
		-------
		None
		"""
		self.__start(iniFilePath, False, timeoutInSec)

	def disconnectAndCloseXil(self) -> None:
		"""
		Disconnect from openXilEnv and close the process, cleaning up attached variables.

		Returns
		-------
		None
		"""
		if self.__xilEnvProcess:
			self.__removeVariables()
			self.DisconnectAndClose(0, 0)

	def __connectToXilEnv(self, timeoutInSec: int) -> int:
		"""
		Attempt to connect to openXilEnv within a timeout period.

		Parameters
		----------
		timeoutInSec : int
			Number of seconds to wait for connection.

		Returns
		-------
		int
			Connection status (0 if successful, nonzero otherwise).
		"""
		connectionStatus = -1
		while timeoutInSec:
			connectionStatus = self.ConnectTo("")
			if connectionStatus == 0 or timeoutInSec == 0:
				return connectionStatus
			timeoutInSec -= 1
			time.sleep(1)
		return connectionStatus

	def __start(self, iniFilePath: str, startWithGUI: bool, timeoutInSec: int) -> None:
		"""
		Start the openXilEnv process and connect to it.

		Parameters
		----------
		iniFilePath : str
			Path to the INI configuration file.
		startWithGUI : bool
			If True, start with GUI; otherwise, start without GUI.
		timeoutInSec : int
			Number of seconds to wait for connection.

		Returns
		-------
		None

		Raises
		------
		SystemExit
			If connection to openXilEnv fails or an exception occurs.
		"""
		try:
			if startWithGUI:
				self.__xilEnvProcess = subprocess.Popen([self.__xilEnvGuiExePath, "-ini", iniFilePath],
														stdout=subprocess.PIPE, stderr=subprocess.PIPE)
			else:
				self.__xilEnvProcess = subprocess.Popen([self.__xilEnvExePath, "-ini", iniFilePath, "-nogui"],
														stdout=subprocess.PIPE, stderr=subprocess.PIPE)

			connectionStatus = self.__connectToXilEnv(timeoutInSec)

			if connectionStatus == 0:
				print("Python has been successfully connected to XilEnv")
				time.sleep(1)  # Wait a moment to ensure stability
				self.__attachDefaultXilEnvVariables()
				return
			else:
				print("Could not connect to openXilEnv")
				sys.exit(1)
		except Exception as e:
			print(e)
			sys.exit(1)

	def ConnectTo(self, Address: str) -> int:
		"""
		Connect to a remote XilEnv instance by address.

		Parameters
		----------
		Address : str
			Hostname or IP address of the remote XilEnv instance.

		Returns
		-------
		int
			0 if connection is successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 0):
			Ret = self.__lib.XilEnv_ConnectTo(self.c_str(Address))
			if (Ret == 0):
				self.__SuccessfulConnected = 1
		else:
			Ret = -1
		return Ret

	def ConnectToInstance(self, Address: str, Instance: str) -> int:
		"""
		Connect to a named XilEnv instance at a given address.

		Parameters
		----------
		Address : str
			Hostname or IP address of the remote XilEnv instance.
		Instance : str
			Name of the XilEnv instance to connect to.

		Returns
		-------
		int
			0 if connection is successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 0):
			Ret = self.__lib.XilEnv_ConnectToInstance(self.c_str(Address), self.c_str(Instance))
			if (Ret == 0):
				self.__SuccessfulConnected = 1
		else:
			Ret = -1
		return Ret

	def isConnected(self) -> bool:
		"""
		Check if connected to a XilEnv instance.

		Returns
		-------
		bool
			True if connected, False otherwise.
		"""
		return bool(self.__lib.XilEnv_IsConnectedTo())

	def DisconnectFrom(self) -> int:
		"""
		Disconnect from the current XilEnv instance.

		Returns
		-------
		int
			0 if disconnection is successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			Ret = self.__lib.XilEnv_DisconnectFrom()
			if (Ret == 0):
				self.__SuccessfulConnected = 0
		else:
			Ret = -1
		return Ret

	def DisconnectAndClose(self, SetErrorLevelFlag: int, ErrorLevel: int) -> int:
		"""
		Disconnect and close the XilEnv instance.

		Parameters
		----------
		SetErrorLevelFlag : int
			If nonzero, set the error level after disconnecting.
		ErrorLevel : int
			Error level to set if SetErrorLevelFlag is nonzero.

		Returns
		-------
		int
			0 if disconnection is successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			Ret = self.__lib.XilEnv_DisconnectAndClose(SetErrorLevelFlag, ErrorLevel)
			if (Ret == 0):
				self.__SuccessfulConnected = 0
		else:
			Ret = -1
		return Ret


# Other 
	def CreateFileWithContent(self, Filename: str, Content: str) -> int:
		"""
		Create a file with the specified content on the remote XilEnv system.

		Parameters
		----------
		Filename : str
			A Path to the file to create.
		Content : str
			Content to write into the file.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_CreateFileWithContent(self.c_str(Filename), self.c_str(Content))
		else:
			return -1

	def GetEnvironVar(self, EnvironVar: str) -> str:
		"""
		Get the value of an environment variable from the remote XilEnv system.

		Parameters
		----------
		EnvironVar : str
			Name of the environment variable.

		Returns
		-------
		str
			Value of the environment variable, or an empty string if not connected.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.str_c(self.__lib.XilEnv_GetEnvironVar(self.c_str(EnvironVar)))
		else:
			return ""

	def SetEnvironVar(self, EnvironVar: str, EnvironValue: str) -> int:
		"""
		Set the value of an environment variable on the remote XilEnv system.

		Parameters
		----------
		EnvironVar : str
			Name of the environment variable.
		EnvironValue : str
			Value to set for the environment variable.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_SetEnvironVar(self.c_str(EnvironVar), self.c_str(EnvironValue))
		else:
			return -1

	def ChangeSettings(self, SettingName: str, SettingValue: str) -> int:
		"""
		Change a setting on the remote XilEnv system.

		Parameters
		----------
		SettingName : str
			Name of the setting to change.
		SettingValue : str
			Value to set for the setting.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_ChangeSettings(self.c_str(SettingName), self.c_str(SettingValue))
		else:
			return -1

	def TextOut(self, TextOut: str) -> int:
		"""
		Output text to the remote XilEnv system's standard output.

		Parameters
		----------
		TextOut : str
			Text to output.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_TextOut(self.c_str(TextOut))
		else:
			return -1

	def ErrorTextOut(self, ErrLevel: int, TextOut: str) -> int:
		"""
		Output error text to the remote XilEnv system's standard error with a specified error level.

		Parameters
		----------
		ErrLevel : int
			Error level for the output.
		TextOut : str
			Error text to output.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_ErrorTextOut(ErrLevel, self.c_str(TextOut))
		else:
			return -1

# Scheduler

	def disableRealTimeFactorSwitch(self):
		"""
		Switch realtime simulation speed off.
		openXilEnv will simulate as fast as possible.
		
		Returns
		-------
		None
		"""
		self.__switchRealTimeFactor("No")

	def enableRealTimeFactorSwitch(self):
		"""
		Switch realtime simulation speed on.
		openXilEnv tries to match simulation speed to real time.
		
		Returns
		-------
		None
		"""
		self.__switchRealTimeFactor("Yes")

	def StopScheduler(self) -> None:
		"""
		Stop the XilEnv scheduler if connected.
		"""
		if (self.__SuccessfulConnected == 1):
			self.__lib.XilEnv_StopScheduler()

	def ContinueScheduler(self) -> None:
		"""
		Continue the XilEnv scheduler if connected.
		"""
		if (self.__SuccessfulConnected == 1):
			self.__lib.XilEnv_ContinueScheduler()

	def SchedulerRunning(self) -> int:
		"""
		Check if the XilEnv scheduler is running.

		Returns
		-------
		int
			1 if running, 0 if stopped, -1 if not connected.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_IsSchedulerRunning()
		else:
			return -1

	def StartProcess(self, Name: str) -> int:
		"""
		Start a process by name on the remote XilEnv system.

		Parameters
		----------
		Name : str
			Name of the process to start.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_StartProcess(self.c_str(Name))
		else:
			return -1

	def StartProcessAndLoadSvl(self, ProcessName: str, SvlName: str) -> int:
		"""
		Start a process and load an SVL file on the remote XilEnv system.

		Parameters
		----------
		ProcessName : str
			Name of the process to start.
		SvlName : str
			Name of the SVL file to load.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_StartProcessAndLoadSvl(self.c_str(ProcessName), self.c_str(SvlName))
		else:
			return -1

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
		"""
		Start a process with extended options on the remote XilEnv system.

		Parameters
		----------
		ProcessName : str
			Name of the process to start.
		Prio : int
			Priority of the process.
		Cycle : int
			Cycle time.
		Delay : int
			Delay before start.
		Timeout : int
			Timeout for the process.
		SVLFile : str
			SVL file to load.
		BBPrefix : str
			Blackboard prefix.
		UseRangeControl : int
			Use range control flag.
		RangeControlBeforeActiveFlags : int
			Range control before active flags.
		RangeControlBehindActiveFlags : int
			Range control behind active flags.
		RangeControlStopSchedFlag : int
			Range control stop scheduler flag.
		RangeControlOutput : int
			Range control output flag.
		RangeErrorCounterFlag : int
			Range error counter flag.
		RangeErrorCounter : str
			Range error counter variable.
		RangeControlVarFlag : int
			Range control variable flag.
		RangeControl : str
			Range control variable.
		RangeControlPhysFlag : int
			Range control physical flag.
		RangeControlLimitValues : int
			Range control limit values flag.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_StartProcessEx(
				self.c_str(ProcessName), Prio, Cycle, Delay, Timeout, self.c_str(SVLFile), self.c_str(BBPrefix),
				UseRangeControl, RangeControlBeforeActiveFlags, RangeControlBehindActiveFlags, RangeControlStopSchedFlag,
				RangeControlOutput, RangeErrorCounterFlag, self.c_str(RangeErrorCounter), RangeControlVarFlag,
				self.c_str(RangeControl), RangeControlPhysFlag, RangeControlLimitValues
			)
		else:
			return -1

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
		"""
		Start a process with extended options and A2L file on the remote XilEnv system.

		Parameters
		----------
		ProcessName : str
			Name of the process to start.
		Prio : int
			Priority of the process.
		Cycle : int
			Cycle time.
		Delay : int
			Delay before start.
		Timeout : int
			Timeout for the process.
		SVLFile : str
			SVL file to load.
		A2LFile : str
			A2L file to load.
		A2LFlags : int
			A2L flags.
		BBPrefix : str
			Blackboard prefix.
		UseRangeControl : int
			Use range control flag.
		RangeControlBeforeActiveFlags : int
			Range control before active flags.
		RangeControlBehindActiveFlags : int
			Range control behind active flags.
		RangeControlStopSchedFlag : int
			Range control stop scheduler flag.
		RangeControlOutput : int
			Range control output flag.
		RangeErrorCounterFlag : int
			Range error counter flag.
		RangeErrorCounter : str
			Range error counter variable.
		RangeControlVarFlag : int
			Range control variable flag.
		RangeControl : str
			Range control variable.
		RangeControlPhysFlag : int
			Range control physical flag.
		RangeControlLimitValues : int
			Range control limit values flag.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_StartProcessEx(
				self.c_str(ProcessName), Prio, Cycle, Delay, Timeout, self.c_str(SVLFile), self.c_str(A2LFile), A2LFlags,
				self.c_str(BBPrefix), UseRangeControl, RangeControlBeforeActiveFlags, RangeControlBehindActiveFlags,
				RangeControlStopSchedFlag, RangeControlOutput, RangeErrorCounterFlag, self.c_str(RangeErrorCounter),
				RangeControlVarFlag, self.c_str(RangeControl), RangeControlPhysFlag, RangeControlLimitValues
			)
		else:
			return -1

	def StopProcess(self, Name: str) -> int:
		"""
		Stop a process by name on the remote XilEnv system.

		Parameters
		----------
		Name : str
			Name of the process to stop.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_StopProcess(self.c_str(Name))
		else:
			return -1

	def GetNextProcess(self, Flag: int, Filter: str) -> str | int:
		"""
		Get the next process matching the filter and flag.

		Parameters
		----------
		Flag : int
			Filter flag for process selection.
		Filter : str
			Filter string for process selection.

		Returns
		-------
		str | int
			Name of the next process as a string, or -1 if not connected.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.str_c(self.__lib.XilEnv_GetNextProcess(Flag, self.c_str(Filter)))
		else:
			return -1

	def GetProcessState(self, Name: str) -> int:
		"""
		Get the state of a process by name.

		Parameters
		----------
		Name : str
			Name of the process.

		Returns
		-------
		int
			State of the process, or -1 if not connected.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_GetProcessState(self.c_str(Name))
		else:
			return -1

	def DoNextCycles(self, Cycles: int) -> None:
		"""
		Execute the next N scheduler cycles.

		Parameters
		----------
		Cycles : int
			Number of cycles to execute.
		"""
		if (self.__SuccessfulConnected == 1):
			self.__lib.XilEnv_DoNextCycles(Cycles)

	def DoNextCyclesAndWait(self, Cycles: int) -> None:
		"""
		Execute the next N scheduler cycles and wait for completion.

		Parameters
		----------
		Cycles : int
			Number of cycles to execute.
		"""
		if (self.__SuccessfulConnected == 1):
			self.__lib.XilEnv_DoNextCyclesAndWait(Cycles)


	def AddBeforeProcessEquationFromFile(self, Nr: int, ProcName: str, EquFile: str) -> int:
		"""
		Add an equation from file to be executed before a process.

		Parameters
		----------
		Nr : int
			Equation number or identifier.
		ProcName : str
			Name of the process.
		EquFile : str
			Path to the equation file.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_AddBeforeProcessEquationFromFile(Nr, self.c_str(ProcName), self.c_str(EquFile))
		else:
			return -1

	def AddBehindProcessEquationFromFile(self, Nr: int, ProcName: str, EquFile: str) -> int:
		"""
		Add an equation from file to be executed after a process.

		Parameters
		----------
		Nr : int
			Equation number or identifier.
		ProcName : str
			Name of the process.
		EquFile : str
			Path to the equation file.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_AddBehindProcessEquationFromFile(Nr, self.c_str(ProcName), self.c_str(EquFile))
		else:
			return -1

	def DelBeforeProcessEquations(self, Nr: int, ProcName: str) -> None:
		"""
		Delete all equations to be executed before a process.

		Parameters
		----------
		Nr : int
			Equation number or identifier.
		ProcName : str
			Name of the process.
		"""
		if (self.__SuccessfulConnected == 1):
			self.__lib.XilEnv_DelBeforeProcessEquations(Nr, self.c_str(ProcName))

	def DelBehindProcessEquations(self, Nr: int, ProcName: str) -> None:
		"""
		Delete all equations to be executed after a process.

		Parameters
		----------
		Nr : int
			Equation number or identifier.
		ProcName : str
			Name of the process.
		"""
		if (self.__SuccessfulConnected == 1):
			self.__lib.XilEnv_DelBehindProcessEquations(Nr, self.c_str(ProcName))

	def WaitUntil(self, Equation: str, Cycles: int) -> int:
		"""
		Wait until the given equation is true or a number of cycles have passed.

		Parameters
		----------
		Equation : str
			Equation to evaluate.
		Cycles : int
			Maximum number of cycles to wait.

		Returns
		-------
		int
			0 if the condition was met, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_WaitUntil(self.c_str(Equation), Cycles)
		else:
			return -1

	def __switchRealTimeFactor(self, setting: str) -> None:
		"""
		Switch the real-time factor setting for the scheduler.

		Parameters
		----------
		setting : str
			The value to set for NOT_FASTER_THAN_REALTIME (e.g., "Yes" or "No").

		Returns
		-------
		None
		"""
		self.StopScheduler()
		self.ChangeSettings("NOT_FASTER_THAN_REALTIME", setting)
		self.ContinueScheduler()

# internal processes
	def StartScript(self, ScriptFile: str) -> int:
		"""
		Start a script on the remote XilEnv system.

		Parameters
		----------
		ScriptFile : str
			Path to the script file to start.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_StartScript(self.c_str(ScriptFile))
		else:
			return -1

	def StopScript(self) -> int:
		"""
		Stop the currently running script on the remote XilEnv system.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_StopScript()
		else:
			return -1

	def StartRecorder(self, CfgFile: str) -> int:
		"""
		Start the recorder with the specified configuration file.

		Parameters
		----------
		CfgFile : str
			Path to the recorder configuration file.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_StartRecorder(self.c_str(CfgFile))
		else:
			return -1

	def RecorderAddComment(self, Comment: str) -> int:
		"""
		Add a comment to the recorder output.

		Parameters
		----------
		Comment : str
			Comment text to add.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_RecorderAddComment(self.c_str(Comment))
		else:
			return -1

	def StopRecorder(self) -> int:
		"""
		Stop the recorder on the remote XilEnv system.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_StopRecorder()
		else:
			return -1

	def StartPlayer(self, CfgFile: str) -> int:
		"""
		Start the player with the specified configuration file.

		Parameters
		----------
		CfgFile : str
			Path to the player configuration file.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_StartPlayer(self.c_str(CfgFile))
		else:
			return -1

	def StopPlayer(self) -> int:
		"""
		Stop the player on the remote XilEnv system.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_StopPlayer()
		else:
			return -1

	def StartEquations(self, EquFile: str) -> int:
		"""
		Start equations from a specified file on the remote XilEnv system.

		Parameters
		----------
		EquFile : str
			Path to the equation file to load and start.

		Returns
		-------
		int
			0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1):
			return self.__lib.XilEnv_StartEquations(self.c_str(EquFile))
		else:
			return -1

	def StopEquations(self) -> int:
		"""
		Stops the currently running equations in the openXilEnv instance.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StopEquations()
		else:
			return -1


	def StartGenerator(self, GenFile: str) -> int:
		"""
		Starts the generator using the specified generator file.

		Args:
			GenFile (str): Path to the generator configuration file.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartGenerator(self.c_str(GenFile))
		else:
			return -1

	def StopGenerator(self) -> int:
		"""
		Stops the currently running generator in the openXilEnv instance.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StopGenerator()
		else:
			return -1


# GUI
	def LoadDesktop(self, DskFile: str) -> int:
		"""
		Load a desktop layout from a file.

		Args:
			DskFile (str): Path to the desktop file.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_LoadDesktop(self.c_str(DskFile))
		else:
			return -1

	def SaveDesktop(self, DskFile: str) -> int:
		"""
		Save the current desktop layout to a file.

		Args:
			DskFile (str): Path to the desktop file.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SaveDesktop(self.c_str(DskFile))
		else:
			return -1

	def ClearDesktop(self) -> int:
		"""
		Clear the current desktop layout.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_ClearDesktop()
		else:
			return -1

	def CreateDialog(self, DialogName: str) -> int:
		"""
		Create a dialog window.

		Args:
			DialogName (str): Name of the dialog.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_CreateDialog(self.c_str(DialogName))
		else:
			return -1

	def AddDialogItem(self, Description: str, VariName: str) -> int:
		"""
		Add an item to a dialog window.

		Args:
			Description (str): Description of the dialog item.
			VariName (str): Name of the variable associated with the item.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_AddDialogItem(self.c_str(Description), self.c_str(VariName))
		else:
			return -1

	def ShowDialog(self) -> int:
		"""
		Show the created dialog window.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_ShowDialog()
		else:
			return -1

	def IsDialogClosed(self) -> int:
		"""
		Check if the dialog window is closed.

		Returns:
			int: 1 if closed, 0 if open, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_IsDialogClosed()
		else:
			return -1

	def SelectSheet(self, SheetName: str) -> int:
		"""
		Select a sheet by name.

		Args:
			SheetName (str): Name of the sheet to select.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SelectSheet(self.c_str(SheetName))
		else:
			return -1

	def AddSheet(self, SheetName: str) -> int:
		"""
		Add a new sheet by name.

		Args:
			SheetName (str): Name of the sheet to add.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_AddSheet(self.c_str(SheetName))
		else:
			return -1

	def DeleteSheet(self, SheetName: str) -> int:
		"""
		Delete a sheet by name.

		Args:
			SheetName (str): Name of the sheet to delete.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_DeleteSheet(self.c_str(SheetName))
		else:
			return -1

	def RenameSheet(self, OldSheetName: str, NewSheetName: str) -> int:
		"""
		Rename a sheet.

		Args:
			OldSheetName (str): Current name of the sheet.
			NewSheetName (str): New name for the sheet.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_RenameSheet(self.c_str(OldSheetName), self.c_str(NewSheetName))
		else:
			return -1

	def OpenWindow(self, WindowName: str) -> int:
		"""
		Open a window by name.

		Args:
			WindowName (str): Name of the window to open.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_OpenWindow(self.c_str(WindowName))
		else:
			return -1

	def CloseWindow(self, WindowName: str) -> int:
		"""
		Close a window by name.

		Args:
			WindowName (str): Name of the window to close.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_CloseWindow(self.c_str(WindowName))
		else:
			return -1

	def DeleteWindow(self, WindowName: str) -> int:
		"""
		Delete a window by name.

		Args:
			WindowName (str): Name of the window to delete.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_DeleteWindow(self.c_str(WindowName))
		else:
			return -1

	def ImportWindow(self, WindowName: str, FileName: str) -> int:
		"""
		Import a window from a file.

		Args:
			WindowName (str): Name of the window.
			FileName (str): Path to the file to import.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_ImportWindow(self.c_str(WindowName), self.c_str(FileName))
		else:
			return -1

	def ExportWindow(self, WindowName: str, FileName: str) -> int:
		"""
		Export a window to a file.

		Args:
			WindowName (str): Name of the window.
			FileName (str): Path to the file to export.

		Returns:
			int: 0 on success, -1 if not connected or on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_ExportWindow(self.c_str(WindowName), self.c_str(FileName))
		else:
			return -1

# Blackboard

	def readMultipleSignals(self, signalNames: list[str]) -> list[float]:
		"""
		Read multiple signal values by their names.

		Args:
			signalNames (list[str]): List of signal names to read.

		Returns:
			list[float]: List of signal values.
		"""
		signalIdList = [self.__attachedVar[name] for name in signalNames]
		signalValues = self.ReadFrame(signalIdList)
		return signalValues

	def readSignal(self, signalName: str) -> float:
		"""
		Read a single signal value by its name.

		Args:
			signalName (str): Name of the signal to read.

		Returns:
			float: Value of the signal.
		"""
		value = self.Get(self.__attachedVar[signalName])
		return value

	def __removeVariables(self) -> None:
		"""
		Remove all attached variables from the internal dictionary.
		"""
		for signalName, variableId in self.__attachedVar.items():
			self.RemoveVari(variableId)

	def writeMultipleSignals(self, signalNames: list[str], signalValues: list[float]) -> None:
		"""
		Write multiple signal values by their names.

		Args:
			signalNames (list[str]): List of signal names to write.
			signalValues (list[float]): List of values to write.
		"""
		signalIdList = [self.__attachedVar[name] for name in signalNames]
		self.WriteFrame(signalIdList, signalValues)

	def writeSignal(self, signalName: str, rawValue: float) -> None:
		"""
		Write a single signal value by its name.

		Args:
			signalName (str): Name of the signal to write.
			rawValue (float): Value to write.
		"""
		self.Set(self.__attachedVar[signalName], rawValue)

	def attachVariables(self, signalNames: list[str]) -> None:
		"""
		Attach a list of signal names to the internal variable dictionary.

		Parameters
		----------
		signalNames : list of str
			List of signal names to attach.

		Returns
		-------
		None
		"""
		for variableName in signalNames:
			variableId = self.AttachVari(variableName)
			if variableId > 0:
				self.__attachedVar[variableName] = variableId
			else:
				print(f"Could not attach variable {variableName}")

	def __attachDefaultXilEnvVariables(self) -> None:
		"""
		Attach default XilEnv variables to the internal variable dictionary.

		Returns
		-------
		None
		"""
		for variableName in self.__defaultXilEnvVariables:
			variableId = self.AttachVari(variableName)
			if variableId > 0:
				self.__attachedVar[variableName] = variableId
			else:
				print(f"Could not attach variable {variableName}")

	def AddVari(self, Label: str, Type: int, Unit: str) -> int:
		"""
		Add a variable to the blackboard.

		Args:
			Label (str): Name of the variable.
			Type (int): Type identifier.
			Unit (str): Unit of the variable.

		Returns:
			int: Variable ID if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_AddVari(self.c_str(Label), Type, self.c_str(Unit))
		else:
			return -1

	def RemoveVari(self, Vid: int) -> int:
		"""
		Remove a variable from the blackboard by its ID.

		Args:
			Vid (int): Variable ID.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_RemoveVari(Vid)
		else:
			return -1

	def AttachVari(self, Label: str) -> int:
		"""
		Attach a variable by its name and return its ID.

		Args:
			Label (str): Name of the variable to attach.

		Returns:
			int: Variable ID if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_AttachVari(self.c_str(Label))
		else:
			return -1

	def Get(self, Vid: int) -> float:
		"""
		Get the value of a variable by its ID.

		Args:
			Vid (int): Variable ID.

		Returns:
			float: Value of the variable, or 0 if not connected.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_Get(Vid)
		else:
			return 0.0

	def GetPhys(self, Vid: int) -> float:
		"""
		Get the physical value of a variable by its ID.

		Args:
			Vid (int): Variable ID.

		Returns:
			float: Physical value of the variable, or 0 if not connected.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_GetPhys(Vid)
		else:
			return 0.0

	def Set(self, Vid: int, Value: float) -> None:
		"""
		Set the value of a variable by its ID.

		Args:
			Vid (int): Variable ID.
			Value (float): Value to set.
		"""
		if self.__SuccessfulConnected == 1:
			self.__lib.XilEnv_Set(Vid, Value)

	def SetPhys(self, Vid: int, Value: float) -> int:
		"""
		Set the physical value of a variable by its ID.

		Args:
			Vid (int): Variable ID.
			Value (float): Physical value to set.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetPhys(Vid, Value)
		else:
			return -1

	def WriteFrame(self, vidList: list[int], valueList: list[float]) -> int:
		"""
		Write a frame of values to multiple variables by their IDs.

		Args:
			vidList (list[int]): List of variable IDs.
			valueList (list[float]): List of values to write.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if (self.__SuccessfulConnected == 1) and len(vidList) == len(valueList):
			listSize = len(vidList)
			vidArray = (ct.c_int * listSize)(*vidList)
			valueArray = (ct.c_double * listSize)(*valueList)
			self.__lib.XilEnv_WriteFrame(vidArray, valueArray, listSize)
			return 0
		else:
			return -1

	def ReadFrame(self, vidList: list[int]) -> list[float]:
		"""
		Read a frame of values from multiple variables by their IDs.

		Args:
			vidList (list[int]): List of variable IDs.

		Returns:
			list[float]: List of variable values, or -1 if not connected.
		"""
		if self.__SuccessfulConnected == 1:
			listSize = len(vidList)
			vidArray = (ct.c_int * listSize)(*vidList)
			valueArrayToReturn = (ct.c_double * listSize)()
			self.__lib.XilEnv_GetFrame(vidArray, valueArrayToReturn, listSize)
			return list(valueArrayToReturn)
		else:
			return -1

	def Equ(self, Equ: str) -> float:
		"""
		Evaluate an equation string in the XilEnv context.

		Args:
			Equ (str): Equation string to evaluate.

		Returns:
			float: Result of the evaluation, or 0 if not connected.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_Equ(self.c_str(Equ))
		else:
			return 0.0

	def WrVariEnable(self, Label: str, Process: str) -> int:
		"""
		Enable write access for a variable in a process.

		Args:
			Label (str): Variable name.
			Process (str): Process name.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_WrVariEnable(self.c_str(Label), self.c_str(Process))
		else:
			return -1

	def WrVariDisable(self, Label: str, Process: str) -> int:
		"""
		Disable write access for a variable in a process.

		Args:
			Label (str): Variable name.
			Process (str): Process name.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_WrVariDisable(self.c_str(Label), self.c_str(Process))
		else:
			return -1

	def IsWrVariEnabled(self, Label: str, Process: str) -> int:
		"""
		Check if write access is enabled for a variable in a process.

		Args:
			Label (str): Variable name.
			Process (str): Process name.

		Returns:
			int: 1 if enabled, 0 if disabled, -1 if not connected.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_IsWrVariEnabled(self.c_str(Label), self.c_str(Process))
		else:
			return -1

	def LoadRefList(self, RefList: str, Process: str) -> int:
		"""
		Load a reference list for a process.

		Args:
			RefList (str): Reference list name.
			Process (str): Process name.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_LoadRefList(self.c_str(RefList), self.c_str(Process))
		else:
			return -1

	def AddRefList(self, RefList: str, Process: str) -> int:
		"""
		Add a reference list to a process.

		Args:
			RefList (str): Reference list name.
			Process (str): Process name.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_AddRefList(self.c_str(RefList), self.c_str(Process))
		else:
			return -1

	def SaveRefList(self, RefList: str, Process: str) -> int:
		"""
		Save a reference list for a process.

		Args:
			RefList (str): Reference list name.
			Process (str): Process name.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SaveRefList(self.c_str(RefList), self.c_str(Process))
		else:
			return -1

	def ExportRobFile(self, RobFile: str) -> int:
		"""
		Export a ROB file.

		Args:
			RobFile (str): Path to the ROB file.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_ExportRobFile(self.c_str(RobFile))
		else:
			return -1

	def GetVariConversionType(self, Vid: int) -> int:
		"""
		Get the conversion type of a variable by its ID.

		Args:
			Vid (int): Variable ID.

		Returns:
			int: Conversion type if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_GetVariConversionType(Vid)
		else:
			return -1

	def GetVariConversionString(self, Vid: int) -> str:
		"""
		Get the conversion string of a variable by its ID.

		Args:
			Vid (int): Variable ID.

		Returns:
			str: Conversion string if successful, empty string otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.str_c(self.__lib.XilEnv_GetVariConversionString(Vid))
		else:
			return ""

	def XilEnv_SetVariConversion(self, Vid: int, ConversionType: int, Conversion: str) -> int:
		"""
		Set the conversion for a variable by its ID.

		Args:
			Vid (int): Variable ID.
			ConversionType (int): Conversion type.
			Conversion (str): Conversion string.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetVariConversion(Vid, ConversionType, self.c_str(Conversion))
		else:
			return -1

	def GetVariType(self, Vid: int) -> int:
		"""
		Get the type of a variable by its ID.

		Args:
			Vid (int): Variable ID.

		Returns:
			int: Variable type if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_GetVariType(Vid)
		else:
			return -1

	def GetVariUnit(self, Vid: int) -> str:
		"""
		Get the unit of a variable by its ID.

		Args:
			Vid (int): Variable ID.

		Returns:
			str: Unit string if successful, empty string otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.str_c(self.__lib.XilEnv_GetVariUnit(Vid))
		else:
			return ""

	def SetVariUnit(self, Vid: int, Unit: str) -> int:
		"""
		Set the unit of a variable by its ID.

		Args:
			Vid (int): Variable ID.
			Unit (str): Unit string to set.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetVariUnit(Vid, self.c_str(Unit))
		else:
			return -1

	def GetVariMin(self, Vid: int) -> float:
		"""
		Get the minimum value of a variable by its ID.

		Args:
			Vid (int): Variable ID.

		Returns:
			float: Minimum value if successful, 0 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_GetVariMin(Vid)
		else:
			return 0.0

	def GetVariMax(self, Vid: int) -> float:
		"""
		Get the maximum value of a variable by its ID.

		Args:
			Vid (int): Variable ID.

		Returns:
			float: Maximum value if successful, 0 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_GetVariMax(Vid)
		else:
			return 0.0

	def SetVariMin(self, Vid: int, Min: float) -> int:
		"""
		Set the minimum value of a variable by its ID.

		Args:
			Vid (int): Variable ID.
			Min (float): Minimum value to set.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetVariMin(Vid, Min)
		else:
			return -1

	def SetVariMax(self, Vid: int, Max: float) -> int:
		"""
		Set the maximum value of a variable by its ID.

		Args:
			Vid (int): Variable ID.
			Max (float): Maximum value to set.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetVariMax(Vid, Max)
		else:
			return -1

	def GetNextVari(self, Flag: int, Filter: str) -> str:
		"""
		Get the next variable matching a filter and flag.

		Args:
			Flag (int): Filter flag.
			Filter (str): Filter string.

		Returns:
			str: Variable name if found, empty string otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.str_c(self.__lib.XilEnv_GetNextVari(Flag, self.c_str(Filter)))
		else:
			return ""

	def GetNextVariEx(self, Flag: int, Filter: str, Process: str, AccessFlags: int) -> str:
		"""
		Get the next variable matching a filter, process, and access flags.

		Args:
			Flag (int): Filter flag.
			Filter (str): Filter string.
			Process (str): Process name.
			AccessFlags (int): Access flags.

		Returns:
			str: Variable name if found, empty string otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.str_c(self.__lib.XilEnv_GetNextVariEx(Flag, self.c_str(Filter), self.c_str(Process), AccessFlags))
		else:
			return ""

	def GetVariEnum(self, Flag: int, Vid: int, Value: float) -> str:
		"""
		Get the enumeration string for a variable value.

		Args:
			Flag (int): Enumeration flag.
			Vid (int): Variable ID.
			Value (float): Value to enumerate.

		Returns:
			str: Enumeration string if found, empty string otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.str_c(self.__lib.XilEnv_GetVariEnum(Flag, Vid, Value))
		else:
			return ""

	def GetVariDisplayFormatWidth(self, Vid: int) -> int:
		"""
		Get the display format width for a variable by its ID.

		Args:
			Vid (int): Variable ID.

		Returns:
			int: Display format width if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_GetVariDisplayFormatWidth(Vid)
		else:
			return -1

	def GetVariDisplayFormatPrec(self, Vid: int) -> int:
		"""
		Get the display format precision for a variable by its ID.

		Args:
			Vid (int): Variable ID.

		Returns:
			int: Display format precision if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_GetVariDisplayFormatPrec(Vid)
		else:
			return -1

	def SetVariDisplayFormat(self, Vid: int, Width: int, Prec: int) -> int:
		"""
		Set the display format width and precision for a variable by its ID.

		Args:
			Vid (int): Variable ID.
			Width (int): Display width.
			Prec (int): Display precision.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetVariDisplayFormat(Vid, Width, Prec)
		else:
			return -1

	def ImportVariProperties(self, Filename: str) -> int:
		"""
		Import variable properties from a file.

		Args:
			Filename (str): Path to the properties file.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_ImportVariProperties(self.c_str(Filename))
		else:
			return -1

	def EnableRangeControl(self, ProcessNameFilter: str, VariableNameFilter: str) -> int:
		"""
		Enable range control for a variable in a process.

		Args:
			ProcessNameFilter (str): Process name filter.
			VariableNameFilter (str): Variable name filter.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_EnableRangeControl(self.c_str(ProcessNameFilter), self.c_str(VariableNameFilter))
		else:
			return -1

	def DisableRangeControl(self, ProcessNameFilter: str, VariableNameFilter: str) -> int:
		"""
		Disable range control for a variable in a process.

		Args:
			ProcessNameFilter (str): Process name filter.
			VariableNameFilter (str): Variable name filter.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_DisableRangeControl(self.c_str(ProcessNameFilter), self.c_str(VariableNameFilter))
		else:
			return -1

	def ReferenceSymbol(self, Symbol: str, DisplayName: str, Process: str, Unit: str, ConversionType: int, Conversion: str, Min: float, Max: float, Color: int, Width: int, Precision: int, Flags: int) -> int:
		"""
		Reference a symbol with detailed properties.

		Args:
			Symbol (str): Symbol name.
			DisplayName (str): Display name.
			Process (str): Process name.
			Unit (str): Unit string.
			ConversionType (int): Conversion type.
			Conversion (str): Conversion string.
			Min (float): Minimum value.
			Max (float): Maximum value.
			Color (int): Color code.
			Width (int): Display width.
			Precision (int): Display precision.
			Flags (int): Additional flags.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_ReferenceSymbol(self.c_str(Symbol), self.c_str(DisplayName), self.c_str(Process), self.c_str(Unit), ConversionType, self.c_str(Conversion), Min, Max, Color, Width, Precision, Flags)
		else:
			return -1

	def DereferenceSymbol(self, Symbol: str, DisplayName: str, Process: str, Flags: int) -> int:
		"""
		Dereference a symbol by its properties.

		Args:
			Symbol (str): Symbol name.
			DisplayName (str): Display name.
			Process (str): Process name.
			Flags (int): Additional flags.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_DereferenceSymbol(self.c_str(Symbol), self.c_str(DisplayName), self.c_str(Process), Flags)
		else:
			return -1

	def GetRaw(self, Vid: int, RetValue: 'BB_VARI') -> int:
		"""
		Get the raw value of a variable by its ID.

		Args:
			Vid (int): Variable ID.
			RetValue (BB_VARI): BB_VARI structure to receive the value.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_GetRaw(Vid, RetValue)
		else:
			return -1

	def SetRaw(self, Vid: int, Type: int, Value: 'BB_VARI', Flag: int) -> int:
		"""
		Set the raw value of a variable by its ID.

		Args:
			Vid (int): Variable ID.
			Type (int): Data type.
			Value (BB_VARI): BB_VARI structure with the value to set.
			Flag (int): Additional flags.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetRaw(Vid, Type, Value, Flag)
		else:
			return -1

# Calibration
	def LoadSvl(self, SvlFile: str, Process: str) -> int:
		"""
		Load an SVL file for a given process.

		Args:
			SvlFile (str): Path to the SVL file.
			Process (str): Process name.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_LoadSvl(self.c_str(SvlFile), self.c_str(Process))
		else:
			return -1

	def SaveSvl(self, SvlFile: str, Process: str, Filter: str) -> int:
		"""
		Save an SVL file for a given process and filter.

		Args:
			SvlFile (str): Path to the SVL file.
			Process (str): Process name.
			Filter (str): Filter string.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SaveSvl(self.c_str(SvlFile), self.c_str(Process), self.c_str(Filter))
		else:
			return -1

	def SaveSal(self, SalFile: str, Process: str, Filter: str) -> int:
		"""
		Save a SAL file for a given process and filter.

		Args:
			SvlFile (str): Path to the SAL file.
			Process (str): Process name.
			Filter (str): Filter string.

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SaveSal(self.c_str(SalFile), self.c_str(Process), self.c_str(Filter))
		else:
			return -1

	def GetSymbolRaw(self, Symbol: str, Process: str, Flags: int, RetValue: 'BB_VARI') -> int:
		"""
		Get the raw value of a symbol for a given process.

		Args:
			Symbol (str): Symbol name.
			Process (str): Process name.
			Flags (int): Flags for the operation.
			RetValue (BB_VARI): BB_VARI structure to receive the value.

		Returns:
			int: Data type if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_GetSymbolRaw(self.c_str(Symbol), self.c_str(Process), Flags, RetValue)
		else:
			return -1

	def GetSymbolRawEx(self, Symbol: str, Process: str):
		"""
		Get the raw value of a symbol for a given process and return the Python value.

		Args:
			Symbol (str): Symbol name.
			Process (str): Process name.

		Returns:
			The value of the symbol in the appropriate Python type, or None if not connected or unknown type.
		"""
		if self.__SuccessfulConnected == 1:
			Data = (BB_VARI)()
			DataType = self.__lib.XilEnv_GetSymbolRaw(self.c_str(Symbol), self.c_str(Process), 0x1000, Data) # flags = 0x1000 -> no error message
			if (DataType==0):  # int8_t
				return Data.b
			elif (DataType==1): # uint8_t
				return Data.ub
			elif (DataType==2): # int16_t
				return Data.w
			elif (DataType==3): # uint16_t
				return Data.uw
			elif (DataType==4): # int32_t
				return Data.dw
			elif (DataType==5): # uint32_t
				return Data.udw
			elif (DataType==6): # float
				return Data.f
			elif (DataType==7): # double
				return Data.d
			elif (DataType==36): # int64_t
				return Data.qw
			elif (DataType==37): # uint64_t
				return Data.uqw
			else:
				return None
		else:
			return None

	def SetSymbolRaw(self, Symbol: str, Process: str, Flags: int, DataType: int, Value) -> int:
		"""
		Set the raw value of a symbol for a given process.

		Args:
			Symbol (str): Symbol name.
			Process (str): Process name.
			Flags (int): Flags for the operation.
			DataType (int): Data type identifier.
			Value: Value to set (type depends on DataType).

		Returns:
			int: 0 if successful, -1 otherwise.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetSymbolRaw(self.c_str(Symbol), self.c_str(Process), Flags, DataType, Value)
		else:
			return -1

	def SetSymbolRawEx(self, Symbol: str, Process: str, Value) -> None:
		"""
		Set the raw value of a symbol for a given process using Python value type.

		Args:
			Symbol (str): Symbol name.
			Process (str): Process name.
			Value: Value to set (type will be mapped to BB_VARI).
		"""
		if self.__SuccessfulConnected == 1:
			Data = (BB_VARI)()
			DataType = self.__lib.XilEnv_GetSymbolRaw(self.c_str(Symbol), self.c_str(Process), 0x1000, Data) # flags = 0x1000 -> no error message
			if (DataType==0):  # int8_t
				Data.b = Value
			elif (DataType==1): # uint8_t
				Data.ub =Value
			elif (DataType==2): # int16_t
				Data.w = Value
			elif (DataType==3): # uint16_t
				Data.uw = Value
			elif (DataType==4): # int32_t
				Data.dw =Value
			elif (DataType==5): # uint32_t
				Data.udw = Value
			elif (DataType==6): # float
				Data.f = Value
			elif (DataType==7): # double
				Data.d = Value
			elif (DataType==36): # int64_t
				Data.qw =Value
			elif (DataType==37): # uint64_t
				Data.uqw = Value
			else:
				return -1
			return self.__lib.XilEnv_SetSymbolRaw(self.c_str(Symbol), self.c_str(Process), 0x1000, DataType, Data) # flags = 0x1000 -> no error message
		else:
			return -1

# CAN
	def LoadCanVariante(self, CanFile: str, Channel: int) -> int:
		"""
		Load a CAN variant file for a specific channel.

		Args:
			CanFile (str): Path to the CAN variant file.
			Channel (int): Channel number.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_LoadCanVariante(self.c_str(CanFile), Channel)
		else:
			return -1

	def LoadAndSelCanVariante(self, CanFile: str, Channel: int) -> int:
		"""
		Load and select a CAN variant file for a specific channel.

		Args:
			CanFile (str): Path to the CAN variant file.
			Channel (int): Channel number.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_LoadAndSelCanVariante(self.c_str(CanFile), Channel)
		else:
			return -1

	def AppendCanVariante(self, CanFile: str, Channel: int) -> int:
		"""
		Append a CAN variant file to a specific channel.

		Args:
			CanFile (str): Path to the CAN variant file.
			Channel (int): Channel number.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_AppendCanVariante(self.c_str(CanFile), Channel)
		else:
			return -1

	def DelAllCanVariants(self) -> None:
		"""
		Delete all loaded CAN variants.

		Returns:
			None
		"""
		if self.__SuccessfulConnected == 1:
			self.__lib.XilEnv_DelAllCanVariants()

	def SetCanChannelCount(self, ChannelCount: int) -> int:
		"""
		Set the number of CAN channels.

		Args:
			ChannelCount (int): Number of CAN channels.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetCanChannelCount(ChannelCount)
		else:
			return -1

	def SetCanChannelStartupState(self, Channel: int, State: int) -> int:
		"""
		Set the startup state for a CAN channel.

		Args:
			Channel (int): Channel number.
			State (int): Startup state value.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetCanChannelStartupState(Channel, State)
		else:
			return -1


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
		"""
		Transmit a standard CAN message.

		Args:
			Id (int): CAN message ID.
			Ext (int): Extended ID flag.
			Size (int): Data length.
			Data0-Data7 (int): Data bytes.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_TransmitCAN(self, Id, Ext, Size, Data0, Data1, Data2, Data3, Data4, Data5, Data6, Data7)
		else:
			return -1

	def XilEnv_TransmitCANFd(self, Id: int, Ext: int, Size: int, Data: bytes) -> int:
		"""
		Transmit a CAN FD message.

		Args:
			Id (int): CAN message ID.
			Ext (int): Extended ID flag.
			Size (int): Data length.
			Data (bytes): Data bytes.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			if Size > len(Data):
				Size = len(Data)
			return self.__lib.XilEnv_TransmitCANFd(self, Id, Ext, Size, Data)
		else:
			return -1

# CAN-Message-Queues (FIFO's)

	def OpenCANQueue(self, Depth: int) -> int:
		"""
		Open a CAN message queue (FIFO).

		Args:
			Depth (int): Queue depth.

		Returns:
			int: Queue handle or -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_OpenCANQueue(Depth)
		else:
			return -1

	def OpenCANFdQueue(self, Depth: int, FdFlag: int) -> int:
		"""
		Open a CAN FD message queue (FIFO).

		Args:
			Depth (int): Queue depth.
			FdFlag (int): FD flag.

		Returns:
			int: Queue handle or -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_OpenCANFdQueue(Depth, FdFlag)
		else:
			return -1

	def SetCANAcceptanceWindows(self, Size: int, AcceptanceWindows) -> int:
		"""
		Set CAN acceptance windows.

		Args:
			Size (int): Number of acceptance windows.
			AcceptanceWindows: Acceptance window configuration.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetCANAcceptanceWindows(Size, AcceptanceWindows)
		else:
			return -1

	def FlushCANQueue(self, Flags: int) -> int:
		"""
		Flush the CAN message queue.

		Args:
			Flags (int): Flush flags.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_FlushCANQueue(Flags)
		else:
			return -1

	def ReadCANQueue(self, ReadMaxElements: int, Elements) -> int:
		"""
		Read messages from the CAN queue.

		Args:
			ReadMaxElements (int): Maximum number of elements to read.
			Elements: Buffer to store read elements.

		Returns:
			int: Number of elements read or -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_ReadCANQueue(ReadMaxElements, Elements)
		else:
			return -1

	def ReadCANFdQueue(self, ReadMaxElements: int, Elements) -> int:
		"""
		Read messages from the CAN FD queue.

		Args:
			ReadMaxElements (int): Maximum number of elements to read.
			Elements: Buffer to store read elements.

		Returns:
			int: Number of elements read or -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_ReadCANFdQueue(ReadMaxElements, Elements)
		else:
			return -1

	def TransmitCANQueue(self, WriteElements: int, Elements) -> int:
		"""
		Transmit messages to the CAN queue.

		Args:
			WriteElements (int): Number of elements to write.
			Elements: Buffer containing elements to transmit.

		Returns:
			int: Number of elements written or -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_TransmitCANQueue(WriteElements, Elements)
		else:
			return -1

	def TransmitCANFdQueue(self, WriteElements: int, Elements) -> int:
		"""
		Transmit messages to the CAN FD queue.

		Args:
			WriteElements (int): Number of elements to write.
			Elements: Buffer containing elements to transmit.

		Returns:
			int: Number of elements written or -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_TransmitCANFdQueue(WriteElements, Elements)
		else:
			return -1

	def CloseCANQueue(self) -> int:
		"""
		Close the CAN message queue.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_CloseCANQueue()
		else:
			return -1

# CCP
	def LoadCCPConfig(self, Connection: int, CcpFile: str) -> int:
		"""
		Load a CCP configuration file for a given connection.

		Args:
			Connection (int): Connection handle or identifier.
			CcpFile (str): Path to the CCP configuration file.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_LoadCCPConfig(self.c_str(CcpFile), Connection)
		else:
			return -1

	def StartCCPBegin(self, Connection: int) -> int:
		"""
		Begin a CCP session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartCCPBegin(Connection)
		else:
			return -1

	def StartCCPAddVar(self, Connection: int, Variable: str) -> int:
		"""
		Add a variable to the CCP session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.
			Variable (str): Name of the variable to add.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartCCPAddVar(Connection, self.c_str(Variable))
		else:
			return -1

	def StartCCPEnd(self, Connection: int) -> int:
		"""
		End the CCP session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartCCPEnd(Connection)
		else:
			return -1

	def StopCCP(self, Connection: int) -> int:
		"""
		Stop the CCP session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StopCCP(Connection)
		else:
			return -1

	def StartCCPCalBegin(self, Connection: int) -> int:
		"""
		Begin a CCP calibration session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartCCPCalBegin(Connection)
		else:
			return -1

	def StartCCPCalAddVar(self, Connection: int, Variable: str) -> int:
		"""
		Add a variable to the CCP calibration session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.
			Variable (str): Name of the variable to add.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartCCPCalAddVar(Connection, self.c_str(Variable))
		else:
			return -1

	def StartCCPCalEnd(self, Connection: int) -> int:
		"""
		End the CCP calibration session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartCCPCalEnd(Connection)
		else:
			return -1

	def StopCCPCal(self, Connection: int) -> int:
		"""
		Stop the CCP calibration session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StopCCPCal(Connection)
		else:
			return -1

# XCP
	def LoadXCPConfig(self, Connection: int, XcpFile: str) -> int:
		"""
		Load an XCP configuration file for a given connection.

		Args:
			Connection (int): Connection handle or identifier.
			XcpFile (str): Path to the XCP configuration file.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_LoadXCPConfig(self.c_str(XcpFile), Connection)
		else:
			return -1

	def StartXCPBegin(self, Connection: int) -> int:
		"""
		Begin an XCP session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartXCPBegin(Connection)
		else:
			return -1

	def StartXCPAddVar(self, Connection: int, Variable: str) -> int:
		"""
		Add a variable to the XCP session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.
			Variable (str): Name of the variable to add.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartXCPAddVar(Connection, self.c_str(Variable))
		else:
			return -1

	def StartXCPEnd(self, Connection: int) -> int:
		"""
		End the XCP session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartXCPEnd(Connection)
		else:
			return -1

	def StopXCP(self, Connection: int) -> int:
		"""
		Stop the XCP session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StopXCP(Connection)
		else:
			return -1

	def StartXCPCalBegin(self, Connection: int) -> int:
		"""
		Begin an XCP calibration session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartXCPCalBegin(Connection)
		else:
			return -1

	def StartXCPCalAddVar(self, Connection: int, Variable: str) -> int:
		"""
		Add a variable to the XCP calibration session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.
			Variable (str): Name of the variable to add.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartXCPCalAddVar(Connection, self.c_str(Variable))
		else:
			return -1

	def StartXCPCalEnd(self, Connection: int) -> int:
		"""
		End the XCP calibration session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StartXCPCalEnd(Connection)
		else:
			return -1

	def StopXCPCal(self, Connection: int) -> int:
		"""
		Stop the XCP calibration session for the specified connection.

		Args:
			Connection (int): Connection handle or identifier.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_StopXCPCal(Connection)
		else:
			return -1

# CAN bit error
	def SetCanErr(self, Channel: int, Id: int, Startbit: int, Bitsize: int, ByteOrder: str, Cycles: int, BitErrValue: int) -> int:
		"""
		Inject a bit error into a CAN message by bit position.

		Args:
			Channel (int): CAN channel number.
			Id (int): CAN message ID.
			Startbit (int): Start bit position.
			Bitsize (int): Number of bits to affect.
			ByteOrder (str): Byte order (e.g., 'Intel' or 'Motorola').
			Cycles (int): Number of cycles to apply the error.
			BitErrValue (int): Value to set for the bit error.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetCanErr(Channel, Id, Startbit, Bitsize, self.c_str(ByteOrder), Cycles, BitErrValue)
		else:
			return -1

	def SetCanErrSignalName(self, Channel: int, Id: int, Signalname: str, Cycles: int, BitErrValue: int) -> int:
		"""
		Inject a bit error into a CAN message by signal name.

		Args:
			Channel (int): CAN channel number.
			Id (int): CAN message ID.
			Signalname (str): Name of the signal.
			Cycles (int): Number of cycles to apply the error.
			BitErrValue (int): Value to set for the bit error.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetCanErrSignalName(Channel, Id, self.c_str(Signalname), Cycles, BitErrValue)
		else:
			return -1

	def ClearCanErr(self) -> int:
		"""
		Clear all CAN bit errors.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_ClearCanErr()
		else:
			return -1

	def SetCanSignalConversion(self, Channel: int, Id: int, Signalname: str, Conversion: str) -> int:
		"""
		Set a conversion formula for a CAN signal.

		Args:
			Channel (int): CAN channel number.
			Id (int): CAN message ID.
			Signalname (str): Name of the signal.
			Conversion (str): Conversion formula or string.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_SetCanSignalConversion(Channel, Id, self.c_str(Signalname), self.c_str(Conversion))
		else:
			return -1

	def ResetCanSignalConversion(self, Channel: int, Id: int, Signalname: str) -> int:
		"""
		Reset the conversion formula for a specific CAN signal.

		Args:
			Channel (int): CAN channel number.
			Id (int): CAN message ID.
			Signalname (str): Name of the signal.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_ResetCanSignalConversion(Channel, Id, self.c_str(Signalname))
		else:
			return -1

	def XilEnv_ResetAllCanSignalConversion(self, Channel: int, Id: int) -> int:
		"""
		Reset all conversion formulas for a CAN message.

		Args:
			Channel (int): CAN channel number.
			Id (int): CAN message ID.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			return self.__lib.XilEnv_ResetAllCanSignalConversion(Channel, Id)
		else:
			return -1

# CAN Recorder
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
		"""
		Start recording CAN messages to a file with specified display and trigger options.

		Args:
			FileName (str): Output file name for the recording.
			TriggerEqu (str): Trigger equation string.
			DisplayColumnCounterFlag (int): Show counter column flag.
			DisplayColumnTimeAbsoluteFlag (int): Show absolute time column flag.
			DisplayColumnTimeDiffFlag (int): Show time difference column flag.
			DisplayColumnTimeDiffMinMaxFlag (int): Show min/max time diff column flag.
			AcceptanceWindows: Acceptance window configuration.

		Returns:
			int: 0 on success, -1 on failure.
		"""
		if self.__SuccessfulConnected == 1:
			Size = len(AcceptanceWindows)
			return self.__lib.XilEnv_StartCANRecorder(
				self.c_str(FileName),
				self.c_str(TriggerEqu),
				DisplayColumnCounterFlag,
				DisplayColumnTimeAbsoluteFlag,
				DisplayColumnTimeDiffFlag,
				DisplayColumnTimeDiffMinMaxFlag,
				Size,
				AcceptanceWindows
			)
		else:
			return -1

	def StopCANRecorder (self):
		def StopCANRecorder(self) -> int:
			"""
			Stop the CAN message recording.

			Returns:
				int: 0 on success, -1 on failure.
			"""
			return self.__lib.XilEnv_StopCANRecorder()

# A2lLink

	def GetA2lLinkNo(self, Process: str) -> int:
		"""
		Get the A2L link number for an external process.

		Args:
			Process (str): Name of the external process.

		Returns:
			int: Link number, or -1 if not found.
		"""
		return self.__lib.XilEnv_GetLinkToExternProcess(self.c_str(Process))

	def FetchA2lData(self, LinkNo: int, Label: str, Flags: int, TypeMask: int = 0xFFFF):
		"""
		Fetch A2L data for a given link and label.

		Args:
			LinkNo (int): Link number.
			Label (str): Name of the label to fetch.
			Flags (int): Flags for data retrieval.
			TypeMask (int, optional): Type mask for filtering. Defaults to 0xFFFF.

		Returns:
			A2LData or None: The fetched A2LData object, or None if not found or error.
		"""
		Index = self.__lib.XilEnv_GetIndexFromLink(LinkNo, self.c_str(Label), TypeMask)
		if Index < 0:
			return None
		Error = ct.c_char_p()
		Data = self.__lib.XilEnv_GetDataFromLink(LinkNo, Index, None, Flags, ct.byref(Error))
		if Data is None:
			print('Error: ', Label, ' ', self.str_c(Error.value))
			return None
		else:
			Ret = A2LData(self.__lib, LinkNo, Index, Data)
			return Ret
