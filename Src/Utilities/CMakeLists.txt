if(UNIX)
    target_sources(LinuxRemoteMasterCore
        PRIVATE 
        StringMaxChar.c
        PrintFormatToString.c
        ConfigurablePrefix.c)
endif()

target_sources(${XilEnvExtProcTargetName} PRIVATE
    PrintFormatToString.c
    StringMaxChar.c)


if(NOT BUILD_32BIT)

    target_sources(XilEnvRpc PRIVATE
        StringMaxChar.c
        PrintFormatToString.c)

    set(CommonFileList
        GetNotANumber.c
        StringMaxChar.c
        PrintFormatToString.c
        ConfigurablePrefix.c
    )

    target_sources(XilEnv  PRIVATE ${CommonFileList})
    target_sources_if_target_exist(XilEnvGui "${CommonFileList}")
endif()
