[中文](../zh/grpc_protocol_service.md)

# Overview

tRPC-Cpp supports server-side gRPC unary services and client-side calling of gRPC unary services.

Unary Service: It can be understood as a request-and-response service, which is distinguished from streaming services.

This article introduces how to develop gRPC Unary services based on tRPC-Cpp(referred to as tRPC below). Developers can
learn the following:

* How to develop gRPC unary services.
* FAQ.

# How to develop gRPC unary services

## Quick start

### Experience a gRPC service

Example: [grpc](../../examples/features/grpc)

Go to the main directory of the tRPC code repository and run the following command.

```shell
sh examples/features/grpc/run.sh
```

The content of the output from the client program is as follows:

``` text
got rsp msg: Hello, fiber
got rsp msg: Hello, future
```

### Basic steps for developing a gRPC service

For convenience, gRPC in tRPC reuses the capabilities of the tRPC protocol during implementation.

If there is an existing tRPC service, it can be directly accessed using the gRPC protocol by modifying the configuration
item `protocol: trpc` to `protocol: grpc`. The example `grpc` code above is implemented in this way.

If a new gRPC service needs to be created, it can first be created as tRPC service according to the tRPC protocol. Then
set the protocol field to `protocol: grpc` to provide gRPC service.

In tRPC, developing a gRPC service is similar to developing a tRPC service. The key steps in developing a gRPC service
are briefly outlined below based on the tRPC service development process:

* Define the interface using proto.
* Implement the service code.
* Set the protocol configuration item to "grpc".
* Register the service.

### Implementation process

Here, the example of "helloworld" is used to illustrate.

* Defining the interface using proto.

gRPC generally uses ProtoBuffers to define the interface, and then generates corresponding stub code based on different
programming languages. Here, we reuse the tRPC protocol code generation tool to generate the C++ stub code.

We will use the helloworld.proto as an example.

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

The content of bazel-BUILD is as follows:

```bzl
# @file: BUILD
...
load("@trpc_cpp//trpc:trpc.bzl", "trpc_proto_library")

trpc_proto_library(
    name = "helloworld_proto",
    srcs = ["helloworld.proto"],
    generate_mock_code = True,
    use_trpc_plugin = True,
)
...
```

* Implementing the gRPC unary service.

*Note: The process of generating a project through tRPC scaffold is omitted here.*

```cpp
// @file: greeter_service.h/cc
namespace helloworld {

class GreeterServiceImpl : public ::trpc::test::helloworld::Greeter {
 public:
  ::trpc::Status SayHello(::trpc::ServerContextPtr context,
                          const ::trpc::test::helloworld::HelloRequest* request,
                          ::trpc::test::helloworld::HelloReply* reply) {
    std::string response = "Hello, " + request->msg();
    reply->set_msg(response);

    return ::trpc::kSuccStatus;
  }
};

}  // namespace helloworld 
```

* Setting the protocol configuration item to "grpc".

```yaml
# tRPC service.
...
  service:
    - name: trpc.test.helloworld.Greeter
      protocol: trpc
      network: tcp
...

# Change to gRPC service:
... 
  service:
    - name: trpc.test.helloworld.Greeter
      protocol: grpc
      network: tcp
...
```

* Registering service.

```cpp
int HelloworldServer::Initialize() {
  const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
  // Set the service name, which must be the same as the value of the `/server/service/name` configuration item
  // in the configuration file, otherwise the framework cannot receive requests normally.
  std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "Greeter");
  TRPC_FMT_INFO("service name:{}", service_name);

  RegisterService(service_name, std::make_shared<GreeterServiceImpl>());

  return 0;
}
```

# FAQ

## Does gRPC support h2 (HTTP2 over SSL)?

It is not currently supported. The gRPC protocol used in tRPC uses h2c at the underlying level, and SSL is not currently
supported(but is currently being developed).
