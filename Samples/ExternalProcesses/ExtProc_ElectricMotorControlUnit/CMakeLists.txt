if(UNIX)
    set(ExtProc_TargetName ExtProc_ElectricMotorControlUnit.EXE)
else()
    set(ExtProc_TargetName ExtProc_ElectricMotorControlUnit)
endif()
add_executable(${ExtProc_TargetName})
target_include_directories(${ExtProc_TargetName} PRIVATE ../../../Src/ExternalProcess)
 
target_sources(${ExtProc_TargetName}
    INTERFACE FILE_SET HEADERS 
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    PRIVATE ExtProc_ElectricMotorControlUnit.c)
target_include_directories(${ExtProc_TargetName} PRIVATE "${PROJECT_SOURCE_DIR}")
if(UNIX)
    target_link_libraries(${ExtProc_TargetName} PRIVATE pthread)
    target_link_libraries(${ExtProc_TargetName} PRIVATE dl)
    if(BUILD_32BIT)
        message ("Build 32 bit ${ExtProc_TargetName}")
        set_target_properties(${ExtProc_TargetName} PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
    endif()
endif()
#this would be dynamic loaded
#target_link_libraries(${ExtProc_TargetName} PRIVATE ${XilEnvExtProcTargetName})

install(TARGETS ${ExtProc_TargetName}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/${SampleOutputFolder}/ExtProc_ElectricMotorControlUnit
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    install(FILES $<TARGET_PDB_FILE:${ExtProc_TargetName}> DESTINATION ${CMAKE_INSTALL_PREFIX}/${SampleOutputFolder}/ExtProc_ElectricMotorControlUnit OPTIONAL)
endif()
