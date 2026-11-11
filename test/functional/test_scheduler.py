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

"""Functional tests for OpenXilEnv scheduler control."""


from openxilenv.xilEnv import XilEnv

import pytest


def test_stop_and_resume_scheduler(owned_xil: tuple[XilEnv, str]) -> None:
    xil, _ = owned_xil

    assert xil.SchedulerRunning() == 1
    try:
        xil.StopScheduler()
        assert xil.SchedulerRunning() == 0
    finally:
        xil.ContinueScheduler()
    assert xil.SchedulerRunning() == 1


def test_advance_exact_number_of_cycles(owned_xil: tuple[XilEnv, str]) -> None:
    xil, _ = owned_xil

    try:
        xil.StopScheduler()
        assert xil.SchedulerRunning() == 0
        before = xil.readSignal("XilEnv.CycleCounter")
        xil.DoNextCyclesAndWait(3)
        after = xil.readSignal("XilEnv.CycleCounter")
        assert after - before == 3
        assert xil.SchedulerRunning() == 0
    finally:
        xil.ContinueScheduler()
