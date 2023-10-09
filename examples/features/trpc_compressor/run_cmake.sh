mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/trpc_compressor/build && cd examples/features/trpc_compressor/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

examples/helloworld/build/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml &
sleep 1
examples/features/trpc_compressor/build/client --client_config=./examples/features/trpc_compressor/client/trpc_cpp_fiber.yaml

killall helloworld_svr
