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

if(NOT DEFINED YAML_VER)
    set(YAML_VER 0.7.0)
endif()
set(YAML_URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-${YAML_VER}.tar.gz)

FetchContent_Declare(
    yaml_cpp
    URL        ${YAML_URL}
    SOURCE_DIR ${TRPC_ROOT_PATH}/cmake_third_party/yaml-cpp
)

FetchContent_GetProperties(yaml_cpp)
if(NOT yaml_cpp_POPULATED)
    FetchContent_Populate(yaml_cpp)

    set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
    set(FLAG_WEXTRA OFF)
    set(YAML_CPP_BUILD_TOOLS OFF)
    add_subdirectory(${TRPC_ROOT_PATH}/cmake_third_party/yaml-cpp)

    set_target_properties(
        yaml-cpp  PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${yaml_cpp_SOURCE_DIR}/include>;$<BUILD_INTERFACE:${yaml_cpp_SOURCE_DIR}/src>"
    )

    add_library(trpc_yaml ALIAS yaml-cpp)
    
    install(DIRECTORY ${TRPC_ROOT_PATH}/cmake_third_party/yaml-cpp/include
                COMPONENT DEVEL
                DESTINATION ./
                FILES_MATCHING PATTERN "*.h")
    
    set(TARGET_INCLUDE_PATHS    ${TARGET_INCLUDE_PATHS}
                                ${yaml_cpp_SOURCE_DIR}/include
                                ${yaml_cpp_SOURCE_DIR}/src)
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} trpc_yaml)
endif()
