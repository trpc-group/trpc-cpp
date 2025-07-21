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

namespace trpc {

/// @brief Thread pool initialization parameters
struct ThreadPoolOption {
  std::size_t thread_num = 1;
  std::size_t task_queue_size = 10000;
  bool bind_core = false;
};

}  // namespace trpc
