bazel build //examples/helloworld/...
bazel build //examples/features/rpcz/client/...
bazel build //examples/features/rpcz/proxy/... --define trpc_include_rpcz=true

echo "begin"
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp.yaml &
sleep 1
./bazel-bin/examples/features/rpcz/proxy/rpcz_proxy --config=./examples/features/rpcz/proxy/trpc_cpp_proxy.yaml &
sleep 1
./bazel-bin/examples/features/rpcz/client/client --client_config=./examples/features/rpcz/client/trpc_cpp_client.yaml

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
