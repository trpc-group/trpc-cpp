#! /bin/bash

cd ../../../../

bazel build examples/helloworld/...
 
cd test/end2end/gracefully_stop/helloworld/

cp -rf ../../../../bazel-bin/examples/helloworld/helloworld_svr .
cp -rf ../../../../bazel-bin/examples/helloworld/test/future_client .
cp -rf ../../../../bazel-bin/examples/helloworld/test/fiber_client .
cp -rf ../../../../examples/helloworld/test/conf/trpc_cpp_fiber.yaml .
cp -rf ../../../../examples/helloworld/test/conf/trpc_cpp_future.yaml .

ulimit -c unlimited

function check_client_test_success {
  if [ $1 -ne 0 ]; then
      echo "Client test failed"
      exit -1
  fi
}

function gracefully_stop {
  killall helloworld_svr
  sleep 1

  # start helloworld_svr
  ./helloworld_svr --config=$1 > helloworld.out &
  sleep 1

  # rpc test
  ./future_client --client_config=trpc_cpp_future.yaml
  check_client_test_success $?
  sleep 1

  ./fiber_client --client_config=trpc_cpp_fiber.yaml
  check_client_test_success $?
  sleep 1

  # kill -12 pid
  pid=`ps aux |grep "helloworld_svr" |grep -v grep|awk '{print $2}'`
  if [ "$pid" == "" ];then
    echo "[helloworld_svr --config=$1] error exit"
    exit -1

    else
      kill -12 "$pid"

      # kill -12 should return 0
      if [ $? -ne 0 ]; then
        echo "kill -12 [helloworld_svr --config=$1] error"
        exit -1
      fi

      # The original process should no longer exist
      killed_pid=`ps aux |grep {pid} |grep -v grep|awk '{print $2}'`
      if [ "$killed_pid" != "" ];then
        echo "[helloworld_svr --config=$1] still exists"
        exit -1
      fi

      # And it should not generate a core file
      core_file=`ls |grep "core." |grep {pid} |grep -v grep|awk '{print $1}'`
      if [ "$core_file" != "" ];then
        echo "[helloworld_svr --config=$1] get core file"
        exit -1
      fi

      echo "gracefully_stop [helloworld_svr --config=$1] success"
  fi

  killall helloworld_svr
}


gracefully_stop default/trpc_cpp_separate_io_1_handle_1.yaml
gracefully_stop default/trpc_cpp_separate_io_2_handle_2.yaml
gracefully_stop default/trpc_cpp_merge_iohandle_1.yaml
gracefully_stop default/trpc_cpp_merge_iohandle_2.yaml

gracefully_stop fiber/trpc_cpp_fiber_concurrency_1.yaml
gracefully_stop fiber/trpc_cpp_fiber_concurrency_4.yaml

# Check that it does not generate a core file.
core_file=`ls |grep "core.*"|grep -v grep|awk '{print $1}'`
if [ "$core_file" != "" ];then
  echo "get core file"
  exit -1
fi