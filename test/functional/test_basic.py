
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
Functional test for basic OpenXilEnv startup and shutdown.

This module contains a pytest-based test for verifying that OpenXilEnv can be started
without a GUI and properly disconnected, using the Python API.
"""

import uuid
import pytest
from pathlib import Path
from openxilenv.xilEnv import XilEnv
from shutil import copyfile
from test.functional.helper_methods import ensure_process_stopped


def test_StartWithoutGui_and_disconnectAndCloseXil(
    xilEnv_installation_path: Path,
    ini_file_path: Path,
) -> None:
    """
    Test starting OpenXilEnv without GUI and disconnecting.

    Args:
        xilEnv_installation_path (Path): Path to the openXilEnv installation directory.
        ini_file_path (Path): Path to the configuration INI file.
    """
    name = f"pytest_{uuid.uuid4().hex}"
    test_ini_path = Path.cwd() / f"BlackboardTest_{uuid.uuid4().hex}.ini"
    xil = None
    try:
        copyfile(ini_file_path, test_ini_path)
        try:
            xil = XilEnv(xilEnv_installation_path)
        except Exception as e:
            raise RuntimeError("XilEnv initialization failed") from e
        xil.startWithoutGui(test_ini_path, name)
        assert xil.isConnected()
        xil.disconnectAndCloseXil()
        assert not xil.isConnected()
    finally:
        ensure_process_stopped(getattr(xil, "_XilEnv__xilEnvProcess", None))


@pytest.mark.parametrize(
    "set_exit_code_flag, requested_exit_code, expected_process_exit_code",
    [
        (1, 42, 42),
        (0, 7, 0),
    ],
)
def test_DisconnectAndClose_sets_process_exit_code(
    xilEnv_installation_path: Path,
    ini_file_path: Path,
    set_exit_code_flag: int,
    requested_exit_code: int,
    expected_process_exit_code: int,
) -> None:
    """DisconnectAndClose applies the exit code only when the flag is set."""
    name = f"pytest_{uuid.uuid4().hex}"
    test_ini_path = Path.cwd() / f"BlackboardTest_{uuid.uuid4().hex}.ini"
    xil = None
    process = None
    try:
        copyfile(ini_file_path, test_ini_path)
        try:
            xil = XilEnv(xilEnv_installation_path)
        except Exception as e:
            raise RuntimeError("XilEnv initialization failed") from e
        xil.startWithoutGui(test_ini_path, name)
        process = getattr(xil, "_XilEnv__xilEnvProcess", None)
        assert process is not None
        assert xil.isConnected()
        assert xil.DisconnectAndClose(set_exit_code_flag, requested_exit_code) == 0
        assert process.wait(timeout=30) == expected_process_exit_code
    finally:
        ensure_process_stopped(process)


def test_StartWithGui(
    xilEnv_installation_path: Path,
    ini_file_path: Path,
) -> None:
    """
    Test starting OpenXilEnv with GUI.

    Args:
        xilEnv_installation_path (Path): Path to the openXilEnv installation directory.
        ini_file_path (Path): Path to the configuration INI file.
    """
    name = f"pytest_{uuid.uuid4().hex}"
    test_ini_path = Path.cwd() / f"BlackboardTest_{uuid.uuid4().hex}.ini"
    xil = None
    try:
        copyfile(ini_file_path, test_ini_path)
        try:
            xil = XilEnv(xilEnv_installation_path)
        except Exception as e:
            raise RuntimeError("XilEnv initialization failed") from e
        xil.startWithGui(test_ini_path, name)
        assert xil.isConnected()
        xil.disconnectAndCloseXil()
        assert not xil.isConnected()
    finally:
        ensure_process_stopped(getattr(xil, "_XilEnv__xilEnvProcess", None))