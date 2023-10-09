# Redis Client Unary calling.
Redis client provides Fiber coroutines model and Future model access to backend Redis Server.
The Redis client sends Redis commands to the backend Redis server through the `Command interface (Fiber coroutines) `and `AsyncCommand interface` (Future mode), such as set、get、mset、mget etc . If the backend Redis server support the Pipeline mode (responding in the order of requests within the connection), the Redis client's connection-layer-pipeline(just config support_pipeline as true,no need change your code) can be used.

## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/redis/
examples/features/redis/
├── client
│   ├── fiber
│   │   ├── BUILD
│   │   ├── fiber_client.cc
│   │   └── fiber_client_config.yaml
│   └── future
│       ├── BUILD
│       ├── future_client.cc
│       └── future_client_config.yaml
├── CMakeLists.txt
├── README.md
├── run_cmake.sh
└── run.sh
```

We can use the following script to quickly compile and run a program.
```shell
sh examples/features/redis/run.sh
```

In the example code, it assumes that the Redis server is running locally on port 6397. Before running this example, you need to ensure that the local Redis Server is started(See more in Deploy and run redis-server). If you want to access Redis Server on a different node, you can modify the IP address in the xxx.yaml file.

* Deploy and run redis-server
```
wget https://download.redis.io/releases/redis-6.2.4.tar.gz
tar -zvxf redis-6.2.4.tar.gz
mv redis-6.2.4 /usr/local/redis
cd /usr/local/redis/
make
make PREFIX=/usr/local/redis install
./bin/redis-server &
```

* Compilation

We can run the following command to compile the client programs.

```shell
bazel build //examples/features/redis/client/...
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/features/redis
$ mkdir -p examples/features/redis/build && cd examples/features/redis/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the client program

We can run the following command to start the client program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
./bazel-bin/examples/features/redis/client/fiber/fiber_client --client_config=examples/features/redis/client/fiber/fiber_client_config.yaml
./bazel-bin/examples/features/redis/client/future/future_client --client_config=examples/features/redis/client/future/future_client_config.yaml
```

The content of the output from the client program is as follows:
``` text
Fiber call redis set command success. 
Fiber call redis get command success, reply: support_redis

....

Futre Async call redis set command success
Futre Async call redis get command success, reply: support_redis
xxx
```