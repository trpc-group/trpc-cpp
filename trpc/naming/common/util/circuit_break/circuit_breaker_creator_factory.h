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

#include <any>
#include <functional>
#include <unordered_map>

#include "yaml-cpp/yaml.h"

#include "trpc/naming/common/util/circuit_break/circuit_breaker.h"

namespace trpc::naming {

class CircuitBreakerCreatorFactory {
 public:
  using CreateFunction =
      std::function<CircuitBreakerPtr(const YAML::Node* plugin_config, const std::string& service_name)>;

  static CircuitBreakerCreatorFactory* GetInstance() {
    static CircuitBreakerCreatorFactory factory;
    return &factory;
  }

  void Register(const std::string& name, CreateFunction&& creator) {
    creators_[name] = std::move(creator);
  }

  CircuitBreakerPtr Create(const std::string& name, const YAML::Node* plugin_config, const std::string& service_name) {
    auto iter = creators_.find(name);
    if (iter != creators_.end()) {
      return iter->second(plugin_config, service_name);
    }

    return nullptr;
  }

 private:
  CircuitBreakerCreatorFactory() = default;
  CircuitBreakerCreatorFactory(const CircuitBreakerCreatorFactory&) = delete;
  CircuitBreakerCreatorFactory& operator=(const CircuitBreakerCreatorFactory&) = delete;

 private:
  std::unordered_map<std::string, CreateFunction> creators_;
};

}  // namespace trpc::naming
