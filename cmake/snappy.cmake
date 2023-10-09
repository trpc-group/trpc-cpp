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

if(NOT DEFINED SNAPPY_VER)
    set(SNAPPY_VER 1.1.8)
endif()
set(SNAPPY_URL https://github.com/google/snappy/archive/refs/tags/${SNAPPY_VER}.tar.gz)

FetchContent_Declare(
    com_github_google_snappy
    URL               ${SNAPPY_URL}
    SOURCE_DIR        ${TRPC_ROOT_PATH}/cmake_third_party/snappy
)

FetchContent_GetProperties(com_github_google_snappy)
if(NOT com_github_google_snappy_POPULATED)
    FetchContent_Populate(com_github_google_snappy)

    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(SNAPPY_BUILD_TESTS OFF)

    add_subdirectory(${TRPC_ROOT_PATH}/cmake_third_party/snappy)

    add_library(trpc_snappy ALIAS snappy)

    set(TARGET_INCLUDE_PATHS    ${TARGET_INCLUDE_PATHS}
                                ${TRPC_ROOT_PATH}/cmake_third_party/snappy
                                ${CMAKE_BINARY_DIR}/trpc-cpp/cmake_third_party/snappy)
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} trpc_snappy)
endif()
