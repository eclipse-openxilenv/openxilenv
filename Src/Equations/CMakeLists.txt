set(CommonFileList
    EquationsSingle.c
    EquationsSplitCompiler.c
)

target_sources(XilEnv  PRIVATE ${CommonFileList})
target_sources_if_target_exist(XilEnvGui "${CommonFileList}")
