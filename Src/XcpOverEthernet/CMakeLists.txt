
set(CommonFileList
    XcpConnector.cpp
    XcpCopyPages.cpp
    XcpOverEthernet.cpp
    XcpWrapper.cpp
)

target_sources(XilEnv PRIVATE ${CommonFileList})
target_sources_if_target_exist(XilEnvGui "${CommonFileList}")
