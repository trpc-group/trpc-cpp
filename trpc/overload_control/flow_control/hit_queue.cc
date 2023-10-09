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

#include "trpc/overload_control/flow_control/hit_queue.h"

#include <numeric>

namespace trpc::overload_control {

HitQueue::HitQueue(int num_fps) : frame_hits_(num_fps + kRedundantFramesNum) {
  for (std::atomic<int64_t>& hit_num : frame_hits_) {
    hit_num.store(0, std::memory_order_release);
  }
}

HitQueue::~HitQueue() {}

void HitQueue::NextFrame() {
  const int begin = window_begin_.load(std::memory_order_acquire);
  const int end = (begin + WindowSize()) % frame_hits_.size();
  // Reset the count of the position that is currently not in use but will be used soon.
  frame_hits_.at(end).store(0, std::memory_order_release);
  // Move the sliding window down by one position.
  window_begin_.store(NextIndex(begin), std::memory_order_release);
}

int64_t HitQueue::AddHit() {
  const int begin = window_begin_.load(std::memory_order_acquire);
  const int current_frame = (begin + WindowSize() - 1) % frame_hits_.size();
  // Add a valid hit request in the current time slot.
  frame_hits_.at(current_frame).fetch_add(1, std::memory_order_release);
  return ActiveSum();
}

int64_t HitQueue::ActiveSum() const {
  const int begin = window_begin_.load(std::memory_order_acquire);
  const int end = (begin + WindowSize()) % frame_hits_.size();
  int64_t sum = 0;
  for (int i = begin; i != end; i = NextIndex(i)) {
    sum += frame_hits_.at(i).load(std::memory_order_acquire);
  }
  return sum;
}

}  // namespace trpc::overload_control

#endif
