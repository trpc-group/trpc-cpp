# Compressor/Decompressor Demo.

The server invocation relationship is as follows:

client <---> server

`client` send msg to `server` use gzip/snappy/lz4.

## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/trpc_compressor/
examples/features/trpc_compressor/
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp_fiber.yaml
├── CMakeLists.txt
├── README.md
├── run_cmake.sh
└── run.sh
```

We can run the following command to compile the client and server programs.

```shell
$ ./examples/features/trpc_compressor/run.sh
```

* Compilation

Run the following command to compile the client and server programs.

```shell
$ bazel build //examples/helloworld/...
$ bazel build //examples/features/trpc_compressor/...
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/helloworld
$ mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/trpc_compressor
$ mkdir -p examples/features/trpc_compressor/build && cd examples/features/trpc_compressor/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server program

Run the following commands to start the server program.

*CMake build targets can be found at `build` of this directory and `examples/helloworld/build`, you can replace below server&client binary path when you use cmake to compile.*

```shell
$ ./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
```

* Run the client program

Run the following command to start the client program.

```shell
$ ./bazel-bin/examples/features/trpc_compressor/client/client --client_config=./examples/features/trpc_compressor/client/trpc_cpp_fiber.yaml
```

The content of the output from the client program is as follows:
``` text
response: msg: "hello, compressor msg"

response: msg: "hello, compressor msg"

response: msg: "hello, compressor msg"

```