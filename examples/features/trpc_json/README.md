# Json Demo.

The server invocation relationship is as follows:

client <---> server

`client` can send json msg to `server` use trpc or http protocol.

## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/trpc_json/
examples/features/trpc_json/
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp_fiber.yaml
├── CMakeLists.txt
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── BUILD
    ├── demo_server.cc
    └── trpc_cpp_fiber.yaml
```

We can run the following command to quickly compile the client and server programs.

```shell
$ ./examples/features/trpc_json/run.sh
```

* Compilation

Run the following command to compile the client and server programs.

```shell
$ bazel build //examples/features/trpc_json/...
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/trpc_json
$ mkdir -p examples/features/trpc_json/build && cd examples/features/trpc_json/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server program

Run the following commands to start the server program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
$ ./bazel-bin/examples/features/trpc_json/server/demo_server --config=./examples/features/trpc_json/server/trpc_cpp_fiber.yaml
```

* Run the client program

Run the following command to start the client program.

```shell
$ ./bazel-bin/examples/features/trpc_json/client/client --client_config=./examples/features/trpc_json/client/trpc_cpp_fiber.yaml
```

The content of the output from the client program is as follows:
``` text
json name: age, value: 18
json name: height, value: 180
```