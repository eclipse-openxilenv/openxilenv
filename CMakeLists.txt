cmake_minimum_required(VERSION 3.19)

project(XilEnvGui VERSION 10.0 LANGUAGES C CXX)

option(BUILD_ONLY_EXAMPLES "Build only examples" OFF)

option(BUILD_32BIT "Build 32 bit target (only librarys and/or examples)" OFF)

option(BUILD_WITH_FMU2_SUPPORT "Build with FMU2 support" OFF)
option(BUILD_WITH_FMU3_SUPPORT "Build with FMU3 support" OFF)
set(PUGIXML_SOURCE_PATH "Default" CACHE PATH "path to pugixml")
#option(PUGIXML_SOURCE_PATH "Path to the sourcees of pugixml")
set(FMI2_SOURCE_PATH "Default" CACHE PATH "path to FMI2")
set(FMI3_SOURCE_PATH "Default" CACHE PATH "path to FMI3")
#option(FMI_SOURCE_PATH "Path to the sourcees of FMI interface")

option(BUILD_EXAMPLES "Build with examples" ON)
option(BUILD_ESMINI_EXAMPLE "Build with Esmini examples (OpenSCENARIO)" OFF)
set(ESMINI_LIBRARY_PATH "Default" CACHE PATH "path to esmini library")

option(BUILD_WITH_J1939_SUPPORT "Build with J1939 support" OFF)
option(BUILD_WITH_GATEWAY_VIRTUAL_CAN "Build with virtual CAN gateway" OFF)
option(BUILD_WITH_PDB_READER_DLL_INTERFACE "Build with PDB reader DLL interface" OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# don't use 'CMAKE_SIZEOF_VOID_P' setup it own 'MY_CMAKE_SIZEOF_VOID_P'
if(BUILD_32BIT)
    set(MY_CMAKE_SIZEOF_VOID_P 4)
else()
    set(MY_CMAKE_SIZEOF_VOID_P 8)
endif()

if(NOT BUILD_ONLY_EXAMPLES AND NOT BUILD_32BIT)
# Qt is only neccessay if 64 bit and not only examples should beebn build
    message("Qt")
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

message("Target cpu architecture byte size")
message(${MY_CMAKE_SIZEOF_VOID_P})
message("Target compiler")
message(${CMAKE_C_COMPILER}) 

if(BUILD_ONLY_EXAMPLES)
    add_subdirectory(Samples)
else()
    # Do not build 32 bit executables of XilEnv, XilEnvGui anf XilEnvRpc
    if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
        message("Build XilEnv, XilEnvGui and XilEnvRpc")

        find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
        find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

        if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
            qt_add_executable(XilEnvGui
                MANUAL_FINALIZATION
                ${PROJECT_SOURCES}
            )
            qt6_disable_unicode_defines(XilEnvGui)
            #set_property(TARGET XilEnvGui PROPERTY QT_NO_UNICODE_DEFINES TRUE)
        else()
            add_executable(XilEnvGui
                ${PROJECT_SOURCES}
            )
        endif()

        target_include_directories(XilEnvGui PRIVATE
            Src/A2lParser
            Src/Blackboard
            Src/CanDataBase
            Src/CanServer
            Src/CanServer/J1939
            Src/DebugInfos
            Src/Equations
            Src/ExternalProcess
            Src/Global
            Src/GUI/Qt
            Src/GUI/Qt/Data
            Src/GUI/Qt/Dialog
            Src/GUI/Qt/Interfaces
            Src/GUI/Qt/Widgets
            Src/GUI/Qt/Widgets/A2LCalMap
            Src/GUI/Qt/Widgets/A2LCalSingle
            Src/GUI/Qt/Widgets/Bargraph
            Src/GUI/Qt/Widgets/CalibrationMap
            Src/GUI/Qt/Widgets/CalibrationTree
            Src/GUI/Qt/Widgets/CanMessage
            Src/GUI/Qt/Widgets/Enum
            Src/GUI/Qt/Widgets/ErrorMemory
            Src/GUI/Qt/Widgets/Knob
            Src/GUI/Qt/Widgets/Lamps
            Src/GUI/Qt/Widgets/Oscilloscope
            Src/GUI/Qt/Widgets/Slider
            Src/GUI/Qt/Widgets/Tacho
            Src/GUI/Qt/Widgets/Text
            Src/GUI/Qt/Widgets/UserControl
            Src/GUI/Qt/Widgets/UserDraw
            Src/IniFileDataBase
            Src/Ramps
            Src/RemoteMaster
            Src/RemoteMaster/Client
            Src/RpcInterface
            Src/Scheduler
            Src/Script
            Src/StimulusPlayer
            Src/TraceRecorder
            Src/Utilities
            Src/XcpOverEthernet)
        target_compile_definitions(XilEnvGui PUBLIC _M_X64)
        if(BUILD_WITH_J1939_SUPPORT)
            target_compile_definitions(XilEnvGui PUBLIC BUILD_WITH_J1939_SUPPORT)
        endif()
        if(BUILD_WITH_GATEWAY_VIRTUAL_CAN)
            target_compile_definitions(XilEnvGui PUBLIC BUILD_WITH_GATEWAY_VIRTUAL_CAN)
        endif()
        if(BUILD_WITH_PDB_READER_DLL_INTERFACE)
            target_compile_definitions(XilEnvGui PUBLIC BUILD_WITH_PDB_READER_DLL_INTERFACE)
        endif()

        target_link_libraries(XilEnvGui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
        if(WIN32)
            if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
                target_compile_options(XilEnvGui PRIVATE /wd5105)
            elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                target_compile_definitions(XilEnvGui PRIVATE WINVER=0x0600)
                target_compile_definitions(XilEnvGui PRIVATE _WIN32_WINNT=0x0600)
                target_compile_definitions(XilEnvGui PRIVATE __STDC_FORMAT_MACROS)
                target_link_libraries(XilEnvGui PRIVATE ws2_32)
                target_link_libraries(XilEnvGui PRIVATE Psapi)
            endif()
            target_link_libraries(XilEnvGui PRIVATE Winmm.lib)
        else()
            target_link_libraries(XilEnvGui PRIVATE pthread)
        endif()

        #
        # XilEnv target
        #

        add_executable(XilEnv)
        target_include_directories(XilEnv PRIVATE
            Src/A2lParser
            Src/Blackboard
            Src/CanDataBase
            Src/CanServer
            Src/CanServer/J1939
            Src/DebugInfos
            Src/Equations
            Src/ExternalProcess
            Src/Global
            Src/GUI/Console
            Src/IniFileDataBase
            Src/Ramps
            Src/RemoteMaster
            Src/RemoteMaster/Client
            Src/RpcInterface
            Src/Scheduler
            Src/Script
            Src/StimulusPlayer
            Src/TraceRecorder
            Src/Utilities
            Src/XcpOverEthernet)
        if(WIN32)
            target_compile_definitions(XilEnv PUBLIC _CRT_NONSTDC_NO_WARNINGS)
            target_compile_definitions(XilEnv PUBLIC _CRT_SECURE_NO_WARNINGS)
            target_compile_definitions(XilEnv PUBLIC WIN32)
            target_compile_definitions(XilEnv PUBLIC NDEBUG)
            target_compile_definitions(XilEnv PUBLIC _CONSOLE)
        endif()
        target_compile_definitions(XilEnv PUBLIC _M_X64)
        target_compile_definitions(XilEnv PUBLIC NO_GUI)
        if(BUILD_WITH_J1939_SUPPORT)
            target_compile_definitions(XilEnv PUBLIC BUILD_WITH_J1939_SUPPORT)
        endif()
        if(BUILD_WITH_GATEWAY_VIRTUAL_CAN)
            target_compile_definitions(XilEnv PUBLIC BUILD_WITH_GATEWAY_VIRTUAL_CAN)
        endif()
        if(BUILD_WITH_PDB_READER_DLL_INTERFACE)
            target_compile_definitions(XilEnv PUBLIC BUILD_WITH_PDB_READER_DLL_INTERFACE)
        endif()
        if(WIN32)
            if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
                target_compile_options(XilEnv PRIVATE /wd5105)
            elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                target_compile_definitions(XilEnv PRIVATE WINVER=0x0600)
                target_compile_definitions(XilEnv PRIVATE _WIN32_WINNT=0x0600)
                target_compile_definitions(XilEnv PRIVATE __STDC_FORMAT_MACROS)
                target_link_libraries(XilEnv PRIVATE ws2_32)
                target_link_libraries(XilEnv PRIVATE Psapi)
            endif()
            target_link_libraries(XilEnv PRIVATE Winmm.lib)
        else()
            target_link_libraries(XilEnv PRIVATE pthread)
        endif()

        #
        # XilEnvRpc target
        #

        add_library(XilEnvRpc SHARED)
        set_target_properties(XilEnvRpc PROPERTIES PUBLIC_HEADER 
            ./Src/RpcInterface/XilEnvRpc.h
        )

        target_include_directories(XilEnvRpc
            PRIVATE
            Src/Global
            Src/Utilities
            Src/Blackboard
            Src/Scheduler
            Src/CanServer
            Src/CanDataBase
            Src/A2lParser
            Src/RpcInterface
        )
        target_compile_definitions(XilEnvRpc PUBLIC _M_X64)
        target_compile_definitions(XilEnvRpc PUBLIC _USRDLL)
        target_compile_definitions(XilEnvRpc PUBLIC SCRPCDLL_EXPORTS)
        if(WIN32)
            if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
                target_compile_options(XilEnvRpc PRIVATE /wd5105)
            elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                # do not as "lib" before the DLL name
                set(CMAKE_SHARED_LIBRARY_PREFIX "")
                target_compile_definitions(XilEnvRpc PRIVATE WINVER=0x0600)
                target_compile_definitions(XilEnvRpc PRIVATE _WIN32_WINNT=0x0600)
                target_compile_definitions(XilEnvRpc PRIVATE __STDC_FORMAT_MACROS)
            endif()
            target_link_libraries(XilEnvRpc PRIVATE Ws2_32)
        endif()

    endif()


    #
    # XilEnvExtProc target
    #

    if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
        set(XilEnvExtProcTargetName XilEnvExtProc64)
        set(XilEnvExtProcLibPath "")
    else()
        set(XilEnvExtProcTargetName XilEnvExtProc32)
        set(XilEnvExtProcLibPath "/Bin32")
    endif()

    add_library(${XilEnvExtProcTargetName} SHARED)
    set_target_properties(${XilEnvExtProcTargetName} PROPERTIES PUBLIC_HEADER
        ./Src/ExternalProcess/XilEnvExtProc.h
        ./Src/ExternalProcess/XilEnvExtProcMain.c
        ./Src/ExternalProcess/XilEnvExtProcInterfaceMethods.c
    )

    target_include_directories(${XilEnvExtProcTargetName}
        PRIVATE
        Src/Global
        Src/Utilities
        Src/Scheduler
        Src/CanDataBase
    )
    if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
        target_compile_definitions(${XilEnvExtProcTargetName} PRIVATE _M_X64)
    endif()
    target_compile_definitions(${XilEnvExtProcTargetName} PRIVATE _USERDLL)
    target_compile_definitions(${XilEnvExtProcTargetName} PRIVATE EXTPROC_DLL_EXPORTS)
    target_compile_definitions(${XilEnvExtProcTargetName} PRIVATE SOFTCAR_INTERFACE_TYPE=0)

    if(WIN32)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
            target_compile_options(${XilEnvExtProcTargetName} PRIVATE /wd5105)
        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            # do not as "lib" before the DLL name
            set(CMAKE_SHARED_LIBRARY_PREFIX "")
            target_compile_definitions(${XilEnvExtProcTargetName} PRIVATE WINVER=0x0600)
            target_compile_definitions(${XilEnvExtProcTargetName} PRIVATE _WIN32_WINNT=0x0600)
            target_compile_definitions(${XilEnvExtProcTargetName} PRIVATE __STDC_FORMAT_MACROS)
            if(BUILD_32BIT)
                message ("Build 32 bit ${XilEnvExtProcTargetName}")
                set_target_properties(${XilEnvExtProcTargetName} PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
                # switch off 2 warnings for 32 bit build because there are a lot of pointers stored inside a 64 bit integer
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast")
            endif()
        endif()
        target_link_libraries(${XilEnvExtProcTargetName} PRIVATE Ws2_32)
    endif()

    if(UNIX)
        if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)

            #
            # LinuxRemoteMasterCore target
            #

            add_library(LinuxRemoteMasterCore SHARED)
            target_include_directories(LinuxRemoteMasterCore PRIVATE
                Src/Blackboard
                Src/CanDataBase
                Src/CanServer
                Src/Global
                Src/Ramps
                Src/RemoteMaster
                Src/RemoteMaster/Client
                Src/RemoteMaster/Server
                Src/RemoteMaster/Server/Drivers
                Src/RemoteMaster/Server/Drivers/SocketCan
                Src/RemoteMaster/Server/Equations
                Src/RemoteMaster/Server/Ramps
                Src/StimulisPlayer
                Src/TraceRecorder
                Src/Utilities
                Src/Scheduler)
            target_compile_definitions(LinuxRemoteMasterCore PUBLIC _M_X64)
            target_compile_definitions(LinuxRemoteMasterCore PUBLIC REMOTE_MASTER)
            if(BUILD_WITH_J1939_SUPPORT)
                target_compile_definitions(LinuxRemoteMasterCore PUBLIC BUILD_WITH_J1939_SUPPORT)
            endif()
            target_compile_options(LinuxRemoteMasterCore PRIVATE -Wno-address-of-packed-member)
            target_link_libraries(LinuxRemoteMasterCore PRIVATE pthread)
            target_link_libraries(LinuxRemoteMasterCore PRIVATE rt)

            #
            # LinuxRemoteMaster.a target
            #

            add_library(LinuxRemoteMaster STATIC)
            target_include_directories(LinuxRemoteMaster
                PRIVATE
                Src/RemoteMaster/Server
                Src/Global)
            target_compile_definitions(LinuxRemoteMaster PUBLIC _M_X64)
            target_compile_definitions(LinuxRemoteMaster PUBLIC REMOTE_MASTER)

            #
            # LinuxRemoteMaster.Out target
            #

            add_executable(LinuxRemoteMaster.Out)
            target_include_directories(LinuxRemoteMaster.Out
                PRIVATE
                Src/RemoteMaster/Server
                Src/Global)
            target_compile_definitions(LinuxRemoteMaster.Out PUBLIC _M_X64)
            target_compile_definitions(LinuxRemoteMaster.Out PUBLIC REMOTE_MASTER)
            target_link_libraries(LinuxRemoteMaster.Out PRIVATE LinuxRemoteMasterCore)
            target_link_libraries(LinuxRemoteMaster.Out PRIVATE LinuxRemoteMaster)
            target_link_libraries(LinuxRemoteMaster.Out PRIVATE m)

            install(TARGETS LinuxRemoteMaster.Out LinuxRemoteMaster LinuxRemoteMasterCore
                RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/rt_linux
                LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/rt_linux
                ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/rt_linux
                PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/rt_linux
            )
        endif()
    endif()   # if(UNIX)

    add_subdirectory(Src)
    if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
        add_subdirectory(Tools)
    endif()
    add_subdirectory(Samples)

    if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
        install(TARGETS XilEnvGui XilEnv XilEnvRpc
            RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
            LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include
        )
    endif()

    install(TARGETS ${XilEnvExtProcTargetName}
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}${XilEnvExtProcLibPath}
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}${XilEnvExtProcLibPath}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}${XilEnvExtProcLibPath}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/include
    )

    # copy additional none *.h files to the include folder
    if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
        INSTALL(FILES 
            ./Src/ExternalProcess/XilEnvExtProcMain.c
            ./Src/ExternalProcess/XilEnvExtProcInterfaceMethods.c 
            ./Src/ExternalProcess/XilEnvExtProcCan.h
            DESTINATION  ${CMAKE_INSTALL_PREFIX}/include
        )
        INSTALL(FILES
            ./Userguide/OpenXiL_Userguide.pdf
            DESTINATION  ${CMAKE_INSTALL_PREFIX}
        )
    endif()

    if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
        if(QT_VERSION_MAJOR EQUAL 6)
            qt_finalize_executable(XilEnvGui)
        endif()
    endif()
endif() #  if(BUILD_ONLY_EXAMPLES)

