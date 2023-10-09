# Transparent Server.

The server invocation relationship is as follows:

client <---> proxy <---> server

`proxy` receives the trpc request (header + body) from the `client`, and forwards the request to the backend `server`, and `proxy` receives the reply from the backend `server `,and sends response back to `client`. `proxy` will not decode the body of the request or the response, but only forward the body to `server` or `client`.


## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/trpc_transparent/
examples/features/trpc_transparent/
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp_fiber.yaml
├── CMakeLists.txt
├── proxy
│   ├── BUILD
│   ├── transparent_server.cc
│   ├── transparent_server.h
│   ├── transparent_service.cc
│   ├── transparent_service.h
│   ├── trpc_cpp_fiber.yaml
│   └── trpc_cpp_future.yaml
├── README.md
├── run_cmake.sh
└── run.sh
```

We can run the following command to quickly compile the client and server programs.

```shell
$ ./examples/features/trpc_transparent/run.sh
```

* Compilation

We can run the following command to compile the client and server programs.

```shell
$ bazel build //examples/helloworld/...
$ bazel build //examples/features/trpc_transparent/...
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/helloworld
$ mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/trpc_transparent
$ mkdir -p examples/features/trpc_transparent/build && cd examples/features/trpc_transparent/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server/proxy program

We can run the following command to start the server/client program.

*CMake build targets can be found at `build` of this directory and `examples/helloworld/build`, you can replace below server&client binary path when you use cmake to compile.*

```shell
$ ./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
```

```shell
$ ./bazel-bin/examples/features/trpc_transparent/proxy/transparentserver --config=./examples/features/trpc_transparent/proxy/trpc_cpp_fiber.yaml
```

* Run the client program

We can run the following command to start the client program.

```shell
$ ./bazel-bin/examples/features/trpc_transparent/client/client --client_config=./examples/features/trpc_transparent/client/trpc_cpp_fiber.yaml
```

The content of the output from the client program is as follows:
``` text
response: msg: "hello, fiber"
```