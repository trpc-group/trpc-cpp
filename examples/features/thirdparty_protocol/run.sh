bazel build //examples/features/thirdparty_protocol/...

echo "begin"
./bazel-bin/examples/features/thirdparty_protocol/server/demo_server --config=./examples/features/thirdparty_protocol/server/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/thirdparty_protocol/client/client --client_config=./examples/features/thirdparty_protocol/client/trpc_cpp_fiber.yaml
killall demo_server
