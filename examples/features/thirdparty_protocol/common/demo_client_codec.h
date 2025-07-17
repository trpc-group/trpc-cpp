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

#include <any>
#include <deque>
#include <string>

#include "trpc/client/client_context.h"
#include "trpc/codec/client_codec.h"

namespace examples::thirdparty_protocol {

class DemoClientCodec : public ::trpc::ClientCodec {
 public:
  ~DemoClientCodec() override = default;

  std::string Name() const override { return "thirdpary_protocol"; }

  int ZeroCopyCheck(const ::trpc::ConnectionPtr& conn, ::trpc::NoncontiguousBuffer& in,
                    std::deque<std::any>& out) override;

  bool ZeroCopyDecode(const ::trpc::ClientContextPtr& ctx, std::any&& in, ::trpc::ProtocolPtr& out) override;

  bool ZeroCopyEncode(const ::trpc::ClientContextPtr& ctx, const ::trpc::ProtocolPtr& in,
                      ::trpc::NoncontiguousBuffer& out) override;

  // bool FillRequest(const ::trpc::ClientContextPtr& ctx, const ::trpc::ProtocolPtr& in, void* body) override;
  // bool FillResponse(const ::trpc::ClientContextPtr& ctx, const ::trpc::ProtocolPtr& in, void* body) override;

  ::trpc::ProtocolPtr CreateRequestPtr() override;

  ::trpc::ProtocolPtr CreateResponsePtr() override;

  uint32_t GetSequenceId(const ::trpc::ProtocolPtr& rsp) const override;

  bool IsComplex() const override { return true; }
};

}  // namespace examples::thirdparty_protocol
