#!/bin/bash

bazel build //examples/helloworld/...

echo "testing server at fiber runtime"
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/helloworld/test/fiber_client --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
killall helloworld_svr
echo "testing server at thread runtime"
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp.yaml &
sleep 1
./bazel-bin/examples/helloworld/test/future_client --client_config=./examples/helloworld/test/conf/trpc_cpp_future.yaml
killall helloworld_svr
