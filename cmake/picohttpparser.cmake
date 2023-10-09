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

set(PICOHTTPPARSER_GIT_URL https://github.com/h2o/picohttpparser.git)

FetchContent_Declare(
    picohttpparser
    GIT_REPOSITORY    ${PICOHTTPPARSER_GIT_URL}
    SOURCE_DIR        ${TRPC_ROOT_PATH}/cmake_third_party/picohttpparser
)


FetchContent_GetProperties(nghttp2)
if(NOT picohttpparser_POPULATED)
    FetchContent_Populate(picohttpparser)
    install(DIRECTORY ${picohttpparser_SOURCE_DIR}
            COMPONENT DEVEL
            DESTINATION include/
            FILES_MATCHING PATTERN "*.h"
            PATTERN "doc" EXCLUDE
            PATTERN "examples" EXCLUDE
            PATTERN "test" EXCLUDE
            PATTERN "googletest" EXCLUDE
            PATTERN "protobuf" EXCLUDE
            PATTERN "objs" EXCLUDE
            PATTERN "yaml-cpp" EXCLUDE)
    set(TARGET_INCLUDE_PATHS  ${TARGET_INCLUDE_PATHS}
                              ${TRPC_ROOT_PATH}/cmake_third_party/picohttpparser)
endif()
