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

set(GTEST_VET  1.11.0)
set(GTEST_URL  https://github.com/google/googletest/archive/release-${GTEST_VET}.tar.gz)

FetchContent_Declare(
    com_google_googletest
    URL             ${GTEST_URL}
    SOURCE_DIR      ${TRPC_ROOT_PATH}/cmake_third_party/gtest
)

FetchContent_GetProperties(com_google_googletest)
if(NOT com_google_googletest_POPULATED)
    FetchContent_Populate(com_google_googletest)
    add_subdirectory(${TRPC_ROOT_PATH}/cmake_third_party/gtest)

    set_target_properties(
        gmock PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${com_google_googletest_SOURCE_DIR}/googlemock/include;>"
    )

    set_target_properties(
        gtest PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${com_google_googletest_SOURCE_DIR}/googletest/include;>"
    )
    add_library(trpc_gmock ALIAS gmock)
    add_library(trpc_gtest ALIAS gtest)

    set(TARGET_INCLUDE_PATHS    ${TARGET_INCLUDE_PATHS}
                                ${com_google_googletest_SOURCE_DIR}/googlemock/include
                                ${com_google_googletest_SOURCE_DIR}/googletest/include)
    set(TARGET_LINK_LIBS  ${TARGET_LINK_LIBS} trpc_gmock trpc_gtest)
endif()
