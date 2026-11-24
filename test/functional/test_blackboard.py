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
Functional tests for the Blackboard functionality in openxilenv.

This module contains pytest-based tests for verifying Blackboard variable operations
using the OpenXilEnv Python API. Fixtures are provided for test setup and teardown.
"""

from contextlib import contextmanager
from pathlib import Path
from typing import Generator
from uuid import uuid4
from openxilenv.xilEnv import BB_VARI, XilEnv
from test.functional.enums import ErrorCode, DataType, ConversionType
from test.functional.helper_methods import wait_for_variable

import pytest
import time


@contextmanager
def temporary_blackboard_variable(
    owned_xil: tuple[XilEnv, str],
    variable_name: str | None = None,
) -> Generator[tuple[str, int], None, None]:
    """Create a temporary Blackboard variable, yield it and remove it when finished.

    Any "trace" of the created variable (additional attachments) will be cleaned up.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
        variable_name (str | None): Optional variable name.

    Yields:
        tuple[str, int]: The variable name and ID.
    """
    xil, _ = owned_xil
    if variable_name is None:
        variable_name = f"BlackboardTest.{uuid4().hex}"
    variable_id = xil.AddVari(variable_name, DataType.BB_DOUBLE, "test")
    assert variable_id > 0, f"Failed to add {variable_name}"
    try:
        yield variable_name, variable_id
    finally:
        remaining = xil.RemoveVari(variable_id)
        if remaining != ErrorCode.UNKNOWN_VARIABLE:
            while remaining > 0:
                next_remaining = xil.RemoveVari(variable_id)
                assert 0 <= next_remaining < remaining
                remaining = next_remaining
            assert remaining == 0


# Variable tests

def test_write_to_and_read_from_blackboard_variable(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Test writing to and reading from a Blackboard variable.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (variable_name, _):
        xil.attachVariables([variable_name])
        xil.writeSignal(variable_name, 50.0)
        assert xil.readSignal(variable_name) == 50.0


def test_add_attach_and_remove_blackboard_variable(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Test adding, attaching, and removing a Blackboard variable.

    Args:
        owned_xil (tuple[XilEnv, str]  ): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    variable_name = f"BlackboardTest.{uuid4().hex}"
    variable_id = xil.AddVari(variable_name, DataType.BB_DOUBLE, "test")
    assert variable_id > 0
    assert xil.AttachVari(variable_name) == variable_id
    assert xil.RemoveVari(variable_id) == 1
    assert xil.RemoveVari(variable_id) == 0
    assert xil.RemoveVari(variable_id) == ErrorCode.UNKNOWN_VARIABLE


def test_repeated_attach_and_balanced_remove(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Test that repeated attachments require a matching number of removals.

    Attaches the same temporary Blackboard variable twice, checks the
    remaining attachment count after each removal, and verifies that the
    variable remains readable while attached.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (variable_name, variable_id):
        xil.Set(variable_id, -3.25)
        assert xil.AttachVari(variable_name) == variable_id
        assert xil.AttachVari(variable_name) == variable_id
        assert xil.RemoveVari(variable_id) == 2
        assert xil.RemoveVari(variable_id) == 1
        assert xil.Get(variable_id) == -3.25


def test_attach_remove_missing_blackboard_variable(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Test that attaching a nonexistent Blackboard variable returns the right error.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    variable_name = f"BlackboardTest.missing.{uuid4().hex}"
    assert xil.AttachVari(variable_name) == ErrorCode.UNKNOWN_VARIABLE


def test_read_multiple_signals(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Test reading multiple signals from the blackboard.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (first_name, first_id):
        with temporary_blackboard_variable(owned_xil) as (second_name, second_id):
            xil.attachVariables([first_name, second_name])
            xil.writeSignal(first_name, 12.5)
            xil.writeSignal(second_name, -3.25)
            assert xil.readMultipleSignals([first_name, second_name]) == [12.5, -3.25]
            assert xil.readMultipleSignals([second_name, first_name]) == [-3.25, 12.5]
            assert xil.ReadFrame([first_id, second_id]) == [12.5, -3.25]


def test_write_multiple_signals(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Test writing values to multiple Blackboard signals in one operation.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (first_name, first_id):
        with temporary_blackboard_variable(owned_xil) as (second_name, second_id):
            xil.attachVariables([first_name, second_name])
            xil.writeMultipleSignals([first_name, second_name], [12.5, -3.25])
            assert xil.readMultipleSignals([first_name, second_name]) == [12.5, -3.25]
            assert xil.readMultipleSignals([second_name, first_name]) == [-3.25, 12.5]
            assert xil.ReadFrame([first_id, second_id]) == [12.5, -3.25]


def test_write_frame_wait_read_frame(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Test writing and reading a frame.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil
    initial_scheduler_state = xil.SchedulerRunning()

    with temporary_blackboard_variable(owned_xil) as (first_name, first_id):
        with temporary_blackboard_variable(owned_xil) as (second_name, second_id):
            try:
                xil.attachVariables([first_name, second_name])
                xil.StopScheduler()
                assert xil.WriteFrameWaitReadFrame([first_id, second_id], [1.23, 4.56]) == [1.23, 4.56]
            finally:
                if initial_scheduler_state == 1:
                    xil.ContinueScheduler()


def test_getType_getUnit_setUnit(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Test getting and setting unit.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    datatype = DataType.BB_DOUBLE
    unit = "m"
    variable_name = f"BlackboardTest.{uuid4().hex}"
    variable_id = xil.AddVari(variable_name, datatype, unit)

    try:
        assert xil.GetVariType(variable_id) == datatype
        assert xil.GetVariUnit(variable_id) == unit
        xil.SetVariUnit(variable_id, "cm")
        assert xil.GetVariUnit(variable_id) == "cm"
    finally:
        xil.RemoveVari(variable_id)


@pytest.mark.parametrize(
    "minimum, maximum",
    [(-20.0, -10.0), (0.0, 1.0), (10.0, 20.0)]
)
def test_set_min_max(
    owned_xil: tuple[XilEnv, str],
    minimum: float,
    maximum: float,
) -> None:
    """
    Test setting min and max.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
        minimum (float): Minimum value.
        maximum (float): Maximum value.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (_, variable_id):
        assert xil.SetVariMin(variable_id, minimum) == 0
        assert xil.GetVariMin(variable_id) == minimum
        assert xil.SetVariMax(variable_id, maximum) == 0
        assert xil.GetVariMax(variable_id) == maximum


def test_import_vari_properties(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Test importing variable properties.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil
    with temporary_blackboard_variable(owned_xil) as (variable_name, variable_id):
        properties_path = Path("variable_properties.ini")
        properties_path.write_text(
            "[Variables]\n"
            f"{variable_name}=7,km/h,-12.5,98.25,8,2,0,0.5,0,,(-1,-1,-1)\n",
            encoding="ascii",
        )

        assert xil.ImportVariProperties(str(properties_path)) == 0
        assert xil.GetVariUnit(variable_id) == "km/h"
        assert xil.GetVariMin(variable_id) == -12.5
        assert xil.GetVariMax(variable_id) == 98.25


# Pretty useless but doesn´t hurt to test anyway
def test_enable_and_disable_range_control(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Test range control toggling.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil
    variable_filter = f"BlackboardTest.RangeControl.{uuid4().hex}"

    assert xil.EnableRangeControl("NoMatchingProcess", variable_filter) == 0
    assert xil.DisableRangeControl("NoMatchingProcess", variable_filter) == 0


def test_write_access_controls_script_process(
    owned_xil: tuple[XilEnv, str],
) -> None:
    """
    Test that WrVariEnable/WrVariDisable control writes made by the Script process.

    A process gets write access to a variable when it first attaches to it, so access
    can only be changed after the script attached. The script and the test therefore
    synchronize through the variables "ready", "go" and "done".

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil
    process = "Script"
    baseline, blocked_probe, enabled_probe = 10.0, 20.0, 30.0

    def wait_for(variable_id: int, value: float, description: str) -> None:
        wait_for_variable(xil, variable_id, value, description)

    with temporary_blackboard_variable(owned_xil) as (target, target_id), \
            temporary_blackboard_variable(owned_xil) as (ready, ready_id), \
            temporary_blackboard_variable(owned_xil) as (go, go_id), \
            temporary_blackboard_variable(owned_xil) as (done, done_id):
        xil.Set(target_id, baseline)
        for variable_id in (ready_id, go_id, done_id):
            xil.Set(variable_id, 0.0)

        script_path = Path("write_probe.scr")
        try:
            script_path.write_text(
                f"SET_BBVARI({target} = {target})\n"  # attach the target
                f"SET_BBVARI({ready} = 1)\n"
                f"WHILE ({go} < 1)\n"
                "    DELAY (1)\n"
                "ENDWHILE\n"
                f"SET_BBVARI({target} = {blocked_probe})\n"
                f"SET_BBVARI({done} = 1)\n"
                f"WHILE ({go} < 2)\n"
                "    DELAY (1)\n"
                "ENDWHILE\n"
                f"SET_BBVARI({target} = {enabled_probe})\n"
                f"SET_BBVARI({done} = 2)\n"
                "DETACH_ALL_BBVARIS\n",
                encoding="ascii",
            )
            assert xil.StartScript(str(script_path)) == 0
            wait_for(ready_id, 1.0, "script to attach the variables")

            assert xil.WrVariDisable(target, process) == 0
            assert xil.IsWrVariEnabled(target, process) == 0
            xil.Set(go_id, 1.0)
            wait_for(done_id, 1.0, "script's blocked write attempt")
            assert xil.Get(target_id) == baseline  # the script's write was blocked

            assert xil.WrVariEnable(target, process) == 0
            assert xil.IsWrVariEnabled(target, process) == 1
            xil.Set(go_id, 2.0)
            wait_for(done_id, 2.0, "script's enabled write attempt")
            assert xil.Get(target_id) == enabled_probe  # the script's write succeeded
        finally:
            try:
                time.sleep(0.5)  # let the script finish DETACH_ALL_BBVARIS before stopping it
                xil.StopScript()
            finally:
                assert xil.WrVariEnable(target, process) == 0


def test_get_next_vari(owned_xil: tuple[XilEnv, str]) -> None:
    """Verify GetNextVari iterates matching names, exhausts, and restarts.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    prefix = f"BlackboardTest.GetNextVari.{uuid4().hex}"
    first_name = f"{prefix}.first"
    second_name = f"{prefix}.second"

    with temporary_blackboard_variable(owned_xil, first_name):
        with temporary_blackboard_variable(owned_xil, second_name):
            filter_string = f"{prefix}.*"
            first = xil.GetNextVari(1, filter_string)
            second = xil.GetNextVari(0, filter_string)

            assert {first, second} == {first_name, second_name}
            assert xil.GetNextVari(0, filter_string) == ""
            assert xil.GetNextVari(1, filter_string) == first


def test_get_next_vari_ex(owned_xil: tuple[XilEnv, str]) -> None:
    """Verify GetNextVariEx filters iteration by process attachment and access.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil
    process = "Script"
    prefix = f"BlackboardTest.GetNextVariEx.{uuid4().hex}"
    first_name = f"{prefix}.first"
    second_name = f"{prefix}.second"

    with temporary_blackboard_variable(owned_xil, first_name), \
            temporary_blackboard_variable(owned_xil, second_name), \
            temporary_blackboard_variable(owned_xil) as (ready_name, ready_id), \
            temporary_blackboard_variable(owned_xil) as (go_name, go_id):
        filter_string = f"{prefix}.*"
        for variable_id in (ready_id, go_id):
            xil.Set(variable_id, 0.0)

        def wait_for(variable_id: int, value: float, description: str) -> None:
            wait_for_variable(xil, variable_id, value, description)

        script_path = Path("attach_variables.scr")
        script_started = False
        first_access_disabled = False
        try:
            script_path.write_text(
                f"SET_BBVARI({first_name} = {first_name})\n"
                f"SET_BBVARI({second_name} = {second_name})\n"
                f"SET_BBVARI({ready_name} = 1)\n"
                f"WHILE ({go_name} < 1)\n"
                "    DELAY (1)\n"
                "ENDWHILE\n"
                "DETACH_ALL_BBVARIS\n",
                encoding="ascii",
            )
            assert xil.StartScript(str(script_path)) == 0
            script_started = True
            wait_for(ready_id, 1.0, "script to attach the variables")

            first = xil.GetNextVariEx(1, filter_string, process, 3)
            second = xil.GetNextVariEx(0, filter_string, process, 3)
            assert {first, second} == {first_name, second_name}
            assert xil.GetNextVariEx(0, filter_string, process, 3) == ""

            assert xil.WrVariDisable(first_name, process) == 0
            first_access_disabled = True
            assert xil.GetNextVariEx(1, filter_string, process, 1) == second_name
            assert xil.GetNextVariEx(0, filter_string, process, 1) == ""
            assert xil.GetNextVariEx(1, filter_string, process, 2) == first_name
            assert xil.GetNextVariEx(0, filter_string, process, 2) == ""
            assert xil.GetNextVariEx(1, filter_string, process, 3) == first
        finally:
            if script_started:
                try:
                    if first_access_disabled:
                        assert xil.WrVariEnable(first_name, process) == 0
                finally:
                    xil.Set(go_id, 1.0)
                try:
                    deadline = time.monotonic() + 10
                    while xil.GetNextVariEx(1, filter_string, process, 3):
                        assert time.monotonic() < deadline, "Timeout waiting for script to detach variables"
                        time.sleep(0.01)
                finally:
                    assert xil.StopScript() == 0


@pytest.mark.parametrize(
    "raw_value",
    [-3.25, 0.0, 17.0]
)
def test_getRaw_setRaw(owned_xil: tuple[XilEnv, str], raw_value: float) -> None:
    """Test raw get/set.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
        raw_value (float): The raw value to set.
    """
    xil, _ = owned_xil
    value = BB_VARI()
    result = BB_VARI()
    value.d = raw_value

    with temporary_blackboard_variable(owned_xil) as (_, variable_id):
        assert xil.SetRaw(variable_id, DataType.BB_DOUBLE, value, 7) == 0
        assert xil.GetRaw(variable_id, result) == 7
        assert result.d == raw_value


# Conversion tests


@pytest.mark.parametrize(
    "raw_value, physical_value",
    [(-2.5, 0.0), (0.0, 5.0), (4.0, 13.0)]
)
def test_formula_conversion(
    owned_xil: tuple[XilEnv, str],
    raw_value: float,
    physical_value: float,
) -> None:
    """
    Verify formula conversion computes physical = 2 * raw + 5 and its inverse write.
    The test also confirms Get and ReadFrame continue to return the raw value.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
        raw_value (float): The raw value to test.
        physical_value (float): The expected physical value.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (_, variable_id):
        assert xil.XilEnv_SetVariConversion(variable_id, ConversionType.BB_CONV_FORMULA, "2*#+5") == 0
        assert xil.GetVariConversionType(variable_id) == ConversionType.BB_CONV_FORMULA
        assert xil.SetPhys(variable_id, physical_value) == 0
        assert xil.Get(variable_id) == raw_value
        assert xil.GetPhys(variable_id) == physical_value
        assert xil.ReadFrame([variable_id]) == [raw_value]


def test_no_conversion_preserves_raw_value(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Verify BB_CONV_NONE applies no raw-to-physical transform and rejects physical writes.
    
    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (_, variable_id):
        assert xil.XilEnv_SetVariConversion(variable_id, ConversionType.BB_CONV_NONE, "") == 0
        assert xil.GetVariConversionType(variable_id) == ConversionType.BB_CONV_NONE
        xil.Set(variable_id, 8.5)
        xil.SetPhys(variable_id, 10)
        assert xil.Get(variable_id) == 8.5


def test_text_replacement_conversion_string(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Verify BB_CONV_TEXTREP maps exact raw values 0 and 1 to the labels OFF and ON.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil
    conversion = '0 0 "OFF";1 1 "ON"'

    with temporary_blackboard_variable(owned_xil) as (_, variable_id):
        assert xil.XilEnv_SetVariConversion(variable_id, ConversionType.BB_CONV_TEXTREP, conversion) == 0
        assert xil.GetVariConversionType(variable_id) == ConversionType.BB_CONV_TEXTREP
        assert xil.GetVariConversionString(variable_id) == conversion
        assert xil.GetVariEnum(variable_id, 0.0) == "OFF"
        assert xil.GetVariEnum(variable_id, 1.0) == "ON"


@pytest.mark.parametrize(
    "physical_value, raw_value",
    [(10.0, 0.0), (16.0, 3.0), (-6.0, -8.0)]
)
def test_offset_factor_conversion(
    owned_xil: tuple[XilEnv, str],
    physical_value: float,
    raw_value: float,
) -> None:
    """
    Verify BB_CONV_OFFFACT computes physical = (raw + offset) * factor.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
        physical_value (float): The physical value to set.
        raw_value (float): The expected raw value.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (_, variable_id):
        assert xil.XilEnv_SetVariConversion(variable_id, ConversionType.BB_CONV_OFFFACT, "2:5") == 0
        assert xil.GetVariConversionType(variable_id) == ConversionType.BB_CONV_OFFFACT
        assert xil.SetPhys(variable_id, physical_value) == 0
        assert xil.Get(variable_id) == raw_value
        assert xil.GetPhys(variable_id) == physical_value


def test_interpolated_table_conversion(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Verify BB_CONV_TAB_INTP linearly interpolates physical values between raw table points.
    
    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (_, variable_id):
        conversion = "0/0:17/6:30/20"
        assert xil.XilEnv_SetVariConversion(variable_id, ConversionType.BB_CONV_TAB_INTP, conversion) == 0
        assert xil.GetVariConversionType(variable_id) == ConversionType.BB_CONV_TAB_INTP
        assert xil.SetPhys(variable_id, 23.5) == 0
        assert xil.GetPhys(variable_id) == 23.5
        assert xil.Get(variable_id) == 13.0
        assert xil.SetPhys(variable_id, -5.0) == 0
        assert xil.Get(variable_id) == 0.0


def test_non_interpolated_table_conversion(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Verify BB_CONV_TAB_NOINTP selects a table point without interpolating between points.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (_, variable_id):
        conversion = "0/0:17/6:30/20"
        assert xil.XilEnv_SetVariConversion(variable_id, ConversionType.BB_CONV_TAB_NOINTP, conversion) == 0
        assert xil.GetVariConversionType(variable_id) == ConversionType.BB_CONV_TAB_NOINTP
        assert xil.SetPhys(variable_id, 17.0) == 0
        assert xil.GetPhys(variable_id) == 17.0
        assert xil.Get(variable_id) == 6.0
        assert xil.SetPhys(variable_id, 18.0) == 0
        assert xil.GetPhys(variable_id) == 30.0
        assert xil.Get(variable_id) == 20.0


def test_rational_function_conversion(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Verify BB_CONV_RAT_FUNC computes raw = (a · phys² + b · phys + c) / (d · phys² + e · phys + f)

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (_, variable_id):
        assert xil.XilEnv_SetVariConversion(variable_id, ConversionType.BB_CONV_RAT_FUNC, "0:6:0:0:0:17") == 0
        assert xil.GetVariConversionType(variable_id) == ConversionType.BB_CONV_RAT_FUNC
        assert xil.SetPhys(variable_id, 17.0) == 0
        assert xil.GetPhys(variable_id) == 17.0
        assert xil.Get(variable_id) == 6.0


@pytest.mark.parametrize(
    "physical_value, raw_value",
    [(5.0, 0.0), (-15.0, -10.0), (20.0, 7.5)]
)
def test_factor_offset_conversion(
    owned_xil: tuple[XilEnv, str],
    physical_value: float,
    raw_value: float,
) -> None:
    """
    Test that BB_CONV_FACTOFF converts a physical write to raw as
    (physical - offset) / factor, inverse to raw * factor + offset.

    Args:
    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
        physical_value (float): The physical value to set.
        raw_value (float): The expected raw value.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (_, variable_id):
        assert xil.XilEnv_SetVariConversion(variable_id, ConversionType.BB_CONV_FACTOFF, "2:5") == 0
        assert xil.GetVariConversionType(variable_id) == ConversionType.BB_CONV_FACTOFF
        assert xil.SetPhys(variable_id, physical_value) == 0
        assert xil.GetPhys(variable_id) == physical_value
        assert xil.Get(variable_id) == raw_value


@pytest.mark.parametrize(
    "first_value, second_value, sum_result, expression_result",
    [
        (1.0, 2.0, 3.0, 14.2),
        (-2.5, 0.0, -2.5, -0.5),
        (0.0, -3.25, -3.25, -22.75),
    ]
)
def test_Equ(
    owned_xil: tuple[XilEnv, str],
    first_value: float,
    second_value: float,
    sum_result: float,
    expression_result: float,
) -> None:
    """Test Equ function."""
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (first_name, _):
        with temporary_blackboard_variable(owned_xil) as (second_name, _):
            xil.attachVariables([first_name, second_name])
            xil.writeMultipleSignals([first_name, second_name], [first_value, second_value])
            assert xil.Equ(f"{first_name}+{second_name}") == sum_result
            assert xil.Equ(f"{first_name} / 5 + {second_name} * 7") == expression_result


def test_division_by_zero(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Test division by zero.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    with temporary_blackboard_variable(owned_xil) as (_, variable_id):
        assert xil.XilEnv_SetVariConversion(variable_id, ConversionType.BB_CONV_OFFFACT, "0:5") == 0
        assert xil.SetPhys(variable_id, 10.0) == -1, "Offset Factor failed division by zero"
        assert xil.XilEnv_SetVariConversion(variable_id, ConversionType.BB_CONV_FACTOFF, "0:5") == 0
        assert xil.SetPhys(variable_id, 10.0) == -1, "Factor Offset failed division by zero"
        assert xil.XilEnv_SetVariConversion(variable_id, ConversionType.BB_CONV_FORMULA, "#/0") == 0
        assert xil.SetPhys(variable_id, 10.0) == -1, "Formula failed division by zero"
