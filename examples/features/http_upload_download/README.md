# HTTP streaming calling (Upload/Download).

In HTTP streaming calling, both the client and the server can upload or download a big file.
Runtime requirements: Please use `Fiber` thread model.

## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/http_upload_download/
examples/features/http_upload_download/
├── client
│   ├── BUILD
│   ├── download_client.cc
│   ├── trpc_cpp_fiber.yaml
│   └── upload_client.cc
├── CMakeLists.txt
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── BUILD
    ├── file_storage_handler.cc
    ├── file_storage_handler.h
    ├── http_server.cc
    └── trpc_cpp_fiber.yaml
```

We can use the following script to quickly compile and run a program.
```shell
sh examples/features/http_upload_download/run.sh
```

* Compilation

We can run the following command to compile the client and server programs.

```shell
bazel build  //examples/features/http_upload_download/server:http_upload_download_server
bazel build  //examples/features/http_upload_download/client:download_client
bazel build  //examples/features/http_upload_download/client:upload_client
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/http_upload_download
$ mkdir -p examples/features/http_upload_download/build && cd examples/features/http_upload_download/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server program

We can run the following command to start the server program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
# Prepares some files to store data.
dd if=/dev/urandom of=download_src.bin bs=1M count=10
dd if=/dev/urandom of=upload_dst.bin bs=1M count=0

# Starts the HTTP server.
bazel-bin/examples/features/http_upload_download/server/http_upload_download_server --download_src_path="download_src.bin" --upload_dst_path="upload_dst.bin" --config=examples/features/http_upload_download/server/trpc_cpp_fiber.yaml
```

* Run the client program

We can run the following command to start the client program.

```shell
# Prepares some files to store data.
dd if=/dev/urandom of=download_dst.bin bs=1M count=0
dd if=/dev/urandom of=upload_src.bin bs=1M count=10

# Downloads a file by download_client.
bazel-bin/examples/features/http_upload_download/client/download_client --dst_path="download_dst.bin" --client_config=examples/features/http_upload_download/client/trpc_cpp_fiber.yaml 
# Downloads a file by curl (optional).
curl http://127.0.0.1:24858/download -o curl_download_dst.bin

# Uploads a file by upload_client.
bazel-bin/examples/features/http_upload_download/client/upload_client --src_path="upload_src.bin" --use_chunked=true --client_config=examples/features/http_upload_download/client/trpc_cpp_fiber.yaml 
bazel-bin/examples/features/http_upload_download/client/upload_client --src_path="upload_src.bin" --use_chunked=false --client_config=examples/features/http_upload_download/client/trpc_cpp_fiber.yaml 
# Uploads a file by curl (optional).
curl http://127.0.0.1:24858/upload -v -T upload_src.bin -X POST  -H "Transfer-Encoding: chunked"
curl http://127.0.0.1:24858/upload -v -T upload_src.bin -X POST
```

The content of the output from the client program is as follows:
``` text
// Downlaods a file from the server:
finish downloading, read size: 10485760
name: download a file from the server, ok: 1
final result of http calling: 1

// Uploads a file to the server:
upload a file with chunked
finish uploading, write size: 10485760
name: upload a file to server, ok: 1
final result of http calling: 1

upload a file with content-length
finish uploading, write size: 10485760
name: upload a file to server, ok: 1
final result of http calling: 1
```