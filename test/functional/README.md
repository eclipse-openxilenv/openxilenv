# OpenXilEnv Functional Tests
This is a set of tests for the XilEnvRpc dll and functionality of the built OpenXilEnv application.
The tests are implemented in [pytest](https://docs.pytest.org/en/stable/index.html#) using the OpenXilEnv python API that comes with OpenXilEnv.

## Dependencies
- `OpenXilEnv installation` under _path\to\openxilenv_ (do not confuse with OpenXilEnv repository)
- python packages `setuptools`, `wheel`, `pytest`
- OpenXilEnv python API

## Installation
- Follow the steps to build and install OpenXilEnv as discribed in the README in the _root_ directory.
- We recommend to set-up a virtual environment to install all python packages. In the _root_ directory run (windows)
  ```sh
    python -m venv .venv
    .\.venv\Scripts\activate
    python -m pip install --upgrade pip
    python -m pip install setuptools wheel pytest
  ```
- Follow the steps under _root\Samples\Automation\OpenXilEnv_ to install the python API. Make sure to install it with the virtual environment activated.

## Configuration
To instantiate the python API, pytest needs the _path\to\openxilenv_, that contains the _XilEnvGui.exe_ and _XilEnvRpc.dll_. As well as a _.ini_ file that configures the Gui itself. These paths need to be set manually in the _root\test\self\conftest.py_ file:
```python
  xilEnvInstallationPath = Path("path\\to\\openxilenv")
  iniFilePath = Path("root\\Samples\\Configurations\\ElectricCarSample.ini")
```

## Execution
From the _root_ directory run
  ```sh
    pytest -v
  ```

## TODO
- clearify usage of openXilEnv vs. XilEnvRpc
- fill tests with life...