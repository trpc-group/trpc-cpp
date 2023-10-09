#! /bin/bash

function check_success {
  if [ $1 -ne 0 ]; then
      echo "Failed"
      exit -1
  fi
}

cd helloworld
chmod +x run_test.sh
./run_test.sh
check_success $?

cd ../forward
chmod +x run_test.sh
./run_test.sh
check_success $?