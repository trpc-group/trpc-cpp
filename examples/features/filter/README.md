# Filter Demo.

This demo includes simple usage of server-side and client-side filters. See [common/user_rpc_filter.h](/examples/features/filter/common/user_rpc_filter.h) and [common/invoke_stat_filter.h](/examples/features/filter/common/invoke_stat_filter.h) for usage instructions.

## user_rpc_filter.h
Demonstrates how to implement filters using the RpcServerFilter and RpcClientFilter base classes in rpc_filter.h.
RpcServerFilter and RpcClientFilter are subclasses of Filter, and their goal is to provide a more readable and consistent interface for intercepting RPC method calls before and after invocation.

For example, there are the following four requirements:
On the server side, before executing the server-side business code upon receiving an RPC request, add the message "server_before_rpc_invoke" to the msg field.
On the server side, after executing the server-side business code and completing the RPC request, add the message "server_after_rpc_invoke" to the msg field.
On the client side, before sending an RPC request, add the message "client_before_rpc_invoke" to the msg field.
On the client side, after receiving an RPC response, add the message "client_after_rpc_invoke" to the msg field.

Here you can implement the server-side and client-side logic based on rpc_filter. Please refer to [common/user_rpc_filter.h](/examples/features/filter/common/user_rpc_filter.h) for details.

Note:
For asynchronous requests, the rsp parameter is a null pointer for BeforeRpcInvoke on the client side and the req parameter is a null pointer for AfterRpcInvoke on the client side.
In case of a failed RPC call the rsp parameter is a null pointer for AfterRpcInvoke on the client side.

## invoke_stat_filter.h
Demonstrates how to implement filters using the MessageServerFilter and MessageClientFilter classes to count the number of successful and failed server-side and client-side RPC calls.

## Note
If your program is a server (inherits from TrpcApp), you need to override the `trpc::TrpcApp::RegisterPlugins` interface and register the required server-side and client-side filters in it.

If your program is a pure client (does not inherit from TrpcApp and only sends requests), the logic for registering filters needs to be placed before calling `trpc::RunInTrpcRuntime`.

## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/filter/
examples/features/filter/
├── BUILD
├── client
│   ├── BUILD
│   ├── demo_client.cc
│   └── trpc_cpp_separate.yaml
├── CMakeLists.txt
├── common
│   ├── BUILD
│   ├── invoke_stat_filter.h
│   └── user_rpc_filter.h
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── BUILD
    ├── demo_server.cc
    └── trpc_cpp_fiber.yaml
```

* Compilation & Run

We can run the following command to compile the client and server programs.

```shell
$ bazel build //examples/features/filter/server:demo_server
$ bazel build //examples/features/filter/client:demo_client
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/filter
$ mkdir -p examples/features/filter/build && cd examples/features/filter/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server/proxy program

We can run the following command to start the server/client program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
$ ./bazel-bin/examples/features/filter/server/demo_server --config=./examples/features/filter/server/trpc_cpp_fiber.yaml
```

```shell
$ ./bazel-bin/examples/features/filter/client/demo_client --client_config=./examples/features/filter/client/trpc_cpp_separate.yaml
```

The content of the output from the client program is as follows:
```text
Invoke stat: success count = 1, fail count = 0
Hello, async_future, client_before_rpc_invoke, server_before_rpc_invoke, server_after_rpc_invoke, client_after_rpc_invoke
Invoke stat: success count = 2, fail count = 0
Hello, sync_future, client_before_rpc_invoke, server_before_rpc_invoke, server_after_rpc_invoke, client_after_rpc_invoke
```
