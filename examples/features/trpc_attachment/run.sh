bazel build //examples/features/trpc_attachment/...

./bazel-bin/examples/features/trpc_attachment/server/demo_server --config=./examples/features/trpc_attachment/server/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/trpc_attachment/client/client --client_config=./examples/features/trpc_attachment/client/trpc_cpp_fiber.yaml

killall demo_server
