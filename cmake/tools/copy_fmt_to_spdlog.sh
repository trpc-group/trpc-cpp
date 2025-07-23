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

TRPC_ROOT_PATH=$1

# override fmt lib in spdlog to avoid conflict
if [ -d "${TRPC_ROOT_PATH}/cmake_third_party/fmt/include/fmt/" ]; then
    cp -rfp ${TRPC_ROOT_PATH}/cmake_third_party/fmt/include/fmt/* ${TRPC_ROOT_PATH}/cmake_third_party/spdlog/include/spdlog/fmt/bundled/
fi
