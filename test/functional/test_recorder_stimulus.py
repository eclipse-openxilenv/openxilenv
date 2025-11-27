import pytest
import sys
from OpenXilEnv.openXilEnv import OpenXilEnv
from pathlib import Path

@pytest.fixture(scope="module")

def runningXil(xilEnvInstallationPath : Path, iniFilePath : Path):
    # setup
    xil = OpenXilEnv(xilEnvInstallationPath)
    #xil.startWithGui(iniFilePath)
    xil.startWithoutGui(iniFilePath)
    assert xil.isConncted()
    yield xil
    # teardown
    xil.disconnectAndCloseXil()

def CompareOneFormat(x, Format):
    x.SetEnvironVar("RECORDER_FILE_EXTENSION", Format)
    # replay the recording in one selected format and compare it
    # create a stimulus configuration file
    x.CreateFileWithContent ("%TEMP%\\stimulus.cfg",
                             "HDPLAYER_CONFIG_FILE\n"
                             "HDPLAYER_FILE %TEMP%\\recorder.%RECORDER_FILE_EXTENSION%\n"
                             "TRIGGER StimulusTriggerEvent > 1.0\n"
                             "STARTVARLIST\n"
                             "  VARIABLE TestSignal1\n"
                             "  VARIABLE TestSignal2\n"
                             "  VARIABLE TestSignal3\n"
                             "ENDVARLIST\n");
    # create a second generator configuration file with other signal names
    x.CreateFileWithContent ("%TEMP%\\compare_ramps.gen",
                             "RAMPE StimulusTriggerEvent 2/0 0/0\n"
                             "RAMPE CompareSignal1 0/0 1000/10 0/20\n"
                             "RAMPE CompareSignal2 0/0 0/10 100/10 50/20 0/20\n"
                             "RAMPE CompareSignal3 0/0 123456789/10 0/20\n")
    # create a equation file for error countig if recorder signals are diffent than the generated signals
    x.CreateFileWithContent ("%TEMP%\\compare_equation.equ",
                             "ErrorCounter = ErrorCounter + (CompareSignal1 != TestSignal1) + (CompareSignal2 != TestSignal2) + (CompareSignal3 != TestSignal3);\n")
    x.StartPlayer("%TEMP%\\stimulus.cfg")
    # wait till the stimulus player is in trigger state
    x.WaitUntil ("XilEnv.StimulusPlayer == enum(XilEnv.StimulusPlayer@Trigger)", 100)
    # start the generator, the stimulus player will be triggered by the generator, so they will be started simultaneous
    x.StartGenerator("%TEMP%\\compare_ramps.gen")
    x.WaitUntil("XilEnv.StimulusPlayer == enum(XilEnv.StimulusPlayer@Play)", 10000)  # wait till stimulus player is running
    x.WaitUntil("XilEnv.StimulusPlayer == enum(XilEnv.StimulusPlayer@Sleep)", 10000)  # wait till stimulus player is finished


def test_RecorderStimulus(runningXil : OpenXilEnv) -> None:
    x = runningXil.Connection()
    x.ChangeSettings("NOT_FASTER_THAN_REALTIME", "No")
    x.AddVari ("ErrorCounter", 5, "")
    x.AddVari ("SumOfAllSignals", 7, "")
    x.AddVari ("RecorderTriggerEvent", 5, "")
    x.AddVari ("StimulusTriggerEvent", 5, "")
    # add 3 test and compare signals
    x.AddVari ("TestSignal1", 7, "")
    x.AddVari ("CompareSignal1", 7, "")
    x.AddVari ("TestSignal2", 7, "")
    x.AddVari ("CompareSignal2", 7, "")
    x.AddVari ("TestSignal3", 5, "")
    x.AddVari ("CompareSignal3", 5, "")

    # first we will do a recording of our own gererated signals
    # create a recorder configuration file (recording simultaneous into all 3 supported file formats)
    x.CreateFileWithContent ("%TEMP%\\recorder.cfg",
                             "HDRECORDER_CONFIG_FILE\n"
                             "HDRECORDER_FILE %TEMP%\\recorder.mf4\n"
                             "HDRECORDER_SAMPLERATE = 1\n"
                             "HDRECORDER_SAMPLELENGTH = 100000\n"
                             "TRIGGER RecorderTriggerEvent > 1.0\n"
                             "TEXT_FORMAT\n"
                             "MDF3_FORMAT\n"
                             "MDF4_FORMAT\n"
                             "STARTVARLIST\n"
                             "  VARIABLE TestSignal1\n"
                             "  VARIABLE TestSignal2\n"
                             "  VARIABLE TestSignal3\n"
                             "ENDVARLIST\n")
    x.StartRecorder ("%TEMP%\\recorder.cfg")
    x.WaitUntil ("XilEnv.TraceRecorderRecorder == enum(XilEnv.TraceRecorder@Trigger)", 100)
    # create a generator configuration file
    x.CreateFileWithContent ("%TEMP%\\ramps.gen",
                             "RAMPE RecorderTriggerEvent 0/0 2/0\n"
                             "RAMPE TestSignal1 0/0 1000/10 0/20\n"
                             "RAMPE TestSignal2 0/0 0/10 100/10 50/20 0/20\n"
                             "RAMPE TestSignal3 0/0 123456789/10 0/20\n")
    x.StartGenerator ("%TEMP%\\ramps.gen")
    # wait till the generator is finished
    x.WaitUntil ("XilEnv.Generator == enum(XilEnv.Generator@Play)", 10000) # wait till generator is running
    x.WaitUntil ("XilEnv.Generator == enum(XilEnv.Generator@Sleep)", 10000) # wait till generator is finished
    # do some additional cycles
    x.WaitUntil ("0", 10)
    x.StopRecorder ()
    x.WaitUntil ("XilEnv.TraceRecorder ==  enum(XilEnv.TraceRecorder@Sleep)", 10000)  # wait till recorder is stopped

    # create a equation file for error counting, if recorded signals are different than the generated signals
    x.CreateFileWithContent ("%TEMP%\\compare_equation.equ",
                             "ErrorCounter = ErrorCounter + (CompareSignal1 != TestSignal1) + (CompareSignal2 != TestSignal2) + (CompareSignal3 != TestSignal3);\n"
                             "SumOfAllSignals = SumOfAllSignals + TestSignal1 + TestSignal2 + TestSignal3;")
    x.StartEquations("%TEMP%\\compare_equation.equ")

    # now we replay the recording and compare it (all 3 formats)
    CompareOneFormat(x, "dat")
    CompareOneFormat(x, "mdf")
    CompareOneFormat(x, "mf4")

    #x.WaitUntil ("0", 10000000)
    # if the "ErrorCounter" is equal to zero no differences exists
    assert x.Equ("ErrorCounter") == 0
    assert x.Equ("equal(SumOfAllSignals, 370373591625.14996, 0.1)") > 0

