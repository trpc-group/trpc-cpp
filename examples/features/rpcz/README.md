# Rpcz In Proxy Scenario

The calling topology is as follows:

client <---> proxy <---> server

With client-rpcz and server-rpcz, proxy MakeClientContext without server context, generating two rpcz spans, one as client type, the other server type.

With Route-rpcz, proxy MakeClientContext with server context, generating only one rpcz span as server type, but chaining all the requests and responses.

## Usage

Use the following command to view the directory tree.
```shell
$ tree examples/features/rpcz/
examples/features/rpcz
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp_client.yaml
├── CMakeLists.txt
├── proxy
│   ├── BUILD
│   ├── rpcz.proto
│   ├── rpcz_proxy.cc
│   ├── rpcz_proxy.h
│   └── trpc_cpp_proxy.yaml
├── README.md
├── run_cmake.sh
└── run.sh
`-- server(use example/helloworld)
```

* Compilation

Run the following command to compile the client, proxy and server programs.

```shell
$ bazel build //examples/helloworld/...
$ bazel build //examples/features/rpcz/client/...
$ bazel build //examples/features/rpcz/proxy/... --define trpc_include_rpcz=true
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DTRPC_BUILD_WITH_RPCZ=ON .. && make -j8 && cd -
# build examples/helloworld
$ mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/rpcz
$ mkdir -p examples/features/rpcz/build && cd examples/features/rpcz/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server/proxy program

Run the following commands to start the server/proxy program.

*CMake build targets can be found at `build` of this directory and `examples/helloworld/build`, you can replace below server&client binary path when you use cmake to compile.*

```shell
$ ./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp.yaml
```

```shell
$ ./bazel-bin/examples/features/rpcz/proxy/rpcz_proxy --config=./examples/features/rpcz/proxy/trpc_cpp_proxy.yaml
```

* Run the client program

Run the following command to start the client program.

```shell
$ ./bazel-bin/examples/features/rpcz/client/client --client_config=./examples/features/rpcz/client/trpc_cpp_client.yaml
```

Finally, use curl to get rpcz general span infos from rpcz proxy server.

```shell
$ curl http://127.0.0.1:32111/cmds/rpcz
```

The content of output from above curl is as follows:

```text
2023-07-19 18:45:43:809573   cost=451(us) span_type=C span_id=1 trpc.test.helloworld.Greeter/SayHello request=0 response=0 [ok]
```

Besides, use curl to get specific rpcz span info based on general span infos which contains span id.

```shell
$ curl http://127.0.0.1:32111/cmds/rpcz?span_id=1
```

The content of output from above curl is as follows:

```text
2023-07-19 18:45:43:809573   start send request(0) to real_service(127.0.0.1:12345) protocal=trpc span_id=1
2023-07-19 18:45:43:809581   8(us) start transport invoke
2023-07-19 18:45:43:809670   89(us) enter send queue
2023-07-19 18:45:43:809675   5(us) leave send queue
2023-07-19 18:45:43:809875   200(us) io send done
2023-07-19 18:45:43:809960   85(us) enter recv queue
2023-07-19 18:45:43:810018   58(us) leave recv func
2023-07-19 18:45:43:810019   1(us) finish transport invoke
2023-07-19 18:45:43:810024   5(us) finish rpc invoke
2023-07-19 18:45:43:810024   0(us)Received response(0) from real_service(127.0.0.1:12345)
```
