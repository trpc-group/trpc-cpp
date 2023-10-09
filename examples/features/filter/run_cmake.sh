#!/bin/bash
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/filter/build && cd examples/features/filter/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

# start server and client
./examples/features/filter/build/demo_server --config=./examples/features/filter/server/trpc_cpp_fiber.yaml &
sleep 1
./examples/features/filter/build/demo_client --client_config=./examples/features/filter/client/trpc_cpp_separate.yaml

killall demo_server