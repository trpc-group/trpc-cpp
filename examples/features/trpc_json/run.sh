bazel build //examples/features/trpc_json/...

echo "begin"
./bazel-bin/examples/features/trpc_json/server/demo_server --config=./examples/features/trpc_json/server/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/trpc_json/client/client --client_config=./examples/features/trpc_json/client/trpc_cpp_fiber.yaml
killall demo_server
