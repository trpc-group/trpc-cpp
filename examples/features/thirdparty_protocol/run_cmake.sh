mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/thirdparty_protocol/build && cd examples/features/thirdparty_protocol/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

echo "begin"
examples/features/thirdparty_protocol/build/demo_server --config=./examples/features/thirdparty_protocol/server/trpc_cpp_fiber.yaml &
sleep 1
examples/features/thirdparty_protocol/build/client --client_config=./examples/features/thirdparty_protocol/client/trpc_cpp_fiber.yaml
killall demo_server
