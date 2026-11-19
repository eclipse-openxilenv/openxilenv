if(UNIX) 
    target_sources(${XilEnvExtProcTargetName}
        PRIVATE Platform.c)
    if(NOT BUILD_32BIT)
        target_sources(LinuxRemoteMasterCore PRIVATE
            Platform.c
            Compare2DoubleEqual.c
            Fifos.c
            VersionInfoSection.c)

        target_sources(LinuxRemoteMaster PRIVATE 
            VersionInfoSection.c) 
    
        target_sources(LinuxRemoteMaster.Out PRIVATE 
            VersionInfoSection.c) 

        target_sources(XilEnvRpc PRIVATE
            Platform.c)
    endif()
endif()

if(NOT BUILD_32BIT)

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
    target_sources_if_target_exist(XilEnvGui "${CmmonFileList}")
endif()
