#
#
# Tencent is pleased to support the open source community by making tRPC available.
#
# Copyright (C) 2023 Tencent.
# All rights reserved.
#
# If you have downloaded a copy of the tRPC source code from Tencent,
# please note that tRPC source code is licensed under the  Apache 2.0 License,
# A copy of the Apache 2.0 License is included in this file.
#
#

cmake_minimum_required(VERSION 3.14)

include(../cmake/common.cmake)

#---------------------------------------------------------------------------------------
# Compile project
#---------------------------------------------------------------------------------------
project(features_http_rpc)

# compile proto
set(PB_SRC ${TRPC_ROOT_PATH}/examples/helloworld/helloworld.proto)

COMPILE_PROTO(OUT_PB_SRCS "${PB_SRC}" ${PB_PROTOC} ${TRPC_ROOT_PATH})

TRPC_COMPILE_PROTO(OUT_TRPC_PB_SRCS "${PB_SRC}" ${PB_PROTOC} ${TRPC_CPP_PLUGIN} ${TRPC_ROOT_PATH})

# compile server
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/server/*.cc
                    ${TRPC_ROOT_PATH}/examples/helloworld/greeter_service.cc)
add_executable(http_rpc_server ${SRC_FILES} ${OUT_PB_SRCS} ${OUT_TRPC_PB_SRCS})
target_link_libraries(http_rpc_server ${LIBRARY})

# compile client
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/client/*.cc)
add_executable(client ${SRC_FILES} ${SRC_FILES} ${OUT_PB_SRCS} ${OUT_TRPC_PB_SRCS})
target_link_libraries(client ${LIBRARY})
