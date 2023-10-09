mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/trpc_noop/build && cd examples/features/trpc_noop/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

echo "begin"
examples/features/trpc_noop/build/demo_server --config=./examples/features/trpc_noop/server/trpc_cpp_fiber.yaml &
sleep 1
examples/features/trpc_noop/build/client --client_config=./examples/features/trpc_noop/client/trpc_cpp_fiber.yaml
killall demo_server
