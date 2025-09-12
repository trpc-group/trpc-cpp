#!/bin/bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd $DIR/../../../.. && pwd)"

echo "Building SSE client..."
cd $ROOT_DIR
bazel build //examples/features/http_sse/client:sse_client

echo "Running SSE client..."
$ROOT_DIR/bazel-bin/examples/features/http_sse/client/sse_client \
  --client_config=$DIR/trpc_cpp_fiber.yaml \
  --addr=127.0.0.1:24856 \
  --path=/sse/test

