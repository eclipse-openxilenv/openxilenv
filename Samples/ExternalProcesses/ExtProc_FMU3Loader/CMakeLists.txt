message("FMU3 loader byte size")
message(${MY_CMAKE_SIZEOF_VOID_P})
if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
    set(FMU3LoaderTargetName ExtProc_FMU3Loader64)
else()
    set(FMU3LoaderTargetName ExtProc_FMU3Loader32)
endif()
message("FMU3 loader name")
message(${FMU3LoaderTargetName})
if(UNIX)
    set(ExtProc_TargetName ${FMU3LoaderTargetName}.EXE)
else()
    set(ExtProc_TargetName ${FMU3LoaderTargetName})
endif()


add_executable(${ExtProc_TargetName})
target_include_directories(${ExtProc_TargetName} PRIVATE ../../../Src/ExternalProcess)
 
target_sources(${ExtProc_TargetName}
    INTERFACE FILE_SET HEADERS 
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    PRIVATE
    ExtProc_FMU3Loader.cpp
    Fmu3Execute.cpp
    Fmu3Extract.cpp
    Fmu3LoadDll.cpp 
    Fmu3Xml.cpp
    ${PUGIXML_SOURCE_PATH}/pugixml.cpp
) 
    
target_include_directories(${ExtProc_TargetName}
    PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PUGIXML_SOURCE_PATH}"
    "${FMI3_SOURCE_PATH}"
)
if(UNIX)
    target_link_libraries(${ExtProc_TargetName} PRIVATE pthread)
    target_link_libraries(${ExtProc_TargetName} PRIVATE dl)
    if(BUILD_32BIT)
        message ("Build 32 bit ${ExtProc_TargetName}")
        set_target_properties(${ExtProc_TargetName} PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
    endif()
endif()
target_link_libraries(${ExtProc_TargetName} PRIVATE ${XilEnvExtProcTargetName})

install(TARGETS ${ExtProc_TargetName}
    #BUNDLE DESTINATION .
    #LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
)
