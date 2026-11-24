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

"""Helpers for stopping XilEnv processes before temporary directories are removed."""

import subprocess
import time
from typing import Optional


def ensure_process_stopped(process: Optional[subprocess.Popen], timeout: float = 10.0) -> None:
    """Make sure a process has exited, terminating and then killing it if necessary.

    Args:
        process: Process to stop; None or an already exited process is ignored.
        timeout: Seconds to wait after terminate before killing.
    """
    if process is None or process.poll() is not None:
        return
    try:
        process.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        process.terminate()
        try:
            process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def wait_for_variable(xil, variable_id: int, value: float, description: str,
                      timeout: float = 10.0) -> None:
    """Poll a blackboard variable until it has the expected value.

    Args:
        xil: The XilEnv client.
        variable_id: Id of the variable to poll.
        value: Value to wait for.
        description: What is being waited for, used in the timeout message.
        timeout: Seconds to wait before failing.
    """
    deadline = time.monotonic() + timeout
    while xil.Get(variable_id) != value:
        assert time.monotonic() < deadline, f"Timeout waiting for {description}"
        time.sleep(0.01)
