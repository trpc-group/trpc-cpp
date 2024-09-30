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

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "trpc/client/service_proxy.h"
#include "trpc/client/service_proxy_option_setter.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/util/concurrency/lightly_concurrent_hashmap.h"
#include "trpc/util/likely.h"

namespace trpc {

/// @brief Management class of service proxy. It provided the ability to dynamically create proxy proxies based on
///        configuration or code.
class ServiceProxyManager {
 public:
  /// @brief Use the framework configuration method or the default value of the framework ServiceProxyOption
  /// to create and obtain the proxy, thread-safe.
  /// If there is already a ServiceProxy corresponding to name, return directly.
  /// If there is no ServiceProxy corresponding to name, create and return.
  /// When creating ServiceProxy:
  /// If there is a configuration corresponding to name under the framework configuration client/service,
  /// use the configuration under the framework configuration client/service to create.
  /// If there is no configuration corresponding to name under the framework configuration client/service,
  /// use the value of the default field inside the ServiceProxyOption class to create it
  /// @param name, needs to be unique
  /// @return when fails, the return is nullptr
  template <typename T>
  std::shared_ptr<T> GetProxy(const std::string& name);

  /// @brief Use code to set option to create and get proxy, framework configuration will be ignored, thread safe.
  /// If there is already a ServiceProxy corresponding to name, return directly.
  /// If there is no ServiceProxy corresponding to name, create and return.
  /// When creating ServiceProxy, only use the second parameter option to create ServiceProxy.
  /// @param name, needs to be unique
  /// @param option, the option of creating ServiceProxy
  /// @return when fails, the return is nullptr
  template <typename T>
  std::shared_ptr<T> GetProxy(const std::string& name, const ServiceProxyOption& option);

  /// @brief Use the framework configuration method or the default value of the framework ServiceProxyOption
  /// to create and obtain the proxy. However, some of the information is set through the second parameter func,
  /// this function is used in scenarios where some sensitive information cannot be placed 
  /// in the framework configuration, thread-safe.
  /// If there is already a ServiceProxy corresponding to name, return directly.
  /// If there is no ServiceProxy corresponding to name, create and return.
  /// When creating ServiceProxy:
  /// If there is a configuration corresponding to name under the framework configuration client/service,
  /// use the configuration under the framework configuration client/service and func to create.
  /// If there is no configuration corresponding to name under the framework configuration client/service,
  /// use the value of the default field inside the ServiceProxyOption class and func to create it
  /// @param name, needs to be unique
  /// @param func, the func that set some value of ServiceProxyOption
  /// @return when fails, the return is nullptr
  template <typename T>
  std::shared_ptr<T> GetProxy(const std::string& name, const std::function<void(ServiceProxyOption*)>& func);

  /// @brief Get service proxy by name(has deprecated), thread-safe.
  /// @param name name of service proxy
  /// @param conf client's configuration that contains configuration of proxy
  /// @param option_ptr specified option of service proxy
  /// @return service proxy
  /// @note Support incremental setting of proxy options, with priority:
  ///       interface settings > configuration file > default values within the framework.
  template <typename T>
  std::shared_ptr<T> GetProxy(const std::string& name, const ServiceProxyOption* option_ptr);

  /// @brief Stop used resources by all service proxys.
  void Stop();

  /// @brief Destroy used resources by all service proxys.
  void Destroy();

 private:
  void SetOptionFromConfig(const ServiceProxyConfig& proxy_conf, std::shared_ptr<ServiceProxyOption>& option);
  void SetOptionDefaultValue(const std::string& name, std::shared_ptr<ServiceProxyOption>& option);

 private:
  concurrency::LightlyConcurrentHashMap<std::string, std::shared_ptr<ServiceProxy>> service_proxys_;

  std::unordered_map<std::string, std::shared_ptr<ServiceProxy>> service_proxys_to_destroy_;

  std::atomic<bool> is_stoped_{false};
  std::atomic<bool> is_destroyed_{false};
};

template <typename T>
std::shared_ptr<T> ServiceProxyManager::GetProxy(const std::string& name) {
  if (TRPC_UNLIKELY(name.empty())) {
    TRPC_FMT_CRITICAL("GetProxy failed, name is empty.");
    return nullptr;
  }

  std::shared_ptr<ServiceProxy> proxy;
  bool ret = service_proxys_.Get(name, proxy);
  if (ret) {
    return std::dynamic_pointer_cast<T>(proxy);
  }

  std::shared_ptr<T> new_proxy = std::make_shared<T>();

  auto option = std::make_shared<ServiceProxyOption>();

  const ClientConfig& conf = TrpcConfig::GetInstance()->GetClientConfig();

  auto iter = std::find_if(conf.service_proxy_config.begin(), conf.service_proxy_config.end(),
                           [&](const ServiceProxyConfig& proxy_conf) { return proxy_conf.name == name; });

  if (iter != conf.service_proxy_config.end()) {
    SetOptionFromConfig(*iter, option);
  } else {
    // Initialize option with default configuration.
    ServiceProxyConfig proxy_config;
    SetOptionFromConfig(proxy_config, option);
  }

  SetOptionDefaultValue(name, option);

  new_proxy->SetServiceProxyOptionInner(option);

  new_proxy->InitOtherMembers();

  // Depend on new_proxy->SetServiceProxyOptionInner to be executed first, which may update selector_name.
  if (option->selector_name.compare("direct") == 0 || option->selector_name.compare("domain") == 0) {
    new_proxy->SetEndpointInfo(option->target);
  }

  ret = service_proxys_.GetOrInsert(name, std::static_pointer_cast<ServiceProxy>(new_proxy), proxy);
  if (!ret) {
    return new_proxy;
  }

  return std::dynamic_pointer_cast<T>(proxy);
}

template <typename T>
std::shared_ptr<T> ServiceProxyManager::GetProxy(const std::string& name, const ServiceProxyOption& option) {
  if (TRPC_UNLIKELY(name.empty())) {
    TRPC_FMT_CRITICAL("GetProxy failed, name is empty.");
    return nullptr;
  }

  std::shared_ptr<ServiceProxy> proxy;
  bool ret = service_proxys_.Get(name, proxy);
  if (ret) {
    return std::dynamic_pointer_cast<T>(proxy);
  }

  std::shared_ptr<T> new_proxy = std::make_shared<T>();

  auto option_ptr = std::make_shared<ServiceProxyOption>();

  *option_ptr = option;

  SetOptionDefaultValue(name, option_ptr);

  new_proxy->SetServiceProxyOptionInner(option_ptr);

  new_proxy->InitOtherMembers();

  // Depend on new_proxy->SetServiceProxyOptionInner to be executed first, which may update selector_name.
  if (option_ptr->selector_name.compare("direct") == 0 || option_ptr->selector_name.compare("domain") == 0) {
    new_proxy->SetEndpointInfo(option_ptr->target);
  }

  ret = service_proxys_.GetOrInsert(name, std::static_pointer_cast<ServiceProxy>(new_proxy), proxy);
  if (!ret) {
    return new_proxy;
  }

  return std::dynamic_pointer_cast<T>(proxy);
}

template <typename T>
std::shared_ptr<T> ServiceProxyManager::GetProxy(const std::string& name,
                                                 const std::function<void(ServiceProxyOption*)>& func) {
  if (TRPC_UNLIKELY(name.empty())) {
    TRPC_FMT_CRITICAL("GetProxy failed, name is empty.");
    return nullptr;
  }

  std::shared_ptr<ServiceProxy> proxy;
  bool ret = service_proxys_.Get(name, proxy);
  if (ret) {
    return std::dynamic_pointer_cast<T>(proxy);
  }

  std::shared_ptr<T> new_proxy = std::make_shared<T>();

  auto option = std::make_shared<ServiceProxyOption>();

  const ClientConfig& conf = TrpcConfig::GetInstance()->GetClientConfig();

  auto iter = std::find_if(conf.service_proxy_config.begin(), conf.service_proxy_config.end(),
                           [&](const ServiceProxyConfig& proxy_conf) { return proxy_conf.name == name; });

  if (iter != conf.service_proxy_config.end()) {
    SetOptionFromConfig(*iter, option);
  } else {
    // Initialize option with default configuration.
    ServiceProxyConfig proxy_config;
    SetOptionFromConfig(proxy_config, option);
  }

  SetOptionDefaultValue(name, option);

  func(option.get());

  new_proxy->SetServiceProxyOptionInner(option);

  new_proxy->InitOtherMembers();

  // Depend on new_proxy->SetServiceProxyOptionInner to be executed first, which may update selector_name.
  if (option->selector_name.compare("direct") == 0 || option->selector_name.compare("domain") == 0) {
    new_proxy->SetEndpointInfo(option->target);
  }

  ret = service_proxys_.GetOrInsert(name, std::static_pointer_cast<ServiceProxy>(new_proxy), proxy);
  if (!ret) {
    return new_proxy;
  }

  return std::dynamic_pointer_cast<T>(proxy);
}

template <typename T>
std::shared_ptr<T> ServiceProxyManager::GetProxy(const std::string& name, const ServiceProxyOption* option_ptr) {
  if (TRPC_UNLIKELY(name.empty())) {
    TRPC_FMT_CRITICAL("GetProxy failed, name is empty.");
    return nullptr;
  }

  std::shared_ptr<ServiceProxy> proxy;
  bool ret = service_proxys_.Get(name, proxy);
  if (ret) {
    return std::dynamic_pointer_cast<T>(proxy);
  }

  std::shared_ptr<T> new_proxy = std::make_shared<T>();

  auto option = std::make_shared<ServiceProxyOption>();

  const ClientConfig& conf = TrpcConfig::GetInstance()->GetClientConfig();

  auto iter = std::find_if(conf.service_proxy_config.begin(), conf.service_proxy_config.end(),
                           [&](const ServiceProxyConfig& proxy_conf) { return proxy_conf.name == name; });

  if (option_ptr) {
    // priority: interface settings > configuration file > default values
    // set option values to default
    detail::SetDefaultOption(option);

    // set option values from configuration file
    if (iter != conf.service_proxy_config.end()) {
      SetOptionFromConfig(*iter, option);
    }

    // Set the specified non-default values in option_ptr.
    detail::SetSpecifiedOption(option_ptr, option);
  } else {
    if (iter != conf.service_proxy_config.end()) {
      SetOptionFromConfig(*iter, option);
    } else {
      // Initialize option with default configuration.
      ServiceProxyConfig proxy_config;
      SetOptionFromConfig(proxy_config, option);
    }
  }

  // The name parameter of option is consistent with the name parameter of GetProxy.
  SetOptionDefaultValue(name, option);

  new_proxy->SetServiceProxyOptionInner(option);

  new_proxy->InitOtherMembers();

  // Depend on new_proxy->SetServiceProxyOptionInner to be executed first, which may update selector_name.
  if (option->selector_name.compare("direct") == 0 || option->selector_name.compare("domain") == 0) {
    new_proxy->SetEndpointInfo(option->target);
  }

  TRPC_FMT_TRACE("ServiceProxy name:{}, target:{}, threadmodel_instance_name:{}", name, new_proxy->option_->target,
                 new_proxy->option_->threadmodel_instance_name);

  ret = service_proxys_.GetOrInsert(name, std::static_pointer_cast<ServiceProxy>(new_proxy), proxy);
  if (!ret) {
    return new_proxy;
  }

  return std::dynamic_pointer_cast<T>(proxy);
}

}  // namespace trpc
