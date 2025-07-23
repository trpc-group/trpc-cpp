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

# @brief                            Generate corresponding .pb.cc and .pb.h stub code based on proto files
# @param OUT_PB_SRCS_FILES          [Output]The generated .pb.cc files from proto files
# @param GEN_PB_PROTO_FILES         [Input]The collection of proto files
# @param PROTOBUF_PROTOC_EXECUTABLE [Input]Protoc executable program generated during the compilation process in the build/bin directory
# @param CMAKESOURCEDIR             [Input]The directory where the CMakeLists.txt of the project is located
function(COMPILE_PROTO OUT_PB_SRCS_FILES GEN_PB_PROTO_FILES  PROTOBUF_PROTOC_EXECUTABLE CMAKESOURCEDIR)
    foreach(PROTO_FILES ${GEN_PB_PROTO_FILES})
        # Get the name of the proto file without suffix
        string(REGEX REPLACE "\\.proto" "" PROTO_FILE_NAME ${PROTO_FILES})

        # Get the proto file located directory.
        string(REGEX REPLACE "[^/]+$" "" PROTO_DIR_NAME ${PROTO_FILES})

        # Get relative path of proto file
        string(REGEX REPLACE "${CMAKESOURCEDIR}/" "" RELATIVE_PATH_PROTO_FILES ${PROTO_FILES})

        # Output src file to list to build trpc lib
        LIST(APPEND PB_SRCS_FILES "${PROTO_FILE_NAME}.pb.cc")

        # Generate .pb.h and .pb.cc using protoc
        add_custom_command(
            OUTPUT ${PROTO_FILE_NAME}.pb.cc ${PROTO_FILE_NAME}.pb.h
            COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} -I ${CMAKESOURCEDIR} -I ${protobuf_SOURCE_DIR}/src --cpp_out=. ${RELATIVE_PATH_PROTO_FILES}
            WORKING_DIRECTORY ${CMAKESOURCEDIR}
            DEPENDS ${PROTO_FILES}
        )
    endforeach()
    set(${OUT_PB_SRCS_FILES} ${PB_SRCS_FILES} PARENT_SCOPE)
endfunction()

# @brief                             Generate corresponding .trpc.pb.cc and .trpc.pb.h stub code based on proto files
# @param OUT_TRPC_PB_SRCS_FILES      [Output]The generated .trpc.pb.cc files from proto files
# @param GEN_TRPC_PB_PROTO_FILES     [Input]The collection of proto files
# @param PROTOBUF_PROTOC_EXECUTABLE  [Input]Protoc executable program generated during the compilation process in the build/bin directory
# @param TRPC_TO_CPP_PLUGIN          [Input]trpc_cpp_plugin executable program generated during the compilation process in the build/bin directory
# @param CMAKESOURCEDIR              [Input]The directory where the CMakeLists.txt of the project is located
function(TRPC_COMPILE_PROTO OUT_TRPC_PB_SRCS_FILES GEN_TRPC_PB_PROTO_FILES PROTOBUF_PROTOC_EXECUTABLE TRPC_TO_CPP_PLUGIN CMAKESOURCEDIR)
    foreach(PROTO_FILES ${GEN_TRPC_PB_PROTO_FILES})
        # Get the name of the proto file without suffix
        string(REGEX REPLACE "\\.proto" "" PROTO_FILE_NAME ${PROTO_FILES})

        # Get the proto file located directory.
        string(REGEX REPLACE "[^/]+$" "" PROTO_DIR_NAME ${PROTO_FILES})

        # trpc_cpp_plugin need relative path start with ./
        string(REGEX REPLACE "${CMAKESOURCEDIR}" "." RELATIVE_PATH_PROTO_FILES ${PROTO_FILES})

        # Output src file to list to build trpc lib
        LIST(APPEND TRPC_PB_SRCS_FILES "${PROTO_FILE_NAME}.trpc.pb.cc")

        # Generate .trpc.pb.h and .trpc.pb.cc using protoc
        add_custom_command(
            OUTPUT ${PROTO_FILE_NAME}.trpc.pb.cc ${PROTO_FILE_NAME}.trpc.pb.h
            COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --plugin=protoc-gen-trpc=${TRPC_TO_CPP_PLUGIN} -I ${protobuf_SOURCE_DIR}/src --trpc_out=. -I. ${RELATIVE_PATH_PROTO_FILES}
            WORKING_DIRECTORY ${CMAKESOURCEDIR}
            DEPENDS ${PROTO_FILES}
        )
    endforeach()
    set(${OUT_TRPC_PB_SRCS_FILES} ${TRPC_PB_SRCS_FILES} PARENT_SCOPE)
endfunction()

# @brief                               Generate trpc mock files (*.trpc.mock.h) based on the pb files
# @param OUT_TRPC_MOCK_SRCS_FILES      [Output]The generated .trpc.pb.mock.h files
# @param GEN_TRPC_PB_PROTO_FILES       [Input]The collection of proto files
# @param PROTOBUF_PROTOC_EXECUTABLE    [Input]Protoc executable program generated during the compilation process in the build/bin directory
# @param TRPC_TO_CPP_PLUGIN            [Input]trpc_cpp_plugin executable program generated during the compilation process in the build/bin directory
# @param CMAKESOURCEDIR                [Input]The directory where the CMakeLists.txt of the project is located
function(TRPC_GEN_MOCK_CODE OUT_TRPC_MOCK_SRCS_FILES GEN_TRPC_PB_PROTO_FILES PROTOBUF_PROTOC_EXECUTABLE TRPC_TO_CPP_PLUGIN CMAKESOURCEDIR)
    foreach(PROTO_FILES ${GEN_TRPC_PB_PROTO_FILES})
        # Get the name of the proto file without suffix
        string(REGEX REPLACE "[^/]proto" "" PROTO_FILE_NAME ${PROTO_FILES})

        # Get the proto file located directory.
        string(REGEX REPLACE "[^/]+$" "" PROTO_DIR_NAME ${PROTO_FILES})

        # trpc_cpp_plugin need relative path start with ./
        string(REGEX REPLACE "${CMAKESOURCEDIR}" "." RELATIVE_PATH_PROTO_FILES ${PROTO_FILES})

        # Output mock file to list
        LIST(APPEND TRPC_PB_MOCK_SRCS_FILES "${PROTO_FILE_NAME}.trpc.pb.mock.h")

        add_custom_command(
            OUTPUT ${PROTO_FILE_NAME}.trpc.pb.mock.h
            COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --plugin=protoc-gen-trpc=${TRPC_TO_CPP_PLUGIN} -I ${TRPC_ROOT_PATH}/cmake_third_party/protobuf/src --trpc_out=generate_new_mock_code=true:. -I. ${RELATIVE_PATH_PROTO_FILES}
            WORKING_DIRECTORY ${CMAKESOURCEDIR}
            DEPENDS ${PROTO_FILES} ${TRPC_TO_CPP_PLUGIN}
        )
    endforeach()
    set(${OUT_TRPC_MOCK_SRCS_FILES} ${TRPC_PB_MOCK_SRCS_FILES} PARENT_SCOPE)
endfunction()

# @brief                             Generate corresponding .trpc.fb.cc and .trpc.fb.h stub code based on fbs files
# @param OUT_TRPC_FBS_SRCS_FILES     [Output]The generated .trpc.fb.cc files from proto files
# @param GEN_TRPC_FBS_PROTO_FILES    [Input]The collection of fbs files
# @param FBS_CPP_PLUGIN              [Input]trpc_cpp_plugin executable program generated during the compilation process in the build/bin directory
# @param CMAKESOURCEDIR              [Input]The directory where the CMakeLists.txt of the project is located
function(TRPC_COMPILE_FBS OUT_TRPC_FBS_SRCS_FILES GEN_TRPC_FBS_PROTO_FILES FBS_CPP_PLUGIN CMAKESOURCEDIR)
    foreach(FBS_FILES ${GEN_TRPC_FBS_PROTO_FILES})
        # Get the name of the fbs file without suffix
        string(REGEX REPLACE "\\.fbs" "" FBS_FILE_NAME ${FBS_FILES})

        # trpc_cpp_plugin need relative path start with ./
        string(REGEX REPLACE "${CMAKESOURCEDIR}" "." RELATIVE_PATH_FBS_FILES ${FBS_FILES})
        # fbs_cpp_plugin need strip ./ from path at start
        string(REGEX REPLACE "\\./" "" RELATIVE_PATH_FBS_FILES ${RELATIVE_PATH_FBS_FILES})

        # Get the fbs file located directory.
        string(REGEX REPLACE "[^/]+$" "" FBS_DIR_NAME ${RELATIVE_PATH_FBS_FILES})

        # Output src file to list to build trpc lib
        LIST(APPEND TRPC_FBS_SRCS_FILES "${FBS_FILE_NAME}.trpc.fb.cc")

        # Generate .trpc.pb.h and .trpc.pb.cc using protoc
        add_custom_command(
            OUTPUT ${FBS_FILE_NAME}.trpc.fb.cc ${FBS_FILE_NAME}.trpc.fb.h ${FBS_FILE_NAME}_generated.h
            COMMAND ${FBS_CPP_PLUGIN} -I ${FBS_DIR_NAME} --gen-object-api --gen-compare --gen-mutable --reflect-names --cpp-ptr-type flatbuffers::unique_ptr --trpc -c -o ${FBS_DIR_NAME} ${RELATIVE_PATH_FBS_FILES}
            WORKING_DIRECTORY ${CMAKESOURCEDIR}
            DEPENDS ${FBS_FILES}
        )
    endforeach()
    set(${OUT_TRPC_FBS_SRCS_FILES} ${TRPC_FBS_SRCS_FILES} PARENT_SCOPE)
endfunction()
