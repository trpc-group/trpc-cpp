//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#pragma once

#include "examples/features/trpc_stream/server/stream.trpc.pb.h"

namespace test::helloworld {

class AsyncStreamGreeterServiceImpl : public ::trpc::test::helloworld::AsyncStreamGreeter {
 public:
  // Client streaming RPC.
  ::trpc::Future<::trpc::test::helloworld::HelloReply> ClientStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::AsyncReaderPtr<::trpc::test::helloworld::HelloRequest>& reader) override;

  // Server streaming RPC.
  ::trpc::Future<> ServerStreamSayHello(
      const ::trpc::ServerContextPtr& context, ::trpc::test::helloworld::HelloRequest&& request,
      const ::trpc::stream::AsyncWriterPtr<::trpc::test::helloworld::HelloReply>& writer) override;

  // Bi-direction streaming RPC.
  ::trpc::Future<> BidiStreamSayHello(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::AsyncReaderWriterPtr<::trpc::test::helloworld::HelloRequest,
                                                 ::trpc::test::helloworld::HelloReply>& rw) override;
};

}  // namespace test::helloworld
