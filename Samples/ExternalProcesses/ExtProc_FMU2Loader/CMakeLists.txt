message("FMU2 loader byte size")
message(${MY_CMAKE_SIZEOF_VOID_P})
if(${MY_CMAKE_SIZEOF_VOID_P} GREATER 4)
    set(FMU2LoaderTargetName ExtProc_FMU2Loader64)
    set(FMU2LoaderTargetPath "")
else()
    set(FMU2LoaderTargetName ExtProc_FMU2Loader32)
    set(FMU2LoaderTargetPath "/Bin32")
endif()
message("FMU2 loader name")
message(${FMU2LoaderTargetName})
if(UNIX)
    set(ExtProc_TargetName ${FMU2LoaderTargetName}.EXE)
else()
    set(ExtProc_TargetName ${FMU2LoaderTargetName})
endif()


add_executable(${ExtProc_TargetName})
target_include_directories(${ExtProc_TargetName} PRIVATE ../../../Src/ExternalProcess)
 
target_sources(${ExtProc_TargetName}
    INTERFACE FILE_SET HEADERS 
    BASE_DIRS ${PROJECT_SOURCE_DIR}
    PRIVATE
    ExtProc_FMU2Loader.cpp
    Fmu2Execute.cpp
    Fmu2Extract.cpp
    Fmu2LoadDll.cpp 
    Fmu2Xml.cpp
    ${PUGIXML_SOURCE_PATH}/pugixml.cpp
) 
    
target_include_directories(${ExtProc_TargetName}
    PRIVATE
    "${PROJECT_SOURCE_DIR}"
    "${PUGIXML_SOURCE_PATH}"
    "${FMI2_SOURCE_PATH}"
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
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}${FMU2LoaderTargetPath}
)
