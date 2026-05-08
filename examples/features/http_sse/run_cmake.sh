#!/bin/bash

# building.
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/http_sse/build && cd examples/features/http_sse/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

# run server.
echo "kill previous process: http_sse_server"
killall http_sse_server 2>/dev/null
sleep 1
echo "try to start..."
examples/features/http_sse/build/http_sse_server --config=examples/features/http_sse/server/trpc_cpp_fiber.yaml &
http_sse_server_pid=$(ps -ef | grep 'bazel-bin/examples/features/http_sse/server/http_sse_server' | grep -v grep | awk '{print $2}')
if [ -n "http_sse_server_pid" ]; then
  echo "start successfully"
  echo "http_sse_server is running, pid = $http_sse_server_pid"
else
  echo "start failed"
  exit -1
fi

sleep 2

# run client.
examples/features/http_sse/build/sse_client --client_config=examples/features/http_sse/client/trpc_cpp_fiber.yaml

killall http_sse_server 2>/dev/null