# Copyright (c) 2024 Contributors to the Eclipse Foundation
#
# This program and the accompanying materials are made available under the
# terms of the Apache License, Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0.
#
# This program is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

"""Functional tests for external process control."""

import sys
import time
from contextlib import contextmanager
from pathlib import Path
from typing import Callable, Generator
from uuid import uuid4

from openxilenv.xilEnv import XilEnv
from test.functional.enums import DataType, ErrorCode


@contextmanager
def _running_simple_process(
    xil: XilEnv,
    xilEnv_installation_path: Path,
) -> Generator[tuple[str, Path], None, None]:
    """
    Start ExtProc_Simple for a test and stop only the process it started.

    Args:
        xil (XilEnv): The XilEnv client.
        xilEnv_installation_path (Path): Path to the XilEnv installation.
    """

    sample_folder = "Samples64" if sys.maxsize > 2**32 else "Samples32"
    executable = (
        xilEnv_installation_path
        / sample_folder
        / "ExtProc_Simple"
        / ("ExtProc_Simple.exe" if sys.platform == "win32" else "ExtProc_Simple.EXE")
    )
    assert executable.is_file(), f"ExtProc_Simple executable not found: {executable}"

    process_name = str(executable)
    assert xil.GetProcessState(process_name) == ErrorCode.UNKNOWN_PROCESS, (
        "ExtProc_Simple is already running before the test"
    )

    started = False
    try:
        start_result = xil.StartProcess(process_name)
        assert start_result == 0, f"StartProcess returned {start_result}, expected 0"
        started = True

        deadline = time.monotonic() + 10
        process_state = xil.GetProcessState(process_name)
        while (
            process_state not in (0, 1, 2)
            and time.monotonic() < deadline
        ):
            time.sleep(0.05)
            process_state = xil.GetProcessState(process_name)

        assert process_state in (0, 1, 2), (
            f"ExtProc_Simple did not log in; process state was {process_state}"
        )
        yield process_name, executable
    finally:
        if started:
            stop_result = xil.StopProcess(process_name)
            assert stop_result in (0, ErrorCode.UNKNOWN_PROCESS), (
                f"StopProcess failed with return value {stop_result}"
            )

            deadline = time.monotonic() + 10
            process_state = xil.GetProcessState(process_name)
            while (
                process_state != ErrorCode.UNKNOWN_PROCESS
                and time.monotonic() < deadline
            ):
                time.sleep(0.05)
                process_state = xil.GetProcessState(process_name)

            assert process_state == ErrorCode.UNKNOWN_PROCESS, (
                f"ExtProc_Simple did not stop; process state was {process_state}"
            )


def _verify_process_equation_add_and_delete(
    xil: XilEnv,
    tmp_path: Path,
    xilEnv_installation_path: Path,
    add_equation: Callable[[int, str, str], int],
    delete_equations: Callable[[int, str], None],
    side: str,
) -> None:
    """
    Verify observable equation execution, selective deletion, and delete-all.

    Args:
        xil (XilEnv): The XilEnv client.
        tmp_path (Path): Temporary path for equations.
        xilEnv_installation_path (Path): Path to XilEnv installation.
        add_equation (Callable[[int, str, str], int]): Function to add an equation.
        delete_equations (Callable[[int, str], None]): Function to delete equations.
        side (str): Side identifier.
    """
    test_id = uuid4().hex
    first_variable = f"ProcessEquationTest.{test_id}.First"
    second_variable = f"ProcessEquationTest.{test_id}.Second"
    first_equation = tmp_path / f"{side.lower()}_first_{test_id}.equ"
    second_equation = tmp_path / f"{side.lower()}_second_{test_id}.equ"
    first_number = 31
    second_number = 47
    variable_ids: list[int] = []
    initial_scheduler_state = xil.SchedulerRunning()
    assert initial_scheduler_state in (0, 1), (
        f"Unexpected scheduler state {initial_scheduler_state}"
    )

    try:
        with _running_simple_process(xil, xilEnv_installation_path) as (process_name, _):
            try:
                first_id = xil.AddVari(first_variable, DataType.BB_DOUBLE, "")
                assert first_id > 0, f"Could not create {first_variable}"
                variable_ids.append(first_id)
                second_id = xil.AddVari(second_variable, DataType.BB_DOUBLE, "")
                assert second_id > 0, f"Could not create {second_variable}"
                variable_ids.append(second_id)

                first_equation.write_text(
                    f"{first_variable} = 113;\n", encoding="utf-8"
                )
                second_equation.write_text(
                    f"{second_variable} = 271;\n", encoding="utf-8"
                )

                if initial_scheduler_state == 1:
                    xil.StopScheduler()
                assert xil.SchedulerRunning() == 0

                xil.Set(first_id, 0)
                xil.Set(second_id, 0)
                assert add_equation(first_number, process_name, str(first_equation)) == 0
                assert add_equation(second_number, process_name, str(second_equation)) == 0

                xil.DoNextCyclesAndWait(1)
                assert xil.Get(first_id) == 113
                assert xil.Get(second_id) == 271

                delete_equations(first_number, process_name)
                xil.Set(first_id, 0)
                xil.Set(second_id, 0)
                xil.DoNextCyclesAndWait(1)
                assert xil.Get(first_id) == 0
                assert xil.Get(second_id) == 271

                delete_equations(-1, process_name)
                xil.Set(second_id, 0)
                xil.DoNextCyclesAndWait(1)
                assert xil.Get(first_id) == 0
                assert xil.Get(second_id) == 0
            finally:
                try:
                    delete_equations(-1, process_name)
                finally:
                    try:
                        if (
                            initial_scheduler_state == 1
                            and xil.SchedulerRunning() == 0
                        ):
                            xil.ContinueScheduler()
                    finally:
                        for variable_id in variable_ids:
                            remaining = xil.RemoveVari(variable_id)
                            while remaining > 0:
                                next_remaining = xil.RemoveVari(variable_id)
                                assert 0 <= next_remaining < remaining
                                remaining = next_remaining
    finally:
        for equation_file in (first_equation, second_equation):
            if equation_file.exists():
                equation_file.unlink()


def test_start_stop_ex_process(
    owned_xil: tuple[XilEnv, str],
    xilEnv_installation_path: Path,
) -> None:
    """
    Test starting and stopping an external process.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
        xilEnv_installation_path (Path): Path to the XilEnv installation.
    """
    xil, _ = owned_xil

    sample_folder = "Samples64" if sys.maxsize > 2**32 else "Samples32"
    executable = (
        xilEnv_installation_path
        / sample_folder
        / "ExtProc_Simple"
        / ("ExtProc_Simple.exe" if sys.platform == "win32" else "ExtProc_Simple.EXE")
    )
    assert executable.is_file(), f"ExtProc_Simple executable not found: {executable}"

    process_name = str(executable)
    try:
        assert xil.StartProcessEx(
            process_name,
            Prio=1,
            Cycle=1,
            Delay=0,
            Timeout=10000,
            SVLFile="",
            BBPrefix="",
            UseRangeControl=0,
            RangeControlBeforeActiveFlags=0,
            RangeControlBehindActiveFlags=0,
            RangeControlStopSchedFlag=0,
            RangeControlOutput=0,
            RangeErrorCounterFlag=0,
            RangeErrorCounter="",
            RangeControlVarFlag=0,
            RangeControl="",
            RangeControlPhysFlag=0,
            RangeControlLimitValues=0,
        ) == 0

        deadline = time.monotonic() + 10
        process_state = xil.GetProcessState(process_name)
        while (
            process_state not in (0, 1, 2)
            and time.monotonic() < deadline
        ):
            time.sleep(0.05)
            process_state = xil.GetProcessState(process_name)

        assert process_state in (0, 1, 2), (
            f"ExtProc_Simple did not log in; process state was {process_state}"
        )
    finally:
        stop_result = xil.StopProcess(process_name)
        assert stop_result in (0, ErrorCode.UNKNOWN_PROCESS), (
            f"StopProcess failed with return value {stop_result}"
        )

        deadline = time.monotonic() + 10
        process_state = xil.GetProcessState(process_name)
        while (
            process_state != ErrorCode.UNKNOWN_PROCESS
            and time.monotonic() < deadline
        ):
            time.sleep(0.05)
            process_state = xil.GetProcessState(process_name)

        assert process_state == ErrorCode.UNKNOWN_PROCESS, (
            f"ExtProc_Simple did not stop; process state was {process_state}"
        )


def test_start_stop_and_list_process(
    owned_xil: tuple[XilEnv, str],
    xilEnv_installation_path: Path,
) -> None:
    """
    Test starting, stopping and listing a process with StartProcess.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
        xilEnv_installation_path (Path): Path to the XilEnv installation.
    """
    xil, _ = owned_xil

    with _running_simple_process(xil, xilEnv_installation_path) as (_, executable):
        process_names = []
        listed_process = xil.GetNextProcess(1, "*")
        while listed_process:
            assert isinstance(listed_process, str)
            process_names.append(listed_process)
            listed_process = xil.GetNextProcess(0, "*")

        assert any(
            Path(name).name.casefold() == executable.name.casefold()
            for name in process_names
        ), f"ExtProc_Simple was not returned by GetNextProcess: {process_names}"


def test_duplicate_start_process_returns_running_error_and_lists_process(
    owned_xil: tuple[XilEnv, str],
    xilEnv_installation_path: Path,
) -> None:
    """
    Verify a running external process cannot be started twice and is listed.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
        xilEnv_installation_path (Path): Path to the XilEnv installation.
    """
    xil, _ = owned_xil

    with _running_simple_process(xil, xilEnv_installation_path) as (running_name, _):
        assert xil.StartProcess(running_name) == ErrorCode.PROCESS_RUNNING


def test_method_returns_unknown_for_missing_process(
    owned_xil: tuple[XilEnv, str],
) -> None:
    """
    Verify StopProcess reports an unknown process as absent.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
    """
    xil, _ = owned_xil

    process_name = "__pytest_process_not_started_for_stop__"
    assert xil.GetProcessState(process_name) == ErrorCode.UNKNOWN_PROCESS
    assert xil.StopProcess(process_name) == ErrorCode.UNKNOWN_PROCESS


def test_add_and_delete_before_process_equations(
    owned_xil: tuple[XilEnv, str],
    xilEnv_installation_path: Path,
    tmp_path: Path,
) -> None:
    """
    Verify before-process equations execute and can be selectively or fully deleted.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
        xilEnv_installation_path (Path): Path to the XilEnv installation.
        tmp_path (Path): Temporary path for test files.
    """
    xil, _ = owned_xil

    _verify_process_equation_add_and_delete(
        xil,
        tmp_path,
        xilEnv_installation_path,
        xil.AddBeforeProcessEquationFromFile,
        xil.DelBeforeProcessEquations,
        "before",
    )


def test_add_and_delete_behind_process_equations(
    owned_xil: tuple[XilEnv, str],
    xilEnv_installation_path: Path,
    tmp_path: Path,
) -> None:
    """
    Verify behind-process equations execute and can be selectively or fully deleted.

    Args:
        owned_xil (tuple[XilEnv, str]): The XilEnv client and instance name.
        xilEnv_installation_path (Path): Path to the XilEnv installation.
        tmp_path (Path): Temporary path for test files.
    """
    xil, _ = owned_xil

    _verify_process_equation_add_and_delete(
        xil,
        tmp_path,
        xilEnv_installation_path,
        xil.AddBehindProcessEquationFromFile,
        xil.DelBehindProcessEquations,
        "behind",
    )
