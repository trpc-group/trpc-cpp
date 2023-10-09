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

if(NOT DEFINED SPDLOG_GIT_TAG)
    set(SPDLOG_GIT_TAG v1.10.0)
endif()

set(SPDLOG_GIT_URL  https://github.com/gabime/spdlog.git)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY    ${SPDLOG_GIT_URL}
    GIT_TAG    ${SPDLOG_GIT_TAG}
    SOURCE_DIR        ${TRPC_ROOT_PATH}/cmake_third_party/spdlog
)
FetchContent_GetProperties(spdlog)

if(NOT spdlog_POPULATED)
    FetchContent_Populate(spdlog)

    # override fmt lib in spdlog to avoid conflict
    execute_process(COMMAND sh ${TRPC_ROOT_PATH}/cmake/tools/copy_fmt_to_spdlog.sh ${TRPC_ROOT_PATH}
                    WORKING_DIRECTORY ${spdlog_SOURCE_DIR})
    # apply patch
    execute_process(COMMAND git apply ${TRPC_ROOT_PATH}/third_party/com_github_gabime_spdlog/0001-flush-unit-change-for-milliseconds.patch
                    WORKING_DIRECTORY ${spdlog_SOURCE_DIR})
    execute_process(COMMAND git apply ${TRPC_ROOT_PATH}/third_party/com_github_gabime_spdlog/0002-pre-allocate-file-buffer-size.patch
                    WORKING_DIRECTORY ${spdlog_SOURCE_DIR})

    include_directories(${TRPC_ROOT_PATH}/cmake_third_party/fmt/include)

    if(TRPC_BUILD_SHARED)
        option(SPDLOG_BUILD_SHARED "Build shared library" ON)
    endif()

    add_definitions(-DSPDLOG_COMPILED_LIB)
    option(SPDLOG_FMT_EXTERNAL "Use external fmt library instead of bundled" ON)
    add_definitions(-DSPDLOG_DISABLE_DEFAULT_LOGGER)
    add_subdirectory(${TRPC_ROOT_PATH}/cmake_third_party/spdlog)

    set_target_properties(
        spdlog PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${TRPC_ROOT_PATH}/cmake_third_party/spdlog/include;${TRPC_ROOT_PATH}/cmake_third_party/spdlog/include/spdlog;"
    )


    target_link_libraries(
        spdlog INTERFACE
        trpc_fmt
    )

    add_library(trpc_spdlog ALIAS spdlog)

    install(DIRECTORY ${spdlog_SOURCE_DIR}/include/spdlog
                COMPONENT DEVEL
                DESTINATION include/
                FILES_MATCHING PATTERN "*.h")

    set(TARGET_INCLUDE_PATHS    ${TARGET_INCLUDE_PATHS}
                                ${TRPC_ROOT_PATH}/cmake_third_party/spdlog/include
                                ${TRPC_ROOT_PATH}/cmake_third_party/spdlog/include/spdlog)
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} trpc_spdlog)
endif()
