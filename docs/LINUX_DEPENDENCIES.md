# Install Dependencies on Linux

This guide explains how to install the Linux build dependencies for OpenXilEnv on **Ubuntu/Debian**. Other distributions may work but are not actively supported.

All commands are for **bash** shell.

**Choose your folders**

We recommend the following folder structure and will stick to it during this build manual.
You can change the layout to your preference and adopt the command lines accordingly.

```
$HOME/dev/
├── tools/                  # $TOOLS_ROOT
│   ├── mingw64/            # MinGW (niXman, 14.2.0, win32-seh-ucrt)
│   ├── Qt6_9_3/            # Qt install prefix
│   └── OpenXilEnv/         # OpenXilEnv install prefix
│
├── src/                    # $SRC_ROOT
│   ├── Qt6_9_3/            # Qt source code
│   └── OpenXilEnv/         # OpenXilEnv source code
│
├── build/                  # $BUILD_ROOT
│   ├── Qt6_9_3/            # Qt build directory
│   └── OpenXilEnv/         # OpenXilEnv build directory
```

Set these variables once and reuse them everywhere. Adjust to your preference.

```bash
export TOOLS_ROOT="$HOME/dev/tools"
export SRC_ROOT="$HOME/dev/src"
export BUILD_ROOT="$HOME/dev/build"
```

**Folder layout example:**

- `$TOOLS_ROOT/openxilenv` - OpenXilEnv install prefix
- `$SRC_ROOT/openxilenv` - OpenXilEnv source code
- `$BUILD_ROOT/openxilenv` - OpenXilEnv build directory

## 1. Prerequisites

Install essential build tools and libraries using apt.

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build python3 \
    libgl1-mesa-dev libglu1-mesa-dev libxkbcommon-dev \
    libxcb1-dev libxcb-cursor-dev libxcb-glx0-dev libxcb-icccm4-dev \
    libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev \
    libxcb-render-util0-dev libxcb-shape0-dev libxcb-shm0-dev \
    libxcb-sync-dev libxcb-xfixes0-dev libxcb-xinerama0-dev \
    libxcb-xkb-dev libx11-xcb-dev libfontconfig1-dev \
    libfreetype6-dev libharfbuzz-dev libpng-dev libjpeg-dev \
    git wget
```

Verify versions:
```bash
cmake --version
gcc --version
python3 --version
ninja --version
```

**Minimum requirements:**
- CMake >= 3.20
- GCC >= 9.0 (for C++17 support)
- Python >= 3.6

## 2. Install Qt 6

The recommended way to install Qt is via the package manager.

### Ubuntu 24.04 (Noble) and later:

```bash
sudo apt install -y qt6-base-dev qt6-base-dev-tools
```

### Ubuntu 22.04 (Jammy) and earlier:

Qt 6.9 is not available in the default repositories. You have two options:

1. **Use a PPA** (if available for your version)
2. **Build from source** (see [Appendix](#appendix-building-qt-69-from-source-alternative-method) below)

After installation, verify Qt is available:
```bash
qmake6 --version
# or
qmake -qt6 --version
```

## 3. Create Directory Structure

```bash
mkdir -p "$TOOLS_ROOT" "$SRC_ROOT" "$BUILD_ROOT"
```

## 4. Optional / Additional Dependencies

- **pugixml 1.15** - required if building with `-DBUILD_WITH_FMU2_SUPPORT=ON` or `-DBUILD_WITH_FMU3_SUPPORT=ON`

```bash
cd "$SRC_ROOT"
wget https://github.com/zeux/pugixml/releases/download/v1.15/pugixml-1.15.tar.gz
tar -xzf pugixml-1.15.tar.gz
```

- **FMU Parser (FMI 2.0 / 3.0)** - required for FMU support

For FMI 2.0:
```bash
cd "$SRC_ROOT"
wget https://github.com/modelica/fmi-standard/releases/download/v2.0.4/FMI-Standard-2.0.4.zip
unzip FMI-Standard-2.0.4.zip -d FMI_2_0_4
```

For FMI 3.0:
```bash
cd "$SRC_ROOT"
wget https://github.com/modelica/fmi-standard/releases/download/v3.0.1/FMI-Standard-3.0.1.zip
unzip FMI-Standard-3.0.1.zip -d FMI_3_0_1
```

Keep the source paths handy to pass via:
```
-DPUGIXML_SOURCE_PATH=$SRC_ROOT/pugixml-1.15/src
-DFMI2_SOURCE_PATH=$SRC_ROOT/FMI_2_0_4/headers
-DFMI3_SOURCE_PATH=$SRC_ROOT/FMI_3_0_1/headers
```

## 5. RT-Preempt Patch (Optional - for HiL systems)

If you plan to use OpenXilEnv as a HiL system with <1 ms response times, install the RT-Preempt kernel patch:

```bash
# Check available RT kernels
apt search linux-image-rt

# Install RT kernel (example for Ubuntu)
sudo apt install linux-image-rt linux-headers-rt
```

For custom kernels or more information, see the official RT-Preempt documentation at:
https://wiki.linuxfoundation.org/realtime/start

After installation, reboot and select the RT kernel from GRUB menu.

## 6. Known-Good Versions (reference)

- GCC: **>= 9.0** (tested with 9.x, 10.x, 11.x, 12.x, 13.x)
- Qt: **6.9.x** (qtbase)
- CMake: **>= 3.20**
- Python: **>= 3.6**
- Ninja: **>= 1.10**

## Appendix: Building Qt 6.9 from Source (Alternative Method)

If Qt 6.9 is not available via package manager, you can build it from source.

**Folder layout for source build:**
- `$TOOLS_ROOT/Qt6.9` - Qt install prefix
- `$SRC_ROOT/Qt6.9` - Qt source code
- `$BUILD_ROOT/Qt6.9` - Qt build directory

### Steps:

1. **Fetch sources** (download [ZIP file](https://github.com/qt/qtbase/archive/refs/tags/v6.9.3.zip) or use git):
```bash
cd "$SRC_ROOT"
wget https://github.com/qt/qtbase/archive/refs/tags/v6.9.3.zip
unzip v6.9.3.zip
```

Or using git:
```bash
cd "$SRC_ROOT"
git clone --branch v6.9.3 --depth 1 https://github.com/qt/qtbase.git
```

2. **Create build directory**:
```bash
mkdir -p "$BUILD_ROOT/Qt6.9"
cd "$BUILD_ROOT/Qt6.9"
```

3. **Configure** (Qt's `configure` script will generate a CMake build tree):
```bash
"$SRC_ROOT/qtbase_6_9_3/configure" \
    -release \
    -opensource \
    -confirm-license \
    -prefix "$TOOLS_ROOT/Qt6.9" \
    -nomake examples \
    -nomake tests
```

**Note:** You can use `-debug-and-release` instead of `-release` for both build types, though this may not be available on all platforms. If you encounter issues, use either `-release` or `-debug` separately.

4. **Build**:
```bash
cmake --build . --parallel $(nproc)
```

5. **Install**:
```bash
cmake --install .
```

If you built with separate debug/release configurations:
```bash
cmake --install . --config Release
cmake --install . --config Debug
```

6. **Add Qt to PATH and LD_LIBRARY_PATH**:

```bash
export PATH="$TOOLS_ROOT/Qt6.9/bin:$PATH"
export LD_LIBRARY_PATH="$TOOLS_ROOT/Qt6.9/lib:$LD_LIBRARY_PATH"
export Qt6_DIR="$TOOLS_ROOT/Qt6.9/lib/cmake/Qt6"
```

To make these permanent, add them to your `~/.bashrc`:
```bash
echo 'export PATH="$TOOLS_ROOT/Qt6.9/bin:$PATH"' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH="$TOOLS_ROOT/Qt6.9/lib:$LD_LIBRARY_PATH"' >> ~/.bashrc
echo 'export Qt6_DIR="$TOOLS_ROOT/Qt6.9/lib/cmake/Qt6"' >> ~/.bashrc
```

Then reload:
```bash
source ~/.bashrc
```

---

## With all dependencies installed, you can now [build OpenXilEnv](./LINUX_BUILD.md).