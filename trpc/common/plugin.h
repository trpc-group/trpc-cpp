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
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "trpc/util/ref_ptr.h"

namespace trpc {

/// @brief all plugin type
enum class PluginType {
  kRegistry,
  kSelector,
  kDiscovery,
  kLoadbalance,
  kCircuitbreaker,
  kLimiter,
  kConfig,
  kConfigProvider,
  kConfigCodec,
  kMetrics,
  kTracing,
  kTelemetry,
  kAuth,
  kAuthCenterFollower,
  kLogging,
  kUnspecified,  // Unspecified type
};

/// @brief tRPC all plugin abstract base class
class Plugin : public RefCounted<Plugin> {
 public:
  Plugin() { plugin_id_ = plugin_id_counter.fetch_add(1); }

  virtual ~Plugin() {}

  /// @brief plugin type
  virtual PluginType Type() const = 0;

  /// @brief plugin name
  virtual std::string Name() const = 0;

  /// @brief get the collection of plugins that this plugin depends on
  ///        dependent plugins need to be initialized first
  /// @note If there are cases of dependencies with the same name (possible in cases where the types are
  ///       different), the plugin_name needs to follow the following format: 'plugin_name#plugin_type', where
  ///       plugin_name is the name of the plugin and plugin_type is the corresponding index of the plugin type.
  virtual void GetDependencies(std::vector<std::string>& plugin_names) const {}

  /// @brief init plugin
  virtual int Init() noexcept { return 0; }

  /// @brief start the runtime environment of the plugin
  virtual void Start() noexcept {}

  /// @brief Stop the runtime environment of the plugin
  virtual void Stop() noexcept {}

  /// @brief destroy plugin internal resources
  virtual void Destroy() noexcept {}

  virtual uint32_t GetPluginID() const noexcept final { return plugin_id_; }

 private:
  // Note that plugin_id_ is allocated in the range
  // (std::numeric_limits<uint16_t>::max(), std::numeric_limits<uint32_t>::max()), which is [65535, 2^32).
  uint32_t plugin_id_ = std::numeric_limits<uint16_t>::max();
  inline static std::atomic<uint32_t> plugin_id_counter{std::numeric_limits<uint16_t>::max()};
};

using PluginPtr = RefPtr<Plugin>;

}  // namespace trpc
