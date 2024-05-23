set(CommonFileList
    EquationsSingle.c
    EquationsSplitCompiler.c
)

target_sources(XilEnv  PRIVATE ${CommonFileList})

target_sources(XilEnvGui PRIVATE ${CommonFileList})
