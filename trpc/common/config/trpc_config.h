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
#include <string>
#include <utility>
#include <vector>

#include "trpc/common/config/client_conf.h"
#include "trpc/common/config/config_helper.h"
#include "trpc/common/config/global_conf.h"
#include "trpc/common/config/server_conf.h"
#include "trpc/util/log/logging.h"

namespace trpc {

/// @brief For trpc framework config
class TrpcConfig {
 public:
  /// @brief Get the object pointer of the configuration singleton.
  static TrpcConfig* GetInstance() {
    static TrpcConfig instance;
    return &instance;
  }

  /// @brief Init and parse config.
  /// @param conf_path absolute path of yaml file.
  /// @return 0: success, other: failure.
  int Init(const std::string& conf_path);

  /// @brief Get global config of framework
  const GlobalConfig& GetGlobalConfig() const { return global_config_; }
  GlobalConfig& GetMutableGlobalConfig() { return global_config_; }

  /// @brief Get server config of framework
  const ServerConfig& GetServerConfig() const { return server_config_; }
  ServerConfig& GetMutableServerConfig() { return server_config_; }

  /// @brief Get client config of framework
  const ClientConfig& GetClientConfig() const { return client_config_; }
  ClientConfig& GetMutableClientConfig() { return client_config_; }

  /// @brief Get tvar config of framework
  const TvarConfig& GetTvarConfig() const { return global_config_.tvar_config; }
  TvarConfig& GetMutableTVarConfig() { return global_config_.tvar_config; }

  /// @brief Get rpc config of framework
  const RpczConfig& GetRpczConfig() const { return global_config_.rpcz_config; }
  RpczConfig& GetMutableRpczConfig() { return global_config_.rpcz_config; }

  /// @brief Get plugin config by plugin-type and plugin-name.
  /// @param plugin_type_name plugin-type(eg: registry/metrics/config/tracing)
  /// @param plugin_name plugin-name(eg: polaris)
  /// @param plugin_config plugin config
  /// @return True: success, False: failure
  template <typename T>
  bool GetPluginConfig(const std::string& plugin_type_name, const std::string& plugin_name, T& plugin_config) {
    return ConfigHelper::GetInstance()->GetConfig({"plugins", plugin_type_name, plugin_name}, plugin_config);
  }

  /// @brief Get the node list by plugin type
  /// @param plugin_type_name plugin type(eg: registry/metrics/config/tracing)
  /// @param nodes node list
  /// @return True: success, False: failure
  bool GetPluginNodes(const std::string& plugin_type_name, std::vector<std::string>& nodes) {
    return ConfigHelper::GetInstance()->GetNodes({"plugins", plugin_type_name}, nodes);
  }

  /// @brief Get yaml-node by name in global-config
  /// @param name node name
  /// @param node yaml node
  /// @return True: success, False: failure
  bool GetGlobalNode(const std::string& name, YAML::Node& node);

  /// @brief Get yaml-node by service name in server-config
  /// @param service_name sevice name
  /// @param node yaml node
  /// @return True: success, False: failure
  bool GetServerServiceNode(const std::string& service_name, YAML::Node& node);

  /// @brief Get service config by service name in server-config
  /// @param service_name service name
  /// @param config service config
  /// @return True: success, False: failure
  bool GetServerServiceConfig(const std::string& service_name, ServiceConfig& config);

  /// @brief Get yaml node by service name and section name in server-config
  /// @param service_name service name
  /// @param section section name
  /// @param node yaml node
  /// @return True: success, False: failure
  bool GetServerServiceSectionNode(const std::string& service_name, const std::string& section, YAML::Node& node);

  /// @brief Get config by service name and section name in server-config
  /// @param service_name service name
  /// @param section section name
  /// @param config config
  /// @return True: success, False: failure
  template <typename T>
  bool GetServerServiceSectionConfig(const std::string& service_name, const std::string& section, T& config) {
    try {
      YAML::Node node;
      if (!GetServerServiceSectionNode(service_name, section, node)) {
        return false;
      }
      config = std::move(node.as<T>());
      return true;
    } catch (std::exception& ex) {
      return false;
    }
  }

  /// @brief Get yaml-node by service name in client-config
  /// @param service_name sevice name
  /// @param node yaml node
  /// @return True: success, False: failure
  bool GetClientServiceNode(const std::string& service_name, YAML::Node& node);

  /// @brief Get service proxy config by service name in client-config
  /// @param service_name service name
  /// @param config service proxy config
  /// @return True: success, False: failure
  bool GetClientServiceConfig(const std::string& service_name, ServiceProxyConfig& config);

  /// @brief Get yaml node by service name and section name in client-config
  /// @param service_name service name
  /// @param section section name
  /// @param node yaml node
  /// @return True: success, False: failure
  bool GetClientServiceSectionNode(const std::string& service_name, const std::string& section, YAML::Node& node);

  /// @brief Get config by service name and section name in client-config
  /// @param service_name service name
  /// @param section section name
  /// @param config config
  /// @return True: success, False: failure
  template <typename T>
  bool GetClientServiceSectionConfig(const std::string& service_name, const std::string& section, T& config) {
    try {
      YAML::Node node;
      if (!GetClientServiceSectionNode(service_name, section, node)) {
        return false;
      }
      config = std::move(node.as<T>());
      return true;
    } catch (std::exception& ex) {
      return false;
    }
  }

 private:
  TrpcConfig() = default;
  TrpcConfig(const TrpcConfig&) = delete;
  TrpcConfig& operator=(const TrpcConfig&) = delete;

  bool GuaranteeValidConfig();
  void Clear();

 private:
  GlobalConfig global_config_;

  ServerConfig server_config_;

  ClientConfig client_config_;
};

}  // namespace trpc
