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
conftest.py

Pytest configuration and fixtures for functional tests in the openxilenv project.

This module provides session-scoped fixtures for test configuration, such as paths to required resources.
"""


from tempfile import TemporaryDirectory
import uuid
import pytest
import os
from typing import Generator
from pathlib import Path
from openxilenv.xilEnv import XilEnv
from shutil import copyfile
from test.functional.helper_methods import ensure_process_stopped

OPENXILENV_SOURCE_ENV_VAR = "OPENXILENV_SOURCE_PATH"
OPENXILENV_INSTALLATION_ENV_VAR = "OPENXILENV_INSTALLATION_PATH"


@pytest.fixture(scope="session")
def xilEnv_source_path() -> Path:
    """
    Pytest fixture providing the path to the openXilEnv source directory.

    Returns:
        Path: Path object pointing to the openXilEnv source directory.
    Raises:
        FileNotFoundError: If the directory does not exist.
    """
    if xilEnv_source := os.environ.get(OPENXILENV_SOURCE_ENV_VAR):
        xilEnv_source_path = Path(xilEnv_source)
    else:
        xilEnv_source_path = Path(__file__).parent.parent.parent
    if not xilEnv_source_path.is_dir():
        raise FileNotFoundError(
            f"Unknown openXilEnv source directory {xilEnv_source_path}"
        )
    return xilEnv_source_path


@pytest.fixture(scope="session")
def xilEnv_installation_path(xilEnv_source_path: Path) -> Path:
    """
    Pytest fixture providing the path to the openXilEnv installation directory.

    Returns:
        Path: Path object pointing to the openXilEnv installation directory.
    Raises:
        FileNotFoundError: If the directory does not exist.
    """
    if xilEnv_installation := os.environ.get(OPENXILENV_INSTALLATION_ENV_VAR):
        xilEnv_installation_path = Path(xilEnv_installation)
    else:
        # assume installation folder structure as in README
        xilEnv_installation_path = xilEnv_source_path.parent.parent.joinpath("tools", "openXilEnv")

    if not xilEnv_installation_path.is_dir():
        raise FileNotFoundError(
            f"Unknown openXilEnv installation directory {xilEnv_installation_path}"
        )
    return xilEnv_installation_path


@pytest.fixture(scope="module")
def ini_file_path(xilEnv_source_path: Path) -> Path:
    """
    Pytest fixture providing the path to the ElectricCarSample.ini configuration file.

    Returns:
        Path: Path object pointing to the ElectricCarSample.ini file.
    Raises:
        FileNotFoundError: If the file does not exist.
    """
    ini_file_path = xilEnv_source_path.joinpath("Samples", "Configurations", "ElectricCarSample.ini")
    if not ini_file_path.is_file():
        raise FileNotFoundError(
            f"Missing ini file at expected location {ini_file_path}"
        )
    return ini_file_path


@pytest.fixture(scope="module", autouse=True)
def test_working_directory() -> Generator[None, None, None]:
    """Run each test module in a temporary working directory.

    Yields:
        None: Once the temporary working directory has been set up.

    Restores the original working directory after the test module completes.
    """
    original_directory = Path.cwd()
    with TemporaryDirectory(prefix="openxilenv-test-") as temporary_directory:
        os.chdir(temporary_directory)
        try:
            yield
        finally:
            os.chdir(original_directory)


@pytest.fixture(scope="module")
def owned_xil(
    xilEnv_installation_path: Path,
    ini_file_path: Path,
) -> Generator[tuple[XilEnv, str], None, None]:
    """
    Pytest fixture to provide a connected OpenXilEnv instance for the duration of a test module.

    Args:
        xilEnv_installation_path (Path): Path to the openXilEnv installation directory.
        ini_file_path (Path): Path to the configuration INI file.

    Yields:
        tuple: The connected XilEnv client and the instance name.

    Handles setup and teardown of the OpenXilEnv connection.
    """
    name = f"pytest_{uuid.uuid4().hex}"
    test_ini_path = Path.cwd() / f"Test_{uuid.uuid4().hex}.ini"
    xil = None
    try:
        copyfile(ini_file_path, test_ini_path)
        try:
            xil = XilEnv(xilEnv_installation_path)
        except Exception as e:
            raise RuntimeError("XilEnv initialization failed") from e
        xil.startWithoutGui(test_ini_path, name)
        if not xil.isConnected():
            raise RuntimeError("Failed to connect to OpenXilEnv")
        yield xil, name
    finally:
        try:
            if xil is not None:
                xil.disconnectAndCloseXil()
        finally:
            if xil is not None:
                ensure_process_stopped(getattr(xil, "_XilEnv__xilEnvProcess", None))