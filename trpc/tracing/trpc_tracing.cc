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

#include "trpc/tracing/trpc_tracing.h"

#include <unordered_map>

#include "trpc/client/make_client_context.h"
#include "trpc/tracing/tracing_factory.h"
#include "trpc/tracing/tracing_filter_index.h"

namespace trpc::tracing {

bool Init() {
  if (TracingFactory::GetInstance()->GetAllPlugins().empty()) {
    return true;
  }

  // registers a MakeClientContextCallback to allow trace data to be passed from the server to the client.
  RegisterMakeClientContextCallback([](const ServerContextPtr& server_ctx, ClientContextPtr& client_ctx) {
    const auto& tracing_plugins = TracingFactory::GetInstance()->GetAllPlugins();
    for (const auto& pair : tracing_plugins) {
      uint16_t plugin_id = pair.second->GetPluginID();

      ClientTracingSpan client_span;
      ServerTracingSpan* server_span = server_ctx->GetFilterData<ServerTracingSpan>(plugin_id);
      if (server_span != nullptr) {
        client_span.parent_span = server_span->span;
      }
      client_ctx->SetFilterData(plugin_id, std::move(client_span));
    }
  });

  return true;
}

void Stop() {}

void Destroy() { TracingFactory::GetInstance()->Clear(); }

}  // namespace trpc::tracing
