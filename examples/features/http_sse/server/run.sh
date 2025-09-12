#!/bin/bash

# Get the directory where the script is located
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd $DIR/../../../.. && pwd)"

# building.
cd $ROOT_DIR
bazel build //examples/features/http_sse/server:http_sse_server

# run server.
echo "kill previous process: http_sse_server"
killall http_sse_server 2>/dev/null
sleep 1
echo "try to start..."
# Execute from the root directory where bazel-bin is located
$ROOT_DIR/bazel-bin/examples/features/http_sse/server/http_sse_server --config=$DIR/trpc_cpp_fiber.yaml &
http_server_pid=$(ps -ef | grep 'http_sse_server' | grep -v grep | awk '{print $2}')
if [ -n "$http_server_pid" ]; then
  echo "start successfully"
  echo "http_sse_server is running, pid = $http_server_pid"
else
  echo "start failed"
  exit -1
fi

echo "SSE server started. Run the client in another terminal to test."

