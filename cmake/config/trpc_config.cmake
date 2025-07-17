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

#---------------------------------------------------------------------------------------
# Set the install directory where the tRPC header lib files are placed after "make install"
#---------------------------------------------------------------------------------------
if (NOT DEFINED TRPC_CMAKE_INSTALL_DEFAULT_PREFIX)
    set(TRPC_CMAKE_INSTALL_DEFAULT_PREFIX /usr/local/trpc-cpp)
endif()
set(TRPC_SYMBOLIC_PATH_PREFIX ${TRPC_CMAKE_INSTALL_DEFAULT_PREFIX}/trpc)

#---------------------------------------------------------------------------------------
# Set the include install directory
#---------------------------------------------------------------------------------------
set(INCLUDE_INSTALL_PATHS ${TRPC_SYMBOLIC_PATH_PREFIX}/include)

#---------------------------------------------------------------------------------------
# Set the lib install directory
#---------------------------------------------------------------------------------------
set(LIBRARY_INSTALL_PATHS ${TRPC_SYMBOLIC_PATH_PREFIX}/lib)


#---------------------------------------------------------------------------------------
# Set third_party include path which used to build tRPC lib.
# If you won't do "make install", you should include below "INCLUDE_PATHS" when you build tRPC lib
#---------------------------------------------------------------------------------------
set(INCLUDE_PATHS   ${TRPC_ROOT_PATH}
                    ${TRPC_ROOT_PATH}/build/cmake_third_party/gflags/include
                    ${TRPC_ROOT_PATH}/cmake_third_party/prometheus/pull/include
                    ${TRPC_ROOT_PATH}/cmake_third_party/prometheus/core/include
                    ${TRPC_ROOT_PATH}/build/cmake_third_party/prometheus/core/include
                    ${TRPC_ROOT_PATH}/build/cmake_third_party/prometheus/pull/include
                    ${TRPC_ROOT_PATH}/build/cmake_third_party/prometheus/push/include
                    ${TRPC_ROOT_PATH}/cmake_third_party/protobuf/src
                    ${TRPC_ROOT_PATH}/cmake_third_party/fmt/include
                    ${TRPC_ROOT_PATH}/cmake_third_party/flatbuffers/include
                    ${TRPC_ROOT_PATH}/cmake_third_party/flatbuffers/src
                    ${TRPC_ROOT_PATH}/cmake_third_party/rapidjson/include
                    ${TRPC_ROOT_PATH}/cmake_third_party/yaml-cpp/include
                    ${TRPC_ROOT_PATH}/cmake_third_party/gtest/googlemock/include
                    ${TRPC_ROOT_PATH}/cmake_third_party/gtest/googletest/include
                    ${TRPC_ROOT_PATH}/cmake_third_party/spdlog/include
                    ${TRPC_ROOT_PATH}/cmake_third_party/spdlog/include/spdlog
                    ${TRPC_ROOT_PATH}/cmake_third_party/jsoncpp/include
                    ${TRPC_ROOT_PATH}/cmake_third_party/nghttp2/lib/includes
                    ${TRPC_ROOT_PATH}/cmake_third_party/picohttpparser
                    ${TRPC_ROOT_PATH}/cmake_third_party/snappy
                    ${TRPC_ROOT_PATH}/cmake_third_party/lz4
                    ${TRPC_ROOT_PATH}/cmake_third_party/jwt_cpp/include)

# When use tRPC as a third-party library, selectively inject the header files at including any-lib.cmake.
set(TARGET_INCLUDE_PATHS  ${TRPC_ROOT_PATH})
# When use tRPC as a third-party library, selectively inject the libs at including any-lib.cmake.
set(TARGET_LINK_LIBS)

#---------------------------------------------------------------------------------------
# Set third-party lib path.
# If you won't do "make install", you should link below "LIBRARY_PATHS" when you build tRPC lib
#---------------------------------------------------------------------------------------
set(LIBRARY_PATHS ${TRPC_ROOT_PATH}/build/lib)

#---------------------------------------------------------------------------------------
# TRPC_BASIC_THIRD_PARTY is only used for linking third-party libs with the tRPC lib.
# No need to link with your own binary if you build tRPC lib as static lib.
#---------------------------------------------------------------------------------------
set(TRPC_BASIC_THIRD_PARTY
                        pthread
                        z
                        dl
                        curl)

#---------------------------------------------------------------------------------------
#                          Relevant library options
#---------------------------------------------------------------------------------------
# LIBS_BASIC              |    basic libs that tRPC depends on
# LIB_GTEST_GMOCK         |    gtest/gmock lib
# LIB_TCMALLOC_PROFILER   |    tcmalloc profiler lib
# LIB_SSL                 |    ssl lib
# ------------------------|-------------------------------------------------------------
# LIB_METRICS_PROMETHEUS  |    prometheus lib
#---------------------------------------------------------------------------------------
# gtest/gmock lib
set(LIB_GTEST_GMOCK gtest
                    gtest_main
                    gmock)

# basic libs that tRPC depends on
set(LIBS_BASIC       
                    protobuf
                    protoc
                    gflags_nothreads
                    spdlog
                    fmt
                    yaml-cpp
                    jsoncpp
                    nghttp2
                    snappy
                    lz4
                    flatbuffers
                    pthread
                    z
                    dl
                    curl)

set(LIB_TCMALLOC_PROFILER /usr/lib64/libtcmalloc_and_profiler.so)

set(LIB_SSL ssl crypto)

set(LIB_METRICS_PROMETHEUS prometheus-cpp-core
                           prometheus-cpp-pull
                           prometheus-cpp-push
                           jwt-cpp)
