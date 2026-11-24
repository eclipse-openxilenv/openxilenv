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
Functional tests that start and close their own OpenXilEnv instance.

Do not use module-scoped instance fixtures (e.g. owned_xil) in this module: the RPC
library holds one process-wide connection, so a fixture-owned instance would still be
connected and make these tests fail when the module is run as a whole.
"""

import re
import subprocess
from uuid import uuid4
from dataclasses import dataclass
from pathlib import Path
from shutil import copyfile
from typing import Callable, Generator

import pytest

from openxilenv.xilEnv import XilEnv
from test.functional.helper_methods import ensure_process_stopped


@dataclass
class StartedInstance:
    """A started OpenXilEnv instance together with its process and ini file."""

    xil: XilEnv
    process: subprocess.Popen | None
    ini_path: Path


@pytest.fixture
def start_instance(
    xilEnv_installation_path: Path,
    ini_file_path: Path,
) -> Generator[Callable[..., StartedInstance], None, None]:
    """Provide a factory that starts an instance; teardown closes whatever is still running.

    Args:
        xilEnv_installation_path (Path): Installation path.
        ini_file_path (Path): INI file path.

    Yields:
        Callable[..., StartedInstance]: Factory taking an optional `gui` flag.
    """

    started: list[StartedInstance] = []

    def start(gui: bool = False) -> StartedInstance:
        ini_path = Path.cwd() / f"StandaloneTest_{uuid4().hex}.ini"
        copyfile(ini_file_path, ini_path)
        try:
            xil = XilEnv(xilEnv_installation_path)
        except Exception as e:
            raise RuntimeError("XilEnv initialization failed") from e
        name = f"pytest_{uuid4().hex}"
        if gui:
            xil.startWithGui(ini_path, Instance=name)
        else:
            xil.startWithoutGui(ini_path, Instance=name)
        process = getattr(xil, "_XilEnv__xilEnvProcess", None)
        instance = StartedInstance(xil, process, ini_path)
        started.append(instance)
        return instance

    yield start

    for instance in started:
        try:
            if instance.process is not None and instance.process.poll() is None:
                instance.xil.disconnectAndCloseXil()
        finally:
            ensure_process_stopped(instance.process)


def test_StartWithoutGui_and_disconnectAndCloseXil(start_instance: Callable[..., StartedInstance]) -> None:
    """Test starting OpenXilEnv without GUI and disconnecting."""

    instance = start_instance()
    xil = instance.xil
    assert instance.process is not None
    assert xil.isConnected()
    xil.disconnectAndCloseXil()
    assert not xil.isConnected()


@pytest.mark.skip("GUI currently not working on the virtual machine")
def test_StartWithGui(start_instance: Callable[..., StartedInstance]) -> None:
    """Test starting OpenXilEnv with GUI."""

    instance = start_instance(gui=True)
    xil = instance.xil
    assert instance.process is not None
    assert xil.isConnected()
    xil.disconnectAndCloseXil()
    assert not xil.isConnected()


@pytest.mark.parametrize(
    "set_exit_code_flag, requested_exit_code, expected_process_exit_code",
    [
        (1, 42, 42),
        (0, 7, 0),
    ],
)
def test_DisconnectAndClose_sets_process_exit_code(
    start_instance: Callable[..., StartedInstance],
    set_exit_code_flag: int,
    requested_exit_code: int,
    expected_process_exit_code: int,
) -> None:
    """DisconnectAndClose applies the exit code only when the flag is set.

    Args:
        start_instance (Callable[..., StartedInstance]): Instance factory.
        set_exit_code_flag (int): Flag for exit code.
        requested_exit_code (int): Requested exit code.
        expected_process_exit_code (int): Expected exit code.
    """

    instance = start_instance()
    assert instance.process is not None
    assert instance.xil.isConnected()
    assert instance.xil.DisconnectAndClose(set_exit_code_flag, requested_exit_code) == 0
    assert instance.process.wait(timeout=30) == expected_process_exit_code


def test_TextOut_writes_message_to_script_output(start_instance: Callable[..., StartedInstance]) -> None:
    """TextOut adds the submitted message to the XilEnv script output."""

    message = f"TextOut functional test {uuid4().hex}"
    instance = start_instance()
    xil = instance.xil
    assert instance.process is not None
    assert xil.isConnected()
    assert xil.TextOut(message) == 0
    xil.disconnectAndCloseXil()
    assert message in (Path.cwd() / "script.msg").read_text(errors="replace")


def test_ChangeSettings_persists_realtime_setting_to_ini(start_instance: Callable[..., StartedInstance]) -> None:
    """ChangeSettings alters NOT_FASTER_THAN_REALTIME, which is saved to the ini on close."""

    setting_line = re.compile(r"^\s*NotFasterThanRealtime\s*=\s*(\S+)\s*$", re.MULTILINE)
    instance = start_instance()
    initial = setting_line.search(instance.ini_path.read_text(errors="replace"))
    assert initial is not None and initial.group(1) == "1"
    assert instance.process is not None
    assert instance.xil.isConnected()
    assert instance.xil.ChangeSettings("NOT_FASTER_THAN_REALTIME", "No") == 0
    assert instance.xil.ChangeSettings("NOT_FASTER_THAN_REALTIME", "invalid") != 0
    instance.xil.disconnectAndCloseXil()
    instance.process.wait(timeout=30)
    saved = setting_line.search(instance.ini_path.read_text(errors="replace"))
    assert saved is not None
    assert saved.group(1) == "0"
