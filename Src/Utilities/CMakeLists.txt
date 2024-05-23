if(UNIX)
    target_sources(LinuxRemoteMasterCore
        PRIVATE 
        StringMaxChar.c
        ConfigurablePrefix.c)
endif()

target_sources(${XilEnvExtProcTargetName}
    PRIVATE StringMaxChar.c)

set(CommonFileList
    GetNotANumber.c
    StringMaxChar.c
    ConfigurablePrefix.c
)

target_sources(XilEnv  PRIVATE ${CommonFileList})

target_sources(XilEnvGui PRIVATE ${CommonFileList})

