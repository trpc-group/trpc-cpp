bazel build //examples/features/request_dispatch/...

./bazel-bin/examples/features/request_dispatch/server/demo_server --config=./examples/features/request_dispatch/server/trpc_cpp_separate.yaml &
sleep 1
./bazel-bin/examples/features/request_dispatch/client/fiber_client --client_config=./examples/features/request_dispatch/client/trpc_cpp_fiber.yaml

killall demo_server

./bazel-bin/examples/features/request_dispatch/server/demo_server --config=./examples/features/request_dispatch/server/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/request_dispatch/client/fiber_client --client_config=./examples/features/request_dispatch/client/trpc_cpp_fiber.yaml

killall demo_server

./bazel-bin/examples/features/request_dispatch/server/demo_server --config=./examples/features/request_dispatch/server/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/request_dispatch/client/future_client --client_config=./examples/features/request_dispatch/client/trpc_cpp_future.yaml

killall demo_server
