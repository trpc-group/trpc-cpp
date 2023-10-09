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

if(NOT DEFINED TOML11_GIT_TAG)
    set(TOML11_GIT_TAG v3.7.0)
endif()

set(TOML11_GIT_URL https://github.com/ToruNiina/toml11/archive/${TOML11_GIT_TAG}.tar.gz)

FetchContent_Declare(
    toml11
    URL            ${TOML11_GIT_URL}
    SOURCE_DIR     ${TRPC_ROOT_PATH}/cmake_third_party/toml11
)
FetchContent_GetProperties(toml11)

if(NOT toml11_POPULATED)
    FetchContent_Populate(toml11)
    add_subdirectory(${TRPC_ROOT_PATH}/cmake_third_party/toml11)
    add_library(trpc_toml11 ALIAS toml11)
    set(TARGET_INCLUDE_PATHS    ${TARGET_INCLUDE_PATHS}
                                ${TRPC_ROOT_PATH}/cmake_third_party/toml11)
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} trpc_toml11)
endif()
