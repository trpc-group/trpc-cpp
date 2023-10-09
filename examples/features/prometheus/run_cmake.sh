rm -rf build
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
rm -rf build
mkdir -p build && cd build && cmake -DTRPC_BUILD_WITH_METRICS_PROMETHEUS=ON -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/prometheus/build && cd examples/features/prometheus/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
rm -rf build

echo "begin"
examples/helloworld/build/helloworld_svr --config=examples/helloworld/conf/trpc_cpp_fiber.yaml &
sleep 1
examples/features/prometheus/build/forward_server --config=examples/features/prometheus/proxy/trpc_cpp_fiber.yaml &
sleep 1
examples/features/prometheus/build/client --client_config=examples/features/prometheus/client/trpc_cpp_fiber.yaml

killall helloworld_svr
if [ $? -ne 0 ]; then
  echo "helloworld_svr exit error"
  exit -1
fi

killall forward_server
if [ $? -ne 0 ]; then
  echo "forward_server exit error"
  exit -1
fi
