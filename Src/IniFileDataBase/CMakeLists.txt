set(CommonFileList
    DeleteWindowFromIni.c
    IniDataBase.c
    IniDataBaseInOutputFilter.c
)

target_sources(XilEnv  PRIVATE ${CommonFileList})
target_sources_if_target_exist(XilEnvGui "${CommonFileList}")
