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
bazel build //examples/features/trpc_stream/server:trpc_stream_server
bazel build //examples/features/trpc_stream/client:client
bazel build //examples/features/trpc_stream/client:rawdata_stream_client

# run server.
echo "kill previous process: trpc_stream_server"
killall trpc_stream_server
sleep 1
echo "try to start..."
bazel-bin/examples/features/trpc_stream/server/trpc_stream_server --config=examples/features/trpc_stream/server/trpc_cpp_fiber.yaml &
server_pid=$(ps -ef | grep 'bazel-bin/examples/features/trpc_stream/server/trpc_stream_server' | grep -v grep | awk '{print $2}')
if [ -n "server_pid" ]; then
  echo "start successfully"
  echo "trpc_stream_server is running, pid = $server_pid"
else
  echo "start failed"
fi

sleep 2

# run client.
bazel-bin/examples/features/trpc_stream/client/client --client_config=examples/features/trpc_stream/client/trpc_cpp_fiber.yaml --rpc_method=ClientStreamSayHello
#bazel-bin/examples/features/trpc_stream/client/client --client_config=examples/features/trpc_stream/client/trpc_cpp_fiber.yaml --rpc_method=ServerStreamSayHello
#bazel-bin/examples/features/trpc_stream/client/client --client_config=examples/features/trpc_stream/client/trpc_cpp_fiber.yaml --rpc_method=BidiStreamSayHello
#bazel-bin/examples/features/trpc_stream/client/rawdata_stream_client --client_config=examples/features/trpc_stream/client/trpc_cpp_fiber.yaml --rpc_method=RawDataReadWrite
