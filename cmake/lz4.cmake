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

include(FetchContent)

if(NOT DEFINED LZ4_VER)
    set(LZ4_VER 1.9.4)
endif()
set(LZ4_URL https://github.com/lz4/lz4/archive/refs/tags/v${LZ4_VER}.tar.gz)

FetchContent_Declare(
    lz4
    URL               ${LZ4_URL}
    SOURCE_DIR        ${TRPC_ROOT_PATH}/cmake_third_party/lz4
)

FetchContent_GetProperties(lz4)
if(NOT lz4_POPULATED)
    FetchContent_Populate(lz4)

    set(LZ4_BUILD_CLI OFF)
    set(LZ4_BUILD_LEGACY_LZ4C OFF)
    if(TRPC_BUILD_SHARED)
        set(LZ4_BUNDLED_MODE OFF)
    endif()

    add_subdirectory(${TRPC_ROOT_PATH}/cmake_third_party/lz4/build/cmake)
    
    if(TRPC_BUILD_SHARED)
        add_library(trpc_lz4 ALIAS lz4_shared)
    else()
        add_library(trpc_lz4 ALIAS lz4_static)
    endif()

    set(TARGET_INCLUDE_PATHS    ${TARGET_INCLUDE_PATHS}
                                ${TRPC_ROOT_PATH}/cmake_third_party/lz4/lib)
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} trpc_lz4)
endif()
