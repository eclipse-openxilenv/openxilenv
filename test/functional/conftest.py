
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

import pytest
import os
from pathlib import Path

OPENXILENV_SOURCE_ENV_VAR = "OPENXILENV_SOURCE_PATH"
OPENXILENV_INSTALLATION_ENV_VAR = "OPENXILENV_INSTALLATION_PATH"

@pytest.fixture(scope="session")
def xilEnv_source_path() -> Path:
    """
    Pytest fixture providing the path to the openXilEnv source directory.

    Returns:
        Path: Path object pointing to the openXilEnv source directory.
    Raises:
        AssertionError: If the directory does not exist.
    """
    if xilEnv_source := os.environ.get(OPENXILENV_SOURCE_ENV_VAR):
        xilEnv_source_path = Path(xilEnv_source)
    else:
        xilEnv_source_path = Path(__file__).parent.parent.parent
    if not xilEnv_source_path.is_dir():
        pytest.skip(f"Unknown openXilEnv source directory {xilEnv_source_path}")
    return xilEnv_source_path

@pytest.fixture(scope="session")
def xilEnv_installation_path(xilEnv_source_path: Path) -> Path:
    """
    Pytest fixture providing the path to the openXilEnv installation directory.

    Returns:
        Path: Path object pointing to the openXilEnv installation directory.
    Raises:
        AssertionError: If the directory does not exist.
    """
    if xilEnv_installation := os.environ.get(OPENXILENV_INSTALLATION_ENV_VAR):
        xilEnv_installation_path = Path(xilEnv_installation)
    else:
        # assume installation folder structure as in README
        xilEnv_installation_path = xilEnv_source_path.parent.parent.joinpath("tools", "openXilEnv")

    if not xilEnv_installation_path.is_dir():
        pytest.skip(f"Unknown openXilEnv installation directory {xilEnv_installation_path}")
    return xilEnv_installation_path

@pytest.fixture(scope="session")
def ini_file_path(xilEnv_source_path: Path) -> Path:
    """
    Pytest fixture providing the path to the ElectricCarSample.ini configuration file.

    Returns:
        Path: Path object pointing to the ElectricCarSample.ini file.
    Raises:
        AssertionError: If the file does not exist.
    """
    ini_file_path = xilEnv_source_path.joinpath("Samples", "Configurations", "ElectricCarSample.ini")
    if not ini_file_path.is_file():
        pytest.skip(f"Missing ini file at expected location {ini_file_path}")
    return ini_file_path