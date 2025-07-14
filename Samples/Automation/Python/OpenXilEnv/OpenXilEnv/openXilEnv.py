import os
import subprocess
import sys
import time

from OpenXilEnv.XilEnvRpc import *


class OpenXilEnv:

    def __init__(self, xilEnvPath):
        self.__xilEnvDllPath = os.path.join(xilEnvPath, "XilEnvRpc.dll")
        self.__xilEnvExePath = os.path.join(xilEnvPath, "XilEnvGui.exe")

        self.__xilEnvProcess = None

        self.__xilEnv = XilEnvRpc(self.__xilEnvDllPath)

        self.__defaultXilEnvVariables = [
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

        self.__attachedVar = {}

    def __attachDefaultXilEnvVariables(self):
        for variableName in self.__defaultXilEnvVariables:
            variableId = self.__xilEnv.AttachVari(variableName)
            if variableId > 0:
                self.__attachedVar[variableName] = variableId
            else:
                print(f"Could not attach variable {variableName}")

    def __connectToXilEnv(self, timeoutInSec):
        connectionStatus = -1
        while timeoutInSec:
            connectionStatus = self.__xilEnv.ConnectTo("")
            if connectionStatus == 0 or timeoutInSec == 0:
                return connectionStatus
            timeoutInSec -= 1
            time.sleep(1)

        return connectionStatus

    def __start(self, iniFilePath, startWithGUI, timeoutInSec):
        try:
            if startWithGUI:
                self.__xilEnvProcess = subprocess.Popen([self.__xilEnvExePath, "-ini", iniFilePath],
                                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            else:
                self.__xilEnvProcess = subprocess.Popen([self.__xilEnvExePath, "-ini", iniFilePath, "-nogui"],
                                                        stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            connectionStatus = self.__connectToXilEnv(timeoutInSec)

            if connectionStatus == 0:
                print("Python has been successfully connected to XilEnv")
                self.__waitUntilApplicationIsReady()
                self.__attachDefaultXilEnvVariables()
                return
            else:
                print("Could not connect to openXilEnv")
                sys.exit(1)
        except Exception as e:
            print(e)
            sys.exit(1)

    def __switchRealTimeFactor(self, setting):
        self.__xilEnv.StopScheduler()
        self.__xilEnv.ChangeSettings("NOT_FASTER_THAN_REALTIME", setting)
        self.__xilEnv.ContinueScheduler()

    def __translateSecondsToCycleCounts(self, seconds):
        sampleFrequency = self.readSignal("XilEnv.SampleFrequency")
        return int(seconds * sampleFrequency)

    def __waitUntilApplicationIsReady(self):
        time.sleep(1)

    def isConncted(self) -> bool:
        return self.__xilEnv.IsConnectedTo()

    def attachVariables(self, signalNames):
        for variableName in signalNames:
            variableId = self.__xilEnv.AttachVari(variableName)
            if variableId > 0:
                self.__attachedVar[variableName] = variableId
            else:
                print(f"Could not attach variable {variableName}")

    def disableRealTimeFactorSwitch(self):
        self.__switchRealTimeFactor("No")

    def disconnectAndCloseXil(self):
        if self.__xilEnvProcess:
            self.__removeVariables()

            self.__xilEnv.DisconnectAndClose(0, 0)

    def enableRealTimeFactorSwitch(self):
        self.__switchRealTimeFactor("Yes")

    def readMultipleSignals(self, signalNames):
        signalIdList = [self.__attachedVar[name] for name in signalNames]
        signalValues = self.__xilEnv.ReadFrame(signalIdList)

        return signalValues

    def readSignal(self, signalName):
        value = self.__xilEnv.Get(self.__attachedVar[signalName])

        return value

    def __removeVariables(self):
        for signalName, variableId in self.__attachedVar.items():
            self.__xilEnv.RemoveVari(variableId)

    def waitSeconds(self, waitTimeInSec):
        cycles = self.__translateSecondsToCycleCounts(waitTimeInSec)

        self.__xilEnv.StopScheduler()
        self.__xilEnv.DoNextCyclesAndWait(cycles)
        self.__xilEnv.ContinueScheduler()

        return waitTimeInSec

    def startWithGui(self, iniFilePath, timeoutInSec=5):
        self.__start(iniFilePath, True, timeoutInSec)

    def startWithoutGui(self, iniFilePath, timeoutInSec=5):
        self.__start(iniFilePath, False, timeoutInSec)

    def writeMultipleSignals(self, signalNames, signalValues):
        signalIdList = [self.__attachedVar[name] for name in signalNames]
        self.__xilEnv.WriteFrame(signalIdList, signalValues)

    def writeSignal(self, signalName, rawValue):
        self.__xilEnv.Set(self.__attachedVar[signalName], rawValue)

    def waitUntilValueAlmostMatch(self, signalName, expectedSignalValue, waitTimeInSec, tolerance):
        cycleCount = self.__translateSecondsToCycleCounts(waitTimeInSec)

        minValue = expectedSignalValue - tolerance
        maxValue = expectedSignalValue + tolerance
        equation = f"{signalName} >= {minValue} && {signalName} <= {maxValue}"
        remainingCycles = self.__xilEnv.WaitUntil(equation, cycleCount)

        return remainingCycles

    def waitUntilValueIsGreater(self, signalName, expectedSignalValue, waitTimeInSec):
        cycleCount = self.__translateSecondsToCycleCounts(waitTimeInSec)

        equation = f"{signalName} > {expectedSignalValue}"
        remainingCycles = self.__xilEnv.WaitUntil(equation, cycleCount)

        return remainingCycles

    def waitUntilValueIsSmaller(self, signalName, expectedSignalValue, waitTimeInSec):
        cycleCount = self.__translateSecondsToCycleCounts(waitTimeInSec)

        equation = f"{signalName} < {expectedSignalValue}"
        remainingCycles = self.__xilEnv.WaitUntil(equation, cycleCount)

        return remainingCycles

    def waitUntilValueMatch(self, signalName, expectedSignalValue, waitTimeInSec):
        cycleCount = self.__translateSecondsToCycleCounts(waitTimeInSec)

        equation = f"{signalName} == {expectedSignalValue}"
        remainingCycles = self.__xilEnv.WaitUntil(equation, cycleCount)

        return remainingCycles
