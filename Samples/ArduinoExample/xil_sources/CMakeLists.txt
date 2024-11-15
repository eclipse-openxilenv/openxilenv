add_executable(${BINARY} arduino_process.cpp)
add_executable(${BINARY}_Model arduino_model.cpp)

set(XIL_LIB_PATH "C:/Program Files (x86)/XilEnvGui")

include_directories(arduinoInclude PRIVATE ${CMAKE_SOURCE_DIR}/driverSimulation/include)
include_directories(xilEnvInclude PRIVATE ${XIL_LIB_PATH}/include)

add_library(pinSimulation ${CMAKE_SOURCE_DIR}/driverSimulation/src/pin.c)

target_link_libraries(${BINARY} pinSimulation)
target_link_libraries(${BINARY}_Model pinSimulation)