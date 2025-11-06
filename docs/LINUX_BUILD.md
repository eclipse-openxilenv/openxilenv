# Build Instructions for Linux

## Before You Begin
Make sure all required tools and libraries are installed as described [here](./LINUX_DEPENDENCIES.md). This includes GCC, Qt, CMake, and optional FMU support libraries.

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

Clone the openxilenv source code from https://github.com/eclipse-openxilenv/openxilenv.git e.g. to `$SRC_ROOT/openxilenv`

```bash
cd "$SRC_ROOT"
git clone https://github.com/eclipse-openxilenv/openxilenv.git
```

## 2. **Create build directory**
```bash
mkdir -p "$BUILD_ROOT/openxilenv"
cd "$BUILD_ROOT/openxilenv"
```

## 3. **Ensure needed tools are in PATH**

Ensure cmake, gcc, ninja and Qt are in PATH. If not:
```bash
export PATH="<path-to-cmake>/bin:$PATH"
export PATH="<path-to-ninja>:$PATH"
export PATH="$TOOLS_ROOT/Qt6.9/bin:$PATH"
export LD_LIBRARY_PATH="$TOOLS_ROOT/Qt6.9/lib:$LD_LIBRARY_PATH"
export Qt6_DIR="$TOOLS_ROOT/Qt6.9/lib/cmake/Qt6"
```

Example with the recommended paths:
```bash
export PATH="$TOOLS_ROOT/Qt6.9/bin:$PATH"
export LD_LIBRARY_PATH="$TOOLS_ROOT/Qt6.9/lib:$LD_LIBRARY_PATH"
export Qt6_DIR="$TOOLS_ROOT/Qt6.9/lib/cmake/Qt6"
```

## 4. **Build and Install**
```bash
cmake -G Ninja -DCMAKE_INSTALL_PREFIX="$TOOLS_ROOT/openxilenv" "$SRC_ROOT/openxilenv" [Options]
cmake --build .
cmake --install .
```

**Common configuration example:**
```bash
cmake -G Ninja \
    -DCMAKE_INSTALL_PREFIX="$TOOLS_ROOT/openxilenv" \
    -DBUILD_EXAMPLES=ON \
    "$SRC_ROOT/openxilenv"
cmake --build . --parallel $(nproc)
cmake --install .
```

**Example with FMU support:**
```bash
cmake -G Ninja \
    -DCMAKE_INSTALL_PREFIX="$TOOLS_ROOT/openxilenv" \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_WITH_FMU2_SUPPORT=ON \
    -DFMI2_SOURCE_PATH="$SRC_ROOT/FMI_2_0_4/headers" \
    -DPUGIXML_SOURCE_PATH="$SRC_ROOT/pugixml-1.15/src" \
    "$SRC_ROOT/openxilenv"
cmake --build . --parallel $(nproc)
cmake --install .
```

## 5. **Add OpenXilEnv to PATH**

To run OpenXilEnv from anywhere, add it to your PATH:
```bash
export PATH="$TOOLS_ROOT/openxilenv:$PATH"
export LD_LIBRARY_PATH="$TOOLS_ROOT/openxilenv:$LD_LIBRARY_PATH"
```

To make this permanent, add to your `~/.bashrc`:
```bash
echo 'export PATH="$TOOLS_ROOT/openxilenv:$PATH"' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH="$TOOLS_ROOT/openxilenv:$LD_LIBRARY_PATH"' >> ~/.bashrc
source ~/.bashrc
```

## ✅ You're Ready to Run OpenXilEnv!

If all steps completed successfully, OpenXilEnv is now ready to run. You can start by:

1. **Launching the GUI**:
```bash
cd "$TOOLS_ROOT/openxilenv"
./XilEnvGui
```
**OR**

2. **Running an example**:
```bash
cd "$TOOLS_ROOT/openxilenv"
./XilEnvGui -ini "$SRC_ROOT/openxilenv/Samples/Configurations/ElectricCarSample.ini"
```

## Troubleshooting

### Qt libraries not found
If you get errors about missing Qt libraries when running XilEnvGui:
```bash
export LD_LIBRARY_PATH="$TOOLS_ROOT/Qt6.9/lib:$LD_LIBRARY_PATH"
```

### CMake cannot find Qt6
Set the Qt6_DIR variable explicitly:
```bash
export Qt6_DIR="$TOOLS_ROOT/Qt6.9/lib/cmake/Qt6"
```

Then reconfigure:
```bash
cd "$BUILD_ROOT/openxilenv"
rm -rf *
cmake -G Ninja -DCMAKE_INSTALL_PREFIX="$TOOLS_ROOT/openxilenv" "$SRC_ROOT/openxilenv"
```