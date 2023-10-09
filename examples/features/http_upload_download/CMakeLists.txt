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

include(../cmake/common.cmake)

#---------------------------------------------------------------------------------------
# Compile project
#---------------------------------------------------------------------------------------
project(features_http_upload_download)

# compile server
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/server/*.cc)
add_executable(http_upload_download_server ${SRC_FILES})
target_link_libraries(http_upload_download_server ${LIBRARY})

# compile client
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/client/download_client.cc)
add_executable(download_client ${SRC_FILES})
target_link_libraries(download_client ${LIBRARY})

file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/client/upload_client.cc)
add_executable(upload_client ${SRC_FILES})
target_link_libraries(upload_client ${LIBRARY})
