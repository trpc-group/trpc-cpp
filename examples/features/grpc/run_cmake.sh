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
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

# run server.
echo "kill previous process: helloworld_svr"
killall helloworld_svr
sleep 1
echo "try to start..."

# It's also available to run helloworld_svr with default merge or separate thread model.
# examples/helloworld/build/helloworld_svr --config=examples/features/grpc/server/trpc_cpp.yaml &
examples/helloworld/build/helloworld_svr --config=examples/features/grpc/server/trpc_cpp_fiber.yaml &
server_pid=$(ps -ef | grep 'bazel-bin/examples/helloworld/helloworld_svr' | grep -v grep | awk '{print $2}')
if [ -n "server_pid" ]; then
  echo "start successfully"
  echo "helloworld_svr is running, pid = $server_pid"
else
  echo "start failed"
fi

sleep 2

# run client.
examples/helloworld/build/fiber_client --client_config=examples/features/grpc/client/trpc_cpp_fiber.yaml
examples/helloworld/build/future_client --client_config=examples/features/grpc/client/trpc_cpp.yaml
