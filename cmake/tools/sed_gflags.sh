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

sed -i '/if (BUILD_TESTING)/,+3d'   CMakeLists.txt
sed -i 's/gflags_define (STRING NAMESPACE "Name(s) of library namespace (separate multiple options by semicolon)" "google;${PACKAGE_NAME}" "${PACKAGE_NAME}")/gflags_define (STRING NAMESPACE "Name(s) of library namespace (separate multiple options by semicolon)" "google;gflags" "google;gflags")/'   CMakeLists.txt
