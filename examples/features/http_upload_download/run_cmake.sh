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
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
mkdir -p examples/features/http_upload_download/build && cd examples/features/http_upload_download/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

# run server.
echo "kill previous process: http_upload_download_server"
killall http_upload_download_server
sleep 1

# Prepares some files to store data.
dd if=/dev/urandom of=download_src.bin bs=1M count=10
dd if=/dev/urandom of=upload_dst.bin bs=1M count=0

echo "try to start..."
examples/features/http_upload_download/build/http_upload_download_server --download_src_path="download_src.bin" --upload_dst_path="upload_dst.bin" --config=examples/features/http_upload_download/server/trpc_cpp_fiber.yaml &
http_server_pid=$(ps -ef | grep 'examples/features/http_upload_download/build/http_upload_download_server' | grep -v grep | awk '{print $2}')
if [ -n "http_server_pid" ]; then
  echo "start successfully"
  echo "http_upload_download_server is running, pid = $http_server_pid"
else
  echo "start failed"
  exit -1
fi

sleep 2

# Prepares some files to store data.
dd if=/dev/urandom of=download_dst.bin bs=1M count=0
dd if=/dev/urandom of=upload_src.bin bs=1M count=10

# run client.
examples/features/http_upload_download/build/download_client --dst_path="download_dst.bin" --client_config=examples/features/http_upload_download/client/trpc_cpp_fiber.yaml
examples/features/http_upload_download/build/upload_client --src_path="upload_src.bin" --use_chunked=true --client_config=examples/features/http_upload_download/client/trpc_cpp_fiber.yaml
examples/features/http_upload_download/build/upload_client --src_path="upload_src.bin" --use_chunked=false --client_config=examples/features/http_upload_download/client/trpc_cpp_fiber.yaml

killall http_upload_download_server
