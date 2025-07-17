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

#include "trpc/runtime/threadmodel/separate/separate_scheduling.h"

#include <unordered_map>

namespace trpc {

static std::unordered_map<std::string, SeparateSchedulingCreateFunction> scheduling_create_funtion_map;

bool RegisterSeparateSchedulingImpl(const std::string& name, SeparateSchedulingCreateFunction&& func) {
  scheduling_create_funtion_map[name] = std::move(func);

  return true;
}

std::unique_ptr<SeparateScheduling> CreateSeparateScheduling(const std::string& name) {
  auto it = scheduling_create_funtion_map.find(name);
  if (it != scheduling_create_funtion_map.end()) {
    return (it->second)();
  }

  return nullptr;
}

}  // namespace trpc
