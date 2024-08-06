if(UNIX) 
target_sources(LinuxRemoteMasterCore 
    PRIVATE
    CanFifo.c
    CanReplaceConversion.c
    CanServer.c
    CcpAndXcpFilterHook.c
)
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

target_sources(XilEnv PRIVATE ${CommonFileList})

target_sources(XilEnvGui PRIVATE ${CommonFileList})

if (BUILD_WITH_J1939_SUPPORT)
    add_subdirectory("J1939")
endif()
