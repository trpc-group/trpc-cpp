# tRPC streaming RPC calling used in forwarding-server scenario.

It's easy to transport streaming data by tRPC streaming RPC forwarding-server scenario. 
The calling chain is shown as follows.

```text
client <---> proxy <---> server
```

Runtime requirements: Please use `Fiber` thread model.

## Usage

We can use the following command to view the directory tree.

```shell
$ tree examples/features/trpc_stream_forward
examples/features/trpc_stream_forward
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp_fiber.yaml
├── CMakeLists.txt
├── proxy
│   ├── BUILD
│   ├── stream_forward.proto
│   ├── stream_forward_server.cc
│   ├── stream_forward_service.cc
│   ├── stream_forward_service.h
│   └── trpc_cpp_fiber.yaml
├── README.md
├── run_cmake.sh
└── run.sh
```

We can use the following script to quickly compile and run a program.

```shell
sh examples/features/trpc_stream_forward/run.sh
```

* Compilation

We can run the following command to compile the client and server programs.

```shell
bazel build //examples/features/trpc_stream/server:trpc_stream_server
bazel build //examples/features/trpc_stream_forward/proxy:trpc_stream_forward_server
bazel build //examples/features/trpc_stream_forward/client:client
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/trpc_stream
$ mkdir -p examples/features/trpc_stream/build && cd examples/features/trpc_stream/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/trpc_stream_forward
$ mkdir -p examples/features/trpc_stream_forward/build && cd examples/features/trpc_stream_forward/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server program

We can run the following command to start the server program.

*CMake build targets can be found at `build` of this directory and `examples/features/trpc_stream/build`, you can replace below server&client binary path when you use cmake to compile.*

```shell
# StreamGreeter Server.
bazel-bin/examples/features/trpc_stream/server/trpc_stream_server --config=examples/features/trpc_stream/server/trpc_cpp_fiber.yaml &

# StreamForward Server.
bazel-bin/examples/features/trpc_stream_forward/proxy/trpc_stream_forward_server --config=examples/features/trpc_stream_forward/proxy/trpc_cpp_fiber.yaml 
```

* Run the client program

We can run the following command to start the client program.

```shell
bazel-bin/examples/features/trpc_stream_forward/client/client --client_config=examples/features/trpc_stream_forward/client/trpc_cpp_fiber.yaml --rpc_method=ClientStreamSayHello
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
bazel-bin/examples/features/trpc_stream_forward/client/client --client_config=examples/features/trpc_stream_forward/client/trpc_cpp_fiber.yaml --rpc_method=ServerStreamSayHello
bazel-bin/examples/features/trpc_stream_forward/client/client --client_config=examples/features/trpc_stream_forward/client/trpc_cpp_fiber.yaml --rpc_method=BidiStreamSayHello
```
