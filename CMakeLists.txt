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

cmake_minimum_required(VERSION 3.14)

#---------------------------------------------------------------------------------------
# Start tRPC-Cpp
#---------------------------------------------------------------------------------------
project(tRPC-Cpp)

#---------------------------------------------------------------------------------------
# Recommend GCC version 7.3.1. If the version is greater than 8, enable the compilation option TRPC_GCC_VERSION_GREATER
#---------------------------------------------------------------------------------------
set(GCC_EXPECTED_VERSION 7.3.1)
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS GCC_EXPECTED_VERSION)
    message(AUTHOR_WARNING "[GCC Warning]: tRPC recommends GCC version ${GCC_EXPECTED_VERSION} to build but found GCC ${CMAKE_CXX_COMPILER_VERSION}")
elseif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8)
    set(TRPC_GCC_VERSION_GREATER ON)
endif()

#---------------------------------------------------------------------------------------
# Set cmake compile option
#---------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(ALLOW_DUPLICATE_CUSTOM_TARGETS TRUE)

#---------------------------------------------------------------------------------------
# Processor architecture, support x86_64 and arm64
# The architecture is auto detected by CMAKE_SYSTEM_PROCESSOR marco
#---------------------------------------------------------------------------------------
option(TRPC_BUILD_WITH_X86_64              "x86_64 plateform"              ON)
option(TRPC_BUILD_WITH_ARM                 "arm64 plateform"               OFF)
option(TRPC_BUILD_SHARED                   "Build shared library"          OFF)

message("-- CMAKE_SYSTEM_NAME:      ${CMAKE_SYSTEM_NAME}")
message("-- CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
# The default release version optimization level of cmake under aarch64/arm is too much.
# It will cause unexpected behavior. Therefore, manually change the release optimization level is necessary.
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    set(CMAKE_CXX_FLAGS_RELEASE "-O1")
    set(CMAKE_C_FLAGS_RELEASE "-O2")
    set(CMAKE_ASM_FLAGS_RELEASE "-O2")
    set(TRPC_BUILD_WITH_ARM ON)
    set(TRPC_BUILD_WITH_X86_64 OFF)
endif()

# Alough we provide TRPC_BUILD_SHARED to build shared libs,
# BUILD_SHARED_LIBS will also be recognized because it's a common option for a lots of sdk.
if(BUILD_SHARED_LIBS)
    set(TRPC_BUILD_SHARED ON)
endif()
if(TRPC_BUILD_SHARED)
    set(BUILD_SHARED_LIBS ON)
    # add_definitions(-fPIC)
endif()

#---------------------------------------------------------------------------------------
# Set ASM compile flag
#---------------------------------------------------------------------------------------
enable_language(ASM)
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}  -fstack-protector -Wall -Wunused-but-set-parameter")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}  -fno-omit-frame-pointer -Wno-free-nonheap-object")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}  -MD -MF -fPIC")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}  -fno-canonical-system-headers -Wno-builtin-macro-redefined")

#---------------------------------------------------------------------------------------
# Set gcc/g++ compile flag
#---------------------------------------------------------------------------------------
if(TRPC_GCC_VERSION_GREATER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wno-error=class-memaccess")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fstack-protector -Wall -Wunused-but-set-parameter -lm")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fno-omit-frame-pointer -Wno-free-nonheap-object")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -MD -MF -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fno-canonical-system-headers -Wno-builtin-macro-redefined")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wno-implicit-fallthrough")

if(TRPC_CXX_FLAGS_FPIC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -fPIC")
endif()

#---------------------------------------------------------------------------------------
# Project configurations
#---------------------------------------------------------------------------------------
set(TRPC_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH "${TRPC_ROOT_PATH}/cmake;${CMAKE_MODULE_PATH}")

include(config/trpc_config)
# Get the latest version of tRPC-Cpp from the changelog.
file(READ "${TRPC_ROOT_PATH}/CHANGELOG.md" CHANGELOG)
STRING (REGEX MATCH "[0-9]\.[0-9][0-9]\.[0-9]([-]rc|\\.)?([-]beta|\\.)?(\\.)?([0-9])?" TRPC_VERSION ${CHANGELOG})
if(NOT DEFINED TRPC_VERSION OR TRPC_VERSION STREQUAL "")
    message(WARNING "[WARNING]: CMake can't found tRPC-Cpp latest version in \"CHANGELOG.md\". Use master as version")
    set(TRPC_VERSION "master")
endif()
set(TRPC_INSTALL_PATH_PREFIX  ${TRPC_CMAKE_INSTALL_DEFAULT_PREFIX}/trpc-${TRPC_VERSION}/)
set(CMAKE_INSTALL_PREFIX ${TRPC_INSTALL_PATH_PREFIX})

#---------------------------------------------------------------------------------------
# tRPC Protocol
#---------------------------------------------------------------------------------------
include(trpc_protocol)

# Print tRPC-Cpp version
message(STATUS "Build tRPC-Cpp: v${TRPC_VERSION}")

#---------------------------------------------------------------------------------------
# Check whether this project is used by add_subdirectory or not.
# If not, your project may locate at trpc-cpp directory.
#---------------------------------------------------------------------------------------
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(TRPC_MASTER_PROJECT ON)
else()
    set(TRPC_MASTER_PROJECT OFF)
endif()
set(TRPC_BUILD_BINARY_PATH ${CMAKE_CURRENT_BINARY_DIR})

#---------------------------------------------------------------------------------------
# trpc-cpp feature options
#---------------------------------------------------------------------------------------
option(TRPC_BUILD_WITH_RPCZ              "rpcz"                            OFF)
option(TRPC_BUILD_WITH_OVERLOAD_CONTROL  "overload_control"                OFF)
option(TRPC_BUILD_WITH_TCMALLOC_PROFILER "Build with tcmalloc profiler"    OFF)
option(TRPC_BUILD_WITH_SSL               "Build with ssl"                  OFF)

#---------------------------------------------------------------------------------------
# trpc-cpp plugins options
# Naming conventions: TRPC_BUILD_WITH_[METRICS|TRACING|TELEMETRY|NAMING|]_PLUGINNAME
#---------------------------------------------------------------------------------------
option(TRPC_BUILD_WITH_METRICS_PROMETHEUS "metrics prometheus"             OFF)

#---------------------------------------------------------------------------------------
# Include third-party lib
#---------------------------------------------------------------------------------------
message("[tRPC-Cpp] cloning relavant 3rd party, please waiting...")
include(gtest_gmock)
include(fmt)
include(gflags)
include(jsoncpp)
include(protobuf)
include(spdlog)
include(rapidjson)
include(yaml)
include(picohttpparser)
include(nghttp2)
include(snappy)
include(lz4)
include(toml11)
include(flatbuffers)
include(jwt_cpp)

#---------------------------------------------------------------------------------------
# Set complie options and include other libs if options are ON
#---------------------------------------------------------------------------------------
if(TRPC_BUILD_WITH_RPCZ)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTRPC_BUILD_INCLUDE_RPCZ=1")
endif()

if(TRPC_BUILD_WITH_OVERLOAD_CONTROL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTRPC_BUILD_INCLUDE_OVERLOAD_CONTROL=1")
endif()

if(TRPC_BUILD_WITH_METRICS_PROMETHEUS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTRPC_BUILD_INCLUDE_PROMETHEUS=1")
    include(prometheus)
endif()

# The compilation options should be passed to parent CMakeLists.txt which use add_subdirectory to import trpc lib.
if(NOT TRPC_MASTER_PROJECT)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" PARENT_SCOPE)
endif()

if(TRPC_BUILD_WITH_TCMALLOC_PROFILER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTRPC_ENABLE_PROFILER=1")
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} ${LIB_TCMALLOC_PROFILER})
endif()

if(TRPC_BUILD_WITH_SSL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTRPC_BUILD_INCLUDE_SSL=1")
    set(TARGET_LINK_LIBS ${TARGET_LINK_LIBS} ${LIB_SSL})
endif()

if(NOT TRPC_MASTER_PROJECT)
    set(TRPC_ROOT_PATH ${TRPC_ROOT_PATH} PARENT_SCOPE)
    include_directories(${INCLUDE_PATHS})
    link_directories(${LIBRARY_PATHS})
endif()

#---------------------------------------------------------------------------------------
# Build trpc/tools idl stub generate executable binary
#---------------------------------------------------------------------------------------
include(tools/trpc_utils)
add_subdirectory(${TRPC_ROOT_PATH}/trpc/tools/trpc_cpp_plugin/)
add_subdirectory(${TRPC_ROOT_PATH}/trpc/tools/flatbuffers_tool/)

#---------------------------------------------------------------------------------------
# Generate .pb.h and .pb.cc from .proto file
#---------------------------------------------------------------------------------------
set(GEN_PB_PROTO_FILES ${TRPC_ROOT_PATH}/trpc/codec/trpc/trpc.proto
                       ${TRPC_ROOT_PATH}/trpc/proto/trpc.proto
                       ${TRPC_ROOT_PATH}/trpc/trpc.proto
                       ${TRPC_ROOT_PATH}/trpc/proto/trpc_options.proto
                       ${TRPC_ROOT_PATH}/trpc/tools/comm/trpc_cpp_options.proto)

COMPILE_PROTO(OUT_PB_SRCS_FILES "${GEN_PB_PROTO_FILES}" 
                       ${PROTOBUF_PROTOC_EXECUTABLE} 
                       ${TRPC_ROOT_PATH})


#---------------------------------------------------------------------------------------
# Set trpc_cpp_plugin which can generate .trpc.pb.h and .trpc.pb.cc from .proto file
#---------------------------------------------------------------------------------------
if(NOT TRPC_MASTER_PROJECT)
  set(TRPC_TO_CPP_PLUGIN ${TRPC_CPP_PLUGIN_DIR}/trpc_cpp_plugin PARENT_SCOPE)
endif()

if(NOT TRPC_MASTER_PROJECT)
  set(FBS_TO_CPP_PLUGIN ${FBS_CPP_PLUGIN_DIR}/fbs_cpp_plugin PARENT_SCOPE)
endif()

if(NOT TRPC_MASTER_PROJECT)
  set(PROTOBUF_PROTOC_EXECUTABLE ${PROTOBUF_PROTOC_EXECUTABLE} PARENT_SCOPE)
endif()

#---------------------------------------------------------------------------------------
# Get all source files
#---------------------------------------------------------------------------------------
file(GLOB_RECURSE SRC_FILES ./trpc/*.cc
                            ./trpc/*.cpp
                            ./trpc/*.S
                            ./cmake_third_party/picohttpparser/picohttpparser.c)

set(EXCLUDE_ASM_FILES   ./trpc/runtime/threadmodel/fiber/detail/fcontext/ppc64le/make_context.S
                            ./trpc/runtime/threadmodel/fiber/detail/fcontext/ppc64le/jump_context.S
                            ./trpc/runtime/threadmodel/fiber/detail/fcontext/aarch64/jump_context.S
                            ./trpc/runtime/threadmodel/fiber/detail/fcontext/aarch64/make_context.S)


if(TRPC_BUILD_WITH_ARM)
    set(EXCLUDE_ASM_FILES   ./trpc/runtime/threadmodel/fiber/detail/fcontext/ppc64le/make_context.S
                            ./trpc/runtime/threadmodel/fiber/detail/fcontext/ppc64le/jump_context.S
                            ./trpc/runtime/threadmodel/fiber/detail/fcontext/x86_64/jump_context.S
                            ./trpc/runtime/threadmodel/fiber/detail/fcontext/x86_64/make_context.S)
endif()

# Exclude test files
file(GLOB_RECURSE TEST_FILES ./trpc/*test.cc
                             ./trpc/*test.cpp
                             ./trpc/*testing.cc
                             ./test/*)
list(REMOVE_ITEM SRC_FILES ${TEST_FILES})

# Exclude specified files
file(GLOB_RECURSE EXCLUDE_FILES ${EXCLUDE_ASM_FILES}
                                ./examples/*
                                ./trpc/tools/*
                                ./trpc/util/async_io/*)

list(REMOVE_ITEM SRC_FILES ${EXCLUDE_FILES})

#---------------------------------------------------------------------------------------
# Build trpc lib
#---------------------------------------------------------------------------------------
if (TRPC_BUILD_SHARED)
    add_library(trpc SHARED ${SRC_FILES}
                            ${OUT_PB_SRCS_FILES})
    set(COLLECT_LIB_COMMAND ${TRPC_ROOT_PATH}/cmake/tools/shared_lib_pack.sh)
else ()
    add_library(trpc STATIC ${SRC_FILES}
                            ${OUT_PB_SRCS_FILES})
    set(COLLECT_LIB_COMMAND ${TRPC_ROOT_PATH}/cmake/tools/static_lib_pack.sh)
endif()
set_property(TARGET trpc
             PROPERTY INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}lib")

target_link_libraries(trpc  PRIVATE
                            ${TRPC_BASIC_THIRD_PARTY}
                            ${TARGET_LINK_LIBS})

add_dependencies(trpc ${PROTOBUF_PROTOC_EXECUTABLE} trpc_cpp_plugin)

target_include_directories(
    trpc PUBLIC
    ${TARGET_INCLUDE_PATHS}
)

#---------------------------------------------------------------------------------------
# Package trpc lib
#---------------------------------------------------------------------------------------
if(TRPC_BUILD_SHARED)
    set(TRPC_LIBS_FILTER "*.so*")
    set(TRPC_COLLECT_LIB_SCRIPT ${TRPC_ROOT_PATH}/cmake/tools/shared_lib_pack.sh)
else()
    set(TRPC_LIBS_FILTER "*.a")
    set(TRPC_COLLECT_LIB_SCRIPT ${TRPC_ROOT_PATH}/cmake/tools/static_lib_pack.sh)
endif()

add_custom_command(
    TARGET  trpc POST_BUILD

    COMMAND sh ${TRPC_COLLECT_LIB_SCRIPT} ${CMAKE_SOURCE_DIR}

    WORKING_DIRECTORY ${TRPC_ROOT_PATH}
    COMMENT "[tRPC-Cpp] collect third-party library files"
    VERBATIM
)

#---------------------------------------------------------------------------------------
# INSTALL
#---------------------------------------------------------------------------------------
# Install relative headers to /usr/local/trpc-cpp/trpc/include directory
# (both for tRPC-Cpp and its' third-party lib)
install(DIRECTORY ${TRPC_ROOT_PATH}/trpc/
            COMPONENT DEVEL
            DESTINATION include/trpc
            FILES_MATCHING PATTERN "*.h"
                           PATTERN "*.hpp")

# Install relative libs to /usr/local/trpc-cpp/trpc/lib directory
# (both for tRPC-Cpp and its' third-party lib)
install(DIRECTORY ${CMAKE_BINARY_DIR}/lib/
            COMPONENT DEVEL
            DESTINATION lib
            FILES_MATCHING PATTERN ${TRPC_LIBS_FILTER})

# install trpc lib(mainly for correctly linking dynamic libs)
install(TARGETS trpc
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

# Install relative binary to /usr/local/trpc-cpp/trpc/bin directory
# protoc/trpc_cpp_plugin/...etc
install(DIRECTORY ${CMAKE_BINARY_DIR}/bin/
            COMPONENT DEVEL
            DESTINATION bin
            FILES_MATCHING PATTERN "*")
# install binary trpc_cpp_plugin(mainly for correctly linking dynamic libs)
install(TARGETS trpc_cpp_plugin
        RUNTIME DESTINATION bin)

# Install cmake/tools/trpc_utils.cmake
install(FILES ${TRPC_ROOT_PATH}/cmake/tools/trpc_utils.cmake
            DESTINATION cmake/tools)

install(FILES ${TRPC_ROOT_PATH}/cmake/config/trpc_config.cmake
            DESTINATION cmake/config)

# Create soft link to /usr/local/trpc-cpp/trpc
install(CODE 
            "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${TRPC_INSTALL_PATH_PREFIX} ${TRPC_SYMBOLIC_PATH_PREFIX})")
install(CODE
            "execute_process(COMMAND chmod +x -R ${TRPC_SYMBOLIC_PATH_PREFIX}/bin)")
