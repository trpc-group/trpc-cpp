### Helloworld demo
## Building the compilation environment
   please reference to: [Compile & Install](../../docs/en/setup_env.md)

## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/helloworld
examples/helloworld
├── BUILD
├── CMakeLists.txt
├── conf
│   ├── trpc_cpp_fiber.yaml
│   └── trpc_cpp.yaml
├── greeter_service.cc
├── greeter_service.h
├── greeter_service_test.cc
├── helloworld.proto
├── helloworld_server.cc
├── README.md
├── run_cmake.sh
├── run.sh
└── test
    ├── BUILD
    ├── conf
    │   ├── trpc_cpp_fiber.yaml
    │   └── trpc_cpp_future.yaml
    ├── fiber_client.cc
    └── future_client.cc
```

We can use the following script to quickly compile and run a program.
```shell
sh examples/helloworld/run.sh
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
# build examples/helloworld
$ mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server&client program

We can run the following command to start the server program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

Test helloworld in fiber runtime
```shell
$ ./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
$ ./bazel-bin/examples/helloworld/test/fiber_client --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
```

Test helloworld in thread runtime
```shell
$ ./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp.yaml
$ ./bazel-bin/examples/helloworld/test/fiber_client --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
```

The content of the output from the server program is as follows. You can check `helloworld.log/helloworld_fiber.log` for server output details.

- server in fiber runtime
```text
[2023-08-08 12:05:51.910] [default] [info] [helloworld_server.cc:43] service name:trpc.test.helloworld.Greeter
[2023-08-08 12:05:51.913] [default] [info] [trpc_server.cc:173] Service trpc.test.helloworld.Greeter auto-start to listen ...
[2023-08-08 12:05:51.913] [default] [info] [trpc_server.cc:173] Service  auto-start to listen ...
[2023-08-08 12:05:52.816] [default] [info] [greeter_service.cc:37] remote address: 127.0.0.1:40639
[2023-08-08 12:05:52.816] [default] [info] [greeter_service.cc:38] request message: fiber
```
- server in thread runtime
```text
[2023-08-08 12:05:53.916] [default] [info] [helloworld_server.cc:43] service name:trpc.test.helloworld.Greeter
[2023-08-08 12:05:53.916] [default] [info] [trpc_server.cc:173] Service trpc.test.helloworld.Greeter auto-start to listen ...
[2023-08-08 12:05:53.917] [default] [info] [trpc_server.cc:173] Service  auto-start to listen ...
[2023-08-08 12:05:54.909] [default] [info] [greeter_service.cc:37] remote address: 127.0.0.1:40640
[2023-08-08 12:05:54.909] [default] [info] [greeter_service.cc:38] request message: future
```

The content of the output from the client program is as follows.

- client in fiber runtime
```text
get rsp msg: Hello, fiber
```
- client in thread runtime
```text
get rsp msg: Hello, future
```
