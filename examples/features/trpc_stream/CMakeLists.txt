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
project(features_trpc_stream)

# compile proto
set(PB_SRC ${TRPC_ROOT_PATH}/examples/features/trpc_stream/server/stream.proto)

COMPILE_PROTO(OUT_PB_SRCS "${PB_SRC}" ${PB_PROTOC} ${TRPC_ROOT_PATH})

TRPC_COMPILE_PROTO(OUT_TRPC_PB_SRCS "${PB_SRC}" ${PB_PROTOC} ${TRPC_CPP_PLUGIN} ${TRPC_ROOT_PATH})

# compile server
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/server/*.cc)
add_executable(trpc_stream_server ${SRC_FILES} ${OUT_PB_SRCS} ${OUT_TRPC_PB_SRCS})
target_link_libraries(trpc_stream_server ${LIBRARY})

# compile client
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/client/client.cc)
add_executable(client ${SRC_FILES} ${OUT_PB_SRCS} ${OUT_TRPC_PB_SRCS})
target_link_libraries(client ${LIBRARY})

file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/client/rawdata_stream_client.cc)
add_executable(rawdata_stream_client ${SRC_FILES} ${OUT_PB_SRCS} ${OUT_TRPC_PB_SRCS})
target_link_libraries(rawdata_stream_client ${LIBRARY})
