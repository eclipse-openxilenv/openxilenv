
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
Functional tests for basic OpenXilEnv API calls on a shared instance.

Tests that start and close their own instance live in test_standalone_instance.py.
"""

import os
import sys
import time
from uuid import uuid4
from pathlib import Path
from openxilenv.xilEnv import XilEnv


def test_disconnect_and_reconnect(owned_xil: tuple[XilEnv, str]) -> None:
    """
    Test disconnect and reconnect.
    
    Args:
        owned_xil (tuple[XilEnv, str]): Owned XilEnv and instance.
    """

    xil, instance = owned_xil
    assert xil.isConnected()
    xil.DisconnectFrom()
    assert not xil.isConnected()
    xil.ConnectToInstance("", instance)
    assert xil.isConnected()


def test_GetAPIVersion(xilEnv_installation_path: Path) -> None:
    """Test getting API version.

    Args:
        xilEnv_installation_path (Path): Installation path.
    """

    xil = XilEnv(xilEnv_installation_path)
    api_version = xil.GetAPIVersion()

    assert isinstance(api_version, int)
    assert api_version >= 0


def test_GetAPIModulePath(xilEnv_installation_path: Path) -> None:
    """Test getting API module path.

    Args:
        xilEnv_installation_path (Path): Installation path.
    """

    xil = XilEnv(xilEnv_installation_path)
    api_module_path = xil.GetAPIModulePath()

    assert isinstance(api_module_path, bytes)
    assert api_module_path
    if sys.platform == "win32":
        assert Path(os.fsdecode(api_module_path)).resolve() == (
            xilEnv_installation_path / "XilEnvRpc.dll"
        ).resolve()


def test_environment_variables(owned_xil: tuple[XilEnv, str]) -> None:
    """Test setting and getting environment variables.

    Args:
        owned_xil (tuple[XilEnv, str]): Owned XilEnv and instance.
    """

    xil, _ = owned_xil

    variable_name = f"BlackboardTest.{uuid4().hex}"
    assert xil.SetEnvironVar(variable_name, "10.0") == 0
    assert xil.GetEnvironVar(variable_name) == "10.0"

