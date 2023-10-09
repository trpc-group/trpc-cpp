#! /bin/bash

cd ../../../../

bazel build examples/helloworld/...
bazel build examples/features/fiber_forward/...
bazel build examples/features/future_forward/...

cd test/end2end/gracefully_stop/forward/

cp -rf ../../../../bazel-bin/examples/helloworld/helloworld_svr .
cp -rf ../../../../bazel-bin/examples/features/fiber_forward/proxy/fiber_forward fiber_forward_svr
cp -rf ../../../../bazel-bin/examples/features/future_forward/proxy/future_forward future_forward_svr

cp -rf ../../../../bazel-bin/examples/features/fiber_forward/client/client fiber_client
cp -rf ../../../../examples/features/fiber_forward/client/trpc_cpp_fiber.yaml .

cp -rf ../../../../bazel-bin/examples/features/future_forward/client/client future_client
cp -rf ../../../../examples/features/future_forward/client/trpc_cpp_future.yaml .

# [future_client or fiber_client]->[fiber_forward_svr or future_forward_svr]->[helloworld_svr]

ulimit -c unlimited

function check_client_test_success {
  if [ $1 -ne 0 ]; then
      echo "Client test failed"
      exit -1
  fi
}

#[tcp,udp]
#[future_forward_svr,fiber_forward_svr]
#[forward_svr_xxx.yaml]
function gracefully_stop {
  killall helloworld_svr
  sleep 1

  # start helloworld_svr
  if [ $1 == "tcp" ]; then
      ./helloworld_svr --config=helloworld_svr_conf/trpc_cpp_tcp.yaml > helloworld.out &
      sleep 1
  fi
  if [ $1 == "udp" ]; then
      ./helloworld_svr --config=helloworld_svr_conf/trpc_cpp_udp.yaml > helloworld.out &
      sleep 1
  fi

  # ensure helloworld_svr start success
  pid=`ps aux |grep "helloworld_svr" |grep -v grep|awk '{print $2}'`
  if [ "$pid" == "" ];then
    echo "[helloworld_svr $1] start fail"
    exit -1
  fi

  killall $2
  sleep 1

  # start forward
  ./$2 --config=$3 > forward_svr.out &
  sleep 1

  # ensure forward_svr start success
  pid=`ps aux |grep "$2" |grep -v grep|awk '{print $2}'`
  if [ "$pid" == "" ];then
    echo "[$2 --config=$3] start fail"
    exit -1
  fi

  # rpc test
  ./future_client --client_config=trpc_cpp_future.yaml
  check_client_test_success $?
  sleep 1

  ./fiber_client --client_config=trpc_cpp_fiber.yaml
  check_client_test_success $?
  sleep 1

  # kill -12 pid
  pid=`ps aux |grep "$2" |grep -v grep|awk '{print $2}'`
  if [ "$pid" == "" ];then
    echo "[$2 --config=$3] error exit"
    exit -1

    else
      kill -12 "$pid"

      # kill -12 should return 0
      if [ $? -ne 0 ]; then
        echo "kill -12 [$2 --config=$3] error"
        exit -1
      fi

      # The original process should no longer exist
      killed_pid=`ps aux |grep {pid} |grep -v grep|awk '{print $2}'`
      if [ "$killed_pid" != "" ];then
        echo "[$2 --config=$3] still exists"
        exit -1
      fi

      # And it should not generate a core file
      core_file=`ls |grep "core." |grep {pid} |grep -v grep|awk '{print $1}'`
      if [ "$core_file" != "" ];then
        echo "[$2 --config=$3] get core file"
        exit -1
      fi

      echo "gracefully_stop [$2 --config=$3] success"
  fi

  killall helloworld_svr
  killall $2
}


# Using Future with ServiceProxy for TCP
gracefully_stop tcp future_forward_svr future/tcp/trpc_cpp_separate_io_1_handle_1_conn_complex.yaml
gracefully_stop tcp future_forward_svr future/tcp/trpc_cpp_separate_io_1_handle_1_conn_pool.yaml
gracefully_stop tcp future_forward_svr future/tcp/trpc_cpp_separate_io_2_handle_2_conn_complex.yaml
gracefully_stop tcp future_forward_svr future/tcp/trpc_cpp_separate_io_2_handle_2_conn_pool.yaml
gracefully_stop tcp future_forward_svr future/tcp/trpc_cpp_merge_iohandle_1_conn_complex.yaml
gracefully_stop tcp future_forward_svr future/tcp/trpc_cpp_merge_iohandle_1_conn_pool.yaml
gracefully_stop tcp future_forward_svr future/tcp/trpc_cpp_merge_iohandle_2_conn_complex.yaml
gracefully_stop tcp future_forward_svr future/tcp/trpc_cpp_merge_iohandle_2_conn_pool.yaml

# Using Fiber with ServiceProxy for TCP
gracefully_stop tcp fiber_forward_svr fiber/tcp/trpc_cpp_fiber_concurrency_1_conn_complex.yaml
gracefully_stop tcp fiber_forward_svr fiber/tcp/trpc_cpp_fiber_concurrency_1_conn_pool.yaml
gracefully_stop tcp fiber_forward_svr fiber/tcp/trpc_cpp_fiber_concurrency_4_conn_complex.yaml
gracefully_stop tcp fiber_forward_svr fiber/tcp/trpc_cpp_fiber_concurrency_4_conn_pool.yaml

# Using Future with ServiceProxy for UDP
gracefully_stop udp future_forward_svr future/udp/trpc_cpp_separate_io_1_handle_1_conn_complex.yaml
gracefully_stop udp future_forward_svr future/udp/trpc_cpp_separate_io_1_handle_1_conn_pool.yaml
gracefully_stop udp future_forward_svr future/udp/trpc_cpp_separate_io_2_handle_2_conn_complex.yaml
gracefully_stop udp future_forward_svr future/udp/trpc_cpp_separate_io_2_handle_2_conn_pool.yaml
gracefully_stop udp future_forward_svr future/udp/trpc_cpp_merge_iohandle_1_conn_complex.yaml
gracefully_stop udp future_forward_svr future/udp/trpc_cpp_merge_iohandle_1_conn_pool.yaml
gracefully_stop udp future_forward_svr future/udp/trpc_cpp_merge_iohandle_2_conn_complex.yaml
gracefully_stop udp future_forward_svr future/udp/trpc_cpp_merge_iohandle_2_conn_pool.yaml

# Using Fiber with ServiceProxy for UDP
gracefully_stop udp fiber_forward_svr fiber/udp/trpc_cpp_fiber_concurrency_1_conn_complex.yaml
gracefully_stop udp fiber_forward_svr fiber/udp/trpc_cpp_fiber_concurrency_1_conn_pool.yaml
gracefully_stop udp fiber_forward_svr fiber/udp/trpc_cpp_fiber_concurrency_4_conn_complex.yaml
gracefully_stop udp fiber_forward_svr fiber/udp/trpc_cpp_fiber_concurrency_4_conn_pool.yaml

# Check that it does not generate a core file
core_file=`ls |grep "core.*"|grep -v grep|awk '{print $1}'`
if [ "$core_file" != "" ];then
  echo "get core file"
  exit -1
fi