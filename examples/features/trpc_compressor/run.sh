bazel build //examples/helloworld/...
bazel build //examples/features/trpc_compressor/...

./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/trpc_compressor/client/client --client_config=./examples/features/trpc_compressor/client/trpc_cpp_fiber.yaml

killall helloworld_svr
