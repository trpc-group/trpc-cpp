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

#! /bin/bash

# building.
bazel build //examples/features/redis/client/...

# run redis client.
./bazel-bin/examples/features/redis/client/fiber/fiber_client --client_config=examples/features/redis/client/fiber/fiber_client_config.yaml
./bazel-bin/examples/features/redis/client/future/future_client --client_config=examples/features/redis/client/future/future_client_config.yaml
