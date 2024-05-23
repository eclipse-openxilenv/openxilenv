add_executable(LinuxRemoteMasterEasySample)
#target_compile_options(LinuxRemoteMasterEasySample PRIVATE -gdwarf-5)
#target_link_options(LinuxRemoteMasterEasySample PRIVATE -gdwarf-5)
target_include_directories(LinuxRemoteMasterEasySample PRIVATE ../../../Src/ExternalProcess)
 
target_sources(LinuxRemoteMasterEasySample
    INTERFACE FILE_SET HEADERS 
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    PRIVATE LinuxRemoteMasterEasySample.c)
 
target_include_directories(LinuxRemoteMasterEasySample PRIVATE "${PROJECT_SOURCE_DIR}")    

message("Library directory:")
message("${CMAKE_BINARY_DIR}")
target_link_directories(LinuxRemoteMasterEasySample PRIVATE "${CMAKE_BINARY_DIR}")    
target_link_libraries(LinuxRemoteMasterEasySample PRIVATE LinuxRemoteMasterCore)
target_link_libraries(LinuxRemoteMasterEasySample PRIVATE LinuxRemoteMaster)
target_link_libraries(LinuxRemoteMasterEasySample PRIVATE m)
