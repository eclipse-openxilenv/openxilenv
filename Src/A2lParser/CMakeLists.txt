set(CommonFileList
    A2LAccess.c
    A2LBuffer.c
    A2LConvert.c
    A2LLink.c
    A2LParser.c
    A2LRecordLayout.c
    A2LTokenizer.c
    A2LValue.c
    A2LAccessData.c
    A2LCache.c
    A2LConvertToXcp.c
    A2LLinkThread.c
    A2LParserModuleIfData.c
    A2LUpdate.c
    S19BinFile.c
)

target_sources(XilEnv PRIVATE ${CommonFileList})

target_sources(XilEnvGui  PRIVATE ${CommonFileList})
