# Prometheus metrics demo

The server invocation relationship is as follows:

client <---> proxy <---> server

Since the usage of the Prometheus plugin is independent of the runtime type, this demo will only use the fiber mode for demonstration purposes.


## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/prometheus/
examples/features/prometheus/
├── client
│   ├── BUILD
│   ├── client.cc
│   └── trpc_cpp_fiber.yaml
├── CMakeLists.txt
├── proxy
│   ├── BUILD
│   ├── forward.proto
│   ├── forward_server.cc
│   ├── forward_service.cc
│   ├── forward_service.h
│   └── trpc_cpp_fiber.yaml
├── README.md
├── run_cmake.sh
└── run.sh
```

* Compilation

We can run the following command to compile the demo.

```shell
$ bazel build //examples/helloworld/...
$ bazel build //examples/features/prometheus/... --define trpc_include_prometheus=true
```

Alternatively, you can use cmake.
```shell
# remove path-to-trpc-cpp/build to reduce the compile affects to other examples
$ rm -rf build
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/helloworld
$ mkdir -p examples/helloworld/build && cd examples/helloworld/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# remove path-to-trpc-cpp/build to reduce the compile affects to other examples
$ rm -rf build
$ mkdir -p build && cd build && cmake -DTRPC_BUILD_WITH_METRICS_PROMETHEUS=ON -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/prometheus
$ mkdir -p examples/features/prometheus/build && cd examples/features/prometheus/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# remove path-to-trpc-cpp/build to reduce the compile affects to other examples
$ rm -rf build
```

* Run the server/proxy program

We can run the following command to start the server and proxy program.

*CMake build targets can be found at `build` of this directory and `examples/helloworld/build`, you can replace below server&client binary path when you use cmake to compile.*

```shell
$ ./bazel-bin/examples/helloworld/helloworld_svr --config=examples/helloworld/conf/trpc_cpp_fiber.yaml
```

```shell
$ ./bazel-bin/examples/features/prometheus/proxy/forward_server --config=examples/features/prometheus/proxy/trpc_cpp_fiber.yaml
```

* Run the client program

We can run the following command to start the client program.

```shell
$ ./bazel-bin/examples/features/prometheus/client/client --client_config=examples/features/prometheus/client/trpc_cpp_fiber.yaml
```

* View the metrics data

You can obtain metrics data by using the "std::vector<::prometheus::MetricFamily> Collect()" interface under the "::trpc::prometheus" namespace. Additionally, if the admin is enabled, you can also obtain monitoring data by accessing http://0.0.0.0:8889/metrics.
