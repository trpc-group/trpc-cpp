
cmake_minimum_required(VERSION 3.14)

include(../cmake/common.cmake)

#---------------------------------------------------------------------------------------
# Compile project
#---------------------------------------------------------------------------------------
project(features_mysql)

# compile client
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/client/fiber/*.cc)
add_executable(fiber_client ${SRC_FILES})
target_link_libraries(fiber_client ${LIBRARY})

file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/client/future/*.cc)
add_executable(future_client ${SRC_FILES})
target_link_libraries(future_client ${LIBRARY})