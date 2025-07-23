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

#include "trpc/runtime/threadmodel/fiber/detail/scheduling/scheduling.h"

#include <unordered_map>

#include "trpc/runtime/threadmodel/fiber/detail/scheduling/v1/scheduling_impl.h"
#include "trpc/runtime/threadmodel/fiber/detail/scheduling/v2/scheduling_impl.h"

namespace trpc::fiber::detail {

static uint32_t trpc_fiber_run_queue_size = 65536 * 4;

static std::unordered_map<std::string_view, SchedulingCreateFunction> scheduling_create_function_map;

void SetFiberRunQueueSize(uint32_t queue_size) {
  bool size_is_power_of_2 = (queue_size >= 2) && ((queue_size & (queue_size - 1)) == 0);
  if (!size_is_power_of_2) {
    uint64_t tmp = 1;
    while (tmp <= queue_size) {
      tmp <<= 1;
    }

    queue_size = tmp;
  }
  trpc_fiber_run_queue_size = queue_size;
}

uint32_t GetFiberRunQueueSize() {
  return trpc_fiber_run_queue_size;
}

void InitSchedulingImp() {
  SchedulingCreateFunction func_v1 = []() -> std::unique_ptr<Scheduling> {
    return std::make_unique<v1::SchedulingImpl>();
  };
  RegisterSchedulingImpl(kSchedulingV1, std::move(func_v1));

  SchedulingCreateFunction func_v2 = []() -> std::unique_ptr<Scheduling> {
    return std::make_unique<v2::SchedulingImpl>();
  };
  RegisterSchedulingImpl(kSchedulingV2, std::move(func_v2));
}

bool RegisterSchedulingImpl(std::string_view name, SchedulingCreateFunction&& func) {
  auto it = scheduling_create_function_map.find(name);
  if (it != scheduling_create_function_map.end()) {
    return false;
  }

  scheduling_create_function_map.emplace(std::make_pair(name, std::move(func)));
  return true;
}

std::unique_ptr<Scheduling> CreateScheduling(std::string_view name) {
  auto it = scheduling_create_function_map.find(name);
  if (it != scheduling_create_function_map.end()) {
    return (it->second)();
  }

  return nullptr;
}

}  // namespace trpc::fiber::detail
