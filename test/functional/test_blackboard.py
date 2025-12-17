
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

import pytest
from pathlib import Path
from typing import Generator

from openxilenv import XilEnv

@pytest.fixture(scope="module")
def xil(xilEnv_installation_path: Path, ini_file_path: Path) -> Generator[XilEnv, None, None]:
    """
    Pytest fixture to provide a connected OpenXilEnv instance for the duration of a test module.

    Args:
        xilEnv_installation_path (Path): Path to the openXilEnv installation directory.
        ini_file_path (Path): Path to the configuration INI file.

    Yields:
        OpenXilEnv: Connected OpenXilEnv instance.

    Handles setup and teardown of the OpenXilEnv connection.
    """
    # setup
    xil = XilEnv(xilEnv_installation_path)
    xil.startWithoutGui(ini_file_path)
    assert xil.isConnected()
    yield xil
    # teardown
    xil.disconnectAndCloseXil()


def test_write_to_and_read_from_blackboard_variable(xil: XilEnv) -> None:
    """
    Test writing to and reading from a Blackboard variable.

    Args:
        xil (XilEnv): Connected OpenXilEnv instance provided by fixture.
    """
    variable_name = "AcceleratorPosition"
    variable_value = 50.0

    # attach the variable
    xil.attachVariables([variable_name])
    # write to the variable
    xil.writeSignal(variable_name, variable_value)
    # read from the variable
    read_value = xil.readSignal(variable_name)
    assert read_value == variable_value, f"Expected {variable_value}, got {read_value}"


def test_add_attach_and_remove_blackboard_variable(xil: XilEnv) -> None:
    """
    Test adding, attaching, and removing a Blackboard variable.

    Args:
        xil (XilEnv): Connected OpenXilEnv instance provided by fixture.
    """
    pytest.skip("not implemented")