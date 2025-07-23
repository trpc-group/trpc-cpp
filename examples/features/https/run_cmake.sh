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
rm -rf build
mkdir -p build && cd build && cmake -DTRPC_BUILD_WITH_SSL=ON -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/https/build && cd examples/features/https/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
rm -rf build

# run server.
echo "kill previous process: https_server"
killall https_server
sleep 1
echo "try to start..."
examples/features/https/build/https_server --config=examples/features/https/server/trpc_cpp_fiber.yaml &
http_server_pid=$(ps -ef | grep 'examples/features/https/build/https_server' | grep -v grep | awk '{print $2}')
if [ -n "http_server_pid" ]; then
  echo "start successfully"
  echo "https_server is running, pid = $http_server_pid"
else
  echo "start failed"
  exit -1
fi

sleep 2

# run client.
examples/features/https/build/client --client_config=examples/features/https/client/trpc_cpp_fiber.yaml

killall http_server
