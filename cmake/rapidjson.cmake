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

if(NOT DEFINED RAPIDJSON_VER)
    set(RAPIDJSON_VER 1.1.0)
endif()
set(RAPIDJSON_URL https://github.com/Tencent/rapidjson/archive/v${RAPIDJSON_VER}.zip)

FetchContent_Declare(
    com_github_tencent_rapidjson
    URL             ${RAPIDJSON_URL}
    SOURCE_DIR      ${TRPC_ROOT_PATH}/cmake_third_party/rapidjson
)

FetchContent_GetProperties(com_github_tencent_rapidjson)
if(NOT com_github_tencent_rapidjson_POPULATED)
    FetchContent_Populate(com_github_tencent_rapidjson)

    execute_process(COMMAND git apply ${TRPC_ROOT_PATH}/third_party/com_github_tencent_rapidjson/0001-remove-non-compiling-assignment-operator.patch
                                      --directory cmake_third_party/rapidjson
                    WORKING_DIRECTORY ${TRPC_ROOT_PATH})

    add_definitions(-DRAPIDJSON_HAS_STDSTRING)
    include_directories("${TRPC_ROOT_PATH}/cmake_third_party/rapidjson/include/")

    install(DIRECTORY "${TRPC_ROOT_PATH}/cmake_third_party/rapidjson/include/rapidjson"
            COMPONENT DEVEL
            DESTINATION include)

    set(TARGET_INCLUDE_PATHS    ${TARGET_INCLUDE_PATHS}
                                ${TRPC_ROOT_PATH}/cmake_third_party/rapidjson/include/)
endif()
