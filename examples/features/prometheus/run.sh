bazel build //examples/helloworld/...

cat > WORKSPACE <<EOF
workspace(name = "trpc_cpp")

load("//trpc:workspace.bzl", "trpc_workspace")

trpc_workspace()

load("@com_github_jupp0r_prometheus_cpp//bazel:repositories.bzl", "prometheus_cpp_repositories")

prometheus_cpp_repositories()
EOF

bazel build //examples/features/prometheus/... --define trpc_include_prometheus=true

echo "begin"
./bazel-bin/examples/helloworld/helloworld_svr --config=examples/helloworld/conf/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/prometheus/proxy/forward_server --config=examples/features/prometheus/proxy/trpc_cpp_fiber.yaml &
sleep 1
./bazel-bin/examples/features/prometheus/client/client --client_config=examples/features/prometheus/client/trpc_cpp_fiber.yaml

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
