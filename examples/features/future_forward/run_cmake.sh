mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/future_forward/build && cd examples/features/future_forward/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

echo "begin"
./examples/helloworld/build/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp.yaml &
sleep 1
./examples/features/future_forward/build/future_forward --config=./examples/features/future_forward/proxy/trpc_cpp_future.yaml &
sleep 1
./examples/features/future_forward/build/client --client_config=./examples/features/future_forward/client/trpc_cpp_future.yaml
killall helloworld_svr
if [ $? -ne 0 ]; then
  echo "helloworld_svr exit error"
  exit -1
fi

killall future_forward
if [ $? -ne 0 ]; then
  echo "future_forward exit error"
  exit -1
fi
