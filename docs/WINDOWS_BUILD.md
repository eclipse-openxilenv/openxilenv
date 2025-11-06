# Build Instructions for Windows


## Before You Begin
Make sure all required tools and libraries are installed as described [here](./WINDOWS_DEPENDENCIES.md). This includes MinGW, Qt, CMake, and optional FMU support libraries.

**Build Options**

>-DBUILD_EXAMPLES=ON/OFF (default ON)\
>-DBUILD_WITH_FMU2_SUPPORT=ON/OFF (default OFF)\
>&nbsp;&nbsp;-DFMI2_SOURCE_PATH=\<path>\
>&nbsp;&nbsp;-DPUGIXML_SOURCE_PATH=\<path>\
>-DBUILD_WITH_FMU3_SUPPORT=ON/OFF (default OFF)\
>&nbsp;&nbsp;-DFMI3_SOURCE_PATH=\<path>\
>&nbsp;&nbsp;-DPUGIXML_SOURCE_PATH=\<path>\
>-DBUILD_ESMINI_EXAMPLE=ON/OFF (default OFF)\
>&nbsp;&nbsp;-DESMINI_LIBRARY_PATH=\<path>\
>-DCMAKE_BUILD_TYPE=Debug/Release (default Release)\
>-DCMAKE_INSTALL_PREFIX=\<path>\
>-DBUILD_32BIT=ON/OFF (default OFF)

## 1. **Fetch source** 

Clone the openxilenv source code from https://github.com/eclipse-openxilenv/openxilenv.git e.g. to `%SRC_ROOT%\openxilenv`

## 2. **Create build directory**
```cmd
mkdir %BUILD_ROOT%\openxilenv
cd %BUILD_ROOT%\openxilenv
```
## 3. **Ensure needed tools are in PATH**

Ensure cmake, mingw, Ninja and Qt are in PATH. If not:
```cmd
set PATH=<path-to-cmake>\bin;%PATH%
set PATH=<path-to-mingw>\bin;%PATH%
set PATH=<path-to-ninja>;%PATH%
set PATH=<path-to-Qt>\bin;%PATH%
```

## 4. **Build and Install**

Common configuration example:
```cmd
cmake -G Ninja -DCMAKE_INSTALL_PREFIX=%TOOLS_ROOT%\openxilenv %SRC_ROOT%\openxilenv [Options]
cmake --build .
cmake --install .
```

## 5. **(Optional) Deploy Qt DLLs**
```cmd
cd %TOOLS_ROOT%\openxilenv
windeployqt6.exe XilEnvGui.exe
```

## :white_check_mark: You're Ready to Run OpenXilEnv!

If all steps completed successfully, OpenXilEnv is now ready to run. You can start by:

1. **Launching the GUI**:
```cmd
cd %TOOLS_ROOT%\openxilenv
XilEnvGui
```
**OR**

2. **Running an example**:
```cmd
cd %TOOLS_ROOT%\openxilenv
.\XilEnvGui -ini %SRC_ROOT%\openxilenv\Samples\Configurations\ElectricCarSample.ini
```
