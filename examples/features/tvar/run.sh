bazel build //examples/features/tvar/client/...
bazel build //examples/features/tvar/server/...

echo "begin"
./bazel-bin/examples/features/tvar/server/tvar_server --config=./examples/features/tvar/server/trpc_cpp_server.yaml &
sleep 1
./bazel-bin/examples/features/tvar/client/client --client_config=./examples/features/tvar/client/trpc_cpp_client.yaml

curl http://127.0.0.1:31111/cmds/var/counter
curl http://127.0.0.1:31111/cmds/var/counter?history=true

curl http://127.0.0.1:31111/cmds/var/gauge
curl http://127.0.0.1:31111/cmds/var/gauge?history=true

curl http://127.0.0.1:31111/cmds/var/miner

curl http://127.0.0.1:31111/cmds/var/maxer

curl http://127.0.0.1:31111/cmds/var/status
curl http://127.0.0.1:31111/cmds/var/status?history=true

curl http://127.0.0.1:31111/cmds/var/passivestatus
curl http://127.0.0.1:31111/cmds/var/passivestatus?history=true

curl http://127.0.0.1:31111/cmds/var/averager

curl http://127.0.0.1:31111/cmds/var/intrecorder

curl http://127.0.0.1:31111/cmds/var/window
curl http://127.0.0.1:31111/cmds/var/window?history=true

curl http://127.0.0.1:31111/cmds/var/persecond
curl http://127.0.0.1:31111/cmds/var/persecond?history=true

curl http://127.0.0.1:31111/cmds/var/latencyrecorder
curl http://127.0.0.1:31111/cmds/var/latencyrecorder?history=true

killall tvar_server
if [ $? -ne 0 ]; then
  echo "tvar_server exit error"
  exit -1
fi
