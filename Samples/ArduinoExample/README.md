# ArduinoCode example for SIL

## Table of contents
- [ArduinoCode example for SIL](#arduinocode-example-for-sil)
  - [Table of contents](#table-of-contents)
  - [Introduction](#introduction)
  - [The example](#the-example)
  - [Simulation of Arduino drivers](#simulation-of-arduino-drivers)
  - [Integration of Arduino source code](#integration-of-arduino-source-code)
  - [Building and using the external processes (Windows)](#building-and-using-the-external-processes-windows)
    - [Generate build files with CMake for external processes](#generate-build-files-with-cmake-for-external-processes)
    - [Build executables files for external processes](#build-executables-files-for-external-processes)
    - [Modify the arduino\_sim\_config.ini](#modify-the-arduino_sim_configini)
- [Arduino-CLI](#arduino-cli)
  - [Setup](#setup)
    - [Commands](#commands)
  - [CMake build (Windows)](#cmake-build-windows)


## Introduction
This is an example for integrating sample source code into the OpenXiLEnv which was written for Arduino UNO. It shall help to understand how the integration of any source code is working which actually runs on a MCU in the real world. <br>
The example includes a simple simulation of the Arduino driver. **The simulated driver were implemented to have a kind of guideline how a simulation of MCU driver can work. They were kept really simple and might have some bugs.**


## The example
To understand what is the Arduino sketch about, take a look to [arduino_example_circuit.pdf](arduino_example_circuit.pdf) and [arduino_example.ino](arduino_example.ino). The code is very ease. Pressing the button turns on the LED, releasing the button turns it off.


## Simulation of Arduino drivers
When you are working with Arduino IDE and .ino sketches you must include the `Arduino.h` header file to use functions like `pinMode`, `digitalRead` and `digitalWrite`. Alle these functions are dependent on registers and addresses which are defined for the specific microchip. In the case of the Arduino UNO it's a AVR microchip. <br><br>
Tho make the Arduino sketch code run in the OpenXilEnv as external process, the `Arduino.h` must be reimplemented. A driver simulation of this header file must be implemented. <br>
For that purpose, a new folder was created called `driverSimulations`. This folder includes a re-definition of `Arduino.h` and a implementation file where the actual simulation is implemented, called `pin.c`. <br>
For now only all necessary simulations are implemented which are required for this Arduino example. **The simulation is not complete!** If required, it can be extended.


## Integration of Arduino source code
The important parts of the code from [arduino_example.ino] (`loop` and `setup` functions)(arduino_example.ino) were integrated into the [arduino_process.cpp](xil_sources/arduino_process.cpp) and [arduino_model.cpp](xil_sources/arduino_model.cpp). All necessary variables were also referenced there.


## Building and using the external processes (Windows)

The cmake build configuration has a option called `BUILD_SIL_SOURCES`. Enabling this option will automatically include the simulated drivers and build the external processes. Disabling this option will compile and upload the Arduino sketch to the connected board. <br>
For this option the Arduino CLI is required. When you want to compile and upload your sketch, follow the instruction in chapter [Arduino-CLI](#arduino-cli). 

### Generate build files with CMake for external processes
Run the following command in the directory of this README file:

    cmake -G "MinGW Makefiles" -S . -B .\build -DCMAKE_BUILD_TYPE=DEBUG -DBUILD_SIL_SOURCES=ON


### Build executables files for external processes
Once the CMake file were generated, run this command to build the executable files (external processes for XiL environment):

    mingw32-make -C .\build\


### Modify the arduino_sim_config.ini
Add the built external processes as process in the INI section "InitStartProcesses" ([arduino_sim_config.ini](arduino_sim_config.ini)).


# Arduino-CLI
- Documentation: https://www.arduino.cc/pro/software-pro-cli/
- Releases: https://github.com/arduino/arduino-cli/releases
- Download-Link for Windows: https://github.com/arduino/arduino-cli/releases/download/v1.0.4/arduino-cli_1.0.4_Windows_64bit.zip


## Setup
Add the path of arduino-cli.exe to PATH.
 

### Commands
 
Initialize configurations:
 
    arduino-cli config init
 
 
Compile sketch (for Arduino UNO):
 
    arduino-cli compile --fqbn arduino:avr:uno .\arduino_example.ino
 
 
Upload sketch:
 
    arduino-cli upload -p COM5 --fqbn arduino:avr:uno .\arduino_example.ino
 
## CMake build (Windows)
 
    cmake -S . -B build -G "MinGW Makefiles" -DBUILD_SIL_SOURCES=OFF
 
    cmake --build build --target build_and_upload