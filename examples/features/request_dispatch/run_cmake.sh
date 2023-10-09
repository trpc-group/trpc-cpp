mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/request_dispatch/build && cd examples/features/request_dispatch/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

examples/features/request_dispatch/build/demo_server --config=./examples/features/request_dispatch/server/trpc_cpp_separate.yaml &
sleep 1
examples/features/request_dispatch/build/fiber_client --client_config=./examples/features/request_dispatch/client/trpc_cpp_fiber.yaml

killall demo_server

examples/features/request_dispatch/build/demo_server --config=./examples/features/request_dispatch/server/trpc_cpp_fiber.yaml &
sleep 1
examples/features/request_dispatch/build/fiber_client --client_config=./examples/features/request_dispatch/client/trpc_cpp_fiber.yaml

killall demo_server

examples/features/request_dispatch/build/demo_server --config=./examples/features/request_dispatch/server/trpc_cpp_fiber.yaml &
sleep 1
examples/features/request_dispatch/build/future_client --client_config=./examples/features/request_dispatch/client/trpc_cpp_future.yaml

killall demo_server
