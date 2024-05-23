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

target_sources(XilEnv PRIVATE ${CommonFileList})

target_sources(XilEnvGui PRIVATE ${CommonFileList})

    
