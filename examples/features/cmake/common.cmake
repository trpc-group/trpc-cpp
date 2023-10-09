#---------------------------------------------------------------------------------------
# Necessary compile env setting
#---------------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)

# 1. 'TRPC_ROOT_PATH' is the src director of trpc-cpp. You must specify it correctly.
set(TRPC_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../)

# 2. load config(hdrs/libs paths) and utils(tools for generating stub code)
include(${TRPC_ROOT_PATH}/cmake/config/trpc_config.cmake)
include(${TRPC_ROOT_PATH}/cmake/tools/trpc_utils.cmake)

# 3. add hdrs and libs paths to cmake build system
include_directories(${INCLUDE_PATHS})
link_directories(${LIBRARY_PATHS})

# 4. set current project dependency libs(trpc and other libs(protobuf/yaml-cpp/pthread/...etc))
set(LIBRARY trpc ${LIBS_BASIC})
# to use LIB_METRICS_PROMETHEUS, including prometheus libs
# set(LIBRARY trpc ${LIBS_BASIC} ${LIB_METRICS_PROMETHEUS})

# 5. Set the paths of protoc and trpc_cpp_plugin which will be generated at ${TRPC_ROOT_PATH}/build/bin
set(PB_PROTOC ${TRPC_ROOT_PATH}/build/bin/protoc)
set(TRPC_CPP_PLUGIN ${TRPC_ROOT_PATH}/build/bin/trpc_cpp_plugin)