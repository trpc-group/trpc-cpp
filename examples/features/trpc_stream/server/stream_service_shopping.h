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

#include "trpc/common/status.h"
#include "trpc/server/stream_rpc_method_handler.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"

#include "examples/features/trpc_stream/server/stream.trpc.pb.h"

namespace test::shopping {

class StreamShoppingServiceImpl : public ::trpc::test::shopping::StreamShopping {
 public:
  // Client streaming RPC.
  ::trpc::Status ClientStreamShopping(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::StreamReader<::trpc::test::shopping::ShoppingRequest>& reader,
      ::trpc::test::shopping::ShoppingReply* reply) override;

  // Server streaming RPC.
  ::trpc::Status ServerStreamShopping(
      const ::trpc::ServerContextPtr& context, const ::trpc::test::shopping::ShoppingRequest& request,
      ::trpc::stream::StreamWriter<::trpc::test::shopping::ShoppingReply>* writer) override;

  // Bi-direction streaming RPC.
  ::trpc::Status BidiStreamShopping(
      const ::trpc::ServerContextPtr& context,
      const ::trpc::stream::StreamReader<::trpc::test::shopping::ShoppingRequest>& reader,
      ::trpc::stream::StreamWriter<::trpc::test::shopping::ShoppingReply>* writer) override;

  // 新增抢购方法声明
  ::trpc::Status Purchase(const ::trpc::ServerContextPtr& context,
                          const ::trpc::test::shopping::ShoppingRequest& request,
                          ::trpc::test::shopping::ShoppingReply* reply) override;
};

class RawDataStreamService : public ::trpc::RpcServiceImpl {
 public:
  RawDataStreamService();
  ::trpc::Status RawDataReadWrite(const ::trpc::ServerContextPtr& context,
                                  const ::trpc::stream::StreamReader<::trpc::NoncontiguousBuffer>& reader,
                                  ::trpc::stream::StreamWriter<::trpc::NoncontiguousBuffer>* writer);
};

}  // namespace test::shopping
