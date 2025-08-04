import pytest
from OpenXilEnv.openXilEnv import OpenXilEnv
from pathlib import Path

@pytest.fixture(scope="module")
def runningXil(xilEnvInstallationPath : Path, iniFilePath : Path):
    # setup
    xil = OpenXilEnv(xilEnvInstallationPath)
    xil.startWithoutGui(iniFilePath)
    assert xil.isConncted()
    yield xil
    # teardown
    xil.disconnectAndCloseXil()

def test_ReadAndWrite(runningXil : OpenXilEnv) -> None:
    assert True

def test_AddAtachAndRemove(runningXil : OpenXilEnv) -> None:
    assert True