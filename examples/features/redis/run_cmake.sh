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

#! /bin/bash

# building.
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/redis/build && cd examples/features/redis/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

# run redis client.
examples/features/redis/build/fiber_client --client_config=examples/features/redis/client/fiber/fiber_client_config.yaml
examples/features/redis/build/future_client --client_config=examples/features/redis/client/future/future_client_config.yaml
