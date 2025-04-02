cmake_minimum_required(VERSION 3.10)

project(arduino_SIL LANGUAGES C CXX)

 
option(BUILD_SIL_SOURCES OFF)


if(BUILD_SIL_SOURCES)
    message("Building sources for SIL environment ...")

    set(BINARY ${CMAKE_PROJECT_NAME})
    set(SIL_ENV_DIR ${CMAKE_SOURCE_DIR})

    add_subdirectory(${CMAKE_SOURCE_DIR}/xil_sources)

else()
    set(ARDUINO_BOARD "arduino:avr:nano")
    set(SERIAL_PORT "COM7")
    
    set(SKETCH_PATH "${CMAKE_SOURCE_DIR}/arduino_example.ino")


    add_custom_target(compile_sketch
    COMMAND arduino-cli compile --fqbn ${ARDUINO_BOARD} ${SKETCH_PATH}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Compiling Arduino Sketch"
    )

    add_custom_target(upload_sketch
    COMMAND arduino-cli upload -p ${SERIAL_PORT} --fqbn ${ARDUINO_BOARD} ${SKETCH_PATH}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS compile_sketch
    COMMENT "Uploading Arduino Sketch"
    )

    add_custom_target(build_and_upload ALL
    DEPENDS upload_sketch
    COMMENT "Building and uploading Arduino project"
    )


endif()

 
