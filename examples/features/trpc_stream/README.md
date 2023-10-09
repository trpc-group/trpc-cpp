# tRPC streaming RPC calling.

A client-side streaming RPC where the client writes a sequence of messages and sends them to the server,
again using a provided stream. Once the client has finished writing messages, it waits for the server to read
them all and returns its response.

A server-side streaming RPC where the client sends a request to the server and gets a stream to read
a sequence of message back. The client reads from the returned stream until there no more messages.

A bidirectional streaming RPC where both sides send a sequence of messages using a read-write stream.
The two streams operate independently, so clients and servers can read and write in whatever order they like.

Runtime requirements: Please use `Fiber` thread model.

## Usage

We can use the following command to view the directory tree.

```shell
$ tree examples/features/trpc_stream/
examples/features/trpc_stream/
├── client
│   ├── BUILD
│   ├── client.cc
│   ├── rawdata_stream_client.cc
│   └── trpc_cpp_fiber.yaml
├── CMakeLists.txt
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── BUILD
    ├── stream.proto
    ├── stream_server.cc
    ├── stream_service.cc
    ├── stream_service.h
    └── trpc_cpp_fiber.yaml
```

We can use the following script to quickly compile and run a program.

```shell
sh examples/features/trpc_stream/run.sh
```

* Compilation

We can run the following command to compile the client and server programs.

```shell
bazel build  //examples/features/trpc_stream/server:trpc_stream_server
bazel build  //examples/features/trpc_stream/client:client
bazel build  //examples/features/trpc_stream/client:rawdata_stream_client
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/trpc_stream
$ mkdir -p examples/features/trpc_stream/build && cd examples/features/trpc_stream/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server program

We can run the following command to start the server program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
bazel-bin/examples/features/trpc_stream/server/trpc_stream_server --config=examples/features/trpc_stream/server/trpc_cpp_fiber.yaml &
```

* Run the client program

We can run the following command to start the client program.

```shell
bazel-bin/examples/features/trpc_stream/client/client --client_config=examples/features/trpc_stream/client/trpc_cpp_fiber.yaml --rpc_method=ClientStreamSayHello
```

The content of the output from the client program is as follows:

``` text
name: Streaming RPC, ClientStreamSayHello1, ok: 1
name: Streaming RPC, ClientStreamSayHello2, ok: 1
name: Streaming RPC, ClientStreamSayHello3, ok: 1
name: Streaming RPC, ClientStreamSayHello4, ok: 1
name: Streaming RPC, ClientStreamSayHello5, ok: 1
name: Streaming RPC, ClientStreamSayHello6, ok: 1
name: Streaming RPC, ClientStreamSayHello7, ok: 1
name: Streaming RPC, ClientStreamSayHello8, ok: 1
final result of streaming RPC calling: 1
```

Other streaming RPC testings.

```shell
bazel-bin/examples/features/trpc_stream/client/client --client_config=examples/features/trpc_stream/client/trpc_cpp_fiber.yaml --rpc_method=ServerStreamSayHello
bazel-bin/examples/features/trpc_stream/client/client --client_config=examples/features/trpc_stream/client/trpc_cpp_fiber.yaml --rpc_method=BidiStreamSayHello
bazel-bin/examples/features/trpc_stream/client/rawdata_stream_client --client_config=examples/features/trpc_stream/client/trpc_cpp_fiber.yaml --rpc_method=RawDataReadWrite
```
