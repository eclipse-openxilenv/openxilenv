# OpenXilEnv

OpenXilEnv is a lightweight SiL/HiL environment. At the moment, OpenXilEnv focuses on SiL environments.

## SiL

With a **S**oftware **I**n the **L**oop system, it is possible to run and test embedded software without a target platform or compiler.
XilEnv provides an environment to set up a SiL system on Windows or Linux hosts. It clearly separates its own components from the embedded software under test through a network layer. Each component can run in its own executable, providing memory protection between them.

Communication between the software under test, models, and XilEnv is done via sockets or (on Windows) Named Pipes / (on Linux) local sockets. These are used for:
- Signal transfers
- Virtual network (CAN and CAN FD) transfers

![XilEnv SiL system](OpenXilEnv_Sil.png)

**XilEnvGui** provides a configurable graphical user interface based on Qt. It includes display/change widgets, oscilloscopes, calibration tools, sliders, knobs, and other interaction elements.

**XilEnv** (CLI) has no graphical user interface and can be used for automation without user interaction.

Main features:
- Supports current GCC compilers with DWARF debug information. Visual Studio compiler is supported but without debug information
- FMUs with FMI 2.0 interface are supported via **ExtProc_FMU2Extract(.exe)**, **ExtProc_FMU2Loader32(.exe)**, and **ExtProc_FMU2Loader64(.exe)**
- Partial FMI 3.0 support (additional datatypes only) via **ExtProc_FMU3Extract(.exe)**, **ExtProc_FMU3Loader32(.exe)**, and **ExtProc_FMU3Loader64(.exe)**
- Includes a small A2L parser for calibration
- Supports XCP over Ethernet for connection to a calibration system
- Multicore support with synchronization barriers
- Automatable via **XilEnvRpc.dll/.so** or the built-in script interpreter
- Supports 32- and 64-bit executables (mixed allowed). XilEnv itself must be 64-bit
- Time simulation with fixed, configurable period and 1 ns resolution
- Residual bus simulation for missing CAN (FD) members
- Recording and stimulation supported via text or MDF3 files

The DLL/shared library **XilEnvExtProc64.dll/.so** or **XilEnvExtProc32.dll/.so** provides the interface for the embedded test software or model. This DLL/shared library must be loaded dynamically. The main module **XilEnvExtProcMain.c** handles this automatically.

Interface functions are declared in **XilEnvRtProc.h**.

An example can be found in `Samples/ExternalProcesses/ExtProc_Simple`.

## Getting Started

1. Install dependencies ([Windows](docs/WINDOWS_DEPENDENCIES.md) | [Linux](docs/LINUX_DEPENDENCIES.md))
2. Build the project ([Windows](docs/WINDOWS_BUILD.md) | [Linux](docs/LINUX_BUILD.md))

## Setting up Your Project

- [Setup an External Process](docs/EXTERNAL_PROCESS_SETUP.md)

## HiL Option

To use OpenXilEnv as a HiL system, a second Linux PC is required. Only **SocketCAN FD** interfaces are supported.

For <1 ms response times, install the **RT-Preempt** patch.

OpenXilEnv is split into:
- **LinuxRemoteMasterCore.so** – real-time (Linux with RT-Preempt)
- **XilEnv executable** – non–real-time (Windows or Linux)

Use a direct Ethernet connection between PCs.

If a model is needed, compile it for Linux and link with **LinuxRemoteMasterCore.so**.  
If not, use **LinuxRemoteMaster.out**.

Ensure **RemoteStartServer** is installed and running on the Linux PC to allow remote execution.

## License
This project is part of the Eclipse Foundation and licensed under the [Apache License 2.0](LICENSE.txt).

## Contributing
Contributions are welcome! Please see the [CONTRIBUTING.md](CONTRIBUTING.md) file for details.