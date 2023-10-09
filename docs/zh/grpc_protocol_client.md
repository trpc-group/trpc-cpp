[English](../en/grpc_protocol_client.md)

[TOC]

# 访问 gRPC 服务

**主题：如何基于 tRPC-Cpp 访问 gRPC 服务**

tRPC-Cpp 支持服务端 gRPC Unary 服务，也支持客户端调用 gRPC Unary 服务。

Unary Service：可以理解为一问一答服务，是为了区别流式服务（Streaming Service）。

本文介绍如何基于 tRPC-Cpp （下面简称 tRPC）访问 gRPC Unary 服务，开发者可以了解到如下内容：

* 如何访问 gRPC Unary 服务。
* FAQ。


# 如何访问 gRPC Unary 服务

示例：[grpc](../../examples/features/grpc)

在 tRPC 中，访问 gRPC 协议服务和访问 tRPC 协议服务过程大体一致，差异在于配置项为 `protocol: grpc`。
下面参照访问 tRPC 服务过程，简单浏览下访问 gRPC 服务过程中的关键步骤：
* 获取 ServiceProxy 对象。
* 创建 ClientContext 对象。
* 发起 RPC 调用。

## 调用过程

* 获取 ServiceProxy 对象。
获取 `XxServiceProxyPtr` 对象 `proxy`：使用 `GetTrpcClient()->GetProxy<XxServiceProxy>(...)` 接口获取。
```cpp
auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>("xx_service_name")
```

配置项：
```yaml
...
client:
  service:
    - name: trpc.test.helloworld.Greeter
      target: 127.0.0.1:24656
      protocol: grpc
      network: tcp
      selector_name: direct
...
```

* 创建 ClientContext 对象。
创建 `ClientContextPtr` 对象 `context`：使用 `::trpc::MakeClientContext(proxy)`。
```cpp
auto ctx = ::trpc::MakeClientContext(proxy);
```

* 发起 RPC 调用。调用 proto 中定义的接口，例如 helloworld.proto 中定义的 SayHello 方法。
* 按照选择的 runtime 类别对应使用同步接口或者异步接口。

同步调用示例代码：[helloworld/fiber_client.cc](../../examples/helloworld/test/fiber_client.cc)

```cpp
// 同步调用代码片段，使用 fiber runtime
int DoRpcCall(const std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>& proxy) {
  ::trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy);
  ::trpc::test::helloworld::HelloRequest req;
  req.set_msg("fiber");
  ::trpc::test::helloworld::HelloReply rsp;
  ::trpc::Status status = proxy->SayHello(client_ctx, req, &rsp);
  if (!status.OK()) {
    std::cerr << "get rpc error: " << status.ErrorMessage() << std::endl;
    return -1;
  }
  std::cout << "get rsp msg: " << rsp.msg() << std::endl;
  return 0;
}
```

异步调用示例代码：[helloworld/future_client.cc](../../examples/helloworld/test/future_client.cc)
```cpp
// 异步调用代码片段，使用 merge 或者 separate runtime
int DoAsyncRpcCall(const std::shared_ptr<::trpc::test::helloworld::GreeterServiceProxy>& proxy) {
  ::trpc::ClientContextPtr client_ctx = ::trpc::MakeClientContext(proxy);
  ::trpc::test::helloworld::HelloRequest req;
  req.set_msg("future");
  bool succ = true;
  ::trpc::Latch latch(1);
  proxy->AsyncSayHello(client_ctx, req)
      .Then([&latch, &succ](::trpc::Future<::trpc::test::helloworld::HelloReply>&& fut) {
        if (fut.IsReady()) {
          auto rsp = fut.GetValue0();
          std::cout << "get rsp msg: " << rsp.msg() << std::endl;
        } else {
          auto exception = fut.GetException();
          succ = false;
          std::cerr << "get rpc error: " << exception.what() << std::endl;
        }
        latch.count_down();
        return ::trpc::MakeReadyFuture<>();
      });
  latch.wait();
  return succ ? 0 : -1;
}
```

# FAQ

## 1 访问 gRPC 是否支持 h2 (HTTP2 over SSL)？
暂未支持。tRPC 中的 gRPC 协议底层使用的 h2c，暂未支持 SSL（支持中）。
