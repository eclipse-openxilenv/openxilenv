set(CommonFileList
    DeleteWindowFromIni.c
    IniDataBase.c
)

target_sources(XilEnv  PRIVATE ${CommonFileList})

target_sources(XilEnvGui PRIVATE ${CommonFileList})
