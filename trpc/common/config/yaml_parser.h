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

#include "yaml-cpp/yaml.h"

namespace trpc {

/// @brief Parse yaml format file
class YamlParser {
 public:
  YamlParser() = default;

  /// @brief construct this class by file path
  /// @param config_path absolute path of yaml file
  explicit YamlParser(const std::string& config_path) { LoadFromPath(config_path); }

  ~YamlParser() = default;

  /// @brief parse the yaml format content
  void Load(const std::string& content) { root_ = YAML::Load(content); }

  /// @brief loading and parse a configuration file of format file
  /// @param config_path absolute path of yaml file
  void LoadFromPath(const std::string& config_path) { root_ = YAML::LoadFile(config_path); }

  /// @brief get root node
  const YAML::Node& GetYAML() const { return root_; }
  YAML::Node& GetYAML() { return root_; }

 private:
  YamlParser(const YamlParser&) = delete;
  YamlParser& operator=(const YamlParser&) = delete;

  YAML::Node root_;
};

}  // namespace trpc
