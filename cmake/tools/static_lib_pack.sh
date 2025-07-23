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

shell_path=$(cd "$(dirname "$0")";pwd)

# tRPC-Cpp project root directory
rootpath=$(cd ${shell_path}/../../; pwd)

# Custom project CMakeLists.txt which include tRPC-Cpp through add_subdirectory
project_path=$1

# Package a static lib
if [ -d "${project_path}/build/" ]; then
    cd ${project_path}/build

    find ./cmake_third_party/* -name '*.a' |xargs -I param sh -c "cp param ./lib "

    # copy a reasonable name for lib(nghttp2)
    cp ./lib/libnghttp2_static.a ./lib/libnghttp2.a

    echo "[finish] All static library files had been collected in directory of '${project_path}/build/lib'"
else
    echo "[warning] Please execute CMakeLists.txt to compile tRPC-Cpp"
fi
