#!/bin/bash

mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/admin/build && cd examples/features/admin/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

echo "begin"
./examples/helloworld/build/helloworld_svr --config=examples/helloworld/conf/trpc_cpp_fiber.yaml &
sleep 1
./examples/features/admin/build/forward_server --config=examples/features/admin/proxy/trpc_cpp.yaml &
sleep 1
./examples/features/admin/build/client --client_config=examples/features/admin/client/trpc_cpp.yaml

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
