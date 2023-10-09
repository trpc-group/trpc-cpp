# gRPC Unary calling (Request-Response).

We can quickly change the `protocol` field from `trpc` to `grpc`, then it will provide a gRPC service as expect
if there is a tRPC service available.

```yaml
# tRPC service.
...
  service:
    - name: trpc.test.helloworld.Greeter
      protocol: trpc                  # Application layer protocol, eg: trpc/http/...
      network: tcp                    # Network type, Support two types: tcp/udp
...

# Change to gRPC service:
... 
  service:
    - name: trpc.test.helloworld.Greeter
      protocol: grpc                  # Application layer protocol, eg: trpc/http/...
      network: tcp                    # Network type, Support two types: tcp/udp
...
```
## Usage

We can use the following command to view the directory tree.

```shell
$ tree examples/features/grpc/
examples/features/grpc
├── client
│   ├── trpc_cpp_fiber.yaml
│   └── trpc_cpp.yaml
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── trpc_cpp_fiber.yaml
    └── trpc_cpp.yaml
```

We can use the following script to quickly compile and run a program.

```shell
sh examples/features/grpc/run.sh
```

* Compilation

We can run the following command to compile the client and server programs.

```shell
bazel build //examples/helloworld/...
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/grpc_stream
$ mkdir -p examples/features/grpc_stream/build && cd examples/features/grpc_stream/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server program

We can run the following command to start the server program.

*CMake build targets can be found at `examples/helloworld/build`, you can replace below server&client binary path when you use cmake to compile.*

```shell
# It's also available to run helloworld_svr with default merge or separate thread model.
# bazel-bin/examples/helloworld/helloworld_svr --config=examples/features/grpc/server/trpc_cpp.yaml &
bazel-bin/examples/helloworld/helloworld_svr --config=examples/features/grpc/server/trpc_cpp_fiber.yaml &
```

* Run the client program

We can run the following command to start the client program.

```shell
bazel-bin/examples/helloworld/test/fiber_client --client_config=examples/features/grpc/client/trpc_cpp_fiber.yaml
bazel-bin/examples/helloworld/test/future_client --client_config=examples/features/grpc/client/trpc_cpp.yaml
```

The content of the log from the client program is as follows:
- check grpc_client.log/grpc_client_fiber.log/grpc_helloworld_fiber.log

``` text
got rsp msg: Hello, fiber
got rsp msg: Hello, future
```
