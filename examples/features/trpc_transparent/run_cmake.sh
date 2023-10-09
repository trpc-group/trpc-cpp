mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/trpc_transparent/build && cd examples/features/trpc_transparent/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

echo "begin"
examples/helloworld/build/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml &
sleep 1
examples/features/trpc_transparent/build/transparentserver --config=./examples/features/trpc_transparent/proxy/trpc_cpp_fiber.yaml &
sleep 1
examples/features/trpc_transparent/build/client --client_config=./examples/features/trpc_transparent/client/trpc_cpp_fiber.yaml

killall transparentserver
sleep 1

examples/features/trpc_transparent/build/transparentserver --config=./examples/features/trpc_transparent/proxy/trpc_cpp_future.yaml &
sleep 1
examples/features/trpc_transparent/build/client --client_config=./examples/features/trpc_transparent/client/trpc_cpp_fiber.yaml

killall transparentserver
killall helloworld_svr
