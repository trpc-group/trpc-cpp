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

#include <atomic>

namespace trpc {

/// @brief generate unique id class
class UniqueId {
 public:
  explicit UniqueId(uint32_t begin) : id_(begin) {}
  UniqueId() : id_(0) {}

  /// @brief  generate unique id
  /// @return uint32_t
  uint32_t GenerateId() { return id_.fetch_add(1, std::memory_order_relaxed); }

 private:
  std::atomic_uint32_t id_;
};

}  // namespace trpc
