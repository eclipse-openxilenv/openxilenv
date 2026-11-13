# Copyright (c) 2024 Contributors to the Eclipse Foundation
#
# This program and the accompanying materials are made available under the
# terms of the Apache License, Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0.
#
# SPDX-License-Identifier: Apache-2.0

import sys
import unittest
from pathlib import Path
from unittest.mock import Mock, call, patch

sample_path = str(Path(__file__).resolve().parents[2] / "Samples/Automation/Python/OpenXilEnv")
sys.path.insert(0, sample_path)
try:
    from OpenXilEnv.openXilEnv import OpenXilEnv
finally:
    sys.path.remove(sample_path)


class TestSampleOpenXilEnv(unittest.TestCase):
    def setUp(self):
        self.rpc = Mock()
        self.rpc.AttachVari.return_value = 1
        self.rpc.DisconnectAndClose.return_value = 0
        self.process = Mock()
        self.now = 0
        self.rpc_patcher = patch("OpenXilEnv.openXilEnv.XilEnvRpc", return_value=self.rpc)
        self.process_patcher = patch("OpenXilEnv.openXilEnv.subprocess.Popen", return_value=self.process)
        self.clock_patcher = patch("OpenXilEnv.openXilEnv.time.monotonic", side_effect=lambda: self.now)
        self.sleep_patcher = patch("OpenXilEnv.openXilEnv.time.sleep", side_effect=self.advance)
        for patcher in (self.rpc_patcher, self.process_patcher, self.clock_patcher, self.sleep_patcher):
            patcher.start()
            self.addCleanup(patcher.stop)

    def advance(self, seconds):
        self.now += seconds

    def test_retries_connection_and_waits_for_rpc_control_before_attaching(self):
        self.rpc.ConnectTo.side_effect = [-1, 0]
        self.rpc.GetProcessState.side_effect = self.check_not_attached_until_ready
        self.states = iter([4, 0])
        xil = OpenXilEnv("installation")
        xil.startWithoutGui("config.ini")
        self.assertEqual(self.rpc.ConnectTo.call_count, 2)
        self.assertEqual(self.rpc.GetProcessState.call_args_list, [call("RPC_Control")] * 2)
        self.assertTrue(self.rpc.AttachVari.called)
        self.assertAlmostEqual(self.now, 1.1)

    def check_not_attached_until_ready(self, name):
        self.assertEqual(name, "RPC_Control")
        self.assertFalse(self.rpc.AttachVari.called)
        return next(self.states)

    def test_timeout_and_zero_timeout_attempt_connection(self):
        self.rpc.ConnectTo.return_value = -1
        xil = OpenXilEnv("installation", timeout=2)
        with self.assertRaises(SystemExit):
            xil.startWithGui("config.ini")
        self.assertEqual(self.rpc.ConnectTo.call_count, 3)
        self.assertEqual(self.now, 2)
        self.rpc.ConnectTo.reset_mock()
        with self.assertRaises(SystemExit):
            xil.startWithGui("config.ini", timeoutInSec=0)
        self.rpc.ConnectTo.assert_called_once_with("")

    def test_readiness_timeout_closes_connection_without_attaching(self):
        self.rpc.ConnectTo.return_value = 0
        self.rpc.GetProcessState.return_value = 4
        xil = OpenXilEnv("installation", timeout=0.2)
        with self.assertRaises(SystemExit):
            xil.startWithoutGui("config.ini")
        self.rpc.AttachVari.assert_not_called()
        self.rpc.DisconnectAndClose.assert_called_once_with(0, 0)
        self.process.wait.assert_called_once_with(timeout=5)

    def test_zero_timeout_accepts_immediate_readiness(self):
        self.rpc.ConnectTo.return_value = 0
        self.rpc.GetProcessState.return_value = 0
        xil = OpenXilEnv("installation")
        xil.startWithoutGui("config.ini", timeoutInSec=0)
        self.rpc.ConnectTo.assert_called_once_with("")
        self.assertTrue(self.rpc.AttachVari.called)
        self.assertEqual(self.now, 0)

    def test_restart_clears_old_ids_and_waits_for_shutdown(self):
        self.rpc.ConnectTo.return_value = 0
        self.rpc.GetProcessState.return_value = 0
        xil = OpenXilEnv("installation")
        xil.startWithoutGui("config.ini")
        self.rpc.AttachVari.return_value = 42
        xil.attachVariables(["old"])
        xil.disconnectAndCloseXil()
        self.process.wait.assert_called_once_with(timeout=5)
        xil.startWithoutGui("config.ini")
        with self.assertRaises(KeyError):
            xil.writeSignal("old", 1)
        self.rpc.AttachVari.return_value = 43
        xil.attachVariables(["old"])
        xil.writeSignal("old", 1)
        self.rpc.Set.assert_called_once_with(43, 1)


if __name__ == "__main__":
    unittest.main()
