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

#include <any>
#include <list>
#include <string>

#include "trpc/codec/server_codec.h"
#include "trpc/server/server_context.h"

namespace examples::thirdparty_protocol {

class DemoServerCodec : public ::trpc::ServerCodec {
 public:
  ~DemoServerCodec() override = default;

  std::string Name() const override { return "thirdpary_protocol"; }

  int ZeroCopyCheck(const ::trpc::ConnectionPtr& conn, ::trpc::NoncontiguousBuffer& in, std::deque<std::any>& out) override;

  bool ZeroCopyDecode(const ::trpc::ServerContextPtr& ctx, std::any&& in, ::trpc::ProtocolPtr& out) override;

  bool ZeroCopyEncode(const ::trpc::ServerContextPtr& ctx, ::trpc::ProtocolPtr& in, ::trpc::NoncontiguousBuffer& out) override;

  ::trpc::ProtocolPtr CreateRequestObject() override;

  ::trpc::ProtocolPtr CreateResponseObject() override;
};

}  // namespace examples::thirdparty_protocol
