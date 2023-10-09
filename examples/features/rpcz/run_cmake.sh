mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DTRPC_BUILD_WITH_RPCZ=ON .. && make -j8 && cd -
mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/rpcz/build && cd examples/features/rpcz/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
rm -rf build

echo "begin"
examples/helloworld/build/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp.yaml &
sleep 1
examples/features/rpcz/build/rpcz_proxy --config=./examples/features/rpcz/proxy/trpc_cpp_proxy.yaml &
sleep 1
examples/features/rpcz/build/client --client_config=./examples/features/rpcz/client/trpc_cpp_client.yaml

# Get general rpcz span infos.
curl http://127.0.0.1:32111/cmds/rpcz

# Change span_id=1 to effective one fetched from above output.
curl http://127.0.0.1:32111/cmds/rpcz?span_id=1

killall helloworld_svr
if [ $? -ne 0 ]; then
  echo "helloworld_svr exit error"
  exit -1
fi

killall rpcz_proxy
if [ $? -ne 0 ]; then
  echo "rpcz_proxy exit error"
  exit -1
fi
