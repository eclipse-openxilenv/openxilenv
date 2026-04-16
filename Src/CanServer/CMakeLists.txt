if(UNIX) 
target_sources(LinuxRemoteMasterCore 
    PRIVATE
    CanFifo.c
    CanReplaceConversion.c
    CanServer.c
    CcpAndXcpFilterHook.c
)
if (BUILD_WITH_J1939_SUPPORT)
target_sources(LinuxRemoteMasterCore
    PRIVATE
    J1939MultiPackage.c
)
endif()
endif()

set(CommonFileList
    CanFifo.c
    CanReplaceConversion.c
    CanServer.c
    CcpAndXcpFilterHook.c
    CcpControl.c
    VirtualCanDriver.c
    XcpControl.c
)

if(BUILD_WITH_GATEWAY_VIRTUAL_CAN)
    set(CommonFileList
        ${CommonFileList}
        GatewayCanDriver.c
    )
endif()

if (BUILD_WITH_J1939_SUPPORT)
set(CommonFileList
    ${CommonFileList}
    J1939MultiPackage.c
)
endif()

target_sources(XilEnv PRIVATE ${CommonFileList})

target_sources(XilEnvGui PRIVATE ${CommonFileList})

