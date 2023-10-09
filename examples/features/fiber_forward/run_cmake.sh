#!/bin/bash

mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/fiber_forward/build && cd examples/features/fiber_forward/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

echo "begin"
examples/helloworld/build/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml &
sleep 1
examples/features/fiber_forward/build/fiber_forward --config=./examples/features/fiber_forward/proxy/trpc_cpp_fiber.yaml &
sleep 1
examples/features/fiber_forward/build/client --client_config=./examples/features/fiber_forward/client/trpc_cpp_fiber.yaml
killall helloworld_svr
if [ $? -ne 0 ]; then
  echo "helloworld_svr exit error"
  exit -1
fi

killall fiber_forward
if [ $? -ne 0 ]; then
  echo "fiber_forward exit error"
  exit -1
fi
