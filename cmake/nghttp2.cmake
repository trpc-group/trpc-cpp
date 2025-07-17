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

set(NGHTTP2_VER     1.40.0)
set(NGHTTP2_GIT_URL https://github.com/nghttp2/nghttp2/releases/download/v${NGHTTP2_VER}/nghttp2-${NGHTTP2_VER}.tar.gz)

FetchContent_Declare(
        nghttp2
        URL                 ${NGHTTP2_GIT_URL}
        SOURCE_DIR          ${TRPC_ROOT_PATH}/cmake_third_party/nghttp2
)
FetchContent_GetProperties(nghttp2)
if(NOT nghttp2_POPULATED)
    FetchContent_Populate(nghttp2)

    set(ENABLE_LIB_ONLY ON)
    if (TRPC_BUILD_SHARED)
        set(ENABLE_STATIC_LIB OFF)
        set(ENABLE_SHARED_LIB ON)
    else()
        set(ENABLE_STATIC_LIB ON)
        set(ENABLE_SHARED_LIB OFF)
    endif()

    add_subdirectory(${nghttp2_SOURCE_DIR})

    if (TRPC_BUILD_SHARED)
        add_library(trpc_nghttp2 ALIAS nghttp2)
    else()
        add_library(trpc_nghttp2 ALIAS nghttp2_static)
    endif()

    set(TARGET_INCLUDE_PATHS    ${TARGET_INCLUDE_PATHS}
                                ${TRPC_ROOT_PATH}/cmake_third_party/nghttp2/lib/includes)
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} trpc_nghttp2)
endif()
