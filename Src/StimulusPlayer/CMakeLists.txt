if(UNIX) 
target_sources(LinuxRemoteMasterCore PRIVATE 
    WriteToBlackboardPipe.c) 
endif()

set(CommonFileList
    StimulusPlayer.c
    StimulusReadDatFile.c
    StimulusReadFile.c
    StimulusReadMdfFile.c
    WriteToBlackboardPipe.c
)

target_sources(XilEnv  PRIVATE ${CommonFileList})

target_sources(XilEnvGui PRIVATE ${CommonFileList})

