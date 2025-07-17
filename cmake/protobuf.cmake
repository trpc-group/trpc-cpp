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

if(NOT DEFINED PROTOBUF_GIT_TAG)
    set(PROTOBUF_GIT_TAG v3.15.8)
endif()
set(PROTOBUF_GIT_URL https://github.com/protocolbuffers/protobuf/archive/${PROTOBUF_GIT_TAG}.tar.gz)

FetchContent_Declare(
    protobuf
    URL            ${PROTOBUF_GIT_URL}
    SOURCE_DIR     ${TRPC_ROOT_PATH}/cmake_third_party/protobuf
)
FetchContent_GetProperties(protobuf)

if(NOT protobuf_POPULATED)
    FetchContent_Populate(protobuf)

    if (TRPC_BUILD_SHARED)
        option(protobuf_BUILD_SHARED_LIBS "Build Shared Libraries" ON)
    endif()

    option(protobuf_BUILD_TESTS "Build tests" OFF)
    add_subdirectory(${TRPC_ROOT_PATH}/cmake_third_party/protobuf/cmake)
    add_library(trpc_protobuf ALIAS libprotobuf)
    add_library(trpc_protoc ALIAS libprotoc)

    set(TARGET_INCLUDE_PATHS ${TARGET_INCLUDE_PATHS}
                             ${TRPC_ROOT_PATH}/cmake_third_party/protobuf/src)
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} trpc_protobuf trpc_protoc)
endif()

# Make sure protoc binary when import protobuf
if(NOT DEFINED PROTOBUF_PROTOC_EXECUTABLE)
    set(PROTOBUF_PROTOC_EXECUTABLE protoc)
endif()
