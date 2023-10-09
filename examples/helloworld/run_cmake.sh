#!/bin/bash

mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/helloworld/build && cd examples/helloworld/build &&cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

echo "testing server at fiber runtime"
./examples/helloworld/build/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml &
sleep 1
./examples/helloworld/build/fiber_client --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
killall helloworld_svr
echo "testing server at thread runtime"
./examples/helloworld/build/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp.yaml &
sleep 1
./examples/helloworld/build/future_client --client_config=./examples/helloworld/test/conf/trpc_cpp_future.yaml
killall helloworld_svr
