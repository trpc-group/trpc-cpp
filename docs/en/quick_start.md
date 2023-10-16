[中文](../zh/quick_start.md)

## Overview

This article will guide you start tRPC-Cpp with a simple example.

## Environment requirments

In the C++ world, there is no universal accepted standard for managing project dependencies. Therefore, you need to install some developing toolkits before compiling and running the example. For details, please refer to [Environment Setup](setup_env.md). If you are new to tRPC-Cpp, we recommend using bazel to build project. It will reduce much of time spending on bootstraping C++ project. You will see how easy it can be when we lead you though the following guidance.

## Compile and Run

### Compile tRPC-Cpp

Clone the tRPC-Cpp repo:

```shell
git clone https://github.com/trpc-group/trpc-cpp
```

Compile the tRPC-Cpp source code:

```shell
cd trpc-cpp
./build.sh
```

If do `cat build.sh`, you will find this script just executs a simple command `bazel build //trpc/...`. It means compling all the targets in trpc subdirectory where all tRPC-Cpp framework source codes placed. It will take a minutes to do the progress for the first time. Once done, you will see a similar output:

```shell
INFO: 2629 processes: 2629 processwrapper-sandbox.
INFO: Build completed successfully, 5459 total actions
```

If failed, it might due to network error(eg. unable to download third-party libs that tRPC-Cpp needs), or environment requirments not satisfied(eg. gcc version is too low). Please refer to [Environment Setup Faq](setup_env.md) to get detail solutions.

### Compile HelloWorld

After the tRPC-Cpp is successfully compiled, the next step is to compile and run the HelloWorld example.

Compile the HelloWorld source code:

```shell
bazel build //examples/helloworld/...
```

### Run HelloWorld

After the HelloWorld is successfully compiled, you can run the server:

```shell
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
```

Then, open another terminal and run the client to send RPC requests to server:

```shell
./bazel-bin/examples/helloworld/test/fiber_client --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
```

After client done, you will see the following output in client's terminal:

```shell
FLAGS_service_name:trpc.test.helloworld.Greeter
FLAGS_client_config:./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
get rsp msg: Hello, fiber
```

At the same time, you can also check the server log file `helloworld_fiber.log` in project root directory of `trpc-cpp`:

```shell
cat helloworld_fiber.log 
```

You will see the following output:

```shell
[2023-08-25 15:30:56.587] [default] [info] [greeter_service.cc:37] remote address: 127.0.0.1:46074
[2023-08-25 15:30:56.587] [default] [info] [greeter_service.cc:38] request message: fiber
```

Congratulations! You’ve just successfully run a client-server application with tRPC-Cpp.

## How to add a new RPC method

Now you can do some modifcation to HelloWorld project, how about adding a new RPC method.

### Update the service definition

First, let's update service definition. You can find it in the file `examples/helloworld/helloworld.proto` (using protocol buffers IDL). It shows that, for RPC method `SayHello` in service `Greeter`, server recives `HelloRequest` type message from client and sends back `HelloReply` type message as serving response.

```protobuf
syntax = "proto3";

package trpc.test.helloworld;

service Greeter {
  rpc SayHello (HelloRequest) returns (HelloReply) {}
}

message HelloRequest {
   string msg = 1;
}

message HelloReply {
   string msg = 1;
}
```

Add a new `SayHelloAgain` method, with the same message types as `SayHello`.

```protobuf
syntax = "proto3";
 
package trpc.demo.helloworld;

service Greeter {
  rpc SayHello (HelloRequest) returns (HelloReply) {}

  rpc SayHelloAgain (HelloRequest) returns (HelloReply) {}
}

message HelloRequest {
  string msg = 1;
}

message HelloReply {
  string msg = 1;
}
```

### Update the server code

After adding a RPC method for service, you need implement it in server side.

Open the file `examples/helloworld/greeter_service.h` and add method declartion `SayHelloAgain` to `GreeterServiceImpl`.

```cpp
class GreeterServiceImpl : public ::trpc::test::helloworld::Greeter {
 public:
  ::trpc::Status SayHello(::trpc::ServerContextPtr context,
                          const ::trpc::test::helloworld::HelloRequest* request,
                          ::trpc::test::helloworld::HelloReply* reply) override;

  ::trpc::Status SayHelloAgain(::trpc::ServerContextPtr context,
                               const ::trpc::test::helloworld::HelloRequest* request,
                               ::trpc::test::helloworld::HelloReply* reply) override;
};
```

Open the file `examples/helloworld/greeter_service.cc` and add method implementation of `SayHelloAgain` to `GreeterServiceImpl`.

```cpp
::trpc::Status GreeterServiceImpl::SayHelloAgain(::trpc::ServerContextPtr context,
                                                 const ::trpc::test::helloworld::HelloRequest* request,
                                                 ::trpc::test::helloworld::HelloReply* reply) {
  // Your can access more information from rpc context, eg: remote ip and port
  TRPC_FMT_INFO("remote address: {}:{}", context->GetIp(), context->GetPort());
  TRPC_FMT_INFO("request message: {}", request->msg());

  std::string response = "Hello, ";
  response += request->msg();
  response += " Again";

  reply->set_msg(response);

  return ::trpc::kSuccStatus;
}
```

### Update the client code

Now the server serves method `SayHelloAgain`, you can asscess it via do a RPC call in client.

The stub code auto-generated using updated proto file already contains `SayHelloAgain` function, you can simply invoke it. Referencing code snippets that calls `SayHello`, you can add below code in `examples/helloworld/test/fiber_client.cc` after `DoRpcCall` function:

```cpp
int DoRpcCallAgain(const std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>& proxy) {
  ::trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy);
  ::trpc::test::helloworld::HelloRequest req;
  req.set_msg("fiber");
  ::trpc::test::helloworld::HelloReply rsp;
  ::trpc::Status status = proxy->SayHelloAgain(client_ctx, req, &rsp);
  if (!status.OK()) {
    std::cerr << "get again rpc error: " << status.ErrorMessage() << std::endl;
    return -1;
  }
  std::cout << "get again rsp msg: " << rsp.msg() << std::endl;
  return 0;
}
```

Then, you should add code to invoke `DoRpcCallAgain` in the `Run` function, as below:

```cpp
int Run() {
  auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>(FLAGS_service_name);

  int ret = DoRpcCall(proxy);

  ret = DoRpcCallAgain(proxy);

  return ret;
}
```

### Run

Compile the updated HelloWorld code

```shell
bazel build //examples/helloworld/...
```

Run the updated HelloWorld server:

```shell
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
```

Open another terminal and run the client to send RPC requests to server:

```shell
./bazel-bin/examples/helloworld/test/fiber_client --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
```

After client done, you will see the following output in client's terminal:

```shell
FLAGS_service_name:trpc.test.helloworld.Greeter
FLAGS_client_config:./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
get rsp msg: Hello, fiber
get again rsp msg: Hello, fiber Again
```

### What's next

- Learn how tRPC-Cpp works in [Architecture Design](architecture_design.md).
- Read [Basic Tutorial](./basic_tutorial.md) to develop tRPC-Cpp service.
- Read [User Guide](../README.md) to use tRPC-Cpp more comprehensively.
