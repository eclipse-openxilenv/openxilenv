add_executable(RemoteStartServer)
target_link_libraries(RemoteStartServer PRIVATE pthread)

target_sources(RemoteStartServer
    PRIVATE RemoteStartServer.c) 

if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
    install(TARGETS RemoteStartServer
        RUNTIME DESTINATION rt_linux
    )
endif()

