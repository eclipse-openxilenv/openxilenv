# Install Dependencies on Windows

This guide explains how to install the Windows build dependencies for OpenXilEnv and how to build **Qt 6.9 (qtbase)** from source with **MinGW 14.2 (UCRT)**. All commands are for **Windows CMD** (not PowerShell).

**Choose your folders**

Set these once and reuse them everywhere. Adjust to your preference.

```cmd
set "TOOLS_ROOT=%SystemDrive%\dev\tools"
set "SRC_ROOT=%SystemDrive%\dev\src"
set "BUILD_ROOT=%SystemDrive%\dev\build"
```

**Folder layout example:**

- `%TOOLS_ROOT%\mingw64` — MinGW (niXman, 14.2.0, win32-seh-ucrt)
- `%TOOLS_ROOT%\Qt6_9` — Qt install prefix produced by the build below
- `%TOOLS_ROOT%\OpenXilEnv` — OpenXilEnv install prefix produced by the build below
- `%SRC_ROOT%\Qt6_9` — Qt source code
- `%SRC_ROOT%\OpenXilEnv` — OpenXilEnv source code
- `%BUILD_ROOT%\Qt6_9` — Qt build directory
- `%BUILD_ROOT%\OpenXilEnv` — OpenXilEnv build directory

## 1. Prerequisites

- **CMake** ≥ 3.20 (Kitware installer for Windows is fine)
- **Python** ≥ 3.x (any recent 64‑bit build)
- **7‑Zip** or compatible tool (to extract `.7z` archives)

Verify versions:
```cmd
cmake --version
python --version
```

## 2. Install MinGW

For quick starters:
1. Download **niXman mingw-builds** package [x86_64-14.2.0-release-win32-seh-ucrt-rt_v12-rev1.7z](https://github.com/niXman/mingw-builds-binaries/releases/download/14.2.0-rt_v12-rev1/x86_64-14.2.0-release-win32-seh-ucrt-rt_v12-rev1.7z)
2. Extract to: `%TOOLS_ROOT%\mingw64` 
3. Add to `PATH` (persistently or for the current session). For a one-time session:
```cmd
set PATH=%TOOLS_ROOT%\mingw64\bin;%PATH%
```

Verify:
```cmd
g++ --version
where g++
```

## 3. Build Qt 6.9 (qtbase) from source

These steps use the **qtbase 6.9.3** source tree and install to `%TOOLS_ROOT%\Qt6_9`.

1. **Fetch sources** (download [ZIP file](https://github.com/qt/qtbase/archive/refs/tags/v6.9.3.zip) or git clone of `qtbase` 6.9.3) and extract to a short path, e.g.:
   - Sources: `%SRC_ROOT%\qtbase_6_9_3`
2. **Create build directory**:
```cmd
mkdir %BUILD_ROOT%\Qt6_9
cd %BUILD_ROOT%\Qt6_9
```
3. **Ensure Ninja is available**:
```cmd
ninja --version
```
Some CMake installers ship Ninja; otherwise install it (download [ZIP file](https://github.com/ninja-build/ninja/releases/download/v1.13.1/ninja-win.zip) and unzip it) and add to PATH.

```cmd
set PATH=%TOOLS_ROOT%\ninja-win;%PATH%
```

4. **Configure** (Qt’s `configure.bat` will generate a CMake build tree):
```cmd
%SRC_ROOT%\qtbase_6_9_3\configure.bat -debug-and-release -opensource -confirm-license -prefix %TOOLS_ROOT%\Qt6_9
```
5. **Build**:
```cmd
cmake --build . --parallel
```
6. **Install (both configs)**:
```cmd
cmake --install . --config Release
cmake --install . --config Debug
```

## 4. Add Qt to PATH

Before configuring OpenXilEnv, make Qt’s bin available in PATH for the session:
```cmd
set PATH=%PATH%;%TOOLS_ROOT%\Qt6_9\bin
```

## 5. Optional / Additional Dependencies

- **pugixml 1.15** — required if building with `-DBUILD_WITH_FMU2_SUPPORT=ON` or `-DBUILD_WITH_FMU3_SUPPORT=ON`\
From https://pugixml.org/ download the [ZIP file](https://github.com/zeux/pugixml/releases/download/v1.15/pugixml-1.15.zip)\
Unzip it e.g. to `%TOOLS_ROOT%\pugixml_1_15`

- **FMU Parser (FMI 2.0 / 3.0)** — required for FMU support\
For FMI 2.0...\
Download the [ZIP file](https://github.com/modelica/fmi-standard/releases/download/v2.0.4/FMI-Standard-2.0.4.zip)\
Unzip it e.g. to `%TOOLS_ROOT%\FMI_2_0_4`\
For FMI 3.0...\
Download the [ZIP file](https://github.com/modelica/fmi-standard/releases/download/v3.0.1/FMI-Standard-3.0.1.zip)\
Unzip it e.g. to `%TOOLS_ROOT%\FMI_3_0_1`


Keep the source paths handy to pass via:
```
-DPUGIXML_SOURCE_PATH=%TOOLS_ROOT%\pugixml_1_15\src
-DFMI2_SOURCE_PATH=%TOOLS_ROOT%\FMI_2_0_4\headers
-DFMI3_SOURCE_PATH=%TOOLS_ROOT%\FMI_3_0_1\headers
```

## 6. Known‑Good Versions (reference)

- MinGW: **x86_64-14.2.0-release-win32-seh-ucrt-rt_v12-rev1**
- Qt: **6.9.x (qtbase)** built with the above MinGW
- CMake: **>= 3.20**
- Python: **>= 3.x**

## Appendix: Example Session (all in one place)

```cmd
:: Set root pathes:
set "TOOLS_ROOT=%SystemDrive%\dev\tools"
set "SRC_ROOT=%SystemDrive%\dev\src"
set "BUILD_ROOT=%SystemDrive%\dev\build"

:: Add mingw to PATH. Make sure necessary tools are on PATH for this session, including cmake and python3
set PATH=%TOOLS_ROOT%\mingw64\bin;<path-to-cmake>;<path-to-python3>;%PATH%

:: Configure, build, install Qt
cd %BUILD_ROOT%\Qt6_9
%SRC_ROOT%\qtbase_6_9_3\configure.bat -debug-and-release -opensource -confirm-license -prefix %TOOLS_ROOT%\Qt6_9
cmake --build . --parallel
cmake --install . --config Release
cmake --install . --config Debug

:: Add Qt to PATH
set PATH=%PATH%;%TOOLS_ROOT%\Qt6_9\bin
```

## With all dependencies installed, you can now [build OpenXilEnv](./WINDOWS_BUILD.md).