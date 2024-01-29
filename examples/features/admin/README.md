# Admin demo

The calling topology is as follows:

client <---> proxy <---> server

In this example, we use the proxy server to demonstrate the admin function, in order to show the complete statistical information of the server and client, as well as the example of the client_detach function. In fact, the usage of any server is the same.

## Usage

Use the following command to view the directory tree.
```shell
$ tree examples/features/admin/
examples/features/admin/
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp.yaml
├── CMakeLists.txt
├── proxy
│   ├── BUILD
│   ├── custom_conf.h
│   ├── forward.proto
│   ├── forward_server.cc
│   ├── forward_service.cc
│   ├── forward_service.h
│   └── trpc_cpp.yaml
├── README.md
├── run_cmake.sh
└── run.sh
```

* Compilation

Run the following command to compile the client and server programs.

```shell
$ bazel build //examples/helloworld/...
$ bazel build //examples/features/admin/proxy/...
$ bazel build //examples/features/admin/client/...
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/helloworld
$ mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/admin
$ mkdir -p examples/features/admin/build && cd examples/features/admin/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server/proxy program

We can run the following command to start the server and proxy program.

*CMake build targets can be found at `build` of this directory and `examples/helloworld/build`, you can replace below server&client binary path when you use cmake to compile.*

```shell
$ ./bazel-bin/examples/helloworld/helloworld_svr --config=examples/helloworld/conf/trpc_cpp_fiber.yaml
```

```shell
$ ./bazel-bin/examples/features/admin/proxy/forward_server --config=examples/features/admin/proxy/trpc_cpp.yaml
```

* Run the client program

Run the following command to start the client program.

```shell
$ ./bazel-bin/examples/features/admin/client/client --client_config=examples/features/admin/client/trpc_cpp.yaml
```

* Use the admin capability

You can trigger admin commands by sending an HTTP request to http://admin_ip:admin_port. We use the curl tool to demonstrate how to use admin commands here.

1. Call the custom admin command.

```shell
$ curl http://127.0.0.1:8889/cmds/myhandler1
{"custom_value":10}
$ curl http://127.0.0.1:8889/cmds/myhandler2
custom_value: 10
```

2. Query framework version
```shell
$ curl http://127.0.0.1:8889/version
```

3. List all cmd commands
```shell
$ curl http://127.0.0.1:8889/cmds
{"cmds":["/client_detach","/cmds","/cmds/loglevel","/cmds/myhandler1","/cmds/myhandler2","/cmds/profile/cpu","/cmds/profile/heap","/cmds/reload-config","/cmds/stats","/cmds/var","/cmds/var/trpc","/cmds/var/user","/cmds/watch","/version"]}
```

4. Query or set log level
```shell
# query
$ curl http://127.0.0.1:8889/cmds/loglevel
{"errorcode":0,"message":"","level":"INFO"}
# set log level to ERROR
$ curl http://127.0.0.1:8889/cmds/loglevel -X PUT -d 'value=ERROR'
{"errorcode":0,"message":"","level":"ERROR"}
# query again
$ curl http://127.0.0.1:8889/cmds/loglevel
{"errorcode":0,"message":"","level":"ERROR"}
```

5. Reload config
```shell
# the custom value is 10 before reloading config
$ curl http://127.0.0.1:8889/cmds/myhandler1
{"custom_value":10}
# change the "custom:value" of server/trpc_cpp_fiber.yaml to 20, and then reload the config
$ curl http://127.0.0.1:8889/cmds/reload-config -X POST
{"errorcode":0,"message":"reload config ok"}
# the custom value is 20 after reloading config
$ curl http://127.0.0.1:8889/cmds/myhandler1
{"custom_value":20}
```

6. Query framework stats
You should make sure that "server:enable_server_stats" configuration item is set to true firstly.
```shell
$ curl http://127.0.0.1:8889/cmds/stats
{"errorcode":0,"message":"","stats":{"conn_count":1,"total_req_count":11,"req_concurrency":1,"now_req_count":3,"last_req_count":4,"total_failed_req_count":0,"now_failed_req_count":0,"last_failed_req_count":0,"total_avg_delay":0.18181818181818183,"now_avg_delay":0.3333333333333333,"last_avg_delay":0.25,"max_delay":1,"last_max_delay":1}}
```

7. Query framework and user-defined variables
```shell
$ curl http://127.0.0.1:8889/cmds/var
{
  "trpc" : 
  {
    "client" : 
    {
      "trpc.test.helloworld.Greeter" : 
      {
        "backup_request" : 0,
        "backup_request_success" : 0
      }
    }
  },
  "user" : 
  {
    "my_count" : 10
  }
}
$ curl http://127.0.0.1:8889/cmds/var/trpc
{
  "client" : 
  {
    "trpc.test.helloworld.Greeter" : 
    {
      "backup_request" : 0,
      "backup_request_success" : 0
    }
  }
}
$ curl http://127.0.0.1:8889/cmds/var/user
{
  "my_count" : 10
}
$ curl http://127.0.0.1:8889/cmds/var/user/my_count
10
```
Please refer to the "examples/features/tvar" for more usage examples.

8. Query rpcz info
Please refer to the "examples/features/rpcz" for specific examples.

9. CPU and heap profiling
You need to make sure that there have "/usr/lib64/libtcmalloc_and_profiler.so" file locally to link tcmalloc_and_profiler firstly. Then add the "--define trpc_enable_profiler=true" compilation option and recompile the proxy program.
```shell
$ bazel build //examples/features/admin/proxy/... --define trpc_enable_profiler=true
$ ./bazel-bin/examples/features/admin/proxy/forward_server --config=examples/features/admin/proxy/trpc_cpp.yaml
```

Alternatively, you can use cmake.
```shell
$ rm -rf build # cmake need to rebuild trpc lib using DTRPC_BUILD_WITH_TCMALLOC_PROFILER=ON
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DTRPC_BUILD_WITH_TCMALLOC_PROFILER=ON .. && make -j8 && cd -
# build examples/features/admin
$ mkdir -p examples/features/admin/build && cd examples/features/admin/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
$ rm -rf build # cmake need to rebuild trpc lib to prevent DTRPC_BUILD_WITH_TCMALLOC_PROFILER=ON affects
```

cpu profiling:
```shell
# start sampling
$ curl http://127.0.0.1:8889/cmds/profile/cpu?enable=y -X POST
{"errorcode":0,"message":"OK"}
# wait for a period of time and stop sampling
$ curl http://127.0.0.1:8889/cmds/profile/cpu?enable=n -X POST
{"errorcode":0,"message":"OK"}
# after successful completion, a file named cpu.prof will be generated at the execution location
# use the built-in pprof tool of gperftools to analyze the output
$ pprof ./bazel-bin/examples/features/admin/server/admin_server ./cpu.prof --pdf > cpu.pdf
```

heap profiling:
```shell
# start sampling
$ curl http://127.0.0.1:8889/cmds/profile/heap?enable=y -X POST
{"errorcode":0,"message":"OK"}
# wait for a period of time and stop sampling
$ curl http://127.0.0.1:8889/cmds/profile/heap?enable=n -X POST
{"errorcode":0,"message":"OK"}
# after successful completion, a file named heap.prof will be generated at the execution location
# use the built-in pprof tool of gperftools to analyze the output
$ pprof ./bazel-bin/examples/features/admin/server/admin_server ./heap.prof --pdf > heap.pdf
```

10. Query metrics data
Please refer to the "examples/features/prometheus" for specific examples.

11. Detach client connection
Note that it currently only works in default thread mode.
```shell
$ curl http://127.0.0.1:8889/client_detach -X POST -d 'service_name=trpc.test.helloworld.Greeter' -d 'remote_ip=0.0.0.0:12345'
{"client_detach":"success."}
```

In addition to the above cmd commands, the framework also provides some get-type commands that support browser access. You can directly enter the address in the browser to access it, for example, enter "http://127.0.0.1:8889" to view tRPC main page.
