# Tvar In Client-Server Scenario

The calling topology is as follows:

client <---> server

Client send requests to server, in order to trigger tvar statistics, which exposed by admin service.

## Usage

Use the following command to view the directory tree.
```shell
$ tree examples/features/tvar/
examples/features/tvar/
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp_client.yaml
├── CMakeLists.txt
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── BUILD
    ├── trpc_cpp_server.yaml
    ├── tvar.proto
    ├── tvar_server.cc
    └── tvar_server.h
`-- server(use example/helloworld)
```

* Compilation

Run the following command to compile the client and server programs.

```shell
$ bazel build //examples/features/tvar/client/...
$ bazel build //examples/features/tvar/server/...
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/tvar
$ mkdir -p examples/features/tvar/build && cd examples/features/tvar/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server program

Run the following commands to start the server program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
$ ./bazel-bin/examples/features/tvar/server/tvar_server --config=./examples/features/tvar/server/trpc_cpp_server.yaml
```

* Run the client program

Run the following command to start the client program.

```shell
$ ./bazel-bin/examples/features/tvar/client/client --client_config=./examples/features/tvar/client/trpc_cpp_client.yaml
```

Finally, use curl to get tvar statistics through admin service.

```shell
$ curl http://127.0.0.1:31111/cmds/var/counter
2
```

```shell
$ curl http://127.0.0.1:31111/cmds/var/counter?history=true
{
  "latest_day" :
  [
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  ],
  "latest_hour" :
  [
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  ],
  "latest_min" :
  [
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  ],
  "latest_sec" :
  [
    4,
    4,
    4,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
  ],
  "now" : [ 4 ]
}
```

The other categories live the similar way, except Miner, Maxer, Averager and IntRecorder do not support history data.
