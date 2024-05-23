if(UNIX) 
    target_sources(${XilEnvExtProcTargetName}
        PRIVATE Platform.c)
    if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
        target_sources(LinuxRemoteMasterCore 
        PRIVATE Compare2DoubleEqual.c Fifos.c VersionInfoSection.c) 

        target_sources(LinuxRemoteMaster PRIVATE 
            VersionInfoSection.c) 
    
        target_sources(LinuxRemoteMaster.Out PRIVATE 
            VersionInfoSection.c) 
    endif()
endif()

if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
    set(CmmonFileList
        CheckIfAlreadyRunning.c
        RunTimeMeasurement.c
        InitProcess.c
        ParseCommandLine.c
        StartExeAndWait.c
        Compare2DoubleEqual.c
        MainValues.c
        Platform.c
        StartupInit.c
        UtilsWindow.c
        EnvironmentVariables.c
        ImExportDskFile.c
        Message.c
        ReadConfig.c
        ThrowError.c
        Fifos.c
        ImExportVarProperties.c
        MyMemory.c
        ReplaceFuncWithProg.c
        TimeProcess.c
        Wildcards.c
        Files.c
        IniFileDontExist.c
        my_udiv128.c
        UniqueNumber.c
        WindowIniHelper.c)

    target_sources(XilEnv PRIVATE ${CmmonFileList})

    target_sources(XilEnvGui PRIVATE ${CmmonFileList})
endif()
