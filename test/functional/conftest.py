
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
from pathlib import Path

@pytest.fixture(scope="session")
def xilEnv_installation_path() -> Path:
    """
    Pytest fixture providing the path to the openXilEnv installation directory.

    Returns:
        Path: Path object pointing to the openXilEnv installation directory.
    Raises:
        AssertionError: If the directory does not exist.
    """
    xilEnv_installation_path = Path("C:\\UserData\\dev\\tools\\openXilEnv")
    assert xilEnv_installation_path.is_dir(), f"no such directory {xilEnv_installation_path}"
    return xilEnv_installation_path

@pytest.fixture(scope="session")
def ini_file_path() -> Path:
    """
    Pytest fixture providing the path to the ElectricCarSample.ini configuration file.

    Returns:
        Path: Path object pointing to the ElectricCarSample.ini file.
    Raises:
        AssertionError: If the file does not exist.
    """
    ini_file_path = Path("C:\\UserData\\dev\\src\\openxilenv\\Samples\\Configurations\\ElectricCarSample.ini")
    assert ini_file_path.is_file(), f"no such directory {ini_file_path}"
    return ini_file_path