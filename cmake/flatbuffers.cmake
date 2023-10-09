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

if(NOT DEFINED FLATBUFFERS_GIT_TAG)
    set(FLATBUFFERS_GIT_TAG v23.5.26)
endif()
set(FLATBUFFERS_GIT_URL https://github.com/google/flatbuffers.git)

FetchContent_Declare(
  flatbuffers
  GIT_REPOSITORY    ${FLATBUFFERS_GIT_URL}
  GIT_TAG           ${FLATBUFFERS_GIT_TAG}
  SOURCE_DIR        ${TRPC_ROOT_PATH}/cmake_third_party/flatbuffers
)
FetchContent_GetProperties(flatbuffers)

if(NOT flatbuffers_POPULATED)
    FetchContent_Populate(flatbuffers)

    execute_process(COMMAND git apply ${TRPC_ROOT_PATH}/third_party/com_github_google_flatbuffers/0001-feat-Modify-bazel-file.patch
                    WORKING_DIRECTORY ${flatbuffers_SOURCE_DIR})
    execute_process(COMMAND cp -rf ${TRPC_ROOT_PATH}/cmake/tools/flatbuffers_CMakeLists.txt CMakeLists.txt
                WORKING_DIRECTORY ${TRPC_ROOT_PATH}/cmake_third_party/flatbuffers)

    set(FLATBUFFERS_BUILD_TESTS OFF)
    set(FLATBUFFERS_BUILD_FLATC OFF)
    if(TRPC_BUILD_SHARED)
        set(FLATBUFFERS_BUILD_FLATLIB OFF)
        set(FLATBUFFERS_BUILD_SHAREDLIB ON)
    endif()

    add_subdirectory(${TRPC_ROOT_PATH}/cmake_third_party/flatbuffers)

    set(TARGET_INCLUDE_PATHS    ${TARGET_INCLUDE_PATHS}
                                ${TRPC_ROOT_PATH}/cmake_third_party/flatbuffers/include
                                ${TRPC_ROOT_PATH}/cmake_third_party/flatbuffers/src)
    if(TRPC_BUILD_SHARED)
        add_library(trpc_flatbuffers ALIAS flatbuffers_shared)
    else()
        add_library(trpc_flatbuffers ALIAS flatbuffers)
    endif()
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} trpc_flatbuffers)
endif()
