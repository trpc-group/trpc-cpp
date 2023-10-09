mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/trpc_attachment/build && cd examples/features/trpc_attachment/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

examples/features/trpc_attachment/build/demo_server --config=./examples/features/trpc_attachment/server/trpc_cpp_fiber.yaml &
sleep 1
examples/features/trpc_attachment/build/client --client_config=./examples/features/trpc_attachment/client/trpc_cpp_fiber.yaml

killall demo_server
