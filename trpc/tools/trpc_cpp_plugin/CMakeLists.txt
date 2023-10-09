cmake_minimum_required(VERSION 3.14)

project(TRPC_CPP_PLUGIN)

set(BUILD_TARGET trpc_cpp_plugin)

set(CMAKE_CXX_STANDARD 17)

set(PROTOBUF_PROTOC_EXECUTABLE protoc)
set(CPP_TOOL_GEN_PB_PROTO_FILES ${TRPC_ROOT_PATH}/trpc/proto/trpc_options.proto
                                ${TRPC_ROOT_PATH}/trpc/tools/comm/trpc_cpp_options.proto)
COMPILE_PROTO(CPP_TOOL_OUT_PB_SRCS_FILES "${CPP_TOOL_GEN_PB_PROTO_FILES}" 
                                ${PROTOBUF_PROTOC_EXECUTABLE} 
                                ${TRPC_ROOT_PATH})

file(GLOB TRPC_CPP_PLUGIN_SOURCE_FILES  ${TRPC_ROOT_PATH}/trpc/tools/trpc_cpp_plugin/gen_hdr.cc
                                        ${TRPC_ROOT_PATH}/trpc/tools/trpc_cpp_plugin/gen_mock.cc
                                        ${TRPC_ROOT_PATH}/trpc/tools/trpc_cpp_plugin/gen_src.cc
                                        ${TRPC_ROOT_PATH}/trpc/tools/trpc_cpp_plugin/main.cc
                                        ${TRPC_ROOT_PATH}/trpc/tools/comm/utils.cc
                                        )

include_directories(${TRPC_ROOT_PATH})

add_executable(trpc_cpp_plugin ${TRPC_CPP_PLUGIN_SOURCE_FILES} ${CPP_TOOL_OUT_PB_SRCS_FILES})

target_compile_definitions(trpc_cpp_plugin PRIVATE FMT_HEADER_ONLY)

if(NOT TRPC_MASTER_PROJECT)
    set(TRPC_CPP_PLUGIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin/ PARENT_SCOPE)
    set_target_properties(trpc_cpp_plugin
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(${BUILD_TARGET} trpc_protobuf trpc_protoc)
set_property(TARGET ${BUILD_TARGET}
             PROPERTY INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}lib")