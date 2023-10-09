#
#
# Tencent is pleased to support the open source community by making tRPC available.
#
# Copyright (C) 2023 THL A29 Limited, a Tencent company.
# All rights reserved.
#
# If you have downloaded a copy of the tRPC source code from Tencent,
# please note that tRPC source code is licensed under the  Apache 2.0 License,
# A copy of the Apache 2.0 License is included in this file.
#
#

cmake_minimum_required(VERSION 3.14)

#---------------------------------------------------------------------------------------
# Necessary compile env setting
#---------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)

# (1) 'TRPC_ROOT_PATH' is the src director of trpc-cpp. You must specify it correctly.
set(TRPC_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../)

# (2) load config(hdrs/libs paths) and utils(tools for generating stub code)
include(${TRPC_ROOT_PATH}/cmake/config/trpc_config.cmake)
include(${TRPC_ROOT_PATH}/cmake/tools/trpc_utils.cmake)

# (3) add hdrs and libs paths to cmake build system
include_directories(${INCLUDE_PATHS})
link_directories(${LIBRARY_PATHS})

# (4) set current project dependency libs(trpc and other libs(protobuf/yaml-cpp/pthread/...etc))
set(LIBRARY trpc ${LIBS_BASIC})
# to use LIB_METRICS_PROMETHEUS, including prometheus libs
# set(LIBRARY trpc ${LIBS_BASIC} ${LIB_METRICS_PROMETHEUS})

#---------------------------------------------------------------------------------------
# Compile project
#---------------------------------------------------------------------------------------
project(helloworld)

# Set the paths of protoc and trpc_cpp_plugin which will be generated at ${TRPC_ROOT_PATH}/build/bin
set(PB_PROTOC ${TRPC_ROOT_PATH}/build/bin/protoc)
set(TRPC_CPP_PLUGIN ${TRPC_ROOT_PATH}/build/bin/trpc_cpp_plugin)

# Set proto file path
set(PB_SRC ${TRPC_ROOT_PATH}/examples/helloworld/helloworld.proto)

# @brief                            Generate corresponding .pb.cc and .pb.h stub code based on proto files
# @param OUT_PB_SRCS_FILES          [Output]The generated .pb.cc files from proto files
# @param GEN_PB_PROTO_FILES         [Input]The collection of proto files
# @param PROTOBUF_PROTOC_EXECUTABLE [Input]Protoc executable program generated during the compilation process in the build/bin directory
# @param CMAKESOURCEDIR             [Input]The directory where the CMakeLists.txt of the project is located
COMPILE_PROTO(OUT_PB_SRCS "${PB_SRC}" ${PB_PROTOC} ${TRPC_ROOT_PATH})

# @brief                             Generate corresponding .trpc.pb.cc and .trpc.pb.h stub code based on proto files
# @param OUT_TRPC_PB_SRCS_FILES      [Output]The generated .trpc.pb.cc files from proto files
# @param GEN_TRPC_PB_PROTO_FILES     [Input]The collection of proto files
# @param PROTOBUF_PROTOC_EXECUTABLE  [Input]Protoc executable program generated during the compilation process in the build/bin directory
# @param TRPC_TO_CPP_PLUGIN          [Input]trpc_cpp_plugin executable program generated during the compilation process in the build/bin directory
# @param CMAKESOURCEDIR              [Input]The directory where the CMakeLists.txt of the project is located
TRPC_COMPILE_PROTO(OUT_TRPC_PB_SRCS "${PB_SRC}" ${PB_PROTOC} ${TRPC_CPP_PLUGIN} ${TRPC_ROOT_PATH})

# compile server
add_executable(helloworld_svr ${CMAKE_CURRENT_SOURCE_DIR}/helloworld_server.cc
                              ${CMAKE_CURRENT_SOURCE_DIR}/greeter_service.cc 
                              ${OUT_PB_SRCS}
                              ${OUT_TRPC_PB_SRCS})
target_link_libraries(helloworld_svr ${LIBRARY})

# compile client
add_executable(fiber_client ${CMAKE_CURRENT_SOURCE_DIR}/test/fiber_client.cc
                            ${OUT_PB_SRCS}
                            ${OUT_TRPC_PB_SRCS})
target_link_libraries(fiber_client ${LIBRARY})

add_executable(future_client  ${CMAKE_CURRENT_SOURCE_DIR}/test/future_client.cc
                              ${OUT_PB_SRCS}
                              ${OUT_TRPC_PB_SRCS})
target_link_libraries(future_client ${LIBRARY})

# compile test 
add_executable(greeter_service_test ${CMAKE_CURRENT_SOURCE_DIR}/greeter_service_test.cc
                                    ${CMAKE_CURRENT_SOURCE_DIR}/greeter_service.cc 
                                    ${OUT_PB_SRCS}
                                    ${OUT_TRPC_PB_SRCS})
# ${LIB_GTEST_GMOCK} must be linked before ${LIBRARY}
target_link_libraries(greeter_service_test ${LIB_GTEST_GMOCK} ${LIBRARY})
enable_testing()
add_test(NAME greeter_service_test
         COMMAND greeter_service_test)
