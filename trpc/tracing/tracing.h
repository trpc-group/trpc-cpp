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

#include <memory>

#include "trpc/common/plugin.h"

namespace trpc {

/// @brief The abstract interface definition for tracing plugins.
class Tracing : public Plugin {
 public:
  /// @brief Gets the plugin type
  /// @return plugin type
  PluginType Type() const override { return PluginType::kTracing; }
};

using TracingPtr = RefPtr<Tracing>;

}  // namespace trpc
