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

#include "trpc/tracing/tracing.h"

namespace trpc {

/// @brief Plugin factory class for tracing
/// @note `Register` interface is not thread-safe and needs to be called when the program starts
class TracingFactory {
 public:
  /// @brief Get the instance of TracingFactory.
  /// @return Return the pointer of TracingFactory
  static TracingFactory* GetInstance() {
    static TracingFactory instance;
    return &instance;
  }

  /// @brief Register a tracing plugin instance to the factory
  /// @param tracing the pointer of tracing plugin instance
  /// @return true: success, false: failed
  bool Register(const TracingPtr& tracing);

  /// @brief Get the tracing plugin instance according to the plugin name
  /// @param name plugin name
  /// @return Returns the pointer of tracing plugin instance on success, nullptr otherwise
  TracingPtr Get(const std::string& name);

  /// @brief Get all tracing plugins
  const std::unordered_map<std::string, TracingPtr>& GetAllPlugins();

  /// @brief Clear tracing plugins
  void Clear();

 private:
  TracingFactory() = default;
  TracingFactory(const TracingFactory&) = delete;
  TracingFactory& operator=(const TracingFactory&) = delete;

 private:
  std::unordered_map<std::string, TracingPtr> tracing_plugins_;
};

}  // namespace trpc
