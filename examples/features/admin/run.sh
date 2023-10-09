#!/bin/bash

bazel build //examples/helloworld/...
bazel build //examples/features/admin/proxy/...
bazel build //examples/features/admin/client/...

echo "begin"
./bazel-bin/examples/helloworld/helloworld_svr --config=examples/helloworld/conf/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/admin/proxy/forward_server --config=examples/features/admin/proxy/trpc_cpp.yaml &
sleep 1
./bazel-bin/examples/features/admin/client/client --client_config=examples/features/admin/client/trpc_cpp.yaml

killall helloworld_svr
if [ $? -ne 0 ]; then
  echo "helloworld_svr exit error"
  exit -1
fi

killall forward_server
if [ $? -ne 0 ]; then
  echo "forward_server exit error"
  exit -1
fi
