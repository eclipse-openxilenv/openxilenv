
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

from pathlib import Path

from openxilenv import XilEnv


def test_StartWithoutGui_and_disconnectAndCloseXil(xilEnv_installation_path: Path, ini_file_path: Path) -> None:
    """
    Test starting OpenXilEnv without GUI and disconnecting.

    Args:
        xilEnv_installation_path (Path): Path to the openXilEnv installation directory.
        ini_file_path (Path): Path to the configuration INI file.
    """
    xil = XilEnv(xilEnv_installation_path)
    xil.startWithoutGui(ini_file_path)
    assert xil.isConnected()
    xil.disconnectAndCloseXil()
    assert not xil.isConnected()