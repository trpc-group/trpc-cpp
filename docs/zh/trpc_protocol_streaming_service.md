[English](../en/trpc_protocol_streaming_service.md)

# tRPC 流式协议服务开发指南

**主题：如何基于 tRPC-Cpp 开发 tRPC 流式服务**

使用单次 RPC（简称 Unary RPC）时，客户端发送单个请求给服务端，并回复单个响应给客户端。
可简单分为三步：

* 客户端发送一个请求消息给服务端，然后等待 RPC 返回。
* 服务端接收请求消息并执行业务逻辑，然后发送一个响应消息给客户端。
* 客户端接收 RPC 返回值，然后处理返回值和响应消息。

流式 RPC 相比单次 RPC 而言，客户端和服务端之间的流通道建立后，双方均可以持续发送、接收消息，请求、响应消息像水一样在持续不断流动。
客户端可以持续不断地发送请求消息、接收响应消息；服务端可以持续不断地接收请求消息、发送响应消息。

本文介绍如何基于 tRPC-Cpp （下面简称 tRPC）开发 tRPC 流式服务，开发者可以了解到如下内容：

* 流式 RPC 特点。
* 如何定义流式 RPC 服务。
* 如何开发同步流式 RPC 服务。
* 如何开发异步流式 RPC 服务。
* FAQ。

# 流式 RPC 特点

## 流式 RPC 适用场景

模拟如下业务场景：
每天早上 6 点，都有一批百万级别的数据集要同从 A 同步到 B，在同步的时候，会做一系列操作（归档、数据分析、画像、日志等）。
同步完成后，会有用户立即查询这批数据，这批数据会经过各种计算后以不同报表形式展示给用户，其中有些数据会推送给订阅用户。

场景特点：

* 一次性涉及的数据量较大。
* 具有一定实时性。

使用单次 RPC 有如下限制：

* 数据包过大造成的瞬时压力。
* 接收数据包时，需要所有数据包都接受成功且正确后，才能够回调响应，进行业务处理（无法做到客户端边发送，服务端边处理）。

流式 RPC 在如下场景有优势：

* 大规模数据包。
  比如，有一个大文件需要传输。
  使用流式 RPC 时，客户端分片读出文件内容后直接写入到流中，服务端可以按客户端写入顺序读取到文件分片内容，然后执行业务逻辑。
  如果使用单次 RPC，需要多次调用RPC方法，会遇到分包、组包、乱序、业务逻辑上下文切换等问题。
* 实时场景。
  比如，股票行情走势，资讯 Feeds 流。
  服务端接收到消息后，需要往多个客户端进行实时消息推送，流式 RPC 可以在一次 RPC 调用过程中，推送完整的消息列表。

## 三种流式 RPC 方法

tRPC 协议的流式 RPC 分为三种类型：

* 客户端流式 RPC：客户端发送一个或者多个请求消息，服务端发送一个响应消息。
* 服务端流式 RPC：客户端发送一个请求消息，服务端发送一个或多个响应消息。
* 双向流式 RPC：客户端发送一个或者多个请求消息，服务端发送一个或者多个响应消息。

## 流式协议和线程模型

tRPC 流式服务支持两种线程模型：

* `fiber` 线程模型支持同步流式 RPC。
* `merge` 线程模型支持异步流式 RPC。

# 如何定义流式 RPC 服务

## 在 `proto` 文件中定义流式 RPC 方法

流式 RPC 接口定义比单次 RPC 接口定义多了一个关键字 `stream` ，用来标注流式 RPC 方法类型。

```protobuf
// Client Streaming RPC （客户端流式）
    rpc  ClientStreamSayHello (stream Request) returns (Reply)  {}
// Server Streaming RPC （服务端流式）
    rpc  ServerStreamSayHello (Request) returns (stream Reply) {}
// Bidirectional Streaming RPC （双向流式）
    rpc  BidiStreamSayHello(stream Request) returns (stream Reply) {}
```

## 关于 RPC 方法命名

* 一个名称只能定义一种 RPC 方法
  比如，不能使用 `SayHello` 同时定义客户端流式和服务端流式 RPC 方法以及单次 RPC 方法。如下将同一个方法名同时定义了三中 RPC
  方法：

```protobuf
// !!! 如下定义是错误的!!!

// Unary RPC
    rpc  SayHello (Request) returns (Reply) {}
// Client Streaming RPC
    rpc  SayHello (stream Request) returns (Reply)  {}
// Server Streaming RPC
    rpc  SayHello (Request) returns (stream Reply) {} 
```

* 单次 RPC 和流式 RPC 放在不同 Service 中

尽管实现上不限制用户如何放置单次 RPC 和流式 RPC 的位置。
但**`推荐`**把单次 RPC 和流式 RPC 分别放在不同的 `Service` 中，不要混合在一起，这样做会降低问题定位难度。
比如，从网络收发包角度看，流式 RPC 中多个流会共享同一连接，流式 RPC 协议帧比单次RPC协议帧处理逻辑复杂，混在一起时，会增加问题排查难度。

推荐做法如下：

```protobuf
// Service of unary RPC
service Greeter {
  rpc  SayHello (Request) returns (Reply) {}
}

// Service of streaming RPC
service StreamGreeter {
  // Client Streaming RPC
  rpc  ClientStreamSayHello (stream Request) returns (Reply)  {}
  // Server Streaming RPC
  rpc  ServerStreamSayHello (Request) returns (stream Reply) {}
}
```

# 如何开发同步流式 RPC 服务

提示：同步流式 RPC 服务需要运行在 `fiber` 线程模型中。

## 体验流式 RPC 服务

示例： [trpc_stream](../../examples/features/trpc_stream)

方法：进入 tRPC-Cpp 代码仓库主目录，然后运行下面的命令。

```shell
sh examples/features/trpc_stream/run.sh
```

示例程序输出：

``` text
name: Streaming RPC, ClientStreamSayHello1, ok: 1
name: Streaming RPC, ClientStreamSayHello2, ok: 1
name: Streaming RPC, ClientStreamSayHello3, ok: 1
name: Streaming RPC, ClientStreamSayHello4, ok: 1
name: Streaming RPC, ClientStreamSayHello5, ok: 1
name: Streaming RPC, ClientStreamSayHello6, ok: 1
name: Streaming RPC, ClientStreamSayHello7, ok: 1
name: Streaming RPC, ClientStreamSayHello8, ok: 1
final result of streaming RPC calling: 1
```

## 同步流式接口

### 流读写器接口

说明：接口中的 `R` `W` 是模版参数，表示待读取的消息和待写入的流消息对象。

| 类/对象               | 接口名称                                  | 功能         | 参数                                                                                            | 返回值    |
|--------------------|---------------------------------------|------------|-----------------------------------------------------------------------------------------------|--------|
| StreamReader       | Status Read(R* msg, int timeout = -1) | 从流中读取消息    | msg 响应消息指针，读到消息后会更新到指针所指对象。 <br> timeout 等待超时，当没有消息就绪时，会阻塞等待。 <br> 取值：-1： 永久等待，>=0：等待超时，单位：毫秒 | Status | 
| StreamReader       | Status Finish(void)                   | 等待RPC结束    | void                                                                                          | Status |
| StreamReader       | Status GetStatus(void)                | 获取流状态      | void                                                                                          | Status |
| StreamWriter       | Status Write(const W& msg)            | 向流中写入消息    | msg 待写入消息引用                                                                                   | Status |
| StreamWriter       | Status WriteDone(void)                | 通知对端消息写入结束 | void                                                                                          | Status |
| StreamWriter       | Status Finish(void)                   | 等待RPC结束    | void                                                                                          | Status |
| StreamWriter       | Status GetStatus(void)                | 获取流状态      | void                                                                                          | Status |
| StreamReaderWriter | Status Read(R* msg, int timeout = -1) | 从流中读取消息    | msg 响应消息指针，读到消息后会更新到指针所指对象。 <br> timeout 等待超时，当没有消息就绪时，会阻塞等待。 <br> 取值：-1： 永久等待，>=0：等待超时，单位：毫秒 | Status |
| StreamReaderWriter | Status Write(const W& msg)            | 向流中写入消息    | msg 待写入消息引用                                                                                   | Status |
| StreamReaderWriter | Status WriteDone(void)                | 通知对端消息写入结束 | void                                                                                          | Status |
| StreamReaderWriter | Status Finish(void)                   | 等待RPC结束    | void                                                                                          | Status |
| StreamReaderWriter | Status GetStatus(void)                | 获取流状态      | void                                                                                          | Status |

### 流式接口返回值

流操作接口返回 `Status`，包含框架错误码、RPC 方法返回码、错误消息等。
流式编程常用如下接口：

| 类/对象   | 接口名称                     | 功能              | 参数   | 返回值                             |
|--------|--------------------------|-----------------|------|---------------------------------|
| Status | bool Status::OK()        | 调用流接口是否正确       | void | bool, true: 正确，false: 出错        |
| Status | bool Status::StreamEof() | 从流中读取数据是否遇到 EOF | void | bool ，true：遇到 EOF，false：未遇到 EOF |

### 流式接口错误码

服务端常用错误码：

| 错误码                                           | 含义        |
|-----------------------------------------------|-----------|
| TRPC_STREAM_SERVER_NETWORK_ERR = 201          | 服务端流式网络错误 |
| TRPC_STREAM_SERVER_MSG_EXCEED_LIMIT_ERR = 211 | 流消息过大     |
| TRPC_STREAM_SERVER_ENCODE_ERR = 221           | 服务端流编码错误  |
| TRPC_STREAM_SERVER_DECODE_ERR = 222           | 服务端流编解码错误 |
| TRPC_STREAM_SERVER_READ_TIMEOUT_ERR = 254     | 服务端流读超时   |

客户端常用错误码：

| 错误码                                           | 含义        |
|-----------------------------------------------|-----------|
| TRPC_STREAM_CLIENT_NETWORK_ERR = 301          | 客户端流式网络错误 |
| TRPC_STREAM_CLIENT_MSG_EXCEED_LIMIT_ERR = 311 | 流消息过大     |
| TRPC_STREAM_CLIENT_ENCODE_ERR = 321           | 客户端流编码错误  |
| TRPC_STREAM_CLIENT_DECODE_ERR = 322           | 客户端流编解码错误 |
| TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR = 354     | 客户端流读超时   |

## 搭建流式 RPC 服务

在搭建流式服务之前，先回顾下单次 RPC 服务搭建过程，主要步骤如下：

* 定义 RPC 接口。
* 生成 RPC 服务代码。
* 实现 RPC 服务业务逻辑。
* 在客户端调用 RPC 服务。

搭建 tRPC 流式 RPC 服务的步骤和搭建单次 RPC 服务大体一致，下面我们就按照这种方式来搭建流式 RPC 服务。
主要以 Client-Server场景、Client-Router-Server 场景举例说明。

### Client-Server 场景

下面以 SayHello 为例，在 Client-Server 场景下演示搭建流式 RPC 服务过程。
两个模块如下：

* StreamClient：流式 RPC 客户端，发起流式 RPC 调用。
* StreamServer：流式 RPC 服务端，提供流式 RPC 服务。

#### 定义流式 RPC 接口

StreamServer 模块流式 RPC 协议文件：stream.proto。

```protobuf
// @file: examples/features/trpc_stream/server/stream.proto
syntax = "proto3";

package trpc.test.helloworld;

service Greeter {
  // Client streaming
  rpc ClientStreamSayHello (stream HelloRequest) returns (HelloReply) {}

  // Server streaming
  rpc ServerStreamSayHello (HelloRequest) returns (stream HelloReply) {}

  // Bidirectional streaming
  rpc BidiStreamSayHello (stream HelloRequest) returns (stream HelloReply) {}
}

message HelloRequest {
  string msg = 1;
}

message HelloReply {
  string msg = 1;
}
```

#### 生成流式 RPC 服务代码

tRPC 提供的代码生成工具会自动生成流式 RPC 服务代码，简单看一下代码片段，对流式 RPC 有个初步印象。
StreamServer 模块服务端代码片段如下：

```
// @file: bazel-bin/examples/features/trpc_stream/server/stream.trpc.pb.h
...
class StreamGreeter: public ::trpc::RpcServiceImpl {
public:
...
  virtual ::trpc::Status ClientStreamSayHello(const ::trpc::ServerContextPtr& context, const ::trpc::stream::StreamReader<trpc::test::helloworld::HelloRequest>& reader, ::trpc::test::helloworld::HelloReply* response);
  virtual ::trpc::Status ServerStreamSayHello(const ::trpc::ServerContextPtr& context, const ::trpc::test::helloworld::HelloRequest& request, ::trpc::stream::StreamWriter<trpc::test::helloworld::HelloReply>* writer);
  virtual ::trpc::Status BidiStreamSayHello(const ::trpc::ServerContextPtr& context, const ::trpc::stream::StreamReader<trpc::test::helloworld::HelloRequest>& reader, ::trpc::stream::StreamWriter<trpc::test::helloworld::HelloReply>* writer);
};
...
```

#### 实现流式 RPC 服务业务逻辑

示例： [stream_service.cc](../../examples/features/trpc_stream/server/stream_server.cc)

*客户端流式*
StreamServer 模块中 ClientStreamSayHello 方法逻辑要点如下：

* 从流中持续读取请求消息，如果出错则停止读取，返回错误状态给客户端。
* 读取到流 EOF 后，将设置响应消息，并设置 RPC 调用结果状态，此状态码会返回给客户端。

简单示例代码：

```cpp
// Client streaming RPC
::trpc::Status StreamGreeterServiceImpl::ClientStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
    ::trpc::test::helloworld::HelloReply* reply) {
  ::trpc::Status status{};
  uint32_t request_counter{0};
  uint32_t request_bytes{0};
  for (;;) {
    ::trpc::test::helloworld::HelloRequest request{};
    // 读取客户端发送的流消息
    status = reader.Read(&request, 3000);
    if (status.OK()) {
      ++request_counter;
      request_bytes += request.msg().size();
      continue;
    }
    // 流结束
    if (status.StreamEof()) {
      std::stringstream reply_msg;
      reply_msg << "server got EOF, reply to client, server got request"
                << ", count:" << request_counter << ", received bytes:" << request_bytes;
      reply->set_msg(reply_msg.str());
      status = ::trpc::Status{0, 0, "OK"};
      break;
    }
    break;
  }
  return status;
}
```

*服务端流式*
StreamServer 模块 ServerStreamSayHello 逻辑要点如下：

* 将客户端发来的请求消息，以流式方式写回给客户端。
* 将RPC调用结果返回给客户端。

简单示例代码：

```cpp
// Server streaming RPC
::trpc::Status StreamGreeterServiceImpl::ServerStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::test::helloworld::HelloRequest& request, 
    ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) {
  ::trpc::Status status{};
  // 一个简单例子：尝试给客户端发送 10 条响应消息
  int request_count = 10;
  for (int i = 0; i < request_count; ++i) {
    std::stringstream reply_msg;
    ::trpc::test::helloworld::HelloReply reply{};
    reply.set_msg(reply_msg.str());
    // 写入发送给客户端的流消息
    status = writer->Write(reply);
    if (status.OK()) {
      continue;
    }
    break;
  }
  return status;
}
```

*双向流式*
StreamServer模块 BidiStreamSayHello 逻辑要点如下：

* 从客户端侧持续读入请求消息，并立即发给给客户端。
* 如果读取到 EOF，则额外发送一个请求消息统计信息给客户端。
* 将 RPC 调用结果返回给客户端。

简单示例代码：

```cpp
// Bidirectional streaming RPC.
::trpc::Status StreamGreeterServiceImpl::BidiStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
    ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) {
  std::vector<std::string> msg_list{};
  ::trpc::Status status{};
  uint32_t request_counter{0};
  uint32_t request_bytes{0};
  for (;;) {
    ::trpc::test::helloworld::HelloRequest request{};
    // 读取客户端发送的流消息
    status = reader.Read(&request, 3000);
    if (status.OK()) {
      ++request_counter;
      request_bytes += request.msg().size();
      std::stringstream reply_msg;
      reply_msg << " reply:" << request_counter << ", received bytes:" << request_bytes;
      ::trpc::test::helloworld::HelloReply reply;
      reply.set_msg(reply_msg.str());
      // 写入发送给客户端的流消息
      writer->Write(reply);
      continue;
    }
    // 流结束
    if (status.StreamEof()) {
      std::stringstream reply_msg;
      reply_msg << "server got EOF, reply to client, server got request"
                << ", count:" << request_counter << ", received bytes:" << request_bytes;
      ::trpc::test::helloworld::HelloReply reply;
      reply.set_msg(reply_msg.str());
      status = writer->Write(reply);
    }
    break;
  }
  return status;
}
```

### Client-Router-Server 场景

下面以 SayHello 为例，在 Client-Router-Server 场景下演示搭建流式 RPC 服务过程。
三个模块如下：

* StreamClient：流式 RPC 客户端，发起流式 RPC 调用。
* StreamServer：流式 RPC 服务端，提供流式 RPC 服务（沿用 Client-Server 场景中的 StreamServer）。
* StreamForwardServer：接收来自 StreamClient 的请求并转发给 StreamServer，然后将 StreamServer 的响应转回给 StreamClient。

#### 定义流式 RPC 接口

StreamForwardServer 模块流式 RPC 协议文件：stream_forward.proto。

```protobuf
// @file: examples/features/trpc_stream/forward/proxy/stream_forward.proto
syntax = "proto3";

package trpc.test.helloworld;

import "examples/features/trpc_stream/server/stream.proto";

service StreamForward {
  // Client streaming
  rpc ClientStreamSayHello (stream HelloRequest) returns (HelloReply) {}

  // Server streaming
  rpc ServerStreamSayHello (HelloRequest) returns (stream HelloReply) {}

  // Bidirectional streaming
  rpc BidiStreamSayHello (stream HelloRequest) returns (stream HelloReply) {}
}
```

#### 生成流式 RPC 服务代码

StreamForwardServer 模块服务端代码片段如下：

```
// @file: bazel-bin/examples/features/trpc_stream/forward/proxy/stream_forward.trpc.pb.h
...
class StreamForward: public ::trpc::RpcServiceImpl {
public:
...
  virtual ::trpc::Status ClientStreamSayHello(const ::trpc::ServerContextPtr& context, const ::trpc::stream::StreamReader<trpc::test::helloworld::HelloRequest>& reader, ::trpc::test::helloworld::HelloReply* response);
  virtual ::trpc::Status ServerStreamSayHello(const ::trpc::ServerContextPtr& context, const ::trpc::test::helloworld::HelloRequest& request, ::trpc::stream::StreamWriter<trpc::test::helloworld::HelloReply>* writer);
  virtual ::trpc::Status BidiStreamSayHello(const ::trpc::ServerContextPtr& context, const ::trpc::stream::StreamReader<trpc::test::helloworld::HelloRequest>& reader, ::trpc::stream::StreamWriter<trpc::test::helloworld::HelloReply>* writer);
};
...
```

#### 实现流式 RPC 服务业务逻辑

示例： [stream_forward_service.cc](../../examples/features/trpc_stream_forward/proxy/stream_forward_service.cc)

*客户端流式*
StreamForwardServer 中 ClientStreamSayHello 逻辑要点如下：

* 调用 StreamServer 的 ClientStreamSayHello 方法，获取到客户端流读取器。
* 从客户端持续读取请求消息，并发送给 StreamServer，过程如果有错误则停止读取/写入。
* 如果从客户端流中读取到 EOF，则等待 StreamServer 返回 RPC 方法调用结果并返回给客户端。

```cpp
// Client streaming RPC.
::trpc::Status StreamForwardImpl::ClientStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
    ::trpc::test::helloworld::HelloReply* reply) {
  ::trpc::Status status{};
  const StreamGreeterServiceProxyPtr& stream_greeter_proxy = GetStreamGreeterServiceProxyPtr();
  auto client_context = ::trpc::MakeClientContext(context, stream_greeter_proxy);
  do {
    // 打开 StreamGreeter 流
    auto stream = stream_greeter_proxy->ClientStreamSayHello(client_context, reply);
    status = stream.GetStatus();
    if (!status.OK()) {
      break;
    }

    for (;;) {
      ::trpc::test::helloworld::HelloRequest request;
      // 读取客户端发送的流消息，并尝试发送给 StreamGreeter
      status = reader.Read(&request);
      if (status.OK()) {
        status = stream.Write(request);
        if (status.OK()) {
          continue;
        }
      }
      // 流结束
      if (status.StreamEof()) {
        status = stream.WriteDone();
        if (status.OK()) {
          status = stream.Finish();
        }
      }
      break;
    }
  } while (0);
  TRPC_FMT_INFO("final status: {}", status.ToString());
  return status;
}
```

*服务端流式*
StreamRouter 模块 ServerStreamSayHello 逻辑要点：

* 调用 StreamServer 模块 ServerStreamSayHello 方法，获取流读取器。
* 从 StreamServer 持续读入响应消息，并发送给客户端，过程如果有错误则停止读取/写入。
* 如果从流中读取到EOF，则等待 StreamServer 返回 RPC 方法调用结果并返回给客户端。

```cpp
// Server streaming RPC.
::trpc::Status StreamForwardImpl::ServerStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::test::helloworld::HelloRequest& request,  // NO LINT
    ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) {
  ::trpc::Status status{};
  const StreamGreeterServiceProxyPtr& stream_greeter_proxy = GetStreamGreeterServiceProxyPtr();
  auto client_context = ::trpc::MakeClientContext(context, stream_greeter_proxy);
  do {
    // 打开 StreamGreeter 流
    auto stream = stream_greeter_proxy->ServerStreamSayHello(client_context, request);
    status = stream.GetStatus();
    if (!status.OK()) {
      break;
    }

    for (;;) {
      ::trpc::test::helloworld::HelloReply reply;
     // 读取 StreamGreeter 回复的流消息，并尝试发送给客户端
      status = stream.Read(&reply);
      if (status.OK()) {
        status = writer->Write(reply);
        if (status.OK()) {
          continue;
        }
      }
      // 流结束
      if (status.StreamEof()) {
        status = stream.Finish();
      }
      break;
    }
  } while (0);
  TRPC_FMT_INFO("final status: {}", status.ToString());
  return status;
}
```

*双向流式*
StreamForwardServer 模 BidiStreamSayHello 逻辑要点：

* 调用 StreamServer 模块 BidiStreamSayHello 方法，获取流读取/写入器。
* 从客户端持续读入请求消息，并发送给 StreamServer，过程如果有错误则停止读取/写入。
* 如果从流中读取到 EOF，则通知 StreamServer 请求写入结束。
* 从 StreamServer 持续读取响应消息，并发送给客户端。
* 如果从流中读取到 EOF，则等待 RPC 调用结果，并返回给客户端。

```cpp
// Bidirectional streaming RPC.
::trpc::Status StreamForwardImpl::BidiStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
    ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) {
  ::trpc::Status status{};
  const StreamGreeterServiceProxyPtr& stream_greeter_proxy = GetStreamGreeterServiceProxyPtr();
  auto client_context = ::trpc::MakeClientContext(context, stream_greeter_proxy);
  do {
    // 打开 StreamGreeter 流
    auto stream = stream_greeter_proxy->BidiStreamSayHello(client_context);
    status = stream.GetStatus();
    if (!status.OK()) {
      break;
    }

    for (;;) {
      ::trpc::test::helloworld::HelloRequest request;
     // 读取客户端发送的流消息，并尝试发送给 StreamGreeter
      status = reader.Read(&request);
      if (status.OK()) {
        status = stream.Write(request);
        if (status.OK()) {
          continue;
        }
      }
      // 客户端流结束
      if (status.StreamEof()) {
        status = stream.WriteDone();
        if (!status.OK()) {
          break;
        }
        for (;;) {
          ::trpc::test::helloworld::HelloReply reply;
          // 读取 StreamGreeter 回复的流消息，并尝试发送给客户端
          status = stream.Read(&reply);
          if (status.OK()) {
            status = writer->Write(reply);
            if (status.OK()) {
              continue;
            }
          }
          if (status.StreamEof()) {
            status = stream.Finish();
          }
          break;
        }
      }
      break;
    }
  } while (0);
  TRPC_FMT_INFO("final status: {}", status.ToString());
  return status;
}
```

# 如何开发异步流式 RPC 服务

提示：异步流式 RPC 服务需要运行在 `merge` 线程模型中。

## 体验流式 RPC 服务

示例： [trpc_async_stream](../../examples/features/trpc_async_stream)

方法：进入 tRPC-Cpp 代码仓库主目录，然后运行下面的命令。

```shell
sh examples/features/trpc_async_stream/run.sh
```

示例程序输出：

``` text
name: Async Streaming RPC, ClientStreamSayHello1, ok: 1
name: Async Streaming RPC, ClientStreamSayHello2, ok: 1
name: Async Streaming RPC, ClientStreamSayHello3, ok: 1
name: Async Streaming RPC, ClientStreamSayHello4, ok: 1
name: Async Streaming RPC, ClientStreamSayHello5, ok: 1
name: Async Streaming RPC, ClientStreamSayHello6, ok: 1
name: Async Streaming RPC, ClientStreamSayHello7, ok: 1
name: Async Streaming RPC, ClientStreamSayHello8, ok: 1
final result of streaming RPC calling: 1
```

## 异步流式接口

在 `merge` 线程模型下，流式 RPC 基本开发流程和规范与 `fiber` 线程模式下相同。
不同之处在于 `merge` 模式下的接口均为异步 `Future/Promise` 风格。如上述的 stream.proto 文件，生成的桩代码为：

```cpp
// 服务端新桩代码（相比于同步流式服务端桩代码）
class AsyncStreamGreeter : public RpcServiceImpl {
 public:
  AsyncGreeter();
  // Unary RPC，异步接口（merge 线程模型）
  virtual Future<HelloReply> SayHello(const ServerContextPtr& context, const HelloRequest* request);
  // Client Streaming RPC，异步接口（merge 线程模型）
  virtual Future<HelloReply> ClientStreamSayHello(const ServerContextPtr& context, const stream::AsyncReaderPtr<HelloRequest>& reader);
  // Server Streaming RPC，异步接口（merge 线程模型）
  virtual Future<> ServerStreamSayHello(const ServerContextPtr& context, HelloRequest&& request, const stream::AsyncWriterPtr<HelloReply>& writer);
  // Bidi Streaming RPC，异步接口（merge 线程模型）
  virtual Future<> BidiStreamSayHello(const ServerContextPtr& context, const stream::AsyncReaderWriterPtr<HelloRequest, HelloReply>& rw);
};
```

### 异步流读写器接口

异步流式 RPC 新增了流异步读写器 `AsyncReader, AsyncWriter` 。

说明：接口中的 `R` `W` 是模版参数，表示待读取的消息和待写入的流消息对象。

| 类/对象              | 接口名称                                                                   | 功能       | 参数                       | 返回值                                                   |
|-------------------|------------------------------------------------------------------------|----------|--------------------------|-------------------------------------------------------|
| AsyncReader       | Future&lt;std::optional&lt;R>> Read(std::chrono::milliseconds timeout) | 从流中读取消息  | timeout 等待超时，单位：毫秒，默认不超时 | Future<std::optional<R>>, std::optional<R> 为空表示对端 EOF |
| AsyncReader       | Future<> Finish()                                                      | RPC 调用结束 | void                     | Future<>                                              |
| AsyncWriter       | Future<> Write(const W& msg)                                           | 写入消息到流中  | msg 待写入的流消息              | Future<>                                              |
| AsyncWriter       | Future<> Finish()                                                      | RPC 调用结束 | void                     | Future<>                                              |
| AsyncReaderWriter | Future&lt;std::optional&lt;R>> Read(std::chrono::milliseconds timeout) | 从流中读取消息  | timeout 等待超时，单位：毫秒，默认不超时 | Future<std::optional<R>>, std::optional<R> 为空表示对端 EOF |
| AsyncReaderWriter | Future<> Write(const W& msg)                                           | 写入消息到流中  | msg 待写入的流消息              | Future<>                                              |
| AsyncReaderWriter | Future<> Finish()                                                      | RPC 调用结束 | void                     | Future<>                                              |

可以看到，异步流读写器接口更加简洁，语义也更加明确，不再需要用户关心流的内部状态。
按照设计要求，异步流使用有如下规范：

* 同一个流的异步读器（AsyncReader）必须串行调用，也即必须在上一次 Read 就绪（Ready/Failed）后，才能调用下一次 Read。
* 同一个流的异步写器（AsyncWriter）必须串行调用，也即必须在上一次 Write 就绪（Ready/Failed）后，才能调用下一次 Write。
* 同一个流的异步读器和写器为双工操作，可并发调用。
* 各个异步读写器的 Finish 语义完全相同，同一个流只能调用最多一次（客户端和服务端独立统计）。并且 Finish 也视为写操作，因此需要和
  Write 串行调用。
* 流的建立：当用户得到一个流的异步读器或写器时，意味着流已经协商建立完成了（tRPC 会自动打开流）。
* 流的关闭：
    - 对于客户端，调用 Finish 将主动关闭流，并在双方协商关闭后就绪（目前只会Ready，即使流错误也不会Failed）。用户可选择调用（建议客户端主动调用）。
    - 对于服务端，在用户服务函数（如用户实现的AsyncStreamGreeter服务函数）返回的 Future 就绪后，tRPC 会自动关闭流；调用
      Finish 则不会发起操作，只会在双方协商关闭后就绪（目前只会 Ready，即使流错误也不会 Failed）。用户可选择调用。
* 当前流式 RPC 的接口均不支持 PB Arena 能力。

### Future 工具接口

为了更好地支持 Future 风格异步编程，tRPC 提供了多种工具，如下：

```cpp
// 以下工具函数均为模板，此处不展示模板参数，详见：trpc/future/future_utility.h

Future<> DoUntil(Stop&& stop, Func&& func);
Future<> DoUntil(Func&& func);
Future<> DoWhile(Condition&& cond, Func&& func);
Future<> Repeat(Func&& func);
Future<> DoForEach(Iterator&& begin, Iterator&& end, Func&& func);
Future<> DoForEach(Container& container, Func&& func);
Future<> DoFor(std::size_t n, Func&& func);

R DoWith(T &&value, Func &&func);
Future<std::tuple<FutsType...>> WhenAll(FutsType &&... futs);
Future<std::vector<FutureType>> WhenAll(InputIterator first, InputIterator last);
Future<size_t, FutureValueType> WhenAny(InputIterator first, InputIterator last);
Future<size_t, FutureValueType> WhenAnyWithoutException(InputIterator first, InputIterator last);
```

## 实现流式 RPC 服务业务逻辑

示例 [stream_service.cc](../../examples/features/trpc_async_stream/server/stream_service.cc)

*客户端流式*

```cpp
// Client streaming RPC.
::trpc::Future<::trpc::test::helloworld::HelloReply> AsyncStreamGreeterServiceImpl::ClientStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::AsyncReaderPtr<::trpc::test::helloworld::HelloRequest>& reader) {
  struct State {
    std::size_t request_counter{0};
    std::size_t request_bytes{0};
  };
  auto state = std::make_shared<State>();

  return ::trpc::DoUntil([state, reader]() {
           // 循环顺序读取流消息
           return reader->Read(3000ms).Then(
               [state, reader](std::optional<::trpc::test::helloworld::HelloRequest>&& req) {
                 if (req) {
                   ++state->request_counter;
                   state->request_bytes += req.value().msg().size();
                   // 循环：继续读取下一个流消息
                   return ::trpc::MakeReadyFuture<bool>(true);
                 } else {
                   // 流结束 EOF
                   // 循环：结束读取
                   return ::trpc::MakeReadyFuture<bool>(false);
                 }
               });
         })
      .Then([state]() {
        auto msg =
            ::trpc::util::FormatString("get {} requests, {} bytes", state->request_counter, state->request_bytes);
        ::trpc::test::helloworld::HelloReply reply;
        reply.set_msg(std::move(msg));
        return ::trpc::MakeReadyFuture<::trpc::test::helloworld::HelloReply>(std::move(reply));
      });
}
```

*服务端流式*

```cpp
// Server streaming RPC.
::trpc::Future<> AsyncStreamGreeterServiceImpl::ServerStreamSayHello(
    const ::trpc::ServerContextPtr& context, ::trpc::test::helloworld::HelloRequest&& request,
    const ::trpc::stream::AsyncWriterPtr<::trpc::test::helloworld::HelloReply>& writer) {
  // 尝试发送 10 个流消息给客户端
  return ::trpc::DoFor(10,
                       [writer, request = std::move(request)](std::size_t i) {
                         auto msg = ::trpc::util::FormatString("{}#{}", request.msg(), i + 1);
                         ::trpc::test::helloworld::HelloReply reply;
                         reply.set_msg(std::move(msg));
                         return writer->Write(std::move(reply));
                       })
      .Then([]() {
        // Trigger a read timeout(>3000ms) for client to show its usage
        return ::trpc::AsyncTimer(false).After(3050 /*ms*/);
      });
}
```

*双端流式*

```cpp
// Bidirectional streaming RPC.
::trpc::Future<> AsyncStreamGreeterServiceImpl::BidiStreamSayHello(
    const ::trpc::ServerContextPtr& context,
    const ::trpc::stream::AsyncReaderWriterPtr<::trpc::test::helloworld::HelloRequest,
                                               ::trpc::test::helloworld::HelloReply>& rw) {
  struct State {
    std::size_t request_counter{0};
    std::size_t request_bytes{0};
  };
  auto state = std::make_shared<State>();

  
  return ::trpc::DoUntil([state, rw]() {
   // 循环顺序读取流消息，并尝试回复给客户端
    return rw->Read(3000ms).Then([state, rw](std::optional<::trpc::test::helloworld::HelloRequest>&& req) {
      if (req) {
        ++state->request_counter;
        state->request_bytes += req.value().msg().size();
        auto msg =
            ::trpc::util::FormatString("get {} requests, {} bytes", state->request_counter, state->request_bytes);
        ::trpc::test::helloworld::HelloReply reply;
        reply.set_msg(std::move(msg));
        return rw->Write(std::move(reply)).Then([]() { return ::trpc::MakeReadyFuture<bool>(true); });
      } else {
        // 流结束 EOF
        // 循环：结束读取
        return ::trpc::MakeReadyFuture<bool>(false);
      }
    });
  });
}
```

# FAQ

## 1 tRPC 流式和 gRPC 流式的区别是什么？

tRPC 流式是基于 tcp 协议的 RPC 协议，而 gRPC 是基于 http2 的通用 7 层协议。
从实现复杂度上说，HTTP/2.0 比 tRPC 复杂，作为一个标准协议，需要考虑和遵循的细节较多。
从功能的角度上说，二者都可以进行流式传输，服务端和客户端可以进行交互式响应，tRPC 更多是基于 RPC 来考虑。

## 2 为什么同步流式接口中，服务端没有调用 Finish/WriteDone 接口

服务端也能调用 Finish/WriteDone，只是什么也不执行。那怎么在服务端停止接收或者发送呢？
如果服务端想停止接收/发送，直接 return 即可，代表流的结束。

## 3 为什么异步流式接口中，没有 WriteDone？

在异步流式接口中，客户端在调用读写器的 Finish 接口时，即是通知了对端数据已写完（相当于WriteDone）。

## 4 异步流式仅能在 `merge` 线程模型运行吗？

是的，其他线程模型还未支持。
