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

#include "trpc/telemetry/trpc_telemetry.h"

#include <vector>

#include "trpc/client/make_client_context.h"
#include "trpc/telemetry/telemetry_factory.h"
#include "trpc/tracing/tracing_filter_index.h"

namespace trpc::telemetry {

namespace {

std::vector<uint32_t> GetTracingPluginIDs() {
  std::vector<uint32_t> ids;

  const auto& telemetry_plugins = TelemetryFactory::GetInstance()->GetAllPlugins();
  for (const auto& pair : telemetry_plugins) {
    auto inner_tracing = pair.second->GetTracing();
    if (!inner_tracing) {
      continue;
    }
    ids.push_back(inner_tracing->GetPluginID());
  }
  return ids;
}

}  // namespace

bool Init() {
  std::vector<uint32_t> tracing_plugin_ids = GetTracingPluginIDs();
  if (tracing_plugin_ids.empty()) {
    return true;
  }

  // registers a MakeClientContextCallback to allow trace data to be passed from the server to the client.
  RegisterMakeClientContextCallback(
      [plugin_ids = std::move(tracing_plugin_ids)](const ServerContextPtr& server_ctx, ClientContextPtr& client_ctx) {
        for (uint32_t plugin_id : plugin_ids) {
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

void Destroy() { TelemetryFactory::GetInstance()->Clear(); }

}  // namespace trpc::telemetry
