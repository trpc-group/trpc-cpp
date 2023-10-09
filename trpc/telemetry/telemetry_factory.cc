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

#include "trpc/telemetry/telemetry_factory.h"

#include "trpc/util/log/logging.h"

namespace trpc {

bool TelemetryFactory::Register(const TelemetryPtr& telemetry) {
  TRPC_ASSERT(telemetry != nullptr);
  telemetry_plugins_[telemetry->Name()] = telemetry;

  return true;
}

TelemetryPtr TelemetryFactory::Get(const std::string& name) {
  auto it = telemetry_plugins_.find(name);
  if (it != telemetry_plugins_.end()) {
    return it->second;
  }
  return nullptr;
}

const std::unordered_map<std::string, TelemetryPtr>& TelemetryFactory::GetAllPlugins() {
  return telemetry_plugins_;
}

void TelemetryFactory::Clear() { telemetry_plugins_.clear(); }

}  // namespace trpc
