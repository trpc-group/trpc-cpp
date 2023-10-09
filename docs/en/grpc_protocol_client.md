[中文](../zh/grpc_protocol_client.md)

[TOC]

# Accessing gRPC service Guide

**Topic: How to access gRPC services based on tRPC-Cpp**

tRPC-Cpp supports server-side gRPC unary services and client-side calling of gRPC unary services.

Unary Service: It can be understood as a request-and-response service, which is distinguished from streaming services.

This article introduces how to develop gRPC Unary services based on tRPC-Cpp(referred to as tRPC below). Developers can
learn the following:

* How to access gRPC unary services.
* FAQ.

# How to access gRPC unary services

Example: [grpc](../../examples/features/grpc)

In tRPC, accessing a gRPC service is similar to accessing a tRPC service, except that configuration item is set
to `protocol: grpc`. 
Below is a brief overview of the key steps in accessing a gRPC service, based on the process of
accessing a tRPC service:
* Obtain the ServiceProxy object.
* Create a ClientContext object.
* Initiate the RPC call.

## Call process

* Get the `XxServiceProxyPtr` object `proxy`: use the `GetClient()->GetProxy<XxServiceProxy>(...)`.
```cpp
auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::test::helloworld::GreeterServiceProxy>("xx_service_name")
```

Configuration items:
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

* Create the `ClientContextPtr` object `context`: use `MakeClientContext(proxy)`.
```cpp
auto ctx = ::trpc::MakeClientContext(proxy);
```

* Initiate the RPC call. Call the interface defined in proto, such as the SayHello method defined in helloworld.proto.
Use the synchronous or asynchronous interface corresponding to the selected runtime category.

Example of synchronous call: [helloworld/fiber_client.cc](../../examples/helloworld/test/fiber_client.cc)
```cpp
// Synchronous call code snippet, using the fiber runtime.
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

Example of asynchronous call: [helloworld/future_client.cc](../../examples/helloworld/test/future_client.cc)
```cpp
// Asynchronous call code snippet, using the merge or the separate runtime.
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

## Does gRPC support h2 (HTTP2 over SSL)?
It is not currently supported. The gRPC protocol used in tRPC uses h2c at the underlying level, and SSL is not currently
supported(but is currently being developed).
