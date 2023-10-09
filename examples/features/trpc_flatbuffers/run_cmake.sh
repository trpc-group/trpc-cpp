mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/trpc_flatbuffers/build && cd examples/features/trpc_flatbuffers/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

echo "begin"
examples/features/trpc_flatbuffers/build/demoserver --config=./examples/features/trpc_flatbuffers/server/trpc_cpp_fiber.yaml &
sleep 1
examples/features/trpc_flatbuffers/build/forwardserver --config=./examples/features/trpc_flatbuffers/proxy/trpc_cpp_fiber.yaml &
sleep 1
examples/features/trpc_flatbuffers/build/client --client_config=./examples/features/trpc_flatbuffers/client/trpc_cpp_fiber.yaml

killall demoserver
killall forwardserver
