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

if(NOT DEFINED JWT_VET)
  set(JWT_VET 0.7.0)
endif()

set(JWT_URL https://github.com/Thalhammer/jwt-cpp/archive/refs/tags/v${JWT_VET}.tar.gz)

FetchContent_Declare(
    jwt_cpp
    URL             ${JWT_URL}
    SOURCE_DIR      ${TRPC_ROOT_PATH}/cmake_third_party/jwt_cpp
)

FetchContent_GetProperties(jwt_cpp)

if(NOT jwt_cpp_POPULATED)
    FetchContent_Populate(jwt_cpp)
    add_subdirectory(${TRPC_ROOT_PATH}/cmake_third_party/jwt_cpp)
    add_library(trpc_jwt_cpp ALIAS jwt-cpp)
    set(TARGET_INCLUDE_PATHS    ${TARGET_INCLUDE_PATHS}
                                ${TRPC_ROOT_PATH}/cmake_third_party/jwt_cpp)
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} trpc_jwt_cpp)
endif()

