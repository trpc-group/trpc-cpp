bazel build //examples/helloworld/...
bazel build //examples/features/trpc_transparent/...

echo "begin"
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/trpc_transparent/proxy/transparentserver --config=./examples/features/trpc_transparent/proxy/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/trpc_transparent/client/client --client_config=./examples/features/trpc_transparent/client/trpc_cpp_fiber.yaml

killall transparentserver
sleep 1

./bazel-bin/examples/features/trpc_transparent/proxy/transparentserver --config=./examples/features/trpc_transparent/proxy/trpc_cpp_future.yaml &
sleep 1
./bazel-bin/examples/features/trpc_transparent/client/client --client_config=./examples/features/trpc_transparent/client/trpc_cpp_fiber.yaml

killall transparentserver
killall helloworld_svr
