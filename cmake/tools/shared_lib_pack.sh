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

shell_path=$(cd "$(dirname "$0")";pwd)

rootpath=$(cd ${shell_path}/../../; pwd)

if [ -d "${rootpath}/build/" ]; then
    cd ${rootpath}/build

    find ./cmake_third_party/* -name '*.so' |xargs -I param sh -c "cp param ./lib "
    find ./cmake_third_party/* -name '*.so.*' |xargs -I param sh -c "cp param ./lib "

    echo "[finish] All static library files had been collected in directory of '${project_path}/build/lib'"
else
    echo "please execute CMakeLists.txt to compile tRPC-Cpp"
fi
