
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
Functional tests for the recorder and stimulus player in openxilenv.

This module contains pytest-based tests for verifying the recording and replaying of signals
in different file formats (dat, mdf, mf4) using the OpenXilEnv Python API.
"""

from openxilenv.xilEnv import XilEnv

import pytest


def compare_one_format(xil: XilEnv, Format: str) -> None:
    """
    Replay the recording in one selected format and compare it.

    Args:
        xil (XilEnv): The XilEnv RPC interface.
        Format (str): The file format to use (e.g., 'dat', 'mdf', 'mf4').
    """
    xil.SetEnvironVar("RECORDER_FILE_EXTENSION", Format)
    # replay the recording in one selected format and compare it
    # create a stimulus configuration file
    xil.CreateFileWithContent("stimulus.cfg",
                             "HDPLAYER_CONFIG_FILE\n"
                             "HDPLAYER_FILE recorder.%RECORDER_FILE_EXTENSION%\n"
                             "TRIGGER StimulusTriggerEvent > 1.0\n"
                             "STARTVARLIST\n"
                             "  VARIABLE TestSignal1\n"
                             "  VARIABLE TestSignal2\n"
                             "  VARIABLE TestSignal3\n"
                             "ENDVARLIST\n")
    # create a second generator configuration file with other signal names
    xil.CreateFileWithContent("compare_ramps.gen",
                             "RAMPE StimulusTriggerEvent 2/0 0/0\n"
                             "RAMPE CompareSignal1 0/0 1000/10 0/20\n"
                             "RAMPE CompareSignal2 0/0 0/10 100/10 50/20 0/20\n"
                             "RAMPE CompareSignal3 0/0 123456789/10 0/20\n")
    # create an equation file for error counting if recorder signals are different than the generated signals
    xil.CreateFileWithContent("compare_equation.equ",
                             "ErrorCounter = ErrorCounter + (CompareSignal1 != TestSignal1) + (CompareSignal2 != TestSignal2) + (CompareSignal3 != TestSignal3);\n")
    xil.StartPlayer("stimulus.cfg")
    # wait till the stimulus player is in trigger state
    xil.WaitUntil("XilEnv.StimulusPlayer == enum(XilEnv.StimulusPlayer@Trigger)", 100)
    # start the generator, the stimulus player will be triggered by the generator, so they will be started simultaneously
    xil.StartGenerator("compare_ramps.gen")
    xil.WaitUntil("XilEnv.StimulusPlayer == enum(XilEnv.StimulusPlayer@Play)", 10000)  # wait till stimulus player is running
    xil.WaitUntil("XilEnv.StimulusPlayer == enum(XilEnv.StimulusPlayer@Sleep)", 10000)  # wait till stimulus player is finished


def test_record_and_run_stimuli_with_dat_mdf_mf4(owned_xil: tuple[XilEnv, str],) -> None:
    """
    Test recording and replaying signals in dat, mdf, and mf4 formats.

    This test records generated signals, replays them in all supported formats, and compares the results.
    It asserts that no errors are found and the sum of all signals matches the expected value.

    Args:
        owned_xil (tuple[XilEnv, str]): Connected OpenXilEnv instance provided by fixture.
    """
    xil, _ = owned_xil
    xil.ChangeSettings("NOT_FASTER_THAN_REALTIME", "No")
    xil.AddVari("ErrorCounter", 5, "")
    sum_of_all_signals_id = xil.AddVari("SumOfAllSignals", 7, "")
    xil.AddVari("RecorderTriggerEvent", 5, "")
    xil.AddVari("StimulusTriggerEvent", 5, "")
    # add 3 test and compare signals
    xil.AddVari("TestSignal1", 7, "")
    xil.AddVari("CompareSignal1", 7, "")
    xil.AddVari("TestSignal2", 7, "")
    xil.AddVari("CompareSignal2", 7, "")
    xil.AddVari("TestSignal3", 5, "")
    xil.AddVari("CompareSignal3", 5, "")

    # first we will do a recording of our own generated signals
    # create a recorder configuration file (recording simultaneously into all 3 supported file formats)
    xil.CreateFileWithContent("recorder.cfg",
                             "HDRECORDER_CONFIG_FILE\n"
                             "HDRECORDER_FILE recorder.mf4\n"
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
    xil.StartRecorder("recorder.cfg")
    recorder_trigger = "XilEnv.TraceRecorder == enum(XilEnv.TraceRecorder@Trigger)"
    xil.WaitUntil(recorder_trigger, 100)
    assert xil.Equ(recorder_trigger) == 1
    # create a generator configuration file
    xil.CreateFileWithContent("ramps.gen",
                             "RAMPE RecorderTriggerEvent 0/0 2/0\n"
                             "RAMPE TestSignal1 0/0 1000/10 0/20\n"
                             "RAMPE TestSignal2 0/0 0/10 100/10 50/20 0/20\n"
                             "RAMPE TestSignal3 0/0 123456789/10 0/20\n")
    xil.StartGenerator("ramps.gen")
    # wait till the generator is finished
    xil.WaitUntil("XilEnv.Generator == enum(XilEnv.Generator@Play)", 10000)  # wait till generator is running
    xil.WaitUntil("XilEnv.Generator == enum(XilEnv.Generator@Sleep)", 10000)  # wait till generator is finished
    # do some additional cycles
    xil.WaitUntil("0", 10)
    xil.StopRecorder()
    xil.WaitUntil("XilEnv.TraceRecorder ==  enum(XilEnv.TraceRecorder@Sleep)", 10000)  # wait till recorder is stopped

    # create an equation file for error counting, if recorded signals are different than the generated signals
    xil.CreateFileWithContent("compare_equation.equ",
                             "ErrorCounter = ErrorCounter + (CompareSignal1 != TestSignal1) + (CompareSignal2 != TestSignal2) + (CompareSignal3 != TestSignal3);\n"
                             "SumOfAllSignals = SumOfAllSignals + TestSignal1 + TestSignal2 + TestSignal3;")
    xil.StartEquations("compare_equation.equ")
    equations_active = "XilEnv.EquationCalculator == enum(XilEnv.EquationCalculator@Active)"
    xil.WaitUntil(equations_active, 1000)
    assert xil.Equ(equations_active) == 1

    # now we replay the recording and compare it (all 3 formats)
    compare_one_format(xil, "dat")
    compare_one_format(xil, "mdf")
    compare_one_format(xil, "mf4")

    # if the "ErrorCounter" is equal to zero no differences exist
    assert xil.Equ("ErrorCounter") == 0
    sum_of_all_signals = xil.Get(sum_of_all_signals_id)
    assert xil.Equ("equal(SumOfAllSignals, 370373591625.14996, 0.1)") > 0, sum_of_all_signals
