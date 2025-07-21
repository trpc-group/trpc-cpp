#
#
# Tencent is pleased to support the open source community by making tRPC available.
#
# Copyright (C) 2024 Tencent.
# All rights reserved.
#
# If you have downloaded a copy of the tRPC source code from Tencent,
# please note that tRPC source code is licensed under the  Apache 2.0 License,
# A copy of the Apache 2.0 License is included in this file.
#
#

# 下载外部依赖源文件需要使用到cmake的FetchContent命令
include(FetchContent)

# 定义外部库的版本
set(TRPC_PROTOCOL_GIT_VER v1.0.0)
set(TRPC_PROTOCOL_GIT_URL https://github.com/trpc-group/trpc.git)

FetchContent_Declare(
    com_github_trpc_protocol
    GIT_REPOSITORY    ${TRPC_PROTOCOL_GIT_URL}
    GIT_TAG           ${TRPC_PROTOCOL_GIT_VER}
    SOURCE_DIR  ${TRPC_ROOT_PATH}/cmake_third_party/trpc_protocol #指定库源文件下载的目录
)
FetchContent_GetProperties(com_github_trpc_protocol)

if(NOT com_github_trpc_protocol_POPULATED)
    FetchContent_Populate(com_github_trpc_protocol)
    execute_process(COMMAND cp -fp ${TRPC_ROOT_PATH}/cmake_third_party/trpc_protocol/trpc/trpc.proto .
                    WORKING_DIRECTORY ${TRPC_ROOT_PATH}/trpc)
    execute_process(COMMAND cp -fp ${TRPC_ROOT_PATH}/cmake_third_party/trpc_protocol/trpc/proto/trpc_options.proto .
                    WORKING_DIRECTORY ${TRPC_ROOT_PATH}/trpc/proto)
endif()
