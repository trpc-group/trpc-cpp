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

#include <string>
#include <unordered_map>

#include "trpc/naming/selector.h"

namespace trpc {

/// @brief Service discovery plugin factory
class SelectorFactory {
 public:
  /// @brief Get the singleton instance of the SelectorFactory
  /// @return SelectorFactory* The singleton instance of the SelectorFactory
  static SelectorFactory* GetInstance() {
    static SelectorFactory instance;
    return &instance;
  }

  SelectorFactory(const SelectorFactory&) = delete;
  SelectorFactory& operator=(const SelectorFactory&) = delete;

  /// @brief Register a service discovery plugin
  ///
  /// @param selector The service discovery plugin to register
  bool Register(const SelectorPtr& selector);

  /// @brief Get a service discovery plugin by name
  /// @param name The name of the service discovery plugin to retrieve
  /// @return SelectorPtr The service discovery plugin with the given name
  SelectorPtr Get(const std::string& name);

  /// @brief Get all selector plugins
  const std::unordered_map<std::string, SelectorPtr>& GetAllPlugins();

  /// @brief Clear all registered service discovery plugins
  void Clear();

 private:
  SelectorFactory() = default;

 private:
  std::unordered_map<std::string, SelectorPtr> selectors_;
};

}  // namespace trpc
