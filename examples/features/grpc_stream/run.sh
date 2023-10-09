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
bazel build //examples/features/grpc_stream/server:grpc_stream_server

# run server.
echo "kill previous process: grpc_stream_server"
killall grpc_stream_server
sleep 1
echo "try to start..."
bazel-bin/examples/features/grpc_stream/server/grpc_stream_server --config=examples/features/grpc_stream/server/trpc_cpp_fiber.yaml --dbpath=examples/features/grpc_stream/common/route_guide_db.json &
sleep 1
server_pid=$(ps -ef | grep 'bazel-bin/examples/features/grpc_stream/server/grpc_stream_server' | grep -v grep | awk '{print $2}')
if [ -n "server_pid" ]; then
  echo "start successfully"
  echo "grpc_stream_server is running, pid = $server_pid"
else
  echo "start failed"
fi

sleep 2

killall grpc_stream_server
