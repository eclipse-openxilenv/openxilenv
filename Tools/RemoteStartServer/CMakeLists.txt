add_executable(RemoteStartServer)
target_link_libraries(RemoteStartServer PRIVATE pthread)

target_sources(RemoteStartServer
    PRIVATE RemoteStartServer.c) 

if(NOT BUILD_32BIT)
    install(TARGETS RemoteStartServer
        RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/rt_linux
    )
endif()

