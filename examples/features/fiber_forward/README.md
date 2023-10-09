# Forward Server Use Fiber (Request-Response).

The server invocation relationship is as follows:

client <---> proxy <---> server

In the proxy server code, in addition to showing the normal request to the downstream server, it also shows a code example of multiple calls to the downstream server in parallel.


## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/fiber_forward/
examples/features/fiber_forward/
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp_fiber.yaml
├── proxy
│   ├── BUILD
│   ├── forward.proto
│   ├── forward_server.cc
│   ├── forward_server.h
│   ├── forward_service.cc
│   ├── forward_service.h
│   └── trpc_cpp_fiber.yaml
├── README.md
├── run_cmake.sh
└── run.sh
`-- server(use example/helloworld)
```

* Compilation

We can run the following command to compile the client and server programs.

```shell
$ bazel build //examples/helloworld/...
$ bazel build //examples/features/fiber_forward/...
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/helloworld
$ mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/fiber_forward
$ mkdir -p examples/features/fiber_forward/build && cd examples/features/fiber_forward/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server/proxy program

We can run the following command to start the server/client program.

*CMake build targets can be found at `build` of this directory and `examples/helloworld/build`, you can replace below server&client binary path when you use cmake to compile.*

```shell
$ ./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
```

```shell
$ ./bazel-bin/examples/features/fiber_forward/proxy/fiber_forward --config=./examples/features/fiber_forward/proxy/trpc_cpp_fiber.yaml
```

* Run the client program

We can run the following command to start the client program.

```shell
$ ./bazel-bin/examples/features/fiber_forward/client/client --client_config=./examples/features/fiber_forward/client/trpc_cpp_fiber.yaml
```

The content of the output from the client program is as follows:
``` text
succ:1600, fail:0, timecost(ms):62
```