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

if(NOT DEFINED JSONCPP_VER)
    set(JSONCPP_VER 1.9.3)
endif()
set(JSONCPP_URL https://github.com/open-source-parsers/jsoncpp/archive/${JSONCPP_VER}.tar.gz)

FetchContent_Declare(
    jsoncpp
    URL         ${JSONCPP_URL}
    SOURCE_DIR  ${TRPC_ROOT_PATH}/cmake_third_party/jsoncpp
)

FetchContent_GetProperties(jsoncpp)
if(NOT jsoncpp_POPULATED)
    FetchContent_Populate(jsoncpp)

    set(JSONCPP_WITH_TESTS OFF)
    set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF)
    add_subdirectory(${TRPC_ROOT_PATH}/cmake_third_party/jsoncpp)

    set_target_properties(
        jsoncpp_lib PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${jsoncpp_SOURCE_DIR};${jsoncpp_SOURCE_DIR}/include;>"
    )
    
    add_library(trpc_jsoncpp ALIAS jsoncpp_lib)
    
    set(TARGET_INCLUDE_PATHS    ${TARGET_INCLUDE_PATHS}
                                ${jsoncpp_SOURCE_DIR}/include)
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} trpc_jsoncpp)
endif()
