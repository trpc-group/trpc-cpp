#!/bin/bash

# Get the directory where the script is located
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd $DIR/../.. && pwd)"

# Building.
echo "Building SSE server and client..."
cd $ROOT_DIR
bazel build //examples/features/http_sse/server:http_sse_server
bazel build //examples/features/http_sse/client:sse_client

# Run server in background.
echo "kill previous process: http_sse_server"
killall http_sse_server 2>/dev/null
sleep 1
echo "Starting SSE server..."
# Execute from the root directory where bazel-bin is located
$ROOT_DIR/bazel-bin/examples/features/http_sse/server/http_sse_server --config=$DIR/server/trpc_cpp_fiber.yaml &
http_server_pid=$(ps -ef | grep 'http_sse_server' | grep -v grep | awk '{print $2}')
if [ -n "$http_server_pid" ]; then
  echo "Server started successfully, pid = $http_server_pid"
else
  echo "Failed to start server"
  exit -1
fi

# Wait a moment for server to be ready
sleep 2

# Run client.
echo "Running SSE client..."
$ROOT_DIR/bazel-bin/examples/features/http_sse/client/sse_client --client_config=$DIR/client/trpc_cpp_fiber.yaml

# Kill server
killall http_sse_server 2>/dev/null
echo "Test completed."