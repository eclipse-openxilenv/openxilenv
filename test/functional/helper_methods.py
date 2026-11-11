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
from typing import Optional


def ensure_process_stopped(process: Optional[subprocess.Popen], timeout: float = 10.0) -> None:
    """Make sure a process has exited, terminating and then killing it if necessary.

    Args:
        process: Process to stop; None or an already exited process is ignored.
        timeout: Seconds to wait after terminate before killing.
    """
    if process is None or process.poll() is not None:
        return
    process.terminate()
    try:
        process.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()
