# Flatbuffers Demo Server.

Flatbuffers is similar to protobuf, it has efficient memory access characteristics and meets some specific scenarios that require high performance, such as game scenarios.

Here, we show how to use flatbuffers IDL to develop server and invoke it.

The server invocation relationship is as follows:

client <---> proxy <---> server

## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/trpc_flatbuffers/
examples/features/trpc_flatbuffers/
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp_fiber.yaml
├── CMakeLists.txt
├── proxy
│   ├── BUILD
│   ├── forward.fbs
│   ├── forward_server.cc
│   ├── forward_server.h
│   ├── forward_service.cc
│   ├── forward_service.h
│   ├── greeter.fbs
│   └── trpc_cpp_fiber.yaml
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── BUILD
    ├── demo_server.cc
    ├── demo_server.h
    ├── demo_service.cc
    ├── demo_service.h
    ├── greeter.fbs
    └── trpc_cpp_fiber.yaml
```

We can run the following command to compile the client and server programs.

```shell
$ ./examples/features/trpc_flatbuffers/run.sh
```

* Compilation

Run the following command to compile the client and server programs.
```shell
$ bazel build //examples/features/trpc_flatbuffers/...
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/trpc_json
$ mkdir -p examples/features/trpc_flatbuffers/build && cd examples/features/trpc_flatbuffers/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server program

Run the following commands to start the server program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
# start server
$ ./bazel-bin/examples/features/trpc_flatbuffers/server/demoserver --config=./examples/features/trpc_flatbuffers/server/trpc_cpp_fiber.yaml
```

```shell
# start forward server
$ ./bazel-bin/examples/features/trpc_flatbuffers/proxy/forwardserver --config=./examples/features/trpc_flatbuffers/proxy/trpc_cpp_fiber.yaml
```

* Run the client program

Run the following command to start the client program.

```shell
$ ./bazel-bin/examples/features/trpc_flatbuffers/client/client --client_config=./examples/features/trpc_flatbuffers/client/trpc_cpp_fiber.yaml
```

The content of the output from the client program is as follows:
``` text
request msg: hello world
response msg: hello world
```
