# Thirdparty Protocol Demo.

The server invocation relationship is as follows:

client <---> server

In common dir code, we implement thirdparty protocol.

## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/thirdparty_protocol/
examples/features/thirdparty_protocol/
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp_fiber.yaml
├── CMakeLists.txt
├── common
│   ├── BUILD
│   ├── demo_client_codec.cc
│   ├── demo_client_codec.h
│   ├── demo_protocol.cc
│   ├── demo_protocol.h
│   ├── demo_server_codec.cc
│   └── demo_server_codec.h
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── BUILD
    ├── demo_server.cc
    └── trpc_cpp_fiber.yaml
```

We can use the following script to quickly compile and run a program.
```shell
sh ./examples/features/thirdparty_protocol/run.sh
```

* Compilation

We can run the following command to compile the client and server programs.

```shell
$ bazel build //examples/features/thirdparty_protocol/...
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/thirdparty_protocol
$ mkdir -p examples/features/thirdparty_protocol/build && cd examples/features/thirdparty_protocol/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server program

We can run the following command to start the server program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
$ ./bazel-bin/examples/features/thirdparty_protocol/server/demo_server --config=./examples/features/thirdparty_protocol/server/trpc_cpp_fiber.yaml &
```

* Run the client program

We can run the following command to start the client program.

```shell
$ ./bazel-bin/examples/features/thirdparty_protocol/client/client --client_config=./examples/features/thirdparty_protocol/client/trpc_cpp_fiber.yaml 
```

The content of the output from the client program is as follows:
``` text
response: hello world
```