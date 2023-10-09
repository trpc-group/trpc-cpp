# Dispatch request to specific thread demo.

you can use framework to dispatch the request to specific thread by handling it in fiber/separate runtime.

under the fiber runtime, it is not possible dispatching the request to specify thread in one fibe scheduling group, but you can achieve this effect by adjusting the number of scheduling group to 1 and setting the task steal ratio to 0 between scheduling groups。

```shell
global:
  threadmodel:
    fiber:
      - instance_name: fiber_instance
        concurrency_hint: 8
        scheduling_group_size: 1
        work_stealing_ratio: 0
```

under the fiber runtime, if you don't need to use the information of the request to dispatch the request to the specify thread, you don't need to call `SetHandleRequestDispatcherFunction` in `Initialize()`, but under the separate runtime, you must call `SetHandleRequestDispatcherFunction`.

```shell
service->SetHandleRequestDispatcherFunction(DispatchRequest);
```

The server invocation relationship is as follows:

client <---> server

## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/request_dispatch/
examples/features/request_dispatch/
├── client
│   ├── BUILD
│   ├── fiber_client.cc
│   ├── future_client.cc
│   ├── trpc_cpp_fiber.yaml
│   └── trpc_cpp_future.yaml
├── CMakeLists.txt
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── BUILD
    ├── demo_server.cc
    ├── trpc_cpp_fiber.yaml
    └── trpc_cpp_separate.yaml
```

We can run the following command to compile the client and server programs.

```shell
$ ./examples/features/request_dispatch/run.sh
```

* Compilation

We can run the following command to compile the client and server programs.

```shell
bazel build //examples/features/request_dispatch/...
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/request_dispatch
$ mkdir -p examples/features/request_dispatch/build && cd examples/features/request_dispatch/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server and client program

We can run the following command to start the server and client program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
$ ./bazel-bin/examples/features/request_dispatch/server/demo_server --config=./examples/features/request_dispatch/server/trpc_cpp_separate.yaml
$ ./bazel-bin/examples/features/request_dispatch/client/fiber_client --client_config=./examples/features/request_dispatch/client/trpc_cpp_fiber.yaml
```

```shell
$ ./bazel-bin/examples/features/request_dispatch/server/demo_server --config=./examples/features/request_dispatch/server/trpc_cpp_fiber.yaml
$ ./bazel-bin/examples/features/request_dispatch/client/fiber_client --client_config=./examples/features/request_dispatch/client/trpc_cpp_fiber.yaml
```

```shell
$ ./bazel-bin/examples/features/request_dispatch/server/demo_server --config=./examples/features/request_dispatch/server/trpc_cpp_fiber.yaml
$ ./bazel-bin/examples/features/request_dispatch/client/future_client --client_config=./examples/features/request_dispatch/client/trpc_cpp_future.yaml
```

The content of the output from the fiber client program is as follows:
``` text
succ:32, fail:0, timecost(ms):10

succ:32, fail:0, timecost(ms):7
```

The log from the server program in separate runtime is as follows:
``` text
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:96] request message: 0
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:100] thread logic id: 0, unique id: 65536
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:96] request message: 1
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:100] thread logic id: 1, unique id: 65537
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:96] request message: 2
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:100] thread logic id: 2, unique id: 65538
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:96] request message: 3
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:100] thread logic id: 3, unique id: 65539
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:96] request message: 4
[2023-07-26 14:55:14.849] [default] [info] [demo_server.cc:100] thread logic id: 4, unique id: 65540
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:96] request message: 5
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:100] thread logic id: 5, unique id: 65541
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:96] request message: 6
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:100] thread logic id: 6, unique id: 65542
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:96] request message: 7
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:100] thread logic id: 7, unique id: 65543
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:96] request message: 8
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:100] thread logic id: 0, unique id: 65536
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:96] request message: 9
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:100] thread logic id: 1, unique id: 65537
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:96] request message: 10
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:100] thread logic id: 2, unique id: 65538
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:96] request message: 11
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:100] thread logic id: 3, unique id: 65539
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:96] request message: 12
[2023-07-26 14:55:14.850] [default] [info] [demo_server.cc:100] thread logic id: 4, unique id: 65540
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:96] request message: 13
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:100] thread logic id: 5, unique id: 65541
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:96] request message: 14
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:100] thread logic id: 6, unique id: 65542
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:96] request message: 15
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:100] thread logic id: 7, unique id: 65543
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:96] request message: 16
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:100] thread logic id: 0, unique id: 65536
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:96] request message: 17
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:100] thread logic id: 1, unique id: 65537
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:96] request message: 18
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:100] thread logic id: 2, unique id: 65538
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:96] request message: 19
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:100] thread logic id: 3, unique id: 65539
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:96] request message: 20
[2023-07-26 14:55:14.851] [default] [info] [demo_server.cc:100] thread logic id: 4, unique id: 65540
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:96] request message: 21
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:100] thread logic id: 5, unique id: 65541
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:96] request message: 22
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:100] thread logic id: 6, unique id: 65542
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:96] request message: 23
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:100] thread logic id: 7, unique id: 65543
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:96] request message: 24
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:100] thread logic id: 0, unique id: 65536
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:96] request message: 25
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:100] thread logic id: 1, unique id: 65537
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:96] request message: 26
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:100] thread logic id: 2, unique id: 65538
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:96] request message: 27
[2023-07-26 14:55:14.852] [default] [info] [demo_server.cc:100] thread logic id: 3, unique id: 65539
[2023-07-26 14:55:14.853] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.853] [default] [info] [demo_server.cc:96] request message: 28
[2023-07-26 14:55:14.853] [default] [info] [demo_server.cc:100] thread logic id: 4, unique id: 65540
[2023-07-26 14:55:14.853] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.853] [default] [info] [demo_server.cc:96] request message: 29
[2023-07-26 14:55:14.853] [default] [info] [demo_server.cc:100] thread logic id: 5, unique id: 65541
[2023-07-26 14:55:14.853] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.853] [default] [info] [demo_server.cc:96] request message: 30
[2023-07-26 14:55:14.853] [default] [info] [demo_server.cc:100] thread logic id: 6, unique id: 65542
[2023-07-26 14:55:14.853] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47010
[2023-07-26 14:55:14.853] [default] [info] [demo_server.cc:96] request message: 31
[2023-07-26 14:55:14.853] [default] [info] [demo_server.cc:100] thread logic id: 7, unique id: 65543
```

The log from the server program in fiber runtime is as follows:
```shell
[2023-07-26 17:09:11.584] [default] [info] [trpc_server.cc:151] Service  auto-start to listen ...
[2023-07-26 17:09:12.278] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.278] [default] [info] [demo_server.cc:96] request message: 0
[2023-07-26 17:09:12.278] [default] [info] [demo_server.cc:100] thread logic id: 1, unique id: 16777217
[2023-07-26 17:09:12.278] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.278] [default] [info] [demo_server.cc:96] request message: 1
[2023-07-26 17:09:12.278] [default] [info] [demo_server.cc:100] thread logic id: 257, unique id: 16777473
[2023-07-26 17:09:12.278] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.278] [default] [info] [demo_server.cc:96] request message: 2
[2023-07-26 17:09:12.278] [default] [info] [demo_server.cc:100] thread logic id: 513, unique id: 16777729
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:96] request message: 3
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:100] thread logic id: 769, unique id: 16777985
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:96] request message: 4
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:100] thread logic id: 1025, unique id: 16778241
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:96] request message: 5
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:100] thread logic id: 1280, unique id: 16778496
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:96] request message: 6
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:100] thread logic id: 1537, unique id: 16778753
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:96] request message: 7
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:100] thread logic id: 1793, unique id: 16779009
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:96] request message: 8
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:100] thread logic id: 1, unique id: 16777217
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:96] request message: 9
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:100] thread logic id: 257, unique id: 16777473
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:96] request message: 10
[2023-07-26 17:09:12.279] [default] [info] [demo_server.cc:100] thread logic id: 513, unique id: 16777729
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:96] request message: 11
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:100] thread logic id: 769, unique id: 16777985
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:96] request message: 12
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:100] thread logic id: 1025, unique id: 16778241
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:96] request message: 13
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:100] thread logic id: 1280, unique id: 16778496
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:96] request message: 14
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:100] thread logic id: 1537, unique id: 16778753
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:96] request message: 15
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:100] thread logic id: 1793, unique id: 16779009
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:96] request message: 16
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:100] thread logic id: 1, unique id: 16777217
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:96] request message: 17
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:100] thread logic id: 257, unique id: 16777473
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:96] request message: 18
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:100] thread logic id: 513, unique id: 16777729
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:96] request message: 19
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:100] thread logic id: 769, unique id: 16777985
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:96] request message: 20
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:100] thread logic id: 1025, unique id: 16778241
[2023-07-26 17:09:12.280] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:96] request message: 21
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:100] thread logic id: 1280, unique id: 16778496
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:96] request message: 22
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:100] thread logic id: 1537, unique id: 16778753
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:96] request message: 23
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:100] thread logic id: 1793, unique id: 16779009
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:96] request message: 24
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:100] thread logic id: 1, unique id: 16777217
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:96] request message: 25
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:100] thread logic id: 257, unique id: 16777473
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:96] request message: 26
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:100] thread logic id: 513, unique id: 16777729
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:96] request message: 27
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:100] thread logic id: 769, unique id: 16777985
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:96] request message: 28
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:100] thread logic id: 1025, unique id: 16778241
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:96] request message: 29
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:100] thread logic id: 1280, unique id: 16778496
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:96] request message: 30
[2023-07-26 17:09:12.281] [default] [info] [demo_server.cc:100] thread logic id: 1537, unique id: 16778753
[2023-07-26 17:09:12.282] [default] [info] [demo_server.cc:95] remote address: 127.0.0.1:47049
[2023-07-26 17:09:12.282] [default] [info] [demo_server.cc:96] request message: 31
[2023-07-26 17:09:12.282] [default] [info] [demo_server.cc:100] thread logic id: 1793, unique id: 16779009
```