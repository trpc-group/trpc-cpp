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

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/common/window.h"

namespace trpc::overload_control {

Window::Window(std::chrono::steady_clock::duration interval, int32_t max_size) { Reset(interval, max_size); }

void Window::Reset(std::chrono::steady_clock::duration interval, int32_t max_size) {
  interval_ = interval;
  max_size_ = max_size;
  start_ = std::chrono::steady_clock::now();
  end_ = start_ + interval_;
  size_ = 0;
}

bool Window::Touch() {
  ++size_;
  return size_ >= max_size_ || std::chrono::steady_clock::now() >= end_;
}

std::chrono::steady_clock::duration Window::GetLastInterval() const {
  return std::chrono::steady_clock::now() - start_;
}

}  // namespace trpc::overload_control

#endif
