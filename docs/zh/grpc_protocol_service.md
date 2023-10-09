[English](../en/grpc_protocol_service.md)

[TOC]

# gRPC 服务开发指南

**主题：如何基于 tRPC-Cpp 开发 gRPC 服务**

tRPC-Cpp 支持服务端 gRPC Unary 服务，也支持客户端调用 gRPC Unary 服务。

Unary Service：可以理解为一问一答服务，是为了区别流式服务（Streaming Service）。

本文介绍如何基于 tRPC-Cpp （下面简称 tRPC）开发 gRPC Unary 服务，开发者可以了解到如下内容：

* 如何开发 gRPC Unary 服务。
* FAQ。

# 如何开发 gRPC 服务

## 快速上手

### 体验 gRPC 服务

示例：[grpc](../../examples/features/grpc)

方法：进入 tRPC-Cpp 代码仓库主目录，然后运行下面的命令。

```shell
sh examples/features/grpc/run.sh
```

从客户端日志中可以看到示例程序输出：

``` text
got rsp msg: Hello, fiber
got rsp msg: Hello, future
```


### 开发步骤

为了使用方便，tRPC 中的 gRPC 在实现时复用了 tRPC 协议相关能力。

如果有一个现成的 tRPC 服务，可以直接修改配置项  `protocol: trpc` -> `protocol: grpc`，这个服务就可以直接使用 gRPC 协议访问了。
上面的 grpc 示例代码就是这样实现的。

如果想新建一个 gRPC 服务，可以先按照 tRPC 协议创建服务。然后，将协议字段设置为 `protocol: grpc` 就可以提供 gRPC 服务了。

在 tRPC 中，开发 gRPC 服务和开发 tRPC 服务过程大体一致， 下面参照 tRPC 服务开发过程，简单浏览下 gRPC 服务开发过程中的关键步骤：
* 使用 proto 定义接口。
* 实现服务代码。
* protocol 配置项设置为 "grpc"。
* 注册服务。

### 实现过程

这里沿用 helloworld 为例说明。

* 使用 proto 定义接口
 
gRPC 一般使用 ProtoBuffers 定义接口，然后根据不同编程语言生成对应的桩代码。
这里复用 tRPC 协议代码生成工具生成 C++ 桩代码。

这里沿用 helloworld.proto :
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

bazel BUILD 文件如下：
```
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

* 服务端代码实现

提示：这里省略了通过 tRPC 脚手架生成项目的过程。

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

* protocol 配置项设置为 "grpc"。

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

* 注册服务

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

## 1 gRPC 是否支持 h2 (HTTP2 over SSL)？
暂未支持。tRPC 中的 gRPC 协议底层使用的 h2c，暂未支持 SSL（支持中）。
