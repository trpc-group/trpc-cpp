bazel build //examples/features/trpc_noop/...

echo "begin"
./bazel-bin/examples/features/trpc_noop/server/demo_server --config=./examples/features/trpc_noop/server/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/trpc_noop/client/client --client_config=./examples/features/trpc_noop/client/trpc_cpp_fiber.yaml
killall demo_server
