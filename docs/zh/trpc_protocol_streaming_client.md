[English](../en/trpc_protocol_streaming_client.md)

# 前言

tRPC 协议的流式 RPC 分为三种类型：

* 客户端流式 RPC：客户端发送一个或者多个请求消息，服务端发送一个响应消息。
* 服务端流式 RPC：客户端发送一个请求消息，服务端发送一个或多个响应消息。
* 双向流式 RPC：客户端发送一个或者多个请求消息，服务端发送一个或者多个响应消息。

tRPC 流式服务支持两种线程模型：

* `fiber` 线程模型支持同步流式 RPC。
* `merge` 线程模型支持异步流式 RPC。

本文介绍如何基于 tRPC-Cpp （下面简称 tRPC）访问 tRPC 流式服务，开发者可以了解到如下内容：

* 如何使用同步接口访问流式 RPC 服务
* 如何使用异步接口访问流式 RPC 服务
* FAQ

开发 tRPC 流式服务的方式可以参考：[trpc_protocol_streaming_service.md](./trpc_protocol_streaming_service.md)

# 如何使用同步接口访问流式 RPC 服务

示例：[trpc_stream](../../examples/features/trpc_stream)

同步流式接口在 [trpc_protocol_streaming_service](trpc_protocol_streaming_service.md) 有介绍，这里简单列举一下，方便查阅。

```cpp
// 流同步读取器，R 为读消息类型
template <typename R>
class StreamReader {
 public:
  Status Read(R* msg, int timeout = -1) const;
  Status Finish();
  Status GetStatus();
};

// 流同步写入器，W 为写消息类型
template <typename W>
class StreamWriter {
 public:
  Status Write(const W& msg);
  Status WriteDone();
  Status Finish();
  Status GetStatus();
};

// 流同步读写器，R 为读消息类型，W 为写消息类型
template <typename W, typename R>
class StreamReaderWriter {
 public:
  Status Read(R* msg, int timeout = -1) const;
  Status Write(const W& msg);
  Status WriteDone();
  Status Finish();
  Status GetStatus();
};
```

这里以 [stream.proto](../../examples/features/trpc_stream/server/stream.proto) 为例说明。

tRPC-Cpp 框架桩代码生成工具会生成如下代码：

```cpp
class StreamGreeterServiceProxy : public ::trpc::RpcServiceProxy {
 public:
  // Client Streaming RPC
  virtual ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloRequest> ClientStreamSayHello(const ::trpc::ClientContextPtr& context, ::trpc::test::helloworld::HelloReply* response);
  // Server Streaming RPC
  virtual ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloReply> ServerStreamSayHello(const ::trpc::ClientContextPtr& context, const ::trpc::test::helloworld::HelloRequest& request);
  // Bidirectional Streaming RPC
  virtual ::trpc::stream::StreamReaderWriter<::trpc::test::helloworld::HelloRequest, ::trpc::test::helloworld::HelloReply> BidiStreamSayHello(const ::trpc::ClientContextPtr& context);
};
```

示例：[client.cc](../../examples/features/trpc_stream/client/client.cc)

这里简单以 *双向流式* 为例说明客户端访问流式 RPC 服务的过程。

* 客户端发送指定数量的请求消息给服务端，发送成功后，告知服务端发送完成。
* 客户端持续读取从服务端发来的响应消息，直至读取到 EOF。
* 成功读取到 EOF 后，结束 RPC 调用，等待最终的 RPC 调用结果。
* 简单代码片段：

  ```cpp
  bool CallBidiStreamSayHello(const StreamGreeterServiceProxyPtr& proxy, int request_count) {
    auto context = ::trpc::MakeClientContext(proxy);
    // 打开双向流
    auto stream = proxy->BidiStreamSayHello(context);
    ::trpc::Status status{0, 0, "OK"};
    bool ok{true};
    do {
      // 检查流是否成功打开
      if (!stream.GetStatus().OK()) {
        std::cout << "stream error:" << stream.GetStatus().ToString() << std::endl;
        ok = false;
        break;
      }
  
      // 客户端发送指定数量的请求消息给服务端 
      for (int i = 0; i < request_count; ++i) {
        std::stringstream request_msg;
        request_msg << "BidiStreamSayHello, " << (i + 1);
        ::trpc::test::helloworld::HelloRequest request;
        request.set_msg(request_msg.str());
        // 发送流消息
        status = stream.Write(request);
        if (status.OK()) {
          continue;
        }
        break;
      }
  
      if (!status.OK()) {
        std::cerr << "write error: " << status.ToString() << std::endl;
        ok = false;
        break;
      }
  
      // 发送成功后，告知服务端发送完成
      status = stream.WriteDone();
      if (!status.OK()) {
        std::cerr << "write done error: " << status.ToString() << std::endl;
        ok = false;
        break;
      }
  
      ::trpc::test::helloworld::HelloReply reply;
      // 客户端持续读取从服务端发来的响应消息，直至读取到 EOF
      for (;;) {
        status = stream.Read(&reply, 2000);
        if (status.OK()) {
          std::stringstream reply_msg;
          reply_msg << "stream got reply:" << reply.msg();
          std::cout << reply_msg.str() << std::endl;
          continue;
        }
        if (status.StreamEof()) {
          std::cout << "got EOF" << std::endl;
          // 成功读取到 EOF 后，结束 RPC 调用，等待最终的 RPC 调用结果
          status = stream.Finish();
        }
        break;
      }
  
      if (status.OK()) {
        std::cout << "stream rpc succeed" << std::endl;
      } else {
        std::cerr << "stream rpc failed:" << status.ToString() << std::endl;
        ok = false;
      }
    } while (0);
    return ok;
  }
  ```

# 如何使用异步接口访问流式 RPC 服务

异步流式接口在 trpc_streaming_service.md 有介绍，这里简单列举一下，方便查阅。

```cpp
// 流异步读取器，R 为读消息类型
template <typename R>
class AsyncReader final : public RefCounted<AsyncReader<R>> {
 public:
  Future<std::optional<R>> Read(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());  // 返回 std::optional<R> 为空表示 EOF
  Future<> Finish();
};

// 流异步写入器，W 为写消息类型
template <typename W>
class AsyncWriter : public RefCounted<AsyncWriter<W>> {
 public:
  Future<> Write(const W& msg);
  Future<> Finish();
};

// 流异步读写器，R 为读消息类型，W 为写消息类型
template <typename R, typename W>
class AsyncReaderWriter final : public RefCounted<AsyncReaderWriter<R, W>> {
 public:
  Future<std::optional<R>> Read(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());  // 返回 std::optional<R> 为空表示 EOF
  Future<> Write(const W& msg);
  Future<> Finish();
};
```

这里以 [stream.proto](../../examples/features/trpc_stream/server/stream.proto) 为例说明。

tRPC-Cpp 框架桩代码生成工具会生成如下代码：

```cpp
class AsyncStreamGreeterServiceProxy : public ::trpc::RpcServiceProxy {
 public:
  // Client Streaming RPC
  ::trpc::Future<std::pair<::trpc::stream::AsyncWriterPtr<::trpc::test::helloworld::HelloRequest>, ::trpc::Future<::trpc::test::helloworld::HelloReply>>> ClientStreamSayHello(const ::trpc::ClientContextPtr& context);
  // Server Streaming RPC
  ::trpc::Future<::trpc::stream::AsyncReaderPtr<::trpc::test::helloworld::HelloReply>> ServerStreamSayHello(const ::trpc::ClientContextPtr& context, ::trpc::test::helloworld::HelloRequest&& request);
  // Bidirectional Streaming RPC
  ::trpc::Future<::trpc::stream::AsyncReaderWriterPtr<::trpc::test::helloworld::HelloReply, ::trpc::test::helloworld::HelloRequest>> BidiStreamSayHello(const ::trpc::ClientContextPtr& context);
};
```

# FAQ

## 同步流式接口和异步流式接口如何选择？

取决于使用的线程模型，`fiber` 只能使用同步流式接口，`merge` 只能使用异步流式接口。

## 为什么异步流式接口中，没有 WriteDone？

在异步流式接口中，客户端在调用读写器的 Finish 接口时，即是通知了对端数据已写完（相当于WriteDone）。

## 3 异步流式仅能在 `merge` 线程模型运行吗？

是的，其他线程模型还未支持。
另外，merge 线程模型当前仅支持 tRPC 流式协议和 HTTP 协议，gRPC 暂未支持。
