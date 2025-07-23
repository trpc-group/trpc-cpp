#
#
# Tencent is pleased to support the open source community by making tRPC available.
#
# Copyright (C) 2023 Tencent.
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
mkdir -p examples/features/http_async_upload_download/build && cd examples/features/http_async_upload_download/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

# run server.
echo "kill previous process: http_async_upload_download_server"
killall http_async_upload_download_server
sleep 1

# Prepares some files to store data.
dd if=/dev/urandom of=async_download_src.bin bs=1M count=10
dd if=/dev/urandom of=async_upload_dst.bin bs=1M count=0

echo "try to start..."
examples/features/http_async_upload_download/build/http_async_upload_download_server --download_src_path="async_download_src.bin" --upload_dst_path="async_upload_dst.bin" --config=examples/features/http_async_upload_download/server/trpc_cpp_merge.yaml &
http_server_pid=$(ps -ef | grep 'examples/features/http_async_upload_download/build/http_async_upload_download_server' | grep -v grep | awk '{print $2}')
if [ -n "http_server_pid" ]; then
  echo "start successfully"
  echo "http_async_upload_download_server is running, pid = $http_server_pid"
else
  echo "start failed"
  exit -1
fi

sleep 2

# Prepares some files to store data.
dd if=/dev/urandom of=download_dst.bin bs=1M count=0
dd if=/dev/urandom of=upload_src.bin bs=1M count=10

# run client.
examples/features/http_upload_download/build/download_client --dst_path="download_dst.bin" --addr="127.0.0.1:24859" --client_config=examples/features/http_upload_download/client/trpc_cpp_fiber.yaml
examples/features/http_upload_download/build/upload_client --src_path="upload_src.bin" --addr="127.0.0.1:24859" --use_chunked=true --client_config=examples/features/http_upload_download/client/trpc_cpp_fiber.yaml

killall http_async_upload_download_server
