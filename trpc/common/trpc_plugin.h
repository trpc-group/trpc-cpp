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

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "trpc/auth/auth.h"
#include "trpc/codec/client_codec.h"
#include "trpc/codec/server_codec.h"
#include "trpc/compressor/compressor.h"
#include "trpc/config/codec.h"
#include "trpc/config/config.h"
#include "trpc/config/provider.h"
#include "trpc/filter/filter.h"
#include "trpc/metrics/metrics.h"
#include "trpc/naming/limiter.h"
#include "trpc/naming/load_balance.h"
#include "trpc/naming/registry.h"
#include "trpc/naming/selector.h"
#include "trpc/serialization/serialization.h"
#include "trpc/telemetry/telemetry.h"
#include "trpc/tracing/tracing.h"

namespace trpc {

/// @brief Plugin management class
class TrpcPlugin {
 public:
  static TrpcPlugin* GetInstance() {
    static TrpcPlugin instance;
    return &instance;
  }

  /////////////////////////////////////

  /// @brief Install plugin
  int RegisterPlugins();

  /// @brief Uninstall plugin
  int UnregisterPlugins();

  /////////////////////////////////////
  // The interface for registering custom codec/serialization/compress plugin

  /// @brief Register custom server codec plugin
  bool RegisterServerCodec(const ServerCodecPtr& codec);

  /// @brief Register custom client codec plugin
  bool RegisterClientCodec(const ClientCodecPtr& codec);

  /// @brief Register custom compress plugin
  bool RegisterCompress(const compressor::CompressorPtr& compressor);

  /// @brief Register custom serialization plugin
  bool RegisterSerialization(const serialization::SerializationPtr& serialization);

  /////////////////////////////////////
  // The interface for registering custom filter

  /// @brief Register custom server filter
  bool RegisterServerFilter(const MessageServerFilterPtr& filter);

  /// @brief Register custom client filter
  bool RegisterClientFilter(const MessageClientFilterPtr& filter);

  /////////////////////////////////////
  // The interface for registering custom registry/selector/loadbalance/limiter plugin

  /// @brief Register custom server registry plugin
  bool RegisterRegistry(const RegistryPtr& registry);

  /// @brief Register custom client selector plugin
  bool RegisterSelector(const SelectorPtr& selector);

  /// @brief Register custom client loadbalance plugin
  /// In design, different selectors do not reuse the same loadbalance object.
  /// If two selectors use the same loadbalance algorithm, you need to create and register two loadbalance plugins.
  /// It is recommended that the name start with the selector name corresponding to `loadbalance name` to distinguish
  bool RegisterLoadBalance(const LoadBalancePtr& load_balance);

  /// @brief Register custom limiter plugin
  bool RegisterLimiter(const LimiterPtr& limiter);

  /////////////////////////////////////

  /// @brief Register custom metrics plugin
  bool RegisterMetrics(const MetricsPtr& metrics);

  /////////////////////////////////////

  /// @brief Register custom config-codec plugin
  bool RegisterConfigCodec(const config::CodecPtr& codec);

  /// @brief Register custom config-provider plugin
  bool RegisterConfigProvider(const config::ProviderPtr& provider);

  /// @brief Register custom config-center plugin(has deprecated)
  bool RegisterConfig(const ConfigPtr& config);

  /////////////////////////////////////

  /// @brief Register the remote logging plugin
  bool RegisterLogging(const LoggingPtr& logging);

  /////////////////////////////////////

  /// @brief Register custom tracing plugin
  bool RegisterTracing(const TracingPtr& tracing);

  /// @brief Register custom telemetry plugin
  bool RegisterTelemetry(const TelemetryPtr& telemetry);

  /////////////////////////////////////

  /// @brief Register custom auth plugin
  bool RegisterAuth(const AuthPtr& auth);

  /// @brief Register custom auth-follower plugin
  bool RegisterAuthFollower(const AuthCenterFollowerPtr& follower);

  /////////////////////////////////////

  /// @brief Whether the plugin register/unregister is invoked by framework
  /// @note This interface only used by framework
  void SetInvokeByFramework();

  /// @brief Destroy plugin-related resources
  /// @note This interface only used by framework
  void DestroyResource();

 private:
  struct PluginInfo {
    PluginPtr plugin;
    bool is_inited{false};
    bool is_started{false};
    bool is_stoped{false};
    bool is_destroyed{false};
  };

  TrpcPlugin() = default;
  TrpcPlugin(const TrpcPlugin&) = delete;
  TrpcPlugin& operator=(const TrpcPlugin&) = delete;

  void CollectPlugins();
  void InitPlugins();
  void InitPlugin(PluginInfo& plugin_info);
  void StartPlugins();
  void StartPlugin(PluginInfo& plugin_info);
  void StopPlugins();
  void StopPlugin(PluginInfo& plugin_info);
  void DestroyPlugins();
  void DestroyPlugin(PluginInfo& plugin_info);
  std::unordered_map<std::string, PluginInfo>::iterator FindPlugin(const std::string& name);
  bool IsDepPluginNameValid(const std::vector<std::string>& dep_plugin_names);

  template <typename T>
  bool AddPlugins(const std::unordered_map<std::string, T>& plugins) {
    for (const auto& it : plugins) {
      PluginInfo plugin_info;
      plugin_info.plugin = it.second;
      plugin_info.is_inited = false;
      plugin_info.is_destroyed = false;

      std::string plugin_name = it.second->Name() + "#" + std::to_string(static_cast<int>(it.second->Type()));
      plugins_.emplace(plugin_name, std::move(plugin_info));

      std::vector<std::string> dep_plugin_names;
      it.second->GetDependencies(dep_plugin_names);
      // Check if the dependent plugin name is valid
      TRPC_ASSERT(IsDepPluginNameValid(dep_plugin_names) && "Dependent plugin name invalid");
      for (auto& dep_plugin_name : dep_plugin_names) {
        plugins_reverse_deps_[dep_plugin_name].push_back(plugin_name);
      }
    }

    return true;
  }

  bool InitRuntime_();
  void DestroyRuntime_();

 private:
  std::mutex mutex_;

  std::unordered_map<std::string, PluginInfo> plugins_;
  // The reverse dependency relationship of the plugins.
  std::unordered_map<std::string, std::vector<std::string>> plugins_reverse_deps_;

  bool is_all_inited_{false};

  bool is_all_destroyed_{false};

  // `RegisterPlugins`/`RegisterPlugins` is invoked by framework
  bool is_invoke_by_framework_{false};
};

}  // namespace trpc
