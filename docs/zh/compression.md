[English](../en/compression.md)

# 压缩-解压缩

# 前言

本文介绍如何基于 tRPC-Cpp （下面简称 tRPC）使用压缩、解压缩特性，开发者可以了解到如下内容：

* 如何开启 RPC 调用的压缩、解压缩特性。
* 如何实现自定义的压缩、解压缩算法。

为了节省数据传输带宽，压缩数据是一个常见的做法。tRPC 支持 RPC 调用时压缩消息内容，并提供了一些常用的压缩算法。
我们可以按需选择合适的算法来做数据压缩。

# 如何开启 RPC 调用的压缩、解压缩特性

tRPC 框架的`tRPC`协议（使用时，一般写作 `trpc`）支持数据压缩/解压缩。
其他协议不一定支持压缩/解压缩，需要看具体实现情况）。

## 特性介绍

当前支持如下压缩算法：

* zlib
* gzip
* snappy
* lz4

当前支持如下压缩等级：

* kFastest
* kDefault
* kBest

压缩效果从上至下逐渐变好，同时会消耗更多的 CPU。如果没有特别指定压缩等级，框架一般采用 kDefault 。

**提示：snappy 无压缩等级参数，使用时，直接设置压缩算法即可。**

以下面的调用链为例，说明在 RPC 调用过程中压缩算法的选择策略。

```mermaid
graph LR
    A[Server A] -->|1 Request-B | B[Server B]
    B -->|2 Request-C| C[Server C]
    C --> |3 Response-C | B
    B --> |4 Response-B| A
```

在 A->B 场景下，B 在回复响应时，默认使用 A 设置的压缩算法（压缩参数会设置在请求中），并且压缩等级使用 kDefault，除非 B
中额外设置了响应消息的压缩参数。

例如，如果上图中的 Request-B 求使用了 kFastest 级别的 gzip 压缩了请求消息，并且 Server B 中没有设置任何压缩参数，则 Server B
在回复 Response-B 时会使用 kDefault 级别的 gzip 来压缩响应消息。

在 A->B->C 这种场景下，A 设置的压缩参数不会默认地传递给 C，除非 B 额外设置了压缩参数给 C。
例如，如果上图中的 Request-B 使用 kFastest 级别的 gzip 压缩了请求消息， 并且上图中 Request-C 未额外设置压缩参数，则 C
会收到未压缩的请求消息。

## 使用压缩、解压缩特性

示例：[trpc_compression](../../examples/features/trpc_compressor)

在访问 RPC 服务过程中，框架会识别压缩算法和压缩等级参数并自动压缩/解压缩消息内容。
如果设置了压缩参数，在客户端，框架会自动压缩请求消息内容并解压缩响应消息；在服务端，框架会自动解压缩请求消息并压缩响应消息。

**设置请求的压缩参数**

请求下游服务时，设置压缩算法和压缩等级。

```cpp
  // ...
  auto context = ::trpc::MakeClientContext(prx);
  context->SetReqCompressType(type);
  context->SetReqCompressLevel(::trpc::compressor::kFastest);
  ::trpc::Status status = prx->SayHello(context, request, &reply);
  // ...
}
```

上述代码给请求设置了 gzip 压缩和 kFastest 压缩等级来压缩请求消息内容。

**更新响应的压缩参数**

如果请求中设置了压缩参数，服务端回复响应时会默认使用请求中的压缩算法，但压缩级别会使用 kDefault 。
如果你希望更新压缩参数，可以自行设置期望的压缩参数。
在服务端回包前，给 ServerContext 设置两个属性即可。

```cpp
  // ...
  // Zlib will be used in the response.
  context->SetRspCompressType(::trpc::compressor::kZlib);
  context->SetRspCompressLevel(::trpc::compressor::kFastest);
  // ...
```

**直接使用压缩、解压缩接口**

某些场景下，希望直接使用框架的压缩/解压缩能力。
- 方法：调用压缩、解压缩接口可以实现此目的。
- 场景：比如，压缩 HTTP 响应消息内容。

```cpp
#include "trpc/compressor/trpc_compressor.h"

::trpc::Status HttpRouteService::SayHello(::trpc::ServerContextPtr context,
                                          ::trpc::http::HttpRequestPtr req,
                                          ::trpc::http::HttpReply* rsp) {
  // ...
  bool ret = ::compressor::CompressIfNeeded(::trpc::compressor::kGzip, *req->GetMutableNonContiguousBufferContent(), ::trpc::compressor::kDefault);
  // ...
  ::trpc::Status status = proxy_->Post(client_context, url_, req->GetContent(), &rsp->GetContent());
  // ... 
  ret = ::trpc::compressor::DecompressIfNeeded(::trpc::compressor::kGzip, *rsp->GetMutableNonContiguousBufferContent());
  // ...
}
```


# 如何实现自定义的压缩、解压缩算法

如果框架当前实现的压缩算法中没有我们想要的压缩算法，我们可以实现 `compressor` 插件来满足自身需求。

## 基本步骤

我们需要关注 compressor 插件这些环节：
* 消息的压缩、解压缩。

基本的步骤：

1. 实现 compressor 插件接口，实现消息的压缩和解压缩。
2. 注册 compressor 插件。

## 实现过程

参考: [trpc_gzip](../../trpc/compressor/gzip)

**1. 实现 compressor 插件接口**

```cpp
class MyCompressor : public Compressor {
 public:
  MyCompressor Type() const override; 

 protected:
  bool DoCompress(const NoncontiguousBuffer& in, NoncontiguousBuffer& out, LevelType level) override;

  bool DoDecompress(const NoncontiguousBuffer& in, NoncontiguousBuffer& out) override;
};
```

*Type* 返回压缩算标识，返回一个 uint8_t 类型整数值，0~127 是框架保留使用的，我们可以选择一个大于 127 的值，不冲突即可。

*DoCompress* 压缩数据，将 |in| 中待压缩的数据成功压缩后存入 |out| 中； |level| 是压缩等级参数。

*DoDecompress* 解压缩数据，将 |in| 中待解压缩的数据成功解压缩后存入 |out| 中。

**2. 注册 compressor 插件**

在程序初始化的时候，注册自定义实现的 compressor 插件。

```cpp
class DemoServer : public ::trpc::TrpcApp {
 public:
  int RegisterPlugins() override {
    ::trpc::TrpcPlugin::GetInstance()->RegisterCompress(::trpc::MakeRefCounted<MyCompressor>());
    return 0;
  }
};
```

注册 MyCompressor 插件后，我们就可以方便地使用这个插件了，例如：在服务端设置响应的压缩算法为 MyCompressor。

```cpp
  // On server-side.
  // ...
  // MyCompressor will be used in the response.
  context->SetRspCompressType(my_compressor_type);
  context->SetRspCompressLevel(::trpc::compressor::kFastest);
  // ...
```
