//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include "trpc/common/status.h"
#include "trpc/server/stream_rpc_method_handler.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

#include "examples/features/trpc_stream/server/stream.trpc.pb.h"

namespace test::helloworld {

class StreamGreeterServiceImpl : public ::trpc::test::helloworld::StreamGreeter {
 public:
  // Client streaming RPC.
  ::trpc::Status ClientStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
      ::trpc::test::helloworld::HelloReply* reply) override;

  // Server streaming RPC.
  ::trpc::Status ServerStreamSayHello(
      const ::trpc::ServerContextPtr& context, const ::trpc::test::helloworld::HelloRequest& request,
      ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) override;

  // Bi-direction streaming RPC.
  ::trpc::Status BidiStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::StreamReader<::trpc::test::helloworld::HelloRequest>& reader,
      ::trpc::stream::StreamWriter<::trpc::test::helloworld::HelloReply>* writer) override;
};

class RawDataStreamService : public ::trpc::RpcServiceImpl {
 public:
  RawDataStreamService();
  ::trpc::Status RawDataReadWrite(const ::trpc::ServerContextPtr& context,
                                  const ::trpc::stream::StreamReader<::trpc::NoncontiguousBuffer>& reader,
                                  ::trpc::stream::StreamWriter<::trpc::NoncontiguousBuffer>* writer);
};

}  // namespace test::helloworld
