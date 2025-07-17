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

#include "trpc/codec/server_codec.h"
#include "trpc/server/server_context.h"
#include "trpc/server/service.h"
#include "trpc/util/time.h"

namespace trpc {

/// @brief Create server context.
/// @return server context
ServerContextPtr MakeServerContext() {
  return MakeRefCounted<ServerContext>(); 
}

/// @brief Provider some interfaces for testing.
namespace testing {

/// @brief Make server context by service and codec
/// @note Only used for test
ServerContextPtr MakeServerContext(Service* service, ServerCodec* codec) {
  TRPC_ASSERT(service != nullptr);
  TRPC_ASSERT(codec != nullptr);
  ServerContextPtr context = MakeRefCounted<ServerContext>();

  context->SetRecvTimestampUs(trpc::time::GetMicroSeconds());
  context->SetService(service);
  context->SetFilterController(&(service->GetFilterController()));
  context->SetServerCodec(codec);
  context->SetRequestMsg(codec->CreateRequestObject());
  context->SetResponseMsg(codec->CreateResponseObject());

  return context;
}

}  // namespace testing

}  // namespace trpc