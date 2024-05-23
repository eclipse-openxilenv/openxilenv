if(UNIX)
    set(ExtProc_TargetName ExtProc_FMUExtract.EXE)
else()
    set(ExtProc_TargetName ExtProc_FMUExtract)
endif()

add_executable(${ExtProc_TargetName})
target_include_directories(${ExtProc_TargetName} PRIVATE ../../../Src/ExternalProcess)

if(BUILD_WITH_FMU2_SUPPORT)
    add_definitions(-DBUILD_WITH_FMU2_SUPPORT)
#    add_compile_definitions(${ExtProc_TargetName} PRIVATE -DBUILD_WITH_FMU2_SUPPORT)
endif()
if(BUILD_WITH_FMU3_SUPPORT)
    add_definitions(-DBUILD_WITH_FMU3_SUPPORT)
#    add_compile_definitions(${ExtProc_TargetName} PRIVATE -DBUILD_WITH_FMU3_SUPPORT)
endif()

target_sources(${ExtProc_TargetName}
    INTERFACE FILE_SET HEADERS 
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    PRIVATE FmuExtract.cpp) 
    
target_include_directories(${ExtProc_TargetName} PRIVATE "${PROJECT_SOURCE_DIR}")

install(TARGETS ${ExtProc_TargetName}
    #BUNDLE DESTINATION .
    #LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
)

