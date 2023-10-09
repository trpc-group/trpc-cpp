#!/bin/bash

# build
bazel build //examples/features/filter/server:demo_server
bazel build //examples/features/filter/client:demo_client

# start server and client
./bazel-bin/examples/features/filter/server/demo_server --config=./examples/features/filter/server/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/filter/client/demo_client --client_config=./examples/features/filter/client/trpc_cpp_separate.yaml

killall demo_server