if(UNIX)
    set(ExtProc_TargetName ExtProc_OpenScenario.EXE)
else()
    set(ExtProc_TargetName ExtProc_OpenScenario)
endif()
add_executable(${ExtProc_TargetName})
target_include_directories(${ExtProc_TargetName} PRIVATE ../../../Src/ExternalProcess)
 
target_sources(${ExtProc_TargetName}
    INTERFACE FILE_SET HEADERS 
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    PRIVATE ExtProc_Esmini.cpp) 
    
target_include_directories(${ExtProc_TargetName}
    PRIVATE 
    "${PROJECT_SOURCE_DIR}"
    ${ESMINI_LIBRARY_PATH}/EnvironmentSimulator/Libraries/esminiLib
)    
target_link_libraries(${ExtProc_TargetName} PRIVATE ${XilEnvExtProcTargetName})
if(WIN32)
    target_link_libraries(${ExtProc_TargetName} PRIVATE ${ESMINI_LIBRARY_PATH}/bin/esminiLib.lib)
else()
    target_link_libraries(${ExtProc_TargetName} PRIVATE ${ESMINI_LIBRARY_PATH}/bin/libesminiLib.so)
endif()

install(TARGETS ${ExtProc_TargetName}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${SampleOutputFolder}/ExtProc_OpenScenario
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
if(WIN32)
    INSTALL(DIRECTORY "${ESMINI_LIBRARY_PATH}/bin/"
            DESTINATION ${CMAKE_INSTALL_PREFIX}/${SampleOutputFolder}/ExtProc_OpenScenario
            FILES_MATCHING PATTERN "*.dll")
 endif()

