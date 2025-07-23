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

#include "trpc/common/config/trpc_config.h"

#include <algorithm>

#include "trpc/common/config/client_conf_parser.h"
#include "trpc/common/config/global_conf_parser.h"
#include "trpc/common/config/server_conf_parser.h"
#include "trpc/util/chrono/chrono.h"

namespace trpc {

namespace {

bool CheckUniqueThreadModelName(const std::vector<FiberThreadModelInstanceConfig>& fiber_models,
                                const std::vector<DefaultThreadModelInstanceConfig>& default_models) {
  std::vector<std::string> threadmodels;
  threadmodels.reserve(fiber_models.size() + default_models.size());
  for (auto& model : fiber_models) {
    threadmodels.push_back(model.instance_name);
  }
  for (auto& model : default_models) {
    threadmodels.push_back(model.instance_name);
  }

  if (threadmodels.size() <= 1) {
    return true;
  }

  std::sort(threadmodels.begin(), threadmodels.end());
  for (std::size_t i = 0; i < threadmodels.size() - 1; i++) {
    if (threadmodels[i] == threadmodels[i + 1]) {
      return false;
    }
  }

  return true;
}

void CheckServicesConfigThreadModel(const std::vector<ServiceConfig>& services_config,
                                    const std::vector<ServiceProxyConfig>& service_proxys_config) {
  // The server needs to configure at least one thread model in the global configuration.
  if (!services_config.empty()) {
    for (auto& config : services_config) {
      if (config.threadmodel_instance_name.empty()) {
        TRPC_FMT_ERROR("Detect [server->service:{}] exist, but [global->threadmodel] not config.", config.service_name);
        TRPC_LOG_WARN("May casue low performance due to use inner created threadmodel(1-io_thread,1-handle_thread)");
      }
    }
  }
  // If the global configuration is not set, the client will default to using the admin thread model and provide a
  // prompt.
  if (!service_proxys_config.empty()) {
    for (auto& config : service_proxys_config) {
      if (config.threadmodel_instance_name.empty()) {
        TRPC_FMT_WARN("Detect [client->service:{}] exist, but [global->threadmodel] not config.", config.name);
        TRPC_LOG_WARN("May casue low performance due to use inner created threadmodel(1-io_thread,1-handle_thread)");
      }
    }
  }
}

bool CheckThreadNumConfig(const std::vector<FiberThreadModelInstanceConfig>& fiber_models,
                          const std::vector<DefaultThreadModelInstanceConfig>& default_models) {
  for (auto& model : default_models) {
    std::string location = "[global->threadmodel->default->" + model.instance_name + "]";
    if (model.io_handle_type == "separate") {
      if (model.io_thread_num == 0) {
        TRPC_LOG_ERROR("Error separate threadmodel io_handle_num config 0 for " << location);
        return false;
      }
      if (model.handle_thread_num == 0) {
        TRPC_LOG_ERROR("Error separate threadmodel handle_thread_num config 0 for " << location);
        return false;
      }
    } else if (model.io_handle_type == "merge") {
      if (model.io_thread_num == 0) {
        TRPC_LOG_WARN("Error merge threadmodel io_handle_num config 0 for " << location);
        return false;
      }
      if (model.handle_thread_num != 0) {
        TRPC_LOG_WARN("Discard config useless [handle_thread_num: " << model.handle_thread_num << "]"
                                                                    << " for merge threadmodel " << location
                                                                    << ", consider delete this config");
      }
    } else {
      TRPC_LOG_ERROR("Error config at [global->threadmodel->default->io_handle_type]: "
                     << model.io_handle_type << ". Only [separate] or [merge] are allowed.");
      return false;
    }
  }
  // Fiber threadmodel contains default value, no need to check.
  return true;
}

bool CheckDuplicatedConfig(const std::vector<ServiceConfig>& services_config,
                           const std::vector<ServiceProxyConfig>& service_proxys_config,
                           const std::vector<FiberThreadModelInstanceConfig>& fiber_models,
                           const std::vector<DefaultThreadModelInstanceConfig>& default_models) {
  std::set<std::string> duplicated_conf;
  // If fiber threadmodel has duplicated instance name, warn user
  for (auto& model : fiber_models) {
    if (duplicated_conf.find(model.instance_name) != duplicated_conf.end()) {
      TRPC_LOG_WARN("Detect duplicated instance name for [global->threadmodel->fiber]: " << model.instance_name);
      return false;
    } else {
      duplicated_conf.insert(model.instance_name);
    }
  }
  duplicated_conf.clear();
  // If default threadmodel has duplicated instance name, warn user.
  // Attention, when use `seperate` and `merge` as threadmodel type, they will be viewd as `default` threadmodel type at
  // checking.
  for (auto& model : default_models) {
    if (duplicated_conf.find(model.instance_name) != duplicated_conf.end()) {
      TRPC_LOG_WARN("Detect duplicated instance name for [global->threadmodel->default]: " << model.instance_name);
      return false;
    } else {
      duplicated_conf.insert(model.instance_name);
    }
  }
  duplicated_conf.clear();
  // Checking duplicated server->service->name
  for (auto& service_config : services_config) {
    if (duplicated_conf.find(service_config.service_name) != duplicated_conf.end()) {
      TRPC_LOG_WARN("Detect duplicated service name for [server->service]: " << service_config.service_name);
      return false;
    } else {
      duplicated_conf.insert(service_config.service_name);
    }
  }
  duplicated_conf.clear();
  // Checking duplidated client->service->name
  for (auto& service_proxy_config : service_proxys_config) {
    if (duplicated_conf.find(service_proxy_config.name) != duplicated_conf.end()) {
      TRPC_LOG_WARN("Detect duplicated service name for [client->service]: " << service_proxy_config.name);
      return false;
    } else {
      duplicated_conf.insert(service_proxy_config.name);
    }
  }
  duplicated_conf.clear();
  return true;
}

}  // namespace

bool TrpcConfig::GuaranteeValidConfig() {
  auto& default_models = GetGlobalConfig().threadmodel_config.default_model;
  auto& fiber_models = GetGlobalConfig().threadmodel_config.fiber_model;
  auto& services_config = GetServerConfig().services_config;
  auto& service_proxys_config = GetClientConfig().service_proxy_config;
  // The following yaml checks correspond to the scenarios covered in trpc/common/config/invalid_test_config folder.
  // More checkings need to be added into both places(here and `invalid_test_config` folder).
  // Threadmodel checking 00: Thread model instance names configured under 'global' are not duplicated.
  if (!CheckUniqueThreadModelName(fiber_models, default_models)) {
    TRPC_LOG_ERROR("Threadmodel instance_name need to be unique");
    return false;
  }

  // Threadmodel checking 01: when global, server->services, client->services all of them haven't config threadmodel
  // type
  if (default_models.size() == 0 && fiber_models.size() == 0) {
    CheckServicesConfigThreadModel(services_config, service_proxys_config);
  }

  // Threadmodel checking 02: server/client configured a threadmodel but can't found at global
  // If use fiber threadmodel, only one instance can be configured currently.
  if (!fiber_models.empty() && fiber_models.size() != 1) {
    TRPC_LOG_ERROR(
        "Detect mutiple [global->threadmodel->fiber] configured. Only one fiber threadmodel is allowed currently");
    return false;
  }

  // Threadmodel checking 03: num of thread checking
  if (!CheckThreadNumConfig(fiber_models, default_models)) {
    return false;
  }

  // Checking duplicate configurations(threadmodel instance/server->service/client->service)
  if (!CheckDuplicatedConfig(services_config, service_proxys_config, fiber_models, default_models)) {
    return false;
  }
  return true;
}

void TrpcConfig::Clear() {
  global_config_ = GlobalConfig();
  server_config_ = ServerConfig();
  client_config_ = ClientConfig();
}

int TrpcConfig::Init(const std::string& conf_path) {
  int ret = 0;
  try {
    TRPC_LOG_DEBUG("Parse Config:" << conf_path);

    if (!ConfigHelper::GetInstance()->Init(conf_path)) {
      TRPC_LOG_ERROR("Init Config error");
      return -1;
    }

    Clear();

    std::string area = "global";
    if (ConfigHelper::GetInstance()->IsNodeExist({area})) {
      if (ConfigHelper::GetInstance()->GetConfig({area}, global_config_)) {
        global_config_.Display();
      } else {
        TRPC_LOG_ERROR("Parse Area error:" << area);
        return -1;
      }
    }

    area = "server";
    if (ConfigHelper::GetInstance()->IsNodeExist({area})) {
      if (ConfigHelper::GetInstance()->GetConfig({area}, server_config_)) {
        server_config_.Display();
      } else {
        std::cout << "server get failed." << std::endl;
        TRPC_LOG_ERROR("Parse Area error:" << area);
        return -1;
      }
    }

    area = "client";
    if (ConfigHelper::GetInstance()->IsNodeExist({area})) {
      if (ConfigHelper::GetInstance()->GetConfig({area}, client_config_)) {
        client_config_.Display();
      } else {
        TRPC_LOG_ERROR("Parse Area error:" << area);
        return -1;
      }
    }
  } catch (std::exception& ex) {
    TRPC_LOG_ERROR("Parse Config error:" << ex.what());
    ret = -1;
  }

  if (!GuaranteeValidConfig()) {
    ret = -1;
  }

  return ret;
}

bool TrpcConfig::GetServerServiceNode(const std::string& service_name, YAML::Node& node) {
  YAML::Node services;
  if (!ConfigHelper::GetInstance()->GetConfig({"server", "service"}, services)) {
    return false;
  }
  for (const auto& service : services) {
    try {
      if (service["name"] && service["name"].as<std::string>() == service_name) {
        node.reset(service);
        return true;
      }
    } catch (...) {
      continue;
    }
  }
  return false;
}

bool TrpcConfig::GetGlobalNode(const std::string& name, YAML::Node& node) {
  YAML::Node global_conf;
  bool ret = true;
  if (name.empty()) {
    ret = ConfigHelper::GetInstance()->GetConfig({"global"}, global_conf);
  } else {
    ret = ConfigHelper::GetInstance()->GetConfig({"global", name}, global_conf);
  }
  if (!ret) {
    return false;
  }

  try {
    node.reset(global_conf);
    return true;
  } catch (...) {
    return false;
  }
}

bool TrpcConfig::GetServerServiceConfig(const std::string& service_name, ServiceConfig& config) {
  for (const auto& service : server_config_.services_config) {
    if (service.service_name == service_name) {
      config = service;
      return true;
    }
  }
  return false;
}

bool TrpcConfig::GetServerServiceSectionNode(const std::string& service_name, const std::string& section,
                                             YAML::Node& node) {
  YAML::Node service;
  if (!GetServerServiceNode(service_name, node)) {
    return false;
  }
  if (!node[section]) {
    return false;
  }
  node.reset(node[section]);
  return true;
}

bool TrpcConfig::GetClientServiceNode(const std::string& service_name, YAML::Node& node) {
  YAML::Node services;
  if (!ConfigHelper::GetInstance()->GetConfig({"client", "service"}, services)) {
    return false;
  }
  for (const auto& service : services) {
    try {
      if (service["name"] && service["name"].as<std::string>() == service_name) {
        node.reset(service);
        return true;
      }
    } catch (...) {
      continue;
    }
  }
  return false;
}

bool TrpcConfig::GetClientServiceConfig(const std::string& service_name, ServiceProxyConfig& config) {
  for (const auto& service : client_config_.service_proxy_config) {
    if (service.name == service_name) {
      config = service;
      return true;
    }
  }
  return false;
}

bool TrpcConfig::GetClientServiceSectionNode(const std::string& service_name, const std::string& section,
                                             YAML::Node& node) {
  YAML::Node service;
  if (!GetClientServiceNode(service_name, node)) {
    return false;
  }
  if (!node[section]) {
    return false;
  }
  node.reset(node[section]);
  return true;
}

}  // namespace trpc
