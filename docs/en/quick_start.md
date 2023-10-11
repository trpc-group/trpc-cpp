[中文](../zh/quick_start.md)

# Overview

This guide gets you started with tRPC-Cpp with a simple working example.

# Install development environment

In the C++ world, there is no universally accepted standard for managing project dependencies. Therefore, before building and running the Hello World example in this article, you need to install the development environment. For details, please refer to [Environment Setup](setup_env.md). It is recommended to use bazel when you are just starting with tRPC-Cpp. The next steps in this article mainly introduce how to use bazel to compile and run the Hello World example.

# Compile and Run

## Compile tRPC-Cpp

Clone the tRPC-Cpp repo

```shell
git clone https://github.com/trpc-group/trpc-cpp
```

Compile tRPC-Cpp source code

```shell
cd trpc-cpp
./build.sh
```

Using bazel to compile the framework will be slower when compiling the framework for the first time, and it may take a few minutes. The compiled result information will output to the terminal. If the compilation is successful, you can see information similar to the following:

```shell
INFO: Elapsed time: 279.100s, Critical Path: 48.83s
INFO: 2629 processes: 2629 processwrapper-sandbox.
INFO: Build completed successfully, 5459 total actions
```

If there is a failure, it may be caused by network reasons (unable to pull remote code), environment installation (gcc, framework dependent library version is too low), etc. please refer to [Environment Setup Faq](setup_env.md)

## Compile HelloWorld

After the tRPC-Cpp framework code is successfully compiled, the next step is to compile and run the Hello World example.

Execute the following command to compile the Hello World sample code.

```shell
bazel build //examples/helloworld/...
```

## Run HelloWorld

After the code is successfully compiled, run the server program.

```shell
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
```

Open another terminal and run the client test program.

```shell
./bazel-bin/examples/helloworld/test/fiber_client --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
```

After the client test program runs successfully, you can see the following output.

```shell
FLAGS_service_name:trpc.test.helloworld.Greeter
FLAGS_client_config:./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
get rsp msg: Hello, fiber
```

At the same time, you can also view the log `helloworld_fiber.log`` file in local directory.

```shell
cat helloworld_fiber.log 
```

You can see the following output.

```shell
[2023-08-25 15:30:56.587] [default] [info] [greeter_service.cc:37] remote address: 127.0.0.1:46074
[2023-08-25 15:30:56.587] [default] [info] [greeter_service.cc:38] request message: fiber
```

Congratulations! You’ve just run a client-server application with tRPC-Cpp.

# Update

Now let's see how to add a new RPC method to update the server program for the client to call.

Currently, the service provided by our tRPC-Cpp is implemented using the protocol buffers IDL definition. Both the server and client stubs have a SayHello() RPC method, which obtains the HelloRequest parameter from the client and returns a HelloReply from the server, and the method's It is defined as follows:

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

Open examples/helloworld/helloworld.proto, and add a new SayHelloAgain() method, with the same request and response types.

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

## Update the server code

Open examples/helloworld/greeter_service.h and add the SayHelloAgain method to GreeterServiceImpl.

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

Open examples/helloworld/greeter_service.cc, and add the code implementation of SayHelloAgain method to GreeterServiceImpl.

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

## Update the client code

A new SayHelloAgain() method is now provided in the stub. We will follow the previous code that calls SayHello(), and implement a function that calls SayHelloAgain(), open examples/helloworld/test/fiber_client.cc and add the following code after the DoRpcCall function:

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

Then, we call the above DoRpcCallAgain function in the Run function, as follows:

```cpp
int Run() {
  auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>(FLAGS_service_name);

  int ret = DoRpcCall(proxy);

  ret = DoRpcCallAgain(proxy);

  return ret;
}
```

## Run

Compile HelloWorld code

```shell
bazel build //examples/helloworld/...
```

Run the server program.

```shell
./bazel-bin/examples/helloworld/helloworld_svr --config=./examples/helloworld/conf/trpc_cpp_fiber.yaml
```

Open another terminal and run the client test program.

```shell
./bazel-bin/examples/helloworld/test/fiber_client --client_config=./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
```

You can see the following output.

```shell
FLAGS_service_name:trpc.test.helloworld.Greeter
FLAGS_client_config:./examples/helloworld/test/conf/trpc_cpp_fiber.yaml
get rsp msg: Hello, fiber
get again rsp msg: Hello, fiber Again
```

## What's next

- Learn how tRPC-Cpp works in [Architecture Design](https://github.com/trpc-group/trpc-cpp/docs/en/architecture_design.md)
  and [Terminology](https://github.com/trpc-group/trpc/blob/main/docs/en/terminology.md).
- Read [Basic Tutorial](basic_tutorial.md) to develop tRPC-Cpp service.
- Read [User Guide](../README.md) to use tRPC-Cpp more comprehensively.
