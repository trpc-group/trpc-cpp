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

include(FetchContent)

set(PROMETHEUS_VER v1.1.0)
set(PROMETHEUS_URL https://github.com/jupp0r/prometheus-cpp/releases/download/${PROMETHEUS_VER}/prometheus-cpp-with-submodules.tar.gz)

FetchContent_Declare(
    prometheus
    URL               ${PROMETHEUS_URL}
    SOURCE_DIR        ${TRPC_ROOT_PATH}/cmake_third_party/prometheus
)
FetchContent_GetProperties(prometheus)
if(NOT prometheus_POPULATED)
    FetchContent_Populate(prometheus)

    set(ENABLE_TESTING OFF)
    add_definitions(-DHAVE_ZLIB)

    add_subdirectory(${TRPC_ROOT_PATH}/cmake_third_party/prometheus)

    set_target_properties(
        pull  PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${prometheus_SOURCE_DIR}/pull/include;\
${CMAKE_SOURCE_DIR}/build/_deps/prometheus-build/pull/include;\
${TRPC_BUILD_BINARY_PATH}/cmake_third_party/prometheus/pull/include>"
    )
    
    set_target_properties(
        core  PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${prometheus_SOURCE_DIR}/core/include;\
${CMAKE_SOURCE_DIR}/build/_deps/prometheus-build/core/include;\
${TRPC_BUILD_BINARY_PATH}/cmake_third_party/prometheus/core/include>"
    )
    
    add_library(trpc_prometheus_pull ALIAS pull)
    add_library(trpc_prometheus_core ALIAS core)
    
    set(TARGET_INCLUDE_PATHS    ${TARGET_INCLUDE_PATHS}
                                ${prometheus_SOURCE_DIR}/pull/include
                                ${TRPC_BUILD_BINARY_PATH}/cmake_third_party/prometheus/pull/include
                                ${CMAKE_SOURCE_DIR}/build/_deps/prometheus-build/pull/include
                                ${prometheus_SOURCE_DIR}/push/include
                                ${TRPC_BUILD_BINARY_PATH}/cmake_third_party/prometheus/push/include
                                ${CMAKE_SOURCE_DIR}/build/_deps/prometheus-build/push/include
                                ${prometheus_SOURCE_DIR}/core/include
                                ${TRPC_BUILD_BINARY_PATH}/cmake_third_party/prometheus/core/include
                                ${CMAKE_SOURCE_DIR}/build/_deps/prometheus-build/core/include)
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} trpc_prometheus_pull trpc_prometheus_core)
endif()
