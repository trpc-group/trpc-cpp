//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2024 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#ifdef TRPC_BUILD_INCLUDE_OVERLOAD_CONTROL

#include "trpc/overload_control/smooth_filter/server_request_roll_queue.h"

#include <numeric>

namespace trpc::overload_control {

RequestRollQueue::RequestRollQueue(int num_fps)
    : time_hits_(num_fps + kRedundantFramesNum), isPowOfTwo_((num_fps & (num_fps - 1)) == 0) {
  // First, construct a vec with a specified number of time slots, and then assign a value of 0 to each count
  // Because it is a write, the release memory sequence can be used for assignment, which is lighter in intensity than
  // the default
  for (std::atomic<int64_t>& hit_num : time_hits_) {
    hit_num.store(0, std::memory_order_release);
  }
}

RequestRollQueue::~RequestRollQueue() {}

void RequestRollQueue::NextTimeFrame() {
  // Read in memory sequence issue as above
  //  obtain the starting position of the current window
  // This series of actions is not synchronized
  const int begin = begin_pos_.load(std::memory_order_acquire);

  const int end =
      isPowOfTwo_ ? (begin + WindowSize()) & (time_hits_.size() - 1) : (begin + WindowSize()) % time_hits_.size();

  current_limit_.fetch_sub(time_hits_.at(begin).load(std::memory_order_acquire), std::memory_order_release);
  // Set the last window slot count to empty
  time_hits_.at(end).store(0, std::memory_order_release);

  // Change the starting position of the window
  begin_pos_.store(NextIndex(begin), std::memory_order_release);
}

int64_t RequestRollQueue::AddTimeHit() {
  // Count the last slot by+1 and return the current total count
  current_limit_.fetch_add(1, std::memory_order_release);
  const int begin = begin_pos_.load(std::memory_order_acquire);
  const int current_frame = isPowOfTwo_ ? (begin + WindowSize() - 1) & (time_hits_.size() - 1)
                                        : (begin + WindowSize() - 1) % time_hits_.size();
  time_hits_.at(current_frame).fetch_add(1, std::memory_order_release);

  return ActiveSum();
}

int64_t RequestRollQueue::ActiveSum() const { return current_limit_.load(std::memory_order_acquire); }

}  // namespace trpc::overload_control
#endif