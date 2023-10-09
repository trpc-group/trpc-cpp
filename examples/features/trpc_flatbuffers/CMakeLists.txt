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

include(../cmake/common.cmake)

set(FBS_CPP_PLUGIN ${TRPC_ROOT_PATH}/build/bin/fbs_cpp_plugin)

#---------------------------------------------------------------------------------------
# Compile project
#---------------------------------------------------------------------------------------
project(features_fiber_forward)

# compile proto
set(SERVER_FBS_SRC ${TRPC_ROOT_PATH}/examples/features/trpc_flatbuffers/server/greeter.fbs)
set(PROXY_FBS_SRC ${TRPC_ROOT_PATH}/examples/features/trpc_flatbuffers/proxy/greeter.fbs
                  ${TRPC_ROOT_PATH}/examples/features/trpc_flatbuffers/proxy/forward.fbs)

TRPC_COMPILE_FBS(OUT_TRPC_FBS_SERVER_SRCS "${SERVER_FBS_SRC}" ${FBS_CPP_PLUGIN} ${TRPC_ROOT_PATH})
TRPC_COMPILE_FBS(OUT_TRPC_FBS_PROXY_SRCS "${PROXY_FBS_SRC}" ${FBS_CPP_PLUGIN} ${TRPC_ROOT_PATH})

# compile server
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/server/*.cc)
add_executable(demoserver ${SRC_FILES} ${OUT_TRPC_FBS_SERVER_SRCS})
# include current direcotry to include fbs stub code deps
target_include_directories(demoserver PUBLIC ${TRPC_ROOT_PATH}/examples/features/trpc_flatbuffers/server)
target_link_libraries(demoserver ${LIBRARY})

# compile forward server
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proxy/*.cc)
add_executable(forwardserver ${SRC_FILES} ${OUT_TRPC_FBS_PROXY_SRCS})
# include current direcotry to include fbs stub code deps
target_include_directories(forwardserver PUBLIC ${TRPC_ROOT_PATH}/examples/features/trpc_flatbuffers/proxy)
target_link_libraries(forwardserver ${LIBRARY})

# compile client
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/client/*.cc)
add_executable(client ${SRC_FILES} ${OUT_TRPC_FBS_PROXY_SRCS})
# include current direcotry to include fbs stub code deps
target_include_directories(client PUBLIC ${TRPC_ROOT_PATH}/examples/features/trpc_flatbuffers/proxy)
target_link_libraries(client ${LIBRARY})
