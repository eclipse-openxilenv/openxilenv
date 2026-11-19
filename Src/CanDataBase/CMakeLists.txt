set(CommonFileList
#    AzgEdicCfg.c
    CanDataBase.c
    CanRecorder.c
    ImportDbc.c
    ReadCanCfg.c
)

target_sources(XilEnv PRIVATE ${CommonFileList})
target_sources_if_target_exist(XilEnvGui "${CommonFileList}")
