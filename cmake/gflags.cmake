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

if(NOT DEFINED GFLAGS_VER)
    set(GFLAGS_VER 2.2.2)
endif()
set(GFLAGS_DOWNLOAD_URL  https://github.com/gflags/gflags/archive/v${GFLAGS_VER}.tar.gz)

FetchContent_Declare(
    gflags
    URL        ${GFLAGS_DOWNLOAD_URL}
    SOURCE_DIR ${TRPC_ROOT_PATH}/cmake_third_party/gflags
)
FetchContent_GetProperties(gflags)
if(NOT gflags_POPULATED)
    FetchContent_Populate(gflags)

    execute_process(COMMAND sh ${TRPC_ROOT_PATH}/cmake/tools/sed_gflags.sh
                    WORKING_DIRECTORY ${TRPC_ROOT_PATH}/cmake_third_party/gflags)
    
    set(GFLAGS_IS_SUBPROJECT ON)
    if(TRPC_BUILD_SHARED)
        set(GFLAGS_BUILD_STATIC_LIBS OFF)
        set(GFLAGS_INSTALL_STATIC_LIBS OFF)
        set(GFLAGS_BUILD_SHARED_LIBS ON)
        set(GFLAGS_INSTALL_SHARED_LIBS ON)
    else()
        set(GFLAGS_BUILD_STATIC_LIBS ON)
        set(GFLAGS_INSTALL_STATIC_LIBS ON)
        set(GFLAGS_BUILD_SHARED_LIBS OFF)
        set(GFLAGS_INSTALL_SHARED_LIBS OFF)
    endif()

    add_subdirectory(${TRPC_ROOT_PATH}/cmake_third_party/gflags)

    install(DIRECTORY ${TRPC_BUILD_BINARY_PATH}/cmake_third_party/gflags/include/gflags
                COMPONENT DEVEL
                DESTINATION include/
                FILES_MATCHING PATTERN "*.h")

    set(TARGET_INCLUDE_PATHS    ${TRPC_BUILD_BINARY_PATH}/cmake_third_party/gflags/include
                                ${TARGET_INCLUDE_PATHS})

    set(TARGET_LINK_LIBS  ${TARGET_LINK_LIBS} gflags)
endif()
