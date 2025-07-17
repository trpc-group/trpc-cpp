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
#include <string>

#include "trpc/client/service_proxy_manager.h"
#include "trpc/common/config/trpc_config.h"

namespace trpc {

/// @brief Implementation of trpc client.
/// @note  Use GetTrpcClient() to obtain the TrpcClient object uniformly.
class TrpcClient {
 public:
  TrpcClient() = default;

  /// @brief Use the framework configuration method or the default value of the framework ServiceProxyOption
  /// to create and obtain the proxy, thread-safe.
  /// If there is already a ServiceProxy corresponding to name, return directly.
  /// If there is no ServiceProxy corresponding to name, create and return.
  /// When creating ServiceProxy:
  /// If there is a configuration corresponding to name under the framework configuration client/service,
  /// use the configuration under the framework configuration client/service to create.
  /// If there is no configuration corresponding to name under the framework configuration client/service,
  /// use the value of the default field inside the ServiceProxyOption class to create it
  /// @param name Name of service proxy that need to be unique
  /// @return when fails, the return is nullptr
  template <typename T>
  std::shared_ptr<T> GetProxy(const std::string& name) {
    return service_proxy_manager_.GetProxy<T>(name);
  }

  /// @brief Use code to set option to create and get proxy, framework configuration will be ignored, thread safe.
  /// If there is already a ServiceProxy corresponding to name, return directly.
  /// If there is no ServiceProxy corresponding to name, create and return.
  /// When creating ServiceProxy, only use the second parameter option to create ServiceProxy.
  /// @param name Name of service proxy that need to be unique
  /// @param option The option of creating ServiceProxy
  /// @return when fails, the return is nullptr
  template <typename T>
  std::shared_ptr<T> GetProxy(const std::string& name, const ServiceProxyOption& option)  {
    return service_proxy_manager_.GetProxy<T>(name, option);
  }

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
  /// @param name Name of service proxy that need to be unique
  /// @param func The func that set some value of ServiceProxyOption
  /// @return when fails, the return is nullptr
  template <typename T>
  std::shared_ptr<T> GetProxy(const std::string& name, const std::function<void(ServiceProxyOption*)>& func)  {
    return service_proxy_manager_.GetProxy<T>(name, func);
  }

  /// @brief Get a service proxy object based on the name(has deprecated).
  ///        If the option is not empty, the value set in the option is used first. It's Priority is higher than the
  ///        configuration file. The properties associated with the service proxy are read-only in design and are
  ///        determined when calling GetProxy. If dynamic changes to the properties are required, use the interface in
  ///        ClientContext to set.
  /// @param name The name identifier of the service proxy.
  /// @param option The specified option.
  /// @note Using the same name will obtain the same ServiceProxy. After the ServiceProxy is obtained for the first
  ///       time, its option is determined and cannot be changed later.
  /// @private
  template <typename T>
  std::shared_ptr<T> GetProxy(const std::string& name, const ServiceProxyOption* option) {
    return service_proxy_manager_.GetProxy<T>(name, option);
  }

  /// @brief Stopping the client, it will call the Stop method of service proxy, such as stopping the timer, closing the
  ///        connection, etc.
  void Stop();

  /// @brief Destroy the resources using by the client.
  void Destroy();

 private:
  TrpcClient(const TrpcClient&) = delete;
  TrpcClient& operator=(const TrpcClient&) = delete;

 private:
  ServiceProxyManager service_proxy_manager_;
};

/// @brief Get the global TrpcClient.
/// @note The global client's 'Stop' and 'Destroy' method will be called by the framework internally. So you don't need
///       to call it actively.
std::shared_ptr<TrpcClient> GetTrpcClient();

}  // namespace trpc
