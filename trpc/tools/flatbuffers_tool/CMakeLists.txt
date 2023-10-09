cmake_minimum_required(VERSION 3.14)

project(FBS_CPP_PLUGIN)

set(BUILD_TARGET fbs_cpp_plugin)

set(CMAKE_CXX_STANDARD 17)

file(GLOB FBS_CPP_PLUGIN_SOURCE_FILES ${TRPC_ROOT_PATH}/trpc/tools/flatbuffers_tool/fbs2cpp.cc
                                      ${TRPC_ROOT_PATH}/trpc/tools/flatbuffers_tool/idl_gen_trpc.cc
                                      ${TRPC_ROOT_PATH}/trpc/tools/flatbuffers_tool/parse_fbs_file.cc
                                      ${TRPC_ROOT_PATH}/trpc/tools/flatbuffers_tool/main.cc)

include_directories(${TRPC_ROOT_PATH}
                    ${TRPC_ROOT_PATH}/cmake_third_party/flatbuffers/include
                    ${TRPC_ROOT_PATH}/cmake_third_party/flatbuffers/src)

add_executable(${BUILD_TARGET} ${FBS_CPP_PLUGIN_SOURCE_FILES})

if(NOT TRPC_MASTER_PROJECT)
    set(FBS_CPP_PLUGIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin/ PARENT_SCOPE)
    set_target_properties(${BUILD_TARGET}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
    )
endif()
target_link_libraries(${BUILD_TARGET} flatbuffers_tool_libs)
