bazel build //examples/features/trpc_flatbuffers/...

echo "begin"
./bazel-bin/examples/features/trpc_flatbuffers/server/demoserver --config=./examples/features/trpc_flatbuffers/server/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/trpc_flatbuffers/proxy/forwardserver --config=./examples/features/trpc_flatbuffers/proxy/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/trpc_flatbuffers/client/client --client_config=./examples/features/trpc_flatbuffers/client/trpc_cpp_fiber.yaml
killall demoserver
killall forwardserver
