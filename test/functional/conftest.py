import pytest
from OpenXilEnv.openXilEnv import OpenXilEnv
from pathlib import Path

@pytest.fixture(scope="session")
def xilEnvInstallationPath() -> OpenXilEnv:
    xilEnvInstallationPath = Path("C:\\app\\tools\\openxilenv")
    assert xilEnvInstallationPath.is_dir(), f"no such directory {xilEnvInstallationPath}"
    return xilEnvInstallationPath

@pytest.fixture(scope="session")
def iniFilePath() -> Path:
    iniFilePath = Path("C:\\UserData\\git_ws\\openxilenv\\Samples\\Configurations\\ElectricCarSample.ini")
    assert iniFilePath.is_file(), f"no such directory {iniFilePath}"
    return iniFilePath