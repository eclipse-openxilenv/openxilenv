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


if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)

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

    target_sources(XilEnvGui PRIVATE ${CommonFileList})
endif()
