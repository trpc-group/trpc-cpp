#
#
# Tencent is pleased to support the open source community by making tRPC available.
#
# Copyright (C) 2023 THL A29 Limited, a Tencent company.
# All rights reserved.
#
# If you have downloaded a copy of the tRPC source code from Tencent,
# please note that tRPC source code is licensed under the  Apache 2.0 License,
# A copy of the Apache 2.0 License is included in this file.
#
#

#! /bin/bash

# building.
bazel build  //examples/features/http/server:http_server
bazel build  //examples/features/http/client:client


# run server.
echo "kill previous process: http_server"
killall http_server
sleep 1
echo "try to start..."
bazel-bin/examples/features/http/server/http_server --config=examples/features/http/server/trpc_cpp_fiber.yaml &
http_server_pid=$(ps -ef | grep 'bazel-bin/examples/features/http/server/http_server' | grep -v grep | awk '{print $2}')
if [ -n "http_server_pid" ]; then
  echo "start successfully"
  echo "http_server is running, pid = $http_server_pid"
else
  echo "start failed"
  exit -1
fi

sleep 2

# run client.
bazel-bin/examples/features/http/client/client --client_config=examples/features/http/client/trpc_cpp_fiber.yaml
killall http_server
