# HTTP SSE (Server-Sent Events) Example

Server-Sent Events (SSE) is a standard allowing a server to send updates to a client over a single HTTP connection. Unlike traditional HTTP requests where the client sends a request and waits for a single response, SSE enables the server to push multiple updates to the client in real-time.

## Usage

We can use the following command to view the directory tree.
```shell
$ tree examples/features/http_sse/
examples/features/http_sse/
├── client
│   ├── BUILD
│   ├── sse_client.cc
│   └── trpc_cpp_fiber.yaml
├── CMakeLists.txt
├── README.md
├── run_cmake.sh
├── run.sh
└── server
    ├── BUILD
    ├── http_sse_server.cc
    └── trpc_cpp_fiber.yaml
```

We can use the following script to quickly compile and run a program.
```shell
sh examples/features/http_sse/run.sh
```

* Compilation

We can run the following command to compile the client and server programs.

```shell
bazel build //examples/features/http_sse/server:http_sse_server
bazel build //examples/features/http_sse/client:sse_client
```

Alternatively, you can use cmake.
```shell
# build trpc-cpp libs first, if already build, just skip this build process.
$ mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
# build examples/http_sse
$ mkdir -p examples/features/http_sse/build && cd examples/features/http_sse/build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -
```

* Run the server program

We can run the following command to start the server program.

*CMake build targets can be found at `build` of this directory, you can replace below server&client binary path when you use cmake to compile.*

```shell
bazel-bin/examples/features/http_sse/server/http_sse_server --config=examples/features/http_sse/server/trpc_cpp_fiber.yaml
```
* Use the curl command to test the server
  
```shell
curl -i -N http://127.0.0.1:24856/sse/test
```
* The curl test results are as follows
  
``` text
HTTP/1.1 200 OK
Connection: keep-alive
Content-Type: text/event-stream
Cache-Control: no-cache
Transfer-Encoding: chunked
Access-Control-Allow-Origin: *
Access-Control-Allow-Headers: Cache-Control

event: message
data: {"msg": "hello", "idx": 0}
id: 0

event: message
data: {"msg": "hello", "idx": 1}
id: 1

event: message
data: {"msg": "hello", "idx": 2}
id: 2
......


```    
* Run the client program

We can run the following command to start the client program.

```shell
bazel-bin/examples/features/http_sse/client/sse_client --client_config=examples/features/http_sse/client/trpc_cpp_fiber.yaml
```

The content of the output from the client program is as follows:
``` text
Received SSE event - id: 0, event: message, data: {"msg": "hello", "idx": 0}
Received SSE event - id: 1, event: message, data: {"msg": "hello", "idx": 1}
Received SSE event - id: 2, event: message, data: {"msg": "hello", "idx": 2}
Received SSE event - id: 3, event: message, data: {"msg": "hello", "idx": 3}
Received SSE event - id: 4, event: message, data: {"msg": "hello", "idx": 4}
Received SSE event - id: 5, event: message, data: {"msg": "hello", "idx": 5}
Received SSE event - id: 6, event: message, data: {"msg": "hello", "idx": 6}
Received SSE event - id: 7, event: message, data: {"msg": "hello", "idx": 7}
Received SSE event - id: 8, event: message, data: {"msg": "hello", "idx": 8}
Received SSE event - id: 9, event: message, data: {"msg": "hello", "idx": 9}
SSE stream finished

```

