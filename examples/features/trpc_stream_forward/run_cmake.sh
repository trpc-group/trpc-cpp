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
mkdir -p examples/features/trpc_stream/build && cd examples/features/trpc_stream/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/trpc_stream_forward/build && cd examples/features/trpc_stream_forward/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

# run server.
echo "kill previous process: trpc_stream_server"
killall trpc_stream_server
sleep 1
echo "try to start..."
examples/features/trpc_stream/build/trpc_stream_server --config=examples/features/trpc_stream/server/trpc_cpp_fiber.yaml &
server_pid=$(ps -ef | grep 'examples/features/trpc_stream/build/trpc_stream_server' | grep -v grep | awk '{print $2}')
if [ -n "server_pid" ]; then
  echo "start successfully"
  echo "trpc_stream_server is running, pid = $server_pid"
else
  echo "start failed"
fi

sleep 2

# run server(proxy).
echo "kill previous process: trpc_stream_forward_server"
killall trpc_stream_forward_server
sleep 1
echo "try to start..."
examples/features/trpc_stream_forward/build/trpc_stream_forward_server --config=examples/features/trpc_stream_forward/proxy/trpc_cpp_fiber.yaml &
server_pid=$(ps -ef | grep 'examples/features/trpc_stream_forward/build/trpc_stream_forward_server' | grep -v grep | awk '{print $2}')
if [ -n "server_pid" ]; then
  echo "start successfully"
  echo "trpc_stream_forward_server is running, pid = $server_pid"
else
  echo "start failed"
  exit -1
fi

sleep 2

# run client.
examples/features/trpc_stream_forward/build/client --client_config=examples/features/trpc_stream_forward/client/trpc_cpp_fiber.yaml --rpc_method=ClientStreamSayHello
#examples/features/trpc_stream_forward/build/client --client_config=examples/features/trpc_stream_forward/client/trpc_cpp_fiber.yaml --rpc_method=ServerStreamSayHello
#examples/features/trpc_stream_forward/build/client --client_config=examples/features/trpc_stream_forward/client/trpc_cpp_fiber.yaml --rpc_method=BidiStreamSayHello

killall trpc_stream_server
killall trpc_stream_forward_server
