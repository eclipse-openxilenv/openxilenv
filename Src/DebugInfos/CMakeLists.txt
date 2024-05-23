set(CommonFileList
    DelayedFetchData.c
    ExtProcessReferences.c
    GetNextStructEntry.c
    ReadWriteValue.c
    DebugInfoAccessExpression.c
    DwarfReader.c
    GetExeDebugSign.c
    LoadSaveToFile.c
    SectionFilter.c
    DebugInfoDB.c
    GetLabelByAddress.c
    ReadExeInfos.c
)

target_sources(XilEnv PRIVATE ${CommonFileList})

target_sources(XilEnvGui PRIVATE ${CommonFileList})
