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

#include "trpc/common/trpc_plugin.h"

#include "trpc/auth/auth_center_follower_factory.h"
#include "trpc/auth/auth_factory.h"
#include "trpc/client/trpc_client.h"
#include "trpc/codec/client_codec_factory.h"
#include "trpc/codec/codec_manager.h"
#include "trpc/codec/server_codec_factory.h"
#include "trpc/compressor/compressor_factory.h"
#include "trpc/compressor/trpc_compressor.h"
#include "trpc/config/config_factory.h"
#include "trpc/config/trpc_conf.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/filter/trpc_filter.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/trpc_metrics.h"
#include "trpc/naming/limiter_factory.h"
#include "trpc/naming/load_balance_factory.h"
#include "trpc/naming/registry_factory.h"
#include "trpc/naming/selector_factory.h"
#include "trpc/naming/trpc_naming_registry.h"
#ifdef TRPC_BUILD_INCLUDE_RPCZ
#include "trpc/rpcz/collector.h"
#endif
#include "trpc/overload_control/trpc_overload_control.h"
#include "trpc/runtime/common/periphery_task_scheduler.h"
#include "trpc/runtime/common/runtime_info_report/runtime_info_reporter.h"
#include "trpc/runtime/common/stats/frame_stats.h"
#include "trpc/runtime/iomodel/reactor/fiber/fiber_reactor.h"
#include "trpc/runtime/merge_runtime.h"
#include "trpc/runtime/runtime.h"
#include "trpc/runtime/separate_runtime.h"
#include "trpc/serialization/serialization_factory.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/stream/stream_handler_manager.h"
#include "trpc/telemetry/telemetry_factory.h"
#include "trpc/telemetry/trpc_telemetry.h"
#include "trpc/tracing/tracing_factory.h"
#include "trpc/tracing/trpc_tracing.h"
#include "trpc/transport/common/connection_handler_manager.h"
#include "trpc/transport/common/io_handler_manager.h"
#include "trpc/transport/common/ssl_helper.h"
#include "trpc/util/log/default/default_log.h"
#include "trpc/util/net_util.h"
#include "trpc/naming/common/util/loadbalance/trpc_load_balance.h"

namespace trpc {

// The plugins supported by the framework by default are registered here
int TrpcPlugin::RegisterPlugins() {
  std::scoped_lock<std::mutex> lock(mutex_);

  if (is_all_inited_) {
    return 0;
  }

  util::IgnorePipe();

  TRPC_ASSERT(compressor::Init());
  TRPC_ASSERT(serialization::Init());
  TRPC_ASSERT(codec::Init());

  TRPC_ASSERT(InitIoHandler());
  TRPC_ASSERT(InitConnectionHandler());
  TRPC_ASSERT(ssl::Init());
  TRPC_ASSERT(stream::InitStreamHandler());

  TRPC_ASSERT(config::Init());
  TRPC_ASSERT(metrics::Init());
  TRPC_ASSERT(tracing::Init());
  TRPC_ASSERT(telemetry::Init());
  TRPC_ASSERT(loadbalance::Init());
  TRPC_ASSERT(naming::Init());

  TRPC_ASSERT(overload_control::Init());

  CollectPlugins();
  InitPlugins();

  TRPC_ASSERT(filter::Init());

  TRPC_ASSERT(InitRuntime_());

  PeripheryTaskScheduler::GetInstance()->Init();
  PeripheryTaskScheduler::GetInstance()->Start();

  StartPlugins();

  FrameStats::GetInstance()->Start();

  runtime::StartReportRuntimeInfo();

#ifdef TRPC_BUILD_INCLUDE_RPCZ
  rpcz::RpczCollector::GetInstance()->Start();
#endif

  is_all_inited_ = true;

  return 0;
}

void TrpcPlugin::CollectPlugins() {
  const auto& config_codec_plugins = config::CodecFactory::GetInstance()->GetAllPlugins();
  AddPlugins<config::CodecPtr>(config_codec_plugins);

  const auto& config_provider_plugins = config::ProviderFactory::GetInstance()->GetAllPlugins();
  AddPlugins<config::ProviderPtr>(config_provider_plugins);

  // for compatible
  const auto& config_plugins = ConfigFactory::GetInstance()->GetAllPlugins();
  AddPlugins<ConfigPtr>(config_plugins);

  const auto& metrics_plugins = MetricsFactory::GetInstance()->GetAllPlugins();
  AddPlugins<MetricsPtr>(metrics_plugins);

  const auto& tracing_plugins = TracingFactory::GetInstance()->GetAllPlugins();
  AddPlugins<TracingPtr>(tracing_plugins);

  const auto& telemetry_plugins = TelemetryFactory::GetInstance()->GetAllPlugins();
  AddPlugins<TelemetryPtr>(telemetry_plugins);

  const auto& naming_registry_plugins = RegistryFactory::GetInstance()->GetAllPlugins();
  AddPlugins<RegistryPtr>(naming_registry_plugins);

  const auto& naming_selector_plugins = SelectorFactory::GetInstance()->GetAllPlugins();
  AddPlugins<SelectorPtr>(naming_selector_plugins);

  const auto& naming_loadbalance_plugins = LoadBalanceFactory::GetInstance()->GetAllPlugins();
  AddPlugins<LoadBalancePtr>(naming_loadbalance_plugins);

  const auto& naming_limiter_plugins = LimiterFactory::GetInstance()->GetAllPlugins();
  AddPlugins<LimiterPtr>(naming_limiter_plugins);
}

void TrpcPlugin::InitPlugins() {
  auto it = plugins_.begin();
  while (it != plugins_.end()) {
    PluginInfo& plugin_info = it->second;

    InitPlugin(plugin_info);

    ++it;
  }
}

void TrpcPlugin::InitPlugin(PluginInfo& plugin_info) {
  if (plugin_info.is_inited) {
    return;
  }

  // Init the dep plugins
  std::vector<std::string> dep_plugin_names;
  plugin_info.plugin->GetDependencies(dep_plugin_names);
  if (!dep_plugin_names.empty()) {
    for (const auto& name : dep_plugin_names) {
      auto it = FindPlugin(name);
      if (it == plugins_.end()) {
        TRPC_FMT_ERROR("plugin `{}` dependence plugin `{}` not found.", plugin_info.plugin->Name(), name);
        TRPC_ASSERT(false);
      }

      InitPlugin(it->second);
    }
  }

  // Init self
  if (plugin_info.plugin->Init() != 0) {
    TRPC_FMT_ERROR("plugin {} init failed.", plugin_info.plugin->Name());
    TRPC_ASSERT(false);
  }
  plugin_info.is_inited = true;
}

void TrpcPlugin::StartPlugins() {
  auto it = plugins_.begin();
  while (it != plugins_.end()) {
    PluginInfo& plugin_info = it->second;

    StartPlugin(plugin_info);

    ++it;
  }
}

void TrpcPlugin::StartPlugin(PluginInfo& plugin_info) {
  if (plugin_info.is_started) {
    return;
  }

  // Start the dep plugins
  std::vector<std::string> dep_plugin_names;
  plugin_info.plugin->GetDependencies(dep_plugin_names);
  if (!dep_plugin_names.empty()) {
    for (const auto& name : dep_plugin_names) {
      auto it = FindPlugin(name);
      TRPC_ASSERT(it != plugins_.end());

      StartPlugin(it->second);
    }
  }

  // Start self
  plugin_info.plugin->Start();
  plugin_info.is_started = true;
}

int TrpcPlugin::UnregisterPlugins() {
  std::scoped_lock<std::mutex> lock(mutex_);

  if (is_all_inited_ && is_all_destroyed_) {
    return 0;
  }

#ifdef TRPC_BUILD_INCLUDE_RPCZ
  rpcz::RpczCollector::GetInstance()->Stop();
#endif

  FrameStats::GetInstance()->Stop();

  runtime::StopReportRuntimeInfo();

  StopPlugins();

  overload_control::Stop();

  PeripheryTaskScheduler::GetInstance()->Stop();
  PeripheryTaskScheduler::GetInstance()->Join();

  GetTrpcClient()->Stop();

  DestroyRuntime_();

  is_all_destroyed_ = true;

  return 0;
}

void TrpcPlugin::StopPlugins() {
  auto it = plugins_.begin();
  while (it != plugins_.end()) {
    PluginInfo& plugin_info = it->second;

    StopPlugin(plugin_info);

    ++it;
  }
}

void TrpcPlugin::StopPlugin(PluginInfo& plugin_info) {
  if (plugin_info.is_stoped) {
    return;
  }

  // Stop the plugins that depend on this plugin
  std::vector<std::string>& reverse_dep_plugin_names = plugins_reverse_deps_[plugin_info.plugin->Name()];
  if (!reverse_dep_plugin_names.empty()) {
    for (const auto& name : reverse_dep_plugin_names) {
      auto it = FindPlugin(name);
      TRPC_ASSERT(it != plugins_.end());

      StopPlugin(it->second);
    }
  }

  // Stop self
  plugin_info.plugin->Stop();
  plugin_info.is_stoped = true;
}

void TrpcPlugin::DestroyPlugins() {
  auto it = plugins_.begin();
  while (it != plugins_.end()) {
    PluginInfo& plugin_info = it->second;

    DestroyPlugin(plugin_info);

    ++it;
  }
}

void TrpcPlugin::DestroyPlugin(PluginInfo& plugin_info) {
  if (plugin_info.is_destroyed) {
    return;
  }

  // Destroy the plugins that depend on this plugin
  std::vector<std::string>& reverse_dep_plugin_names = plugins_reverse_deps_[plugin_info.plugin->Name()];
  if (!reverse_dep_plugin_names.empty()) {
    for (const auto& name : reverse_dep_plugin_names) {
      auto it = FindPlugin(name);
      TRPC_ASSERT(it != plugins_.end());

      DestroyPlugin(it->second);
    }
  }

  // Destroy self
  plugin_info.plugin->Destroy();
  plugin_info.is_destroyed = true;
}

std::unordered_map<std::string, TrpcPlugin::PluginInfo>::iterator TrpcPlugin::FindPlugin(const std::string& name) {
  auto it = plugins_.find(name);
  if (it != plugins_.end()) {
    return it;
  }

  std::unordered_map<std::string, TrpcPlugin::PluginInfo>::iterator match_it = plugins_.end();
  bool already_match = false;
  auto new_name = name + "#";
  for (it = plugins_.begin(); it != plugins_.end(); ++it) {
    auto& plugin_name = it->first;
    if (!plugin_name.compare(0, new_name.size(), new_name)) {
      if (already_match) {
        TRPC_FMT_ERROR("Find multiple match {} plugins, you need specify the dependencies with the plugin_type", name);
        TRPC_ASSERT(false && "Find multiple match plugins");
      } else {
        already_match = true;
        match_it = it;
      }
    }
  }

  return match_it;
}

bool TrpcPlugin::IsDepPluginNameValid(const std::vector<std::string>& dep_plugin_names) {
  bool ret = true;
  for (auto& plugin_name : dep_plugin_names) {
    auto pos = plugin_name.find_first_of("#");
    if (pos == std::string::npos) {
      continue;
    }

    // If it contains '#', then the string at the end should be PluginType.
    auto str = plugin_name.substr(pos + 1);
    try {
      auto plugin_type = std::stoi(str);
      if (plugin_type > static_cast<int>(PluginType::kUnspecified)) {
        ret = false;
        TRPC_FMT_ERROR("dependent plugin name {} invalid", plugin_name);
      }
    } catch (const std::exception&) {
      ret = false;
      TRPC_FMT_ERROR("dependent plugin name {} invalid", plugin_name);
    }
  }

  return ret;
}

bool TrpcPlugin::RegisterServerCodec(const ServerCodecPtr& codec) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return ServerCodecFactory::GetInstance()->Register(codec);
}

bool TrpcPlugin::RegisterClientCodec(const ClientCodecPtr& codec) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return ClientCodecFactory::GetInstance()->Register(codec);
}

bool TrpcPlugin::RegisterCompress(const compressor::CompressorPtr& compressor) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return compressor::CompressorFactory::GetInstance()->Register(compressor);
}

bool TrpcPlugin::RegisterSerialization(const serialization::SerializationPtr& serialization) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return serialization::SerializationFactory::GetInstance()->Register(serialization);
}

bool TrpcPlugin::RegisterServerFilter(const MessageServerFilterPtr& filter) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return FilterManager::GetInstance()->AddMessageServerFilter(filter);
}

bool TrpcPlugin::RegisterClientFilter(const MessageClientFilterPtr& filter) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return FilterManager::GetInstance()->AddMessageClientFilter(filter);
}

bool TrpcPlugin::RegisterRegistry(const RegistryPtr& registry) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return RegistryFactory::GetInstance()->Register(registry);
}

bool TrpcPlugin::RegisterSelector(const SelectorPtr& selector) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return SelectorFactory::GetInstance()->Register(selector);
}

bool TrpcPlugin::RegisterLoadBalance(const LoadBalancePtr& load_balance) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return LoadBalanceFactory::GetInstance()->Register(load_balance);
}

bool TrpcPlugin::RegisterLimiter(const LimiterPtr& limiter) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return LimiterFactory::GetInstance()->Register(limiter);
}

bool TrpcPlugin::RegisterMetrics(const MetricsPtr& metrics) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return MetricsFactory::GetInstance()->Register(metrics);
}

bool TrpcPlugin::RegisterConfigCodec(const config::CodecPtr& codec) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return config::CodecFactory::GetInstance()->Register(codec);
}

bool TrpcPlugin::RegisterConfigProvider(const config::ProviderPtr& provider) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return config::ProviderFactory::GetInstance()->Register(provider);
}

bool TrpcPlugin::RegisterConfig(const ConfigPtr& config) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return ConfigFactory::GetInstance()->Register(config);
}

bool TrpcPlugin::RegisterLogging(const LoggingPtr& logging) {
  std::scoped_lock<std::mutex> lock(mutex_);

  auto log = LogFactory::GetInstance()->Get();
  return static_pointer_cast<DefaultLog>(log)->RegisterRawSink(logging);
}

bool TrpcPlugin::RegisterTracing(const TracingPtr& tracing) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return TracingFactory::GetInstance()->Register(tracing);
}

bool TrpcPlugin::RegisterTelemetry(const TelemetryPtr& telemetry) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return TelemetryFactory::GetInstance()->Register(telemetry);
}

bool TrpcPlugin::RegisterAuth(const AuthPtr& auth) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return AuthFactory::GetInstance()->Register(auth);
}

bool TrpcPlugin::RegisterAuthFollower(const AuthCenterFollowerPtr& follower) {
  std::scoped_lock<std::mutex> lock(mutex_);

  return AuthCenterFollowerFactory::GetInstance()->Register(follower);
}

void TrpcPlugin::SetInvokeByFramework() {
  std::scoped_lock<std::mutex> lock(mutex_);

  is_invoke_by_framework_ = true;
}

bool TrpcPlugin::InitRuntime_() {
  // if is_invoke_by_framework_ is true, framework will automatically start the running environment of the thread model
  // if false, use framework in client-tool, this will be entered here
  if (!is_invoke_by_framework_) {
    separate::StartAdminRuntime();
    merge::StartRuntime();
    separate::StartRuntime();

    if (runtime::IsInFiberRuntime()) {
      fiber::StartAllReactor();
    }
  }

  return true;
}

void TrpcPlugin::DestroyRuntime_() {
  if (!is_invoke_by_framework_) {
    if (runtime::IsInFiberRuntime()) {
      fiber::TerminateAllReactor();

      separate::TerminateRuntime();
      merge::TerminateRuntime();
    } else {
      separate::TerminateRuntime();
      merge::TerminateRuntime();

      separate::TerminateAdminRuntime();

      DestroyResource();
    }
  }
}

void TrpcPlugin::DestroyResource() {
  DestroyPlugins();

  ssl::Destroy();
  DestroyConnectionHandler();
  DestroyIoHandler();

  naming::Destroy();
  telemetry::Destroy();
  tracing::Destroy();
  metrics::Destroy();
  config::Destroy();

  codec::Destroy();
  serialization::Destroy();
  compressor::Destroy();

  log::Destroy();

  overload_control::Destroy();

  GetTrpcClient()->Destroy();

  is_all_inited_ = false;
  is_all_destroyed_ = false;
  is_invoke_by_framework_ = false;
  plugins_.clear();
  plugins_reverse_deps_.clear();
}

}  // namespace trpc
