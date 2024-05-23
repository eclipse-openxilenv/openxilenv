
set(CommonFileList
    XcpConnector.cpp
    XcpCopyPages.cpp
    XcpOverEthernet.cpp
    XcpWrapper.cpp
)

target_sources(XilEnv  PRIVATE ${CommonFileList})

target_sources(XilEnvGui PRIVATE ${CommonFileList})
