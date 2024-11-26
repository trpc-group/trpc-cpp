killall helloworld_svr
if [ $? -ne 0 ]; then
	echo "helloworld_svr exit error"
fi

killall forward_server
if [ $? -ne 0 ]; then
	echo "forward_server exit error"
fi

bazel build //examples/helloworld/...
bazel build //examples/features/prometheus/... --define trpc_include_prometheus=true

# instance 1
echo "begin"
./bazel-bin/examples/helloworld/helloworld_svr --config=examples/helloworld/conf/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/prometheus/proxy/forward_server --config=examples/features/prometheus/proxy/trpc_cpp_fiber.yaml &

# instance 2
echo "begin"
./bazel-bin/examples/helloworld/helloworld_svr --config=examples/helloworld/conf/trpc_cpp_fiber2.yaml &
sleep 1
./bazel-bin/examples/features/prometheus/proxy/forward_server --config=examples/features/prometheus/proxy/trpc_cpp_fiber2.yaml &

sleep 1 
ss -antp | grep 8888
ss -antp | grep 8889
ss -antp | grep 8899 
ss -antp | grep 8898

