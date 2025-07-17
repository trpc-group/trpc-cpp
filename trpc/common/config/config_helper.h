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

#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "trpc/common/config/yaml_parser.h"

namespace trpc {

/// @brief The implementation of parsing trpc framework configuration file.
class ConfigHelper {
 public:
  static ConfigHelper* GetInstance() {
    static ConfigHelper instance;
    return &instance;
  }

  /// @brief Init and parse config.
  /// @param config_path absolute path of yaml file
  bool Init(const std::string& config_path);

  /// @brief Reset config patch.
  void ResetConfigPath(const std::string& config_path) { conf_path_ = config_path; }

  /// @brief Reload config.
  bool Reload();

  /// @brief Register config update notifiers.
  /// @param notify_name is only to avoid repeatation
  /// @param notify_cb is the callback
  void RegisterConfigUpdateNotifier(const std::string& notify_name,
                                    const std::function<void(const YAML::Node&)>& notify_cb);

  /// @brief Check if node exists.
  /// @param areas a list of path in yaml
  /// @return true node exists, false node does not exist
  bool IsNodeExist(const std::initializer_list<std::string>& areas) const;

  /// @brief Get the Node object by path.
  /// @param areas a list of path in yaml
  /// @param node eturn the corresponding result
  /// @return true success, false failed
  bool GetNode(const std::initializer_list<std::string>& areas, YAML::Node& node) const;

  /// @brief Get the Child Node objects by path.
  /// @param areas a list of config path in yaml
  /// @param nodes the child nodes' key
  /// @return true success, false failed
  bool GetNodes(const std::initializer_list<std::string>& areas,
                std::vector<std::string>& nodes) const;

  /// @brief Parse yaml node to config by path
  /// @tparam T Config Type
  /// @param areas a list of config path in yaml
  /// @param config result
  /// @return true success, false failed
  template <typename T>
  bool GetConfig(const std::initializer_list<std::string>& areas, T& config) const {
    return GetConfig(yaml_parser_.GetYAML(), areas, config);
  }

  /// @brief Get the Node object by path from parent
  /// @param parent start node
  /// @param areas a list of path in yaml from parent
  /// @param node result
  /// @return true success, false failed
  static bool GetNode(const YAML::Node& parent, const std::initializer_list<std::string>& areas,
                      YAML::Node& node);

  /// @brief Get the Child Node objects by path from parent
  /// @param parent start node
  /// @param areas a list of config path in yaml
  /// @param nodes the child nodes' key
  /// @return true success, false failed
  static bool GetNodes(const YAML::Node& parent, const std::initializer_list<std::string>& areas,
                       std::vector<std::string>& nodes);

  /// @brief Filter Child Node objects in list from parent.
  /// @param parent start node
  /// @param nodes results
  /// @param filter a callable object for filtering
  /// @return true success, false failed
  static bool FilterNode(const YAML::Node& parent, std::vector<YAML::Node>& nodes,
                         std::function<bool(size_t, const YAML::Node&)> filter);

  /// @brief Parse yaml node to config by path from parent
  /// @tparam T Config Type
  /// @param parent start node
  /// @param areas a list of config path in yaml from parent
  /// @param config result
  /// @return true success, false failed
  template <typename T>
  static bool GetConfig(const YAML::Node& parent, const std::initializer_list<std::string>& areas,
                        T& config) {
    try {
      YAML::Node node;
      if (!GetNode(parent, areas, node)) return false;

      config = std::move(node.as<T>());
      return true;
    } catch (std::exception& ex) {
      std::cerr << "get config error: " << ex.what() << std::endl;
      return false;
    }
  }

  /// @brief Loading the configuration file content
  /// @param config_path the absolute path of yaml file
  /// @return return the file content
  static std::string LoadFromPath(const std::string& config_path);

  /// @brief Use environment variable replace placeholder
  /// @param str input
  /// @return the result after replace
  static std::string ExpandEnv(const std::string &str);

 private:
  ConfigHelper(const ConfigHelper&) = delete;
  ConfigHelper() = default;
  ConfigHelper& operator=(const ConfigHelper&) = delete;

  YamlParser yaml_parser_;

  // after initiation, store the path for reloading
  std::string conf_path_;

  // store the config update callbacks
  std::map<std::string, std::function<void(const YAML::Node&)>> update_notifiers_;
};

}  // namespace trpc
