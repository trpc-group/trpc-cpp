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
mkdir -p examples/features/grpc_stream/build && cd examples/features/grpc_stream/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

# run server.
echo "kill previous process: grpc_stream_server"
killall grpc_stream_server
sleep 1
echo "try to start..."
examples/features/grpc_stream/build/grpc_stream_server --config=examples/features/grpc_stream/server/trpc_cpp_fiber.yaml --dbpath=examples/features/grpc_stream/common/route_guide_db.json &
sleep 1
server_pid=$(ps -ef | grep 'examples/features/grpc_stream/build/grpc_stream_server' | grep -v grep | awk '{print $2}')
if [ -n "server_pid" ]; then
  echo "start successfully"
  echo "grpc_stream_server is running, pid = $server_pid"
else
  echo "start failed"
  exit -1
fi

sleep 2

killall grpc_stream_server
