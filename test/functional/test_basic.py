import pytest
from OpenXilEnv.openXilEnv import OpenXilEnv
from pathlib import Path


def test_StartWithoutGuiAndDisconnect(xilEnvInstallationPath : Path, iniFilePath : Path) -> None:
    xil = OpenXilEnv(xilEnvInstallationPath)
    xil.startWithoutGui(iniFilePath)
    assert xil.isConncted()
    xil.disconnectAndCloseXil()
    assert not xil.isConncted()