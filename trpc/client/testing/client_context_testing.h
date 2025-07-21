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

#include <string>

#include "trpc/codec/client_codec_factory.h"
#include "trpc/client/client_context.h"

namespace trpc::testing {

ClientContextPtr MakeTestClientContext(uint32_t seq_id, int timeout, NetworkAddress addr) {
  ClientContextPtr context = MakeRefCounted<ClientContext>();

  context->SetRequestId(seq_id);
  context->SetTimeout(timeout);
  context->SetAddr(addr.Ip(), addr.Port());

  return context;
}

ClientContextPtr MakeTestClientContext(const std::string& protocol, uint32_t seq_id,
                                       uint32_t timeout, NetworkAddress addr) {
  ClientCodecPtr codec = ClientCodecFactory::GetInstance()->Get(protocol);

  ClientContextPtr context = MakeRefCounted<ClientContext>(codec);

  context->SetRequestId(seq_id);
  context->SetTimeout(timeout);
  context->SetAddr(addr.Ip(), addr.Port());

  return context;
}

}  // namespace trpc::testing
