[中文](../zh/compression.md)

# Compression or Decompression

# Overview

This article introduces how to use compression and decompression features based on tRPC-Cpp (referred to as tRPC below).
Developers can learn the following:

* How to enable compression and decompression features for RPC calls.
* How to implement custom compression and decompression algorithms.

To save data transmission bandwidth, compressing data is a common practice. tRPC supports compressing message content
during RPC calls and provides some commonly used compression algorithms. We can choose the appropriate algorithm as
needed for data compression.

# How to enable compression and decompression features for RPC calls

The "tRPC" protocol of the tRPC framework (usually written as "trpc" when used) supports data compression.
Other protocols may not necessarily support data compression, it depends on their specific implementation.

## Features

Currently, the following compression algorithms are supported:

* zlib
* gzip
* snappy
* lz4

The following compression levels are currently supported:

* kFastest
* kDefault
* kBest

The compression effect gradually improves from `kFastest` to `kBest`, but it also consumes more CPU. If the compression
level is not specified, the framework generally uses kDefault.

Note: Snappy does not have a compression level parameter. When using it, simply set the compression algorithm directly.

Taking the following call chain as an example, let's explain the strategy for choosing the compression algorithm during
an RPC call.

```mermaid
graph LR
    A[Server A] -->|1 Request-B | B[Server B]
    B -->|2 Request-C| C[Server C]
    C --> |3 Response-C | B
    B --> |4 Response-B| A
```

In the scenario of A->B, when B replies to the client, it will use the compression algorithm set in the request
by A at the kDefault level by default, unless B sets additional compression parameters for the response message.

For example, if Request-B in the figure uses gzip compression at the kFastest level for the request message, and Server
B does not set any compression parameters, then Server B will use gzip compression at the kDefault level to compress the
Response-B message when replying.

In the scenario of A->B->C, the compression parameters set by A will not be passed to C by default unless B sets
compression parameters for C.

For example, if Request-B in the figure compresses the request message using gzip at the kFastest level, and Request-C
in the figure does not set additional compression parameters, then C will receive an uncompressed request message.

## Usages

Example: [trpc_compression](../../examples/features/trpc_compressor)

During the process of accessing RPC services, the framework will recognize the compression algorithm and compression
level parameters and automatically compress/uncompress message content. If compression parameters are set, on the client
side, the framework will automatically compress the request message content and uncompress the response message content;
on the server side, the framework will automatically uncompress the request message content and compress the response
message content.

**Setting compression parameters for requests**

When requesting downstream services, set the compression algorithm and compression level.

```cpp
  // ...
  auto context = ::trpc::MakeClientContext(prx);
  context->SetReqCompressType(type);
  context->SetReqCompressLevel(::trpc::compressor::kFastest);
  ::trpc::Status status = prx->SayHello(context, request, &reply);
  // ...
}
```

The above code sets gzip compression and kFastest compression level to compress the request message content.

**Updating compression parameters for responses**

If compression parameters are set in the request, the server will use the compression algorithm specified in the request
by default, but the compression level will use kDefault. If you want to update the compression parameters, you can set
the expected compression parameters yourself. Before sending the response on the server side, set two properties for
ServerContext.

```cpp
  // ...
  // Zlib will be used in the response.
  context->SetRspCompressType(::trpc::compressor::kZlib);
  context->SetRspCompressLevel(::trpc::compressor::kFastest);
  // ...
```

**Using compression and decompression interfaces directly**

In some scenarios, you may want to use the framework's compression/decompression capabilities directly.

- Method: Calling the compression and decompression interfaces can achieve this purpose.
- Scenario: For example, compressing the content of an HTTP response message.

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

# How to implement custom compression and decompression algorithms

If the compression algorithm implemented by the framework does not meet our requirements, we can implement
a `compressor` plugin to meet our own needs.

## Basic steps

We need to focus on the following aspects of the compressor plugin:

* Compression and decompression of messages.

The basic steps are:

1. Implement the compressor plugin interface to compress and decompress messages.
2. Register the compressor plugin.

## Implementation process

Reference: [trpc_gzip](../../trpc/compressor/gzip)

**1. Implement the compressor plugin interface.**

```cpp
class MyCompressor : public Compressor {
 public:
  MyCompressor Type() const override; 

 protected:
  bool DoCompress(const NoncontiguousBuffer& in, NoncontiguousBuffer& out, LevelType level) override;

  bool DoDecompress(const NoncontiguousBuffer& in, NoncontiguousBuffer& out) override;
};
```

*Type* returns the compression algorithm identifier, which is a uint8_t integer value. Values from 0 to 127 are reserved
for use by the framework. We can choose a value greater than 127 that does not conflict with existing values.

*DoCompress* compresses the data in |in| and stores the compressed data in |out|. The |level| parameter is the
compression level.

*DoDecompress* decompresses the data in |in| and stores the decompressed data in |out|.

**2. Register the compressor plugin**

Register the custom implemented compressor plugin during program initialization.

```cpp
class DemoServer : public ::trpc::TrpcApp {
 public:
  int RegisterPlugins() override {
    ::trpc::TrpcPlugin::GetInstance()->RegisterCompress(::trpc::MakeRefCounted<MyCompressor>());
    return 0;
  }
};
```

After registering the MyCompressor plugin, we can easily use this plugin, for example, by setting the compression
algorithm of the response to MyCompressor on server-side.

```cpp
  // On server-side.
  // ...
  // MyCompressor will be used in the response.
  context->SetRspCompressType(my_compressor_type);
  context->SetRspCompressLevel(::trpc::compressor::kFastest);
  // ...
```
